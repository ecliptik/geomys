/*
 * connection.c - TCP connection management for Gopher browser
 * Adapted from Flynn's connection.c
 */

#include <Quickdraw.h>
#include <Dialogs.h>
#include <TextEdit.h>
#include <Events.h>
#include <Memory.h>
#include <OSUtils.h>
#include <ToolUtils.h>
#include <Resources.h>
#include <Multiverse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include <Fonts.h>

#include "MacTCP.h"
#include "tcp.h"
#include "dns.h"
#include "connection.h"
#include "dialogs.h"
#include "macutil.h"

/* TCP connection state for TIME_WAIT */
#define TCP_STATE_TIME_WAIT 14

/* Receive timeout: 30 seconds (30 * 60 ticks at 60Hz) */
#define CONN_TIMEOUT_TICKS  (30L * 60L)

/* Async-connect timeout: 12 seconds. Sits just above MacTCP's own 10 s
 * ulpTimeout on the ActiveOpen so the stack normally reports the failure
 * itself; this is the backstop for a wedged parameter block whose
 * ioResult never updates. On expiry we retry (see conn_connect_poll). */
#define CONN_CONNECT_TIMEOUT_TICKS  (12L * 60L)

static Boolean tcp_initialized = false;

/* Multi-entry LRU DNS cache with TTL support */
#define DNS_CACHE_SIZE   8
#define DNS_TTL_MIN    300    /* 5 minutes minimum */
#define DNS_TTL_MAX    3600   /* 1 hour maximum */

typedef struct {
	char           host[68];       /* hostname (64 + padding) */
	unsigned long  ip;             /* resolved IP address */
	unsigned long  expire_tick;    /* TickCount when entry expires */
	unsigned long  access_tick;    /* last access for LRU eviction */
} DNSCacheEntry;

static DNSCacheEntry dns_cache[DNS_CACHE_SIZE];

void
dns_cache_init(void)
{
	short i;

	for (i = 0; i < DNS_CACHE_SIZE; i++) {
		dns_cache[i].host[0] = '\0';
		dns_cache[i].ip = 0;
	}
}

unsigned long
dns_cache_lookup(const char *host)
{
	short i;
	unsigned long now = TickCount();

	for (i = 0; i < DNS_CACHE_SIZE; i++) {
		if (dns_cache[i].ip == 0)
			continue;
		if (strcmp(host, dns_cache[i].host) != 0)
			continue;
		/* Check TTL expiry (signed comparison handles wrap) */
		if (dns_cache[i].expire_tick != 0 &&
		    (long)(now - dns_cache[i].expire_tick) > 0) {
			/* Expired — invalidate */
			dns_cache[i].ip = 0;
			dns_cache[i].host[0] = '\0';
			return 0;
		}
		dns_cache[i].access_tick = now;
		return dns_cache[i].ip;
	}
	return 0;
}

void
dns_cache_store(const char *host, unsigned long ip,
    unsigned long ttl_seconds)
{
	short i, lru_idx = 0;
	unsigned long lru_tick = 0xFFFFFFFFUL;
	unsigned long now = TickCount();

	/* Clamp TTL */
	if (ttl_seconds < DNS_TTL_MIN)
		ttl_seconds = DNS_TTL_MIN;
	if (ttl_seconds > DNS_TTL_MAX)
		ttl_seconds = DNS_TTL_MAX;

	/* Look for empty slot, existing entry, or LRU victim */
	for (i = 0; i < DNS_CACHE_SIZE; i++) {
		/* Reuse existing entry for same host */
		if (dns_cache[i].ip != 0 &&
		    strcmp(host, dns_cache[i].host) == 0) {
			lru_idx = i;
			break;
		}
		/* Empty slot */
		if (dns_cache[i].ip == 0) {
			lru_idx = i;
			break;
		}
		/* Track LRU for eviction */
		if (dns_cache[i].access_tick < lru_tick) {
			lru_tick = dns_cache[i].access_tick;
			lru_idx = i;
		}
	}

	strncpy(dns_cache[lru_idx].host, host,
	    sizeof(dns_cache[lru_idx].host) - 1);
	dns_cache[lru_idx].host[sizeof(dns_cache[lru_idx].host) - 1]
	    = '\0';
	dns_cache[lru_idx].ip = ip;
	dns_cache[lru_idx].expire_tick = now + ttl_seconds * 60L;
	dns_cache[lru_idx].access_tick = now;
}

static OSErr
conn_ensure_tcp(void)
{
	OSErr err;
	if (tcp_initialized)
		return noErr;
	err = _TCPInit();
	if (err == noErr)
		tcp_initialized = true;
	return err;
}

void
conn_init_tcp(void)
{
	(void)conn_ensure_tcp();
}

static Boolean
conn_validate_host(const char *host)
{
	short len, i, label_len;
	char c;
	Boolean has_alpha = false;

	len = strlen(host);
	if (len == 0 || len > 63)
		return false;

	label_len = 0;
	for (i = 0; i < len; i++) {
		c = host[i];
		if (c == '.') {
			if (label_len == 0)
				return false;
			label_len = 0;
		} else if ((c >= 'a' && c <= 'z') ||
		    (c >= 'A' && c <= 'Z')) {
			has_alpha = true;
			label_len++;
			if (label_len > 63)
				return false;
		} else if ((c >= '0' && c <= '9') || c == '-') {
			label_len++;
			if (label_len > 63)
				return false;
		} else {
			return false;
		}
	}

	if (label_len == 0)
		return false;

	if (!has_alpha) {
		if (ip2long((char *)host) == 0)
			return false;
	}

	return true;
}

static Boolean
conn_fail(Connection *conn, const char *msg)
{
	Str255 pmsg;

	conn->state = CONN_STATE_ERROR;
	InitCursor();

	c2pstr(pmsg, msg);
	ParamText(pmsg, "\p", "\p", "\p");
	StopAlert(128, 0L);
	return false;
}

static Boolean
conn_resolve_host(Connection *conn, WindowPtr status_win)
{
	unsigned long ip;
	char status_msg[80];

	{
		OSErr tcp_err = conn_ensure_tcp();
		if (tcp_err != noErr)
			return conn_fail(conn,
			    "MacTCP is not available. "
			    "Verify MacTCP is installed in "
			    "your System Folder.");
	}

	if (!conn_validate_host(conn->host)) {
		char msg[180];

		snprintf(msg, sizeof(msg),
		    "Invalid address \xD4%.60s\xD5. "
		    "Check for typos or extra spaces.",
		    conn->host);
		return conn_fail(conn, msg);
	}

	conn->state = CONN_STATE_RESOLVING;
	SetCursor(*GetCursor(watchCursor));

	ip = ip2long(conn->host);
	if (ip != 0) {
		conn->remote_ip = ip;
	} else {
		/* Check multi-entry LRU DNS cache */
		ip = dns_cache_lookup(conn->host);
		if (ip != 0) {
			conn->remote_ip = ip;
		} else {
			unsigned long ttl = 0;

			snprintf(status_msg, sizeof(status_msg),
			    "Resolving %.50s\311", conn->host);
			conn_status_update(status_win, status_msg);

			{
				short dns_err = dns_resolve(
				    conn->host, &ip,
				    conn->dns_server, &ttl);
				switch (dns_err) {
				case DNS_OK:
					conn->remote_ip = ip;
					dns_cache_store(conn->host,
					    ip, ttl);
					break;
				case DNS_ERR_NXDOMAIN: {
					char msg[180];
					snprintf(msg, sizeof(msg),
					    "The server \xD4%.50s\xD5 "
					    "could not be found. "
					    "Check the address for "
					    "typos.",
					    conn->host);
					return conn_fail(conn, msg);
				}
				case DNS_ERR_TIMEOUT: {
					char msg[180];
					snprintf(msg, sizeof(msg),
					    "DNS lookup for "
					    "\xD4%.50s\xD5 timed out. "
					    "Verify your network "
					    "connection.",
					    conn->host);
					return conn_fail(conn, msg);
				}
				default: {
					char msg[180];
					snprintf(msg, sizeof(msg),
					    "DNS lookup for "
					    "\xD4%.50s\xD5 failed. "
					    "Verify your network "
					    "connection.",
					    conn->host);
					return conn_fail(conn, msg);
				}
				}
			}
		}
	}

	return true;
}

/*
 * conn_start_open - allocate buffers, create the stream, and issue an
 * async TCPActiveOpen using the already-resolved conn->remote_ip and
 * conn->port. Shared by the initial connect and the retry path in
 * conn_connect_poll, so it must not touch DNS, host, or retry counters.
 *
 * Returns:
 *    1  async open queued — state is CONN_STATE_OPENING, poll from idle
 *    0  hard failure (out of memory / stream create) — *err_msg set,
 *       state left CONN_STATE_ERROR, all buffers/stream freed
 */
static short
conn_start_open(Connection *conn, const char **err_msg)
{
	OSErr err;

	conn->rcv_buf = NewPtr(TCP_RCV_BUFSIZ);
	if (!conn->rcv_buf) {
		*err_msg = "Not enough memory. Try closing "
		    "other windows or applications.";
		conn->state = CONN_STATE_ERROR;
		return 0;
	}

	conn->read_buf = NewPtr(TCP_READ_BUFSIZ);
	if (!conn->read_buf) {
		DisposePtr(conn->rcv_buf);
		conn->rcv_buf = 0L;
		*err_msg = "Not enough memory. Try closing "
		    "other windows or applications.";
		conn->state = CONN_STATE_ERROR;
		return 0;
	}

	err = _TCPCreate(&conn->pb, &conn->stream, conn->rcv_buf,
	    TCP_RCV_BUFSIZ, 0L, 0L, 0L, false);
	if (err != noErr) {
		DisposePtr(conn->read_buf);
		conn->read_buf = 0L;
		DisposePtr(conn->rcv_buf);
		conn->rcv_buf = 0L;
		conn->state = CONN_STATE_ERROR;
		*err_msg = (err == openErr)
		    ? "Too many open connections. "
		      "Close a window and try again."
		    : "Failed to create TCP stream. "
		      "Verify MacTCP is configured.";
		return 0;
	}

	conn->state = CONN_STATE_CONNECTING;
	conn->local_port = 0;

	err = _TCPActiveOpen(&conn->pb, conn->stream,
	    conn->remote_ip, conn->remote_port,
	    &conn->local_ip, &conn->local_port,
	    0L, 0L, true);  /* async — returns immediately */
	if (err != noErr && err != 1) {
		/* Async call failed to queue — clean up without modal alert.
		 * Treat as a soft failure the caller can retry: leave state
		 * ERROR but report no message so conn_connect_poll retries. */
		_TCPRelease(&conn->pb, conn->stream, 0L, 0L, false);
		DisposePtr(conn->read_buf);
		conn->read_buf = 0L;
		DisposePtr(conn->rcv_buf);
		conn->rcv_buf = 0L;
		conn->stream = 0L;
		conn->state = CONN_STATE_ERROR;
		*err_msg = 0L;
		return 0;
	}

	/* Async open started — poll from idle loop */
	conn->state = CONN_STATE_OPENING;
	conn->connect_start_tick = TickCount();
	return 1;
}

Boolean
conn_connect(Connection *conn, const char *host, short port,
    WindowPtr status_win)
{
	char status_msg[80];
	const char *err_msg = 0L;

	if (conn->state != CONN_STATE_IDLE) {
		SysBeep(10);
		return false;
	}

	if (host != conn->host) {
		strncpy(conn->host, host, sizeof(conn->host) - 1);
		conn->host[sizeof(conn->host) - 1] = '\0';
	}
	conn->port = port;

	if (!conn_resolve_host(conn, status_win))
		return false;

	/* Yield to Process Manager between DNS and TCP open */
	{ EventRecord dummy; WaitNextEvent(0, &dummy, 0, 0L); }

	conn->remote_port = conn->port;

	snprintf(status_msg, sizeof(status_msg),
	    "Connecting to %.50s\311", conn->host);
	conn_status_update(status_win, status_msg);

	conn->connect_retries = 0;

	if (conn_start_open(conn, &err_msg))
		return true;

	/* Failed before the async open could even be queued (out of memory,
	 * stream create, or a rejected PBControl). The poll-driven retry only
	 * covers opens that queued and then failed the handshake, so report
	 * here: the specific message if conn_start_open set one, else a
	 * generic connect error for a rejected queue. */
	if (err_msg)
		return conn_fail(conn, err_msg);
	return conn_fail(conn,
	    "Could not connect to the server. "
	    "Check the address and your network connection.");
}

/*
 * conn_retry_open - tear down the current (failed/stalled) stream and
 * issue a fresh async open to the same already-resolved endpoint, if any
 * retries remain. Returns 1 if a new attempt is now in progress, -1 if
 * retries are exhausted or the re-open failed. On -1 the connection is
 * left in CONN_STATE_ERROR with buffers freed.
 */
static short
conn_retry_open(Connection *conn)
{
	const char *err_msg = 0L;

	if (conn->connect_retries >= CONN_MAX_CONNECT_RETRIES) {
		conn->state = CONN_STATE_ERROR;
		return -1;
	}

	/* conn_close frees the stream/buffers and resets state to IDLE.
	 * It picks the safe pb-teardown path based on conn->pb.ioResult
	 * (separate pb while an open is still queued), so it is correct
	 * whether we got here from a timeout or a reported failure. */
	conn_close(conn);
	conn->connect_retries++;

	if (conn_start_open(conn, &err_msg))
		return 1;   /* fresh attempt in progress */

	conn->state = CONN_STATE_ERROR;
	return -1;
}

short
conn_connect_poll(Connection *conn)
{
	if (conn->state != CONN_STATE_OPENING)
		return -1;

	/* Check async operation status */
	if (conn->pb.ioResult == 1) {
		/* Still in progress — check for timeout. A stalled open
		 * retries on a fresh stream before giving up. */
		if (TickCount() - conn->connect_start_tick >
		    CONN_CONNECT_TIMEOUT_TICKS) {
			short r = conn_retry_open(conn);
			if (r == 1)
				return 1;   /* retrying */
			conn->timed_out = true;
			conn->state = CONN_STATE_ERROR;
			return -1;
		}
		return 1;  /* still connecting */
	}

	if (conn->pb.ioResult != noErr) {
		/* Connection failed — retry on a fresh stream if we can,
		 * otherwise surface the error. The first open to a host
		 * intermittently fails on MacTCP; a clean retry succeeds. */
		return conn_retry_open(conn);
	}

	/* Connected — read back local address from completed pb */
	conn->local_ip = conn->pb.csParam.open.localHost;
	conn->local_port = conn->pb.csParam.open.localPort;

	InitCursor();
	conn->state = CONN_STATE_RECEIVING;
	conn->start_tick = TickCount();
	conn->timed_out = false;
	conn->tw_drain = 0;
	conn->read_len = 0;
	return 0;  /* connected */
}

OSErr
conn_send_selector(Connection *conn, const char *selector)
{
	char buf[260];
	short len;

	if (conn->state != CONN_STATE_RECEIVING)
		return -1;

	len = strlen(selector);
	if (len > 256)
		len = 256;
	memcpy(buf, selector, len);
	buf[len] = '\r';
	buf[len + 1] = '\n';

	return conn_send(conn, buf, len + 2);
}

void
conn_idle(Connection *conn)
{
	struct TCPStatusPB status;
	OSErr err;
	unsigned short len;

	conn->pending_data = 0;

	if (conn->state != CONN_STATE_RECEIVING)
		return;

	if (!conn->read_buf)
		return;

	/* Check for receive timeout */
	if (conn->start_tick &&
	    (TickCount() - conn->start_tick > CONN_TIMEOUT_TICKS)) {
		conn_close(conn);
		conn->timed_out = true;
		conn->state = CONN_STATE_ERROR;
		return;
	}

	err = _TCPStatus(&conn->pb, conn->stream, &status,
	    0L, 0L, false);
	if (err != noErr) {
		conn_close(conn);
		conn->state = CONN_STATE_DONE;
		return;
	}

	conn->pending_data = status.amtUnreadData;

	/* Remote closed — drain remaining data, then mark done.
	 * Don't close immediately when amtUnreadData==0; the
	 * emulated TCP stack may still be delivering data from
	 * the DaynaPORT pipeline.  Wait for 3 consecutive polls
	 * with no data before closing. */
	if (status.connectionState == TCP_STATE_TIME_WAIT) {
		if (status.amtUnreadData == 0) {
			if (++conn->tw_drain >= 3) {
				conn_close(conn);
				conn->state = CONN_STATE_DONE;
				return;
			}
			return;  /* check again next poll */
		}
		conn->tw_drain = 0;  /* data arrived, reset */
	}

	if (status.amtUnreadData == 0)
		return;

	len = status.amtUnreadData;
	if (len > TCP_READ_BUFSIZ)
		len = TCP_READ_BUFSIZ;

	err = _TCPRcv(&conn->pb, conn->stream, (Ptr)conn->read_buf,
	    &len, 0L, 0L, false);
	if (err != noErr) {
		conn_close(conn);
		conn->state = CONN_STATE_DONE;
		return;
	}

	conn->read_len = len;
	conn->start_tick = TickCount();  /* reset timeout on successful read */
	conn->pending_data = (status.amtUnreadData > (unsigned long)len)
	    ? status.amtUnreadData - len : 0;
}

void
conn_close(Connection *conn)
{
	if (conn->state == CONN_STATE_IDLE)
		return;

	if (conn->stream) {
		/*
		 * If an async TCPActiveOpen is still pending, conn->pb is
		 * owned by the Device Manager (ioResult == 1) and sits on
		 * the driver's I/O queue.  _TCPAbort() and _TCPRelease()
		 * both begin by memset()ing the pb they are handed, which
		 * would zero the qLink/ioResult of a live queued block and
		 * re-issue PBControl on it — classic "never reuse a pending
		 * pb" corruption.  Tear the stream down through a separate
		 * parameter block in that case; the synchronous abort forces
		 * the queued open (on conn->pb) to complete before we
		 * release the stream, so conn->pb is safe to reuse
		 * afterwards.
		 */
		if (conn->pb.ioResult == 1) {
			TCPiopb abort_pb;

			_TCPAbort(&abort_pb, conn->stream, 0L, 0L, false);
			_TCPRelease(&abort_pb, conn->stream, 0L, 0L, false);
		} else {
			_TCPAbort(&conn->pb, conn->stream, 0L, 0L, false);
			_TCPRelease(&conn->pb, conn->stream, 0L, 0L, false);
		}
		conn->stream = 0L;
	}

	if (conn->rcv_buf) {
		DisposePtr(conn->rcv_buf);
		conn->rcv_buf = 0L;
	}

	if (conn->read_buf) {
		DisposePtr(conn->read_buf);
		conn->read_buf = 0L;
	}

	conn->read_len = 0;
	conn->start_tick = 0;
	conn->state = CONN_STATE_IDLE;
}

OSErr
conn_send(Connection *conn, char *data, short len)
{
	wdsEntry wds[2];

	if (len <= 0)
		return -1;
	if (conn->state != CONN_STATE_RECEIVING)
		return -1;

	memset(&wds, 0, sizeof(wds));
	wds[0].ptr = (Ptr)data;
	wds[0].length = len;

	return _TCPSend(&conn->pb, conn->stream, wds, 0L, 0L, false);
}

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

static Boolean tcp_initialized = false;

/* Single-entry DNS cache to avoid redundant lookups */
static char dns_cache_host[256];
static unsigned long dns_cache_ip;

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

static Boolean
conn_validate_host(const char *host)
{
	short len, i, label_len;
	char c;
	Boolean has_alpha = false;

	len = strlen(host);
	if (len == 0 || len > 253)
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
			return conn_fail(conn, "MacTCP is not available");
	}

	if (!conn_validate_host(conn->host))
		return conn_fail(conn,
		    "Invalid hostname or IP address");

	conn->state = CONN_STATE_RESOLVING;
	SetCursor(*GetCursor(watchCursor));

	ip = ip2long(conn->host);
	if (ip != 0) {
		conn->remote_ip = ip;
	} else {
		/* Check single-entry DNS cache first */
		if (dns_cache_ip != 0 &&
		    strcmp(conn->host, dns_cache_host) == 0) {
			conn->remote_ip = dns_cache_ip;
		} else {
			snprintf(status_msg, sizeof(status_msg),
			    "Resolving %.50s\311", conn->host);
			conn_status_update(status_win, status_msg);

			{
				short dns_err = dns_resolve(
				    conn->host, &ip,
				    conn->dns_server);
				switch (dns_err) {
				case DNS_OK:
					conn->remote_ip = ip;
					strncpy(dns_cache_host,
					    conn->host,
					    sizeof(dns_cache_host) - 1);
					dns_cache_host[
					    sizeof(dns_cache_host) - 1]
					    = '\0';
					dns_cache_ip = ip;
					break;
				case DNS_ERR_NXDOMAIN:
					return conn_fail(conn,
					    "Host not found");
				case DNS_ERR_TIMEOUT:
					return conn_fail(conn,
					    "DNS lookup timed out");
				default:
					return conn_fail(conn,
					    "DNS lookup failed");
				}
			}
		}
	}

	return true;
}

Boolean
conn_connect(Connection *conn, const char *host, short port,
    WindowPtr status_win)
{
	OSErr err;
	char status_msg[80];

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

	conn->rcv_buf = NewPtr(TCP_RCV_BUFSIZ);
	if (!conn->rcv_buf)
		return conn_fail(conn, "Out of memory");

	err = _TCPCreate(&conn->pb, &conn->stream, conn->rcv_buf,
	    TCP_RCV_BUFSIZ, 0L, 0L, 0L, false);
	if (err != noErr) {
		DisposePtr(conn->rcv_buf);
		conn->rcv_buf = 0L;
		return conn_fail(conn,
		    "Failed to create TCP stream");
	}

	conn->state = CONN_STATE_CONNECTING;
	conn->local_port = 0;

	err = _TCPActiveOpen(&conn->pb, conn->stream,
	    conn->remote_ip, conn->remote_port,
	    &conn->local_ip, &conn->local_port,
	    0L, 0L, false);
	if (err != noErr) {
		_TCPRelease(&conn->pb, conn->stream, 0L, 0L, false);
		DisposePtr(conn->rcv_buf);
		conn->rcv_buf = 0L;
		conn->stream = 0L;
		return conn_fail(conn, "Failed to connect");
	}

	InitCursor();
	conn->state = CONN_STATE_RECEIVING;
	conn->read_len = 0;
	return true;
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

	err = _TCPStatus(&conn->pb, conn->stream, &status,
	    0L, 0L, false);
	if (err != noErr) {
		conn_close(conn);
		conn->state = CONN_STATE_DONE;
		return;
	}

	conn->pending_data = status.amtUnreadData;

	/* Remote closed — drain remaining data, then mark done */
	if (status.connectionState == TCP_STATE_TIME_WAIT) {
		if (status.amtUnreadData == 0) {
			conn_close(conn);
			conn->state = CONN_STATE_DONE;
			return;
		}
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
	conn->pending_data = (status.amtUnreadData > (unsigned long)len)
	    ? status.amtUnreadData - len : 0;
}

void
conn_close(Connection *conn)
{
	if (conn->state == CONN_STATE_IDLE)
		return;

	if (conn->stream) {
		_TCPAbort(&conn->pb, conn->stream, 0L, 0L, false);
		_TCPRelease(&conn->pb, conn->stream, 0L, 0L, false);
		conn->stream = 0L;
	}

	if (conn->rcv_buf) {
		DisposePtr(conn->rcv_buf);
		conn->rcv_buf = 0L;
	}

	conn->read_len = 0;
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

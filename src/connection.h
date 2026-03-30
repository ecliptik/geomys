/*
 * connection.h - TCP connection management for Gopher browser
 * Adapted from Flynn's connection.h
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "MacTCP.h"
#include "tcp.h"

/* Connection states */
#define CONN_STATE_IDLE        0
#define CONN_STATE_RESOLVING   1
#define CONN_STATE_CONNECTING  2
#define CONN_STATE_SENDING     3
#define CONN_STATE_RECEIVING   4
#define CONN_STATE_DONE        5
#define CONN_STATE_ERROR       6
#define CONN_STATE_CLOSING     7
#define CONN_STATE_OPENING     8  /* async TCPActiveOpen in progress */

/* TCP buffer sizes */
#define TCP_RCV_BUFSIZ   8192
#define TCP_READ_BUFSIZ  8192

/* Gopher default port */
#define GOPHER_DEFAULT_PORT  70

typedef struct {
	short       state;
	StreamPtr   stream;
	TCPiopb     pb;
	ip_addr     remote_ip;
	tcp_port    remote_port;
	ip_addr     local_ip;
	tcp_port    local_port;
	Ptr         rcv_buf;
	char        *read_buf;
	short       read_len;
	unsigned long pending_data;
	unsigned long start_tick;   /* TickCount at receive start for timeout */
	unsigned long connect_start_tick;  /* TickCount at async open start */
	Boolean     timed_out;     /* set when receive timeout fires */
	short       tw_drain;      /* TIME_WAIT drain countdown */
	char        host[68];
	short       port;
	ip_addr     dns_server;
} Connection;

/* Initialize MacTCP driver — call at startup for faster first connect */
void conn_init_tcp(void);

/* DNS LRU cache — 8-entry cache with TTL support */
void dns_cache_init(void);
unsigned long dns_cache_lookup(const char *host);
void dns_cache_store(const char *host, unsigned long ip,
    unsigned long ttl_seconds);

/* Connect to host:port, showing progress in status_win */
Boolean conn_connect(Connection *conn, const char *host, short port,
    WindowPtr status_win);

/* Poll async connect — returns 1=in progress, 0=connected, -1=error */
short conn_connect_poll(Connection *conn);

/* Send selector string + CRLF to initiate Gopher request */
OSErr conn_send_selector(Connection *conn, const char *selector);

/* Poll for incoming data — call from event loop */
void conn_idle(Connection *conn);

/* Close connection and release resources */
void conn_close(Connection *conn);

/* Send raw data */
OSErr conn_send(Connection *conn, char *data, short len);

#endif /* CONNECTION_H */

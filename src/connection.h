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

/* TCP buffer sizes */
#define TCP_RCV_BUFSIZ   8192
#define TCP_READ_BUFSIZ  4096

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
	char        read_buf[TCP_READ_BUFSIZ];
	short       read_len;
	unsigned long pending_data;
	unsigned long start_tick;   /* TickCount at receive start for timeout */
	Boolean     timed_out;     /* set when receive timeout fires */
	char        host[256];
	short       port;
	ip_addr     dns_server;
} Connection;

/* Connect to host:port, showing progress in status_win */
Boolean conn_connect(Connection *conn, const char *host, short port,
    WindowPtr status_win);

/* Send selector string + CRLF to initiate Gopher request */
OSErr conn_send_selector(Connection *conn, const char *selector);

/* Poll for incoming data — call from event loop */
void conn_idle(Connection *conn);

/* Close connection and release resources */
void conn_close(Connection *conn);

/* Send raw data */
OSErr conn_send(Connection *conn, char *data, short len);

#endif /* CONNECTION_H */

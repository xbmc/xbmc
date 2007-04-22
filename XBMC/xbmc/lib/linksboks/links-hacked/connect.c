/* connect.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

#ifdef HAVE_SSL
#include <openssl/err.h>
#endif

/*
#define LOG_TRANSFER	"/tmp/log"
*/

#ifdef LOG_TRANSFER
void log_data(unsigned char *data, int len)
{
	static hlaseno = 0;
	int fd;
	if (!hlaseno) {
		printf("\n\e[1mWARNING -- LOGGING NETWORK TRANSFERS !!!\e[0m%c\n", 7);
		fflush(stdout);
		sleep(1);
		hlaseno = 1;
	}
	if ((fd = open(LOG_TRANSFER, O_WRONLY | O_APPEND | O_CREAT | O_BINARY, 0622)) != -1) {
		set_bin(fd);
		write(fd, data, len);
		close(fd);
	}
}

#else
#define log_data(x, y)
#endif

void exception(struct connection *c)
{
	setcstate(c, S_EXCEPT);
	retry_connection(c);
}

void close_socket(int *s)
{
	if (*s == -1) return;
	close(*s);
	set_handlers(*s, NULL, NULL, NULL, NULL);
	*s = -1;
}

void connected(struct connection *);

struct conn_info {
	void (*func)(struct connection *);
	struct sockaddr_in sa;
	ip__address addr;
	int port;
	int *sock;
};

void dns_found(struct connection *, int);

void make_connection(struct connection *c, int port, int *sock, void (*func)(struct connection *))
{
	int as;
	unsigned char *host;
	struct conn_info *b;
	if (!(host = get_host_name(c->url))) {
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	if (!(b = mem_alloc(sizeof(struct conn_info)))) {
		mem_free(host);
		setcstate(c, S_OUT_OF_MEM);
		retry_connection(c);
		return;
	}
	b->func = func;
	b->sock = sock;
	b->port = port;
	c->buffer = b;
	log_data("\nCONNECTION: ", 13);
	log_data(host, strlen(host));
	log_data("\n", 1);
	if (c->no_cache >= NC_RELOAD) as = find_host_no_cache(host, &b->addr, &c->dnsquery, (void(*)(void *, int))dns_found, c);
	else as = find_host(host, &b->addr, &c->dnsquery, (void(*)(void *, int))dns_found, c);
	mem_free(host);
	if (as) setcstate(c, S_DNS);
}

int get_pasv_socket(struct connection *c, int cc, int *sock, unsigned char *port)
{
	int s;
	struct sockaddr_in sa;
	struct sockaddr_in sb;
	int len = sizeof(sa);
	memset(&sa, 0, sizeof sa);
	memset(&sb, 0, sizeof sb);
	if (getsockname(cc, (struct sockaddr *)&sa, &len)) {
		e:
		setcstate(c, -errno);
		retry_connection(c);
		return -2;
	}
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) goto e;
	*sock = s;
	fcntl(s, F_SETFL, O_NONBLOCK);
	memcpy(&sb, &sa, sizeof(struct sockaddr_in));
	sb.sin_port = 0;
	if (bind(s, (struct sockaddr *)&sb, sizeof sb)) goto e;
	len = sizeof(sa);
	if (getsockname(s, (struct sockaddr *)&sa, &len)) goto e;
	if (listen(s, 1)) goto e;
	memcpy(port, &sa.sin_addr.s_addr, 4);
	memcpy(port + 4, &sa.sin_port, 2);
#if defined(IP_TOS) && defined(IPTOS_THROUGHPUT)
	{
		int on = IPTOS_THROUGHPUT;
		setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int));
	}
#endif
	return 0;
}

#ifdef HAVE_SSL
void ssl_want_read(struct connection *c)
{
	struct conn_info *b = c->buffer;

	if (c->no_tsl) c->ssl->options |= SSL_OP_NO_TLSv1;
	switch (SSL_get_error(c->ssl, SSL_connect(c->ssl))) {
		case SSL_ERROR_NONE:
			c->buffer = NULL;
			b->func(c);
			mem_free(b);
		case SSL_ERROR_WANT_READ:
			break;
		default:
			c->no_tsl++;
			setcstate(c, S_SSL_ERROR);
			retry_connection(c);
	}
}
#endif

void dns_found(struct connection *c, int state)
{
	int s;
	struct conn_info *b = c->buffer;
	OutputDebugString("\nbegin dns_found....");
	if (state) {
		setcstate(c, S_NO_DNS);
		retry_connection(c);
		return;
	}
	OutputDebugString("socket...");
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		setcstate(c, -errno);
		retry_connection(c);
		return;
	}
	*b->sock = s;
	fcntl(s, F_SETFL, O_NONBLOCK);
	memset(&b->sa, 0, sizeof(struct sockaddr_in));
	b->sa.sin_family = PF_INET;
	b->sa.sin_addr.s_addr = b->addr;
	b->sa.sin_port = htons(b->port);
	OutputDebugString("connect...");
	if (connect(s, (struct sockaddr *)&b->sa, sizeof b->sa)) {
#ifndef __XBOX__
		if (errno != EALREADY && errno != EINPROGRESS) {
			setcstate(c, -errno);
#else
		if (WSAGetLastError() != WSAEALREADY && WSAGetLastError() != WSAEINPROGRESS && WSAGetLastError() != WSAEWOULDBLOCK) {
			setcstate(c, -WSAGetLastError());
#endif
			retry_connection(c);
			return;
		}
		set_handlers(s, NULL, (void(*)(void *))connected, (void(*)(void *))exception, c);
		setcstate(c, S_CONN);
	} else {
#ifdef HAVE_SSL
		OutputDebugString("connected...");
		if(c->ssl) {
			BIO *errbio = BIO_new(BIO_s_mem());
			BUF_MEM *bufmem;
			OutputDebugString("SSL...");

			c->ssl = getSSL();
#ifdef __XBOX__
			SSL_set_fd(c->ssl,xbox_get_socket(s));
#else
			SSL_set_fd(c->ssl,s);
#endif
			if (c->no_tsl) c->ssl->options |= SSL_OP_NO_TLSv1;
			switch (SSL_get_error(c->ssl, SSL_connect(c->ssl))) {
				case SSL_ERROR_WANT_READ:
					setcstate(c, S_SSL_NEG);
					set_handlers(s, (void(*)(void *))ssl_want_read, NULL, (void(*)(void *))exception, c);
					return;
				case SSL_ERROR_NONE:
					break;
				default:
					ERR_print_errors(errbio);
					bufmem = (BUF_MEM *)errbio->ptr;
					OutputDebugString(bufmem->data);
					c->no_tsl++;
					setcstate(c, S_SSL_ERROR);
					retry_connection(c);
					return;
			}
			OutputDebugString("SSLed...");
		}
#endif
		c->buffer = NULL;
		OutputDebugString("func...");
		b->func(c);
		OutputDebugString("end dns_found!!\n");
		mem_free(b);
	}
}

void connected(struct connection *c)
{
	struct conn_info *b = c->buffer;
	void (*func)(struct connection *) = b->func;
	int err = 0;
	int len = sizeof(int);
	/*if (!connect(*b->sock, (struct sockaddr *)&b->sa, sizeof b->sa) || errno == EISCONN) {
		mem_free(b);
		func(c);
		return;
	}
	mem_free(b);
	setcstate(c, -errno);*/

#ifndef __XBOX__	/* Will have to check this otherwise maybe */
	if (getsockopt(*b->sock, SOL_SOCKET, SO_ERROR, (void *)&err, &len))
		if (!(err = errno)) {
			err = -(S_STATE);
			goto bla;
		}
	if (err >= 10000) err -= 10000;	/* Why does EMX return so large values? */
	bla:
#endif
	if (err > 0) setcstate(c, -err), retry_connection(c);
	else {
#ifdef HAVE_SSL
		if(c->ssl) {
			c->ssl = getSSL();
#ifdef __XBOX__
			SSL_set_fd(c->ssl,xbox_get_socket(*b->sock));
#else
			SSL_set_fd(c->ssl,*b->sock);
#endif
			if (c->no_tsl) c->ssl->options |= SSL_OP_NO_TLSv1;
			switch (SSL_get_error(c->ssl, SSL_connect(c->ssl))) {
				case SSL_ERROR_WANT_READ:
					setcstate(c, S_SSL_NEG);
					set_handlers(*b->sock, (void(*)(void *))ssl_want_read, NULL, (void(*)(void *))exception, c);
					return;
				case SSL_ERROR_NONE:
					break;
				default:
					c->no_tsl++;
					setcstate(c, S_SSL_ERROR);
					retry_connection(c);
					return;
			}
		}
#endif
		c->buffer = NULL;
		mem_free(b);
		func(c);
	}
}

struct write_buffer {
	int sock;
	int len;
	int pos;
	void (*done)(struct connection *);
	unsigned char data[1];
};

void write_select(struct connection *c)
{
	struct write_buffer *wb;
	int wr;
	if (!(wb = c->buffer)) {
		internal("write socket has no buffer");
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	/*printf("ws: %d\n",wb->len-wb->pos);
	for (wr = wb->pos; wr < wb->len; wr++) printf("%c", wb->data[wr]);
	printf("-\n");*/

#ifdef HAVE_SSL
	if(c->ssl) {
		if ((wr = SSL_write(c->ssl, wb->data + wb->pos, wb->len - wb->pos)) <= 0) {
			int err;
			if ((err = SSL_get_error(c->ssl, wr)) != SSL_ERROR_WANT_WRITE) {
				setcstate(c, wr ? (err == SSL_ERROR_SYSCALL ? -errno : S_SSL_ERROR) : S_CANT_WRITE);
				if (!wr || err == SSL_ERROR_SYSCALL) retry_connection(c);
				else abort_connection(c);
				return;
			}
			else return;
		}
	} else
#endif
		if ((wr = write(wb->sock, wb->data + wb->pos, wb->len - wb->pos)) <= 0) {
			setcstate(c, wr ? -errno : S_CANT_WRITE);
			retry_connection(c);
			return;
		}

	/*printf("wr: %d\n", wr);*/
	if ((wb->pos += wr) == wb->len) {
		void (*f)(struct connection *) = wb->done;
		c->buffer = NULL;
		set_handlers(wb->sock, NULL, NULL, NULL, NULL);
		mem_free(wb);
		f(c);
	}
}

void write_to_socket(struct connection *c, int s, unsigned char *data, int len, void (*write_func)(struct connection *))
{
	struct write_buffer *wb;
	log_data(data, len);
	if (!(wb = mem_alloc(sizeof(struct write_buffer) + len))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	wb->sock = s;
	wb->len = len;
	wb->pos = 0;
	wb->done = write_func;
	memcpy(wb->data, data, len);
	if (c->buffer) mem_free(c->buffer);
	c->buffer = wb;
	set_handlers(s, NULL, (void (*)())write_select, (void (*)())exception, c);
}

#define READ_SIZE 16384

void read_select(struct connection *c)
{
	struct read_buffer *rb;
	int rd;
	if (!(rb = c->buffer)) {
		internal("read socket has no buffer");
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	set_handlers(rb->sock, NULL, NULL, NULL, NULL);
	if (!(rb = mem_realloc(rb, sizeof(struct read_buffer) + rb->len + READ_SIZE))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return;
	}
	c->buffer = rb;

#ifdef HAVE_SSL
	if(c->ssl) {
		if ((rd = SSL_read(c->ssl, rb->data + rb->len, READ_SIZE)) <= 0) {
			int err;
			if ((err = SSL_get_error(c->ssl, rd)) == SSL_ERROR_WANT_READ) {
				read_from_socket(c, rb->sock, rb, rb->done);
				return;
			}
			if (rb->close && !rd) {
				rb->close = 2;
				rb->done(c, rb);
				return;
			}
			setcstate(c, rd ? (err == SSL_ERROR_SYSCALL ? -errno : S_SSL_ERROR) : S_CANT_READ);
			/*mem_free(rb);*/
			if (!rd || err == SSL_ERROR_SYSCALL) retry_connection(c);
			else abort_connection(c);
			return;
		}
	} else
#endif
		if ((rd = read(rb->sock, rb->data + rb->len, READ_SIZE)) <= 0) {
			if (rb->close && !rd) {
				rb->close = 2;
				rb->done(c, rb);
				return;
			}
			setcstate(c, rd ? -errno : S_CANT_READ);
			/*mem_free(rb);*/
			retry_connection(c);
			return;
		}
	log_data(rb->data + rb->len, rd);
	rb->len += rd;
	rb->done(c, rb);
}

struct read_buffer *alloc_read_buffer(struct connection *c)
{
	struct read_buffer *rb;
	if (!(rb = mem_alloc(sizeof(struct read_buffer) + READ_SIZE))) {
		setcstate(c, S_OUT_OF_MEM);
		abort_connection(c);
		return NULL;
	}
	memset(rb, 0, sizeof(struct read_buffer));
	return rb;
}

void read_from_socket(struct connection *c, int s, struct read_buffer *buf, void (*read_func)(struct connection *, struct read_buffer *))
{
	buf->done = read_func;
	buf->sock = s;
	if (c->buffer && buf != c->buffer) mem_free(c->buffer);
	c->buffer = buf;
	set_handlers(s, (void (*)())read_select, NULL, (void (*)())exception, c);
}

void kill_buffer_data(struct read_buffer *rb, int n)
{
	if (n > rb->len || n < 0) {
		internal("called kill_buffer_data with bad value");
		rb->len = 0;
		return;
	}
	memmove(rb->data, rb->data + n, rb->len - n);
	rb->len -= n;
}

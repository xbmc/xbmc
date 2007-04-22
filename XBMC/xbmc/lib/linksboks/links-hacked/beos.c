/* beos.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL
 */

#ifdef __BEOS__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <be/kernel/OS.h>

#define SHS	128

#ifndef MAXINT
#define MAXINT 0x7fffffff
#endif

int be_read(int s, void *ptr, int len)
{
	if (s >= SHS) return recv(s - SHS, ptr, len, 0);
	return read(s, ptr, len);
}

int be_write(int s, void *ptr, int len)
{
	if (s >= SHS) return send(s - SHS, ptr, len, 0);
	return write(s, ptr, len);
}

int be_close(int s)
{
	if (s >= SHS) return closesocket(s - SHS);
	return close(s);
}

int be_socket(int af, int sock, int prot)
{
	int h = socket(af, sock, prot);
	if (h < 0) return h;
	return h + SHS;
}

int  be_connect(int s, struct sockaddr *sa, int sal)
{
	return connect(s - SHS, sa, sal);
}

int be_getpeername(int s, struct sockaddr *sa, int *sal)
{
	return getpeername(s - SHS, sa, sal);
}

int be_getsockname(int s, struct sockaddr *sa, int *sal)
{
	return getsockname(s - SHS, sa, sal);
}

int be_listen(int s, int c)
{
	return listen(s - SHS, c);
}

int be_accept(int s, struct sockaddr *sa, int *sal)
{
	int a = accept(s - SHS, sa, sal);
	if (a < 0) return -1;
	return a + SHS;
}

int be_bind(int s, struct sockaddr *sa, int sal)
{
	/*struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	if (!sin->sin_port) {
		int i;
		for (i = 16384; i < 49152; i++) {
			sin->sin_port = i;
			if (!be_bind(s, sa, sal)) return 0;
		}
		return -1;
	}*/
	if (bind(s - SHS, sa, sal)) return -1;
	getsockname(s - SHS, sa, &sal);
	return 0;
}

#define PIPE_RETRIES	10

int be_pipe(int *fd)
{
	int s1, s2, s3, l;
	struct sockaddr_in sa1, sa2;
	int retry_count = 0;
	again:
	if ((s1 = be_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		/*perror("socket1");*/
		goto fatal_retry;
	}
	if ((s2 = be_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		/*perror("socket2");*/
		be_close(s1);
		goto fatal_retry;
	}
	memset(&sa1, 0, sizeof(sa1));
	sa1.sin_family = AF_INET;
	sa1.sin_port = 0;
	sa1.sin_addr.s_addr = INADDR_ANY;
	if (be_bind(s1, (struct sockaddr *)&sa1, sizeof(sa1))) {
		/*perror("bind");*/
		clo:
		be_close(s1);
		be_close(s2);
		goto fatal_retry;
	}
	if (be_listen(s1, 1)) {
		/*perror("listen");*/
		goto clo;
	}
	if (be_connect(s2, (struct sockaddr *)&sa1, sizeof(sa1))) {
		/*perror("connect");*/
		goto clo;
	}
	l = sizeof(sa2);
	if ((s3 = be_accept(s1, (struct sockaddr *)&sa2, &l)) < 0) {
		/*perror("accept");*/
		goto clo;
	}
	be_getsockname(s3, (struct sockaddr *)&sa1, &l);
	if (sa1.sin_addr.s_addr != sa2.sin_addr.s_addr) {
		be_close(s3);
		goto clo;
	}
	be_close(s1);
	fd[0] = s2;
	fd[1] = s3;
	return 0;

	fatal_retry:
	if (++retry_count > PIPE_RETRIES) return -1;
	sleep(1);
	goto again;
}

int be_select(int n, struct fd_set *rd, struct fd_set *wr, struct fd_set *exc, struct timeval *tm)
{
	int i, s;
	struct fd_set d, rrd;
	FD_ZERO(&d);
	if (!rd) rd = &d;
	if (!wr) wr = &d;
	if (!exc) exc = &d;
	if (n >= FDSETSIZE) n = FDSETSIZE;
	FD_ZERO(exc);
	for (i = 0; i < n; i++) if ((i < SHS && FD_ISSET(i, rd)) || FD_ISSET(i, wr)) {
		for (i = SHS; i < n; i++) FD_CLR(i, rd);
		return MAXINT;
	}
	FD_ZERO(&rrd);
	for (i = SHS; i < n; i++) if (FD_ISSET(i, rd)) FD_SET(i - SHS, &rrd);
	if ((s = select(FD_SETSIZE, &rrd, &d, &d, tm)) < 0) {
		FD_ZERO(rd);
		return 0;
	}
	FD_ZERO(rd);
	for (i = SHS; i < n; i++) if (FD_ISSET(i - SHS, &rrd)) FD_SET(i, rd);
	return s;
}

#ifndef SO_ERROR
#define SO_ERROR	10001
#endif

int be_getsockopt(int s, int level, int optname, void *optval, int *optlen)
{
	if (optname == SO_ERROR && *optlen >= sizeof(int)) {
		*(int *)optval = 0;
		*optlen = sizeof(int);
		return 0;
	}
	return -1;
}

int start_thr(void (*)(void *), void *, unsigned char *);

int ihpipe[2];

int inth;

#include <errno.h>

void input_handle_th(void *p)
{
	char c;
	int b = 0;
	setsockopt(ihpipe[1], SOL_SOCKET, SO_NONBLOCK, &b, sizeof(b));
	while (1) if (read(0, &c, 1) == 1) be_write(ihpipe[1], &c, 1);
}

int get_input_handle()
{
	static int h = -1;
	if (h >= 0) return h;
	if (be_pipe(ihpipe) < 0) return -1;
	if ((inth = start_thr(input_handle_th, NULL, "input_thread")) < 0) {
		closesocket(ihpipe[0]);
		closesocket(ihpipe[1]);
		return -1;
	}
	return h = ihpipe[0];
}

void block_stdin()
{
	suspend_thread(inth);
}

void unblock_stdin()
{
	resume_thread(inth);
}

/*int ohpipe[2];

#define O_BUF	16384

void output_handle_th(void *p)
{
	char *c = malloc(O_BUF);
	int r, b = 0;
	if (!c) return;
	setsockopt(ohpipe[1], SOL_SOCKET, SO_NONBLOCK, &b, sizeof(b));
	while ((r = be_read(ohpipe[0], c, O_BUF)) > 0) write(1, c, r);
	free(c);
}

int get_output_handle()
{
	static int h = -1;
	if (h >= 0) return h;
	if (be_pipe(ohpipe) < 0) return -1;
	if (start_thr(output_handle_th, NULL, "output_thread") < 0) {
		closesocket(ohpipe[0]);
		closesocket(ohpipe[1]);
		return -1;
	}
	return h = ohpipe[1];
}*/

#endif

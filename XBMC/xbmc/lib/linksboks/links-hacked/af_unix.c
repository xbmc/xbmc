/* af_unix.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL
 */

#include "links.h"

#ifdef DONT_USE_AF_UNIX

int bind_to_af_unix()
{
	return -1;
}

void af_unix_close()
{
}

#else

#ifdef USE_AF_UNIX
#include <sys/un.h>
#endif

void af_unix_connection(void *);

struct sockaddr *s_unix_acc = NULL;
struct sockaddr *s_unix = NULL;
int s_unix_l;
int s_unix_fd = -1;

struct g_connection
{
	int fd;
	unsigned char *queue;
	int qlen;
};

void destroy_g_out(struct g_connection *g_con);
void destroy_g_in(struct g_connection *g_con);
void in_g_con(struct g_connection *g_con);
void out_g_con(struct g_connection *g_con);
void init_g_out(int fd, unsigned char *url);


#ifdef USE_AF_UNIX

int get_address()
{
	struct sockaddr_un *su;
	unsigned char *path;
	if (!links_home) return -1;
	path = stracpy(links_home);
	if (!(su = mem_alloc(sizeof(struct sockaddr_un) + strlen(path) + 1))) {
		mem_free(path);
		return -1;
	}
	if (!(s_unix_acc = mem_alloc(sizeof(struct sockaddr_un) + strlen(path) + 1))) {
		mem_free(su);
		mem_free(path);
		return -1;
	}
	memset(su, 0, sizeof(struct sockaddr_un) + strlen(path) + 1);
	su->sun_family = AF_UNIX;
	add_to_strn(&path, ggr ? LINKS_G_SOCK_NAME : LINKS_SOCK_NAME);
	strcpy(su->sun_path, path);
	mem_free(path);
	s_unix = (struct sockaddr *)su;
	s_unix_l = (char *)&su->sun_path - (char *)su + strlen(su->sun_path) + 1;
	return AF_UNIX;
}

void unlink_unix()
{
	if (unlink(((struct sockaddr_un *)s_unix)->sun_path)) {
		/*perror("unlink");
		debug("unlink: %s", ((struct sockaddr_un *)s_unix)->sun_path);*/
	}
}

#else

int get_address()
{
	struct sockaddr_in *sin;
	if (!(sin = mem_alloc(sizeof(struct sockaddr_in)))) return -1;
	if (!(s_unix_acc = mem_alloc(sizeof(struct sockaddr_in)))) {
		mem_free(sin);
		return -1;
	}
	memset(sin, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(LINKS_PORT);
#ifdef __XBOX__
	sin->sin_addr.s_addr = 0;
#else
	sin->sin_addr.s_addr = htonl(0x7f000001);
#endif
	s_unix = (struct sockaddr *)sin;
	s_unix_l = sizeof(struct sockaddr_in);
	return AF_INET;
}

void unlink_unix()
{
}

#endif

int bind_to_af_unix()
{
	int u = 0;
	int a1 = 1;
	int cnt = 0;
	int af;

        if ((af = get_address(ggr)) == -1) return -1;
	again:
	if ((s_unix_fd = socket(af, SOCK_STREAM, 0)) == -1) return -1;
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR) && !defined(__XBOX__)
	setsockopt(s_unix_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&a1, sizeof a1);
#endif
	if (bind(s_unix_fd, s_unix, s_unix_l)) {
		/*perror("");
		debug("bind: %d", errno);*/
		close(s_unix_fd);
		if ((s_unix_fd = socket(af, SOCK_STREAM, 0)) == -1) return -1;
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR)
		setsockopt(s_unix_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&a1, sizeof a1);
#endif
		if (connect(s_unix_fd, s_unix, s_unix_l)) {
			/*perror("");
			debug("connect: %d", errno);*/
			if (++cnt < MAX_BIND_TRIES) {
				struct timeval tv = { 0, 100000 };
				fd_set dummy;
				FD_ZERO(&dummy);
				select(0, &dummy, &dummy, &dummy, &tv);
				close(s_unix_fd);
				goto again;
			}
			close(s_unix_fd), s_unix_fd = -1;
			if (!u) {
				unlink_unix();
				u = 1;
				goto again;
			}
			mem_free(s_unix), s_unix = NULL;
			return -1;
		}
		mem_free(s_unix), s_unix = NULL;
		return s_unix_fd;
	}
	if (listen(s_unix_fd, 100)) {
		error("ERROR: listen failed: %d", errno);
		mem_free(s_unix), s_unix = NULL;
		close(s_unix_fd), s_unix_fd = -1;
		return -1;
	}
        set_handlers(s_unix_fd,
                     af_unix_connection,
                     NULL,
                     NULL,
                     NULL);

        return -1;
}

void af_unix_connection(void *xxx)
{
	int l = s_unix_l;
	int ns;

        memset(s_unix_acc, 0, l);
	ns = accept(s_unix_fd, (struct sockaddr *)s_unix_acc, &l);

        if(!ggr){

                init_term(ns, ns, win_func);
                set_highpri();

        } else {

                struct g_connection *g_con = mem_calloc(sizeof (struct g_connection));

                fcntl(ns, F_SETFD, FD_CLOEXEC);
                fcntl(ns, F_SETFL, O_NONBLOCK);
                if (!g_con) {
                        close(ns);
                        return;
                }
                g_con->fd = ns;
                g_con->queue = mem_alloc(sizeof(unsigned char));
                *(g_con->queue) = 0;
                g_con->qlen = 0;
                set_handlers(ns, (void (*)(void *))in_g_con, NULL,
                             (void (*)(void *))destroy_g_in, g_con);
        }
 /*BOOK0000010x000bb*/}

void af_unix_close()
{
        if (s_unix_fd != -1) close(s_unix_fd);
	if (s_unix) unlink_unix(), mem_free(s_unix), s_unix = NULL;
	if (s_unix_acc) mem_free(s_unix_acc), s_unix_acc = NULL;
}

void destroy_g_out(struct g_connection *g_con)
{ 
	af_unix_close();
	terminate_loop = 1;
	set_handlers(g_con->fd, NULL, NULL, NULL, NULL);
	if (g_con->queue) mem_free(g_con->queue);
	mem_free(g_con);
}
    
void destroy_g_in(struct g_connection *g_con)
{
	close(g_con->fd);
	set_handlers(g_con->fd, NULL, NULL, NULL, NULL);
        {
                int *i;
                unsigned char *url;

                i = (int *)g_con->queue;
                if ( (g_con->qlen<2*sizeof(int)) || (g_con->qlen != i[0]) ) return;
                url = (unsigned char *) (i+2);

                
                /* New window request? */
                if(!i[1]){
                        int len;
                        void *info=create_session_info(0, url, &len, NULL);

                        if (info)
                                attach_g_terminal(info, len);
                }
                else {
                        /* Reuse existing window */
                        struct session *s=(struct session*)&sessions;
                        goto_url(s, url);
                }
        }

        mem_free(g_con->queue);
	mem_free(g_con);
}

void out_g_con(struct g_connection *g_con)
{
        int r;

        if (g_con->qlen == 0) {
             destroy_g_out(g_con);
             return;
        }     
top:
	if ((r = write(g_con->fd, g_con->queue, g_con->qlen)) <= 0) {
//		if (r == -1 && errno != ECONNRESET) error("ERROR: error %d on terminal: could not read event", errno);
                if (r == -1 && errno == EAGAIN)
                     return;
                if (r == -1 && errno == EINTR)
                     goto top;
		destroy_g_out(g_con);
		return;
	}
        g_con->qlen -= r;
        memmove(g_con->queue, g_con->queue+r, g_con->qlen);
}
       
void in_g_con(struct g_connection *g_con)
{
	int r;
	unsigned char *iq, *end;
        
        if (!(iq = mem_realloc(g_con->queue, g_con->qlen + ALLOC_GR))) {
		destroy_g_in(g_con);
		return;
	}
	g_con->queue = iq;

top:
	if ((r = read(g_con->fd, iq + g_con->qlen, ALLOC_GR-1)) <= 0) {
//		if (r == -1 && errno != ECONNRESET) error("ERROR: error %d on terminal: could not read event", errno);
                if (r == -1 && errno == EAGAIN)
			return;
		if (r == -1 && errno == EINTR)
			goto top;
		destroy_g_in(g_con);
		return;
	}
	g_con->qlen += r;
	g_con->queue[g_con->qlen] = 0;    
} 

void init_g_out(int fd, unsigned char *u)
{
	struct g_connection *g_con; 
	int l;
	int *i;

	if ( !(g_con = mem_calloc(sizeof (struct g_connection))))
	{
                terminate_loop = 1;
		af_unix_close();        
		return;
	}
        g_con->fd = fd;

	l  = strlen(u);
	g_con->qlen = 2*sizeof(int)+l;
	if ( !(i = mem_alloc(g_con->qlen)) ) {
		destroy_g_out(g_con);
		return;
	}
        
	i[0] = g_con->qlen;
        i[1] = reuse_window;
	memcpy(i+2,  u, l);
	g_con->queue = (unsigned char *)i;
	
	fcntl(fd, F_SETFL, O_NONBLOCK);
	set_handlers(fd, NULL, (void (*)(void *))out_g_con,
			(void (*)(void *))destroy_g_out, g_con);                            
}		

#endif
 /*BOOK0000011x000bb*/
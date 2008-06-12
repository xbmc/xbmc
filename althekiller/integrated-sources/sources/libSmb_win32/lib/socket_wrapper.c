/* 
   Socket wrapper library. Passes all socket communication over 
   unix domain sockets if the environment variable SOCKET_WRAPPER_DIR 
   is set.
   Copyright (C) Jelmer Vernooij 2005
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

#ifdef _PUBLIC_
#undef _PUBLIC_
#endif
#define _PUBLIC_

#ifdef SOCKET_WRAPPER_REPLACE
#undef accept
#undef connect
#undef bind
#undef getpeername
#undef getsockname
#undef getsockopt
#undef setsockopt
#undef recvfrom
#undef sendto
#undef socket
#undef close
#endif

/* LD_PRELOAD doesn't work yet, so REWRITE_CALLS is all we support
 * for now */
#define REWRITE_CALLS 

#ifdef REWRITE_CALLS
#define real_accept accept
#define real_connect connect
#define real_bind bind
#define real_getpeername getpeername
#define real_getsockname getsockname
#define real_getsockopt getsockopt
#define real_setsockopt setsockopt
#define real_recvfrom recvfrom
#define real_sendto sendto
#define real_socket socket
#define real_close close
#endif

#undef malloc
#undef calloc
#undef strdup
/* we need to use a very terse format here as IRIX 6.4 silently
   truncates names to 16 chars, so if we use a longer name then we
   can't tell which port a packet came from with recvfrom() 
   
   with this format we have 8 chars left for the directory name
*/
#define SOCKET_FORMAT "%c%02X%04X"
#define SOCKET_TYPE_CHAR_TCP		'T'
#define SOCKET_TYPE_CHAR_UDP		'U'

static struct sockaddr *sockaddr_dup(const void *data, socklen_t len)
{
	struct sockaddr *ret = (struct sockaddr *)malloc(len);
	memcpy(ret, data, len);
	return ret;
}

struct socket_info
{
	int fd;

	int domain;
	int type;
	int protocol;
	int bound;
	int bcast;

	char *path;
	char *tmp_path;

	struct sockaddr *myname;
	socklen_t myname_len;

	struct sockaddr *peername;
	socklen_t peername_len;

	struct socket_info *prev, *next;
};

static struct socket_info *sockets = NULL;


static const char *socket_wrapper_dir(void)
{
	const char *s = getenv("SOCKET_WRAPPER_DIR");
	if (s == NULL) {
		return NULL;
	}
	if (strncmp(s, "./", 2) == 0) {
		s += 2;
	}
	return s;
}

static unsigned int socket_wrapper_default_iface(void)
{
	const char *s = getenv("SOCKET_WRAPPER_DEFAULT_IFACE");
	if (s) {
		unsigned int iface;
		if (sscanf(s, "%u", &iface) == 1) {
			if (iface >= 1 && iface <= 0xFF) {
				return iface;
			}
		}
	}

	return 1;/* 127.0.0.1 */
}

static int convert_un_in(const struct sockaddr_un *un, struct sockaddr_in *in, socklen_t *len)
{
	unsigned int iface;
	unsigned int prt;
	const char *p;
	char type;

	if ((*len) < sizeof(struct sockaddr_in)) {
		return 0;
	}

	p = strrchr(un->sun_path, '/');
	if (p) p++; else p = un->sun_path;

	if (sscanf(p, SOCKET_FORMAT, &type, &iface, &prt) != 3) {
		errno = EINVAL;
		return -1;
	}

	if (type != SOCKET_TYPE_CHAR_TCP && type != SOCKET_TYPE_CHAR_UDP) {
		errno = EINVAL;
		return -1;
	}

	if (iface == 0 || iface > 0xFF) {
		errno = EINVAL;
		return -1;
	}

	if (prt > 0xFFFF) {
		errno = EINVAL;
		return -1;
	}

	in->sin_family = AF_INET;
	in->sin_addr.s_addr = htonl((127<<24) | iface);
	in->sin_port = htons(prt);

	*len = sizeof(struct sockaddr_in);
	return 0;
}

static int convert_in_un_remote(struct socket_info *si, const struct sockaddr_in *in, struct sockaddr_un *un,
				int *bcast)
{
	char u_type = '\0';
	char b_type = '\0';
	char a_type = '\0';
	char type = '\0';
	unsigned int addr= ntohl(in->sin_addr.s_addr);
	unsigned int prt = ntohs(in->sin_port);
	unsigned int iface;
	int is_bcast = 0;

	if (bcast) *bcast = 0;

	if (prt == 0) {
		errno = EINVAL;
		return -1;
	}

	switch (si->type) {
	case SOCK_STREAM:
		u_type = SOCKET_TYPE_CHAR_TCP;
		break;
	case SOCK_DGRAM:
		u_type = SOCKET_TYPE_CHAR_UDP;
		a_type = SOCKET_TYPE_CHAR_UDP;
		b_type = SOCKET_TYPE_CHAR_UDP;
		break;
	}

	if (a_type && addr == 0xFFFFFFFF) {
		/* 255.255.255.255 only udp */
		is_bcast = 2;
		type = a_type;
		iface = socket_wrapper_default_iface();
	} else if (b_type && addr == 0x7FFFFFFF) {
		/* 127.255.255.255 only udp */
		is_bcast = 1;
		type = b_type;
		iface = socket_wrapper_default_iface();
	} else if ((addr & 0xFFFFFF00) == 0x7F000000) {
		/* 127.0.0.X */
		is_bcast = 0;
		type = u_type;
		iface = (addr & 0x000000FF);
	} else {
		errno = ENETUNREACH;
		return -1;
	}

	if (bcast) *bcast = is_bcast;

	if (is_bcast) {
		snprintf(un->sun_path, sizeof(un->sun_path), "%s/EINVAL", 
			 socket_wrapper_dir());
		/* the caller need to do more processing */
		return 0;
	}

	snprintf(un->sun_path, sizeof(un->sun_path), "%s/"SOCKET_FORMAT, 
		 socket_wrapper_dir(), type, iface, prt);

	return 0;
}

static int convert_in_un_alloc(struct socket_info *si, const struct sockaddr_in *in, struct sockaddr_un *un,
			       int *bcast)
{
	char u_type = '\0';
	char d_type = '\0';
	char b_type = '\0';
	char a_type = '\0';
	char type = '\0';
	unsigned int addr= ntohl(in->sin_addr.s_addr);
	unsigned int prt = ntohs(in->sin_port);
	unsigned int iface;
	struct stat st;
	int is_bcast = 0;

	if (bcast) *bcast = 0;

	switch (si->type) {
	case SOCK_STREAM:
		u_type = SOCKET_TYPE_CHAR_TCP;
		d_type = SOCKET_TYPE_CHAR_TCP;
		break;
	case SOCK_DGRAM:
		u_type = SOCKET_TYPE_CHAR_UDP;
		d_type = SOCKET_TYPE_CHAR_UDP;
		a_type = SOCKET_TYPE_CHAR_UDP;
		b_type = SOCKET_TYPE_CHAR_UDP;
		break;
	}

	if (addr == 0) {
		/* 0.0.0.0 */
		is_bcast = 0;
		type = d_type;
		iface = socket_wrapper_default_iface();
	} else if (a_type && addr == 0xFFFFFFFF) {
		/* 255.255.255.255 only udp */
		is_bcast = 2;
		type = a_type;
		iface = socket_wrapper_default_iface();
	} else if (b_type && addr == 0x7FFFFFFF) {
		/* 127.255.255.255 only udp */
		is_bcast = 1;
		type = b_type;
		iface = socket_wrapper_default_iface();
	} else if ((addr & 0xFFFFFF00) == 0x7F000000) {
		/* 127.0.0.X */
		is_bcast = 0;
		type = u_type;
		iface = (addr & 0x000000FF);
	} else {
		errno = EADDRNOTAVAIL;
		return -1;
	}

	if (bcast) *bcast = is_bcast;

	if (prt == 0) {
		/* handle auto-allocation of ephemeral ports */
		for (prt = 5001; prt < 10000; prt++) {
			snprintf(un->sun_path, sizeof(un->sun_path), "%s/"SOCKET_FORMAT, 
				 socket_wrapper_dir(), type, iface, prt);
			if (stat(un->sun_path, &st) == 0) continue;

			((struct sockaddr_in *)si->myname)->sin_port = htons(prt);
			return 0;
		}
		errno = ENFILE;
		return -1;
	}

	snprintf(un->sun_path, sizeof(un->sun_path), "%s/"SOCKET_FORMAT, 
		 socket_wrapper_dir(), type, iface, prt);
	return 0;
}

static struct socket_info *find_socket_info(int fd)
{
	struct socket_info *i;
	for (i = sockets; i; i = i->next) {
		if (i->fd == fd) 
			return i;
	}

	return NULL;
}

static int sockaddr_convert_to_un(struct socket_info *si, const struct sockaddr *in_addr, socklen_t in_len, 
				  struct sockaddr_un *out_addr, int alloc_sock, int *bcast)
{
	if (!out_addr)
		return 0;

	out_addr->sun_family = AF_UNIX;

	switch (in_addr->sa_family) {
	case AF_INET:
		switch (si->type) {
		case SOCK_STREAM:
		case SOCK_DGRAM:
			break;
		default:
			errno = ESOCKTNOSUPPORT;
			return -1;
		}
		if (alloc_sock) {
			return convert_in_un_alloc(si, (const struct sockaddr_in *)in_addr, out_addr, bcast);
		} else {
			return convert_in_un_remote(si, (const struct sockaddr_in *)in_addr, out_addr, bcast);
		}
	case AF_UNIX:
		memcpy(out_addr, in_addr, sizeof(*out_addr));
		return 0;
	default:
		break;
	}
	
	errno = EAFNOSUPPORT;
	return -1;
}

static int sockaddr_convert_from_un(const struct socket_info *si, 
				    const struct sockaddr_un *in_addr, 
				    socklen_t un_addrlen,
				    int family,
				    struct sockaddr *out_addr,
				    socklen_t *_out_addrlen)
{
	socklen_t out_addrlen;

	if (out_addr == NULL || _out_addrlen == NULL) 
		return 0;

	if (un_addrlen == 0) {
		*_out_addrlen = 0;
		return 0;
	}

	out_addrlen = *_out_addrlen;
	if (out_addrlen > un_addrlen) {
		out_addrlen = un_addrlen;
	}

	switch (family) {
	case AF_INET:
		switch (si->type) {
		case SOCK_STREAM:
		case SOCK_DGRAM:
			break;
		default:
			errno = ESOCKTNOSUPPORT;
			return -1;
		}
		return convert_un_in(in_addr, (struct sockaddr_in *)out_addr, _out_addrlen);
	case AF_UNIX:
		memcpy(out_addr, in_addr, out_addrlen);
		*_out_addrlen = out_addrlen;
		return 0;
	default:
		break;
	}

	errno = EAFNOSUPPORT;
	return -1;
}

_PUBLIC_ int swrap_socket(int domain, int type, int protocol)
{
	struct socket_info *si;
	int fd;

	if (!socket_wrapper_dir()) {
		return real_socket(domain, type, protocol);
	}
	
	fd = real_socket(AF_UNIX, type, 0);

	if (fd == -1) return -1;

	si = calloc(1, sizeof(struct socket_info));

	si->domain = domain;
	si->type = type;
	si->protocol = protocol;
	si->fd = fd;

	DLIST_ADD(sockets, si);

	return si->fd;
}

_PUBLIC_ int swrap_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	struct socket_info *parent_si, *child_si;
	int fd;
	struct sockaddr_un un_addr;
	socklen_t un_addrlen = sizeof(un_addr);
	struct sockaddr_un un_my_addr;
	socklen_t un_my_addrlen = sizeof(un_my_addr);
	struct sockaddr my_addr;
	socklen_t my_addrlen = sizeof(my_addr);
	int ret;

	parent_si = find_socket_info(s);
	if (!parent_si) {
		return real_accept(s, addr, addrlen);
	}

	memset(&un_addr, 0, sizeof(un_addr));
	memset(&un_my_addr, 0, sizeof(un_my_addr));
	memset(&my_addr, 0, sizeof(my_addr));

	ret = real_accept(s, (struct sockaddr *)&un_addr, &un_addrlen);
	if (ret == -1) return ret;

	fd = ret;

	ret = sockaddr_convert_from_un(parent_si, &un_addr, un_addrlen,
				       parent_si->domain, addr, addrlen);
	if (ret == -1) return ret;

	child_si = malloc(sizeof(struct socket_info));
	memset(child_si, 0, sizeof(*child_si));

	child_si->fd = fd;
	child_si->domain = parent_si->domain;
	child_si->type = parent_si->type;
	child_si->protocol = parent_si->protocol;
	child_si->bound = 1;

	ret = real_getsockname(fd, &un_my_addr, &un_my_addrlen);
	if (ret == -1) return ret;

	ret = sockaddr_convert_from_un(child_si, &un_my_addr, un_my_addrlen,
				       child_si->domain, &my_addr, &my_addrlen);
	if (ret == -1) return ret;

	child_si->myname_len = my_addrlen;
	child_si->myname = sockaddr_dup(&my_addr, my_addrlen);

	child_si->peername_len = *addrlen;
	child_si->peername = sockaddr_dup(addr, *addrlen);

	DLIST_ADD(sockets, child_si);

	return fd;
}

/* using sendto() or connect() on an unbound socket would give the
   recipient no way to reply, as unlike UDP and TCP, a unix domain
   socket can't auto-assign emphemeral port numbers, so we need to
   assign it here */
static int swrap_auto_bind(struct socket_info *si)
{
	struct sockaddr_un un_addr;
	struct sockaddr_in in;
	int i;
	char type;
	int ret;
	struct stat st;
	
	un_addr.sun_family = AF_UNIX;

	switch (si->type) {
	case SOCK_STREAM:
		type = SOCKET_TYPE_CHAR_TCP;
		break;
	case SOCK_DGRAM:
		type = SOCKET_TYPE_CHAR_UDP;
		break;
	default:
		errno = ESOCKTNOSUPPORT;
		return -1;
	}
	
	for (i=0;i<1000;i++) {
		snprintf(un_addr.sun_path, sizeof(un_addr.sun_path), 
			 "%s/"SOCKET_FORMAT, socket_wrapper_dir(),
			 type, socket_wrapper_default_iface(), i + 10000);
		if (stat(un_addr.sun_path, &st) == 0) continue;
		
		ret = real_bind(si->fd, (struct sockaddr *)&un_addr, sizeof(un_addr));
		if (ret == -1) return ret;

		si->tmp_path = strdup(un_addr.sun_path);
		si->bound = 1;
		break;
	}
	if (i == 1000) {
		errno = ENFILE;
		return -1;
	}
	
	memset(&in, 0, sizeof(in));
	in.sin_family = AF_INET;
	in.sin_port   = htons(i);
	in.sin_addr.s_addr = htonl(127<<24 | socket_wrapper_default_iface());
	
	si->myname_len = sizeof(in);
	si->myname = sockaddr_dup(&in, si->myname_len);
	si->bound = 1;
	return 0;
}


_PUBLIC_ int swrap_connect(int s, const struct sockaddr *serv_addr, socklen_t addrlen)
{
	int ret;
	struct sockaddr_un un_addr;
	struct socket_info *si = find_socket_info(s);

	if (!si) {
		return real_connect(s, serv_addr, addrlen);
	}

	if (si->bound == 0 && si->domain != AF_UNIX) {
		ret = swrap_auto_bind(si);
		if (ret == -1) return -1;
	}

	ret = sockaddr_convert_to_un(si, (const struct sockaddr *)serv_addr, addrlen, &un_addr, 0, NULL);
	if (ret == -1) return -1;

	ret = real_connect(s, (struct sockaddr *)&un_addr, 
			   sizeof(struct sockaddr_un));

	/* to give better errors */
	if (serv_addr->sa_family == AF_INET) {
		if (ret == -1 && errno == ENOENT) {
			errno = EHOSTUNREACH;
		}
	}

	if (ret == 0) {
		si->peername_len = addrlen;
		si->peername = sockaddr_dup(serv_addr, addrlen);
	}

	return ret;
}

_PUBLIC_ int swrap_bind(int s, const struct sockaddr *myaddr, socklen_t addrlen)
{
	int ret;
	struct sockaddr_un un_addr;
	struct socket_info *si = find_socket_info(s);

	if (!si) {
		return real_bind(s, myaddr, addrlen);
	}

	si->myname_len = addrlen;
	si->myname = sockaddr_dup(myaddr, addrlen);

	ret = sockaddr_convert_to_un(si, (const struct sockaddr *)myaddr, addrlen, &un_addr, 1, &si->bcast);
	if (ret == -1) return -1;

	unlink(un_addr.sun_path);

	ret = real_bind(s, (struct sockaddr *)&un_addr,
			sizeof(struct sockaddr_un));

	if (ret == 0) {
		si->bound = 1;
	}

	return ret;
}

_PUBLIC_ int swrap_getpeername(int s, struct sockaddr *name, socklen_t *addrlen)
{
	struct socket_info *si = find_socket_info(s);

	if (!si) {
		return real_getpeername(s, name, addrlen);
	}

	if (!si->peername) 
	{
		errno = ENOTCONN;
		return -1;
	}

	memcpy(name, si->peername, si->peername_len);
	*addrlen = si->peername_len;

	return 0;
}

_PUBLIC_ int swrap_getsockname(int s, struct sockaddr *name, socklen_t *addrlen)
{
	struct socket_info *si = find_socket_info(s);

	if (!si) {
		return real_getsockname(s, name, addrlen);
	}

	memcpy(name, si->myname, si->myname_len);
	*addrlen = si->myname_len;

	return 0;
}

_PUBLIC_ int swrap_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
	struct socket_info *si = find_socket_info(s);

	if (!si) {
		return real_getsockopt(s, level, optname, optval, optlen);
	}

	if (level == SOL_SOCKET) {
		return real_getsockopt(s, level, optname, optval, optlen);
	} 

	switch (si->domain) {
	case AF_UNIX:
		return real_getsockopt(s, level, optname, optval, optlen);
	default:
		errno = ENOPROTOOPT;
		return -1;
	}
}

_PUBLIC_ int swrap_setsockopt(int s, int  level,  int  optname,  const  void  *optval, socklen_t optlen)
{
	struct socket_info *si = find_socket_info(s);

	if (!si) {
		return real_setsockopt(s, level, optname, optval, optlen);
	}

	if (level == SOL_SOCKET) {
		return real_setsockopt(s, level, optname, optval, optlen);
	}

	switch (si->domain) {
	case AF_UNIX:
		return real_setsockopt(s, level, optname, optval, optlen);
	case AF_INET:
		/* Silence some warnings */
#ifdef TCP_NODELAY
		if (optname == TCP_NODELAY) 
			return 0;
#endif
	default:
		errno = ENOPROTOOPT;
		return -1;
	}
}

_PUBLIC_ ssize_t swrap_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	struct sockaddr_un un_addr;
	socklen_t un_addrlen = sizeof(un_addr);
	int ret;
	struct socket_info *si = find_socket_info(s);

	if (!si) {
		return real_recvfrom(s, buf, len, flags, from, fromlen);
	}

	/* irix 6.4 forgets to null terminate the sun_path string :-( */
	memset(&un_addr, 0, sizeof(un_addr));
	ret = real_recvfrom(s, buf, len, flags, (struct sockaddr *)&un_addr, &un_addrlen);
	if (ret == -1) 
		return ret;

	if (sockaddr_convert_from_un(si, &un_addr, un_addrlen,
				     si->domain, from, fromlen) == -1) {
		return -1;
	}
	
	return ret;
}


_PUBLIC_ ssize_t swrap_sendto(int  s,  const  void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
	struct sockaddr_un un_addr;
	int ret;
	struct socket_info *si = find_socket_info(s);
	int bcast = 0;

	if (!si) {
		return real_sendto(s, buf, len, flags, to, tolen);
	}

	if (si->bound == 0 && si->domain != AF_UNIX) {
		ret = swrap_auto_bind(si);
		if (ret == -1) return -1;
	}

	ret = sockaddr_convert_to_un(si, to, tolen, &un_addr, 0, &bcast);
	if (ret == -1) return -1;

	if (bcast) {
		struct stat st;
		unsigned int iface;
		unsigned int prt = ntohs(((const struct sockaddr_in *)to)->sin_port);
		char type;

		type = SOCKET_TYPE_CHAR_UDP;

		for(iface=0; iface <= 0xFF; iface++) {
			snprintf(un_addr.sun_path, sizeof(un_addr.sun_path), "%s/"SOCKET_FORMAT, 
				 socket_wrapper_dir(), type, iface, prt);
			if (stat(un_addr.sun_path, &st) != 0) continue;

			/* ignore the any errors in broadcast sends */
			real_sendto(s, buf, len, flags, (struct sockaddr *)&un_addr, sizeof(un_addr));
		}
		return len;
	}

	ret = real_sendto(s, buf, len, flags, (struct sockaddr *)&un_addr, sizeof(un_addr));

	/* to give better errors */
	if (to->sa_family == AF_INET) {
		if (ret == -1 && errno == ENOENT) {
			errno = EHOSTUNREACH;
		}
	}

	return ret;
}

_PUBLIC_ int swrap_close(int fd)
{
	struct socket_info *si = find_socket_info(fd);

	if (si) {
		DLIST_REMOVE(sockets, si);

		free(si->path);
		free(si->myname);
		free(si->peername);
		if (si->tmp_path) {
			unlink(si->tmp_path);
			free(si->tmp_path);
		}
		free(si);
	}

	return real_close(fd);
}

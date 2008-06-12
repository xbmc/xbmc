/* 
   Unix SMB/CIFS implementation.
   Samba utility functions
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Tim Potter      2000-2001
   Copyright (C) Jeremy Allison  1992-2005
   
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

#ifdef _XBOX
#define close(a) closesocket(a)
#endif

/* the following 3 client_*() functions are nasty ways of allowing
   some generic functions to get info that really should be hidden in
   particular modules */
static int client_fd = -1;
/* What to print out on a client disconnect error. */
static char client_ip_string[16];

void client_setfd(int fd)
{
	client_fd = fd;
	safe_strcpy(client_ip_string, get_peer_addr(client_fd), sizeof(client_ip_string)-1);
}

static char *get_socket_addr(int fd)
{
	struct sockaddr sa;
	struct sockaddr_in *sockin = (struct sockaddr_in *) (&sa);
	socklen_t length = sizeof(sa);
	static fstring addr_buf;

	fstrcpy(addr_buf,"0.0.0.0");

	if (fd == -1) {
		return addr_buf;
	}
	
	if (getsockname(fd, &sa, &length) < 0) {
		DEBUG(0,("getsockname failed. Error was %s\n", strerror(errno) ));
		return addr_buf;
	}
	
	fstrcpy(addr_buf,(char *)inet_ntoa(sockin->sin_addr));
	
	return addr_buf;
}

static int get_socket_port(int fd)
{
	struct sockaddr sa;
	struct sockaddr_in *sockin = (struct sockaddr_in *) (&sa);
	socklen_t length = sizeof(sa);

	if (fd == -1)
		return -1;
	
	if (getsockname(fd, &sa, &length) < 0) {
		DEBUG(0,("getpeername failed. Error was %s\n", strerror(errno) ));
		return -1;
	}
	
	return ntohs(sockin->sin_port);
}

char *client_name(void)
{
	return get_peer_name(client_fd,False);
}

char *client_addr(void)
{
	return get_peer_addr(client_fd);
}

char *client_socket_addr(void)
{
	return get_socket_addr(client_fd);
}

int client_socket_port(void)
{
	return get_socket_port(client_fd);
}

struct in_addr *client_inaddr(struct sockaddr *sa)
{
	struct sockaddr_in *sockin = (struct sockaddr_in *) (sa);
	socklen_t  length = sizeof(*sa);
	
	if (getpeername(client_fd, sa, &length) < 0) {
		DEBUG(0,("getpeername failed. Error was %s\n", strerror(errno) ));
		return NULL;
	}
	
	return &sockin->sin_addr;
}

/* the last IP received from */
struct in_addr lastip;

/* the last port received from */
int lastport=0;

int smb_read_error = 0;

/****************************************************************************
 Determine if a file descriptor is in fact a socket.
****************************************************************************/

BOOL is_a_socket(int fd)
{
	int v;
	socklen_t l;
	l = sizeof(int);
	return(getsockopt(fd, SOL_SOCKET, SO_TYPE, (char *)&v, &l) == 0);
}

enum SOCK_OPT_TYPES {OPT_BOOL,OPT_INT,OPT_ON};

typedef struct smb_socket_option {
	const char *name;
	int level;
	int option;
	int value;
	int opttype;
} smb_socket_option;

static const smb_socket_option socket_options[] = {
  {"SO_KEEPALIVE",      SOL_SOCKET,    SO_KEEPALIVE,    0,                 OPT_BOOL},
  {"SO_REUSEADDR",      SOL_SOCKET,    SO_REUSEADDR,    0,                 OPT_BOOL},
  {"SO_BROADCAST",      SOL_SOCKET,    SO_BROADCAST,    0,                 OPT_BOOL},
#ifdef TCP_NODELAY
  {"TCP_NODELAY",       IPPROTO_TCP,   TCP_NODELAY,     0,                 OPT_BOOL},
#endif
#ifdef TCP_KEEPCNT
  {"TCP_KEEPCNT",       IPPROTO_TCP,   TCP_KEEPCNT,     0,                 OPT_INT},
#endif
#ifdef TCP_KEEPIDLE
  {"TCP_KEEPIDLE",      IPPROTO_TCP,   TCP_KEEPIDLE,    0,                 OPT_INT},
#endif
#ifdef TCP_KEEPINTVL
  {"TCP_KEEPINTVL",     IPPROTO_TCP,   TCP_KEEPINTVL,   0,                 OPT_INT},
#endif
#ifdef IPTOS_LOWDELAY
  {"IPTOS_LOWDELAY",    IPPROTO_IP,    IP_TOS,          IPTOS_LOWDELAY,    OPT_ON},
#endif
#ifdef IPTOS_THROUGHPUT
  {"IPTOS_THROUGHPUT",  IPPROTO_IP,    IP_TOS,          IPTOS_THROUGHPUT,  OPT_ON},
#endif
#ifdef SO_REUSEPORT
  {"SO_REUSEPORT",      SOL_SOCKET,    SO_REUSEPORT,    0,                 OPT_BOOL},
#endif
#ifdef SO_SNDBUF
  {"SO_SNDBUF",         SOL_SOCKET,    SO_SNDBUF,       0,                 OPT_INT},
#endif
#ifdef SO_RCVBUF
  {"SO_RCVBUF",         SOL_SOCKET,    SO_RCVBUF,       0,                 OPT_INT},
#endif
#ifdef SO_SNDLOWAT
  {"SO_SNDLOWAT",       SOL_SOCKET,    SO_SNDLOWAT,     0,                 OPT_INT},
#endif
#ifdef SO_RCVLOWAT
  {"SO_RCVLOWAT",       SOL_SOCKET,    SO_RCVLOWAT,     0,                 OPT_INT},
#endif
#ifdef SO_SNDTIMEO
  {"SO_SNDTIMEO",       SOL_SOCKET,    SO_SNDTIMEO,     0,                 OPT_INT},
#endif
#ifdef SO_RCVTIMEO
  {"SO_RCVTIMEO",       SOL_SOCKET,    SO_RCVTIMEO,     0,                 OPT_INT},
#endif
#ifdef TCP_FASTACK
  {"TCP_FASTACK",       IPPROTO_TCP,   TCP_FASTACK,     0,                 OPT_INT},
#endif
  {NULL,0,0,0,0}};

/****************************************************************************
 Print socket options.
****************************************************************************/

static void print_socket_options(int s)
{
	int value;
	socklen_t vlen = 4;
	const smb_socket_option *p = &socket_options[0];

	/* wrapped in if statement to prevent streams leak in SCO Openserver 5.0 */
	/* reported on samba-technical  --jerry */
	if ( DEBUGLEVEL >= 5 ) {
	for (; p->name != NULL; p++) {
		if (getsockopt(s, p->level, p->option, (void *)&value, &vlen) == -1) {
			DEBUG(5,("Could not test socket option %s.\n", p->name));
		} else {
			DEBUG(5,("socket option %s = %d\n",p->name,value));
			}
		}
	}
 }

/****************************************************************************
 Set user socket options.
****************************************************************************/

void set_socket_options(int fd, const char *options)
{
	fstring tok;

	while (next_token(&options,tok," \t,", sizeof(tok))) {
		int ret=0,i;
		int value = 1;
		char *p;
		BOOL got_value = False;

		if ((p = strchr_m(tok,'='))) {
			*p = 0;
			value = atoi(p+1);
			got_value = True;
		}

		for (i=0;socket_options[i].name;i++)
			if (strequal(socket_options[i].name,tok))
				break;

		if (!socket_options[i].name) {
			DEBUG(0,("Unknown socket option %s\n",tok));
			continue;
		}

		switch (socket_options[i].opttype) {
		case OPT_BOOL:
		case OPT_INT:
			ret = setsockopt(fd,socket_options[i].level,
						socket_options[i].option,(char *)&value,sizeof(int));
			break;

		case OPT_ON:
			if (got_value)
				DEBUG(0,("syntax error - %s does not take a value\n",tok));

			{
				int on = socket_options[i].value;
				ret = setsockopt(fd,socket_options[i].level,
							socket_options[i].option,(char *)&on,sizeof(int));
			}
			break;	  
		}
      
		if (ret != 0)
			DEBUG(0,("Failed to set socket option %s (Error %s)\n",tok, strerror(errno) ));
	}

	print_socket_options(fd);
}

/****************************************************************************
 Read from a socket.
****************************************************************************/

ssize_t read_udp_socket(int fd,char *buf,size_t len)
{
	ssize_t ret;
	struct sockaddr_in sock;
	socklen_t socklen = sizeof(sock);

	memset((char *)&sock,'\0',socklen);
	memset((char *)&lastip,'\0',sizeof(lastip));
	ret = (ssize_t)sys_recvfrom(fd,buf,len,0,(struct sockaddr *)&sock,&socklen);
	if (ret <= 0) {
		/* Don't print a low debug error for a non-blocking socket. */
		if (errno == EAGAIN) {
			DEBUG(10,("read socket returned EAGAIN. ERRNO=%s\n",strerror(errno)));
		} else {
			DEBUG(2,("read socket failed. ERRNO=%s\n",strerror(errno)));
		}
		return(0);
	}

	lastip = sock.sin_addr;
	lastport = ntohs(sock.sin_port);

	DEBUG(10,("read_udp_socket: lastip %s lastport %d read: %lu\n",
			inet_ntoa(lastip), lastport, (unsigned long)ret));

	return(ret);
}

#if 0

Socket routines from HEAD - maybe re-enable in future. JRA.

/****************************************************************************
 Work out if we've timed out.
****************************************************************************/

static BOOL timeout_until(struct timeval *timeout, const struct timeval *endtime)
{
	struct timeval now;
	SMB_BIG_INT t_dif;

	GetTimeOfDay(&now);

	t_dif = usec_time_diff(endtime, &now);
	if (t_dif <= 0) {
		return False;
	}

	timeout->tv_sec = (t_dif / (SMB_BIG_INT)1000000);
	timeout->tv_usec = (t_dif % (SMB_BIG_INT)1000000);
	return True;
}

/****************************************************************************
 Read data from the client, reading exactly N bytes, or until endtime timeout.
 Use with a non-blocking socket if endtime != NULL.
****************************************************************************/

ssize_t read_data_until(int fd,char *buffer,size_t N, const struct timeval *endtime)
{
	ssize_t ret;
	size_t total=0;

	smb_read_error = 0;

	while (total < N) {

		if (endtime != NULL) {
			fd_set r_fds;
			struct timeval timeout;
			int selrtn;

			if (!timeout_until(&timeout, endtime)) {
				DEBUG(10,("read_data_until: read timed out\n"));
				smb_read_error = READ_TIMEOUT;
				return -1;
			}

			FD_ZERO(&r_fds);
			FD_SET(fd, &r_fds);

			/* Select but ignore EINTR. */
			selrtn = sys_select_intr(fd+1, &r_fds, NULL, NULL, &timeout);
			if (selrtn == -1) {
				/* something is wrong. Maybe the socket is dead? */
				DEBUG(0,("read_data_until: select error = %s.\n", strerror(errno) ));
				smb_read_error = READ_ERROR;
				return -1;
			}

			/* Did we timeout ? */
			if (selrtn == 0) {
				DEBUG(10,("read_data_until: select timed out.\n"));
				smb_read_error = READ_TIMEOUT;
				return -1;
			}
		}

		ret = sys_read(fd,buffer + total,N - total);

		if (ret == 0) {
			DEBUG(10,("read_data_until: read of %d returned 0. Error = %s\n", (int)(N - total), strerror(errno) ));
			smb_read_error = READ_EOF;
			return 0;
		}

		if (ret == -1) {
			if (errno == EAGAIN) {
				/* Non-blocking socket with no data available. Try select again. */
				continue;
			}
			DEBUG(0,("read_data_until: read failure for %d. Error = %s\n", (int)(N - total), strerror(errno) ));
			smb_read_error = READ_ERROR;
			return -1;
		}
		total += ret;
	}
	return (ssize_t)total;
}
#endif

/****************************************************************************
 Read data from a socket with a timout in msec.
 mincount = if timeout, minimum to read before returning
 maxcount = number to be read.
 time_out = timeout in milliseconds
****************************************************************************/

ssize_t read_socket_with_timeout(int fd,char *buf,size_t mincnt,size_t maxcnt,unsigned int time_out)
{
	fd_set fds;
	int selrtn;
	ssize_t readret;
	size_t nread = 0;
	struct timeval timeout;
	
	/* just checking .... */
	if (maxcnt <= 0)
		return(0);
	
	smb_read_error = 0;
	
	/* Blocking read */
	if (time_out == 0) {
		if (mincnt == 0) {
			mincnt = maxcnt;
		}
		
		while (nread < mincnt) {
			readret = sys_read(fd, buf + nread, maxcnt - nread);
			
			if (readret == 0) {
				DEBUG(5,("read_socket_with_timeout: blocking read. EOF from client.\n"));
				smb_read_error = READ_EOF;
				return -1;
			}
			
			if (readret == -1) {
				if (fd == client_fd) {
					/* Try and give an error message saying what client failed. */
					DEBUG(0,("read_socket_with_timeout: client %s read error = %s.\n",
						client_ip_string, strerror(errno) ));
				} else {
					DEBUG(0,("read_socket_with_timeout: read error = %s.\n", strerror(errno) ));
				}
				smb_read_error = READ_ERROR;
				return -1;
			}
			nread += readret;
		}
		return((ssize_t)nread);
	}
	
	/* Most difficult - timeout read */
	/* If this is ever called on a disk file and 
	   mincnt is greater then the filesize then
	   system performance will suffer severely as 
	   select always returns true on disk files */
	
	/* Set initial timeout */
	timeout.tv_sec = (time_t)(time_out / 1000);
	timeout.tv_usec = (long)(1000 * (time_out % 1000));
	
	for (nread=0; nread < mincnt; ) {      
		FD_ZERO(&fds);
		FD_SET(fd,&fds);
		
		selrtn = sys_select_intr(fd+1,&fds,NULL,NULL,&timeout);
		
		/* Check if error */
		if (selrtn == -1) {
			/* something is wrong. Maybe the socket is dead? */
			if (fd == client_fd) {
				/* Try and give an error message saying what client failed. */
				DEBUG(0,("read_socket_with_timeout: timeout read for client %s. select error = %s.\n",
					client_ip_string, strerror(errno) ));
			} else {
				DEBUG(0,("read_socket_with_timeout: timeout read. select error = %s.\n", strerror(errno) ));
			}
			smb_read_error = READ_ERROR;
			return -1;
		}
		
		/* Did we timeout ? */
		if (selrtn == 0) {
			DEBUG(10,("read_socket_with_timeout: timeout read. select timed out.\n"));
			smb_read_error = READ_TIMEOUT;
			return -1;
		}
		
		readret = sys_read(fd, buf+nread, maxcnt-nread);
		
		if (readret == 0) {
			/* we got EOF on the file descriptor */
			DEBUG(5,("read_socket_with_timeout: timeout read. EOF from client.\n"));
			smb_read_error = READ_EOF;
			return -1;
		}
		
		if (readret == -1) {
			/* the descriptor is probably dead */
			if (fd == client_fd) {
				/* Try and give an error message saying what client failed. */
				DEBUG(0,("read_socket_with_timeout: timeout read to client %s. read error = %s.\n",
					client_ip_string, strerror(errno) ));
			} else {
				DEBUG(0,("read_socket_with_timeout: timeout read. read error = %s.\n", strerror(errno) ));
			}
			smb_read_error = READ_ERROR;
			return -1;
		}
		
		nread += readret;
	}
	
	/* Return the number we got */
	return (ssize_t)nread;
}

/****************************************************************************
 Read data from the client, reading exactly N bytes. 
****************************************************************************/

ssize_t read_data(int fd,char *buffer,size_t N)
{
	ssize_t ret;
	size_t total=0;  
 
	smb_read_error = 0;

	while (total < N) {
		ret = sys_read(fd,buffer + total,N - total);

		if (ret == 0) {
			DEBUG(10,("read_data: read of %d returned 0. Error = %s\n", (int)(N - total), strerror(errno) ));
			smb_read_error = READ_EOF;
			return 0;
		}

		if (ret == -1) {
			if (fd == client_fd) {
				/* Try and give an error message saying what client failed. */
				DEBUG(0,("read_data: read failure for %d bytes to client %s. Error = %s\n",
					(int)(N - total), client_ip_string, strerror(errno) ));
			} else {
				DEBUG(0,("read_data: read failure for %d. Error = %s\n", (int)(N - total), strerror(errno) ));
			}
			smb_read_error = READ_ERROR;
			return -1;
		}
		total += ret;
	}
	return (ssize_t)total;
}

/****************************************************************************
 Write data to a fd.
****************************************************************************/

ssize_t write_data(int fd, const char *buffer, size_t N)
{
	size_t total=0;
	ssize_t ret;

	while (total < N) {
		ret = sys_write(fd,buffer + total,N - total);

		if (ret == -1) {
			if (fd == client_fd) {
				/* Try and give an error message saying what client failed. */
				DEBUG(0,("write_data: write failure in writing to client %s. Error %s\n",
					client_ip_string, strerror(errno) ));
			} else {
				DEBUG(0,("write_data: write failure. Error = %s\n", strerror(errno) ));
			}
			return -1;
		}

		if (ret == 0) {
			return total;
		}

		total += ret;
	}
	return (ssize_t)total;
}

/****************************************************************************
 Send a keepalive packet (rfc1002).
****************************************************************************/

BOOL send_keepalive(int client)
{
	unsigned char buf[4];

	buf[0] = SMBkeepalive;
	buf[1] = buf[2] = buf[3] = 0;

	return(write_data(client,(char *)buf,4) == 4);
}


/****************************************************************************
 Read 4 bytes of a smb packet and return the smb length of the packet.
 Store the result in the buffer.
 This version of the function will return a length of zero on receiving
 a keepalive packet.
 Timeout is in milliseconds.
****************************************************************************/

static ssize_t read_smb_length_return_keepalive(int fd, char *inbuf, unsigned int timeout)
{
	ssize_t len=0;
	int msg_type;
	BOOL ok = False;

	while (!ok) {
		if (timeout > 0)
			ok = (read_socket_with_timeout(fd,inbuf,4,4,timeout) == 4);
		else 
			ok = (read_data(fd,inbuf,4) == 4);

		if (!ok)
			return(-1);

		len = smb_len(inbuf);
		msg_type = CVAL(inbuf,0);

		if (msg_type == SMBkeepalive) 
			DEBUG(5,("Got keepalive packet\n"));
	}

	DEBUG(10,("got smb length of %lu\n",(unsigned long)len));

	return(len);
}

/****************************************************************************
 Read 4 bytes of a smb packet and return the smb length of the packet.
 Store the result in the buffer. This version of the function will
 never return a session keepalive (length of zero).
 Timeout is in milliseconds.
****************************************************************************/

ssize_t read_smb_length(int fd, char *inbuf, unsigned int timeout)
{
	ssize_t len;

	for(;;) {
		len = read_smb_length_return_keepalive(fd, inbuf, timeout);

		if(len < 0)
			return len;

		/* Ignore session keepalives. */
		if(CVAL(inbuf,0) != SMBkeepalive)
			break;
	}

	DEBUG(10,("read_smb_length: got smb length of %lu\n",
		  (unsigned long)len));

	return len;
}

/****************************************************************************
 Read an smb from a fd. Note that the buffer *MUST* be of size
 BUFFER_SIZE+SAFETY_MARGIN.
 The timeout is in milliseconds. 
 This function will return on receipt of a session keepalive packet.
 Doesn't check the MAC on signed packets.
****************************************************************************/

BOOL receive_smb_raw(int fd, char *buffer, unsigned int timeout)
{
	ssize_t len,ret;

	smb_read_error = 0;

	len = read_smb_length_return_keepalive(fd,buffer,timeout);
	if (len < 0) {
		DEBUG(10,("receive_smb_raw: length < 0!\n"));

		/*
		 * Correct fix. smb_read_error may have already been
		 * set. Only set it here if not already set. Global
		 * variables still suck :-). JRA.
		 */

		if (smb_read_error == 0)
			smb_read_error = READ_ERROR;
		return False;
	}

	/*
	 * A WRITEX with CAP_LARGE_WRITEX can be 64k worth of data plus 65 bytes
	 * of header. Don't print the error if this fits.... JRA.
	 */

	if (len > (BUFFER_SIZE + LARGE_WRITEX_HDR_SIZE)) {
		DEBUG(0,("Invalid packet length! (%lu bytes).\n",(unsigned long)len));
		if (len > BUFFER_SIZE + (SAFETY_MARGIN/2)) {

			/*
			 * Correct fix. smb_read_error may have already been
			 * set. Only set it here if not already set. Global
			 * variables still suck :-). JRA.
			 */

			if (smb_read_error == 0)
				smb_read_error = READ_ERROR;
			return False;
		}
	}

	if(len > 0) {
		if (timeout > 0) {
			ret = read_socket_with_timeout(fd,buffer+4,len,len,timeout);
		} else {
			ret = read_data(fd,buffer+4,len);
		}

		if (ret != len) {
			if (smb_read_error == 0) {
				smb_read_error = READ_ERROR;
			}
			return False;
		}
		
		/* not all of samba3 properly checks for packet-termination of strings. This
		   ensures that we don't run off into empty space. */
		SSVAL(buffer+4,len, 0);
	}

	return True;
}

/****************************************************************************
 Wrapper for receive_smb_raw().
 Checks the MAC on signed packets.
****************************************************************************/

BOOL receive_smb(int fd, char *buffer, unsigned int timeout)
{
	if (!receive_smb_raw(fd, buffer, timeout)) {
		return False;
	}

	/* Check the incoming SMB signature. */
	if (!srv_check_sign_mac(buffer, True)) {
		DEBUG(0, ("receive_smb: SMB Signature verification failed on incoming packet!\n"));
		if (smb_read_error == 0)
			smb_read_error = READ_BAD_SIG;
		return False;
	};

	return(True);
}

/****************************************************************************
 Send an smb to a fd.
****************************************************************************/

BOOL send_smb(int fd, char *buffer)
{
	size_t len;
	size_t nwritten=0;
	ssize_t ret;

	/* Sign the outgoing packet if required. */
	srv_calculate_sign_mac(buffer);

	len = smb_len(buffer) + 4;

	while (nwritten < len) {
		ret = write_data(fd,buffer+nwritten,len - nwritten);
		if (ret <= 0) {
			DEBUG(0,("Error writing %d bytes to client. %d. (%s)\n",
				(int)len,(int)ret, strerror(errno) ));
			return False;
		}
		nwritten += ret;
	}

	return True;
}

/****************************************************************************
 Open a socket of the specified type, port, and address for incoming data.
****************************************************************************/

int open_socket_in( int type, int port, int dlevel, uint32 socket_addr, BOOL rebind )
{
	struct sockaddr_in sock;
	int res;

	memset( (char *)&sock, '\0', sizeof(sock) );

#ifdef HAVE_SOCK_SIN_LEN
	sock.sin_len         = sizeof(sock);
#endif
	sock.sin_port        = htons( port );
	sock.sin_family      = AF_INET;
	sock.sin_addr.s_addr = socket_addr;

	res = socket( AF_INET, type, 0 );
	if( res == -1 ) {
		if( DEBUGLVL(0) ) {
			dbgtext( "open_socket_in(): socket() call failed: " );
			dbgtext( "%s\n", strerror( errno ) );
		}
		return -1;
	}

	/* This block sets/clears the SO_REUSEADDR and possibly SO_REUSEPORT. */
	{
		int val = rebind ? 1 : 0;
		if( setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val)) == -1 ) {
			if( DEBUGLVL( dlevel ) ) {
				dbgtext( "open_socket_in(): setsockopt: " );
				dbgtext( "SO_REUSEADDR = %s ", val?"True":"False" );
				dbgtext( "on port %d failed ", port );
				dbgtext( "with error = %s\n", strerror(errno) );
			}
		}
#ifdef SO_REUSEPORT
		if( setsockopt(res,SOL_SOCKET,SO_REUSEPORT,(char *)&val,sizeof(val)) == -1 ) {
			if( DEBUGLVL( dlevel ) ) {
				dbgtext( "open_socket_in(): setsockopt: ");
				dbgtext( "SO_REUSEPORT = %s ", val?"True":"False" );
				dbgtext( "on port %d failed ", port );
				dbgtext( "with error = %s\n", strerror(errno) );
			}
		}
#endif /* SO_REUSEPORT */
	}

	/* now we've got a socket - we need to bind it */
	if( bind( res, (struct sockaddr *)&sock, sizeof(sock) ) == -1 ) {
		if( DEBUGLVL(dlevel) && (port == SMB_PORT1 || port == SMB_PORT2 || port == NMB_PORT) ) {
			dbgtext( "bind failed on port %d ", port );
			dbgtext( "socket_addr = %s.\n", inet_ntoa( sock.sin_addr ) );
			dbgtext( "Error = %s\n", strerror(errno) );
		}
		close( res ); 
		return( -1 ); 
	}

	DEBUG( 10, ( "bind succeeded on port %d\n", port ) );

	return( res );
 }

/****************************************************************************
 Create an outgoing socket. timeout is in milliseconds.
**************************************************************************/

int open_socket_out(int type, struct in_addr *addr, int port ,int timeout)
{
	struct sockaddr_in sock_out;
	int res,ret;
	int connect_loop = 10;
	int increment = 10;

	/* create a socket to write to */
	res = socket(PF_INET, type, 0);
	if (res == -1) {
                DEBUG(0,("socket error (%s)\n", strerror(errno)));
		return -1;
	}

	if (type != SOCK_STREAM)
		return(res);
  
	memset((char *)&sock_out,'\0',sizeof(sock_out));
	putip((char *)&sock_out.sin_addr,(char *)addr);
  
	sock_out.sin_port = htons( port );
	sock_out.sin_family = PF_INET;

	/* set it non-blocking */
	set_blocking(res,False);

	DEBUG(3,("Connecting to %s at port %d\n",inet_ntoa(*addr),port));
  
	/* and connect it to the destination */
  connect_again:

	ret = connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out));

#ifdef _XBOX
	if (ret < 0) errno = socketerrno(1);
#endif

	/* Some systems return EAGAIN when they mean EINPROGRESS */
	if (ret < 0 && (errno == EINPROGRESS || errno == EALREADY ||
			errno == EAGAIN) && (connect_loop < timeout) ) {
		smb_msleep(connect_loop);
		timeout -= connect_loop;
		connect_loop += increment;
		if (increment < 250) {
			/* After 8 rounds we end up at a max of 255 msec */
			increment *= 1.5;
		}
		goto connect_again;
	}

	if (ret < 0 && (errno == EINPROGRESS || errno == EALREADY ||
			errno == EAGAIN)) {
		DEBUG(1,("timeout connecting to %s:%d\n",inet_ntoa(*addr),port));
		close(res);
		return -1;
	}

#ifdef EISCONN

	if (ret < 0 && errno == EISCONN) {
		errno = 0;
		ret = 0;
	}
#endif

	if (ret < 0) {
		DEBUG(2,("error connecting to %s:%d (%s)\n",
				inet_ntoa(*addr),port,strerror(errno)));
		close(res);
		return -1;
	}

	/* set it blocking again */
	set_blocking(res,True);

	return res;
}

/****************************************************************************
 Create an outgoing TCP socket to any of the addrs. This is for
 simultaneous connects to port 445 and 139 of a host or even a variety
 of DC's all of which are equivalent for our purposes.
**************************************************************************/

BOOL open_any_socket_out(struct sockaddr_in *addrs, int num_addrs,
			 int timeout, int *fd_index, int *fd)
{
	int i, resulting_index, res;
	int *sockets;
	BOOL good_connect;

	fd_set r_fds, wr_fds;
	struct timeval tv;
	int maxfd;

	int connect_loop = 10000; /* 10 milliseconds */

	timeout *= 1000; 	/* convert to microseconds */

	sockets = SMB_MALLOC_ARRAY(int, num_addrs);

	if (sockets == NULL)
		return False;

	resulting_index = -1;

	for (i=0; i<num_addrs; i++)
		sockets[i] = -1;

	for (i=0; i<num_addrs; i++) {
		sockets[i] = socket(PF_INET, SOCK_STREAM, 0);
		if (sockets[i] < 0)
			goto done;
		set_blocking(sockets[i], False);
	}

 connect_again:
	good_connect = False;

	for (i=0; i<num_addrs; i++) {

		if (sockets[i] == -1)
			continue;

		if (connect(sockets[i], (struct sockaddr *)&(addrs[i]),
			    sizeof(*addrs)) == 0) {
			/* Rather unlikely as we are non-blocking, but it
			 * might actually happen. */
			resulting_index = i;
			goto done;
		}

#ifdef _XBOX
		errno = socketerrno(1);
#endif

		if (errno == EINPROGRESS || errno == EALREADY ||
		    errno == EAGAIN) {
			/* These are the error messages that something is
			   progressing. */
			good_connect = True;
		} else if (errno != 0) {
			/* There was a direct error */
			close(sockets[i]);
			sockets[i] = -1;
		}
	}

	if (!good_connect) {
		/* All of the connect's resulted in real error conditions */
		goto done;
	}

	/* Lets see if any of the connect attempts succeeded */

	maxfd = 0;
	FD_ZERO(&wr_fds);
	FD_ZERO(&r_fds);

	for (i=0; i<num_addrs; i++) {
		if (sockets[i] == -1)
			continue;
		FD_SET(sockets[i], &wr_fds);
		FD_SET(sockets[i], &r_fds);
		if (sockets[i]>maxfd)
			maxfd = sockets[i];
	}

	tv.tv_sec = 0;
	tv.tv_usec = connect_loop;

	res = sys_select_intr(maxfd+1, &r_fds, &wr_fds, NULL, &tv);

	if (res < 0)
		goto done;

	if (res == 0)
		goto next_round;

	for (i=0; i<num_addrs; i++) {

		if (sockets[i] == -1)
			continue;

		/* Stevens, Network Programming says that if there's a
		 * successful connect, the socket is only writable. Upon an
		 * error, it's both readable and writable. */

		if (FD_ISSET(sockets[i], &r_fds) &&
		    FD_ISSET(sockets[i], &wr_fds)) {
			/* readable and writable, so it's an error */
			close(sockets[i]);
			sockets[i] = -1;
			continue;
		}

		if (!FD_ISSET(sockets[i], &r_fds) &&
		    FD_ISSET(sockets[i], &wr_fds)) {
			/* Only writable, so it's connected */
			resulting_index = i;
			goto done;
		}
	}

 next_round:

	timeout -= connect_loop;
	if (timeout <= 0)
		goto done;
	connect_loop *= 1.5;
	if (connect_loop > timeout)
		connect_loop = timeout;
	goto connect_again;

 done:
	for (i=0; i<num_addrs; i++) {
		if (i == resulting_index)
			continue;
		if (sockets[i] >= 0)
			close(sockets[i]);
	}

	if (resulting_index >= 0) {
		*fd_index = resulting_index;
		*fd = sockets[*fd_index];
		set_blocking(*fd, True);
	}

	free(sockets);

	return (resulting_index >= 0);
}
/****************************************************************************
 Open a connected UDP socket to host on port
**************************************************************************/

int open_udp_socket(const char *host, int port)
{
	int type = SOCK_DGRAM;
	struct sockaddr_in sock_out;
	int res;
	struct in_addr *addr;

	addr = interpret_addr2(host);

	res = socket(PF_INET, type, 0);
	if (res == -1) {
		return -1;
	}

	memset((char *)&sock_out,'\0',sizeof(sock_out));
	putip((char *)&sock_out.sin_addr,(char *)addr);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		close(res);
		return -1;
	}

	return res;
}


/*******************************************************************
 Matchname - determine if host name matches IP address. Used to
 confirm a hostname lookup to prevent spoof attacks.
******************************************************************/

static BOOL matchname(char *remotehost,struct in_addr  addr)
{
	struct hostent *hp;
	int     i;
	
	if ((hp = sys_gethostbyname(remotehost)) == 0) {
		DEBUG(0,("sys_gethostbyname(%s): lookup failure.\n", remotehost));
		return False;
	} 

	/*
	 * Make sure that gethostbyname() returns the "correct" host name.
	 * Unfortunately, gethostbyname("localhost") sometimes yields
	 * "localhost.domain". Since the latter host name comes from the
	 * local DNS, we just have to trust it (all bets are off if the local
	 * DNS is perverted). We always check the address list, though.
	 */
	
	if (!strequal(remotehost, hp->h_name)
	    && !strequal(remotehost, "localhost")) {
		DEBUG(0,("host name/name mismatch: %s != %s\n",
			 remotehost, hp->h_name));
		return False;
	}
	
	/* Look up the host address in the address list we just got. */
	for (i = 0; hp->h_addr_list[i]; i++) {
		if (memcmp(hp->h_addr_list[i], (char *) & addr, sizeof(addr)) == 0)
			return True;
	}
	
	/*
	 * The host name does not map to the original host address. Perhaps
	 * someone has compromised a name server. More likely someone botched
	 * it, but that could be dangerous, too.
	 */
	
	DEBUG(0,("host name/address mismatch: %s != %s\n",
		 inet_ntoa(addr), hp->h_name));
	return False;
}

/*******************************************************************
 Return the DNS name of the remote end of a socket.
******************************************************************/

char *get_peer_name(int fd, BOOL force_lookup)
{
	static pstring name_buf;
	pstring tmp_name;
	static fstring addr_buf;
	struct hostent *hp;
	struct in_addr addr;
	char *p;

	/* reverse lookups can be *very* expensive, and in many
	   situations won't work because many networks don't link dhcp
	   with dns. To avoid the delay we avoid the lookup if
	   possible */
	if (!lp_hostname_lookups() && (force_lookup == False)) {
		return get_peer_addr(fd);
	}
	
	p = get_peer_addr(fd);

	/* it might be the same as the last one - save some DNS work */
	if (strcmp(p, addr_buf) == 0) 
		return name_buf;

	pstrcpy(name_buf,"UNKNOWN");
	if (fd == -1) 
		return name_buf;

	fstrcpy(addr_buf, p);

	addr = *interpret_addr2(p);
	
	/* Look up the remote host name. */
	if ((hp = gethostbyaddr((char *)&addr.s_addr, sizeof(addr.s_addr), AF_INET)) == 0) {
		DEBUG(1,("Gethostbyaddr failed for %s\n",p));
		pstrcpy(name_buf, p);
	} else {
		pstrcpy(name_buf,(char *)hp->h_name);
		if (!matchname(name_buf, addr)) {
			DEBUG(0,("Matchname failed on %s %s\n",name_buf,p));
			pstrcpy(name_buf,"UNKNOWN");
		}
	}

	/* can't pass the same source and dest strings in when you 
	   use --enable-developer or the clobber_region() call will 
	   get you */
	
	pstrcpy( tmp_name, name_buf );
	alpha_strcpy(name_buf, tmp_name, "_-.", sizeof(name_buf));
	if (strstr(name_buf,"..")) {
		pstrcpy(name_buf, "UNKNOWN");
	}

	return name_buf;
}

/*******************************************************************
 Return the IP addr of the remote end of a socket as a string.
 ******************************************************************/

char *get_peer_addr(int fd)
{
	struct sockaddr sa;
	struct sockaddr_in *sockin = (struct sockaddr_in *) (&sa);
	socklen_t length = sizeof(sa);
	static fstring addr_buf;

	fstrcpy(addr_buf,"0.0.0.0");

	if (fd == -1) {
		return addr_buf;
	}
	
	if (getpeername(fd, &sa, &length) < 0) {
		DEBUG(0,("getpeername failed. Error was %s\n", strerror(errno) ));
		return addr_buf;
	}
	
	fstrcpy(addr_buf,(char *)inet_ntoa(sockin->sin_addr));
	
	return addr_buf;
}

/*******************************************************************
 Create protected unix domain socket.

 Some unixes cannot set permissions on a ux-dom-sock, so we
 have to make sure that the directory contains the protection
 permissions instead.
 ******************************************************************/

int create_pipe_sock(const char *socket_dir,
		     const char *socket_name,
		     mode_t dir_perms)
{
#ifdef HAVE_UNIXSOCKET
	struct sockaddr_un sunaddr;
	struct stat st;
	int sock;
	mode_t old_umask;
	pstring path;
        
	old_umask = umask(0);
        
	/* Create the socket directory or reuse the existing one */
        
	if (lstat(socket_dir, &st) == -1) {
		if (errno == ENOENT) {
			/* Create directory */
			if (mkdir(socket_dir, dir_perms) == -1) {
				DEBUG(0, ("error creating socket directory "
					"%s: %s\n", socket_dir, 
					strerror(errno)));
				goto out_umask;
			}
		} else {
			DEBUG(0, ("lstat failed on socket directory %s: %s\n",
				socket_dir, strerror(errno)));
			goto out_umask;
		}
	} else {
		/* Check ownership and permission on existing directory */
		if (!S_ISDIR(st.st_mode)) {
			DEBUG(0, ("socket directory %s isn't a directory\n",
				socket_dir));
			goto out_umask;
		}
		if ((st.st_uid != sec_initial_uid()) || 
				((st.st_mode & 0777) != dir_perms)) {
			DEBUG(0, ("invalid permissions on socket directory "
				"%s\n", socket_dir));
			goto out_umask;
		}
	}
        
	/* Create the socket file */
        
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
        
	if (sock == -1) {
		perror("socket");
                goto out_umask;
	}
        
	pstr_sprintf(path, "%s/%s", socket_dir, socket_name);
        
	unlink(path);
	memset(&sunaddr, 0, sizeof(sunaddr));
	sunaddr.sun_family = AF_UNIX;
	safe_strcpy(sunaddr.sun_path, path, sizeof(sunaddr.sun_path)-1);
        
	if (bind(sock, (struct sockaddr *)&sunaddr, sizeof(sunaddr)) == -1) {
		DEBUG(0, ("bind failed on pipe socket %s: %s\n", path,
			strerror(errno)));
		goto out_close;
	}
        
	if (listen(sock, 5) == -1) {
		DEBUG(0, ("listen failed on pipe socket %s: %s\n", path,
			strerror(errno)));
		goto out_close;
	}
        
	umask(old_umask);
	return sock;

out_close:
	close(sock);

out_umask:
	umask(old_umask);
	return -1;

#else
        DEBUG(0, ("create_pipe_sock: No Unix sockets on this system\n"));
        return -1;
#endif /* HAVE_UNIXSOCKET */
}

/* 
   Unix SMB/CIFS implementation.
   SMB filter/socket plugin
   Copyright (C) Andrew Tridgell 1999
   
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

#define SECURITY_MASK 0
#define SECURITY_SET  0

/* this forces non-unicode */
#define CAPABILITY_MASK 0
#define CAPABILITY_SET  0

/* and non-unicode for the client too */
#define CLI_CAPABILITY_MASK 0
#define CLI_CAPABILITY_SET  0

static char *netbiosname;
static char packet[BUFFER_SIZE];

static void save_file(const char *fname, void *ppacket, size_t length)
{
	int fd;
	fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		perror(fname);
		return;
	}
	if (write(fd, ppacket, length) != length) {
		fprintf(stderr,"Failed to write %s\n", fname);
		return;
	}
	close(fd);
	printf("Wrote %ld bytes to %s\n", (unsigned long)length, fname);
}

static void filter_reply(char *buf)
{
	int msg_type = CVAL(buf,0);
	int type = CVAL(buf,smb_com);
	unsigned x;

	if (msg_type) return;

	switch (type) {

	case SMBnegprot:
		/* force the security bits */
		x = CVAL(buf, smb_vwv1);
		x = (x | SECURITY_SET) & ~SECURITY_MASK;
		SCVAL(buf, smb_vwv1, x);

		/* force the capabilities */
		x = IVAL(buf,smb_vwv9+1);
		x = (x | CAPABILITY_SET) & ~CAPABILITY_MASK;
		SIVAL(buf, smb_vwv9+1, x);
		break;

	}
}

static void filter_request(char *buf)
{
	int msg_type = CVAL(buf,0);
	int type = CVAL(buf,smb_com);
	pstring name1,name2;
	unsigned x;

	if (msg_type) {
		/* it's a netbios special */
		switch (msg_type) {
		case 0x81:
			/* session request */
			name_extract(buf,4,name1);
			name_extract(buf,4 + name_len(buf + 4),name2);
			d_printf("sesion_request: %s -> %s\n",
				 name1, name2);
			if (netbiosname) {
				/* replace the destination netbios name */
				name_mangle(netbiosname, buf+4, 0x20);
			}
		}
		return;
	}

	/* it's an ordinary SMB request */
	switch (type) {
	case SMBsesssetupX:
		/* force the client capabilities */
		x = IVAL(buf,smb_vwv11);
		d_printf("SMBsesssetupX cap=0x%08x\n", x);
		d_printf("pwlen=%d/%d\n", SVAL(buf, smb_vwv7), SVAL(buf, smb_vwv8));
		system("mv sessionsetup.dat sessionsetup1.dat");
		save_file("sessionsetup.dat", smb_buf(buf), SVAL(buf, smb_vwv7));
		x = (x | CLI_CAPABILITY_SET) & ~CLI_CAPABILITY_MASK;
		SIVAL(buf, smb_vwv11, x);
		break;
	}

}


static void filter_child(int c, struct in_addr dest_ip)
{
	int s;

	/* we have a connection from a new client, now connect to the server */
	s = open_socket_out(SOCK_STREAM, &dest_ip, 445, LONG_CONNECT_TIMEOUT);

	if (s == -1) {
		d_printf("Unable to connect to %s\n", inet_ntoa(dest_ip));
		exit(1);
	}

	while (c != -1 || s != -1) {
		fd_set fds;
		int num;
		
		FD_ZERO(&fds);
		if (s != -1) FD_SET(s, &fds);
		if (c != -1) FD_SET(c, &fds);

		num = sys_select_intr(MAX(s+1, c+1),&fds,NULL,NULL,NULL);
		if (num <= 0) continue;
		
		if (c != -1 && FD_ISSET(c, &fds)) {
			if (!receive_smb(c, packet, 0)) {
				d_printf("client closed connection\n");
				exit(0);
			}
			filter_request(packet);
			if (!send_smb(s, packet)) {
				d_printf("server is dead\n");
				exit(1);
			}			
		}
		if (s != -1 && FD_ISSET(s, &fds)) {
			if (!receive_smb(s, packet, 0)) {
				d_printf("server closed connection\n");
				exit(0);
			}
			filter_reply(packet);
			if (!send_smb(c, packet)) {
				d_printf("client is dead\n");
				exit(1);
			}			
		}
	}
	d_printf("Connection closed\n");
	exit(0);
}


static void start_filter(char *desthost)
{
	int s, c;
	struct in_addr dest_ip;

	CatchChild();

	/* start listening on port 445 locally */
	s = open_socket_in(SOCK_STREAM, 445, 0, 0, True);
	
	if (s == -1) {
		d_printf("bind failed\n");
		exit(1);
	}

	if (listen(s, 5) == -1) {
		d_printf("listen failed\n");
	}

	if (!resolve_name(desthost, &dest_ip, 0x20)) {
		d_printf("Unable to resolve host %s\n", desthost);
		exit(1);
	}

	while (1) {
		fd_set fds;
		int num;
		struct sockaddr addr;
		socklen_t in_addrlen = sizeof(addr);
		
		FD_ZERO(&fds);
		FD_SET(s, &fds);

		num = sys_select_intr(s+1,&fds,NULL,NULL,NULL);
		if (num > 0) {
			c = accept(s, &addr, &in_addrlen);
			if (c != -1) {
				if (fork() == 0) {
#ifdef _XBOX
					closesocket(s);
#elif	
					close(s);
#endif //_XBOX				
					filter_child(c, dest_ip);
					exit(0);
				} else {
#ifdef _XBOX
					closesocket(c);
#elif	
					close(c);
#endif //_XBOX
				}
			}
		}
	}
}


int main(int argc, char *argv[])
{
	char *desthost;
	pstring configfile;

	setup_logging(argv[0],True);
  
	pstrcpy(configfile,dyn_CONFIGFILE);
 
	if (argc < 2) {
		fprintf(stderr,"smbfilter <desthost> <netbiosname>\n");
		exit(1);
	}

	desthost = argv[1];
	if (argc > 2) {
		netbiosname = argv[2];
	}

	if (!lp_load(configfile,True,False,False,True)) {
		d_printf("Unable to load config file\n");
	}

	start_filter(desthost);
	return 0;
}

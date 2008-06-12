/* 
   Unix SMB/CIFS implementation.
   client dgram calls
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Richard Sharpe 2001
   Copyright (C) John Terpstra 2001

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

/*
 * cli_send_mailslot, send a mailslot for client code ...
 */

BOOL cli_send_mailslot(BOOL unique, const char *mailslot,
		       uint16 priority,
		       char *buf, int len,
		       const char *srcname, int src_type, 
		       const char *dstname, int dest_type,
		       struct in_addr dest_ip)
{
	struct packet_struct p;
	struct dgram_packet *dgram = &p.packet.dgram;
	char *ptr, *p2;
	char tmp[4];
	pid_t nmbd_pid;

	if ((nmbd_pid = pidfile_pid("nmbd")) == 0) {
		DEBUG(3, ("No nmbd found\n"));
		return False;
	}

	if (!message_init())
		return False;

	memset((char *)&p, '\0', sizeof(p));

	/*
	 * Next, build the DGRAM ...
	 */

	/* DIRECT GROUP or UNIQUE datagram. */
	dgram->header.msg_type = unique ? 0x10 : 0x11; 
	dgram->header.flags.node_type = M_NODE;
	dgram->header.flags.first = True;
	dgram->header.flags.more = False;
	dgram->header.dgm_id = ((unsigned)time(NULL)%(unsigned)0x7FFF) +
		((unsigned)sys_getpid()%(unsigned)100);
	/* source ip is filled by nmbd */
	dgram->header.dgm_length = 0; /* Let build_dgram() handle this. */
	dgram->header.packet_offset = 0;
  
	make_nmb_name(&dgram->source_name,srcname,src_type);
	make_nmb_name(&dgram->dest_name,dstname,dest_type);

	ptr = &dgram->data[0];

	/* Setup the smb part. */
	ptr -= 4; /* XXX Ugliness because of handling of tcp SMB length. */
	memcpy(tmp,ptr,4);
	set_message(ptr,17,strlen(mailslot) + 1 + len,True);
	memcpy(ptr,tmp,4);

	SCVAL(ptr,smb_com,SMBtrans);
	SSVAL(ptr,smb_vwv1,len);
	SSVAL(ptr,smb_vwv11,len);
	SSVAL(ptr,smb_vwv12,70 + strlen(mailslot));
	SSVAL(ptr,smb_vwv13,3);
	SSVAL(ptr,smb_vwv14,1);
	SSVAL(ptr,smb_vwv15,priority);
	SSVAL(ptr,smb_vwv16,2);
	p2 = smb_buf(ptr);
	fstrcpy(p2,mailslot);
	p2 = skip_string(p2,1);

	memcpy(p2,buf,len);
	p2 += len;

	dgram->datasize = PTR_DIFF(p2,ptr+4); /* +4 for tcp length. */

	p.packet_type = DGRAM_PACKET;
	p.ip = dest_ip;
	p.timestamp = time(NULL);

	DEBUG(4,("send_mailslot: Sending to mailslot %s from %s ",
		 mailslot, nmb_namestr(&dgram->source_name)));
	DEBUGADD(4,("to %s IP %s\n", nmb_namestr(&dgram->dest_name),
		    inet_ntoa(dest_ip)));

	return message_send_pid(pid_to_procid(nmbd_pid), MSG_SEND_PACKET, &p, sizeof(p),
				False);
}

/*
 * cli_get_response: Get a response ...
 */
BOOL cli_get_response(const char *mailslot, char *buf, int bufsiz)
{
	struct packet_struct *p;

	p = receive_unexpected(DGRAM_PACKET, 0, mailslot);

	if (p == NULL)
		return False;

	memcpy(buf, &p->packet.dgram.data[92],
	       MIN(bufsiz, p->packet.dgram.datasize-92));

	return True;
}

/*
 * cli_get_backup_list: Send a get backup list request ...
 */

static char cli_backup_list[1024];

int cli_get_backup_list(const char *myname, const char *send_to_name)
{
	pstring outbuf;
	char *p;
	struct in_addr sendto_ip;

	if (!resolve_name(send_to_name, &sendto_ip, 0x1d)) {

		DEBUG(0, ("Could not resolve name: %s<1D>\n", send_to_name));
		return False;

	}

	memset(cli_backup_list, '\0', sizeof(cli_backup_list));
	memset(outbuf, '\0', sizeof(outbuf));

	p = outbuf;

	SCVAL(p, 0, ANN_GetBackupListReq);
	p++;

	SCVAL(p, 0, 1); /* Count pointer ... */
	p++;

	SIVAL(p, 0, 1); /* The sender's token ... */
	p += 4;

	cli_send_mailslot(True, "\\MAILSLOT\\BROWSE", 1, outbuf, 
			  PTR_DIFF(p, outbuf), myname, 0, send_to_name, 
			  0x1d, sendto_ip);

	/* We should check the error and return if we got one */

	/* Now, get the response ... */

	cli_get_response("\\MAILSLOT\\BROWSE",
			 cli_backup_list, sizeof(cli_backup_list));

	return True;

}

/*
 * cli_get_backup_server: Get the backup list and retrieve a server from it
 */

int cli_get_backup_server(char *my_name, char *target, char *servername, int namesize)
{

  /* Get the backup list first. We could pull this from the cache later */

  cli_get_backup_list(my_name, target);  /* FIXME: Check the response */

  if (!cli_backup_list[0]) { /* Empty list ... try again */

    cli_get_backup_list(my_name, target);

  }

  strncpy(servername, cli_backup_list, MIN(16, namesize));

  return True;

}

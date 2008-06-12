/* 
   Unix SMB/CIFS implementation.
   NBT netbios library routines
   Copyright (C) Andrew Tridgell 1994-1998
   
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

extern struct in_addr lastip;
extern int lastport;

int num_good_sends = 0;
int num_good_receives = 0;

static const struct opcode_names {
	const char *nmb_opcode_name;
	int opcode;
} nmb_header_opcode_names[] = {
	{"Query",           0 },
	{"Registration",      5 },
	{"Release",           6 },
	{"WACK",              7 },
	{"Refresh",           8 },
	{"Refresh(altcode)",  9 },
	{"Multi-homed Registration", 15 },
	{0, -1 }
};

/****************************************************************************
 Lookup a nmb opcode name.
****************************************************************************/

static const char *lookup_opcode_name( int opcode )
{
	const struct opcode_names *op_namep;
	int i;

	for(i = 0; nmb_header_opcode_names[i].nmb_opcode_name != 0; i++) {
		op_namep = &nmb_header_opcode_names[i];
		if(opcode == op_namep->opcode)
			return op_namep->nmb_opcode_name;
	}
	return "<unknown opcode>";
}

/****************************************************************************
 Print out a res_rec structure.
****************************************************************************/

static void debug_nmb_res_rec(struct res_rec *res, const char *hdr)
{
	int i, j;

	DEBUGADD( 4, ( "    %s: nmb_name=%s rr_type=%d rr_class=%d ttl=%d\n",
		hdr,
		nmb_namestr(&res->rr_name),
		res->rr_type,
		res->rr_class,
		res->ttl ) );

	if( res->rdlength == 0 || res->rdata == NULL )
		return;

	for (i = 0; i < res->rdlength; i+= MAX_NETBIOSNAME_LEN) {
		DEBUGADD(4, ("    %s %3x char ", hdr, i));

		for (j = 0; j < MAX_NETBIOSNAME_LEN; j++) {
			unsigned char x = res->rdata[i+j];
			if (x < 32 || x > 127)
				x = '.';
	  
			if (i+j >= res->rdlength)
				break;
			DEBUGADD(4, ("%c", x));
		}
      
		DEBUGADD(4, ("   hex "));

		for (j = 0; j < MAX_NETBIOSNAME_LEN; j++) {
			if (i+j >= res->rdlength)
				break;
			DEBUGADD(4, ("%02X", (unsigned char)res->rdata[i+j]));
		}
      
		DEBUGADD(4, ("\n"));
	}
}

/****************************************************************************
 Process a nmb packet.
****************************************************************************/

void debug_nmb_packet(struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;

	if( DEBUGLVL( 4 ) ) {
		dbgtext( "nmb packet from %s(%d) header: id=%d opcode=%s(%d) response=%s\n",
			inet_ntoa(p->ip), p->port,
			nmb->header.name_trn_id,
			lookup_opcode_name(nmb->header.opcode),
			nmb->header.opcode,
			BOOLSTR(nmb->header.response) );
		dbgtext( "    header: flags: bcast=%s rec_avail=%s rec_des=%s trunc=%s auth=%s\n",
			BOOLSTR(nmb->header.nm_flags.bcast),
			BOOLSTR(nmb->header.nm_flags.recursion_available),
			BOOLSTR(nmb->header.nm_flags.recursion_desired),
			BOOLSTR(nmb->header.nm_flags.trunc),
			BOOLSTR(nmb->header.nm_flags.authoritative) );
		dbgtext( "    header: rcode=%d qdcount=%d ancount=%d nscount=%d arcount=%d\n",
			nmb->header.rcode,
			nmb->header.qdcount,
			nmb->header.ancount,
			nmb->header.nscount,
			nmb->header.arcount );
	}

	if (nmb->header.qdcount) {
		DEBUGADD( 4, ( "    question: q_name=%s q_type=%d q_class=%d\n",
			nmb_namestr(&nmb->question.question_name),
			nmb->question.question_type,
			nmb->question.question_class) );
	}

	if (nmb->answers && nmb->header.ancount) {
		debug_nmb_res_rec(nmb->answers,"answers");
	}
	if (nmb->nsrecs && nmb->header.nscount) {
		debug_nmb_res_rec(nmb->nsrecs,"nsrecs");
	}
	if (nmb->additional && nmb->header.arcount) {
		debug_nmb_res_rec(nmb->additional,"additional");
	}
}

/*******************************************************************
 Handle "compressed" name pointers.
******************************************************************/

static BOOL handle_name_ptrs(unsigned char *ubuf,int *offset,int length,
			     BOOL *got_pointer,int *ret)
{
	int loop_count=0;
  
	while ((ubuf[*offset] & 0xC0) == 0xC0) {
		if (!*got_pointer)
			(*ret) += 2;
		(*got_pointer)=True;
		(*offset) = ((ubuf[*offset] & ~0xC0)<<8) | ubuf[(*offset)+1];
		if (loop_count++ == 10 || (*offset) < 0 || (*offset)>(length-2)) {
			return(False);
		}
	}
	return(True);
}

/*******************************************************************
 Parse a nmb name from "compressed" format to something readable
 return the space taken by the name, or 0 if the name is invalid
******************************************************************/

static int parse_nmb_name(char *inbuf,int ofs,int length, struct nmb_name *name)
{
	int m,n=0;
	unsigned char *ubuf = (unsigned char *)inbuf;
	int ret = 0;
	BOOL got_pointer=False;
	int loop_count=0;
	int offset = ofs;

	if (length - offset < 2)
		return(0);  

	/* handle initial name pointers */
	if (!handle_name_ptrs(ubuf,&offset,length,&got_pointer,&ret))
		return(0);
  
	m = ubuf[offset];

	if (!m)
		return(0);
	if ((m & 0xC0) || offset+m+2 > length)
		return(0);

	memset((char *)name,'\0',sizeof(*name));

	/* the "compressed" part */
	if (!got_pointer)
		ret += m + 2;
	offset++;
	while (m > 0) {
		unsigned char c1,c2;
		c1 = ubuf[offset++]-'A';
		c2 = ubuf[offset++]-'A';
		if ((c1 & 0xF0) || (c2 & 0xF0) || (n > sizeof(name->name)-1))
			return(0);
		name->name[n++] = (c1<<4) | c2;
		m -= 2;
	}
	name->name[n] = 0;

	if (n==MAX_NETBIOSNAME_LEN) {
		/* parse out the name type, its always in the 16th byte of the name */
		name->name_type = ((unsigned char)name->name[15]) & 0xff;
  
		/* remove trailing spaces */
		name->name[15] = 0;
		n = 14;
		while (n && name->name[n]==' ')
			name->name[n--] = 0;  
	}

	/* now the domain parts (if any) */
	n = 0;
	while (ubuf[offset]) {
		/* we can have pointers within the domain part as well */
		if (!handle_name_ptrs(ubuf,&offset,length,&got_pointer,&ret))
			return(0);

		m = ubuf[offset];
		/*
		 * Don't allow null domain parts.
		 */
		if (!m)
			return(0);
		if (!got_pointer)
			ret += m+1;
		if (n)
			name->scope[n++] = '.';
		if (m+2+offset>length || n+m+1>sizeof(name->scope))
			return(0);
		offset++;
		while (m--)
			name->scope[n++] = (char)ubuf[offset++];

		/*
		 * Watch for malicious loops.
		 */
		if (loop_count++ == 10)
			return 0;
	}
	name->scope[n++] = 0;  

	return(ret);
}

/****************************************************************************
 Put a netbios name, padding(s) and a name type into a 16 character buffer.
 name is already in DOS charset.
 [15 bytes name + padding][1 byte name type].
****************************************************************************/

void put_name(char *dest, const char *name, int pad, unsigned int name_type)
{
	size_t len = strlen(name);

	memcpy(dest, name, (len < MAX_NETBIOSNAME_LEN) ? len : MAX_NETBIOSNAME_LEN - 1);
	if (len < MAX_NETBIOSNAME_LEN - 1) {
		memset(dest + len, pad, MAX_NETBIOSNAME_LEN - 1 - len);
	}
	dest[MAX_NETBIOSNAME_LEN - 1] = name_type;
}

/*******************************************************************
 Put a compressed nmb name into a buffer. Return the length of the
 compressed name.

 Compressed names are really weird. The "compression" doubles the
 size. The idea is that it also means that compressed names conform
 to the doman name system. See RFC1002.
******************************************************************/

static int put_nmb_name(char *buf,int offset,struct nmb_name *name)
{
	int ret,m;
	nstring buf1;
	char *p;

	if (strcmp(name->name,"*") == 0) {
		/* special case for wildcard name */
		put_name(buf1, "*", '\0', name->name_type);
	} else {
		put_name(buf1, name->name, ' ', name->name_type);
	}

	buf[offset] = 0x20;

	ret = 34;

	for (m=0;m<MAX_NETBIOSNAME_LEN;m++) {
		buf[offset+1+2*m] = 'A' + ((buf1[m]>>4)&0xF);
		buf[offset+2+2*m] = 'A' + (buf1[m]&0xF);
	}
	offset += 33;

	buf[offset] = 0;

	if (name->scope[0]) {
		/* XXXX this scope handling needs testing */
		ret += strlen(name->scope) + 1;
		safe_strcpy(&buf[offset+1],name->scope,sizeof(name->scope));  
  
		p = &buf[offset+1];
		while ((p = strchr_m(p,'.'))) {
			buf[offset] = PTR_DIFF(p,&buf[offset+1]);
			offset += (buf[offset] + 1);
			p = &buf[offset+1];
		}
		buf[offset] = strlen(&buf[offset+1]);
	}

	return(ret);
}

/*******************************************************************
 Useful for debugging messages.
******************************************************************/

char *nmb_namestr(const struct nmb_name *n)
{
	static int i=0;
	static fstring ret[4];
	fstring name;
	char *p = ret[i];

	pull_ascii_fstring(name, n->name);
	if (!n->scope[0])
		slprintf(p,sizeof(fstring)-1, "%s<%02x>",name,n->name_type);
	else
		slprintf(p,sizeof(fstring)-1, "%s<%02x>.%s",name,n->name_type,n->scope);

	i = (i+1)%4;
	return(p);
}

/*******************************************************************
 Allocate and parse some resource records.
******************************************************************/

static BOOL parse_alloc_res_rec(char *inbuf,int *offset,int length,
				struct res_rec **recs, int count)
{
	int i;

	*recs = SMB_MALLOC_ARRAY(struct res_rec, count);
	if (!*recs)
		return(False);

	memset((char *)*recs,'\0',sizeof(**recs)*count);

	for (i=0;i<count;i++) {
		int l = parse_nmb_name(inbuf,*offset,length,&(*recs)[i].rr_name);
		(*offset) += l;
		if (!l || (*offset)+10 > length) {
			SAFE_FREE(*recs);
			return(False);
		}
		(*recs)[i].rr_type = RSVAL(inbuf,(*offset));
		(*recs)[i].rr_class = RSVAL(inbuf,(*offset)+2);
		(*recs)[i].ttl = RIVAL(inbuf,(*offset)+4);
		(*recs)[i].rdlength = RSVAL(inbuf,(*offset)+8);
		(*offset) += 10;
		if ((*recs)[i].rdlength>sizeof((*recs)[i].rdata) || 
				(*offset)+(*recs)[i].rdlength > length) {
			SAFE_FREE(*recs);
			return(False);
		}
		memcpy((*recs)[i].rdata,inbuf+(*offset),(*recs)[i].rdlength);
		(*offset) += (*recs)[i].rdlength;    
	}
	return(True);
}

/*******************************************************************
 Put a resource record into a packet.
******************************************************************/

static int put_res_rec(char *buf,int offset,struct res_rec *recs,int count)
{
	int ret=0;
	int i;

	for (i=0;i<count;i++) {
		int l = put_nmb_name(buf,offset,&recs[i].rr_name);
		offset += l;
		ret += l;
		RSSVAL(buf,offset,recs[i].rr_type);
		RSSVAL(buf,offset+2,recs[i].rr_class);
		RSIVAL(buf,offset+4,recs[i].ttl);
		RSSVAL(buf,offset+8,recs[i].rdlength);
		memcpy(buf+offset+10,recs[i].rdata,recs[i].rdlength);
		offset += 10+recs[i].rdlength;
		ret += 10+recs[i].rdlength;
	}

	return(ret);
}

/*******************************************************************
 Put a compressed name pointer record into a packet.
******************************************************************/

static int put_compressed_name_ptr(unsigned char *buf,int offset,struct res_rec *rec,int ptr_offset)
{  
	int ret=0;
	buf[offset] = (0xC0 | ((ptr_offset >> 8) & 0xFF));
	buf[offset+1] = (ptr_offset & 0xFF);
	offset += 2;
	ret += 2;
	RSSVAL(buf,offset,rec->rr_type);
	RSSVAL(buf,offset+2,rec->rr_class);
	RSIVAL(buf,offset+4,rec->ttl);
	RSSVAL(buf,offset+8,rec->rdlength);
	memcpy(buf+offset+10,rec->rdata,rec->rdlength);
	offset += 10+rec->rdlength;
	ret += 10+rec->rdlength;
    
	return(ret);
}

/*******************************************************************
 Parse a dgram packet. Return False if the packet can't be parsed 
 or is invalid for some reason, True otherwise.

 This is documented in section 4.4.1 of RFC1002.
******************************************************************/

static BOOL parse_dgram(char *inbuf,int length,struct dgram_packet *dgram)
{
	int offset;
	int flags;

	memset((char *)dgram,'\0',sizeof(*dgram));

	if (length < 14)
		return(False);

	dgram->header.msg_type = CVAL(inbuf,0);
	flags = CVAL(inbuf,1);
	dgram->header.flags.node_type = (enum node_type)((flags>>2)&3);
	if (flags & 1)
		dgram->header.flags.more = True;
	if (flags & 2)
		dgram->header.flags.first = True;
	dgram->header.dgm_id = RSVAL(inbuf,2);
	putip((char *)&dgram->header.source_ip,inbuf+4);
	dgram->header.source_port = RSVAL(inbuf,8);
	dgram->header.dgm_length = RSVAL(inbuf,10);
	dgram->header.packet_offset = RSVAL(inbuf,12);

	offset = 14;

	if (dgram->header.msg_type == 0x10 ||
			dgram->header.msg_type == 0x11 ||
			dgram->header.msg_type == 0x12) {      
		offset += parse_nmb_name(inbuf,offset,length,&dgram->source_name);
		offset += parse_nmb_name(inbuf,offset,length,&dgram->dest_name);
	}

	if (offset >= length || (length-offset > sizeof(dgram->data))) 
		return(False);

	dgram->datasize = length-offset;
	memcpy(dgram->data,inbuf+offset,dgram->datasize);

	/* Paranioa. Ensure the last 2 bytes in the dgram buffer are
	   zero. This should be true anyway, just enforce it for paranioa sake. JRA. */
	SMB_ASSERT(dgram->datasize <= (sizeof(dgram->data)-2));
	memset(&dgram->data[sizeof(dgram->data)-2], '\0', 2);

	return(True);
}

/*******************************************************************
 Parse a nmb packet. Return False if the packet can't be parsed 
 or is invalid for some reason, True otherwise.
******************************************************************/

static BOOL parse_nmb(char *inbuf,int length,struct nmb_packet *nmb)
{
	int nm_flags,offset;

	memset((char *)nmb,'\0',sizeof(*nmb));

	if (length < 12)
		return(False);

	/* parse the header */
	nmb->header.name_trn_id = RSVAL(inbuf,0);

	DEBUG(10,("parse_nmb: packet id = %d\n", nmb->header.name_trn_id));

	nmb->header.opcode = (CVAL(inbuf,2) >> 3) & 0xF;
	nmb->header.response = ((CVAL(inbuf,2)>>7)&1)?True:False;
	nm_flags = ((CVAL(inbuf,2) & 0x7) << 4) + (CVAL(inbuf,3)>>4);
	nmb->header.nm_flags.bcast = (nm_flags&1)?True:False;
	nmb->header.nm_flags.recursion_available = (nm_flags&8)?True:False;
	nmb->header.nm_flags.recursion_desired = (nm_flags&0x10)?True:False;
	nmb->header.nm_flags.trunc = (nm_flags&0x20)?True:False;
	nmb->header.nm_flags.authoritative = (nm_flags&0x40)?True:False;  
	nmb->header.rcode = CVAL(inbuf,3) & 0xF;
	nmb->header.qdcount = RSVAL(inbuf,4);
	nmb->header.ancount = RSVAL(inbuf,6);
	nmb->header.nscount = RSVAL(inbuf,8);
	nmb->header.arcount = RSVAL(inbuf,10);
  
	if (nmb->header.qdcount) {
		offset = parse_nmb_name(inbuf,12,length,&nmb->question.question_name);
		if (!offset)
			return(False);

		if (length - (12+offset) < 4)
			return(False);
		nmb->question.question_type = RSVAL(inbuf,12+offset);
		nmb->question.question_class = RSVAL(inbuf,12+offset+2);

		offset += 12+4;
	} else {
		offset = 12;
	}

	/* and any resource records */
	if (nmb->header.ancount && !parse_alloc_res_rec(inbuf,&offset,length,&nmb->answers,
					nmb->header.ancount))
		return(False);

	if (nmb->header.nscount && !parse_alloc_res_rec(inbuf,&offset,length,&nmb->nsrecs,
					nmb->header.nscount))
		return(False);
  
	if (nmb->header.arcount && !parse_alloc_res_rec(inbuf,&offset,length,&nmb->additional,
					nmb->header.arcount))
		return(False);

	return(True);
}

/*******************************************************************
 'Copy constructor' for an nmb packet.
******************************************************************/

static struct packet_struct *copy_nmb_packet(struct packet_struct *packet)
{  
	struct nmb_packet *nmb;
	struct nmb_packet *copy_nmb;
	struct packet_struct *pkt_copy;

	if(( pkt_copy = SMB_MALLOC_P(struct packet_struct)) == NULL) {
		DEBUG(0,("copy_nmb_packet: malloc fail.\n"));
		return NULL;
	}

	/* Structure copy of entire thing. */

	*pkt_copy = *packet;

	/* Ensure this copy is not locked. */
	pkt_copy->locked = False;

	/* Ensure this copy has no resource records. */
	nmb = &packet->packet.nmb;
	copy_nmb = &pkt_copy->packet.nmb;

	copy_nmb->answers = NULL;
	copy_nmb->nsrecs = NULL;
	copy_nmb->additional = NULL;

	/* Now copy any resource records. */

	if (nmb->answers) {
		if((copy_nmb->answers = SMB_MALLOC_ARRAY(struct res_rec,nmb->header.ancount)) == NULL)
			goto free_and_exit;
		memcpy((char *)copy_nmb->answers, (char *)nmb->answers, 
				nmb->header.ancount * sizeof(struct res_rec));
	}
	if (nmb->nsrecs) {
		if((copy_nmb->nsrecs = SMB_MALLOC_ARRAY(struct res_rec, nmb->header.nscount)) == NULL)
			goto free_and_exit;
		memcpy((char *)copy_nmb->nsrecs, (char *)nmb->nsrecs, 
				nmb->header.nscount * sizeof(struct res_rec));
	}
	if (nmb->additional) {
		if((copy_nmb->additional = SMB_MALLOC_ARRAY(struct res_rec, nmb->header.arcount)) == NULL)
			goto free_and_exit;
		memcpy((char *)copy_nmb->additional, (char *)nmb->additional, 
				nmb->header.arcount * sizeof(struct res_rec));
	}

	return pkt_copy;

 free_and_exit:

	SAFE_FREE(copy_nmb->answers);
	SAFE_FREE(copy_nmb->nsrecs);
	SAFE_FREE(copy_nmb->additional);
	SAFE_FREE(pkt_copy);

	DEBUG(0,("copy_nmb_packet: malloc fail in resource records.\n"));
	return NULL;
}

/*******************************************************************
  'Copy constructor' for a dgram packet.
******************************************************************/

static struct packet_struct *copy_dgram_packet(struct packet_struct *packet)
{ 
	struct packet_struct *pkt_copy;

	if(( pkt_copy = SMB_MALLOC_P(struct packet_struct)) == NULL) {
		DEBUG(0,("copy_dgram_packet: malloc fail.\n"));
		return NULL;
	}

	/* Structure copy of entire thing. */

	*pkt_copy = *packet;

	/* Ensure this copy is not locked. */
	pkt_copy->locked = False;

	/* There are no additional pointers in a dgram packet,
		we are finished. */
	return pkt_copy;
}

/*******************************************************************
 'Copy constructor' for a generic packet.
******************************************************************/

struct packet_struct *copy_packet(struct packet_struct *packet)
{  
	if(packet->packet_type == NMB_PACKET)
		return copy_nmb_packet(packet);
	else if (packet->packet_type == DGRAM_PACKET)
		return copy_dgram_packet(packet);
	return NULL;
}
 
/*******************************************************************
 Free up any resources associated with an nmb packet.
******************************************************************/

static void free_nmb_packet(struct nmb_packet *nmb)
{  
	SAFE_FREE(nmb->answers);
	SAFE_FREE(nmb->nsrecs);
	SAFE_FREE(nmb->additional);
}

/*******************************************************************
 Free up any resources associated with a dgram packet.
******************************************************************/

static void free_dgram_packet(struct dgram_packet *nmb)
{  
	/* We have nothing to do for a dgram packet. */
}

/*******************************************************************
 Free up any resources associated with a packet.
******************************************************************/

void free_packet(struct packet_struct *packet)
{  
	if (packet->locked) 
		return;
	if (packet->packet_type == NMB_PACKET)
		free_nmb_packet(&packet->packet.nmb);
	else if (packet->packet_type == DGRAM_PACKET)
		free_dgram_packet(&packet->packet.dgram);
	ZERO_STRUCTPN(packet);
	SAFE_FREE(packet);
}

/*******************************************************************
 Parse a packet buffer into a packet structure.
******************************************************************/

struct packet_struct *parse_packet(char *buf,int length,
				   enum packet_type packet_type)
{
	struct packet_struct *p;
	BOOL ok=False;

	p = SMB_MALLOC_P(struct packet_struct);
	if (!p)
		return(NULL);

	p->next = NULL;
	p->prev = NULL;
	p->ip = lastip;
	p->port = lastport;
	p->locked = False;
	p->timestamp = time(NULL);
	p->packet_type = packet_type;

	switch (packet_type) {
	case NMB_PACKET:
		ok = parse_nmb(buf,length,&p->packet.nmb);
		break;
		
	case DGRAM_PACKET:
		ok = parse_dgram(buf,length,&p->packet.dgram);
		break;
	}

	if (!ok) {
		free_packet(p);
		return NULL;
	}

	return p;
}

/*******************************************************************
 Read a packet from a socket and parse it, returning a packet ready
 to be used or put on the queue. This assumes a UDP socket.
******************************************************************/

struct packet_struct *read_packet(int fd,enum packet_type packet_type)
{
	struct packet_struct *packet;
	char buf[MAX_DGRAM_SIZE];
	int length;
	
	length = read_udp_socket(fd,buf,sizeof(buf));
	if (length < MIN_DGRAM_SIZE)
		return(NULL);
	
	packet = parse_packet(buf, length, packet_type);
	if (!packet)
		return NULL;

	packet->fd = fd;
	
	num_good_receives++;
	
	DEBUG(5,("Received a packet of len %d from (%s) port %d\n",
		 length, inet_ntoa(packet->ip), packet->port ) );
	
	return(packet);
}
					 
/*******************************************************************
 Send a udp packet on a already open socket.
******************************************************************/

static BOOL send_udp(int fd,char *buf,int len,struct in_addr ip,int port)
{
	BOOL ret = False;
	int i;
	struct sockaddr_in sock_out;

	/* set the address and port */
	memset((char *)&sock_out,'\0',sizeof(sock_out));
	putip((char *)&sock_out.sin_addr,(char *)&ip);
	sock_out.sin_port = htons( port );
	sock_out.sin_family = AF_INET;
  
	DEBUG( 5, ( "Sending a packet of len %d to (%s) on port %d\n",
			len, inet_ntoa(ip), port ) );

	/*
	 * Patch to fix asynch error notifications from Linux kernel.
	 */
	
	for (i = 0; i < 5; i++) {
		ret = (sendto(fd,buf,len,0,(struct sockaddr *)&sock_out, sizeof(sock_out)) >= 0);
		if (ret || errno != ECONNREFUSED)
			break;
	}

	if (!ret)
		DEBUG(0,("Packet send failed to %s(%d) ERRNO=%s\n",
			inet_ntoa(ip),port,strerror(errno)));

	if (ret)
		num_good_sends++;

	return(ret);
}

/*******************************************************************
 Build a dgram packet ready for sending.

 XXXX This currently doesn't handle packets too big for one
 datagram. It should split them and use the packet_offset, more and
 first flags to handle the fragmentation. Yuck.

   [...but it isn't clear that we would ever need to send a
   a fragmented NBT Datagram.  The IP layer does its own
   fragmentation to ensure that messages can fit into the path
   MTU.  It *is* important to be able to receive and rebuild
   fragmented NBT datagrams, just in case someone out there
   really has implemented this 'feature'.  crh -)------ ]

******************************************************************/

static int build_dgram(char *buf,struct packet_struct *p)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	unsigned char *ubuf = (unsigned char *)buf;
	int offset=0;

	/* put in the header */
	ubuf[0] = dgram->header.msg_type;
	ubuf[1] = (((int)dgram->header.flags.node_type)<<2);
	if (dgram->header.flags.more)
		ubuf[1] |= 1;
	if (dgram->header.flags.first)
		ubuf[1] |= 2;
	RSSVAL(ubuf,2,dgram->header.dgm_id);
	putip(ubuf+4,(char *)&dgram->header.source_ip);
	RSSVAL(ubuf,8,dgram->header.source_port);
	RSSVAL(ubuf,12,dgram->header.packet_offset);

	offset = 14;

	if (dgram->header.msg_type == 0x10 ||
			dgram->header.msg_type == 0x11 ||
			dgram->header.msg_type == 0x12) {      
		offset += put_nmb_name((char *)ubuf,offset,&dgram->source_name);
		offset += put_nmb_name((char *)ubuf,offset,&dgram->dest_name);
	}

	memcpy(ubuf+offset,dgram->data,dgram->datasize);
	offset += dgram->datasize;

	/* automatically set the dgm_length
	 * NOTE: RFC1002 says the dgm_length does *not*
	 *       include the fourteen-byte header. crh
	 */
	dgram->header.dgm_length = (offset - 14);
	RSSVAL(ubuf,10,dgram->header.dgm_length); 

	return(offset);
}

/*******************************************************************
 Build a nmb name
*******************************************************************/

void make_nmb_name( struct nmb_name *n, const char *name, int type)
{
	fstring unix_name;
	memset( (char *)n, '\0', sizeof(struct nmb_name) );
	fstrcpy(unix_name, name);
	strupper_m(unix_name);
	push_ascii(n->name, unix_name, sizeof(n->name), STR_TERMINATE);
	n->name_type = (unsigned int)type & 0xFF;
	push_ascii(n->scope,  global_scope(), 64, STR_TERMINATE);
}

/*******************************************************************
  Compare two nmb names
******************************************************************/

BOOL nmb_name_equal(struct nmb_name *n1, struct nmb_name *n2)
{
	return ((n1->name_type == n2->name_type) &&
		strequal(n1->name ,n2->name ) &&
		strequal(n1->scope,n2->scope));
}

/*******************************************************************
 Build a nmb packet ready for sending.

 XXXX this currently relies on not being passed something that expands
 to a packet too big for the buffer. Eventually this should be
 changed to set the trunc bit so the receiver can request the rest
 via tcp (when that becomes supported)
******************************************************************/

static int build_nmb(char *buf,struct packet_struct *p)
{
	struct nmb_packet *nmb = &p->packet.nmb;
	unsigned char *ubuf = (unsigned char *)buf;
	int offset=0;

	/* put in the header */
	RSSVAL(ubuf,offset,nmb->header.name_trn_id);
	ubuf[offset+2] = (nmb->header.opcode & 0xF) << 3;
	if (nmb->header.response)
		ubuf[offset+2] |= (1<<7);
	if (nmb->header.nm_flags.authoritative && 
			nmb->header.response)
		ubuf[offset+2] |= 0x4;
	if (nmb->header.nm_flags.trunc)
		ubuf[offset+2] |= 0x2;
	if (nmb->header.nm_flags.recursion_desired)
		ubuf[offset+2] |= 0x1;
	if (nmb->header.nm_flags.recursion_available &&
			nmb->header.response)
		ubuf[offset+3] |= 0x80;
	if (nmb->header.nm_flags.bcast)
		ubuf[offset+3] |= 0x10;
	ubuf[offset+3] |= (nmb->header.rcode & 0xF);

	RSSVAL(ubuf,offset+4,nmb->header.qdcount);
	RSSVAL(ubuf,offset+6,nmb->header.ancount);
	RSSVAL(ubuf,offset+8,nmb->header.nscount);
	RSSVAL(ubuf,offset+10,nmb->header.arcount);
  
	offset += 12;
	if (nmb->header.qdcount) {
		/* XXXX this doesn't handle a qdcount of > 1 */
		offset += put_nmb_name((char *)ubuf,offset,&nmb->question.question_name);
		RSSVAL(ubuf,offset,nmb->question.question_type);
		RSSVAL(ubuf,offset+2,nmb->question.question_class);
		offset += 4;
	}

	if (nmb->header.ancount)
		offset += put_res_rec((char *)ubuf,offset,nmb->answers,
				nmb->header.ancount);

	if (nmb->header.nscount)
		offset += put_res_rec((char *)ubuf,offset,nmb->nsrecs,
				nmb->header.nscount);

	/*
	 * The spec says we must put compressed name pointers
	 * in the following outgoing packets :
	 * NAME_REGISTRATION_REQUEST, NAME_REFRESH_REQUEST,
	 * NAME_RELEASE_REQUEST.
	 */

	if((nmb->header.response == False) &&
			((nmb->header.opcode == NMB_NAME_REG_OPCODE) ||
			(nmb->header.opcode == NMB_NAME_RELEASE_OPCODE) ||
			(nmb->header.opcode == NMB_NAME_REFRESH_OPCODE_8) ||
			(nmb->header.opcode == NMB_NAME_REFRESH_OPCODE_9) ||
			(nmb->header.opcode == NMB_NAME_MULTIHOMED_REG_OPCODE)) &&
			(nmb->header.arcount == 1)) {

		offset += put_compressed_name_ptr(ubuf,offset,nmb->additional,12);

	} else if (nmb->header.arcount) {
		offset += put_res_rec((char *)ubuf,offset,nmb->additional,
			nmb->header.arcount);  
	}
	return(offset);
}

/*******************************************************************
 Linearise a packet.
******************************************************************/

int build_packet(char *buf, struct packet_struct *p)
{
	int len = 0;

	switch (p->packet_type) {
	case NMB_PACKET:
		len = build_nmb(buf,p);
		break;

	case DGRAM_PACKET:
		len = build_dgram(buf,p);
		break;
	}

	return len;
}

/*******************************************************************
 Send a packet_struct.
******************************************************************/

BOOL send_packet(struct packet_struct *p)
{
	char buf[1024];
	int len=0;

	memset(buf,'\0',sizeof(buf));

	len = build_packet(buf, p);

	if (!len)
		return(False);

	return(send_udp(p->fd,buf,len,p->ip,p->port));
}

/****************************************************************************
 Receive a packet with timeout on a open UDP filedescriptor.
 The timeout is in milliseconds
***************************************************************************/

struct packet_struct *receive_packet(int fd,enum packet_type type,int t)
{
	fd_set fds;
	struct timeval timeout;
	int ret;

	FD_ZERO(&fds);
	FD_SET(fd,&fds);
	timeout.tv_sec = t/1000;
	timeout.tv_usec = 1000*(t%1000);

	if ((ret = sys_select_intr(fd+1,&fds,NULL,NULL,&timeout)) == -1) {
		/* errno should be EBADF or EINVAL. */
		DEBUG(0,("select returned -1, errno = %s (%d)\n", strerror(errno), errno));
		return NULL;
	}

	if (ret == 0) /* timeout */
		return NULL;

	if (FD_ISSET(fd,&fds)) 
		return(read_packet(fd,type));
	
	return(NULL);
}

/****************************************************************************
 Receive a UDP/137 packet either via UDP or from the unexpected packet
 queue. The packet must be a reply packet and have the specified trn_id.
 The timeout is in milliseconds.
***************************************************************************/

struct packet_struct *receive_nmb_packet(int fd, int t, int trn_id)
{
	struct packet_struct *p;

	p = receive_packet(fd, NMB_PACKET, t);

	if (p && p->packet.nmb.header.response &&
			p->packet.nmb.header.name_trn_id == trn_id) {
		return p;
	}
	if (p)
		free_packet(p);

	/* try the unexpected packet queue */
	return receive_unexpected(NMB_PACKET, trn_id, NULL);
}

/****************************************************************************
 Receive a UDP/138 packet either via UDP or from the unexpected packet
 queue. The packet must be a reply packet and have the specified mailslot name
 The timeout is in milliseconds.
***************************************************************************/

struct packet_struct *receive_dgram_packet(int fd, int t, const char *mailslot_name)
{
	struct packet_struct *p;

	p = receive_packet(fd, DGRAM_PACKET, t);

	if (p && match_mailslot_name(p, mailslot_name)) {
		return p;
	}
	if (p)
		free_packet(p);

	/* try the unexpected packet queue */
	return receive_unexpected(DGRAM_PACKET, 0, mailslot_name);
}

/****************************************************************************
 See if a datagram has the right mailslot name.
***************************************************************************/

BOOL match_mailslot_name(struct packet_struct *p, const char *mailslot_name)
{
	struct dgram_packet *dgram = &p->packet.dgram;
	char *buf;

	buf = &dgram->data[0];
	buf -= 4;

	buf = smb_buf(buf);

	if (memcmp(buf, mailslot_name, strlen(mailslot_name)+1) == 0) {
		return True;
	}

	return False;
}

/****************************************************************************
 Return the number of bits that match between two 4 character buffers
***************************************************************************/

int matching_quad_bits(unsigned char *p1, unsigned char *p2)
{
	int i, j, ret = 0;
	for (i=0; i<4; i++) {
		if (p1[i] != p2[i])
			break;
		ret += 8;
	}

	if (i==4)
		return ret;

	for (j=0; j<8; j++) {
		if ((p1[i] & (1<<(7-j))) != (p2[i] & (1<<(7-j))))
			break;
		ret++;
	}	
	
	return ret;
}

static unsigned char sort_ip[4];

/****************************************************************************
 Compare two query reply records.
***************************************************************************/

static int name_query_comp(unsigned char *p1, unsigned char *p2)
{
	return matching_quad_bits(p2+2, sort_ip) - matching_quad_bits(p1+2, sort_ip);
}

/****************************************************************************
 Sort a set of 6 byte name query response records so that the IPs that
 have the most leading bits in common with the specified address come first.
***************************************************************************/

void sort_query_replies(char *data, int n, struct in_addr ip)
{
	if (n <= 1)
		return;

	putip(sort_ip, (char *)&ip);

	qsort(data, n, 6, QSORT_CAST name_query_comp);
}

/*******************************************************************
 Convert, possibly using a stupid microsoft-ism which has destroyed
 the transport independence of netbios (for CIFS vendors that usually
 use the Win95-type methods, not for NT to NT communication, which uses
 DCE/RPC and therefore full-length unicode strings...) a dns name into
 a netbios name.

 The netbios name (NOT necessarily null-terminated) is truncated to 15
 characters.

 ******************************************************************/

char *dns_to_netbios_name(const char *dns_name)
{
	static nstring netbios_name;
	int i;
	StrnCpy(netbios_name, dns_name, MAX_NETBIOSNAME_LEN-1);
	netbios_name[15] = 0;
	
	/* ok.  this is because of a stupid microsoft-ism.  if the called host
	   name contains a '.', microsoft clients expect you to truncate the
	   netbios name up to and including the '.'  this even applies, by
	   mistake, to workgroup (domain) names, which is _really_ daft.
	 */
	for (i = 0; i < 15; i++) {
		if (netbios_name[i] == '.') {
			netbios_name[i] = 0;
			break;
		}
	}

	return netbios_name;
}

/****************************************************************************
 Interpret the weird netbios "name" into a unix fstring. Return the name type.
****************************************************************************/

static int name_interpret(char *in, fstring name)
{
	int ret;
	int len = (*in++) / 2;
	fstring out_string;
	char *out = out_string;

	*out=0;

	if (len > 30 || len<1)
		return(0);

	while (len--) {
		if (in[0] < 'A' || in[0] > 'P' || in[1] < 'A' || in[1] > 'P') {
			*out = 0;
			return(0);
		}
		*out = ((in[0]-'A')<<4) + (in[1]-'A');
		in += 2;
		out++;
	}
	ret = out[-1];
	out[-1] = 0;

#ifdef NETBIOS_SCOPE
	/* Handle any scope names */
	while(*in) {
		*out++ = '.'; /* Scope names are separated by periods */
		len = *(unsigned char *)in++;
		StrnCpy(out, in, len);
		out += len;
		*out=0;
		in += len;
	}
#endif
	pull_ascii_fstring(name, out_string);

	return(ret);
}

/****************************************************************************
 Mangle a name into netbios format.
 Note:  <Out> must be (33 + strlen(scope) + 2) bytes long, at minimum.
****************************************************************************/

int name_mangle( char *In, char *Out, char name_type )
{
	int   i;
	int   len;
	nstring buf;
	char *p = Out;

	/* Safely copy the input string, In, into buf[]. */
	if (strcmp(In,"*") == 0)
		put_name(buf, "*", '\0', 0x00);
	else {
		/* We use an fstring here as mb dos names can expend x3 when
		   going to utf8. */
		fstring buf_unix;
		nstring buf_dos;

		pull_ascii_fstring(buf_unix, In);
		strupper_m(buf_unix);

		push_ascii_nstring(buf_dos, buf_unix);
		put_name(buf, buf_dos, ' ', name_type);
	}

	/* Place the length of the first field into the output buffer. */
	p[0] = 32;
	p++;

	/* Now convert the name to the rfc1001/1002 format. */
	for( i = 0; i < MAX_NETBIOSNAME_LEN; i++ ) {
		p[i*2]     = ( (buf[i] >> 4) & 0x000F ) + 'A';
		p[(i*2)+1] = (buf[i] & 0x000F) + 'A';
	}
	p += 32;
	p[0] = '\0';

	/* Add the scope string. */
	for( i = 0, len = 0; *(global_scope()) != '\0'; i++, len++ ) {
		switch( (global_scope())[i] ) {
			case '\0':
				p[0] = len;
				if( len > 0 )
					p[len+1] = 0;
				return( name_len(Out) );
			case '.':
				p[0] = len;
				p   += (len + 1);
				len  = -1;
				break;
			default:
				p[len+1] = (global_scope())[i];
				break;
		}
	}

	return( name_len(Out) );
}

/****************************************************************************
 Find a pointer to a netbios name.
****************************************************************************/

static char *name_ptr(char *buf,int ofs)
{
	unsigned char c = *(unsigned char *)(buf+ofs);

	if ((c & 0xC0) == 0xC0) {
		uint16 l = RSVAL(buf, ofs) & 0x3FFF;
		DEBUG(5,("name ptr to pos %d from %d is %s\n",l,ofs,buf+l));
		return(buf + l);
	} else {
		return(buf+ofs);
	}
}  

/****************************************************************************
 Extract a netbios name from a buf (into a unix string) return name type.
****************************************************************************/

int name_extract(char *buf,int ofs, fstring name)
{
	char *p = name_ptr(buf,ofs);
	int d = PTR_DIFF(p,buf+ofs);

	name[0] = '\0';
	if (d < -50 || d > 50)
		return(0);
	return(name_interpret(p,name));
}
  
/****************************************************************************
 Return the total storage length of a mangled name.
****************************************************************************/

int name_len(char *s1)
{
	/* NOTE: this argument _must_ be unsigned */
	unsigned char *s = (unsigned char *)s1;
	int len;

	/* If the two high bits of the byte are set, return 2. */
	if (0xC0 == (*s & 0xC0))
		return(2);

	/* Add up the length bytes. */
	for (len = 1; (*s); s += (*s) + 1) {
		len += *s + 1;
		SMB_ASSERT(len < 80);
	}

	return(len);
}

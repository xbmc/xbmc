/* 
   Unix SMB/CIFS implementation.
   Utility to extract pcap files from samba (log level 10) log files

   Copyright (C) Jelmer Vernooij 2003
   Thanks to Tim Potter for the genial idea

   Portions (from capconvert.c) (C) Andrew Tridgell 1997
   Portions (from text2pcap.c) (C) Ashok Narayanan 2001

   Example use with -h parameter: 
   	log2pcaphex < samba-log-file | text2pcap -T 139,139 - foo.pcap

   TODO: Have correct IP and TCP checksums.

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

/* We don't care about the paranoid malloc checker in this standalone
   program */
#undef malloc

#include <assert.h>

int quiet = 0;
int hexformat = 0;

#define itoa(a) ((a) < 0xa?'0'+(a):'A' + (a-0xa))

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>

#define TCPDUMP_MAGIC 0xa1b2c3d4

/* tcpdump file format */
struct tcpdump_file_header {
	uint32 magic;
	uint16 major;
	uint16 minor;
	int32 zone;
	uint32 sigfigs;
	uint32 snaplen;
	uint32 linktype;
};

struct tcpdump_packet {
	struct timeval ts;
	uint32 caplen;
	uint32 len;
};

typedef struct {
    uint8  ver_hdrlen;
    uint8  dscp;
    uint16 packet_length;
    uint16 identification;
    uint8  flags;
    uint8  fragment;
    uint8  ttl;
    uint8  protocol;
    uint16 hdr_checksum;
    uint32 src_addr;
    uint32 dest_addr;
} hdr_ip_t;

static hdr_ip_t HDR_IP = {0x45, 0, 0, 0x3412, 0, 0, 0xff, 6, 0, 0x01010101, 0x02020202};

typedef struct {
    uint16 source_port;
    uint16 dest_port;
    uint32 seq_num;
    uint32 ack_num;
    uint8  hdr_length;
    uint8  flags;
    uint16 window;
    uint16 checksum;
    uint16 urg;
} hdr_tcp_t;

static hdr_tcp_t HDR_TCP = {139, 139, 0, 0, 0x50, 0, 0, 0, 0};

void print_pcap_header(FILE *out)
{
	struct tcpdump_file_header h;
	h.magic = TCPDUMP_MAGIC;
	h.major = 2;
	h.minor = 4;
	h.zone = 0;
	h.sigfigs = 0;
	h.snaplen = 102400; /* As long packets as possible */
	h.linktype = 101; /* Raw IP */
	fwrite(&h, sizeof(struct tcpdump_file_header), 1, out);
}

void print_pcap_packet(FILE *out, unsigned char *data, long length, long caplen)
{
	static int i = 0;
	struct tcpdump_packet p;
	i++;
	p.ts.tv_usec = 0;
	p.ts.tv_sec = 0;
	p.caplen = caplen;
	p.len = length;
	fwrite(&p, sizeof(struct tcpdump_packet), 1, out);
	fwrite(data, sizeof(unsigned char), caplen, out);
}

void print_hex_packet(FILE *out, unsigned char *data, long length)
{
	long i,cur = 0;
	while(cur < length) {
		fprintf(out, "%06lX ", cur);
		for(i = cur; i < length && i < cur + 16; i++) {
			fprintf(out, "%02x ", data[i]);
		}
	
		cur = i;
		fprintf(out, "\n");
	}
}

void print_netbios_packet(FILE *out, unsigned char *data, long length, long actual_length)
{	
	unsigned char *newdata; long offset = 0;
	long newlen;
	
	newlen = length+sizeof(HDR_IP)+sizeof(HDR_TCP);
	newdata = malloc(newlen);

	HDR_IP.packet_length = htons(newlen);
	HDR_TCP.window = htons(0x2000);
	HDR_TCP.source_port = HDR_TCP.dest_port = htons(139);

	memcpy(newdata+offset, &HDR_IP, sizeof(HDR_IP));offset+=sizeof(HDR_IP);
	memcpy(newdata+offset, &HDR_TCP, sizeof(HDR_TCP));offset+=sizeof(HDR_TCP);
	memcpy(newdata+offset,data,length);
	
	print_pcap_packet(out, newdata, newlen, actual_length+offset);
	free(newdata);
}

unsigned char *curpacket = NULL;
long curpacket_len = 0;

void read_log_msg(FILE *in, unsigned char **_buffer, long *buffersize, long *data_offset, long *data_length)
{
	unsigned char *buffer;
	int tmp; long i;
	assert(fscanf(in, " size=%ld\n", buffersize));
	*buffersize+=4; /* for netbios */
	buffer = malloc(*buffersize);
	memset(buffer, 0, *buffersize);
	/* NetBIOS */
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	memcpy(buffer+2, &buffersize, 2);
	buffer[4] = 0xFF;
	buffer[5] = 'S';
	buffer[6] = 'M';
	buffer[7] = 'B';
	assert(fscanf(in, "  smb_com=0x%x\n", &tmp)); buffer[smb_com] = tmp;
	assert(fscanf(in, "  smb_rcls=%d\n", &tmp)); buffer[smb_rcls] = tmp;
	assert(fscanf(in, "  smb_reh=%d\n", &tmp)); buffer[smb_reh] = tmp;
	assert(fscanf(in, "  smb_err=%d\n", &tmp)); memcpy(buffer+smb_err, &tmp, 2);
	assert(fscanf(in, "  smb_flg=%d\n", &tmp)); buffer[smb_flg] = tmp;
	assert(fscanf(in, "  smb_flg2=%d\n", &tmp)); memcpy(buffer+smb_flg2, &tmp, 2);
	assert(fscanf(in, "  smb_tid=%d\n", &tmp)); memcpy(buffer+smb_tid, &tmp, 2);
	assert(fscanf(in, "  smb_pid=%d\n", &tmp)); memcpy(buffer+smb_pid, &tmp, 2);
	assert(fscanf(in, "  smb_uid=%d\n", &tmp)); memcpy(buffer+smb_uid, &tmp, 2);
	assert(fscanf(in, "  smb_mid=%d\n", &tmp)); memcpy(buffer+smb_mid, &tmp, 2);
	assert(fscanf(in, "  smt_wct=%d\n", &tmp)); buffer[smb_wct] = tmp;
	for(i = 0; i < buffer[smb_wct]; i++) {
		assert(fscanf(in, "  smb_vwv[%*2d]=%*5d (0x%X)\n", &tmp));
		memcpy(buffer+smb_vwv+i*2, &tmp, 2);
	}

	*data_offset = smb_vwv+buffer[smb_wct]*2;
	assert(fscanf(in, "  smb_bcc=%ld\n", data_length)); buffer[(*data_offset)] = *data_length;
	(*data_offset)+=2;
	*_buffer = buffer;
}

long read_log_data(FILE *in, unsigned char *buffer, long data_length)
{
	long i, addr; char real[2][16]; int ret;
	unsigned int tmp;
	for(i = 0; i < data_length; i++) {
		if(i % 16 == 0){
			if(i != 0) { /* Read data after each line */
				assert(fscanf(in, "%8s %8s", real[0], real[1]) == 2);
			}
			ret = fscanf(in, "  [%03lX]", &addr);
			if(!ret) {
				if(!quiet)fprintf(stderr, "Only first %ld bytes are logged, packet trace will be incomplete\nTry a higher log level\n", i);
				return i-1;
			}
			assert(addr == i);
		}
		if(!fscanf(in, "%02X", &tmp)) {
			if(!quiet)fprintf(stderr, "Only first %ld bytes are logged, packet trace will be incomplete\nTry a higher log level\n", i-1);
			return i-1;
		}
		buffer[i] = tmp;
	}
	return data_length;
}

int main (int argc, char **argv)
{
	const char *infile, *outfile;
	FILE *out, *in;
	int opt;
	poptContext pc;
	char buffer[4096];
	long data_offset, data_length;
	long data_bytes_read = 0;
	int in_packet = 0;
	struct poptOption long_options[] = {
		POPT_AUTOHELP
		{ "quiet", 'q', POPT_ARG_NONE, &quiet, 0, "Be quiet, don't output warnings" },
		{ "hex", 'h', POPT_ARG_NONE, &hexformat, 0, "Output format readable by text2pcap" },
		POPT_TABLEEND
	};
	
	pc = poptGetContext(NULL, argc, (const char **) argv, long_options,
			    POPT_CONTEXT_KEEP_FIRST);
	poptSetOtherOptionHelp(pc, "[<infile> [<outfile>]]");
	
	
	while((opt = poptGetNextOpt(pc)) != -1) {
		switch (opt) {
		}
	}

	poptGetArg(pc); /* Drop argv[0], the program name */

	infile = poptGetArg(pc);

	if(infile) {
		in  = fopen(infile, "r");
		if(!in) {
			perror("fopen");
			return 1;
		}
	} else in = stdin;
	
	outfile = poptGetArg(pc);

	if(outfile) {
		out = fopen(outfile, "w+");
		if(!out) { 
			perror("fopen"); 
			fprintf(stderr, "Can't find %s, using stdout...\n", outfile);
		}
	}

	if(!outfile) out = stdout;

	if(!hexformat)print_pcap_header(out);

	while(!feof(in)) {
		fgets(buffer, sizeof(buffer), in);
		if(buffer[0] == '[') { /* Header */
			if(strstr(buffer, "show_msg")) {
				in_packet++;
				if(in_packet == 1)continue;
				read_log_msg(in, &curpacket, &curpacket_len, &data_offset, &data_length);
			} else if(in_packet && strstr(buffer, "dump_data")) {
				data_bytes_read = read_log_data(in, curpacket+data_offset, data_length);
			}  else { 
				if(in_packet){ 
					if(hexformat) print_hex_packet(out, curpacket, curpacket_len); 
					else print_netbios_packet(out, curpacket, curpacket_len, data_bytes_read+data_offset);
					free(curpacket); 
				}
				in_packet = 0;
			}
		} 
	}

	return 0;
}

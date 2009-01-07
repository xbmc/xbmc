/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/***************************************************************
 name: ogg_dll  dll: ogg.dll 
***************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_VORBIS_DLL

#include <windows.h>
#include <ogg/ogg.h>

extern int load_ogg_dll(void);
extern void free_ogg_dll(void);

typedef void (*type_oggpack_writeinit)(oggpack_buffer *b);
typedef void (*type_oggpack_reset)(oggpack_buffer *b);
typedef void (*type_oggpack_writeclear)(oggpack_buffer *b);
typedef void (*type_oggpack_readinit)(oggpack_buffer *b,unsigned char *buf,int bytes);
typedef void (*type_oggpack_write)(oggpack_buffer *b,unsigned long value,int bits);
typedef long (*type_oggpack_look)(oggpack_buffer *b,int bits);
typedef long (*type_oggpack_look_huff)(oggpack_buffer *b,int bits);
typedef long (*type_oggpack_look1)(oggpack_buffer *b);
typedef void (*type_oggpack_adv)(oggpack_buffer *b,int bits);
typedef int  (*type_oggpack_adv_huff)(oggpack_buffer *b,int bits);
typedef void (*type_oggpack_adv1)(oggpack_buffer *b);
typedef long (*type_oggpack_read)(oggpack_buffer *b,int bits);
typedef long (*type_oggpack_read1)(oggpack_buffer *b);
typedef long (*type_oggpack_bytes)(oggpack_buffer *b);
typedef long (*type_oggpack_bits)(oggpack_buffer *b);
typedef unsigned char*(*type_oggpack_get_buffer)(oggpack_buffer *b);
typedef int     (*type_ogg_stream_packetin)(ogg_stream_state *os, ogg_packet *op);
typedef int     (*type_ogg_stream_pageout)(ogg_stream_state *os, ogg_page *og);
typedef int     (*type_ogg_stream_flush)(ogg_stream_state *os, ogg_page *og);
typedef int     (*type_ogg_sync_init)(ogg_sync_state *oy);
typedef int     (*type_ogg_sync_clear)(ogg_sync_state *oy);
typedef int     (*type_ogg_sync_reset)(ogg_sync_state *oy);
typedef int(*type_ogg_sync_destroy)(ogg_sync_state *oy);
typedef char   *(*type_ogg_sync_buffer)(ogg_sync_state *oy, long size);
typedef int     (*type_ogg_sync_wrote)(ogg_sync_state *oy, long bytes);
typedef long    (*type_ogg_sync_pageseek)(ogg_sync_state *oy,ogg_page *og);
typedef int     (*type_ogg_sync_pageout)(ogg_sync_state *oy, ogg_page *og);
typedef int     (*type_ogg_stream_pagein)(ogg_stream_state *os, ogg_page *og);
typedef int     (*type_ogg_stream_packetout)(ogg_stream_state *os,ogg_packet *op);
typedef int     (*type_ogg_stream_init)(ogg_stream_state *os,int serialno);
typedef int     (*type_ogg_stream_clear)(ogg_stream_state *os);
typedef int     (*type_ogg_stream_reset)(ogg_stream_state *os);
typedef int     (*type_ogg_stream_destroy)(ogg_stream_state *os);
typedef int     (*type_ogg_stream_eos)(ogg_stream_state *os);
typedef int     (*type_ogg_page_version)(ogg_page *og);
typedef int     (*type_ogg_page_continued)(ogg_page *og);
typedef int     (*type_ogg_page_bos)(ogg_page *og);
typedef int     (*type_ogg_page_eos)(ogg_page *og);
typedef ogg_int64_t (*type_ogg_page_granulepos)(ogg_page *og);
typedef int     (*type_ogg_page_serialno)(ogg_page *og);
typedef long    (*type_ogg_page_pageno)(ogg_page *og);
typedef int     (*type_ogg_page_packets)(ogg_page *og);
////typedef void    (*type_ogg_packet_clear)(ogg_packet *op);

static struct ogg_dll_ {
//	 type_oggpack_writeinit oggpack_writeinit;
//	 type_oggpack_reset oggpack_reset;
//	 type_oggpack_writeclear oggpack_writeclear;
//	 type_oggpack_readinit oggpack_readinit;
//	 type_oggpack_write oggpack_write;
//	 type_oggpack_look oggpack_look;
//	 type_oggpack_look_huff oggpack_look_huff;
//	 type_oggpack_look1 oggpack_look1;
//	 type_oggpack_adv oggpack_adv;
//	 type_oggpack_adv_huff oggpack_adv_huff;
//	 type_oggpack_adv1 oggpack_adv1;
//	 type_oggpack_read oggpack_read;
//	 type_oggpack_read1 oggpack_read1;
//	 type_oggpack_bytes oggpack_bytes;
//	 type_oggpack_bits oggpack_bits;
//	 type_oggpack_get_buffer oggpack_get_buffer;
	 type_ogg_stream_packetin ogg_stream_packetin;
	 type_ogg_stream_pageout ogg_stream_pageout;
//	 type_ogg_stream_flush ogg_stream_flush;
//	 type_ogg_sync_init ogg_sync_init;
//	 type_ogg_sync_clear ogg_sync_clear;
//	 type_ogg_sync_reset ogg_sync_reset;
//	 type_ogg_sync_destroy ogg_sync_destroy;
//	 type_ogg_sync_buffer ogg_sync_buffer;
//	 type_ogg_sync_wrote ogg_sync_wrote;
//	 type_ogg_sync_pageseek ogg_sync_pageseek;
//	 type_ogg_sync_pageout ogg_sync_pageout;
//	 type_ogg_stream_pagein ogg_stream_pagein;
//	 type_ogg_stream_packetout ogg_stream_packetout;
	 type_ogg_stream_init ogg_stream_init;
	 type_ogg_stream_clear ogg_stream_clear;
//	 type_ogg_stream_reset ogg_stream_reset;
//	 type_ogg_stream_destroy ogg_stream_destroy;
//	 type_ogg_stream_eos ogg_stream_eos;
//	 type_ogg_page_version ogg_page_version;
//	 type_ogg_page_continued ogg_page_continued;
//	 type_ogg_page_bos ogg_page_bos;
	 type_ogg_page_eos ogg_page_eos;
//	 type_ogg_page_granulepos ogg_page_granulepos;
//	 type_ogg_page_serialno ogg_page_serialno;
//	 type_ogg_page_pageno ogg_page_pageno;
//	 type_ogg_page_packets ogg_page_packets;
////	 type_ogg_packet_clear ogg_packet_clear;
} ogg_dll;

static volatile HANDLE h_ogg_dll = NULL;

void free_ogg_dll(void)
{
	if(h_ogg_dll){
		FreeLibrary(h_ogg_dll);
		h_ogg_dll = NULL;
	}
}

int load_ogg_dll(void)
{
	if(!h_ogg_dll){
		h_ogg_dll = LoadLibrary("ogg.dll");
		if(!h_ogg_dll) return -1;
	}
//	ogg_dll.oggpack_writeinit = (type_oggpack_writeinit)GetProcAddress(h_ogg_dll,"oggpack_writeinit");
//	if(!ogg_dll.oggpack_writeinit){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_reset = (type_oggpack_reset)GetProcAddress(h_ogg_dll,"oggpack_reset");
//	if(!ogg_dll.oggpack_reset){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_writeclear = (type_oggpack_writeclear)GetProcAddress(h_ogg_dll,"oggpack_writeclear");
//	if(!ogg_dll.oggpack_writeclear){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_readinit = (type_oggpack_readinit)GetProcAddress(h_ogg_dll,"oggpack_readinit");
//	if(!ogg_dll.oggpack_readinit){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_write = (type_oggpack_write)GetProcAddress(h_ogg_dll,"oggpack_write");
//	if(!ogg_dll.oggpack_write){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_look = (type_oggpack_look)GetProcAddress(h_ogg_dll,"oggpack_look");
//	if(!ogg_dll.oggpack_look){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_look_huff = (type_oggpack_look_huff)GetProcAddress(h_ogg_dll,"oggpack_look_huff");
//	if(!ogg_dll.oggpack_look_huff){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_look1 = (type_oggpack_look1)GetProcAddress(h_ogg_dll,"oggpack_look1");
//	if(!ogg_dll.oggpack_look1){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_adv = (type_oggpack_adv)GetProcAddress(h_ogg_dll,"oggpack_adv");
//	if(!ogg_dll.oggpack_adv){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_adv_huff = (type_oggpack_adv_huff)GetProcAddress(h_ogg_dll,"oggpack_adv_huff");
//	if(!ogg_dll.oggpack_adv_huff){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_adv1 = (type_oggpack_adv1)GetProcAddress(h_ogg_dll,"oggpack_adv1");
//	if(!ogg_dll.oggpack_adv1){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_read = (type_oggpack_read)GetProcAddress(h_ogg_dll,"oggpack_read");
//	if(!ogg_dll.oggpack_read){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_read1 = (type_oggpack_read1)GetProcAddress(h_ogg_dll,"oggpack_read1");
//	if(!ogg_dll.oggpack_read1){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_bytes = (type_oggpack_bytes)GetProcAddress(h_ogg_dll,"oggpack_bytes");
//	if(!ogg_dll.oggpack_bytes){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_bits = (type_oggpack_bits)GetProcAddress(h_ogg_dll,"oggpack_bits");
//	if(!ogg_dll.oggpack_bits){ free_ogg_dll(); return -1; }
//	ogg_dll.oggpack_get_buffer = (type_oggpack_get_buffer)GetProcAddress(h_ogg_dll,"oggpack_get_buffer");
//	if(!ogg_dll.oggpack_get_buffer){ free_ogg_dll(); return -1; }
	ogg_dll.ogg_stream_packetin = (type_ogg_stream_packetin)GetProcAddress(h_ogg_dll,"ogg_stream_packetin");
	if(!ogg_dll.ogg_stream_packetin){ free_ogg_dll(); return -1; }
	ogg_dll.ogg_stream_pageout = (type_ogg_stream_pageout)GetProcAddress(h_ogg_dll,"ogg_stream_pageout");
	if(!ogg_dll.ogg_stream_pageout){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_stream_flush = (type_ogg_stream_flush)GetProcAddress(h_ogg_dll,"ogg_stream_flush");
//	if(!ogg_dll.ogg_stream_flush){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_init = (type_ogg_sync_init)GetProcAddress(h_ogg_dll,"ogg_sync_init");
//	if(!ogg_dll.ogg_sync_init){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_clear = (type_ogg_sync_clear)GetProcAddress(h_ogg_dll,"ogg_sync_clear");
//	if(!ogg_dll.ogg_sync_clear){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_reset = (type_ogg_sync_reset)GetProcAddress(h_ogg_dll,"ogg_sync_reset");
//	if(!ogg_dll.ogg_sync_reset){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_destroy = (type_ogg_sync_destroy)GetProcAddress(h_ogg_dll,"ogg_sync_destroy");
//	if(!ogg_dll.ogg_sync_destroy){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_buffer = (type_ogg_sync_buffer)GetProcAddress(h_ogg_dll,"ogg_sync_buffer");
//	if(!ogg_dll.ogg_sync_buffer){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_wrote = (type_ogg_sync_wrote)GetProcAddress(h_ogg_dll,"ogg_sync_wrote");
//	if(!ogg_dll.ogg_sync_wrote){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_pageseek = (type_ogg_sync_pageseek)GetProcAddress(h_ogg_dll,"ogg_sync_pageseek");
//	if(!ogg_dll.ogg_sync_pageseek){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_sync_pageout = (type_ogg_sync_pageout)GetProcAddress(h_ogg_dll,"ogg_sync_pageout");
//	if(!ogg_dll.ogg_sync_pageout){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_stream_pagein = (type_ogg_stream_pagein)GetProcAddress(h_ogg_dll,"ogg_stream_pagein");
//	if(!ogg_dll.ogg_stream_pagein){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_stream_packetout = (type_ogg_stream_packetout)GetProcAddress(h_ogg_dll,"ogg_stream_packetout");
//	if(!ogg_dll.ogg_stream_packetout){ free_ogg_dll(); return -1; }
	ogg_dll.ogg_stream_init = (type_ogg_stream_init)GetProcAddress(h_ogg_dll,"ogg_stream_init");
	if(!ogg_dll.ogg_stream_init){ free_ogg_dll(); return -1; }
	ogg_dll.ogg_stream_clear = (type_ogg_stream_clear)GetProcAddress(h_ogg_dll,"ogg_stream_clear");
	if(!ogg_dll.ogg_stream_clear){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_stream_reset = (type_ogg_stream_reset)GetProcAddress(h_ogg_dll,"ogg_stream_reset");
//	if(!ogg_dll.ogg_stream_reset){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_stream_destroy = (type_ogg_stream_destroy)GetProcAddress(h_ogg_dll,"ogg_stream_destroy");
//	if(!ogg_dll.ogg_stream_destroy){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_stream_eos = (type_ogg_stream_eos)GetProcAddress(h_ogg_dll,"ogg_stream_eos");
//	if(!ogg_dll.ogg_stream_eos){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_page_version = (type_ogg_page_version)GetProcAddress(h_ogg_dll,"ogg_page_version");
//	if(!ogg_dll.ogg_page_version){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_page_continued = (type_ogg_page_continued)GetProcAddress(h_ogg_dll,"ogg_page_continued");
//	if(!ogg_dll.ogg_page_continued){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_page_bos = (type_ogg_page_bos)GetProcAddress(h_ogg_dll,"ogg_page_bos");
//	if(!ogg_dll.ogg_page_bos){ free_ogg_dll(); return -1; }
	ogg_dll.ogg_page_eos = (type_ogg_page_eos)GetProcAddress(h_ogg_dll,"ogg_page_eos");
	if(!ogg_dll.ogg_page_eos){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_page_granulepos = (type_ogg_page_granulepos)GetProcAddress(h_ogg_dll,"ogg_page_granulepos");
//	if(!ogg_dll.ogg_page_granulepos){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_page_serialno = (type_ogg_page_serialno)GetProcAddress(h_ogg_dll,"ogg_page_serialno");
//	if(!ogg_dll.ogg_page_serialno){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_page_pageno = (type_ogg_page_pageno)GetProcAddress(h_ogg_dll,"ogg_page_pageno");
//	if(!ogg_dll.ogg_page_pageno){ free_ogg_dll(); return -1; }
//	ogg_dll.ogg_page_packets = (type_ogg_page_packets)GetProcAddress(h_ogg_dll,"ogg_page_packets");
//	if(!ogg_dll.ogg_page_packets){ free_ogg_dll(); return -1; }
////	ogg_dll.ogg_packet_clear = (type_ogg_packet_clear)GetProcAddress(h_ogg_dll,"ogg_packet_clear");
////	if(!ogg_dll.ogg_packet_clear){ free_ogg_dll(); return -1; }
	return 0;
}

#if 0
void oggpack_writeinit(oggpack_buffer *b)
{
	if(h_ogg_dll){
		ogg_dll.oggpack_writeinit(b);
	}
}

void oggpack_reset(oggpack_buffer *b)
{
	if(h_ogg_dll){
		ogg_dll.oggpack_reset(b);
	}
}

void oggpack_writeclear(oggpack_buffer *b)
{
	if(h_ogg_dll){
		ogg_dll.oggpack_writeclear(b);
	}
}

void oggpack_readinit(oggpack_buffer *b,unsigned char *buf,int bytes)
{
	if(h_ogg_dll){
		ogg_dll.oggpack_readinit(b,buf,bytes);
	}
}

void oggpack_write(oggpack_buffer *b,unsigned long value,int bits)
{
	if(h_ogg_dll){
		ogg_dll.oggpack_write(b,value,bits);
	}
}

long oggpack_look(oggpack_buffer *b,int bits)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_look(b,bits);
	}
	return (long )0;
}

long oggpack_look_huff(oggpack_buffer *b,int bits)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_look_huff(b,bits);
	}
	return (long )0;
}

long oggpack_look1(oggpack_buffer *b)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_look1(b);
	}
	return (long )0;
}

void oggpack_adv(oggpack_buffer *b,int bits)
{
	if(h_ogg_dll){
		ogg_dll.oggpack_adv(b,bits);
	}
}

int  oggpack_adv_huff(oggpack_buffer *b,int bits)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_adv_huff(b,bits);
	}
	return (int  )0;
}

void oggpack_adv1(oggpack_buffer *b)
{
	if(h_ogg_dll){
		ogg_dll.oggpack_adv1(b);
	}
}

long oggpack_read(oggpack_buffer *b,int bits)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_read(b,bits);
	}
	return (long )0;
}

long oggpack_read1(oggpack_buffer *b)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_read1(b);
	}
	return (long )0;
}

long oggpack_bytes(oggpack_buffer *b)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_bytes(b);
	}
	return (long )0;
}

long oggpack_bits(oggpack_buffer *b)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_bits(b);
	}
	return (long )0;
}

unsigned char*oggpack_get_buffer(oggpack_buffer *b)
{
	if(h_ogg_dll){
		return ogg_dll.oggpack_get_buffer(b);
	}
	return (unsigned char*)0;
}
#endif

int     ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_packetin(os,op);
	}
	return (int     )0;
}

int     ogg_stream_pageout(ogg_stream_state *os, ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_pageout(os,og);
	}
	return (int     )0;
}

#if 0
int     ogg_stream_flush(ogg_stream_state *os, ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_flush(os,og);
	}
	return (int     )0;
}

int     ogg_sync_init(ogg_sync_state *oy)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_init(oy);
	}
	return (int     )0;
}

int     ogg_sync_clear(ogg_sync_state *oy)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_clear(oy);
	}
	return (int     )0;
}

int     ogg_sync_reset(ogg_sync_state *oy)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_reset(oy);
	}
	return (int     )0;
}

intogg_sync_destroy(ogg_sync_state *oy)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_destroy(oy);
	}
	return (int)0;
}

char   *ogg_sync_buffer(ogg_sync_state *oy, long size)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_buffer(oy,size);
	}
	return (char   *)0;
}

int     ogg_sync_wrote(ogg_sync_state *oy, long bytes)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_wrote(oy,bytes);
	}
	return (int     )0;
}

long    ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_pageseek(oy,og);
	}
	return (long    )0;
}

int     ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_sync_pageout(oy,og);
	}
	return (int     )0;
}

int     ogg_stream_pagein(ogg_stream_state *os, ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_pagein(os,og);
	}
	return (int     )0;
}

int     ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_packetout(os,op);
	}
	return (int     )0;
}
#endif

int     ogg_stream_init(ogg_stream_state *os,int serialno)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_init(os,serialno);
	}
	return (int     )0;
}

int     ogg_stream_clear(ogg_stream_state *os)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_clear(os);
	}
	return (int     )0;
}

#if 0
int     ogg_stream_reset(ogg_stream_state *os)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_reset(os);
	}
	return (int     )0;
}

int     ogg_stream_destroy(ogg_stream_state *os)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_destroy(os);
	}
	return (int     )0;
}

int     ogg_stream_eos(ogg_stream_state *os)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_stream_eos(os);
	}
	return (int     )0;
}

int     ogg_page_version(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_version(og);
	}
	return (int     )0;
}

int     ogg_page_continued(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_continued(og);
	}
	return (int     )0;
}

int     ogg_page_bos(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_bos(og);
	}
	return (int     )0;
}
#endif

int     ogg_page_eos(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_eos(og);
	}
	return (int     )0;
}

#if 0
ogg_int64_t ogg_page_granulepos(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_granulepos(og);
	}
	return (ogg_int64_t )0;
}

int     ogg_page_serialno(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_serialno(og);
	}
	return (int     )0;
}

long    ogg_page_pageno(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_pageno(og);
	}
	return (long    )0;
}

int     ogg_page_packets(ogg_page *og)
{
	if(h_ogg_dll){
		return ogg_dll.ogg_page_packets(og);
	}
	return (int     )0;
}
#endif

//void    ogg_packet_clear(ogg_packet *op)
//{
//	if(h_ogg_dll){
//		ogg_dll.ogg_packet_clear(op);
//	}
//}

/***************************************************************/
#endif /* AU_VORBIS_DLL */

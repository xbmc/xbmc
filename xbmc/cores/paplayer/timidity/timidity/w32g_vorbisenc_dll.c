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
 name: vorbisenc_dll  dll: vorbisenc.dll 
***************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_VORBIS_DLL

#include <windows.h>
#include <vorbis/vorbisenc.h>

extern int load_vorbisenc_dll(void);
extern void free_vorbisenc_dll(void);

typedef int(*type_vorbis_encode_init)(vorbis_info *vi,long channels,long rate,long max_bitrate,long nominal_bitrate,long min_bitrate);
typedef int(*type_vorbis_encode_init_vbr)(vorbis_info *vi,long channels,long rate,float base_quality);
typedef int(*type_vorbis_encode_ctl)(vorbis_info *vi,int number,void *arg);

static struct vorbisenc_dll_ {
	 type_vorbis_encode_init vorbis_encode_init;
	 type_vorbis_encode_init_vbr vorbis_encode_init_vbr;
	 type_vorbis_encode_ctl vorbis_encode_ctl;
} vorbisenc_dll;

static volatile HANDLE h_vorbisenc_dll = NULL;

void free_vorbisenc_dll(void)
{
	if(h_vorbisenc_dll){
		FreeLibrary(h_vorbisenc_dll);
		h_vorbisenc_dll = NULL;
	}
}

int load_vorbisenc_dll(void)
{
	if(!h_vorbisenc_dll){
		h_vorbisenc_dll = LoadLibrary("vorbisenc.dll");
		if(!h_vorbisenc_dll) return -1;
	}
	vorbisenc_dll.vorbis_encode_init = (type_vorbis_encode_init)GetProcAddress(h_vorbisenc_dll,"vorbis_encode_init");
	if(!vorbisenc_dll.vorbis_encode_init){ free_vorbisenc_dll(); return -1; }
	vorbisenc_dll.vorbis_encode_init_vbr = (type_vorbis_encode_init_vbr)GetProcAddress(h_vorbisenc_dll,"vorbis_encode_init_vbr");
	if(!vorbisenc_dll.vorbis_encode_init_vbr){ free_vorbisenc_dll(); return -1; }
	vorbisenc_dll.vorbis_encode_ctl = (type_vorbis_encode_ctl)GetProcAddress(h_vorbisenc_dll,"vorbis_encode_ctl");
	if(!vorbisenc_dll.vorbis_encode_ctl){ free_vorbisenc_dll(); return -1; }
	return 0;
}

int vorbis_encode_init(vorbis_info *vi,long channels,long rate,long max_bitrate,long nominal_bitrate,long min_bitrate)
{
	if(h_vorbisenc_dll){
		return vorbisenc_dll.vorbis_encode_init(vi,channels,rate,max_bitrate,nominal_bitrate,min_bitrate);
	}
	return (int)0;
}

int vorbis_encode_init_vbr(vorbis_info *vi,long channels,long rate,float base_quality)
{
	if(h_vorbisenc_dll){
		return vorbisenc_dll.vorbis_encode_init_vbr(vi,channels,rate,base_quality);
	}
	return (int)0;
}

int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg)
{
	if(h_vorbisenc_dll){
		return vorbisenc_dll.vorbis_encode_ctl(vi,number,arg);
	}
	return (int)0;
}

/***************************************************************/
#endif /* AU_VORBIS_DLL */

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
 name: vorbis_dll  dll: vorbis.dll 
***************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_VORBIS_DLL

#include <windows.h>
#include <vorbis/codec.h>

extern int load_vorbis_dll(void);
extern void free_vorbis_dll(void);

typedef void    (*type_vorbis_info_init)(vorbis_info *vi);
typedef void    (*type_vorbis_info_clear)(vorbis_info *vi);
typedef void    (*type_vorbis_comment_init)(vorbis_comment *vc);
typedef void    (*type_vorbis_comment_add)(vorbis_comment *vc, char *comment);
typedef void    (*type_vorbis_comment_add_tag)(vorbis_comment *vc, char *tag, char *contents);
typedef char   *(*type_vorbis_comment_query)(vorbis_comment *vc, char *tag, int count);
typedef int     (*type_vorbis_comment_query_count)(vorbis_comment *vc, char *tag);
typedef void    (*type_vorbis_comment_clear)(vorbis_comment *vc);
typedef int     (*type_vorbis_block_init)(vorbis_dsp_state *v, vorbis_block *vb);
typedef int     (*type_vorbis_block_clear)(vorbis_block *vb);
typedef void    (*type_vorbis_dsp_clear)(vorbis_dsp_state *v);
typedef int     (*type_vorbis_analysis_init)(vorbis_dsp_state *v,vorbis_info *vi);
////typedef int     (*type_vorbis_commentheader_out)(vorbis_comment *vc, ogg_packet *op);
typedef int     (*type_vorbis_analysis_headerout)(vorbis_dsp_state *v,vorbis_comment *vc,ogg_packet *op,ogg_packet *op_comm,ogg_packet *op_code);
typedef float **(*type_vorbis_analysis_buffer)(vorbis_dsp_state *v,int vals);
typedef int     (*type_vorbis_analysis_wrote)(vorbis_dsp_state *v,int vals);
typedef int     (*type_vorbis_analysis_blockout)(vorbis_dsp_state *v,vorbis_block *vb);
typedef int     (*type_vorbis_analysis)(vorbis_block *vb,ogg_packet *op);
typedef int     (*type_vorbis_bitrate_addblock)(vorbis_block *vb);
typedef int     (*type_vorbis_bitrate_flushpacket)(vorbis_dsp_state *vd, ogg_packet *op);
typedef int     (*type_vorbis_synthesis_headerin)(vorbis_info *vi,vorbis_comment *vc,ogg_packet *op);
typedef int     (*type_vorbis_synthesis_init)(vorbis_dsp_state *v,vorbis_info *vi);
typedef int     (*type_vorbis_synthesis)(vorbis_block *vb,ogg_packet *op);
typedef int     (*type_vorbis_synthesis_blockin)(vorbis_dsp_state *v,vorbis_block *vb);
typedef int     (*type_vorbis_synthesis_pcmout)(vorbis_dsp_state *v,float ***pcm);
typedef int     (*type_vorbis_synthesis_read)(vorbis_dsp_state *v,int samples);

static struct vorbis_dll_ {
	 type_vorbis_info_init vorbis_info_init;
	 type_vorbis_info_clear vorbis_info_clear;
	 type_vorbis_comment_init vorbis_comment_init;
	 type_vorbis_comment_add vorbis_comment_add;
	 type_vorbis_comment_add_tag vorbis_comment_add_tag;
//	 type_vorbis_comment_query vorbis_comment_query;
//	 type_vorbis_comment_query_count vorbis_comment_query_count;
	 type_vorbis_comment_clear vorbis_comment_clear;
	 type_vorbis_block_init vorbis_block_init;
	 type_vorbis_block_clear vorbis_block_clear;
	 type_vorbis_dsp_clear vorbis_dsp_clear;
	 type_vorbis_analysis_init vorbis_analysis_init;
////	 type_vorbis_commentheader_out vorbis_commentheader_out;
	 type_vorbis_analysis_headerout vorbis_analysis_headerout;
	 type_vorbis_analysis_buffer vorbis_analysis_buffer;
	 type_vorbis_analysis_wrote vorbis_analysis_wrote;
	 type_vorbis_analysis_blockout vorbis_analysis_blockout;
	 type_vorbis_analysis vorbis_analysis;
	 type_vorbis_bitrate_addblock vorbis_bitrate_addblock;
     type_vorbis_bitrate_flushpacket vorbis_bitrate_flushpacket;
//	 type_vorbis_synthesis_headerin vorbis_synthesis_headerin;
//	 type_vorbis_synthesis_init vorbis_synthesis_init;
//	 type_vorbis_synthesis vorbis_synthesis;
//	 type_vorbis_synthesis_blockin vorbis_synthesis_blockin;
//	 type_vorbis_synthesis_pcmout vorbis_synthesis_pcmout;
//	 type_vorbis_synthesis_read vorbis_synthesis_read;
} vorbis_dll;

static volatile HANDLE h_vorbis_dll = NULL;

void free_vorbis_dll(void)
{
	if(h_vorbis_dll){
		FreeLibrary(h_vorbis_dll);
		h_vorbis_dll = NULL;
	}
}

int load_vorbis_dll(void)
{
	if(!h_vorbis_dll){
		h_vorbis_dll = LoadLibrary("vorbis.dll");
		if(!h_vorbis_dll) return -1;
	}
	vorbis_dll.vorbis_info_init = (type_vorbis_info_init)GetProcAddress(h_vorbis_dll,"vorbis_info_init");
	if(!vorbis_dll.vorbis_info_init){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_info_clear = (type_vorbis_info_clear)GetProcAddress(h_vorbis_dll,"vorbis_info_clear");
	if(!vorbis_dll.vorbis_info_clear){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_comment_init = (type_vorbis_comment_init)GetProcAddress(h_vorbis_dll,"vorbis_comment_init");
	if(!vorbis_dll.vorbis_comment_init){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_comment_add = (type_vorbis_comment_add)GetProcAddress(h_vorbis_dll,"vorbis_comment_add");
	if(!vorbis_dll.vorbis_comment_add){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_comment_add_tag = (type_vorbis_comment_add_tag)GetProcAddress(h_vorbis_dll,"vorbis_comment_add_tag");
	if(!vorbis_dll.vorbis_comment_add_tag){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_comment_query = (type_vorbis_comment_query)GetProcAddress(h_vorbis_dll,"vorbis_comment_query");
//	if(!vorbis_dll.vorbis_comment_query){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_comment_query_count = (type_vorbis_comment_query_count)GetProcAddress(h_vorbis_dll,"vorbis_comment_query_count");
//	if(!vorbis_dll.vorbis_comment_query_count){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_comment_clear = (type_vorbis_comment_clear)GetProcAddress(h_vorbis_dll,"vorbis_comment_clear");
	if(!vorbis_dll.vorbis_comment_clear){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_block_init = (type_vorbis_block_init)GetProcAddress(h_vorbis_dll,"vorbis_block_init");
	if(!vorbis_dll.vorbis_block_init){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_block_clear = (type_vorbis_block_clear)GetProcAddress(h_vorbis_dll,"vorbis_block_clear");
	if(!vorbis_dll.vorbis_block_clear){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_dsp_clear = (type_vorbis_dsp_clear)GetProcAddress(h_vorbis_dll,"vorbis_dsp_clear");
	if(!vorbis_dll.vorbis_dsp_clear){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_analysis_init = (type_vorbis_analysis_init)GetProcAddress(h_vorbis_dll,"vorbis_analysis_init");
	if(!vorbis_dll.vorbis_analysis_init){ free_vorbis_dll(); return -1; }
////	vorbis_dll.vorbis_commentheader_out = (type_vorbis_commentheader_out)GetProcAddress(h_vorbis_dll,"vorbis_commentheader_out");
////	if(!vorbis_dll.vorbis_commentheader_out){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_analysis_headerout = (type_vorbis_analysis_headerout)GetProcAddress(h_vorbis_dll,"vorbis_analysis_headerout");
	if(!vorbis_dll.vorbis_analysis_headerout){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_analysis_buffer = (type_vorbis_analysis_buffer)GetProcAddress(h_vorbis_dll,"vorbis_analysis_buffer");
	if(!vorbis_dll.vorbis_analysis_buffer){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_analysis_wrote = (type_vorbis_analysis_wrote)GetProcAddress(h_vorbis_dll,"vorbis_analysis_wrote");
	if(!vorbis_dll.vorbis_analysis_wrote){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_analysis_blockout = (type_vorbis_analysis_blockout)GetProcAddress(h_vorbis_dll,"vorbis_analysis_blockout");
	if(!vorbis_dll.vorbis_analysis_blockout){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_analysis = (type_vorbis_analysis)GetProcAddress(h_vorbis_dll,"vorbis_analysis");
	if(!vorbis_dll.vorbis_analysis){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_bitrate_addblock = (type_vorbis_bitrate_addblock)GetProcAddress(h_vorbis_dll,"vorbis_bitrate_addblock");
	if(!vorbis_dll.vorbis_bitrate_addblock){ free_vorbis_dll(); return -1; }
	vorbis_dll.vorbis_bitrate_flushpacket = (type_vorbis_bitrate_flushpacket)GetProcAddress(h_vorbis_dll,"vorbis_bitrate_flushpacket");
	if(!vorbis_dll.vorbis_bitrate_flushpacket){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_synthesis_headerin = (type_vorbis_synthesis_headerin)GetProcAddress(h_vorbis_dll,"vorbis_synthesis_headerin");
//	if(!vorbis_dll.vorbis_synthesis_headerin){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_synthesis_init = (type_vorbis_synthesis_init)GetProcAddress(h_vorbis_dll,"vorbis_synthesis_init");
//	if(!vorbis_dll.vorbis_synthesis_init){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_synthesis = (type_vorbis_synthesis)GetProcAddress(h_vorbis_dll,"vorbis_synthesis");
//	if(!vorbis_dll.vorbis_synthesis){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_synthesis_blockin = (type_vorbis_synthesis_blockin)GetProcAddress(h_vorbis_dll,"vorbis_synthesis_blockin");
//	if(!vorbis_dll.vorbis_synthesis_blockin){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_synthesis_pcmout = (type_vorbis_synthesis_pcmout)GetProcAddress(h_vorbis_dll,"vorbis_synthesis_pcmout");
//	if(!vorbis_dll.vorbis_synthesis_pcmout){ free_vorbis_dll(); return -1; }
//	vorbis_dll.vorbis_synthesis_read = (type_vorbis_synthesis_read)GetProcAddress(h_vorbis_dll,"vorbis_synthesis_read");
//	if(!vorbis_dll.vorbis_synthesis_read){ free_vorbis_dll(); return -1; }
	return 0;
}

void    vorbis_info_init(vorbis_info *vi)
{
	if(h_vorbis_dll){
		vorbis_dll.vorbis_info_init(vi);
	}
}

void    vorbis_info_clear(vorbis_info *vi)
{
	if(h_vorbis_dll){
		vorbis_dll.vorbis_info_clear(vi);
	}
}

void    vorbis_comment_init(vorbis_comment *vc)
{
	if(h_vorbis_dll){
		vorbis_dll.vorbis_comment_init(vc);
	}
}

void    vorbis_comment_add(vorbis_comment *vc, char *comment)
{
	if(h_vorbis_dll){
		vorbis_dll.vorbis_comment_add(vc,comment);
	}
}

void    vorbis_comment_add_tag(vorbis_comment *vc, char *tag, char *contents)
{
	if(h_vorbis_dll){
		vorbis_dll.vorbis_comment_add_tag(vc,tag,contents);
	}
}

#if 0
char   *vorbis_comment_query(vorbis_comment *vc, char *tag, int count)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_comment_query(vc,tag,count);
	}
	return (char   *)0;
}

int     vorbis_comment_query_count(vorbis_comment *vc, char *tag)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_comment_query_count(vc,tag);
	}
	return (int     )0;
}
#endif

void    vorbis_comment_clear(vorbis_comment *vc)
{
	if(h_vorbis_dll){
		vorbis_dll.vorbis_comment_clear(vc);
	}
}

int     vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_block_init(v,vb);
	}
	return (int     )0;
}

int     vorbis_block_clear(vorbis_block *vb)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_block_clear(vb);
	}
	return (int     )0;
}

void    vorbis_dsp_clear(vorbis_dsp_state *v)
{
	if(h_vorbis_dll){
		vorbis_dll.vorbis_dsp_clear(v);
	}
}

int     vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_analysis_init(v,vi);
	}
	return (int     )0;
}

//int     vorbis_commentheader_out(vorbis_comment *vc, ogg_packet *op)
//{
//	if(h_vorbis_dll){
//		return vorbis_dll.vorbis_commentheader_out(vc,op);
//	}
//	return (int     )0;
//}

int     vorbis_analysis_headerout(vorbis_dsp_state *v,vorbis_comment *vc,ogg_packet *op,ogg_packet *op_comm,ogg_packet *op_code)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_analysis_headerout(v,vc,op,op_comm,op_code);
	}
	return (int     )0;
}

float **vorbis_analysis_buffer(vorbis_dsp_state *v,int vals)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_analysis_buffer(v,vals);
	}
	return (float **)0;
}

int     vorbis_analysis_wrote(vorbis_dsp_state *v,int vals)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_analysis_wrote(v,vals);
	}
	return (int     )0;
}

int     vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_analysis_blockout(v,vb);
	}
	return (int     )0;
}

int     vorbis_analysis(vorbis_block *vb,ogg_packet *op)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_analysis(vb,op);
	}
	return (int     )0;
}

int     vorbis_bitrate_addblock(vorbis_block *vb)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_bitrate_addblock(vb);
	}
	return (int     )0;
}

int     vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_bitrate_flushpacket(vd,op);
	}
	return (int     )0;
}

#if 0
int     vorbis_synthesis_headerin(vorbis_info *vi,vorbis_comment *vc,ogg_packet *op)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_synthesis_headerin(vi,vc,op);
	}
	return (int     )0;
}

int     vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_synthesis_init(v,vi);
	}
	return (int     )0;
}

int     vorbis_synthesis(vorbis_block *vb,ogg_packet *op)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_synthesis(vb,op);
	}
	return (int     )0;
}

int     vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_synthesis_blockin(v,vb);
	}
	return (int     )0;
}

int     vorbis_synthesis_pcmout(vorbis_dsp_state *v,float ***pcm)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_synthesis_pcmout(v,pcm);
	}
	return (int     )0;
}

int     vorbis_synthesis_read(vorbis_dsp_state *v,int samples)
{
	if(h_vorbis_dll){
		return vorbis_dll.vorbis_synthesis_read(v,samples);
	}
	return (int     )0;
}
#endif

/***************************************************************/
#endif /* AU_VORBIS_DLL */

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

    smfconv.c - from Aoki Daisuke <dai@y7.net>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef SMFCONV

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdio.h>
#include "timidity.h"
#include "common.h"
#include "controls.h"

/*
	If not smf but midi file, convert to smf to use other tools
	as possible.

	Example tool(dll)s for Win95
		rcpcv.dll by ÂçÃÝÉÒ»Ë

*/

#ifdef __W32__
#include <wtypes.h>
#include <dir.h>
#include "rcpcv.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "smfconv.h"

static struct {
	int type;
    char *ext;
    char *header;
    int len;
} is_midifile_check[] = {
/* type, ext, header, header_len */
	{ IS_RCP_FILE,".RCP","RCM-PC98V2.0(C)COME ON MUSIC",28 },	/* RCP */
	{ IS_R36_FILE,".R36","RCM-PC98V2.0(C)COME ON MUSIC",28 },	/* R36 */
	{ IS_G18_FILE,".G18","COME ON MUSIC RECOMPOSER RCP3.0",31 },	/* G18 */
	{ IS_G36_FILE,".G36","COME ON MUSIC RECOMPOSER RCP3.0",31 },	/* G36 */
	{ IS_MCP_FILE,".MCP","M1",2 },	/* MCP */
	{ IS_OTHER_FILE,NULL,NULL,0 }
};

/* return IS_???_FILE (readmidi.h) */
int is_midifile_filename(char *filename)
{
	char *ext_pos;
	int i;

	ext_pos=strrchr(filename,'.');
	if(ext_pos==NULL)
		return IS_OTHER_FILE;
	else
		for(i=0;is_midifile_check[i].type!=IS_OTHER_FILE;i++)
			if(strncasecmp(ext_pos,is_midifile_check[i].ext,4)==0)
				return is_midifile_check[i].type;
	return IS_OTHER_FILE;
}

/* Remark:
	url's position becomes indefinite ,so url->rewind will be demanded. */
/* return IS_???_FILE (smfconv.h) */
static int is_midifile_url(URL url)
{
	char buffer[1024];
    long len;
    int i;

	len=url_nread(url,buffer,100);

	for(i=0;is_midifile_check[i].type!=IS_OTHER_FILE;i++) {
		if(len<is_midifile_check[i].len)
			continue;
		if(memcmp(buffer,is_midifile_check[i].header,is_midifile_check[i].len)==0)
			return is_midifile_check[i].type;
	}
	return IS_OTHER_FILE;
}

/* Remark:
	url's position becomes indefinite ,so url->rewind will be demanded. */
/* return allocated memory void pointer */
static void *url2mem(URL url,long *lenp)
{
#define URL2MEM_BUFFER_BLOCK_SIZE 10240
	char *buffer = NULL;
    long buffer_size = 0;
    long buffer_size2 = 0;
    long ret;

	for(;;){
		if(buffer_size2 < buffer_size + URL2MEM_BUFFER_BLOCK_SIZE){
	    	if((buffer = (char *)safe_realloc(buffer,buffer_size2 + URL2MEM_BUFFER_BLOCK_SIZE))==NULL)
            	return NULL;
            buffer_size2+=URL2MEM_BUFFER_BLOCK_SIZE;
        }
		ret=url_nread(url,buffer+buffer_size,URL2MEM_BUFFER_BLOCK_SIZE);
		if(ret<0)
        	break;
        buffer_size+=ret;
        if(ret<URL2MEM_BUFFER_BLOCK_SIZE)
        	break;
    }
    *lenp = buffer_size;
	return (void *)buffer;
}

/* return not NULL (exist)
          NULL (not exist) */
static int exist_rcpcv_dll(void)
{
    HINSTANCE hRcpcv;
	UINT fuErrorMode;

	fuErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	hRcpcv = LoadLibrary("rcpcv.dll");
    SetErrorMode(fuErrorMode);
    if(hRcpcv==NULL)
		return NULL;
	FreeLibrary(hRcpcv);
	return 1;
}

/* Remark:
	url's position becomes indefinite ,so url->rewind will be demanded. */
/* return new URL
          NULL (error) */
static URL rcpcv_convert(URL url,int type)
{
	char *buffer;
    HINSTANCE hRcpcv;
	UINT fuErrorMode;
	long len;
	HRCPCV CALLBACK h;
	int rcpcv_type;
	URL new_url;

	HRCPCV (CALLBACK *lpRcpcvConvertFileFromBuffer)(LPCSTR,UINT,UINT,UINT,DWORD,UINT,DWORD);
	void (CALLBACK *lpRcpcvDeleteObject)(HRCPCV);
	LPCSTR (CALLBACK *lpRcpcvGetSMF)(HRCPCV);
	int (CALLBACK *lpRcpcvGetSMFLength)(HRCPCV);

	/* type */
	switch(type){
    	case IS_RCP_FILE:
    	case IS_R36_FILE:
			rcpcv_type = RCPCV_FORMATTYPE_RCM25F;
			break;
		case IS_G18_FILE:
		case IS_G36_FILE:
			rcpcv_type = RCPCV_FORMATTYPE_RCM25G;
			break;
        default:
        	return NULL;
    }

    /* rcpcv 1*/
	fuErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	hRcpcv = LoadLibrary("rcpcv.dll");
    SetErrorMode(fuErrorMode);
    if(hRcpcv==NULL)
		return NULL;

	/* url -> buffer */
	buffer = (char *)url2mem(url,&len);

    /* rcpcv 2*/
	lpRcpcvConvertFileFromBuffer = GetProcAddress(hRcpcv,"rcpcvConvertFileFromBuffer");
	lpRcpcvDeleteObject = GetProcAddress(hRcpcv,"rcpcvDeleteObject");
	lpRcpcvGetSMF = GetProcAddress(hRcpcv,"rcpcvGetSMF");
	lpRcpcvGetSMFLength = GetProcAddress(hRcpcv,"rcpcvGetSMFLength");

	h = (*lpRcpcvConvertFileFromBuffer)((LPCSTR)buffer,(UINT)len,(UINT)rcpcv_type,RCPCV_CALLBACK_NULL,NULL, 0, 0);
	if (h==NULL){
    	free(buffer);
		FreeLibrary(hRcpcv);
		return NULL;
	}
	len = (*lpRcpcvGetSMFLength)(h);
    if((buffer = (char *)safe_realloc(buffer,len + 10))==NULL){
		FreeLibrary(hRcpcv);
		return NULL;
	}
    memcpy(buffer,(*lpRcpcvGetSMF)(h),len);
    (*lpRcpcvDeleteObject)(h);
	FreeLibrary(hRcpcv);

	/* url_mem_open */
	if((new_url=url_mem_open(buffer,len,1))==NULL){
    	return NULL;
    }

	url_close(url);
	return new_url;
}

/* return 0 (successful or not convert)
          -1 (error and lost tf->url) */
int smfconv_w32(struct timidity_file *tf, char *fn)
{
	URL url;
    int ret;
	struct midi_file_info *infop;

/* rcpcv.dll convertion stage */
rcpcv_dll_stage:
	if(exist_rcpcv_dll())
	{

	ret = is_midifile_filename(fn);
	if(ret == IS_RCP_FILE || ret == IS_R36_FILE || ret == IS_G18_FILE || ret == IS_G36_FILE)
    {
    	if(!IS_URL_SEEK_SAFE(tf->url))
    	    tf->url = url_cache_open(tf->url, 1);
        ret = is_midifile_url(tf->url);
		url_rewind(tf->url);
		if(ret == IS_RCP_FILE || ret == IS_R36_FILE || ret == IS_G18_FILE || ret == IS_G36_FILE)
    	{
			ctl->cmsg(CMSG_INFO,VERB_NORMAL,"Try to Convert RCP,R36,G18,G36 to SMF by RCPCV.DLL (c)1997 Fumy.");
	        url = rcpcv_convert(tf->url,ret);
			if(url == NULL){
				/* url_arc or url_cash is buggy ? */
				/*
				url_rewind(tf->url);
				url_cache_disable(tf->url);
				ctl->cmsg(CMSG_INFO,VERB_NORMAL,"Convert Failed.");
				goto end_of_rcpcv_dll_stage;
				*/

				char *buffer;
				int len;
                URL new_url;

				url_rewind(tf->url);
				if((buffer = (char *)url2mem(tf->url,&len))==NULL){
					ctl->cmsg(CMSG_INFO,VERB_NORMAL,"Convert Failed.");
					goto end_of_rcpcv_dll_stage;
				}
				if((new_url=url_mem_open(buffer,len,1))==NULL){
					ctl->cmsg(CMSG_INFO,VERB_NORMAL,"Convert Failed and Memory Allocate Error.");
					url_cache_disable(tf->url);
    				return -1;
				}
				url_close(tf->url);
				tf->url = new_url;
				ctl->cmsg(CMSG_INFO,VERB_NORMAL,"Convert Failed.");
				goto end_of_rcpcv_dll_stage;
            }
			url_cache_disable(tf->url);
            tf->url = url;
			ctl->cmsg(CMSG_INFO,VERB_NORMAL,"Convert Completed.");

			/* Store the midi file type information */
			infop = get_midi_file_info(fn, 1);
			infop->file_type = ret;
		} else
        	url_cache_disable(tf->url);
    }

	}
/* end of rcpcv.dll convertion stage */
end_of_rcpcv_dll_stage:

/* smfk32.dll convertion stage */
smfk32_dll_stage:

	/* not exist */

/* end of smfk32.dll convertion stage */
end_of_smfk32_dll_stage:

last_stage:
	return 0;
}

#endif /* __W32__ */

#endif /* SMFCONV */

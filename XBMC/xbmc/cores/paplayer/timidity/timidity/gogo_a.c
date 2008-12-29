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

    gogo_a.c

    Functions to output mp3 by gogo.lib or gogo.dll.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef AU_GOGO

#ifdef __W32__
#include <io.h>
#include <windows.h>
#include <process.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <fcntl.h>

/* #include <musenc.h>		/* for gogo */
#include <gogo/gogo.h>		/* for gogo */

#include "w32_gogo.h"
#include "gogo_a.h"

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

#if defined(__W32__) && !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
#include "w32g.h" //for crt_beginthreadex();
#endif

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);

/* export the playback mode */
#define dpm gogo_play_mode

PlayMode dpm = {
    DEFAULT_RATE,
#ifdef LITTLE_ENDIAN
    PE_16BIT|PE_SIGNED,
#else
    PE_16BIT|PE_SIGNED|PE_BYTESWAP,
#endif
    PF_PCM_STREAM,
    -1,
    {0,0,0,0,0},
    "MP3 GOGO", 'g',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl
};
static char *tag_title = NULL;

#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
#if defined(_MSC_VER) || defined(__WATCOMC__)
//typedef void (__cdecl *MSVC_BEGINTHREAD_START_ADDRESS)(void *);
typedef LPTHREAD_START_ROUTINE MSVC_BEGINTHREAD_START_ADDRESS;
#elif defined(_BORLANDC_)
// typedef _USERENTRY (*BCC_BEGINTHREAD_START_ADDRESS)(void *);
typedef LPTHREAD_START_ROUTINE BCC_BEGINTHREAD_START_ADDRESS;
#endif
// (HANDLE)crt_beginthreadex(LPSECURITY_ATTRIBUTES security, DWORD stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID arglist, DWORD initflag, LPDWORD thrdaddr );
#if defined(_MSC_VER) || defined(__WATCOMC__)
#define crt_beginthreadex(security,stack_size,start_address,arglist,initflag,thrdaddr ) \
(HANDLE)_beginthreadex((void *)security,(unsigned)stack_size,(MSVC_BEGINTHREAD_START_ADDRESS)start_address,(void *)arglist,(unsigned)initflag,(unsigned *)thrdaddr)
#elif defined(_BORLANDC_)
#define crt_beginthreadex(security,stack_size,start_address,arglist,initflag,thrdaddr ) \
(HANDLE)_beginthreadNT((BCC_BEGINTHREAD_START_ADDRESS)start_address,(unsigned)stack_size,(void *)arglist,(void *)security_attrib,(unsigned long)create_flags,(unsigned long *)thread_id)
#else
#define crt_beginthreadex(security,stack_size,start_address,arglist,initflag,thrdaddr ) \
(HANDLE)CreateThread((LPSECURITY_ATTRIBUTES)security,(DWORD)stack_size,(LPTHREAD_START_ROUTINE)start_address,(LPVOID)arglist,(DWORD)initflag,(LPDWORD)thrdaddr)
#endif

volatile extern char *w32g_output_dir;
volatile extern int w32g_auto_output_mode;

extern int gogo_ConfigDialogInfoApply(void);
#endif

#ifdef __W32__
volatile static HANDLE hMPGEthread = NULL;
volatile static HANDLE hMutexgogo = NULL;
volatile static DWORD dwMPGEthreadID = 0;
#endif

volatile gogo_opts_t gogo_opts;

/*************************************************************************/

static int gogo_error(MERET rval)
{
	switch(rval){
	case ME_NOERR:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : return normally (%d).",rval);
		break;
	case ME_EMPTYSTREAM:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : stream becomes empty (%d).",rval);
		break;
	case ME_HALTED:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : stopped by user (%d).",rval);
		break;
	case ME_INTERNALERROR:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : internal error (%d).",rval);
		break;
	case ME_PARAMERROR:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : parameters error (%d).",rval);
		break;
	case ME_NOFPU:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : no FPU (%d).",rval);
		break;
	case ME_INFILE_NOFOUND:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : can't open input file (%d).",rval);
		break;
	case ME_OUTFILE_NOFOUND:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : can't open output file (%d).",rval);
		break;
	case ME_FREQERROR:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : frequency is not good (%d).",rval);
		break;
	case ME_BITRATEERROR	:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : bitrate is not good (%d).",rval);
		break;
	case ME_WAVETYPE_ERR:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : WAV format is not good (%d).",rval);
		break;
	case ME_CANNOT_SEEK:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : can't seek (%d).",rval);
		break;
	case ME_BITRATE_ERR:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : only for compatibility (%d).",rval);
		break;
	case ME_BADMODEORLAYER:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : mode/layer not good (%d).",rval);
		break;
	case ME_NOMEMORY:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : fail to allocate memory (%d).",rval);
		break;
	case ME_CANNOT_SET_SCOPE:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : thread error (%d).",rval);
		break;
	case ME_CANNOT_CREATE_THREAD:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : fail to create thear (%d).",rval);
		break;
	case ME_WRITEERROR:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : lack of capacity of disk (%d).",rval);
		break;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : unknown error (%d).",rval);
		break;
	}
	return 0;
}

/* mode
  0,1: Default mode.
  2: Remove the directory path of input_filename, then add output_dir.
  3: Replace directory separator characters ('/','\',':') with '_', then add output_dir.
 */
extern char *create_auto_output_name(const char *input_filename, char *ext_str, char *output_dir, int mode);
 
volatile static int gogo_lock_flag = 0;
static int gogo_lock()
{
#ifndef __W32__
	while(gogo_lock_flag){
		usleep(100);
	}
	gogo_lock_flag++;
	return 0;
#else
	if(hMutexgogo==NULL){
		hMutexgogo = CreateMutex(NULL,FALSE,NULL);
		if(hMutexgogo==NULL)
			return -1;
	}
	if(WaitForSingleObject(hMutexgogo,INFINITE)==WAIT_FAILED){
		return -1;
	}
	return 0;
#endif
}

static int gogo_unlock()
{
#ifndef __W32__
	if(gogo_lock_flag)
		gogo_lock_flag--;
	return 0;
#else
	if(hMutexgogo!=NULL){
		ReleaseMutex(hMutexgogo);
		Sleep(0);
	}
	return 0;
#endif
}

//volatile int32 gogo_buffer_size = 1024*1024;
volatile int32 gogo_buffer_size = 1024*256;
volatile static char *gogo_buffer = NULL;	// loop buffer
volatile static char *gogo_buffer_from = NULL;
volatile static char *gogo_buffer_start = NULL;
volatile static char *gogo_buffer_end = NULL;
volatile static int32 gogo_buffer_cur_size = 0;
volatile static int gogo_buffer_termination = 0;

volatile int gogo_buffer_initflag = 0;
static void gogo_buffer_init(void)
{
	gogo_lock();
	if(!gogo_buffer_initflag){
		gogo_buffer = (char *)safe_malloc(sizeof(char)*(gogo_buffer_size+1));
		gogo_buffer_from = gogo_buffer;
		gogo_buffer_start = gogo_buffer;
		gogo_buffer_end = gogo_buffer + gogo_buffer_size - 1;
		gogo_buffer_cur_size = 0;
	}
	gogo_buffer_initflag = 1;
	gogo_unlock();
}
static void gogo_buffer_reset(void)
{
	gogo_lock();
	if(!gogo_buffer_initflag)
		gogo_buffer_init();
	gogo_buffer_from = gogo_buffer;
	gogo_buffer_cur_size = 0;
	gogo_unlock();
}

// 同期ノーチェック
// gogo_buffer_cur_size まで buf  に gogo_buffer の内容を size だけ
// コピーする。moveflag があるときは、コピーした分だけ
// gogo_buffer_from と gogo_buffer_cur_size を変化させる。
// コピーしたサイズを返す。
static int32 __gogo_buffer_copy(char *buf, int32 size, int moveflag)
{
	int i;
	int32 size1, ret = 0;
	gogo_buffer_init();
	if(size>gogo_buffer_cur_size)
		size = gogo_buffer_cur_size;
	for(i=0;i<2;i++){
		size1 = gogo_buffer_end-gogo_buffer_from+1;
		if(size<=size1)
			size1 = size;
		memcpy(buf,(char *)gogo_buffer_from,size1);
		ret += size1;
		size -= size1;
		buf += size1;
		if(moveflag){
			gogo_buffer_from += size1;
			if(gogo_buffer_from>=gogo_buffer_end+1)
				gogo_buffer_from = gogo_buffer_start;
			gogo_buffer_cur_size -= size1;
		}
		if(size<=0){
			return ret;
		}
	}
	return ret;
}

// 同期ノーチェック
// gogo_buffer_cur_size まで buf  の内容を size だけ gogo_buffer に
// 格納する。格納したサイズを返す。
static int32 __gogo_buffer_push(char *buf, int32 size)
{
	int i;
	int32 size1, ret = 0;
	char *gogo_buffer_pointer;
	gogo_buffer_init();
	if(size>gogo_buffer_size-gogo_buffer_cur_size)
		size = gogo_buffer_size-gogo_buffer_cur_size;
	if(gogo_buffer+gogo_buffer_size>gogo_buffer_from+gogo_buffer_cur_size){
		gogo_buffer_pointer = (char *)gogo_buffer_from + gogo_buffer_cur_size;
		i = 0;
	} else {
		gogo_buffer_pointer = (char *)gogo_buffer_from+gogo_buffer_cur_size-gogo_buffer_size;
		i = 1;
	}
	for(;i<2;i++){
		size1 = gogo_buffer_end-gogo_buffer_pointer+1;
		if(size<=size1)
			size1 = size;
		memcpy(gogo_buffer_pointer,buf,size1);
		ret += size1;
		size -= size1;
		buf += size1;
		gogo_buffer_cur_size += size1;
		gogo_buffer_pointer += size1;
		if(gogo_buffer_pointer>=gogo_buffer_end+1)
			gogo_buffer_pointer = (char *)gogo_buffer_start;
		if(size<=0)
			return ret;
	}
	return ret;
}

static int __cdecl gogoUserFunc(char *buf, unsigned long nLength )
{
	int flag = 0;
	gogo_buffer_init();
	if(buf==NULL || nLength==0){
		if(!gogo_buffer_termination)
			gogo_buffer_termination = 1;
		return  ME_NOERR;
	}
	for(;;){
		int32 size;
		if(gogo_buffer_termination<0){
			return ME_EMPTYSTREAM;
		}
		if(gogo_lock()){
			gogo_buffer_termination = -1;
			return ME_EMPTYSTREAM;
		}
		size = __gogo_buffer_copy(buf,nLength,1);
		buf += size;
		nLength -= size;
		if(nLength<=0){
			gogo_unlock();
			if(size>0)
				return ME_NOERR;
			else
				return ME_EMPTYSTREAM;
		}
		if(gogo_buffer_termination>0){
			gogo_unlock();
			if(size>0 && nLength>0){
				if(dpm.encoding & PE_16BIT){
					memset(buf,0x00,nLength);
				} else {
					memset(buf,0x80,nLength);
				}
				return ME_NOERR;
			} else {
				return ME_EMPTYSTREAM;
			}
		}
		if(gogo_buffer_termination<0){
			gogo_unlock();
			return ME_EMPTYSTREAM;
		}
		if(gogo_buffer_cur_size<=0){
			gogo_unlock();
#ifndef __W32__
			usleep(100);
#else
			Sleep(0);
			Sleep(100);
#endif
		} else {
			gogo_unlock();
		}
	}
}

void gogo_opts_reset(void)
{
	gogo_opts.optENCODEMODE = -1;
	gogo_opts.optINPFREQ = -1;
	gogo_opts.optOUTFREQ = -1;
	gogo_opts.optSTARTOFFSET = -1;
	gogo_opts.optUSEPSY = -1;
	gogo_opts.optUSELPF16 = -1;
	gogo_opts.optUSEMMX = -1;
	gogo_opts.optUSE3DNOW = -1;
	gogo_opts.optUSEKNI = -1;
	gogo_opts.optUSEE3DNOW = -1;
	gogo_opts.optADDTAGnum = 0; // 追加するたびにインクリメントされるためデフォルトは0
	gogo_opts.optEMPHASIS = -1;
	gogo_opts.optVBR = -1;
	gogo_opts.optCPU = -1;
	gogo_opts.optBYTE_SWAP = -1;
	gogo_opts.opt8BIT_PCM = -1;
	gogo_opts.optMONO_PCM = -1;
	gogo_opts.optTOWNS_SND = -1;
	gogo_opts.optTHREAD_PRIORITY = -10000;
	gogo_opts.optREADTHREAD_PRIORITY = -10000;
	gogo_opts.optOUTPUT_FORMAT = -1;
	gogo_opts.optENHANCEDFILTER_A = -1;
	gogo_opts.optENHANCEDFILTER_B = -1;
	gogo_opts.optVBRBITRATE_low = -1;
	gogo_opts.optVBRBITRATE_high = -1;
	gogo_opts.optMSTHRESHOLD_threshold = -1;
	gogo_opts.optMSTHRESHOLD_mspower = -1;
	gogo_opts.optVERIFY = -1;
	gogo_opts.optOUTPUTDIR[0] = '\0';
	gogo_opts.optBITRATE1 = -1;
	gogo_opts.optBITRATE2 = -1;
}

static volatile int gogo_opts_initflag = 0;
void gogo_opts_init(void)
{
	gogo_lock();
	if(!gogo_opts_initflag){
		gogo_opts_initflag = 1;
		gogo_opts.optADDTAGnum = 0;
		gogo_opts.output_name[0] = '\0';
	}
	gogo_opts_reset();
	gogo_unlock();
}
void gogo_opts_reset_tag(void)
{
  if (gogo_opts.optADDTAGnum >= 0) {
    int i;
    for (i = 0; i < gogo_opts.optADDTAGnum; i++) {
      gogo_opts.optADDTAG_len[i] = 0;
      if (gogo_opts.optADDTAG_buf[i] != NULL) {
	free(gogo_opts.optADDTAG_buf[i]);
	gogo_opts.optADDTAG_buf[i] = NULL;
      }
    }
  }
}

static void set_gogo_opts(void)
{
	if(gogo_opts.optENCODEMODE>=0)
		MPGE_setConfigure(MC_ENCODEMODE,(UPARAM)gogo_opts.optENCODEMODE,(UPARAM)0);
	if(gogo_opts.optINPFREQ>=0)
		MPGE_setConfigure(MC_INPFREQ,(UPARAM)gogo_opts.optINPFREQ,(UPARAM)0);
	if(gogo_opts.optOUTFREQ>=0)
		MPGE_setConfigure(MC_OUTFREQ,(UPARAM)gogo_opts.optOUTFREQ,(UPARAM)0);
	if(gogo_opts.optSTARTOFFSET>=0)
		MPGE_setConfigure(MC_STARTOFFSET,(UPARAM)gogo_opts.optSTARTOFFSET,(UPARAM)0);
	if(gogo_opts.optUSEPSY>=0)
		MPGE_setConfigure(MC_USEPSY,(UPARAM)gogo_opts.optUSEPSY,(UPARAM)0);
	if(gogo_opts.optUSELPF16>=0)
		MPGE_setConfigure(MC_USELPF16,(UPARAM)gogo_opts.optUSELPF16,(UPARAM)0);
	if(gogo_opts.optUSEMMX>=0)
		MPGE_setConfigure(MC_USEMMX,(UPARAM)gogo_opts.optUSEMMX,(UPARAM)0);
	if(gogo_opts.optUSE3DNOW>=0)
		MPGE_setConfigure(MC_USE3DNOW,(UPARAM)gogo_opts.optUSE3DNOW,(UPARAM)0);
	if(gogo_opts.optUSEKNI>=0)
		MPGE_setConfigure(MC_USEKNI,(UPARAM)gogo_opts.optUSEKNI,(UPARAM)0);
	if(gogo_opts.optUSEE3DNOW>=0)
		MPGE_setConfigure(MC_USEE3DNOW,(UPARAM)gogo_opts.optUSEE3DNOW,(UPARAM)0);
	if(gogo_opts.optUSESSE>=0)
		MPGE_setConfigure(MC_USESSE,(UPARAM)gogo_opts.optUSESSE,(UPARAM)0);
	if(gogo_opts.optUSECMOV>=0)
		MPGE_setConfigure(MC_USECMOV,(UPARAM)gogo_opts.optUSECMOV,(UPARAM)0);
	if(gogo_opts.optUSEEMMX>=0)
		MPGE_setConfigure(MC_USEEMMX,(UPARAM)gogo_opts.optUSEEMMX,(UPARAM)0);
	if(gogo_opts.optUSESSE2>=0)
		MPGE_setConfigure(MC_USESSE2,(UPARAM)gogo_opts.optUSESSE2,(UPARAM)0);
	if(gogo_opts.optADDTAGnum>=0){
		int i;
		for(i=0;i<gogo_opts.optADDTAGnum;i++)
			MPGE_setConfigure(MC_ADDTAG,(UPARAM)gogo_opts.optADDTAG_len[i],(UPARAM)gogo_opts.optADDTAG_buf[i]);
	}
	if(gogo_opts.optEMPHASIS>=0)
		MPGE_setConfigure(MC_EMPHASIS,(UPARAM)gogo_opts.optEMPHASIS,(UPARAM)0);
	if(gogo_opts.optVBR>=0)
		MPGE_setConfigure(MC_VBR,(UPARAM)gogo_opts.optVBR,(UPARAM)0);
	if(gogo_opts.optCPU>=0)
		MPGE_setConfigure(MC_CPU,(UPARAM)gogo_opts.optCPU,(UPARAM)0);
	if(gogo_opts.optBYTE_SWAP>=0)
		MPGE_setConfigure(MC_BYTE_SWAP,(UPARAM)gogo_opts.optBYTE_SWAP,(UPARAM)0);
	if(gogo_opts.opt8BIT_PCM>=0)
		MPGE_setConfigure(MC_8BIT_PCM,(UPARAM)gogo_opts.opt8BIT_PCM,(UPARAM)0);
	if(gogo_opts.optMONO_PCM>=0)
		MPGE_setConfigure(MC_MONO_PCM,(UPARAM)gogo_opts.optMONO_PCM,(UPARAM)0);
	if(gogo_opts.optTOWNS_SND>=0)
		MPGE_setConfigure(MC_TOWNS_SND,(UPARAM)gogo_opts.optTOWNS_SND,(UPARAM)0);
	if(gogo_opts.optTHREAD_PRIORITY!=-10000)
		MPGE_setConfigure(MC_THREAD_PRIORITY,(UPARAM)gogo_opts.optTHREAD_PRIORITY,(UPARAM)0);
	if(gogo_opts.optREADTHREAD_PRIORITY!=-10000)
		MPGE_setConfigure(MC_READTHREAD_PRIORITY,(UPARAM)gogo_opts.optREADTHREAD_PRIORITY,(UPARAM)0);
	if(gogo_opts.optOUTPUT_FORMAT>=0)
		MPGE_setConfigure(MC_OUTPUT_FORMAT,(UPARAM)gogo_opts.optOUTPUT_FORMAT,(UPARAM)0);
	if(gogo_opts.optENHANCEDFILTER_A>=0 && gogo_opts.optENHANCEDFILTER_B>=0)
		MPGE_setConfigure(MC_ENHANCEDFILTER,(UPARAM)gogo_opts.optENHANCEDFILTER_A,(UPARAM)gogo_opts.optENHANCEDFILTER_B);
	if(gogo_opts.optVBRBITRATE_low>=0 && gogo_opts.optVBRBITRATE_high>=0)
		MPGE_setConfigure(MC_VBRBITRATE,(UPARAM)gogo_opts.optVBRBITRATE_low,(UPARAM)gogo_opts.optVBRBITRATE_high);
	if(gogo_opts.optMSTHRESHOLD_threshold>=0 && gogo_opts.optMSTHRESHOLD_mspower>=0)
		MPGE_setConfigure(MC_MSTHRESHOLD,(UPARAM)gogo_opts.optMSTHRESHOLD_threshold,(UPARAM)gogo_opts.optMSTHRESHOLD_mspower);
	if(gogo_opts.optVERIFY>=0)
		MPGE_setConfigure(MC_VERIFY,(UPARAM)gogo_opts.optVERIFY,(UPARAM)0);
	if(gogo_opts.optOUTPUTDIR[0]!='\0')
		MPGE_setConfigure(MC_OUTPUTDIR,(UPARAM)gogo_opts.optOUTPUTDIR,(UPARAM)0);
	if(strcmp((char *)gogo_opts.output_name,"-")==0){
		MPGE_setConfigure(MC_OUTPUTFILE,(UPARAM)MC_OUTDEV_STDOUT,(UPARAM)0);
	} else {
		MPGE_setConfigure(MC_OUTPUTFILE,(UPARAM)MC_OUTDEV_FILE,(UPARAM)gogo_opts.output_name);
	}
	if(dpm.rate>=32000){
		if(gogo_opts.optBITRATE1>=0)
			MPGE_setConfigure(MC_BITRATE,(UPARAM)gogo_opts.optBITRATE1,(UPARAM)0);
	} else {
		if(gogo_opts.optBITRATE2>=0)
			MPGE_setConfigure(MC_BITRATE,(UPARAM)gogo_opts.optBITRATE2,(UPARAM)0);
	}
}
static int id3_to_buffer(mp3_id3_tag_t *id3_tag, char *buffer)
{
	memset(buffer,0x20,128);
#if 0
	memcpy(buffer+0,id3_tag->tag);
#else
	memcpy(buffer+0,"TAG",3); //ID3 Tag v1
#endif
	memcpy(buffer+3,id3_tag->title,30);
	memcpy(buffer+33,id3_tag->artist,30);
	memcpy(buffer+63,id3_tag->album,30);
	memcpy(buffer+93,id3_tag->year,4);
	memcpy(buffer+97,id3_tag->comment,30);
	buffer[127] = id3_tag->genre;
	return 0;
}
static char *id3_tag_strncpy(char *dst, const char *src, int num)
{
	// NULL終端を付加してはならない
	int len = strlen(src);
	return memcpy (dst, src, (len <= num) ? len : num);
}
int gogo_opts_id3_tag(const char *title, const char *artist, const char *album, const char *year, const char *comment, int genre)
{
	mp3_id3_tag_t id3_tag;
	char *buffer;
	buffer = (char *)safe_malloc(128);
	if(buffer==NULL)
		return -1;
	memset(&id3_tag,0x20,128); // TAG全体を空白文字でfill
	if(title){
		id3_tag_strncpy(id3_tag.title,title,30);
	}
	if(artist){
		id3_tag_strncpy(id3_tag.artist,artist,30);
	}
	if(album){
		id3_tag_strncpy(id3_tag.album,album,30);
	}
	if(year){
		id3_tag_strncpy(id3_tag.year,year,4);
	}
	if(comment){
		id3_tag_strncpy(id3_tag.comment,comment,30);
	}
	if(genre>=0)
		id3_tag.genre = (unsigned char)(genre & 0xff);
	else // ジャンル未設定
		id3_tag.genre = (unsigned char)0xff;
	id3_to_buffer(&id3_tag,buffer);
	gogo_opts.optADDTAGnum++;
	gogo_opts.optADDTAG_len[gogo_opts.optADDTAGnum-1] = 128;
	gogo_opts.optADDTAG_buf[gogo_opts.optADDTAGnum-1] = buffer;
//	MPGE_setConfigure(MC_ADDTAG,(UPARAM)128,(UPARAM)buffer);
	return 0;
}

// コマンドラインを展開する
int commandline_to_argc_argv(char *commandline, int *argc, char ***argv)
{
	char *p1, *p2, *p3;
	int argc_max = 0;
	*argc = 0;
	*argv = NULL;
	p1 = commandline;
	for(;;){
		int quot = 0;
		while(isspace(*p1))
			p1++;
		if(*p1=='"')
			quot = 1;
		else if(*p1=='\'')
			quot = 2;
		if(*p1=='\0')
			return 0;
		p2 = p1+1;
		while((quot==1&&*p2=='"') || (quot==2&&*p2=='\'') || *p2=='\0')
			p2++;
		while(!isspace(*p2) || *p2=='\0')
			p2++;
		if(*p2!='\0'){
			p3 = p2+1;
			while(isspace(*p3))
				p3++;
		} else{
			p3 = NULL;
		}
		(*argc)++;
		if(*argc>argc_max)
			argc_max += 100;
		*argv = (char **)safe_realloc(*argv,sizeof(char*)*argc_max);
		if(*argv==NULL)
			return -1;
		*argv[*argc-1] = (char *)safe_malloc(sizeof(char)*(p2-p1));
		if(*argv[*argc-1]==NULL){
			(*argc)--;
			return 0;
		}
		strncpy(*argv[*argc-1],p1,p2-p1);
		*argv[*argc-1][p2-p1] = '\0';
		if(p3==NULL)
			return 0;
		p1 = p3;
	}
}
void free_argv(char **argv, int argc)
{
	if(argv!=NULL){
		int i;
		for(i=0;i<argc;i++)
			if(argv[i]!=NULL)
				free(argv[i]);
		free(argv);
	}
}
	
volatile char *gogo_commandline_options = NULL;
volatile int use_gogo_commandline_options = 0;
void set_gogo_opts_use_commandline_options(char *commandline)
{
	int argc;
	char **argv;
	int num;
	if(commandline_to_argc_argv(commandline,&argc,&argv)){
		return;
	}
	for(num = 0;num<argc;){
		// output bitrate
		// -b kbps
        //     input PCM is higher than 32kHz [ 32,40,48,56,64,80,96,112,128,160,192,224,256,320 ]
        //     input PCM is lower than 32kHz   [ 8,16,24,32,40,48,56,64,80,96,112,128,144,160 ]
		if(strcasecmp("-b",argv[num])==0){
			if(num+1>=argc) break;
			gogo_opts.optBITRATE1 = gogo_opts.optBITRATE2 = atoi(argv[num+1]);
			num += 2;
			continue;
		}
		// output bitrate for VBR
		// -br low_kbps high_kbps
        //     [ 32,40,48,56,64,80,96,112,128,160,192,224,256,320 ]
		if(strcasecmp("-br",argv[num])==0){
			if(num+2>=argc) break;
			if(gogo_opts.optVBR==-1)
				gogo_opts.optVBR = 9;
			gogo_opts.optVBRBITRATE_low = atoi(argv[num+1]);
			gogo_opts.optVBRBITRATE_high = atoi(argv[num+2]);
			num += 3;
			continue;
		}
		// low pass filter
		// -lpf on : 16kHz low pass filter ON
		// -lpf off : 16kHz low pass filter OFF
		// -lpf para1 para2 :  enhanced low pass filter
		//     para1 (0-100)   para2 (0-100)
		if(strcasecmp("-lpf",argv[num])==0){
			if(num+1>=argc) break;
			if(strcasecmp("on",argv[num+1])==0){
				gogo_opts.optENHANCEDFILTER_A = -1;
				gogo_opts.optENHANCEDFILTER_B = -1;
				gogo_opts.optUSELPF16 = 1;
				num += 2;
				continue;
			} else if(strcasecmp("off",argv[num+1])==0){
				gogo_opts.optENHANCEDFILTER_A = -1;
				gogo_opts.optENHANCEDFILTER_B = -1;
				gogo_opts.optUSELPF16 = 0;
				num += 2;
				continue;
			}
			if(num+2>=argc) break;
			gogo_opts.optENHANCEDFILTER_A = atoi(argv[num+1]);
			gogo_opts.optENHANCEDFILTER_B = atoi(argv[num+2]);
			num += 3;
			continue;
		}
		// encodemode
		// -m m : monoral
		// -m s : stereo
		// -m j : joint stereo
		// -m f : mid/side stereo
		// -m d : dual channel
		if(strcasecmp("-m",argv[num])==0){
			if(num+1>=argc) break;
			if(strcasecmp("m",argv[num+1])==0){
				gogo_opts.optENCODEMODE = MC_MODE_MONO;
			} else if(strcasecmp("s",argv[num+1])==0){
				gogo_opts.optENCODEMODE = MC_MODE_STEREO;
			} else if(strcasecmp("j",argv[num+1])==0){
				gogo_opts.optENCODEMODE = MC_MODE_JOINT;
			} else if(strcasecmp("f",argv[num+1])==0){
				gogo_opts.optENCODEMODE = MC_MODE_MSSTEREO;
			} else if(strcasecmp("d",argv[num+1])==0){
				gogo_opts.optENCODEMODE = MC_MODE_DUALCHANNEL;
			}
			num += 2;
			continue;
		}
		// -psy
		// -nopsy
		if(strcasecmp("-psy",argv[num])==0){
			gogo_opts.optUSEPSY = 1;
			num += 1;
			continue;
		}
		if(strcasecmp("-nopsy",argv[num])==0){
			gogo_opts.optUSEPSY = 0;
			num += 1;
			continue;
		}
		// -on 3dn
		// -on mmx
		// -on sse
		// -on kni
		// -on e3dn
		// -on sse
		// -on cmov
		// -on emmx
		// -on sse2
		if(strcasecmp("-on",argv[num])==0){
			if(num+1>=argc) break;
			if(strcasecmp("3dn",argv[num+1])==0){
				gogo_opts.optUSE3DNOW = 1;
			} else if(strcasecmp("mmx",argv[num+1])==0){
				gogo_opts.optUSEMMX = 1;
			} else if(strcasecmp("sse",argv[num+1])==0 || strcasecmp("kni",argv[num+1])==0){
				gogo_opts.optUSEKNI = 1;
			} else if(strcasecmp("e3dn",argv[num+1])==0){
				gogo_opts.optUSEE3DNOW = 1;
			} else if(strcasecmp("sse",argv[num+1])==0){
				gogo_opts.optUSESSE = 1;
			} else if(strcasecmp("cmov",argv[num+1])==0){
				gogo_opts.optUSECMOV = 1;
			} else if(strcasecmp("emmx",argv[num+1])==0){
				gogo_opts.optUSEEMMX = 1;
			} else if(strcasecmp("sse2",argv[num+1])==0){
				gogo_opts.optUSESSE2 = 1;
			}
			num += 2;
			continue;
		}
		// -off 3dn
		// -off mmx
		// -off sse
		// -off kni
		// -off e3dn
		// -off sse
		// -off cmov
		// -off emmx
		// -off sse2
		if(strcasecmp("-on",argv[num])==0){
			if(num+1>=argc) break;
			if(strcasecmp("3dn",argv[num+1])==0){
				gogo_opts.optUSE3DNOW = 0;
			} else if(strcasecmp("mmx",argv[num+1])==0){
				gogo_opts.optUSEMMX = 0;
			} else if(strcasecmp("sse",argv[num+1])==0 || strcasecmp("kni",argv[num+1])==0){
				gogo_opts.optUSEKNI = 0;
			} else if(strcasecmp("e3dn",argv[num+1])==0){
				gogo_opts.optUSEE3DNOW = 0;
			} else if(strcasecmp("sse",argv[num+1])==0){
				gogo_opts.optUSESSE = 0;
			} else if(strcasecmp("cmov",argv[num+1])==0){
				gogo_opts.optUSECMOV = 0;
			} else if(strcasecmp("emmx",argv[num+1])==0){
				gogo_opts.optUSEEMMX = 0;
			} else if(strcasecmp("sse2",argv[num+1])==0){
				gogo_opts.optUSESSE2 = 0;
			}
			num += 2;
			continue;
		}
		// emphasis type
		// -emp n : none
		// -emp 5 : 50/15ms (normal CD-DA emphasis)
		// -emp c : CCITT
		if(strcasecmp("-emp",argv[num])==0){
			if(num+1>=argc) break;
			if(strcasecmp("n",argv[num+1])==0){
				gogo_opts.optEMPHASIS = MC_EMP_NONE;
			} else if(strcasecmp("5",argv[num+1])==0){
				gogo_opts.optEMPHASIS = MC_EMP_5015MS;
			} else if(strcasecmp("c",argv[num+1])==0){
				gogo_opts.optEMPHASIS = MC_EMP_CCITT;
			}
			num += 2;
			continue;
		}
		// -cpu num
		if(strcasecmp("-cpu",argv[num])==0){
			if(num+1>=argc) break;
			gogo_opts.optCPU = atoi(argv[num+1]);
			num += 2;
			continue;
		}
		// -v 0..9 (low compression - high compression)
		if(strcasecmp("-v",argv[num])==0){
			if(num+1>=argc) break;
			gogo_opts.optVBR = atoi(argv[num+1]);
			num += 2;
			continue;
		}
		// -d freq : output MP3 frequency(kHz) 
		if(strcasecmp("-d",argv[num])==0){
			if(num+1>=argc) break;
			gogo_opts.optOUTFREQ = (int)(atof(argv[num+1])*1000);
			num += 2;
			continue;
		}
#if 0   // Do not use it.
		// -mono
		// -stereo
		// -8bit
		// -16bit
		// -bswap
		// -bswap-  (not bitswap)
		// -tos       (snd file for TonwsOS)
		// -tos-      (not snd file for TonwsOS
		if(strcasecmp("-mono",argv[num])==0){
			gogo_opts.optMONO_PCM = 1;
			num += 1;
			continue;
		}
		if(strcasecmp("-stereo",argv[num])==0){
			gogo_opts.optMONO_PCM = 0;
			num += 1;
			continue;
		}
		if(strcasecmp("-8bit",argv[num])==0){
			gogo_opts.opt8BIT_PCM = 1;
			num += 1;
			continue;
		}
		if(strcasecmp("-16bit",argv[num])==0){
			gogo_opts.opt8BIT_PCM = 0;
			num += 1;
			continue;
		}
		if(strcasecmp("-bswap",argv[num])==0){
			gogo_opts.optBYTE_SWAP = 1;
			num += 1;
			continue;
		}
		if(strcasecmp("-bswap-",argv[num])==0){
			gogo_opts.optBYTE_SWAP = 0;
			num += 1;
			continue;
		}
		if(strcasecmp("-ts",argv[num])==0){
			gogo_opts.optTOWNS_SND = 1;
			num += 1;
			continue;
		}
		if(strcasecmp("-ts-",argv[num])==0){
			gogo_opts.optTOWNS_SND = 1;
			num += 1;
			continue;
		}
		// -s freq  :  input PCM freqency(kHz)
		if(strcasecmp("-s",argv[num])==0){
			if(num+1>=argc) break;
			gogo_opts.optINPFREQ = (int)(atof(argv[num+1])*1000);
			num += 2;
			continue;
		}
		//  -offset byte   :  jump byte bytes from head.
		if(strcasecmp("-offset",argv[num])==0){
			if(num+1>=argc) break;
			gogo_opts.optSTARTOFFSET = atol(argv[num+1]);
			num += 2;
			continue;
		}
#endif
		// -th threshold mspower
		if(strcasecmp("-th",argv[num])==0){
			if(num+2>=argc) break;
			gogo_opts.optMSTHRESHOLD_threshold = atoi(argv[num+1]);
			gogo_opts.optMSTHRESHOLD_mspower = atoi(argv[num+2]);
			num += 3;
			continue;
		}
		// -riff normal	  :  mp3+TAG
		// -riff wave     :  RIFF/WAVE
		// -riff rmp      :  RIFF/RMP
		if(strcasecmp("-riff",argv[num])==0){
			if(num+1>=argc) break;
			if(strcasecmp("wave",argv[num+1])==0){
				gogo_opts.optOUTPUT_FORMAT = MC_OUTPUT_NORMAL;
			} else if(strcasecmp("wave",argv[num+1])==0){
				gogo_opts.optOUTPUT_FORMAT = MC_OUTPUT_RIFF_WAVE;
			} else if(strcasecmp("wave",argv[num+1])==0){
				gogo_opts.optOUTPUT_FORMAT = MC_OUTPUT_RIFF_RMP;
			}
			num += 2;
			continue;
		}
		//   -priority normal  : normal priority
		//   -priority num
		if(strcasecmp("-priority",argv[num])==0){
			if(num+1>=argc) break;
			if(strcasecmp("normal",argv[num+1])==0){
				gogo_opts.optTHREAD_PRIORITY = 10;
			} else {
				gogo_opts.optTHREAD_PRIORITY = atol(argv[num+1]);
			}
			num += 2;
			continue;
		}
		//  -readthread num
		if(strcasecmp("-readthread",argv[num])==0){
			if(num+1>=argc) break;
			gogo_opts.optREADTHREAD_PRIORITY = atol(argv[num+1]);
			num += 2;
			continue;
		}
		// -i
		// -i-
		if(strcasecmp("-i",argv[num])==0){
			gogo_opts.optVERIFY = 1;
			num += 1;
			continue;
		}
		if(strcasecmp("-i-",argv[num])==0){
			gogo_opts.optVERIFY = 0;
			num += 1;
			continue;
		}
		// -o directory_path
		if(strcasecmp("-o",argv[num])==0){
			if(num+1>=argc) break;
			strncpy((char *)gogo_opts.optOUTPUTDIR,argv[num+1],1023);
			gogo_opts.optOUTPUTDIR[1023] = '\0';
			num += 2;
			continue;
		}
		num += 1;
	}
	return;
}

// gogo.dll or gogo.lib の関数を用いた mp3 作成部分は別スレッドにて行う。　
// スレッド作成には CreateMPGEthread() を用いる。
// そのスレッドは、gogo_buffer_termination が非 0 になったとき必ず終了する。
// そのスレッドが終了したことは、IsTerminatedMPGEthread()により検知する。
// スレッドが終了したことを確認してから close_output() は完了する。 
// データの渡しは gogo_buffer にて行うが、バッファが一杯のときは、元スレッドが
// 停止し、バッファの読み込み待機時は gogo 用スレッドが停止する。
static int __stdcall MPGEthread(void)
{
	MERET	rval;
	struct MCP_INPDEV_USERFUNC mcp_inpdev_userfunc;
	unsigned long gogo_vercode;
	char gogo_verstring[1024];

	gogo_buffer_reset();
	rval = MPGE_initializeWork();
	if(rval != ME_NOERR){
		gogo_error(rval);
		gogo_buffer_termination  = -1;
		MPGE_endCoder();
		return -1;
	}
	MPGE_getVersion(&gogo_vercode,gogo_verstring);
	ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Gogo: %s", gogo_verstring);
	memset(&mcp_inpdev_userfunc,0,sizeof(struct MCP_INPDEV_USERFUNC));
	mcp_inpdev_userfunc.pUserFunc = (MPGE_USERFUNC)gogoUserFunc;
	mcp_inpdev_userfunc.nSize = MC_INPDEV_MEMORY_NOSIZE;
	mcp_inpdev_userfunc.nBit = (dpm.encoding & PE_16BIT) ? 16 : 8;
	mcp_inpdev_userfunc.nFreq = dpm.rate;
	mcp_inpdev_userfunc.nChn = (dpm.encoding & PE_MONO) ? 1 : 2;
#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
	if(use_gogo_commandline_options && gogo_commandline_options!=NULL){
		gogo_opts_reset();
		set_gogo_opts_use_commandline_options(gogo_commandline_options);
	}
#else
	gogo_ConfigDialogInfoApply();
#endif
	// titleがある場合はデフォルトでTAG付加
	if (tag_title != NULL) {
		gogo_opts_id3_tag(tag_title, NULL, NULL, NULL, NULL, -1);
	}
	set_gogo_opts();
	rval = MPGE_setConfigure( MC_INPUTFILE,MC_INPDEV_USERFUNC,(UPARAM)&mcp_inpdev_userfunc);
	if(rval != ME_NOERR){
		gogo_error(rval);
		gogo_buffer_termination  = -1;
		MPGE_endCoder();
		return -1;
	}
	if(gogo_vercode<300){
		rval = MPGE_setConfigure(MC_STARTOFFSET,(UPARAM)0,(UPARAM)0);
		if(rval != ME_NOERR){
			gogo_error(rval);
			gogo_buffer_termination  = -1;
			MPGE_endCoder();
			return -1;
		}
	}
#if 0
	rval = MPGE_setConfigure(MC_INPFREQ,(UPARAM)dpm.rate,(UPARAM)0);
	if(rval != ME_NOERR){
		gogo_error(rval);
		gogo_buffer_termination  = -1;
		MPGE_endCoder();
		return -1;
	}
#endif
	if(gogo_vercode<300){
	rval = MPGE_setConfigure(MC_BYTE_SWAP,(UPARAM)((dpm.encoding & PE_BYTESWAP) ? TRUE : FALSE),(UPARAM)0);
	if(rval != ME_NOERR){
		gogo_error(rval);
		gogo_buffer_termination  = -1;
		MPGE_endCoder();
		return -1;
	}
	rval = MPGE_setConfigure(MC_8BIT_PCM,(UPARAM)((dpm.encoding & PE_16BIT) ? FALSE : TRUE),(UPARAM)0);
	if(rval != ME_NOERR){
		gogo_error(rval);
		gogo_buffer_termination  = -1;
		MPGE_endCoder();
		return -1;
	}
	rval = MPGE_setConfigure(MC_MONO_PCM,(UPARAM)((dpm.encoding & PE_MONO) ? TRUE : FALSE),(UPARAM)0);
	if(rval != ME_NOERR){
		gogo_error(rval);
		gogo_buffer_termination  = -1;
		MPGE_endCoder();
		return -1;
	}
	}
	rval = MPGE_detectConfigure();
	if( rval != ME_NOERR ){
		gogo_error(rval);
		gogo_buffer_termination  = -1;
		MPGE_endCoder();
		return -1;
	} else {
//		UPARAM curFrame;
//		curFrame = 0;
		do {
//			ctl->cmsg(CMSG_INFO, VERB_DEBUG,"gogo_a : frame %d",curFrame);
//			curFrame++;
			rval = MPGE_processFrame();
		} while( rval == ME_NOERR );
		if( rval != ME_EMPTYSTREAM ){
			gogo_buffer_termination  = -1;
			gogo_error(rval);
		}
	}
	MPGE_closeCoder();
	MPGE_endCoder();
	return 0;
}

// Whether the thread of gogo.dll is terminated ?
static int IsTerminatedMPGEthread(void)
{
#ifndef __W32__
#else
	DWORD dwRes;
	if(hMPGEthread==NULL)
		return 1;
	dwRes = WaitForSingleObject(hMPGEthread,0);
	if(dwRes==WAIT_TIMEOUT){
		return 0;
	} else {
		return 1;
	}
#endif
}

// Create the thread of gogo.dll
static int CreateMPGEthread(void)
{
	// gogo 用のスレッドが終了しているか確認する。
	gogo_buffer_termination  = 1;
	while(!IsTerminatedMPGEthread()){
#ifndef __W32__
		usleep(100);
#else
		Sleep(0);
		Sleep(100);
#endif
	}
	gogo_buffer_termination  = 0;

	// gogo 用のスレッドを作成する。
#ifndef __W32__

#else
	// ハンドルはクローズしておく。
	if(hMPGEthread!=NULL){
		CloseHandle(hMPGEthread);
	}
	hMPGEthread = (HANDLE)crt_beginthreadex(NULL,0,(LPTHREAD_START_ROUTINE)MPGEthread,NULL,0,&dwMPGEthreadID);
	if(hMPGEthread==(HANDLE)-1 || hMPGEthread==NULL){
		hMPGEthread = NULL;
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : cannot create thread.");
		return -1;
	}
#endif
	return 0;
}


/**********************************************************************/
#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
extern volatile int w32g_gogo_id3_tag_dialog(void);
extern int w32g_interactive_id3_tag_set;
#endif
static int gogo_output_open(const char *fname)
{
	if(fname[0]=='\0')
		return -1;
	if(!gogo_opts_initflag)
		gogo_opts_init();
	strncpy((char *)gogo_opts.output_name,fname,1023);
	gogo_opts.output_name[1023] = '\0';
#if defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
	gogo_opts_reset_tag();
	if(w32g_interactive_id3_tag_set){	// 演奏時にID3タグ情報入力ダイアログを開く。
		w32g_gogo_id3_tag_dialog();
	}
#endif

	if(CreateMPGEthread()){
		gogo_buffer_termination  = -1;
		return -1;
	}

	return 0;
}

static int auto_gogo_output_open(const char *input_filename, const char *title)
{
  char *output_filename;

#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
  output_filename = create_auto_output_name(input_filename,"mp3",NULL,0);
#else
  gogo_ConfigDialogInfoApply();
  switch(gogo_opts.optOUTPUT_FORMAT){
  case MC_OUTPUT_NORMAL:
    output_filename = create_auto_output_name(input_filename,"mp3",(char *)w32g_output_dir,w32g_auto_output_mode);
    break;
  case MC_OUTPUT_RIFF_WAVE:
    output_filename = create_auto_output_name(input_filename,"wav",(char *)w32g_output_dir,w32g_auto_output_mode);
    break;
  case MC_OUTPUT_RIFF_RMP:
    output_filename = create_auto_output_name(input_filename,"rmp",(char *)w32g_output_dir,w32g_auto_output_mode);
    break;
  default:
    output_filename = create_auto_output_name(input_filename,"mp3",(char *)w32g_output_dir,w32g_auto_output_mode);
    break;
  }
#endif
  if(output_filename==NULL){
	  return -1;
  }
  if (tag_title != NULL) {
	free(tag_title);
	tag_title = NULL;
  }
  if (title != NULL) {
	tag_title = (char *)safe_malloc(sizeof(char)*(strlen(title)+1));
	strcpy(tag_title, title);
  }
  if((dpm.fd = gogo_output_open(output_filename)) == -1) {
    free(output_filename);
    return -1;
  }
  if(dpm.name != NULL){
    free(dpm.name);
    dpm.name = NULL;
  }
  dpm.name = output_filename;
  ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Output %s", dpm.name);
  return 0;
}

#ifdef AU_GOGO_DLL	
extern int gogo_dll_check(void);
#endif
static int open_output(void)
{
    int include_enc, exclude_enc;

#ifdef AU_VORBIS_DLL
	if(!gogo_dll_check()){
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "can not find gogo.dll.");
		return -1;
	}
#endif

    include_enc = exclude_enc = 0;
    if(dpm.encoding & PE_24BIT) {	/* 24 bit is not supported */
		exclude_enc |= PE_24BIT;
		include_enc |= PE_16BIT;
	}
    if(dpm.encoding & PE_16BIT || dpm.encoding & PE_24BIT) {
#ifdef LITTLE_ENDIAN
		exclude_enc |= PE_BYTESWAP;
#else
		include_enc |= PE_BYTESWAP;
#endif /* LITTLE_ENDIAN */
		include_enc |= PE_SIGNED;
    } else {
		exclude_enc |= PE_SIGNED;
#if 0
    } else if(!(dpm.encoding & (PE_ULAW|PE_ALAW))){
		exclude_enc = PE_SIGNED;
#endif
    }

    dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
    if(dpm.name == NULL) {
      dpm.flag |= PF_AUTO_SPLIT_FILE;
      dpm.name = NULL;
    } else {
      dpm.flag &= ~PF_AUTO_SPLIT_FILE;
      if((dpm.fd = gogo_output_open(dpm.name)) == -1)
		return -1;
    }
#else
	if(w32g_auto_output_mode>0){
      dpm.flag |= PF_AUTO_SPLIT_FILE;
      dpm.name = NULL;
    } else {
      dpm.flag &= ~PF_AUTO_SPLIT_FILE;
      if((dpm.fd = gogo_output_open(dpm.name)) == -1)
		return -1;
    }
#endif

    return 0;
}

static int output_data(char *readbuffer, int32 bytes)
{
	int32 rest_bytes = bytes;
	int32 out_bytes = 0;
 
	if(dpm.fd<0)
	  return 0;
 
	gogo_buffer_init();
	if(bytes>gogo_buffer_size){
		gogo_buffer_termination = -1;
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : buffer size is small.");
		return -1;
	}
	for(;;){
		int32 size;
		if(bytes<=0)
			break;
		if(gogo_buffer_termination<0){
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : output_data error.");
			return -1;
		}
		if(gogo_buffer_termination)
			return out_bytes;
		if(gogo_lock()){
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "gogo_a : lock error.");
			gogo_buffer_termination = -1;
			return -1;
		}
		size = __gogo_buffer_push(readbuffer,bytes);
		readbuffer += size;
		bytes -= size;
		out_bytes += size;
		gogo_unlock();
		if(bytes>0){
#ifndef __W32__
			usleep(100);
#else
			Sleep(0);
			Sleep(100);
#endif
		}
	}
  return out_bytes;
}

static void close_output(void)
{
  if (dpm.fd < 0)
    return;
 
  gogo_buffer_termination = 1;
  for (;;) {
    if (IsTerminatedMPGEthread()) {
      break;
    }
#ifndef __W32__
    usleep(100000);
#else
    Sleep(100);
#endif
  }
  dpm.fd = -1;
}

static int acntl(int request, void *arg)
{
	switch(request) {
	case PM_REQ_PLAY_START:
		if(dpm.flag & PF_AUTO_SPLIT_FILE)
			return auto_gogo_output_open(current_file_info->filename,current_file_info->seq_name);
		break;
	case PM_REQ_PLAY_END:
		if(dpm.flag & PF_AUTO_SPLIT_FILE) {
			close_output();
			return 0;
		}
		break;
	case PM_REQ_DISCARD:
#if 1
		gogo_buffer_reset();
#endif
		return 0;
	case PM_REQ_FLUSH:
	case PM_REQ_OUTPUT_FINISH:
		break;
	default:
		break;
	}
	return -1;
}

#endif

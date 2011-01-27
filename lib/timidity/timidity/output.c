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

    output.c

    Audio output (to file / device) functions.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifdef STDC_HEADERS
#include <string.h>
#include <ctype.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif
#include "timidity.h"
#include "common.h"
#include "output.h"
#include "tables.h"
#include "controls.h"
#include "audio_cnv.h"

int audio_buffer_bits = DEFAULT_AUDIO_BUFFER_BITS;

#define DEV_PLAY_MODE &wave_play_mode
extern PlayMode wave_play_mode;

PlayMode *play_mode_list[] = {
  DEV_PLAY_MODE,
  0
};

PlayMode *play_mode = &wave_play_mode;
PlayMode *target_play_mode = NULL;

/*****************************************************************/
/* Some functions to convert signed 32-bit data to other formats */

void s32tos8(int32 *lp, int32 c)
{
    int8 *cp=(int8 *)(lp);
    int32 l, i;

    for(i = 0; i < c; i++)
    {
	l=(lp[i])>>(32-8-GUARD_BITS);
	if (l>127) l=127;
	else if (l<-128) l=-128;
	cp[i] = (int8)(l);
    }
}

void s32tou8(int32 *lp, int32 c)
{
    uint8 *cp=(uint8 *)(lp);
    int32 l, i;

    for(i = 0; i < c; i++)
    {
	l=(lp[i])>>(32-8-GUARD_BITS);
	if (l>127) l=127;
	else if (l<-128) l=-128;
	cp[i] = 0x80 ^ ((uint8) l);
    }
}

void s32tos16(int32 *lp, int32 c)
{
  int16 *sp=(int16 *)(lp);
  int32 l, i;

  for(i = 0; i < c; i++)
    {
      l=(lp[i])>>(32-16-GUARD_BITS);
      if (l > 32767) l=32767;
      else if (l<-32768) l=-32768;
      sp[i] = (int16)(l);
    }
}

void s32tou16(int32 *lp, int32 c)
{
  uint16 *sp=(uint16 *)(lp);
  int32 l, i;

  for(i = 0; i < c; i++)
    {
      l=(lp[i])>>(32-16-GUARD_BITS);
      if (l > 32767) l=32767;
      else if (l<-32768) l=-32768;
      sp[i] = 0x8000 ^ (uint16)(l);
    }
}

void s32tos16x(int32 *lp, int32 c)
{
  int16 *sp=(int16 *)(lp);
  int32 l, i;

  for(i = 0; i < c; i++)
    {
      l=(lp[i])>>(32-16-GUARD_BITS);
      if (l > 32767) l=32767;
      else if (l<-32768) l=-32768;
      sp[i] = XCHG_SHORT((int16)(l));
    }
}

void s32tou16x(int32 *lp, int32 c)
{
  uint16 *sp=(uint16 *)(lp);
  int32 l, i;

  for(i = 0; i < c; i++)
    {
      l=(lp[i])>>(32-16-GUARD_BITS);
      if (l > 32767) l=32767;
      else if (l<-32768) l=-32768;
      sp[i] = XCHG_SHORT(0x8000 ^ (uint16)(l));
    }
}

#define MAX_24BIT_SIGNED (8388607)
#define MIN_24BIT_SIGNED (-8388608)

#define STORE_S24_LE(cp, l) *cp++ = l & 0xFF, *cp++ = l >> 8 & 0xFF, *cp++ = l >> 16
#define STORE_S24_BE(cp, l) *cp++ = l >> 16, *cp++ = l >> 8 & 0xFF, *cp++ = l & 0xFF
#define STORE_U24_LE(cp, l) *cp++ = l & 0xFF, *cp++ = l >> 8 & 0xFF, *cp++ = l >> 16 ^ 0x80
#define STORE_U24_BE(cp, l) *cp++ = l >> 16 ^ 0x80, *cp++ = l >> 8 & 0xFF, *cp++ = l & 0xFF

#ifdef LITTLE_ENDIAN
  #define STORE_S24  STORE_S24_LE
  #define STORE_S24X STORE_S24_BE
  #define STORE_U24  STORE_U24_LE
  #define STORE_U24X STORE_U24_BE
#else
  #define STORE_S24  STORE_S24_BE
  #define STORE_S24X STORE_S24_LE
  #define STORE_U24  STORE_U24_BE
  #define STORE_U24X STORE_U24_LE
#endif

void s32tos24(int32 *lp, int32 c)
{
	uint8 *cp = (uint8 *)(lp);
	int32 l, i;

	for(i = 0; i < c; i++)
	{
		l = (lp[i]) >> (32 - 24 - GUARD_BITS);
		l = (l > MAX_24BIT_SIGNED) ? MAX_24BIT_SIGNED
				: (l < MIN_24BIT_SIGNED) ? MIN_24BIT_SIGNED : l;
		STORE_S24(cp, l);
	}
}

void s32tou24(int32 *lp, int32 c)
{
	uint8 *cp = (uint8 *)(lp);
	int32 l, i;

	for(i = 0; i < c; i++)
	{
		l = (lp[i]) >> (32 - 24 - GUARD_BITS);
		l = (l > MAX_24BIT_SIGNED) ? MAX_24BIT_SIGNED
				: (l < MIN_24BIT_SIGNED) ? MIN_24BIT_SIGNED : l;
		STORE_U24(cp, l);
	}
}

void s32tos24x(int32 *lp, int32 c)
{
	uint8 *cp = (uint8 *)(lp);
	int32 l, i;

	for(i = 0; i < c; i++)
	{
		l = (lp[i]) >> (32 - 24 - GUARD_BITS);
		l = (l > MAX_24BIT_SIGNED) ? MAX_24BIT_SIGNED
				: (l < MIN_24BIT_SIGNED) ? MIN_24BIT_SIGNED : l;
		STORE_S24X(cp, l);
	}
}

void s32tou24x(int32 *lp, int32 c)
{
	uint8 *cp = (uint8 *)(lp);
	int32 l, i;

	for(i = 0; i < c; i++)
	{
		l = (lp[i]) >> (32 - 24 - GUARD_BITS);
		l = (l > MAX_24BIT_SIGNED) ? MAX_24BIT_SIGNED
				: (l < MIN_24BIT_SIGNED) ? MIN_24BIT_SIGNED : l;
		STORE_U24X(cp, l);
	}
}

void s32toulaw(int32 *lp, int32 c)
{
    int8 *up=(int8 *)(lp);
    int32 l, i;

    for(i = 0; i < c; i++)
    {
	l=(lp[i])>>(32-16-GUARD_BITS);
	if (l > 32767) l=32767;
	else if (l<-32768) l=-32768;
	up[i] = AUDIO_S2U(l);
    }
}

void s32toalaw(int32 *lp, int32 c)
{
    int8 *up=(int8 *)(lp);
    int32 l, i;

    for(i = 0; i < c; i++)
    {
	l=(lp[i])>>(32-16-GUARD_BITS);
	if (l > 32767) l=32767;
	else if (l<-32768) l=-32768;
	up[i] = AUDIO_S2A(l);
    }
}

/* return: number of bytes */
int32 general_output_convert(int32 *buf, int32 count)
{
    int32 bytes;

    if(!(play_mode->encoding & PE_MONO))
	count *= 2; /* Stereo samples */
    bytes = count;
    if(play_mode->encoding & PE_16BIT)
    {
	bytes *= 2;
	if(play_mode->encoding & PE_BYTESWAP)
	{
	    if(play_mode->encoding & PE_SIGNED)
		s32tos16x(buf, count);
	    else
		s32tou16x(buf, count);
	}
	else if(play_mode->encoding & PE_SIGNED)
	    s32tos16(buf, count);
	else
	    s32tou16(buf, count);
    }
	else if(play_mode->encoding & PE_24BIT) {
		bytes *= 3;
		if(play_mode->encoding & PE_BYTESWAP)
		{
			if(play_mode->encoding & PE_SIGNED)
			s32tos24x(buf, count);
			else
			s32tou24x(buf, count);
		} else if(play_mode->encoding & PE_SIGNED)
			s32tos24(buf, count);
		else
			s32tou24(buf, count);
    }
	else if(play_mode->encoding & PE_ULAW)
	s32toulaw(buf, count);
    else if(play_mode->encoding & PE_ALAW)
	s32toalaw(buf, count);
    else if(play_mode->encoding & PE_SIGNED)
	s32tos8(buf, count);
    else
	s32tou8(buf, count);
    return bytes;
}

int validate_encoding(int enc, int include_enc, int exclude_enc)
{
    const char *orig_enc_name, *enc_name;
    int orig_enc;

    orig_enc = enc;
    orig_enc_name = output_encoding_string(enc);
    enc |= include_enc;
    enc &= ~exclude_enc;
    if(enc & (PE_ULAW|PE_ALAW))
	enc &= ~(PE_24BIT|PE_16BIT|PE_SIGNED|PE_BYTESWAP);
    if(!(enc & PE_16BIT || enc & PE_24BIT))
	enc &= ~PE_BYTESWAP;
	if(enc & PE_24BIT)
	enc &= ~PE_16BIT;	/* 24bit overrides 16bit */
    enc_name = output_encoding_string(enc);
    if(strcmp(orig_enc_name, enc_name) != 0)
	ctl->cmsg(CMSG_WARNING, VERB_NOISY,
		  "Notice: Audio encoding is changed `%s' to `%s'",
		  orig_enc_name, enc_name);
    return enc;
}

const char *output_encoding_string(int enc)
{
    if(enc & PE_MONO)
    {
	if(enc & PE_16BIT)
	{
	    if(enc & PE_SIGNED)
		return "16bit (mono)";
	    else
		return "unsigned 16bit (mono)";
	}
	else if(enc & PE_24BIT)
	{
	    if(enc & PE_SIGNED)
		return "24bit (mono)";
	    else
		return "unsigned 24bit (mono)";
	}
	else
	{
	    if(enc & PE_ULAW)
		return "U-law (mono)";
	    else if(enc & PE_ALAW)
		return "A-law (mono)";
	    else if(enc & PE_SIGNED)
		return "8bit (mono)";
	    else
		return "unsigned 8bit (mono)";
	}
    }
    else if(enc & PE_16BIT)
    {
	if(enc & PE_BYTESWAP)
	{
	    if(enc & PE_SIGNED)
		return "16bit (swap)";
	    else
		return "unsigned 16bit (swap)";
	}
	else if(enc & PE_SIGNED)
	    return "16bit";
	else
	    return "unsigned 16bit";
    }
    else if(enc & PE_24BIT)
    {
	if(enc & PE_SIGNED)
	    return "24bit";
	else
	    return "unsigned 24bit";
    }
    else
	if(enc & PE_ULAW)
	    return "U-law";
	else if(enc & PE_ALAW)
	    return "A-law";
	else if(enc & PE_SIGNED)
	    return "8bit";
	else
	    return "unsigned 8bit";
    /*NOTREACHED*/
}

/* mode
  0,1: Default mode.
  2: Remove the directory path of input_filename, then add output_dir.
  3: Replace directory separator characters ('/','\',':') with '_', then add output_dir.
 */
char *create_auto_output_name(const char *input_filename, char *ext_str, char *output_dir, int mode)
{
  char *output_filename;
  char *ext, *p;
  int32 dir_len = 0;
  char ext_str_tmp[65];

  output_filename = (char *)safe_malloc((output_dir?strlen(output_dir):0) + strlen(input_filename) + 6);
  if(output_filename==NULL)
    return NULL;
  output_filename[0] = '\0';
  if(output_dir!=NULL && (mode==2 || mode==3)) {
    strcat(output_filename,output_dir);
    dir_len = strlen(output_filename);
#ifndef __W32__
    if(dir_len>0 && output_filename[dir_len-1]!=PATH_SEP){
#else
      if(dir_len>0 && output_filename[dir_len-1]!='/' && output_filename[dir_len-1]!='\\' && output_filename[dir_len-1]!=':'){
#endif
	strcat(output_filename,PATH_STRING);
	dir_len++;
      }
    }
    strcat(output_filename, input_filename);

    if((ext = strrchr(output_filename, '.')) == NULL)
      ext = output_filename + strlen(output_filename);
    else {
      /* strip ".gz" */
      if(strcasecmp(ext, ".gz") == 0) {
	*ext = '\0';
	if((ext = strrchr(output_filename, '.')) == NULL)
	  ext = output_filename + strlen(output_filename);
      }
    }

    /* replace '\' , '/' or PATH_SEP between '#' and ext */
    p = strrchr(output_filename,'#');
    if(p!=NULL){
      char *p1;
#ifdef _mbsrchr
#define STRCHR(a,b) _mbschr(a,b)
#else
#define STRCHR(a,b) strchr(a,b)
#endif
#ifndef __W32__
      p1 = p + 1;
      while((p1 = STRCHR(p1,PATH_SEP))!=NULL && p1<ext){
        *p1 = '_';
	p1++;
      }
#else
      p1 = p + 1;
      while((p1 = STRCHR(p1,'\\'))!=NULL && p1<ext){
      	*p1 = '_';
	p1++;
      }
      p1 = p;
      while((p1 = STRCHR(p1,'/'))!=NULL && p1<ext){
	*p1 = '_';
	p1++;
      }
#endif
#undef STRCHR
    }

    /* replace '.' and '#' before ext */
    for(p = output_filename; p < ext; p++)
#ifndef __W32__
      if(*p == '.' || *p == '#')
#else
	if(*p == '#')
#endif
	  *p = '_';

    if(mode==2){
      char *p1,*p2,*p3;
#ifndef __W32__
      p = strrchr(output_filename+dir_len,PATH_SEP);
#else
#ifdef _mbsrchr
#define STRRCHR _mbsrchr
#else
#define STRRCHR strrchr
#endif
      p1 = STRRCHR(output_filename+dir_len,'/');
      p2 = STRRCHR(output_filename+dir_len,'\\');
      p3 = STRRCHR(output_filename+dir_len,':');
#undef STRRCHR
      p1>p2 ? (p1>p3 ? (p = p1) : (p = p3)) : (p2>p3 ? (p = p2) : (p = p3));
#endif
      if(p!=NULL){
	for(p1=output_filename+dir_len,p2=p+1; *p2; p1++,p2++)
	  *p1 = *p2;
	*p1 = '\0';
      }
    }

    if(mode==3){
      for(p=output_filename+dir_len; *p; p++)
#ifndef __W32__
	if(*p==PATH_SEP)
#else
	  if(*p=='/' || *p=='\\' || *p==':')
#endif
	    *p = '_';
    }

    if((ext = strrchr(output_filename, '.')) == NULL)
      ext = output_filename + strlen(output_filename);
    if(*ext){
      strncpy(ext_str_tmp,ext_str,64);
      ext_str_tmp[64]=0;
      if(isupper(*(ext + 1))){
	for(p=ext_str_tmp;*p;p++)
	  *p = toupper(*p);
	*p = '\0';
      } else {
	for(p=ext_str_tmp;*p;p++)
	  *p = tolower(*p);
	*p = '\0';
      }
      strcpy(ext+1,ext_str_tmp);
    }
    return output_filename;
}

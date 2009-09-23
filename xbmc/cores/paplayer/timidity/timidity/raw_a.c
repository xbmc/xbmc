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

    raw_audio.c

    Functions to output raw sound data to a file or stdout.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>

#ifdef __W32__
#include <stdlib.h>
#include <io.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

#include <fcntl.h>

#ifdef __FreeBSD__
#include <stdio.h>
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);

/* export the playback mode */

#define dpm raw_play_mode

PlayMode dpm = {
    DEFAULT_RATE, PE_16BIT|PE_SIGNED, PF_PCM_STREAM,
    -1,
    {0,0,0,0,0},
    "Raw waveform data", 'r',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl
};

/*************************************************************************/

/*
 * Get the filename extention from the encoding
 * This extension is available for sox.
 */
static const char *encoding_ext(int32 encoding) {
  static char ext[5], *p;

  if(encoding & PE_ULAW) {
    return ".ul";
  }
  if(encoding & PE_ALAW) {
    return ".raw"; /* ?? */
  }

  p = ext;
  *p++ = '.';
  if(encoding & PE_SIGNED)
    *p++ = 's';
  else
    *p++ = 'u';
  if(encoding & PE_16BIT)
    *p++ = 'w';
  else if(encoding & PE_24BIT)
    *p++ = '2', *p++ = '4'; /* is there any common extension? */
  else
    *p++ = 'b';
  *p = '\0';
  return ext;
}

static int raw_output_open(const char *fname)
{
  int fd;

  if(strcmp(fname, "-") == 0)
    return 1; /* data to stdout */
  if((fd = open(fname, FILE_OUTPUT_MODE)) < 0)
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
	      fname, strerror(errno));
  return fd;
}

static int auto_raw_output_open(const char *input_filename)
{
  
  char *output_filename = (char *)safe_malloc(strlen(input_filename) + 5);
  char *ext, *p;

  strcpy(output_filename, input_filename);
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

  /* replace '.' and '#' before ext */
  for(p = output_filename; p < ext; p++)
    if(*p == '.' || *p == '#')
      *p = '_';

  if(*ext && isupper(*(ext + 1))) {
    strcpy(ext, encoding_ext(dpm.encoding));
    ext++;
    while(*ext) {
      if(islower(*ext))
	*ext = toupper(*ext);
      ext++;
    }
  } else
    strcpy(ext, encoding_ext(dpm.encoding));
  if((dpm.fd = raw_output_open(output_filename)) < 0) {
    free(output_filename);
    return -1;
  }
  if(dpm.name != NULL)
    free(dpm.name);
  dpm.name = output_filename;
  ctl->cmsg(CMSG_INFO, VERB_NORMAL, "Output %s", dpm.name);
  return 0;

}

static int open_output(void)
{
  dpm.encoding = validate_encoding(dpm.encoding, 0, 0);

  if(dpm.name == NULL) {
    dpm.flag |= PF_AUTO_SPLIT_FILE;
    dpm.name = NULL;
  } else {
    dpm.flag &= ~PF_AUTO_SPLIT_FILE;
    if((dpm.fd = raw_output_open(dpm.name)) == -1)
      return -1;
  }

  return 0;
}

static int output_data(char *buf, int32 bytes)
{
    int n;

    if(dpm.fd == -1)
      return -1;

    while(((n = write(dpm.fd, buf, bytes)) == -1) && errno == EINTR)
	;
    if(n == -1)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s",
		  dpm.name, strerror(errno));
	return -1;
    }
    return n;
}

static void close_output(void)
{
    if(dpm.fd != 1 && dpm.fd != -1) /* We don't close stdout */
	close(dpm.fd);
    dpm.fd = -1;
}

static int acntl(int request, void *arg)
{
  switch(request) {
  case PM_REQ_PLAY_START:
    if(dpm.flag & PF_AUTO_SPLIT_FILE)
      return auto_raw_output_open(current_file_info->filename);
    break;
  case PM_REQ_PLAY_END:
    if(dpm.flag & PF_AUTO_SPLIT_FILE) {
      close_output();
      return 0;
    }
    break;
  case PM_REQ_DISCARD:
    return 0;
  }
  return -1;
}

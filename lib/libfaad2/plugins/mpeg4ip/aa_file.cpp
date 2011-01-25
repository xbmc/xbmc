/*
** MPEG4IP plugin for FAAD2
** Copyright (C) 2003 Bill May wmay@cisco.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: aa_file.cpp,v 1.2 2004/01/05 14:05:12 menno Exp $
**/
/*
 * aa_file.cpp - create media structure for aac files
 */

#include "faad2.h"
codec_data_t *aac_file_check (lib_message_func_t message,
                  const char *name,
                  double *max,
                  char *desc[4]
#ifdef HAVE_PLUGIN_VERSION_0_8
                , CConfigSet *pConfig
#endif
)
{
  aac_codec_t *aac;
  int len = strlen(name);
  if (strcasecmp(name + len - 4, ".aac") != 0) {
    return (NULL);
  }

  aac = MALLOC_STRUCTURE(aac_codec_t);
  memset(aac, 0, sizeof(*aac));
  *max = 0;

  aac->m_buffer = (uint8_t *)malloc(MAX_READ_BUFFER);
  aac->m_buffer_size_max = MAX_READ_BUFFER;
  aac->m_ifile = fopen(name, FOPEN_READ_BINARY);
  if (aac->m_ifile == NULL) {
    free(aac);
    return NULL;
  }
  aac->m_output_frame_size = 1024;
  aac->m_info = faacDecOpen(); // use defaults here...
  aac->m_buffer_size = fread(aac->m_buffer,
                 1,
                 aac->m_buffer_size_max,
                 aac->m_ifile);

  unsigned long freq;
  unsigned char chans;

  faacDecInit(aac->m_info, (unsigned char *)aac->m_buffer,
          aac->m_buffer_size, &freq, &chans);
  // may want to actually decode the first frame...
  if (freq == 0) {
    message(LOG_ERR, aaclib, "Couldn't determine AAC frame rate");
    aac_close((codec_data_t *)aac);
    return (NULL);
  }
  aac->m_freq = freq;
  aac->m_chans = chans;
  aac->m_faad_inited = 1;
  aac->m_framecount = 0;
  return ((codec_data_t *)aac);
}


int aac_file_next_frame (codec_data_t *your,
             uint8_t **buffer,
             uint64_t *ts)
{
  aac_codec_t *aac = (aac_codec_t *)your;

  if (aac->m_buffer_on > 0) {
    memmove(aac->m_buffer,
        &aac->m_buffer[aac->m_buffer_on],
        aac->m_buffer_size - aac->m_buffer_on);
  }
  aac->m_buffer_size -= aac->m_buffer_on;
  aac->m_buffer_size += fread(aac->m_buffer + aac->m_buffer_size,
                  1,
                  aac->m_buffer_size_max - aac->m_buffer_size,
                  aac->m_ifile);
  aac->m_buffer_on = 0;
  if (aac->m_buffer_size == 0) return 0;


  uint64_t calc;
  calc = aac->m_framecount * 1024 * M_LLU;
  calc /= aac->m_freq;
  *ts = calc;
  *buffer = aac->m_buffer;
  aac->m_framecount++;
  return (aac->m_buffer_size);
}

void aac_file_used_for_frame (codec_data_t *ifptr,
                 uint32_t bytes)
{
  aac_codec_t *aac = (aac_codec_t *)ifptr;
  aac->m_buffer_on += bytes;
  if (aac->m_buffer_on > aac->m_buffer_size) aac->m_buffer_on = aac->m_buffer_size;
}

int aac_file_eof (codec_data_t *ifptr)
{
  aac_codec_t *aac = (aac_codec_t *)ifptr;
  return aac->m_buffer_on == aac->m_buffer_size && feof(aac->m_ifile);
}

int aac_raw_file_seek_to (codec_data_t *ifptr, uint64_t ts)
{
  if (ts != 0) return -1;

  aac_codec_t *aac = (aac_codec_t *)ifptr;
  rewind(aac->m_ifile);
  aac->m_buffer_size = aac->m_buffer_on = 0;
  aac->m_framecount = 0;
  return 0;
}

/*
 * Copyright (C) 2002-2003 the xine project
 * 
 * This file is part of xine, a free video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: input_mms.c,v 1.1 2004/02/15 19:57:21 mathrick Exp $
 *
 * mms input plugin based on work from major mms
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_MODULE "input_mms"
#define LOG_VERBOSE
/*
#define LOG
*/

#include "bswap.h"
#include "xine_internal.h"
#include "xineutils.h"
#include "input_plugin.h"

#include "mms.h"
#include "mmsh.h"
#include "net_buf_ctrl.h"

#define PROTOCOL_UNDEFINED 0
#define PROTOCOL_MMST      1
#define PROTOCOL_MMSH      2

#if !defined(NDELAY) && defined(O_NDELAY)
#define FNDELAY O_NDELAY
#endif

/* network bandwidth */
const uint32_t mms_bandwidths[]={14400,19200,28800,33600,34430,57600,
                                  115200,262200,393216,524300,1544000,10485800};

const char * mms_bandwidth_strs[]={"14.4 Kbps (Modem)", "19.2 Kbps (Modem)",
                                   "28.8 Kbps (Modem)", "33.6 Kbps (Modem)",
                                   "34.4 Kbps (Modem)", "57.6 Kbps (Modem)",
                                   "115.2 Kbps (ISDN)", "262.2 Kbps (Cable/DSL)",
                                   "393.2 Kbps (Cable/DSL)","524.3 Kbps (Cable/DSL)",
                                   "1.5 Mbps (T1)", "10.5 Mbps (LAN)", NULL};

typedef struct {
  input_plugin_t   input_plugin;

  xine_stream_t   *stream;
  mms_t           *mms;
  mmsh_t          *mmsh;

  char            *mrl;

  off_t            curpos;

  nbc_t           *nbc; 

  char             scratch[1025];

  int              bandwidth;
  int              protocol;       /* mmst or mmsh */
  
} mms_input_plugin_t;

typedef struct {

  input_class_t       input_class;
  
  mms_input_plugin_t *ip;

  xine_t             *xine;
} mms_input_class_t;

static off_t mms_plugin_read (input_plugin_t *this_gen, 
                              char *buf, off_t len) {
  mms_input_plugin_t *this = (mms_input_plugin_t *) this_gen;
  off_t               n = 0;

  lprintf ("mms_plugin_read: %lld bytes ...\n", len);

  nbc_check_buffers (this->nbc);

  switch (this->protocol) {
    case PROTOCOL_MMST:
      n = mms_read (this->mms, buf, len);
      break;
    case PROTOCOL_MMSH:
      n = mmsh_read (this->mmsh, buf, len);
      break;
  }
              
  this->curpos += n;

  return n;
}

static buf_element_t *mms_plugin_read_block (input_plugin_t *this_gen,
                                             fifo_buffer_t *fifo, off_t todo) {
  /*mms_input_plugin_t   *this = (mms_input_plugin_t *) this_gen; */
  buf_element_t        *buf = fifo->buffer_pool_alloc (fifo);
  int                   total_bytes;

  lprintf ("mms_plugin_read_block: %lld bytes...\n", todo);

  buf->content = buf->mem;
  buf->type = BUF_DEMUX_BLOCK;
  
  total_bytes = mms_plugin_read (this_gen, buf->content, todo);

  if (total_bytes != todo) {
    buf->free_buffer (buf);
    return NULL;
  }

  buf->size = total_bytes;

  return buf;
}

static off_t mms_plugin_seek (input_plugin_t *this_gen, off_t offset, int origin) {
  mms_input_plugin_t   *this = (mms_input_plugin_t *) this_gen; 
  off_t                 dest = this->curpos;

  lprintf ("mms_plugin_seek: %lld offset, %d origin...\n", offset, origin);

  switch (origin) {
  case SEEK_SET:
    dest = offset;
    break;
  case SEEK_CUR:
    dest = this->curpos + offset;
    break;
  case SEEK_END:
    printf ("input_mms: SEEK_END not implemented!\n");
    return this->curpos;
  default:
    printf ("input_mms: unknown origin in seek!\n");
    return this->curpos;
  }

  if (this->curpos > dest) {
    printf ("input_mms: cannot seek back!\n");
    return this->curpos;
  }

  while (this->curpos<dest) {
    int n = 0;
    int diff;

    diff = dest - this->curpos;

    if (diff>1024)
      diff = 1024;

    switch (this->protocol) {
      case PROTOCOL_MMST:
        n = mms_read (this->mms, this->scratch, diff);
        break;
      case PROTOCOL_MMSH:
        n = mmsh_read (this->mmsh, this->scratch, diff);
        break;
    }
    
    this->curpos += n;

    if (n < diff)
      return this->curpos;

  }

  return this->curpos;
}

static off_t mms_plugin_get_length (input_plugin_t *this_gen) {
  mms_input_plugin_t   *this = (mms_input_plugin_t *) this_gen; 
  off_t                 length = 0;

  if (!this->mms)
    return 0;

  switch (this->protocol) {
    case PROTOCOL_MMST:
      length = mms_get_length (this->mms);
      break;
    case PROTOCOL_MMSH:
      length = mmsh_get_length (this->mmsh);
      break;
  }

  lprintf ("length is %lld\n", length);

  return length;

}

static uint32_t mms_plugin_get_capabilities (input_plugin_t *this_gen) {
  return INPUT_CAP_PREVIEW;
}

static uint32_t mms_plugin_get_blocksize (input_plugin_t *this_gen) {
  return 0;
}

static off_t mms_plugin_get_current_pos (input_plugin_t *this_gen){
  mms_input_plugin_t *this = (mms_input_plugin_t *) this_gen;

  /*
  printf ("current pos is %lld\n", this->curpos);
  */

  return this->curpos;
}

static void mms_plugin_dispose (input_plugin_t *this_gen) {
  mms_input_plugin_t *this = (mms_input_plugin_t *) this_gen;

  if (this->mms)
    mms_close (this->mms);
  
  if (this->mmsh)
    mmsh_close (this->mmsh);
  
  this->mms  = NULL;
  this->mmsh = NULL;

  if (this->nbc) {
    nbc_close (this->nbc);
    this->nbc = NULL;
  }
  
  if(this->mrl)
    free(this->mrl);
  
  free (this);
}

static char* mms_plugin_get_mrl (input_plugin_t *this_gen) {
  mms_input_plugin_t *this = (mms_input_plugin_t *) this_gen;

  return this->mrl;
}

static int mms_plugin_get_optional_data (input_plugin_t *this_gen, 
                                         void *data, int data_type) {
  mms_input_plugin_t *this = (mms_input_plugin_t *) this_gen;

  switch (data_type) {

  case INPUT_OPTIONAL_DATA_PREVIEW:
    switch (this->protocol) {
      case PROTOCOL_MMST:
        return mms_peek_header (this->mms, data, MAX_PREVIEW_SIZE);
        break;
      case PROTOCOL_MMSH:
        return mmsh_peek_header (this->mmsh, data, MAX_PREVIEW_SIZE);
        break;
    }
    break;
    
  default:
    return INPUT_OPTIONAL_UNSUPPORTED;
    break;

  }

  return INPUT_OPTIONAL_UNSUPPORTED;
}

static void bandwidth_changed_cb (void *this_gen, xine_cfg_entry_t *entry) {
  mms_input_class_t *class = (mms_input_class_t*) this_gen;

  lprintf ("bandwidth_changed_cb %d\n", entry->num_value);

  if(!class)
   return;

  if(class->ip && ((entry->num_value >= 0) && (entry->num_value <= 11))) {
    mms_input_plugin_t *this = class->ip;

    this->bandwidth = mms_bandwidths[entry->num_value];
  }
}

static int mms_plugin_open (input_plugin_t *this_gen) {
  mms_input_plugin_t *this = (mms_input_plugin_t *) this_gen;
  mms_t              *mms  = NULL;
  mmsh_t             *mmsh = NULL;
  
  switch (this->protocol) {
    case PROTOCOL_UNDEFINED:
      mms = mms_connect (this->stream, this->mrl, this->bandwidth);
      if (mms) {
        this->protocol = PROTOCOL_MMST;
      } else {
        mmsh = mmsh_connect (this->stream, this->mrl, this->bandwidth);
        this->protocol = PROTOCOL_MMSH;
      }
      break;
    case PROTOCOL_MMST:
      mms = mms_connect (this->stream, this->mrl, this->bandwidth);
      break;
    case PROTOCOL_MMSH:
      mmsh = mmsh_connect (this->stream, this->mrl, this->bandwidth);
      break;
  }
  
  if (!mms && !mmsh) {
    return 0;
  }
  
  this->mms      = mms;
  this->mmsh     = mmsh;
  this->curpos   = 0;
  
  return 1;
}

static input_plugin_t *mms_class_get_instance (input_class_t *cls_gen, xine_stream_t *stream, 
				    const char *data) {

  mms_input_class_t  *cls = (mms_input_class_t *) cls_gen;
  mms_input_plugin_t *this;
  char               *mrl  = strdup(data);
  xine_cfg_entry_t    bandwidth_entry;
  int                 protocol;
  
  lprintf ("trying to open '%s'\n", mrl);

  if (!strncasecmp (mrl, "mms://", 6)) {
    protocol = PROTOCOL_UNDEFINED;
  } else if (!strncasecmp (mrl, "mmst://", 7)) {
    protocol =   PROTOCOL_MMST;
  } else if (!strncasecmp (mrl, "mmsh://", 7)) {
    protocol =   PROTOCOL_MMSH;
  } else {
    free (mrl);
    return NULL;
  }

  this = (mms_input_plugin_t *) xine_xmalloc (sizeof (mms_input_plugin_t));
  cls->ip = this;
  this->stream   = stream;
  this->mms      = NULL;
  this->mmsh     = NULL;
  this->protocol = protocol;
  this->mrl      = mrl; 
  this->curpos   = 0;
  this->nbc      = nbc_init (this->stream);

  if (xine_config_lookup_entry (stream->xine, "input.mms_network_bandwidth", 
                                &bandwidth_entry)) {
    bandwidth_changed_cb(cls, &bandwidth_entry);
  }

  this->input_plugin.open              = mms_plugin_open;
  this->input_plugin.get_capabilities  = mms_plugin_get_capabilities;
  this->input_plugin.read              = mms_plugin_read;
  this->input_plugin.read_block        = mms_plugin_read_block;
  this->input_plugin.seek              = mms_plugin_seek;
  this->input_plugin.get_current_pos   = mms_plugin_get_current_pos;
  this->input_plugin.get_length        = mms_plugin_get_length;
  this->input_plugin.get_blocksize     = mms_plugin_get_blocksize;
  this->input_plugin.get_mrl           = mms_plugin_get_mrl;
  this->input_plugin.dispose           = mms_plugin_dispose;
  this->input_plugin.get_optional_data = mms_plugin_get_optional_data;
  this->input_plugin.input_class       = cls_gen;

  
  return &this->input_plugin;
}

/*
 * mms input plugin class stuff
 */

static char *mms_class_get_description (input_class_t *this_gen) {
  return _("mms streaming input plugin");
}

static char *mms_class_get_identifier (input_class_t *this_gen) {
  return "mms";
}

static void mms_class_dispose (input_class_t *this_gen) {
  mms_input_class_t  *this = (mms_input_class_t *) this_gen;

  free (this);
}

static void *init_class (xine_t *xine, void *data) {

  mms_input_class_t  *this;

  this = (mms_input_class_t *) xine_xmalloc (sizeof (mms_input_class_t));

  this->xine   = xine;
  this->ip                             = NULL;

  this->input_class.get_instance       = mms_class_get_instance;
  this->input_class.get_identifier     = mms_class_get_identifier;
  this->input_class.get_description    = mms_class_get_description;
  this->input_class.get_dir            = NULL;
  this->input_class.get_autoplay_list  = NULL;
  this->input_class.dispose            = mms_class_dispose;
  this->input_class.eject_media        = NULL;

  xine->config->register_enum(xine->config, "input.mms_network_bandwidth", 10,
			      (char **)mms_bandwidth_strs,
			      "Network bandwidth",
			      NULL, 0, bandwidth_changed_cb, (void*) this);
  
  return this;
}

/*
 * exported plugin catalog entry
 */

plugin_info_t xine_plugin_info[] = {
  /* type, API, "name", version, special_info, init_function */  
  { PLUGIN_INPUT, 14, "mms", XINE_VERSION_CODE, NULL, init_class },
  { PLUGIN_NONE, 0, "", 0, NULL, NULL }
};

/*
 * Copyright (C) 2000-2003 the xine project
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
 * $Id: asfheader.h,v 1.2 2005/12/12 09:24:19 theuraeus Exp $
 *
 * demultiplexer for asf streams
 *
 * based on ffmpeg's
 * ASF compatible encoder and decoder.
 * Copyright (c) 2000, 2001 Gerard Lantau.
 *
 * GUID list from avifile
 * some other ideas from MPlayer
 */

#ifndef ASFHEADER_H
#define ASFHEADER_H

/*
 * define asf GUIDs (list from avifile)
 */
#define GUID_ERROR                              0

    /* base ASF objects */
#define GUID_ASF_HEADER                         1
#define GUID_ASF_DATA                           2
#define GUID_ASF_SIMPLE_INDEX                   3
#define GUID_INDEX                              4
#define GUID_MEDIA_OBJECT_INDEX                 5
#define GUID_TIMECODE_INDEX                     6

    /* header ASF objects */
#define GUID_ASF_FILE_PROPERTIES                7
#define GUID_ASF_STREAM_PROPERTIES              8
#define GUID_ASF_HEADER_EXTENSION               9
#define GUID_ASF_CODEC_LIST                    10
#define GUID_ASF_SCRIPT_COMMAND                11
#define GUID_ASF_MARKER                        12
#define GUID_ASF_BITRATE_MUTUAL_EXCLUSION      13
#define GUID_ASF_ERROR_CORRECTION              14
#define GUID_ASF_CONTENT_DESCRIPTION           15
#define GUID_ASF_EXTENDED_CONTENT_DESCRIPTION  16
#define GUID_ASF_STREAM_BITRATE_PROPERTIES     17
#define GUID_ASF_EXTENDED_CONTENT_ENCRYPTION   18
#define GUID_ASF_PADDING                       19
    
    /* stream properties object stream type */
#define GUID_ASF_AUDIO_MEDIA                   20
#define GUID_ASF_VIDEO_MEDIA                   21
#define GUID_ASF_COMMAND_MEDIA                 22
#define GUID_ASF_JFIF_MEDIA                    23
#define GUID_ASF_DEGRADABLE_JPEG_MEDIA         24
#define GUID_ASF_FILE_TRANSFER_MEDIA           25
#define GUID_ASF_BINARY_MEDIA                  26

    /* stream properties object error correction type */
#define GUID_ASF_NO_ERROR_CORRECTION           27
#define GUID_ASF_AUDIO_SPREAD                  28

    /* mutual exclusion object exlusion type */
#define GUID_ASF_MUTEX_BITRATE                 29
#define GUID_ASF_MUTEX_UKNOWN                  30

    /* header extension */
#define GUID_ASF_RESERVED_1                    31
    
    /* script command */
#define GUID_ASF_RESERVED_SCRIPT_COMMNAND      32

    /* marker object */
#define GUID_ASF_RESERVED_MARKER               33

    /* various */
/*
#define GUID_ASF_HEAD2                         27
*/
#define GUID_ASF_AUDIO_CONCEAL_NONE            34
#define GUID_ASF_CODEC_COMMENT1_HEADER         35
#define GUID_ASF_2_0_HEADER                    36

#define GUID_END                               37


/* asf stream types */
#define ASF_STREAM_TYPE_UNKNOWN           0
#define ASF_STREAM_TYPE_AUDIO             1
#define ASF_STREAM_TYPE_VIDEO             2
#define ASF_STREAM_TYPE_CONTROL           3
#define ASF_STREAM_TYPE_JFIF              4
#define ASF_STREAM_TYPE_DEGRADABLE_JPEG   5
#define ASF_STREAM_TYPE_FILE_TRANSFER     6
#define ASF_STREAM_TYPE_BINARY            7

#define ASF_MAX_NUM_STREAMS     23

#ifndef GUID_DEFINED
#define GUID_DEFINED

typedef struct _GUID {          /* size is 16 */
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t  Data4[8];
} GUID;

#endif /* !GUID_DEFINED */

static const struct
{
    const char* name;
    const GUID  guid;
} guids[] =
{
    { "error",
    { 0x0,} },


    /* base ASF objects */
    { "header",
    { 0x75b22630, 0x668e, 0x11cf, { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c }} },

    { "data",
    { 0x75b22636, 0x668e, 0x11cf, { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c }} },

    { "simple index",
    { 0x33000890, 0xe5b1, 0x11cf, { 0x89, 0xf4, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xcb }} },

    { "index",
    { 0xd6e229d3, 0x35da, 0x11d1, { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe }} },

    { "media object index",
    { 0xfeb103f8, 0x12ad, 0x4c64, { 0x84, 0x0f, 0x2a, 0x1d, 0x2f, 0x7a, 0xd4, 0x8c }} },

    { "timecode index",
    { 0x3cb73fd0, 0x0c4a, 0x4803, { 0x95, 0x3d, 0xed, 0xf7, 0xb6, 0x22, 0x8f, 0x0c }} },

    /* header ASF objects */
    { "file properties",
    { 0x8cabdca1, 0xa947, 0x11cf, { 0x8e, 0xe4, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 }} },

    { "stream header",
    { 0xb7dc0791, 0xa9b7, 0x11cf, { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 }} },

    { "header extension",
    { 0x5fbf03b5, 0xa92e, 0x11cf, { 0x8e, 0xe3, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 }} },

    { "codec list",
    { 0x86d15240, 0x311d, 0x11d0, { 0xa3, 0xa4, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 }} },

    { "script command",
    { 0x1efb1a30, 0x0b62, 0x11d0, { 0xa3, 0x9b, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 }} },

    { "marker",
    { 0xf487cd01, 0xa951, 0x11cf, { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 }} },

    { "bitrate mutual exclusion",
    { 0xd6e229dc, 0x35da, 0x11d1, { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe }} },

    { "error correction",
    { 0x75b22635, 0x668e, 0x11cf, { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c }} },

    { "content description",
    { 0x75b22633, 0x668e, 0x11cf, { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c }} },

    { "extended content description",
    { 0xd2d0a440, 0xe307, 0x11d2, { 0x97, 0xf0, 0x00, 0xa0, 0xc9, 0x5e, 0xa8, 0x50 }} },

    { "stream bitrate properties", /* (http://get.to/sdp) */
    { 0x7bf875ce, 0x468d, 0x11d1, { 0x8d, 0x82, 0x00, 0x60, 0x97, 0xc9, 0xa2, 0xb2 }} },

    { "extended content encryption",
    { 0x298ae614, 0x2622, 0x4c17, { 0xb9, 0x35, 0xda, 0xe0, 0x7e, 0xe9, 0x28, 0x9c }} },

    { "padding",
    { 0x1806d474, 0xcadf, 0x4509, { 0xa4, 0xba, 0x9a, 0xab, 0xcb, 0x96, 0xaa, 0xe8 }} },


    /* stream properties object stream type */
    { "audio media",
    { 0xf8699e40, 0x5b4d, 0x11cf, { 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b }} },

    { "video media",
    { 0xbc19efc0, 0x5b4d, 0x11cf, { 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b }} },

    { "command media",
    { 0x59dacfc0, 0x59e6, 0x11d0, { 0xa3, 0xac, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 }} },

    { "JFIF media (JPEG)",
    { 0xb61be100, 0x5b4e, 0x11cf, { 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b }} },

    { "Degradable JPEG media",
    { 0x35907de0, 0xe415, 0x11cf, { 0xa9, 0x17, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b }} },

    { "File Transfer media",
    { 0x91bd222c, 0xf21c, 0x497a, { 0x8b, 0x6d, 0x5a, 0xa8, 0x6b, 0xfc, 0x01, 0x85 }} },

    { "Binary media",
    { 0x3afb65e2, 0x47ef, 0x40f2, { 0xac, 0x2c, 0x70, 0xa9, 0x0d, 0x71, 0xd3, 0x43 }} },

    /* stream properties object error correction */
    { "no error correction",
    { 0x20fb5700, 0x5b55, 0x11cf, { 0xa8, 0xfd, 0x00, 0x80, 0x5f, 0x5c, 0x44, 0x2b }} },

    { "audio spread",
    { 0xbfc3cd50, 0x618f, 0x11cf, { 0x8b, 0xb2, 0x00, 0xaa, 0x00, 0xb4, 0xe2, 0x20 }} },


    /* mutual exclusion object exlusion type */
    { "mutex bitrate",
    { 0xd6e22a01, 0x35da, 0x11d1, { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe }} },

    { "mutex unknown", 
    { 0xd6e22a02, 0x35da, 0x11d1, { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe }} },


    /* header extension */
    { "reserved_1",
    { 0xabd3d211, 0xa9ba, 0x11cf, { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 }} },


    /* script command */
    { "reserved script command",
    { 0x4B1ACBE3, 0x100B, 0x11D0, { 0xA3, 0x9B, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6 }} },

    /* marker object */
    { "reserved marker",
    { 0x4CFEDB20, 0x75F6, 0x11CF, { 0x9C, 0x0F, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB }} },

    /* various */
    /* Already defined (reserved_1)
    { "head2",
    { 0xabd3d211, 0xa9ba, 0x11cf, { 0x8e, 0xe6, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 }} },
    */
    { "audio conceal none",
    { 0x49f1a440, 0x4ece, 0x11d0, { 0xa3, 0xac, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 }} },

    { "codec comment1 header",
    { 0x86d15241, 0x311d, 0x11d0, { 0xa3, 0xa4, 0x00, 0xa0, 0xc9, 0x03, 0x48, 0xf6 }} },

    { "asf 2.0 header",
    { 0xd6e229d1, 0x35da, 0x11d1, { 0x90, 0x34, 0x00, 0xa0, 0xc9, 0x03, 0x49, 0xbe }} },

};

#endif

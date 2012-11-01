#ifndef _RENDER_FLAGS_H_
#define _RENDER_FLAGS_H_

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define RENDER_FLAG_BOT         0x01
#define RENDER_FLAG_TOP         0x02
#define RENDER_FLAG_BOTH (RENDER_FLAG_BOT | RENDER_FLAG_TOP)
#define RENDER_FLAG_FIELDMASK   0x03

#define RENDER_FLAG_FIELD0      0x80
#define RENDER_FLAG_FIELD1      0x100
#define RENDER_FLAG_WEAVE       0x200

// #define RENDER_FLAG_LAST        0x40

#define RENDER_FLAG_NOOSD       0x04 /* don't draw any osd */
#define RENDER_FLAG_NOOSDALPHA  0x08 /* don't allow alpha when osd is drawn */

/* these two flags will be used if we need to render same image twice (bob deinterlacing) */
#define RENDER_FLAG_NOLOCK      0x10   /* don't attempt to lock texture before rendering */
#define RENDER_FLAG_NOUNLOCK    0x20   /* don't unlock texture after rendering */

/* this defines what color translation coefficients */
#define CONF_FLAGS_YUVCOEF_MASK(a) ((a) & 0x07)
#define CONF_FLAGS_YUVCOEF_BT709 0x01
#define CONF_FLAGS_YUVCOEF_BT601 0x02
#define CONF_FLAGS_YUVCOEF_240M  0x03
#define CONF_FLAGS_YUVCOEF_EBU   0x04

#define CONF_FLAGS_YUV_FULLRANGE 0x08
#define CONF_FLAGS_FULLSCREEN    0x10

/* defines color primaries */
#define CONF_FLAGS_COLPRI_MASK(a) ((a) & 0xe0)
#define CONF_FLAGS_COLPRI_BT709   0x20
#define CONF_FLAGS_COLPRI_BT470M  0x40
#define CONF_FLAGS_COLPRI_BT470BG 0x60
#define CONF_FLAGS_COLPRI_170M    0x80
#define CONF_FLAGS_COLPRI_240M    0xa0

/* defines chroma subsampling sample location */
#define CONF_FLAGS_CHROMA_MASK(a) ((a) & 0x0300)
#define CONF_FLAGS_CHROMA_LEFT    0x0100
#define CONF_FLAGS_CHROMA_CENTER  0x0200
#define CONF_FLAGS_CHROMA_TOPLEFT 0x0300

/* defines color transfer function */
#define CONF_FLAGS_TRC_MASK(a) ((a) & 0x0c00)
#define CONF_FLAGS_TRC_BT709      0x0400
#define CONF_FLAGS_TRC_GAMMA22    0x0800
#define CONF_FLAGS_TRC_GAMMA28    0x0c00

/* defines 3d modes */
#define CONF_FLAGS_FORMAT_SBS     0x001000
#define CONF_FLAGS_FORMAT_TB      0x002000

#endif

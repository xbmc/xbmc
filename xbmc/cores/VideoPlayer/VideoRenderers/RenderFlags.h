/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#define RENDER_FLAG_BOT         0x01
#define RENDER_FLAG_TOP         0x02
#define RENDER_FLAG_BOTH (RENDER_FLAG_BOT | RENDER_FLAG_TOP)
#define RENDER_FLAG_FIELDMASK   0x03

#define RENDER_FLAG_FIELD0      0x80
#define RENDER_FLAG_FIELD1      0x100

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
#define CONF_FLAGS_YUVCOEF_BT2020 0x05

#define CONF_FLAGS_YUV_FULLRANGE 0x08
#define CONF_FLAGS_FULLSCREEN    0x10

/* defines color primaries */
#define CONF_FLAGS_COLPRI_MASK(a) ((a) & 0xe0)
#define CONF_FLAGS_COLPRI_BT709   0x20  // sRGB, HDTV (ITU-R BT.709)
#define CONF_FLAGS_COLPRI_BT470M  0x40  // NTSC (1953) (FCC 1953, ITU-R BT.470 System M)
#define CONF_FLAGS_COLPRI_BT470BG 0x60  // PAL/SECAM (1970) (EBU Tech. 3213, ITU-R BT.470 System B, G)
#define CONF_FLAGS_COLPRI_170M    0x80  // NTSC (1987) (SMPTE RP 145 "SMPTE C", SMPTE 170M)
#define CONF_FLAGS_COLPRI_240M    0xa0  // SMPTE-240M
#define CONF_FLAGS_COLPRI_BT2020  0xc0  // UHDTV (ITU-R BT.2020)

/* defines chroma subsampling sample location */
#define CONF_FLAGS_CHROMA_MASK(a) ((a) & 0x0300)
#define CONF_FLAGS_CHROMA_LEFT    0x0100
#define CONF_FLAGS_CHROMA_CENTER  0x0200
#define CONF_FLAGS_CHROMA_TOPLEFT 0x0300

/* defines 3d modes */
#define CONF_FLAGS_STEREO_MODE_MASK(a) ((a) & 0x007000)
#define CONF_FLAGS_STEREO_MODE_SBS     0x001000
#define CONF_FLAGS_STEREO_MODE_TAB     0x002000

#define CONF_FLAGS_STEREO_CADENCE(a) ((a) & 0x008000)
#define CONF_FLAGS_STEREO_CADANCE_LEFT_RIGHT 0x000000
#define CONF_FLAGS_STEREO_CADANCE_RIGHT_LEFT 0x008000

unsigned int GetFlagsColorMatrix(unsigned int color_matrix, unsigned width, unsigned height);
unsigned int GetFlagsChromaPosition(unsigned int chroma_position);
unsigned int GetFlagsColorPrimaries(unsigned int color_primaries);
unsigned int GetFlagsStereoMode(const std::string& mode);

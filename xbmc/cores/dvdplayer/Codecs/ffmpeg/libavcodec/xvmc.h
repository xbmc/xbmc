/*
 * Copyright (C) 2003 Ivan Kalvachev
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_XVMC_H
#define AVCODEC_XVMC_H

#include <X11/extensions/XvMC.h>

#include "avcodec.h"

#define AV_XVMC_STATE_DISPLAY_PENDING          1  /**  the surface should be shown, the video driver manipulates this */
#define AV_XVMC_STATE_PREDICTION               2  /**  the surface is needed for prediction, the codec manipulates this */
#define AV_XVMC_STATE_OSD_SOURCE               4  /**  this surface is needed for subpicture rendering */
#define AV_XVMC_RENDER_MAGIC          0x1DC711C0  /**< magic value to ensure that regular pixel routines haven't corrupted the struct */
                                                  //   1337 IDCT MCo

#if LIBAVCODEC_VERSION_MAJOR < 53
#define MP_XVMC_STATE_DISPLAY_PENDING AV_XVMC_STATE_DISPLAY_PENDING
#define MP_XVMC_STATE_PREDICTION      AV_XVMC_STATE_PREDICTION
#define MP_XVMC_STATE_OSD_SOURCE      AV_XVMC_STATE_OSD_SOURCE
#define MP_XVMC_RENDER_MAGIC          AV_XVMC_RENDER_MAGIC
#endif

struct xvmc_render_state {
/** set by calling application */
//@{
    int             magic;                        ///< used as check for memory corruption by regular pixel routines

    short*          data_blocks;
    XvMCMacroBlock* mv_blocks;
    int             total_number_of_mv_blocks;
    int             total_number_of_data_blocks;
    int             mc_type;                      ///< XVMC_MPEG1/2/4,XVMC_H263 without XVMC_IDCT
    int             idct;                         ///< indicate that IDCT acceleration level is used
    int             chroma_format;                ///< XVMC_CHROMA_FORMAT_420/422/444
    int             unsigned_intra;               ///< +-128 for intra pictures after clipping
    XvMCSurface*    p_surface;                    ///< pointer to rendered surface, never changed
//}@

/** set by the decoder
    used by the XvMCRenderSurface function */
//@{
    XvMCSurface*    p_past_surface;               ///< pointer to the past surface
    XvMCSurface*    p_future_surface;             ///< pointer to the future prediction surface

    unsigned int    picture_structure;            ///< top/bottom fields or frame!
    unsigned int    flags;                        ///< XVMC_SECOND_FIELD - 1st or 2nd field in the sequence
    unsigned int    display_flags;                ///< 1,2 or 1+2 fields for XvMCPutSurface
//}@

/** modified by calling application and the decoder */
//@{
    int             state;                        ///< 0 - free, 1 - waiting to display, 2 - waiting for prediction
    int             start_mv_blocks_num;          ///< offset in the array for the current slice, updated by vo
    int             filled_mv_blocks_num;         ///< processed mv block in this slice, changed by decoder

    int             next_free_data_block_num;     ///< used in add_mv_block, pointer to next free block
//}@
/** extensions */
//@{
    void*           p_osd_target_surface_render;  ///< pointer to the surface where subpicture is rendered
//}@

};

#endif /* AVCODEC_XVMC_H */

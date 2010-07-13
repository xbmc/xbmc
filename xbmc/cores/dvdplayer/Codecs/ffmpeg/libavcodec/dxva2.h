/*
 * DXVA2 HW acceleration
 *
 * copyright (c) 2009 Laurent Aimar
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

#ifndef AVCODEC_DXVA_H
#define AVCODEC_DXVA_H

#include <stdint.h>

#include <dxva2api.h>

/**
 * This structure is used to provides the necessary configurations and data
 * to the DXVA2 FFmpeg HWAccel implementation.
 *
 * The application must make it available as AVCodecContext.hwaccel_context.
 */
 
 enum DxvaDecoderType {
	DECODER_TYPE_DXVA_1,   ///< IAMVideoAccelerator
	DECODER_TYPE_DXVA_2    ///< IDirectXVideoDecoder
};
#if 0
struct dxva_context {
    /**
     * DXVA2 decoder object
     */
    IDirectXVideoDecoder *decoder;

    /**
     * DXVA2 configuration used to create the decoder
     */
    const DXVA2_ConfigPictureDecode *cfg;

    /**
     * The number of surface in the surface array
     */
    unsigned surface_count;

    /**
     * The array of Direct3D surfaces used to create the decoder
     */
    LPDIRECT3DSURFACE9 *surface;

    /**
     * A bit field configuring the workarounds needed for using the decoder
     */
    uint64_t workaround;

    /**
     * Private to the FFmpeg AVHWAccel implementation
     */
    unsigned report_id;
};
#endif
struct dxva_context {
    /**
     * for dxva2 decoder.dxvadecoder should be a IDirectXVideoDecoder
     */
     
	 struct dxva_decoder_context *decoder;

    /**
     * DXVA 1 and 2 configuration is actually the same structure on another name
     */
    
    const DXVA2_ConfigPictureDecode *cfg;
    /**
     * The amount of the buffer used for the decoding process
     */
    unsigned surface_count;

    /**
     * The array of Direct3D surfaces used to create the decoder
     */
    LPDIRECT3DSURFACE9 *surface;

    /**
     * A bit field configuring the workarounds needed for using the decoder
     */
    uint64_t workaround;

    /**
     * Private to the FFmpeg AVHWAccel implementation
     */
    unsigned report_id;
};

typedef struct dxva_decoder_context {
  
	/*DXVA_1_DECODER or DXVA_2_DECODER*/
	enum DxvaDecoderType type;
    void *dxvadecoder;
    int (*dxva2_decoder_begin_frame)(struct dxva_context *ctx, unsigned index);
	int (*dxva2_decoder_end_frame)(struct dxva_context *ctx, unsigned index);
	int (*dxva2_decoder_execute)(struct dxva_context *ctx, DXVA2_DecodeExecuteParams *exec);
	int (*dxva2_decoder_get_buffer)(struct dxva_context *ctx, unsigned type, void *dxva_data, unsigned dxva_size);
	int (*dxva2_decoder_release_buffer)(struct dxva_context *ctx, unsigned type);
} dxva_decoder_context;
/*int (*AddExecuteBuffer)(dxva_directshow_context*, unsigned type, unsigned size, const void *data, unsigned *real_size);*/

#endif /* AVCODEC_DXVA_H */

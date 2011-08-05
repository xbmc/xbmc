/*
 * DXVA2 HW acceleration.
 *
 * copyright (c) 2010 Laurent Aimar
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

#include "dxva2_internal.h"
/*this is the order of the sequence for rendering the image*/
/*
AVCodecContext->get_buffer
start_frame
decode_slice
end_frame
AVCodecContext->release_buffer
*/

static int dxva2_default_begin_frame(struct dxva_context *ctx,
                                     unsigned index)
{	
	if (FAILED(IDirectXVideoDecoder_BeginFrame((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, ctx->surface[index],NULL)))
		return -1;
	return 0;
}



static int dxva2_default_get_buffer(struct dxva_context *ctx, unsigned type, void **dxva_data, unsigned *dxva_size)
{
    void *data;
	unsigned size;
	if (FAILED(IDirectXVideoDecoder_GetBuffer((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, type, &data, &size))) {
        return -1;
    }
	*dxva_data = data;
	*dxva_size = size;
	return 0;
}

static int dxva2_default_execute(struct dxva_context *ctx)
{
    unsigned i;
    for (i=0; i<ctx->exec.NumCompBuffers; i++)
	{
        if (FAILED(IDirectXVideoDecoder_ReleaseBuffer((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, ctx->exec.pCompressedBuffers[i].CompressedBufferType))) {
            av_log(ctx, AV_LOG_ERROR, "Failed to IDirectXVideoDecoder_ReleaseBuffer\n");
        }
	}
	return 0;
    if (FAILED(IDirectXVideoDecoder_Execute((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, &ctx->exec))) {
        av_log(ctx, AV_LOG_ERROR, "Failed to execute\n");
        return -1;
    }
	ctx->exec.NumCompBuffers = 0;
	return 0;
}

static int dxva2_default_end_frame(struct dxva_context *ctx, unsigned index)
{
	if (FAILED(IDirectXVideoDecoder_EndFrame((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, NULL)))
		return -1;
	return 0;
}

static int dxva2_default_release_buffer(struct dxva_context *ctx, unsigned type)
{
	if (FAILED(IDirectXVideoDecoder_ReleaseBuffer((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, type))) {
        return -1;
    }
	return 0;
}

dxva_decoder_context idirectxvideo_decoder = {
	.type                         = DECODER_TYPE_DXVA_2,
	.dxva2_decoder_begin_frame    = dxva2_default_begin_frame,
	.dxva2_decoder_end_frame      = dxva2_default_end_frame,
    .dxva2_decoder_execute        = dxva2_default_execute,
	.dxva2_decoder_get_buffer     = dxva2_default_get_buffer,
	.dxva2_decoder_release_buffer = dxva2_default_release_buffer,
};

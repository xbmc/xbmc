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

static unsigned convert_dxva2type(unsigned type)
{
    if (type <= DXVA2_BitStreamDateBufferType)
		return type + 1;
	else if (type == DXVA2_MotionVectorBuffer)
		return 16;//DXVA_MOTION_VECTOR_BUFFER
    else if (type == DXVA2_FilmGrainBuffer)
		return 17;//DXVA_FILM_GRAIN_BUFFER
	else
	{
		assert(0);
		return 0;//DXVA_COMPBUFFER_TYPE_THAT_IS_NOT_USED
	}
}

void *ff_dxva2_get_surface(const Picture *picture)
{
    return picture->data[3];
}

unsigned ff_dxva2_get_surface_index(const struct dxva_context *ctx,
                                    const Picture *picture)
{
    void *surface = ff_dxva2_get_surface(picture);
    unsigned i;

    for (i = 0; i < ctx->surface_count; i++)
        if (ctx->surface[i] == surface)
            return i;

    assert(0);
    return 0;
}

int ff_dxva2_commit_buffer(AVCodecContext *avctx,
                           struct dxva_context *ctx,
                           DXVA2_DecodeBufferDesc *dsc,
                           unsigned type, const void *data, unsigned size,
                           unsigned mb_count)
{
    void     *dxva_data;
    unsigned dxva_size;
    int      result;
    
    if (ctx->decoder->dxva2_decoder_get_buffer(ctx, type, &dxva_data, &dxva_size)<0) {
	    
        return -1;
    }
    if (size <= dxva_size) {
        memcpy(dxva_data, data, size);

        memset(dsc, 0, sizeof(*dsc));
        dsc->CompressedBufferType = type;
        dsc->DataSize             = size;
        dsc->NumMBsInBuffer       = mb_count;

        result = 0;
    } else {
        av_log(avctx, AV_LOG_ERROR, "Buffer for type %d was too small\n", type);
        result = -1;
    }
	
    if (ctx->decoder->dxva2_decoder_release_buffer(ctx, type)<0) {
        result = -1;
    }
    return result;
}
/*dxva2 decoder function*/

int ff_dxva2_common_end_frame(AVCodecContext *avctx, MpegEncContext *s,
                              const void *pp, unsigned pp_size,
                              const void *qm, unsigned qm_size,
                              int (*commit_bs_si)(AVCodecContext *,
                                                  DXVA2_DecodeBufferDesc *bs,
                                                  DXVA2_DecodeBufferDesc *slice))
{
    struct dxva_context *ctx = avctx->hwaccel_context;
	unsigned               buffer_count = 0;
    DXVA2_DecodeBufferDesc buffer[4];
    DXVA2_DecodeExecuteParams exec;
    int      result, surfaceindex;
	const Picture *current_picture = s->current_picture_ptr;
	
    surfaceindex = ff_dxva2_get_surface_index(ctx, current_picture);
	if (ctx->decoder->dxva2_decoder_begin_frame(ctx ,surfaceindex) < 0){
		return -1;
	}

    result = ff_dxva2_commit_buffer(avctx, ctx, &buffer[buffer_count],
                                    DXVA2_PictureParametersBufferType,
                                    pp, pp_size, 0);
    if (result) {
        av_log(avctx, AV_LOG_ERROR,
               "Failed to add picture parameter buffer\n");
        goto end;
    }
    buffer_count++;

    if (qm_size > 0) {
        result = ff_dxva2_commit_buffer(avctx, ctx, &buffer[buffer_count],
                                        DXVA2_InverseQuantizationMatrixBufferType,
                                        qm, qm_size, 0);
        if (result) {
            av_log(avctx, AV_LOG_ERROR,
                   "Failed to add inverse quantization matrix buffer\n");
            goto end;
        }
        buffer_count++;
    }

    result = commit_bs_si(avctx,
                          &buffer[buffer_count + 0],
                          &buffer[buffer_count + 1]);
    if (result) {
        av_log(avctx, AV_LOG_ERROR,
               "Failed to add bitstream or slice control buffer\n");
        goto end;
    }
    buffer_count += 2;

    /* TODO Film Grain when possible */

    assert(buffer_count == 1 + (qm_size > 0) + 2);

    memset(&exec, 0, sizeof(exec));
    exec.NumCompBuffers      = buffer_count;
    exec.pCompressedBuffers  = buffer;
    exec.pExtensionData      = NULL;
	
    if (ctx->decoder->dxva2_decoder_execute(ctx ,&exec)<0) {
        result = -1;
    }

end:
    if (ctx->decoder->dxva2_decoder_end_frame(ctx ,surfaceindex)<0){
        av_log(avctx, AV_LOG_ERROR, "Failed to end frame\n");
        result = -1;
    }

    if (!result)
        ff_draw_horiz_band(s, 0, s->avctx->height);
    return result;
}

static int dxva2_default_begin_frame(struct dxva_context *ctx,
                                     unsigned index)
{
	if (FAILED(IDirectXVideoDecoder_BeginFrame((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, index,NULL)))
		return -1;
	return 0;
}

static int dxva2_default_end_frame(struct dxva_context *ctx, unsigned index)
{
	if (FAILED(IDirectXVideoDecoder_EndFrame((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, NULL)))
		return -1;
	return 0;
}

static int dxva2_default_execute(struct dxva_context *ctx, DXVA2_DecodeExecuteParams *exec)
{
    if (FAILED(IDirectXVideoDecoder_Execute((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, &exec))) {
        //av_log(avctx, AV_LOG_ERROR, "Failed to execute\n");
        return -1;
    }
	return 0;
}

static int dxva2_default_get_buffer(struct dxva_context *ctx, unsigned type, void *dxva_data, unsigned dxva_size)
{

	if (FAILED(IDirectXVideoDecoder_GetBuffer((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, type, &dxva_data, &dxva_size))) {
        return -1;
    }
	return 0;
}

static int dxva2_default_release_buffer(struct dxva_context *ctx, unsigned type)
{
	if (FAILED(IDirectXVideoDecoder_ReleaseBuffer((IDirectXVideoDecoder *)ctx->decoder->dxvadecoder, type))) {
        //av_log(avctx, AV_LOG_ERROR, "Failed to release buffer type %d\n", type);
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
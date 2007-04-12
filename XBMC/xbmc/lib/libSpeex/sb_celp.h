/* Copyright (C) 2002 Jean-Marc Valin */
/**
   @file sb_celp.h
   @brief Sub-band CELP mode used for wideband encoding
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef SB_CELP_H
#define SB_CELP_H

#include "modes.h"
#include "speex_bits.h"
#include "nb_celp.h"

/**Structure representing the full state of the sub-band encoder*/
typedef struct SBEncState {
   const SpeexMode *mode;            /**< Pointer to the mode (containing for vtable info) */
   void *st_low;               /**< State of the low-band (narrowband) encoder */
   int    full_frame_size;     /**< Length of full-band frames*/
   int    frame_size;          /**< Length of high-band frames*/
   int    subframeSize;        /**< Length of high-band sub-frames*/
   int    nbSubframes;         /**< Number of high-band sub-frames*/
   int    windowSize;          /**< Length of high-band LPC window*/
   int    lpcSize;             /**< Order of high-band LPC analysis */
   int    bufSize;             /**< Buffer size */
   int    first;               /**< First frame? */
   float  lag_factor;          /**< Lag-windowing control parameter */
   float  lpc_floor;           /**< Controls LPC analysis noise floor */
   spx_word16_t  gamma1;              /**< Perceptual weighting coef 1 */
   spx_word16_t  gamma2;              /**< Perceptual weighting coef 2 */

   char  *stack;               /**< Temporary allocation stack */
   spx_sig_t *x0d, *x1d; /**< QMF filter signals*/
   spx_sig_t *high;                /**< High-band signal (buffer) */
   spx_sig_t *y0, *y1;             /**< QMF synthesis signals */
   spx_word16_t *h0_mem, *h1_mem;
   spx_word32_t *g0_mem, *g1_mem; /**< QMF memories */

   spx_sig_t *excBuf;              /**< High-band excitation */
   spx_sig_t *exc;                 /**< High-band excitation (for QMF only)*/
   spx_sig_t *buf;                 /**< Temporary buffer */
   spx_sig_t *res;                 /**< Zero-input response (ringing) */
   spx_sig_t *sw;                  /**< Perceptually weighted signal */
   spx_sig_t *target;              /**< Weighted target signal (analysis by synthesis) */
   spx_word16_t *window;              /**< LPC analysis window */
   spx_word16_t *lagWindow;           /**< Auto-correlation window */
   spx_word16_t *autocorr;            /**< Auto-correlation (for LPC analysis) */
   spx_coef_t *lpc;                 /**< LPC coefficients */
   spx_lsp_t *lsp;                 /**< LSP coefficients */
   spx_lsp_t *qlsp;                /**< Quantized LSPs */
   spx_lsp_t *old_lsp;             /**< LSPs of previous frame */
   spx_lsp_t *old_qlsp;            /**< Quantized LSPs of previous frame */
   spx_lsp_t *interp_lsp;          /**< Interpolated LSPs for current sub-frame */
   spx_lsp_t *interp_qlsp;         /**< Interpolated quantized LSPs for current sub-frame */
   spx_coef_t *interp_lpc;          /**< Interpolated LPCs for current sub-frame */
   spx_coef_t *interp_qlpc;         /**< Interpolated quantized LPCs for current sub-frame */
   spx_coef_t *bw_lpc1;             /**< Bandwidth-expanded version of LPCs (#1) */
   spx_coef_t *bw_lpc2;             /**< Bandwidth-expanded version of LPCs (#2) */

   spx_mem_t *mem_sp;              /**< Synthesis signal memory */
   spx_mem_t *mem_sp2;
   spx_mem_t *mem_sw;              /**< Perceptual signal memory */
   spx_word32_t *pi_gain;

   float  vbr_quality;         /**< Quality setting for VBR encoding */
   int    vbr_enabled;         /**< 1 for enabling VBR, 0 otherwise */
   int    abr_enabled;         /**< ABR setting (in bps), 0 if off */
   float  abr_drift;
   float  abr_drift2;
   float  abr_count;
   int    vad_enabled;         /**< 1 for enabling VAD, 0 otherwise */
   float  relative_quality;

   int    encode_submode;
   const SpeexSubmode * const *submodes;
   int    submodeID;
   int    submodeSelect;
   int    complexity;
   int    sampling_rate;

} SBEncState;


/**Structure representing the full state of the sub-band decoder*/
typedef struct SBDecState {
   const SpeexMode *mode;            /**< Pointer to the mode (containing for vtable info) */
   void *st_low;               /**< State of the low-band (narrowband) encoder */
   int    full_frame_size;
   int    frame_size;
   int    subframeSize;
   int    nbSubframes;
   int    lpcSize;
   int    first;
   int    sampling_rate;
   int    lpc_enh_enabled;

   char  *stack;
   spx_sig_t *x0d, *x1d;
   spx_sig_t *high;
   spx_sig_t *y0, *y1;
   spx_word32_t *g0_mem, *g1_mem;

   spx_sig_t *exc;
   spx_lsp_t *qlsp;
   spx_lsp_t *old_qlsp;
   spx_lsp_t *interp_qlsp;
   spx_coef_t *interp_qlpc;

   spx_mem_t *mem_sp;
   spx_word32_t *pi_gain;

   int    encode_submode;
   const SpeexSubmode * const *submodes;
   int    submodeID;
} SBDecState;


/**Initializes encoder state*/
void *sb_encoder_init(const SpeexMode *m);

/**De-allocates encoder state resources*/
void sb_encoder_destroy(void *state);

/**Encodes one frame*/
int sb_encode(void *state, short *in, SpeexBits *bits);


/**Initializes decoder state*/
void *sb_decoder_init(const SpeexMode *m);

/**De-allocates decoder state resources*/
void sb_decoder_destroy(void *state);

/**Decodes one frame*/
int sb_decode(void *state, SpeexBits *bits, short *out);

int sb_encoder_ctl(void *state, int request, void *ptr);

int sb_decoder_ctl(void *state, int request, void *ptr);

#endif

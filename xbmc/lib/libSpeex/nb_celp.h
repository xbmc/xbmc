/* Copyright (C) 2002 Jean-Marc Valin */
/**
    @file nb_celp.h
    @brief Narrowband CELP encoder/decoder
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

#ifndef NB_CELP_H
#define NB_CELP_H

#include "modes.h"
#include "speex_bits.h"
#include "speex_callbacks.h"
#include "vbr.h"
#include "filters.h"

/**Structure representing the full state of the narrowband encoder*/
typedef struct EncState {
   const SpeexMode *mode;       /**< Mode corresponding to the state */
   int    first;          /**< Is this the first frame? */
   int    frameSize;      /**< Size of frames */
   int    subframeSize;   /**< Size of sub-frames */
   int    nbSubframes;    /**< Number of sub-frames */
   int    windowSize;     /**< Analysis (LPC) window length */
   int    lpcSize;        /**< LPC order */
   int    bufSize;        /**< Buffer size */
   int    min_pitch;      /**< Minimum pitch value allowed */
   int    max_pitch;      /**< Maximum pitch value allowed */

   int    safe_pitch;     /**< Don't use too large values for pitch (in case we lose a packet) */
   int    bounded_pitch;  /**< Next frame should not rely on previous frames for pitch */
   int    ol_pitch;       /**< Open-loop pitch */
   int    ol_voiced;      /**< Open-loop voiced/non-voiced decision */
   int   *pitch;

#ifdef EPIC_48K
   int    lbr_48k;
#endif

   spx_word16_t  gamma1;         /**< Perceptual filter: A(z/gamma1) */
   spx_word16_t  gamma2;         /**< Perceptual filter: A(z/gamma2) */
   float  lag_factor;     /**< Lag windowing Gaussian width */
   float  lpc_floor;      /**< Noise floor multiplier for A[0] in LPC analysis*/
   char  *stack;          /**< Pseudo-stack allocation for temporary memory */
   spx_sig_t *inBuf;          /**< Input buffer (original signal) */
   spx_sig_t *frame;          /**< Start of original frame */
   spx_sig_t *excBuf;         /**< Excitation buffer */
   spx_sig_t *exc;            /**< Start of excitation frame */
   spx_sig_t *exc2Buf;        /**< "Pitch enhanced" excitation */
   spx_sig_t *exc2;           /**< "Pitch enhanced" excitation */
   spx_sig_t *swBuf;          /**< Weighted signal buffer */
   spx_sig_t *sw;             /**< Start of weighted signal frame */
   spx_sig_t *innov;          /**< Innovation for the frame */
   spx_word16_t *window;         /**< Temporary (Hanning) window */
   spx_sig_t *buf2;           /**< 2nd temporary buffer */
   spx_word16_t *autocorr;       /**< auto-correlation */
   spx_word16_t *lagWindow;      /**< Window applied to auto-correlation */
   spx_coef_t *lpc;            /**< LPCs for current frame */
   spx_lsp_t *lsp;            /**< LSPs for current frame */
   spx_lsp_t *qlsp;           /**< Quantized LSPs for current frame */
   spx_lsp_t *old_lsp;        /**< LSPs for previous frame */
   spx_lsp_t *old_qlsp;       /**< Quantized LSPs for previous frame */
   spx_lsp_t *interp_lsp;     /**< Interpolated LSPs */
   spx_lsp_t *interp_qlsp;    /**< Interpolated quantized LSPs */
   spx_coef_t *interp_lpc;     /**< Interpolated LPCs */
   spx_coef_t *interp_qlpc;    /**< Interpolated quantized LPCs */
   spx_coef_t *bw_lpc1;        /**< LPCs after bandwidth expansion by gamma1 for perceptual weighting*/
   spx_coef_t *bw_lpc2;        /**< LPCs after bandwidth expansion by gamma2 for perceptual weighting*/
   spx_mem_t *mem_sp;         /**< Filter memory for signal synthesis */
   spx_mem_t *mem_sw;         /**< Filter memory for perceptually-weighted signal */
   spx_mem_t *mem_sw_whole;   /**< Filter memory for perceptually-weighted signal (whole frame)*/
   spx_mem_t *mem_exc;        /**< Filter memory for excitation (whole frame) */
   spx_word32_t *pi_gain;        /**< Gain of LPC filter at theta=pi (fe/2) */

   VBRState *vbr;         /**< State of the VBR data */
   float  vbr_quality;    /**< Quality setting for VBR encoding */
   float  relative_quality; /**< Relative quality that will be needed by VBR */
   int    vbr_enabled;    /**< 1 for enabling VBR, 0 otherwise */
   int    vad_enabled;    /**< 1 for enabling VAD, 0 otherwise */
   int    dtx_enabled;    /**< 1 for enabling DTX, 0 otherwise */
   int    dtx_count;      /**< Number of consecutive DTX frames */
   int    abr_enabled;    /**< ABR setting (in bps), 0 if off */
   float  abr_drift;
   float  abr_drift2;
   float  abr_count;
   int    complexity;     /**< Complexity setting (0-10 from least complex to most complex) */
   int    sampling_rate;

   int    encode_submode;
   const SpeexSubmode * const *submodes; /**< Sub-mode data */
   int    submodeID;      /**< Activated sub-mode */
   int    submodeSelect;  /**< Mode chosen by the user (may differ from submodeID if VAD is on) */
} EncState;

/**Structure representing the full state of the narrowband decoder*/
typedef struct DecState {
   const SpeexMode *mode;       /**< Mode corresponding to the state */
   int    first;          /**< Is this the first frame? */
   int    count_lost;     /**< Was the last frame lost? */
   int    frameSize;      /**< Size of frames */
   int    subframeSize;   /**< Size of sub-frames */
   int    nbSubframes;    /**< Number of sub-frames */
   int    windowSize;     /**< Analysis (LPC) window length */
   int    lpcSize;        /**< LPC order */
   int    bufSize;        /**< Buffer size */
   int    min_pitch;      /**< Minimum pitch value allowed */
   int    max_pitch;      /**< Maximum pitch value allowed */
   int    sampling_rate;

#ifdef EPIC_48K
   int    lbr_48k;
#endif

   spx_word16_t  last_ol_gain;   /**< Open-loop gain for previous frame */

   char  *stack;          /**< Pseudo-stack allocation for temporary memory */
   spx_sig_t *inBuf;          /**< Input buffer (original signal) */
   spx_sig_t *frame;          /**< Start of original frame */
   spx_sig_t *excBuf;         /**< Excitation buffer */
   spx_sig_t *exc;            /**< Start of excitation frame */
   spx_sig_t *innov;          /**< Innovation for the frame */
   spx_lsp_t *qlsp;           /**< Quantized LSPs for current frame */
   spx_lsp_t *old_qlsp;       /**< Quantized LSPs for previous frame */
   spx_lsp_t *interp_qlsp;    /**< Interpolated quantized LSPs */
   spx_coef_t *interp_qlpc;    /**< Interpolated quantized LPCs */
   spx_mem_t *mem_sp;         /**< Filter memory for synthesis signal */
   spx_word32_t *pi_gain;        /**< Gain of LPC filter at theta=pi (fe/2) */
   int    last_pitch;     /**< Pitch of last correctly decoded frame */
   spx_word16_t  last_pitch_gain; /**< Pitch gain of last correctly decoded frame */
   spx_word16_t  pitch_gain_buf[3];  /**< Pitch gain of last decoded frames */
   int    pitch_gain_buf_idx; /**< Tail of the buffer */

   int    encode_submode;
   const SpeexSubmode * const *submodes; /**< Sub-mode data */
   int    submodeID;      /**< Activated sub-mode */
   int    lpc_enh_enabled; /**< 1 when LPC enhancer is on, 0 otherwise */
   CombFilterMem *comb_mem;
   SpeexCallback speex_callbacks[SPEEX_MAX_CALLBACKS];

   SpeexCallback user_callback;

   /*Vocoder data*/
   float  voc_m1;
   float  voc_m2;
   float  voc_mean;
   int    voc_offset;

   int    dtx_enabled;
} DecState;

/** Initializes encoder state*/
void *nb_encoder_init(const SpeexMode *m);

/** De-allocates encoder state resources*/
void nb_encoder_destroy(void *state);

/** Encodes one frame*/
int nb_encode(void *state, short *in, SpeexBits *bits);


/** Initializes decoder state*/
void *nb_decoder_init(const SpeexMode *m);

/** De-allocates decoder state resources*/
void nb_decoder_destroy(void *state);

/** Decodes one frame*/
int nb_decode(void *state, SpeexBits *bits, short *out);

/** ioctl-like function for controlling a narrowband encoder */
int nb_encoder_ctl(void *state, int request, void *ptr);

/** ioctl-like function for controlling a narrowband decoder */
int nb_decoder_ctl(void *state, int request, void *ptr);


#endif

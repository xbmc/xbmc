/* Copyright (C) 2003 Epic Games 
   Written by Jean-Marc Valin

   File: speex_preprocess.h


   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/


#ifdef __cplusplus
extern "C" {
#endif

struct drft_lookup;

typedef struct SpeexPreprocessState {
   int    frame_size;        /**< Number of samples processed each time */
   int    ps_size;           /**< Number of points in the power spectrum */
   int    sampling_rate;     /**< Sampling rate of the input/output */
   
   /* parameters */
   int    denoise_enabled;
   int    agc_enabled;
   float  agc_level;
   int    vad_enabled;

   float *frame;             /**< Processing frame (2*ps_size) */
   float *ps;                /**< Current power spectrum */
   float *gain2;             /**< Adjusted gains */
   float *window;            /**< Analysis/Synthesis window */
   float *noise;             /**< Noise estimate */
   float *old_ps;            /**< Power spectrum for last frame */
   float *gain;              /**< Ephraim Malah gain */
   float *prior;             /**< A-priori SNR */
   float *post;              /**< A-posteriori SNR */

   float *S;                 /**< Smoothed power spectrum */
   float *Smin;              /**< See Cohen paper */
   float *Stmp;              /**< See Cohen paper */
   float *update_prob;       /**< Propability of speech presence for noise update */

   float *zeta;              /**< Smoothed a priori SNR */
   float  Zpeak;
   float  Zlast;

   float *loudness_weight;   /**< Perceptual loudness curve */

   float *echo_noise;

   float *noise_bands;
   float *noise_bands2;
   int    noise_bandsN;
   float *speech_bands;
   float *speech_bands2;
   int    speech_bandsN;

   float *inbuf;             /**< Input buffer (overlapped analysis) */
   float *outbuf;            /**< Output buffer (for overlap and add) */

   float  speech_prob;
   int    last_speech;
   float  loudness;          /**< loudness estimate */
   float  loudness2;         /**< loudness estimate */
   int    nb_adapt;          /**< Number of frames used for adaptation so far */
   int    nb_loudness_adapt; /**< Number of frames used for loudness adaptation so far */
   int    consec_noise;      /**< Number of consecutive noise frames */
   int    nb_preprocess;     /**< Number of frames processed so far */
   struct drft_lookup *fft_lookup;   /**< Lookup table for the FFT */

} SpeexPreprocessState;

/** Creates a new preprocessing state */
SpeexPreprocessState *speex_preprocess_state_init(int frame_size, int sampling_rate);

/** Destroys a denoising state */
void speex_preprocess_state_destroy(SpeexPreprocessState *st);

/** Preprocess a frame */
int speex_preprocess(SpeexPreprocessState *st, short *x, float *noise);

/** Preprocess a frame */
void speex_preprocess_estimate_update(SpeexPreprocessState *st, short *x, float *noise);

/** Used like the ioctl function to control the preprocessor parameters */
int speex_preprocess_ctl(SpeexPreprocessState *st, int request, void *ptr);



#define SPEEX_PREPROCESS_SET_DENOISE 0
#define SPEEX_PREPROCESS_GET_DENOISE 1

#define SPEEX_PREPROCESS_SET_AGC 2
#define SPEEX_PREPROCESS_GET_AGC 3

#define SPEEX_PREPROCESS_SET_VAD 4
#define SPEEX_PREPROCESS_GET_VAD 5

#define SPEEX_PREPROCESS_SET_AGC_LEVEL 6
#define SPEEX_PREPROCESS_GET_AGC_LEVEL 7

#ifdef __cplusplus
}
#endif

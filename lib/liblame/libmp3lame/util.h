/*
 *      lame utility library include file
 *
 *      Copyright (c) 1999 Albert L Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_UTIL_H
#define LAME_UTIL_H

#include "l3side.h"
#include "id3tag.h"

#ifdef __cplusplus
extern  "C" {
#endif

/***********************************************************************
*
*  Global Definitions
*
***********************************************************************/

#ifndef FALSE
#define         FALSE                   0
#endif

#ifndef TRUE
#define         TRUE                    (!FALSE)
#endif

#ifdef UINT_MAX
# define         MAX_U_32_NUM            UINT_MAX
#else
# define         MAX_U_32_NUM            0xFFFFFFFF
#endif

#ifndef PI
# ifdef M_PI
#  define       PI                      M_PI
# else
#  define       PI                      3.14159265358979323846
# endif
#endif


#ifdef M_LN2
# define        LOG2                    M_LN2
#else
# define        LOG2                    0.69314718055994530942
#endif

#ifdef M_LN10
# define        LOG10                   M_LN10
#else
# define        LOG10                   2.30258509299404568402
#endif


#ifdef M_SQRT2
# define        SQRT2                   M_SQRT2
#else
# define        SQRT2                   1.41421356237309504880
#endif


#define         HAN_SIZE                512
#define         CRC16_POLYNOMIAL        0x8005
    
#define MAX_BITS_PER_CHANNEL 4095
#define MAX_BITS_PER_GRANULE 7680

/* "bit_stream.h" Definitions */
#define         BUFFER_SIZE     LAME_MAXMP3BUFFER

#define         Min(A, B)       ((A) < (B) ? (A) : (B))
#define         Max(A, B)       ((A) > (B) ? (A) : (B))

/* log/log10 approximations */
#ifdef USE_FAST_LOG
#define         FAST_LOG10(x)       (fast_log2(x)*(LOG2/LOG10))
#define         FAST_LOG(x)         (fast_log2(x)*LOG2)
#define         FAST_LOG10_X(x,y)   (fast_log2(x)*(LOG2/LOG10*(y)))
#define         FAST_LOG_X(x,y)     (fast_log2(x)*(LOG2*(y)))
#else
#define         FAST_LOG10(x)       log10(x)
#define         FAST_LOG(x)         log(x)
#define         FAST_LOG10_X(x,y)   (log10(x)*(y))
#define         FAST_LOG_X(x,y)     (log(x)*(y))
#endif


    struct replaygain_data;
#ifndef replaygain_data_defined
#define replaygain_data_defined
    typedef struct replaygain_data replaygain_t;
#endif
    struct plotting_data;
#ifndef plotting_data_defined
#define plotting_data_defined
    typedef struct plotting_data plotting_data;
#endif

/***********************************************************************
*
*  Global Type Definitions
*
***********************************************************************/

    typedef struct {
        void   *aligned;     /* pointer to ie. 128 bit aligned memory */
        void   *pointer;     /* to use with malloc/free */
    } aligned_pointer_t;

    void    malloc_aligned(aligned_pointer_t * ptr, unsigned int size, unsigned int bytes);
    void    free_aligned(aligned_pointer_t * ptr);



    typedef void (*iteration_loop_t) (lame_global_flags const * gfp,
                                      FLOAT pe[2][2], FLOAT ms_ratio[2],
                                      III_psy_ratio ratio[2][2]);


    /* "bit_stream.h" Type Definitions */

    typedef struct bit_stream_struc {
        unsigned char *buf;  /* bit stream buffer */
        int     buf_size;    /* size of buffer (in number of bytes) */
        int     totbit;      /* bit counter of bit stream */
        int     buf_byte_idx; /* pointer to top byte in buffer */
        int     buf_bit_idx; /* pointer to top bit of top byte in buffer */

        /* format of file in rd mode (BINARY/ASCII) */
    } Bit_stream_struc;



    /* variables used for --nspsytune */
    typedef struct {
        /* variables for nspsytune */
        FLOAT   last_en_subshort[4][9];
        int     last_attacks[4];
        FLOAT   pefirbuf[19];
        FLOAT   longfact[SBMAX_l];
        FLOAT   shortfact[SBMAX_s];

        /* short block tuning */
        FLOAT   attackthre;
        FLOAT   attackthre_s;
    } nsPsy_t;


    typedef struct {
        int     sum;         /* what we have seen so far */
        int     seen;        /* how many frames we have seen in this chunk */
        int     want;        /* how many frames we want to collect into one chunk */
        int     pos;         /* actual position in our bag */
        int     size;        /* size of our bag */
        int    *bag;         /* pointer to our bag */
    	unsigned int nVbrNumFrames;
	unsigned long nBytesWritten;
        /* VBR tag data */
        unsigned int TotalFrameSize;
    } VBR_seek_info_t;


    /**
     *  ATH related stuff, if something new ATH related has to be added,
     *  please plugg it here into the ATH_t struct
     */
    typedef struct {
        int     use_adjust;  /* method for the auto adjustment  */
        FLOAT   aa_sensitivity_p; /* factor for tuning the (sample power)
                                     point below which adaptive threshold
                                     of hearing adjustment occurs */
        FLOAT   adjust;      /* lowering based on peak volume, 1 = no lowering */
        FLOAT   adjust_limit; /* limit for dynamic ATH adjust */
        FLOAT   decay;       /* determined to lower x dB each second */
        FLOAT   floor;       /* lowest ATH value */
        FLOAT   l[SBMAX_l];  /* ATH for sfbs in long blocks */
        FLOAT   s[SBMAX_s];  /* ATH for sfbs in short blocks */
        FLOAT   psfb21[PSFB21]; /* ATH for partitionned sfb21 in long blocks */
        FLOAT   psfb12[PSFB12]; /* ATH for partitionned sfb12 in short blocks */
        FLOAT   cb_l[CBANDS]; /* ATH for long block convolution bands */
        FLOAT   cb_s[CBANDS]; /* ATH for short block convolution bands */
        FLOAT   eql_w[BLKSIZE / 2]; /* equal loudness weights (based on ATH) */
    } ATH_t;

    /**
     *  PSY Model related stuff
     */
    typedef struct {
        FLOAT   mask_adjust; /* the dbQ stuff */
        FLOAT   mask_adjust_short; /* the dbQ stuff */        
        /* at transition from one scalefactor band to next */
        FLOAT   bo_l_weight[SBMAX_l]; /* band weight long scalefactor bands */
        FLOAT   bo_s_weight[SBMAX_s]; /* band weight short scalefactor bands */
    } PSY_t;


#define MAX_CHANNELS  2



    struct lame_internal_flags {

  /********************************************************************
   * internal variables NOT set by calling program, and should not be *
   * modified by the calling program                                  *
   ********************************************************************/

        /*
         * Some remarks to the Class_ID field:
         * The Class ID is an Identifier for a pointer to this struct.
         * It is very unlikely that a pointer to lame_global_flags has the same 32 bits
         * in it's structure (large and other special properties, for instance prime).
         *
         * To test that the structure is right and initialized, use:
         *     if ( gfc -> Class_ID == LAME_ID ) ...
         * Other remark:
         *     If you set a flag to 0 for uninit data and 1 for init data, the right test
         *     should be "if (flag == 1)" and NOT "if (flag)". Unintended modification
         *     of this element will be otherwise misinterpreted as an init.
         */
#  define  LAME_ID   0xFFF88E3B
        unsigned long Class_ID;

        int     lame_encode_frame_init;
        int     iteration_init_init;
        int     fill_buffer_resample_init;

#ifndef  MFSIZE
# define MFSIZE  ( 3*1152 + ENCDELAY - MDCTDELAY )
#endif
        sample_t mfbuf[2][MFSIZE];


        struct {
            void    (*msgf) (const char *format, va_list ap);
            void    (*debugf) (const char *format, va_list ap);
            void    (*errorf) (const char *format, va_list ap);
        } report;

        int     mode_gr;     /* granules per frame */
        int     channels_in; /* number of channels in the input data stream (PCM or decoded PCM) */
        int     channels_out; /* number of channels in the output data stream (not used for decoding) */
        double  resample_ratio; /* input_samp_rate/output_samp_rate */

        int     mf_samples_to_encode;
        int     mf_size;
        int     VBR_min_bitrate; /* min bitrate index */
        int     VBR_max_bitrate; /* max bitrate index */
        int     bitrate_index;
        int     samplerate_index;
        int     mode_ext;


        /* lowpass and highpass filter control */
        FLOAT   lowpass1, lowpass2; /* normalized frequency bounds of passband */
        FLOAT   highpass1, highpass2; /* normalized frequency bounds of passband */

        int     noise_shaping; /* 0 = none
                                  1 = ISO AAC model
                                  2 = allow scalefac_select=1
                                */

        int     noise_shaping_amp; /*  0 = ISO model: amplify all distorted bands
                                      1 = amplify within 50% of max (on db scale)
                                      2 = amplify only most distorted band
                                      3 = method 1 and refine with method 2
                                    */
        int     substep_shaping; /* 0 = no substep
                                    1 = use substep shaping at last step(VBR only)
                                    (not implemented yet)
                                    2 = use substep inside loop
                                    3 = use substep inside loop and last step
                                  */

        int     psymodel;    /* 1 = gpsycho. 0 = none */
        int     noise_shaping_stop; /* 0 = stop at over=0, all scalefacs amplified or
                                       a scalefac has reached max value
                                       1 = stop when all scalefacs amplified or
                                       a scalefac has reached max value
                                       2 = stop when all scalefacs amplified
                                     */

        int     subblock_gain; /*  0 = no, 1 = yes */
        int     use_best_huffman; /* 0 = no.  1=outside loop  2=inside loop(slow) */

        int     full_outer_loop; /* 0 = stop early after 0 distortion found. 1 = full search */


        /* variables used by lame.c */
        Bit_stream_struc bs;
        III_side_info_t l3_side;
        FLOAT   ms_ratio[2];

        /* used for padding */
        int     padding;     /* padding for the current frame? */
        int     frac_SpF;
        int     slot_lag;


        /* optional ID3 tags, used in id3tag.c  */
        struct id3tag_spec tag_spec;
        uint16_t nMusicCRC;


        /* variables used by quantize.c */
        int     OldValue[2];
        int     CurrentStep[2];

        FLOAT   masking_lower;
        char    bv_scf[576];
        int     pseudohalf[SFBMAX];

        int     sfb21_extra; /* will be set in lame_init_params */

        /* variables used by util.c */
        /* BPC = maximum number of filter convolution windows to precompute */
#define BPC 320
        sample_t *inbuf_old[2];
        sample_t *blackfilt[2 * BPC + 1];
        double  itime[2];
        int     sideinfo_len;

        /* variables for newmdct.c */
        FLOAT   sb_sample[2][2][18][SBLIMIT];
        FLOAT   amp_filter[32];

        /* variables for bitstream.c */
        /* mpeg1: buffer=511 bytes  smallest frame: 96-38(sideinfo)=58
         * max number of frames in reservoir:  8
         * mpeg2: buffer=255 bytes.  smallest frame: 24-23bytes=1
         * with VBR, if you are encoding all silence, it is possible to
         * have 8kbs/24khz frames with 1byte of data each, which means we need
         * to buffer up to 255 headers! */
        /* also, max_header_buf has to be a power of two */
#define MAX_HEADER_BUF 256
#define MAX_HEADER_LEN 40    /* max size of header is 38 */
        struct {
            int     write_timing;
            int     ptr;
            char    buf[MAX_HEADER_LEN];
        } header[MAX_HEADER_BUF];

        int     h_ptr;
        int     w_ptr;
        int     ancillary_flag;

        /* variables for reservoir.c */
        int     ResvSize;    /* in bits */
        int     ResvMax;     /* in bits */

        scalefac_struct scalefac_band;

        /* DATA FROM PSYMODEL.C */
/* The static variables "r", "phi_sav", "new", "old" and "oldest" have    */
/* to be remembered for the unpredictability measure.  For "r" and        */
/* "phi_sav", the first index from the left is the channel select and     */
/* the second index is the "age" of the data.                             */
        FLOAT   minval_l[CBANDS];
        FLOAT   minval_s[CBANDS];
        FLOAT   nb_1[4][CBANDS], nb_2[4][CBANDS];
        FLOAT   nb_s1[4][CBANDS], nb_s2[4][CBANDS];
        FLOAT  *s3_ss;
        FLOAT  *s3_ll;
        FLOAT   decay;

        III_psy_xmin thm[4];
        III_psy_xmin en[4];

        /* fft and energy calculation    */
        FLOAT   tot_ener[4];

        /* loudness calculation (for adaptive threshold of hearing) */
        FLOAT   loudness_sq[2][2]; /* loudness^2 approx. per granule and channel */
        FLOAT   loudness_sq_save[2]; /* account for granule delay of L3psycho_anal */


        /* Scale Factor Bands    */
        FLOAT   mld_l[SBMAX_l], mld_s[SBMAX_s];
        int     bm_l[SBMAX_l], bo_l[SBMAX_l];
        int     bm_s[SBMAX_s], bo_s[SBMAX_s];
        int     npart_l, npart_s;

        int     s3ind[CBANDS][2];
        int     s3ind_s[CBANDS][2];

        int     numlines_s[CBANDS];
        int     numlines_l[CBANDS];
        FLOAT   rnumlines_l[CBANDS];
        FLOAT   mld_cb_l[CBANDS], mld_cb_s[CBANDS];
        int     numlines_s_num1;
        int     numlines_l_num1;

        /* ratios  */
        FLOAT   pe[4];
        FLOAT   ms_ratio_s_old, ms_ratio_l_old;
        FLOAT   ms_ener_ratio_old;

        /* block type */
        int     blocktype_old[2];

        /* CPU features */
        struct {
            unsigned int MMX:1; /* Pentium MMX, Pentium II...IV, K6, K6-2,
                                   K6-III, Athlon */
            unsigned int AMD_3DNow:1; /* K6-2, K6-III, Athlon      */
            unsigned int SSE:1; /* Pentium III, Pentium 4    */
            unsigned int SSE2:1; /* Pentium 4, K8             */
        } CPU_features;

        /* functions to replace with CPU feature optimized versions in takehiro.c */
        int     (*choose_table) (const int *ix, const int *const end, int *const s);
        void    (*fft_fht) (FLOAT *, int);
        void    (*init_xrpow_core) (gr_info * const cod_info, FLOAT xrpow[576], int upper,
                                    FLOAT * sum);



        nsPsy_t nsPsy;       /* variables used for --nspsytune */

        VBR_seek_info_t VBR_seek_table; /* used for Xing VBR header */

        ATH_t  *ATH;         /* all ATH related stuff */
        PSY_t  *PSY;

        int     nogap_total;
        int     nogap_current;


        /* ReplayGain */
        unsigned int decode_on_the_fly:1;
        unsigned int findReplayGain:1;
        unsigned int findPeakSample:1;
        sample_t PeakSample;
        int     RadioGain;
        int     AudiophileGain;
        replaygain_t *rgdata;

        int     noclipGainChange; /* gain change required for preventing clipping */
        FLOAT   noclipScale; /* user-specified scale factor required for preventing clipping */


        /* simple statistics */
        int     bitrate_stereoMode_Hist[16][4 + 1];
        int     bitrate_blockType_Hist[16][4 + 1 + 1]; /*norm/start/short/stop/mixed(short)/sum */

        /* used by the frame analyzer */
        plotting_data *pinfo;
        hip_t hip;

        int     in_buffer_nsamples;
        sample_t *in_buffer_0;
        sample_t *in_buffer_1;

        iteration_loop_t iteration_loop;
    };

#ifndef lame_internal_flags_defined
#define lame_internal_flags_defined
    typedef struct lame_internal_flags lame_internal_flags;
#endif


/***********************************************************************
*
*  Global Function Prototype Declarations
*
***********************************************************************/
    void    freegfc(lame_internal_flags * const gfc);
    void    free_id3tag(lame_internal_flags * const gfc);
    extern int BitrateIndex(int, int, int);
    extern int FindNearestBitrate(int, int, int);
    extern int map2MP3Frequency(int freq);
    extern int SmpFrqIndex(int, int *const);
    extern int nearestBitrateFullIndex(const int brate);
    extern FLOAT ATHformula(FLOAT freq, lame_global_flags const *gfp);
    extern FLOAT freq2bark(FLOAT freq);
    extern FLOAT freq2cbw(FLOAT freq);
    void    disable_FPE(void);

/* log/log10 approximations */
    extern void init_log_table(void);
    extern ieee754_float32_t fast_log2(ieee754_float32_t x);


    void    fill_buffer(lame_global_flags const *gfp,
                        sample_t * mfbuf[2],
                        sample_t const *in_buffer[2], int nsamples, int *n_in, int *n_out);

    int     fill_buffer_resample(lame_global_flags const *gfp,
                                 sample_t * outbuf,
                                 int desired_len,
                                 sample_t const *inbuf, int len, int *num_used, int channels);

/* same as hip_decode1 (look in lame.h), but returns
   unclipped raw floating-point samples. It is declared
   here, not in lame.h, because it returns LAME's
   internal type sample_t. No more than 1152 samples
   per channel are allowed. */
    int     hip_decode1_unclipped(hip_t hip, unsigned char *buffer,
                                  size_t len, sample_t pcm_l[], sample_t pcm_r[]);


    extern int has_MMX(void);
    extern int has_3DNow(void);
    extern int has_SSE(void);
    extern int has_SSE2(void);



/***********************************************************************
*
*  Macros about Message Printing and Exit
*
***********************************************************************/
    extern void lame_errorf(const lame_internal_flags * gfc, const char *, ...);
    extern void lame_debugf(const lame_internal_flags * gfc, const char *, ...);
    extern void lame_msgf(const lame_internal_flags * gfc, const char *, ...);
#define DEBUGF  lame_debugf
#define ERRORF  lame_errorf
#define MSGF    lame_msgf

    extern void hip_set_pinfo(hip_t hip, plotting_data* pinfo);

#ifdef __cplusplus
}
#endif
#endif                       /* LAME_UTIL_H */

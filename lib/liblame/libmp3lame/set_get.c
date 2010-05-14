/* -*- mode: C; mode: fold -*- */
/*
 * set/get functions for lame_global_flags
 *
 * Copyright (c) 2001-2005 Alexander Leidinger
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

/* $Id: set_get.c,v 1.88 2008/04/12 18:18:07 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "util.h"
#include "bitstream.h"  /* because of compute_flushbits */

#include "set_get.h"
#include "lame_global_flags.h"

/*
 * input stream description
 */

/* number of samples */
/* it's unlikely for this function to return an error */
int
lame_set_num_samples(lame_global_flags * gfp, unsigned long num_samples)
{
    /* default = 2^32-1 */

    gfp->num_samples = num_samples;

    return 0;
}

unsigned long
lame_get_num_samples(const lame_global_flags * gfp)
{
    return gfp->num_samples;
}


/* input samplerate */
int
lame_set_in_samplerate(lame_global_flags * gfp, int in_samplerate)
{
    /* input sample rate in Hz,  default = 44100 Hz */
    gfp->in_samplerate = in_samplerate;

    return 0;
}

int
lame_get_in_samplerate(const lame_global_flags * gfp)
{
    return gfp->in_samplerate;
}


/* number of channels in input stream */
int
lame_set_num_channels(lame_global_flags * gfp, int num_channels)
{
    /* default = 2 */

    if (2 < num_channels || 0 == num_channels)
        return -1;      /* we don't support more than 2 channels */

    gfp->num_channels = num_channels;

    return 0;
}

int
lame_get_num_channels(const lame_global_flags * gfp)
{
    return gfp->num_channels;
}


/* scale the input by this amount before encoding (not used for decoding) */
int
lame_set_scale(lame_global_flags * gfp, float scale)
{
    /* default = 0 */
    gfp->scale = scale;

    return 0;
}

float
lame_get_scale(const lame_global_flags * gfp)
{
    return gfp->scale;
}


/* scale the channel 0 (left) input by this amount before 
   encoding (not used for decoding) */
int
lame_set_scale_left(lame_global_flags * gfp, float scale)
{
    /* default = 0 */
    gfp->scale_left = scale;

    return 0;
}

float
lame_get_scale_left(const lame_global_flags * gfp)
{
    return gfp->scale_left;
}


/* scale the channel 1 (right) input by this amount before 
   encoding (not used for decoding) */
int
lame_set_scale_right(lame_global_flags * gfp, float scale)
{
    /* default = 0 */
    gfp->scale_right = scale;

    return 0;
}

float
lame_get_scale_right(const lame_global_flags * gfp)
{
    return gfp->scale_right;
}


/* output sample rate in Hz */
int
lame_set_out_samplerate(lame_global_flags * gfp, int out_samplerate)
{
    /*
     * default = 0: LAME picks best value based on the amount
     *              of compression
     * MPEG only allows:
     *  MPEG1    32, 44.1,   48khz
     *  MPEG2    16, 22.05,  24
     *  MPEG2.5   8, 11.025, 12
     *
     * (not used by decoding routines)
     */
    gfp->out_samplerate = out_samplerate;

    return 0;
}

int
lame_get_out_samplerate(const lame_global_flags * gfp)
{
    return gfp->out_samplerate;
}




/*
 * general control parameters
 */

/* collect data for an MP3 frame analzyer */
int
lame_set_analysis(lame_global_flags * gfp, int analysis)
{
    /* default = 0 */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > analysis || 1 < analysis)
        return -1;

    gfp->analysis = analysis;

    return 0;
}

int
lame_get_analysis(const lame_global_flags * gfp)
{
    assert(0 <= gfp->analysis && 1 >= gfp->analysis);

    return gfp->analysis;
}


/* write a Xing VBR header frame */
int
lame_set_bWriteVbrTag(lame_global_flags * gfp, int bWriteVbrTag)
{
    /* default = 1 (on) for VBR/ABR modes, 0 (off) for CBR mode */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > bWriteVbrTag || 1 < bWriteVbrTag)
        return -1;

    gfp->bWriteVbrTag = bWriteVbrTag;

    return 0;
}

int
lame_get_bWriteVbrTag(const lame_global_flags * gfp)
{
    assert(0 <= gfp->bWriteVbrTag && 1 >= gfp->bWriteVbrTag);

    return gfp->bWriteVbrTag;
}



/* decode only, use lame/mpglib to convert mp3 to wav */
int
lame_set_decode_only(lame_global_flags * gfp, int decode_only)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > decode_only || 1 < decode_only)
        return -1;

    gfp->decode_only = decode_only;

    return 0;
}

int
lame_get_decode_only(const lame_global_flags * gfp)
{
    assert(0 <= gfp->decode_only && 1 >= gfp->decode_only);

    return gfp->decode_only;
}


#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
/* 1=encode a Vorbis .ogg file.  default=0 */
/* DEPRECATED */
int CDECL lame_set_ogg(lame_global_flags *, int);
int CDECL lame_get_ogg(const lame_global_flags *);
#else
#endif

/* encode a Vorbis .ogg file */
/* DEPRECATED */
int
lame_set_ogg(lame_global_flags * gfp, int ogg)
{
    (void) gfp;
    (void) ogg;
    return -1;
}

int
lame_get_ogg(const lame_global_flags * gfp)
{
    (void) gfp;
    return 0;
}


/*
 * Internal algorithm selection.
 * True quality is determined by the bitrate but this variable will effect
 * quality by selecting expensive or cheap algorithms.
 * quality=0..9.  0=best (very slow).  9=worst.  
 * recommended:  3     near-best quality, not too slow
 *               5     good quality, fast
 *               7     ok quality, really fast
 */
int
lame_set_quality(lame_global_flags * gfp, int quality)
{
    gfp->quality = quality;

    return 0;
}

int
lame_get_quality(const lame_global_flags * gfp)
{
    return gfp->quality;
}


/* mode = STEREO, JOINT_STEREO, DUAL_CHANNEL (not supported), MONO */
int
lame_set_mode(lame_global_flags * gfp, MPEG_mode mode)
{
    int     mpg_mode = mode;

    /* default: lame chooses based on compression ratio and input channels */

    if (mpg_mode < 0 || MAX_INDICATOR <= mpg_mode)
        return -1;      /* Unknown MPEG mode! */

    gfp->mode = mode;

    return 0;
}

MPEG_mode
lame_get_mode(const lame_global_flags * gfp)
{
    assert(gfp->mode < MAX_INDICATOR);

    return gfp->mode;
}


#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
/*
  mode_automs.  Use a M/S mode with a switching threshold based on
  compression ratio
  DEPRECATED
*/
int CDECL lame_set_mode_automs(lame_global_flags *, int);
int CDECL lame_get_mode_automs(const lame_global_flags *);
#else
#endif

/* Us a M/S mode with a switching threshold based on compression ratio */
/* DEPRECATED */
int
lame_set_mode_automs(lame_global_flags * gfp, int mode_automs)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > mode_automs || 1 < mode_automs)
        return -1;

    lame_set_mode(gfp, JOINT_STEREO);

    return 0;
}

int
lame_get_mode_automs(const lame_global_flags * gfp)
{
    (void) gfp;
    return 1;
}


/*
 * Force M/S for all frames.  For testing only.
 * Requires mode = 1.
 */
int
lame_set_force_ms(lame_global_flags * gfp, int force_ms)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > force_ms || 1 < force_ms)
        return -1;

    gfp->force_ms = force_ms;

    return 0;
}

int
lame_get_force_ms(const lame_global_flags * gfp)
{
    assert(0 <= gfp->force_ms && 1 >= gfp->force_ms);

    return gfp->force_ms;
}


/* Use free_format. */
int
lame_set_free_format(lame_global_flags * gfp, int free_format)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > free_format || 1 < free_format)
        return -1;

    gfp->free_format = free_format;

    return 0;
}

int
lame_get_free_format(const lame_global_flags * gfp)
{
    assert(0 <= gfp->free_format && 1 >= gfp->free_format);

    return gfp->free_format;
}



/* Perform ReplayGain analysis */
int
lame_set_findReplayGain(lame_global_flags * gfp, int findReplayGain)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > findReplayGain || 1 < findReplayGain)
        return -1;

    gfp->findReplayGain = findReplayGain;

    return 0;
}

int
lame_get_findReplayGain(const lame_global_flags * gfp)
{
    assert(0 <= gfp->findReplayGain && 1 >= gfp->findReplayGain);

    return gfp->findReplayGain;
}


/* Decode on the fly. Find the peak sample. If ReplayGain analysis is 
   enabled then perform it on the decoded data. */
int
lame_set_decode_on_the_fly(lame_global_flags * gfp, int decode_on_the_fly)
{
#ifndef DECODE_ON_THE_FLY
    return -1;
#else
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > decode_on_the_fly || 1 < decode_on_the_fly)
        return -1;

    gfp->decode_on_the_fly = decode_on_the_fly;

    return 0;
#endif
}

int
lame_get_decode_on_the_fly(const lame_global_flags * gfp)
{
    assert(0 <= gfp->decode_on_the_fly && 1 >= gfp->decode_on_the_fly);

    return gfp->decode_on_the_fly;
}

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
/* DEPRECATED: now does the same as lame_set_findReplayGain()
   default = 0 (disabled) */
int CDECL lame_set_ReplayGain_input(lame_global_flags *, int);
int CDECL lame_get_ReplayGain_input(const lame_global_flags *);

/* DEPRECATED: now does the same as
   lame_set_decode_on_the_fly() && lame_set_findReplayGain()
   default = 0 (disabled) */
int CDECL lame_set_ReplayGain_decode(lame_global_flags *, int);
int CDECL lame_get_ReplayGain_decode(const lame_global_flags *);

/* DEPRECATED: now does the same as lame_set_decode_on_the_fly()
   default = 0 (disabled) */
int CDECL lame_set_findPeakSample(lame_global_flags *, int);
int CDECL lame_get_findPeakSample(const lame_global_flags *);
#else
#endif

/* DEPRECATED. same as lame_set_decode_on_the_fly() */
int
lame_set_findPeakSample(lame_global_flags * gfp, int arg)
{
    return lame_set_decode_on_the_fly(gfp, arg);
}

int
lame_get_findPeakSample(const lame_global_flags * gfp)
{
    return lame_get_decode_on_the_fly(gfp);
}

/* DEPRECATED. same as lame_set_findReplayGain() */
int
lame_set_ReplayGain_input(lame_global_flags * gfp, int arg)
{
    return lame_set_findReplayGain(gfp, arg);
}

int
lame_get_ReplayGain_input(const lame_global_flags * gfp)
{
    return lame_get_findReplayGain(gfp);
}

/* DEPRECATED. same as lame_set_decode_on_the_fly() &&
   lame_set_findReplayGain() */
int
lame_set_ReplayGain_decode(lame_global_flags * gfp, int arg)
{
    if (lame_set_decode_on_the_fly(gfp, arg) < 0 || lame_set_findReplayGain(gfp, arg) < 0)
        return -1;
    else
        return 0;
}

int
lame_get_ReplayGain_decode(const lame_global_flags * gfp)
{
    if (lame_get_decode_on_the_fly(gfp) > 0 && lame_get_findReplayGain(gfp) > 0)
        return 1;
    else
        return 0;
}


/* set and get some gapless encoding flags */

int
lame_set_nogap_total(lame_global_flags * gfp, int the_nogap_total)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    gfc->nogap_total = the_nogap_total;
    return 0;
}

int
lame_get_nogap_total(const lame_global_flags * gfp)
{
    return gfp->internal_flags->nogap_total;
}

int
lame_set_nogap_currentindex(lame_global_flags * gfp, int the_nogap_index)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    gfc->nogap_current = the_nogap_index;
    return 0;
}

int
lame_get_nogap_currentindex(const lame_global_flags * gfp)
{
    return gfp->internal_flags->nogap_current;
}


/* message handlers */
int
lame_set_errorf(lame_global_flags * gfp, void (*func) (const char *, va_list))
{
    gfp->report.errorf = func;

    return 0;
}

int
lame_set_debugf(lame_global_flags * gfp, void (*func) (const char *, va_list))
{
    gfp->report.debugf = func;

    return 0;
}

int
lame_set_msgf(lame_global_flags * gfp, void (*func) (const char *, va_list))
{
    gfp->report.msgf = func;

    return 0;
}


/*
 * Set one of
 *  - brate
 *  - compression ratio.
 *
 * Default is compression ratio of 11.
 */
int
lame_set_brate(lame_global_flags * gfp, int brate)
{
    gfp->brate = brate;

    if (brate > 320) {
        gfp->disable_reservoir = 1;
    }

    return 0;
}

int
lame_get_brate(const lame_global_flags * gfp)
{
    return gfp->brate;
}

int
lame_set_compression_ratio(lame_global_flags * gfp, float compression_ratio)
{
    gfp->compression_ratio = compression_ratio;

    return 0;
}

float
lame_get_compression_ratio(const lame_global_flags * gfp)
{
    return gfp->compression_ratio;
}




/*
 * frame parameters
 */

/* Mark as copyright protected. */
int
lame_set_copyright(lame_global_flags * gfp, int copyright)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > copyright || 1 < copyright)
        return -1;

    gfp->copyright = copyright;

    return 0;
}

int
lame_get_copyright(const lame_global_flags * gfp)
{
    assert(0 <= gfp->copyright && 1 >= gfp->copyright);

    return gfp->copyright;
}


/* Mark as original. */
int
lame_set_original(lame_global_flags * gfp, int original)
{
    /* default = 1 (enabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > original || 1 < original)
        return -1;

    gfp->original = original;

    return 0;
}

int
lame_get_original(const lame_global_flags * gfp)
{
    assert(0 <= gfp->original && 1 >= gfp->original);

    return gfp->original;
}


/*
 * error_protection.
 * Use 2 bytes from each frame for CRC checksum.
 */
int
lame_set_error_protection(lame_global_flags * gfp, int error_protection)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > error_protection || 1 < error_protection)
        return -1;

    gfp->error_protection = error_protection;

    return 0;
}

int
lame_get_error_protection(const lame_global_flags * gfp)
{
    assert(0 <= gfp->error_protection && 1 >= gfp->error_protection);

    return gfp->error_protection;
}


#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
/* padding_type. 0=pad no frames  1=pad all frames 2=adjust padding(default) */
int CDECL lame_set_padding_type(lame_global_flags *, Padding_type);
Padding_type CDECL lame_get_padding_type(const lame_global_flags *);
#else
#endif

/*
 * padding_type.
 *  PAD_NO     = pad no frames
 *  PAD_ALL    = pad all frames
 *  PAD_ADJUST = adjust padding
 */
int
lame_set_padding_type(lame_global_flags * gfp, Padding_type padding_type)
{
    (void) gfp;
    (void) padding_type;
    return 0;
}

Padding_type
lame_get_padding_type(const lame_global_flags * gfp)
{
    (void) gfp;
    return PAD_ADJUST;
}


/* MP3 'private extension' bit. Meaningless. */
int
lame_set_extension(lame_global_flags * gfp, int extension)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > extension || 1 < extension)
        return -1;

    gfp->extension = extension;

    return 0;
}

int
lame_get_extension(const lame_global_flags * gfp)
{
    assert(0 <= gfp->extension && 1 >= gfp->extension);

    return gfp->extension;
}


/* Enforce strict ISO compliance. */
int
lame_set_strict_ISO(lame_global_flags * gfp, int strict_ISO)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > strict_ISO || 1 < strict_ISO)
        return -1;

    gfp->strict_ISO = strict_ISO;

    return 0;
}

int
lame_get_strict_ISO(const lame_global_flags * gfp)
{
    assert(0 <= gfp->strict_ISO && 1 >= gfp->strict_ISO);

    return gfp->strict_ISO;
}




/********************************************************************
 * quantization/noise shaping 
 ***********************************************************************/

/* Disable the bit reservoir. For testing only. */
int
lame_set_disable_reservoir(lame_global_flags * gfp, int disable_reservoir)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > disable_reservoir || 1 < disable_reservoir)
        return -1;

    gfp->disable_reservoir = disable_reservoir;

    return 0;
}

int
lame_get_disable_reservoir(const lame_global_flags * gfp)
{
    assert(0 <= gfp->disable_reservoir && 1 >= gfp->disable_reservoir);

    return gfp->disable_reservoir;
}




int
lame_set_experimentalX(lame_global_flags * gfp, int experimentalX)
{
    lame_set_quant_comp(gfp, experimentalX);
    lame_set_quant_comp_short(gfp, experimentalX);

    return 0;
}

int
lame_get_experimentalX(const lame_global_flags * gfp)
{
    return lame_get_quant_comp(gfp);
}


/* Select a different "best quantization" function. default = 0 */
int
lame_set_quant_comp(lame_global_flags * gfp, int quant_type)
{
    gfp->quant_comp = quant_type;

    return 0;
}

int
lame_get_quant_comp(const lame_global_flags * gfp)
{
    return gfp->quant_comp;
}


/* Select a different "best quantization" function. default = 0 */
int
lame_set_quant_comp_short(lame_global_flags * gfp, int quant_type)
{
    gfp->quant_comp_short = quant_type;

    return 0;
}

int
lame_get_quant_comp_short(const lame_global_flags * gfp)
{
    return gfp->quant_comp_short;
}


/* Another experimental option. For testing only. */
int
lame_set_experimentalY(lame_global_flags * gfp, int experimentalY)
{
    gfp->experimentalY = experimentalY;

    return 0;
}

int
lame_get_experimentalY(const lame_global_flags * gfp)
{
    return gfp->experimentalY;
}


int
lame_set_experimentalZ(lame_global_flags * gfp, int experimentalZ)
{
    gfp->experimentalZ = experimentalZ;
    return 0;
}

int
lame_get_experimentalZ(const lame_global_flags * gfp)
{
    return gfp->experimentalZ;
}


/* Naoki's psycho acoustic model. */
int
lame_set_exp_nspsytune(lame_global_flags * gfp, int exp_nspsytune)
{
    /* default = 0 (disabled) */

    gfp->exp_nspsytune = exp_nspsytune;

    return 0;
}

int
lame_get_exp_nspsytune(const lame_global_flags * gfp)
{
    return gfp->exp_nspsytune;
}




/********************************************************************
 * VBR control
 ***********************************************************************/

/* Types of VBR.  default = vbr_off = CBR */
int
lame_set_VBR(lame_global_flags * gfp, vbr_mode VBR)
{
    int     vbr_q = VBR;

    if (0 > vbr_q || vbr_max_indicator <= vbr_q)
        return -1;      /* Unknown VBR mode! */

    gfp->VBR = VBR;

    return 0;
}

vbr_mode
lame_get_VBR(const lame_global_flags * gfp)
{
    assert(gfp->VBR < vbr_max_indicator);

    return gfp->VBR;
}


/*
 * VBR quality level.
 *  0 = highest
 *  9 = lowest 
 */
int
lame_set_VBR_q(lame_global_flags * gfp, int VBR_q)
{
    int     ret = 0;

    if (0 > VBR_q) {
        ret = -1;       /* Unknown VBR quality level! */
        VBR_q = 0;
    }
    if (9 < VBR_q) {
        ret = -1;
        VBR_q = 9;
    }

    gfp->VBR_q = VBR_q;
    gfp->VBR_q_frac = 0;
/*    lame_set_ATHcurve(gfp, VBR_q);
*/
    return ret;
}

int
lame_get_VBR_q(const lame_global_flags * gfp)
{
    assert(0 <= gfp->VBR_q && 10 > gfp->VBR_q);

    return gfp->VBR_q;
}

int
lame_set_VBR_quality(lame_global_flags * gfp, float VBR_q)
{
    int     ret = 0;

    if (0 > VBR_q) {
        ret = -1;       /* Unknown VBR quality level! */
        VBR_q = 0;
    }
    if (9.999 < VBR_q) {
        ret = -1;
        VBR_q = 9.999;
    }

    gfp->VBR_q = (int) VBR_q;
    gfp->VBR_q_frac = VBR_q - gfp->VBR_q;

    return ret;
}

float
lame_get_VBR_quality(const lame_global_flags * gfp)
{
    return gfp->VBR_q + gfp->VBR_q_frac;
}


/* Ignored except for VBR = vbr_abr (ABR mode) */
int
lame_set_VBR_mean_bitrate_kbps(lame_global_flags * gfp, int VBR_mean_bitrate_kbps)
{
    gfp->VBR_mean_bitrate_kbps = VBR_mean_bitrate_kbps;

    return 0;
}

int
lame_get_VBR_mean_bitrate_kbps(const lame_global_flags * gfp)
{
    return gfp->VBR_mean_bitrate_kbps;
}

int
lame_set_VBR_min_bitrate_kbps(lame_global_flags * gfp, int VBR_min_bitrate_kbps)
{
    gfp->VBR_min_bitrate_kbps = VBR_min_bitrate_kbps;

    return 0;
}

int
lame_get_VBR_min_bitrate_kbps(const lame_global_flags * gfp)
{
    return gfp->VBR_min_bitrate_kbps;
}

int
lame_set_VBR_max_bitrate_kbps(lame_global_flags * gfp, int VBR_max_bitrate_kbps)
{
    gfp->VBR_max_bitrate_kbps = VBR_max_bitrate_kbps;

    return 0;
}

int
lame_get_VBR_max_bitrate_kbps(const lame_global_flags * gfp)
{
    return gfp->VBR_max_bitrate_kbps;
}


/*
 * Strictly enforce VBR_min_bitrate.
 * Normally it will be violated for analog silence.
 */
int
lame_set_VBR_hard_min(lame_global_flags * gfp, int VBR_hard_min)
{
    /* default = 0 (disabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > VBR_hard_min || 1 < VBR_hard_min)
        return -1;

    gfp->VBR_hard_min = VBR_hard_min;

    return 0;
}

int
lame_get_VBR_hard_min(const lame_global_flags * gfp)
{
    assert(0 <= gfp->VBR_hard_min && 1 >= gfp->VBR_hard_min);

    return gfp->VBR_hard_min;
}


/********************************************************************
 * Filtering control
 ***********************************************************************/

/*
 * Freqency in Hz to apply lowpass.
 *   0 = default = lame chooses
 *  -1 = disabled
 */
int
lame_set_lowpassfreq(lame_global_flags * gfp, int lowpassfreq)
{
    gfp->lowpassfreq = lowpassfreq;

    return 0;
}

int
lame_get_lowpassfreq(const lame_global_flags * gfp)
{
    return gfp->lowpassfreq;
}


/*
 * Width of transition band (in Hz).
 *  default = one polyphase filter band
 */
int
lame_set_lowpasswidth(lame_global_flags * gfp, int lowpasswidth)
{
    gfp->lowpasswidth = lowpasswidth;

    return 0;
}

int
lame_get_lowpasswidth(const lame_global_flags * gfp)
{
    return gfp->lowpasswidth;
}


/*
 * Frequency in Hz to apply highpass.
 *   0 = default = lame chooses
 *  -1 = disabled
 */
int
lame_set_highpassfreq(lame_global_flags * gfp, int highpassfreq)
{
    gfp->highpassfreq = highpassfreq;

    return 0;
}

int
lame_get_highpassfreq(const lame_global_flags * gfp)
{
    return gfp->highpassfreq;
}


/*
 * Width of transition band (in Hz).
 *  default = one polyphase filter band
 */
int
lame_set_highpasswidth(lame_global_flags * gfp, int highpasswidth)
{
    gfp->highpasswidth = highpasswidth;

    return 0;
}

int
lame_get_highpasswidth(const lame_global_flags * gfp)
{
    return gfp->highpasswidth;
}




/*
 * psycho acoustics and other arguments which you should not change 
 * unless you know what you are doing
 */


/* Adjust masking values. */
int
lame_set_maskingadjust(lame_global_flags * gfp, float adjust)
{
    gfp->maskingadjust = adjust;
    return 0;
}

float
lame_get_maskingadjust(const lame_global_flags * gfp)
{
    return gfp->maskingadjust;
}

int
lame_set_maskingadjust_short(lame_global_flags * gfp, float adjust)
{
    gfp->maskingadjust_short = adjust;
    return 0;
}

float
lame_get_maskingadjust_short(const lame_global_flags * gfp)
{
    return gfp->maskingadjust_short;
}

/* Only use ATH for masking. */
int
lame_set_ATHonly(lame_global_flags * gfp, int ATHonly)
{
    gfp->ATHonly = ATHonly;

    return 0;
}

int
lame_get_ATHonly(const lame_global_flags * gfp)
{
    return gfp->ATHonly;
}


/* Only use ATH for short blocks. */
int
lame_set_ATHshort(lame_global_flags * gfp, int ATHshort)
{
    gfp->ATHshort = ATHshort;

    return 0;
}

int
lame_get_ATHshort(const lame_global_flags * gfp)
{
    return gfp->ATHshort;
}


/* Disable ATH. */
int
lame_set_noATH(lame_global_flags * gfp, int noATH)
{
    gfp->noATH = noATH;

    return 0;
}

int
lame_get_noATH(const lame_global_flags * gfp)
{
    return gfp->noATH;
}


/* Select ATH formula. */
int
lame_set_ATHtype(lame_global_flags * gfp, int ATHtype)
{
    /* XXX: ATHtype should be converted to an enum. */
    gfp->ATHtype = ATHtype;

    return 0;
}

int
lame_get_ATHtype(const lame_global_flags * gfp)
{
    return gfp->ATHtype;
}


/* Select ATH formula 4 shape. */
int
lame_set_ATHcurve(lame_global_flags * gfp, float ATHcurve)
{
    gfp->ATHcurve = ATHcurve;

    return 0;
}

float
lame_get_ATHcurve(const lame_global_flags * gfp)
{
    return gfp->ATHcurve;
}


/* Lower ATH by this many db. */
int
lame_set_ATHlower(lame_global_flags * gfp, float ATHlower)
{
    gfp->ATHlower = -ATHlower / 10.0;
    return 0;
}

float
lame_get_ATHlower(const lame_global_flags * gfp)
{
    return -gfp->ATHlower * 10.0;
}


/* Select ATH adaptive adjustment scheme. */
int
lame_set_athaa_type(lame_global_flags * gfp, int athaa_type)
{
    gfp->athaa_type = athaa_type;
    return 0;
}

int
lame_get_athaa_type(const lame_global_flags * gfp)
{
    return gfp->athaa_type;
}


/* Select the loudness approximation used by the ATH adaptive auto-leveling. */
int
lame_set_athaa_loudapprox(lame_global_flags * gfp, int athaa_loudapprox)
{
    (void) athaa_loudapprox;
    ERRORF(gfp->internal_flags, "--athaa-loudapprox is obsolete\n");
    return 0;
}

int
lame_get_athaa_loudapprox(const lame_global_flags * gfp)
{
    (void) gfp;
    /* obsolete, the type known under number 2 is the only survival */
    return 2;
}


/* Adjust (in dB) the point below which adaptive ATH level adjustment occurs. */
int
lame_set_athaa_sensitivity(lame_global_flags * gfp, float athaa_sensitivity)
{
    gfp->athaa_sensitivity = athaa_sensitivity;

    return 0;
}

float
lame_get_athaa_sensitivity(const lame_global_flags * gfp)
{
    return gfp->athaa_sensitivity;
}


/* Predictability limit (ISO tonality formula) */
int     lame_set_cwlimit(lame_global_flags * gfp, int cwlimit);
int     lame_get_cwlimit(const lame_global_flags * gfp);

int
lame_set_cwlimit(lame_global_flags * gfp, int cwlimit)
{
    (void) gfp;
    (void) cwlimit;
    return 0;
}

int
lame_get_cwlimit(const lame_global_flags * gfp)
{
    (void) gfp;
    return 0;
}



/*
 * Allow blocktypes to differ between channels.
 * default:
 *  0 for jstereo => block types coupled
 *  1 for stereo  => block types may differ
 */
int
lame_set_allow_diff_short(lame_global_flags * gfp, int allow_diff_short)
{
    gfp->short_blocks = allow_diff_short ? short_block_allowed : short_block_coupled;

    return 0;
}

int
lame_get_allow_diff_short(const lame_global_flags * gfp)
{
    if (gfp->short_blocks == short_block_allowed)
        return 1;       /* short blocks allowed to differ */
    else
        return 0;       /* not set, dispensed, forced or coupled */
}


/* Use temporal masking effect */
int
lame_set_useTemporal(lame_global_flags * gfp, int useTemporal)
{
    /* default = 1 (enabled) */

    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > useTemporal || 1 < useTemporal)
        return -1;

    gfp->useTemporal = useTemporal;

    return 0;
}

int
lame_get_useTemporal(const lame_global_flags * gfp)
{
    assert(0 <= gfp->useTemporal && 1 >= gfp->useTemporal);

    return gfp->useTemporal;
}


/* Use inter-channel masking effect */
int
lame_set_interChRatio(lame_global_flags * gfp, float ratio)
{
    /* default = 0.0 (no inter-channel maskin) */
    if (!(0 <= ratio && ratio <= 1.0))
        return -1;

    gfp->interChRatio = ratio;

    return 0;
}

float
lame_get_interChRatio(const lame_global_flags * gfp)
{
    assert((0 <= gfp->interChRatio && gfp->interChRatio <= 1.0) || EQ(gfp->interChRatio, -1));

    return gfp->interChRatio;
}


/* Use pseudo substep shaping method */
int
lame_set_substep(lame_global_flags * gfp, int method)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    /* default = 0.0 (no substep noise shaping) */
    if (!(0 <= method && method <= 7))
        return -1;

    gfc->substep_shaping = method;
    return 0;
}

int
lame_get_substep(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    assert(0 <= gfc->substep_shaping && gfc->substep_shaping <= 7);
    return gfc->substep_shaping;
}

/* scalefactors scale */
int
lame_set_sfscale(lame_global_flags * gfp, int val)
{
    if (val)
        gfp->internal_flags->noise_shaping = 2;
    else
        gfp->internal_flags->noise_shaping = 1;
    return 0;
}

int
lame_get_sfscale(const lame_global_flags * gfp)
{
    return (gfp->internal_flags->noise_shaping == 2);
}

/* subblock gain */
int
lame_set_subblock_gain(lame_global_flags * gfp, int sbgain)
{
    gfp->internal_flags->subblock_gain = sbgain;
    return sbgain;
}

int
lame_get_subblock_gain(const lame_global_flags * gfp)
{
    return gfp->internal_flags->subblock_gain;
}


/* Disable short blocks. */
int
lame_set_no_short_blocks(lame_global_flags * gfp, int no_short_blocks)
{
    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > no_short_blocks || 1 < no_short_blocks)
        return -1;

    gfp->short_blocks = no_short_blocks ? short_block_dispensed : short_block_allowed;

    return 0;
}

int
lame_get_no_short_blocks(const lame_global_flags * gfp)
{
    switch (gfp->short_blocks) {
    default:
    case short_block_not_set:
        return -1;
    case short_block_dispensed:
        return 1;
    case short_block_allowed:
    case short_block_coupled:
    case short_block_forced:
        return 0;
    }
}


/* Force short blocks. */
int
lame_set_force_short_blocks(lame_global_flags * gfp, int short_blocks)
{
    /* enforce disable/enable meaning, if we need more than two values
       we need to switch to an enum to have an apropriate representation
       of the possible meanings of the value */
    if (0 > short_blocks || 1 < short_blocks)
        return -1;

    if (short_blocks == 1)
        gfp->short_blocks = short_block_forced;
    else if (gfp->short_blocks == short_block_forced)
        gfp->short_blocks = short_block_allowed;

    return 0;
}

int
lame_get_force_short_blocks(const lame_global_flags * gfp)
{
    switch (gfp->short_blocks) {
    default:
    case short_block_not_set:
        return -1;
    case short_block_dispensed:
    case short_block_allowed:
    case short_block_coupled:
        return 0;
    case short_block_forced:
        return 1;
    }
}

int
lame_set_short_threshold_lrm(lame_global_flags * gfp, float lrm)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    gfc->nsPsy.attackthre = lrm;
    return 0;
}

float
lame_get_short_threshold_lrm(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return gfc->nsPsy.attackthre;
}

int
lame_set_short_threshold_s(lame_global_flags * gfp, float s)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    gfc->nsPsy.attackthre_s = s;
    return 0;
}

float
lame_get_short_threshold_s(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return gfc->nsPsy.attackthre_s;
}

int
lame_set_short_threshold(lame_global_flags * gfp, float lrm, float s)
{
    lame_set_short_threshold_lrm(gfp, lrm);
    lame_set_short_threshold_s(gfp, s);
    return 0;
}


/*
 * Input PCM is emphased PCM
 * (for instance from one of the rarely emphased CDs).
 *
 * It is STRONGLY not recommended to use this, because psycho does not
 * take it into account, and last but not least many decoders
 * ignore these bits
 */
int
lame_set_emphasis(lame_global_flags * gfp, int emphasis)
{
    /* XXX: emphasis should be converted to an enum */
    if (0 > emphasis || 4 <= emphasis)
        return -1;

    gfp->emphasis = emphasis;

    return 0;
}

int
lame_get_emphasis(const lame_global_flags * gfp)
{
    assert(0 <= gfp->emphasis && 4 > gfp->emphasis);

    return gfp->emphasis;
}




/***************************************************************/
/* internal variables, cannot be set...                        */
/* provided because they may be of use to calling application  */
/***************************************************************/

/* MPEG version.
 *  0 = MPEG-2
 *  1 = MPEG-1
 * (2 = MPEG-2.5)    
 */
int
lame_get_version(const lame_global_flags * gfp)
{
    return gfp->version;
}


/* Encoder delay. */
int
lame_get_encoder_delay(const lame_global_flags * gfp)
{
    return gfp->encoder_delay;
}

/* padding added to the end of the input */
int
lame_get_encoder_padding(const lame_global_flags * gfp)
{
    return gfp->encoder_padding;
}


/* Size of MPEG frame. */
int
lame_get_framesize(const lame_global_flags * gfp)
{
    return gfp->framesize;
}


/* Number of frames encoded so far. */
int
lame_get_frameNum(const lame_global_flags * gfp)
{
    return gfp->frameNum;
}

int
lame_get_mf_samples_to_encode(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return gfc->mf_samples_to_encode;
}

int     CDECL
lame_get_size_mp3buffer(const lame_global_flags * gfp)
{
    int     size;
    compute_flushbits(gfp, &size);
    return size;
}

int
lame_get_RadioGain(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return gfc->RadioGain;
}

int
lame_get_AudiophileGain(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return gfc->AudiophileGain;
}

float
lame_get_PeakSample(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return (float) gfc->PeakSample;
}

int
lame_get_noclipGainChange(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return gfc->noclipGainChange;
}

float
lame_get_noclipScale(const lame_global_flags * gfp)
{
    lame_internal_flags *gfc = gfp->internal_flags;
    return gfc->noclipScale;
}


/*
 * LAME's estimate of the total number of frames to be encoded.
 * Only valid if calling program set num_samples.
 */
int
lame_get_totalframes(const lame_global_flags * gfp)
{
    int     totalframes;
    /* estimate based on user set num_samples: */
    totalframes =
        2 + ((double) gfp->num_samples * gfp->out_samplerate) /
        ((double) gfp->in_samplerate * gfp->framesize);

    /* check to see if we underestimated totalframes */
    /*    if (totalframes < gfp->frameNum) */
    /*        totalframes = gfp->frameNum; */

    return totalframes;
}





int
lame_set_preset(lame_global_flags * gfp, int preset)
{
    gfp->preset = preset;
    return apply_preset(gfp, preset, 1);
}



int
lame_set_asm_optimizations(lame_global_flags * gfp, int optim, int mode)
{
    mode = (mode == 1 ? 1 : 0);
    switch (optim) {
    case MMX:{
            gfp->asm_optimizations.mmx = mode;
            return optim;
        }
    case AMD_3DNOW:{
            gfp->asm_optimizations.amd3dnow = mode;
            return optim;
        }
    case SSE:{
            gfp->asm_optimizations.sse = mode;
            return optim;
        }
    default:
        return optim;
    }
}


void
lame_set_write_id3tag_automatic(lame_global_flags * gfp, int v)
{
    if (gfp) {
        gfp->write_id3tag_automatic = v;
    }
}


int
lame_get_write_id3tag_automatic(lame_global_flags const *gfp)
{
    if (gfp) {
        return gfp->write_id3tag_automatic;
    }
    return 1;
}


/*

UNDOCUMENTED, experimental settings.  These routines are not prototyped
in lame.h.  You should not use them, they are experimental and may
change.  

*/


/*
 *  just another daily changing developer switch  
 */
void CDECL lame_set_tune(lame_global_flags *, float);

void
lame_set_tune(lame_global_flags * gfp, float val)
{
    gfp->tune_value_a = val;
    gfp->tune = 1;
}

/* Custom msfix hack */
void
lame_set_msfix(lame_global_flags * gfp, double msfix)
{
    /* default = 0 */
    gfp->msfix = msfix;
}

float
lame_get_msfix(const lame_global_flags * gfp)
{
    return gfp->msfix;
}

#if DEPRECATED_OR_OBSOLETE_CODE_REMOVED
int CDECL lame_set_preset_expopts(lame_global_flags *, int);
#else
#endif

int
lame_set_preset_expopts(lame_global_flags * gfp, int preset_expopts)
{
    (void) gfp;
    (void) preset_expopts;
    return 0;
}


int
lame_set_preset_notune(lame_global_flags * gfp, int preset_notune)
{
    (void) gfp;
    (void) preset_notune;
    return 0;
}

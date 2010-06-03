/*
 *      time status related function source file
 *
 *      Copyright (c) 1999 Mark Taylor
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

/* $Id: timestatus.c,v 1.46 2008/04/12 18:18:06 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


/* Hope it works now, otherwise complain or flame ;-)
 */


#if 1
# define SPEED_CHAR     "x" /* character x */
# define SPEED_MULT     1.
#else
# define SPEED_CHAR     "%%"
# define SPEED_MULT     100.
#endif

#include <assert.h>
#include <time.h>
#include <string.h>

#include "lame.h"
#include "main.h"
#include "lametime.h"
#include "timestatus.h"

#if defined(BRHIST)
# include "brhist.h"
#endif
#include "console.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

typedef struct {
    double  last_time;       /* result of last call to clock */
    double  elapsed_time;    /* total time */
    double  estimated_time;  /* estimated total duration time [s] */
    double  speed_index;     /* speed relative to realtime coding [100%] */
} timestatus_t;

/*
 *  Calculates from the input (see below) the following values:
 *    - total estimated time
 *    - a speed index
 */

static void
ts_calc_times(timestatus_t * const tstime, /* tstime->elapsed_time: elapsed time */
              const int sample_freq, /* sample frequency [Hz/kHz]  */
              const int frameNum, /* Number of the current Frame */
              const int totalframes, /* total umber of Frames */
              const int framesize)
{                       /* Size of a frame [bps/kbps] */
    assert(sample_freq >= 8000 && sample_freq <= 48000);

    if (frameNum > 0 && tstime->elapsed_time > 0) {
        tstime->estimated_time = tstime->elapsed_time * totalframes / frameNum;
        tstime->speed_index = framesize * frameNum / (sample_freq * tstime->elapsed_time);
    }
    else {
        tstime->estimated_time = 0.;
        tstime->speed_index = 0.;
    }
}

/* Decomposes a given number of seconds into a easy to read hh:mm:ss format
 * padded with an additional character
 */

static void
ts_time_decompose(const unsigned long time_in_sec, const char padded_char)
{
    const unsigned long hour = time_in_sec / 3600;
    const unsigned int min = time_in_sec / 60 % 60;
    const unsigned int sec = time_in_sec % 60;

    if (hour == 0)
        console_printf("   %2u:%02u%c", min, sec, padded_char);
    else if (hour < 100)
        console_printf("%2lu:%02u:%02u%c", hour, min, sec, padded_char);
    else
        console_printf("%6lu h%c", hour, padded_char);
}

static void
timestatus(const lame_global_flags * const gfp)
{
    static timestatus_t real_time;
    static timestatus_t proc_time;
    int     percent;
    static int init = 0;     /* What happens here? A work around instead of a bug fix ??? */
    double  tmx, delta;
    int samp_rate     = lame_get_out_samplerate(gfp)
      , frameNum      = lame_get_frameNum(gfp)
      , totalframes   = lame_get_totalframes(gfp)
      , framesize     = lame_get_framesize(gfp)
      ;

    if (totalframes < frameNum) {
        totalframes = frameNum;
    }
    if (frameNum == 0) {
        real_time.last_time = GetRealTime();
        proc_time.last_time = GetCPUTime();
        real_time.elapsed_time = 0;
        proc_time.elapsed_time = 0;
    }

    /* we need rollover protection for GetCPUTime, and maybe GetRealTime(): */
    tmx = GetRealTime();
    delta = tmx - real_time.last_time;
    if (delta < 0)
        delta = 0;      /* ignore, clock has rolled over */
    real_time.elapsed_time += delta;
    real_time.last_time = tmx;


    tmx = GetCPUTime();
    delta = tmx - proc_time.last_time;
    if (delta < 0)
        delta = 0;      /* ignore, clock has rolled over */
    proc_time.elapsed_time += delta;
    proc_time.last_time = tmx;

    if (frameNum == 0 && init == 0) {
        console_printf("\r"
                       "    Frame          |  CPU time/estim | REAL time/estim | play/CPU |    ETA \n"
                       "     0/       ( 0%%)|    0:00/     :  |    0:00/     :  |         "
                       SPEED_CHAR "|     :  \r"
                       /* , Console_IO.str_clreoln, Console_IO.str_clreoln */ );
        init = 1;
        return;
    }
    /* reset init counter for next time we are called with frameNum==0 */
    if (frameNum > 0)
        init = 0;

    ts_calc_times(&real_time, samp_rate, frameNum, totalframes, framesize);
    ts_calc_times(&proc_time, samp_rate, frameNum, totalframes, framesize);

    if (frameNum < totalframes) {
        percent = (int) (100. * frameNum / totalframes + 0.5);
    }
    else {
        percent = 100;
    }

    console_printf("\r%6i/%-6i", frameNum, totalframes);
    console_printf(percent < 100 ? " (%2d%%)|" : "(%3.3d%%)|", percent);
    ts_time_decompose((unsigned long) proc_time.elapsed_time, '/');
    ts_time_decompose((unsigned long) proc_time.estimated_time, '|');
    ts_time_decompose((unsigned long) real_time.elapsed_time, '/');
    ts_time_decompose((unsigned long) real_time.estimated_time, '|');
    console_printf(proc_time.speed_index <= 1. ?
                   "%9.4f" SPEED_CHAR "|" : "%#9.5g" SPEED_CHAR "|",
                   SPEED_MULT * proc_time.speed_index);
    ts_time_decompose((unsigned long) (real_time.estimated_time - real_time.elapsed_time), ' ');
}

static void
timestatus_finish(void)
{
    console_printf("\n");
}


void
encoder_progress_begin( lame_global_flags const* gf
                      , char              const* inPath
                      , char              const* outPath
                      )
{
    if (silent < 10) {
        lame_print_config(gf); /* print useful information about options being used */

        console_printf("Encoding %s%s to %s\n",
                       strcmp(inPath, "-") ? inPath : "<stdin>",
                       strlen(inPath) + strlen(outPath) < 66 ? "" : "\n     ",
                       strcmp(outPath, "-") ? outPath : "<stdout>");

        console_printf("Encoding as %g kHz ", 1.e-3 * lame_get_out_samplerate(gf));

        {
            static const char *mode_names[2][4] = {
                {"stereo", "j-stereo", "dual-ch", "single-ch"},
                {"stereo", "force-ms", "dual-ch", "single-ch"}
            };
            switch (lame_get_VBR(gf)) {
            case vbr_rh:
                console_printf("%s MPEG-%u%s Layer III VBR(q=%g) qval=%i\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               lame_get_VBR_quality(gf),
                               lame_get_quality(gf));
                break;
            case vbr_mt:
            case vbr_mtrh:
                console_printf("%s MPEG-%u%s Layer III VBR(q=%g)\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               lame_get_VBR_quality(gf));
                break;
            case vbr_abr:
                console_printf("%s MPEG-%u%s Layer III (%gx) average %d kbps qval=%i\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               0.1 * (int) (10. * lame_get_compression_ratio(gf) + 0.5),
                               lame_get_VBR_mean_bitrate_kbps(gf),
                               lame_get_quality(gf));
                break;
            default:
                console_printf("%s MPEG-%u%s Layer III (%gx) %3d kbps qval=%i\n",
                               mode_names[lame_get_force_ms(gf)][lame_get_mode(gf)],
                               2 - lame_get_version(gf),
                               lame_get_out_samplerate(gf) < 16000 ? ".5" : "",
                               0.1 * (int) (10. * lame_get_compression_ratio(gf) + 0.5),
                               lame_get_brate(gf),
                               lame_get_quality(gf));
                break;
            }
        }

        if (silent <= -10) {
            lame_print_internals(gf);
        }
    }
}

void
encoder_progress( lame_global_flags const* gf )
{
    if (silent <= 0) {
        int const frames = lame_get_frameNum(gf);
        if (update_interval <= 0) {     /*  most likely --disptime x not used */
            if ((frames % 100) != 0) {  /*  true, most of the time */
                return;
            }
        }
        else {
            static double last_time = 0.0;
            if (frames != 0 && frames != 9) {
                double const act = GetRealTime();
                double const dif = act - last_time;
                if (dif >= 0 && dif < update_interval) {
                    return;
                }
            }
            last_time = GetRealTime(); /* from now! disp_time seconds */
        }
#ifdef BRHIST
        if (brhist) {
            brhist_jump_back();
        }
#endif
        timestatus(gf);
#ifdef BRHIST
        if (brhist) {
            brhist_disp(gf);
        }
#endif
        console_flush();
    }
}

void
encoder_progress_end( lame_global_flags const* gf )
{
    if (silent <= 0) {
#ifdef BRHIST
        if (brhist) {
            brhist_jump_back();
        }
#endif
        timestatus(gf);
#ifdef BRHIST
        if (brhist) {
            brhist_disp(gf);
        }
#endif
        timestatus_finish();
    }
}


/* these functions are used in get_audio.c */

void
decoder_progress(const mp3data_struct * const mp3data)
{
    static int last;
    console_printf("\rFrame#%6i/%-6i %3i kbps",
                   mp3data->framenum, mp3data->totalframes, mp3data->bitrate);

    /* Programmed with a single frame hold delay */
    /* Attention: static data */

    /* MP2 Playback is still buggy. */
    /* "'00' subbands 4-31 in intensity_stereo, bound==4" */
    /* is this really intensity_stereo or is it MS stereo? */

    if (mp3data->mode == JOINT_STEREO) {
        int     curr = mp3data->mode_ext;
        console_printf("  %s  %c",
                       curr & 2 ? last & 2 ? " MS " : "LMSR" : last & 2 ? "LMSR" : "L  R",
                       curr & 1 ? last & 1 ? 'I' : 'i' : last & 1 ? 'i' : ' ');
        last = curr;
    }
    else {
        console_printf("         ");
        last = 0;
    }
/*    console_printf ("%s", Console_IO.str_clreoln ); */
    console_printf("        \b\b\b\b\b\b\b\b");
}

void
decoder_progress_finish()
{
    console_printf("\n");
}

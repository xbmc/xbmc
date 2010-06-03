/*
 *	Bitrate histogram source file
 *
 *	Copyright (c) 2000 Mark Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: brhist.c,v 1.53 2008/04/05 17:38:50 robert Exp $ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef BRHIST

/* basic #define's */

#ifndef BRHIST_WIDTH
# define BRHIST_WIDTH    14
#endif
#ifndef BRHIST_RES
# define BRHIST_RES      14
#endif


/* #includes */

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#endif

#include "brhist.h"
#include "console.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif


/* Structure holding all data related to the Console I/O
 * may be this should be a more global frontend structure. So it
 * makes sense to print all files instead with
 * printf ( "blah\n") with printf ( "blah%s\n", Console_IO.str_clreoln );
 */

extern Console_IO_t Console_IO;

static struct {
    int     vbr_bitrate_min_index;
    int     vbr_bitrate_max_index;
    int     kbps[BRHIST_WIDTH];
    int     hist_printed_lines;
    char    bar_asterisk[512 + 1]; /* buffer filled up with a lot of '*' to print a bar     */
    char    bar_percent[512 + 1]; /* buffer filled up with a lot of '%' to print a bar     */
    char    bar_coded[512 + 1]; /* buffer filled up with a lot of ' ' to print a bar     */
    char    bar_space[512 + 1]; /* buffer filled up with a lot of ' ' to print a bar     */
} brhist;

static int
calculate_index(const int *const array, const int len, const int value)
{
    int     i;

    for (i = 0; i < len; i++)
        if (array[i] == value)
            return i;
    return -1;
}

int
brhist_init(const lame_global_flags * gf, const int bitrate_kbps_min, const int bitrate_kbps_max)
{
    brhist.hist_printed_lines = 0;

    /* initialize histogramming data structure */
    lame_bitrate_kbps(gf, brhist.kbps);
    brhist.vbr_bitrate_min_index = calculate_index(brhist.kbps, BRHIST_WIDTH, bitrate_kbps_min);
    brhist.vbr_bitrate_max_index = calculate_index(brhist.kbps, BRHIST_WIDTH, bitrate_kbps_max);

    if (brhist.vbr_bitrate_min_index >= BRHIST_WIDTH ||
        brhist.vbr_bitrate_max_index >= BRHIST_WIDTH) {
        error_printf("lame internal error: VBR min %d kbps or VBR max %d kbps not allowed.\n",
                     bitrate_kbps_min, bitrate_kbps_max);
        return -1;
    }

    memset(brhist.bar_asterisk, '*', sizeof(brhist.bar_asterisk) - 1);
    memset(brhist.bar_percent, '%', sizeof(brhist.bar_percent) - 1);
    memset(brhist.bar_space, '-', sizeof(brhist.bar_space) - 1);
    memset(brhist.bar_coded, '-', sizeof(brhist.bar_space) - 1);

    return 0;
}

static int
digits(unsigned number)
{
    int     ret = 1;

    if (number >= 100000000) {
        ret += 8;
        number /= 100000000;
    }
    if (number >= 10000) {
        ret += 4;
        number /= 10000;
    }
    if (number >= 100) {
        ret += 2;
        number /= 100;
    }
    if (number >= 10) {
        ret += 1;
    }

    return ret;
}


static void
brhist_disp_line(int i, int br_hist_TOT, int br_hist_LR, int full, int frames)
{
    char    brppt[14];       /* [%] and max. 10 characters for kbps */
    int     barlen_TOT;
    int     barlen_LR;
    int     ppt = 0;
    int     res = digits(frames) + 3 + 4 + 1;

    if (full != 0) {
        /* some problems when br_hist_TOT \approx br_hist_LR: You can't see that there are still MS frames */
        barlen_TOT = (br_hist_TOT * (Console_IO.disp_width - res) + full - 1) / full; /* round up */
        barlen_LR = (br_hist_LR * (Console_IO.disp_width - res) + full - 1) / full; /* round up */
    }
    else {
        barlen_TOT = barlen_LR = 0;
    }

    if (frames > 0)
        ppt = (1000 * br_hist_TOT + frames / 2) / frames; /* round nearest */

    sprintf(brppt, " [%*i]", digits(frames), br_hist_TOT);

    if (Console_IO.str_clreoln[0]) /* ClearEndOfLine available */
        console_printf("\n%3d%s %.*s%.*s%s",
                       brhist.kbps[i], brppt,
                       barlen_LR, brhist.bar_percent,
                       barlen_TOT - barlen_LR, brhist.bar_asterisk, Console_IO.str_clreoln);
    else
        console_printf("\n%3d%s %.*s%.*s%*s",
                       brhist.kbps[i], brppt,
                       barlen_LR, brhist.bar_percent,
                       barlen_TOT - barlen_LR, brhist.bar_asterisk,
                       Console_IO.disp_width - res - barlen_TOT, "");

    brhist.hist_printed_lines++;
}



static void
progress_line(const lame_global_flags * gf, int full, int frames)
{
    char    rst[20] = "\0";
    int     barlen_TOT = 0, barlen_COD = 0, barlen_RST = 0;
    int     res = 1;
    float   time_in_sec = 0;
    unsigned int hour, min, sec;
    int     fsize = lame_get_framesize(gf);
    int     srate = lame_get_out_samplerate(gf);

    if (full < frames) {
        full = frames;
    }
    if (srate > 0) {
        time_in_sec = (float)(full - frames);
        time_in_sec *= fsize;
        time_in_sec /= srate;
    }
    hour = (unsigned int)(time_in_sec / 3600);
    time_in_sec -= hour * 3600;
    min = (unsigned int)(time_in_sec / 60);
    time_in_sec -= min * 60;
    sec = (unsigned int)time_in_sec;
    if (full != 0) {
        if (hour > 0) {
            sprintf(rst, "%*d:%02u:%02u", digits(hour), hour, min, sec);
            res += digits(hour) + 1 + 5;
        }
        else {
            sprintf(rst, "%02u:%02u", min, sec);
            res += 5;
        }
        /* some problems when br_hist_TOT \approx br_hist_LR: You can't see that there are still MS frames */
        barlen_TOT = (full * (Console_IO.disp_width - res) + full - 1) / full; /* round up */
        barlen_COD = (frames * (Console_IO.disp_width - res) + full - 1) / full; /* round up */
        barlen_RST = barlen_TOT - barlen_COD;
        if (barlen_RST == 0) {
            sprintf(rst, "%.*s", res - 1, brhist.bar_coded);
        }
    }
    else {
        barlen_TOT = barlen_COD = barlen_RST = 0;
    }
    if (Console_IO.str_clreoln[0]) { /* ClearEndOfLine available */
        console_printf("\n%.*s%s%.*s%s",
                       barlen_COD, brhist.bar_coded,
                       rst, barlen_RST, brhist.bar_space, Console_IO.str_clreoln);
    }
    else {
        console_printf("\n%.*s%s%.*s%*s",
                       barlen_COD, brhist.bar_coded,
                       rst, barlen_RST, brhist.bar_space, Console_IO.disp_width - res - barlen_TOT,
                       "");
    }
    brhist.hist_printed_lines++;
}


static int
stats_value(double x)
{
    if (x > 0.0) {
        console_printf(" %5.1f", x);
        return 6;
    }
    return 0;
}

static int
stats_head(double x, const char *txt)
{
    if (x > 0.0) {
        console_printf(txt);
        return 6;
    }
    return 0;
}


static void
stats_line(double *stat)
{
    int     n = 1;
    console_printf("\n   kbps     ");
    n += 12;
    n += stats_head(stat[1], "  mono");
    n += stats_head(stat[2], "   IS ");
    n += stats_head(stat[3], "   LR ");
    n += stats_head(stat[4], "   MS ");
    console_printf(" %%    ");
    n += 6;
    n += stats_head(stat[5], " long ");
    n += stats_head(stat[6], "switch");
    n += stats_head(stat[7], " short");
    n += stats_head(stat[8], " mixed");
    n += console_printf(" %%");
    if (Console_IO.str_clreoln[0]) { /* ClearEndOfLine available */
        console_printf("%s", Console_IO.str_clreoln);
    }
    else {
        console_printf("%*s", Console_IO.disp_width - n, "");
    }
    brhist.hist_printed_lines++;

    n = 1;
    console_printf("\n  %5.1f     ", stat[0]);
    n += 12;
    n += stats_value(stat[1]);
    n += stats_value(stat[2]);
    n += stats_value(stat[3]);
    n += stats_value(stat[4]);
    console_printf("      ");
    n += 6;
    n += stats_value(stat[5]);
    n += stats_value(stat[6]);
    n += stats_value(stat[7]);
    n += stats_value(stat[8]);
    if (Console_IO.str_clreoln[0]) { /* ClearEndOfLine available */
        console_printf("%s", Console_IO.str_clreoln);
    }
    else {
        console_printf("%*s", Console_IO.disp_width - n, "");
    }
    brhist.hist_printed_lines++;
}


/* Yes, not very good */
#define LR  0
#define MS  2

void
brhist_disp(const lame_global_flags * gf)
{
    int     i, lines_used = 0;
    int     br_hist[BRHIST_WIDTH]; /* how often a frame size was used */
    int     br_sm_hist[BRHIST_WIDTH][4]; /* how often a special frame size/stereo mode commbination was used */
    int     st_mode[4];
    int     bl_type[6];
    int     frames;          /* total number of encoded frames */
    int     most_often;      /* usage count of the most often used frame size, but not smaller than Console_IO.disp_width-BRHIST_RES (makes this sense?) and 1 */
    double  sum = 0.;

    double  stat[9] = { 0 };
    int     st_frames = 0;


    brhist.hist_printed_lines = 0; /* printed number of lines for the brhist functionality, used to skip back the right number of lines */

    lame_bitrate_stereo_mode_hist(gf, br_sm_hist);
    lame_bitrate_hist(gf, br_hist);
    lame_stereo_mode_hist(gf, st_mode);
    lame_block_type_hist(gf, bl_type);

    frames = most_often = 0;
    for (i = 0; i < BRHIST_WIDTH; i++) {
        frames += br_hist[i];
        sum += br_hist[i] * brhist.kbps[i];
        if (most_often < br_hist[i])
            most_often = br_hist[i];
        if (br_hist[i])
            ++lines_used;
    }

    for (i = 0; i < BRHIST_WIDTH; i++) {
        int     show = br_hist[i];
        show = show && (lines_used > 1);
        if (show || (i >= brhist.vbr_bitrate_min_index && i <= brhist.vbr_bitrate_max_index))
            brhist_disp_line(i, br_hist[i], br_sm_hist[i][LR], most_often, frames);
    }
    for (i = 0; i < 4; i++) {
        st_frames += st_mode[i];
    }
    if (frames > 0) {
        stat[0] = sum / frames;
        stat[1] = 100. * (frames - st_frames) / frames;
    }
    if (st_frames > 0) {
        stat[2] = 0.0;
        stat[3] = 100. * st_mode[LR] / st_frames;
        stat[4] = 100. * st_mode[MS] / st_frames;
    }
    if (bl_type[5] > 0) {
        stat[5] = 100. * bl_type[0] / bl_type[5];
        stat[6] = 100. * (bl_type[1] + bl_type[3]) / bl_type[5];
        stat[7] = 100. * bl_type[2] / bl_type[5];
        stat[8] = 100. * bl_type[4] / bl_type[5];
    }
    progress_line(gf, lame_get_totalframes(gf), frames);
    stats_line(stat);
}

void
brhist_jump_back(void)
{
    console_up(brhist.hist_printed_lines);
    brhist.hist_printed_lines = 0;
}

/*
 * 1)
 *
 * Taken from Termcap_Manual.html:
 *
 * With the Unix version of termcap, you must allocate space for the description yourself and pass
 * the address of the space as the argument buffer. There is no way you can tell how much space is
 * needed, so the convention is to allocate a buffer 2048 characters long and assume that is
 * enough.  (Formerly the convention was to allocate 1024 characters and assume that was enough.
 * But one day, for one kind of terminal, that was not enough.)
 */

#endif /* ifdef BRHIST */

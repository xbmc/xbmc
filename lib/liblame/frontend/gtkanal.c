/*
 *      GTK plotting routines source file
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

/* $Id: gtkanal.c,v 1.41.8.3 2009/01/18 15:44:28 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>

#include "main.h"
#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "lame-analysis.h"
#include "get_audio.h"
#include "gtkanal.h"
#include "gpkplotting.h"
#include "lame_global_flags.h"

/* this file should be removed. The few data items accessed in 'gfc'
   should be made accessable by writing a lame_set_variable() function */
#include "util.h"

#include "console.h"


#ifdef _WIN32
#  include <windows.h>
#  define msleep(t) Sleep(t)
#else
#  include <unistd.h>
#  define msleep(t) usleep((t) * 1000)
#endif




/*! Stringify \a x. */
#define STR(x)   #x
/*! Stringify \a x, perform macro expansion. */
#define XSTR(x)  STR(x)

#define MP3X_MAJOR_VERSION      0 /* Major version number */
#define MP3X_MINOR_VERSION     82 /* Minor version number */
#define MP3X_ALPHA_VERSION      0 /* Set number if this is an alpha version, otherwise zero */
#define MP3X_BETA_VERSION       0 /* Set number if this is a beta version, otherwise zero */


plotting_data *pinfo;
plotting_data *pplot;
plotting_data Pinfo[NUMPINFO];


/* global variables for the state of the system */
static gint idle_keepgoing;  /* processing of frames is ON */
static gint idle_count_max;  /* number of frames to process before plotting */
static gint idle_count;      /* pause & plot when idle_count=idel_count_max */
static gint idle_end = 0;    /* process all frames, stop at last frame  */
static gint idle_back = 0;   /* set when we are displaying the old data */
static int mp3done = 0;      /* last frame has been read */
static GtkWidget *frameprogress; /* progress bar */
static GtkWidget *framecounter; /* progress counter */

static int subblock_draw[3] = { 1, 1, 1 };

/* main window */
GtkWidget *window;
/* Backing pixmap for drawing areas */
GtkWidget *pcmbox;           /* PCM data plotted here */
GtkWidget *winbox;           /* mpg123 synthesis data plotted here */
GtkWidget *enerbox[2];       /* spectrum, gr=0,1 plotted here */
GtkWidget *mdctbox[2];       /* mdct coefficients gr=0,1 plotted here */
GtkWidget *sfbbox[2];        /* scalefactors gr=0,1 plotted here */
GtkWidget *headerbox;        /* mpg123 header info shown here */


struct gtkinfostruct {
    int     filetype;        /* input file type 0=WAV, 1=MP3 */
    int     msflag;          /* toggle between L&R vs M&S PCM data display */
    int     chflag;          /* toggle between L & R channels */
    int     kbflag;          /* toggle between wave # and barks */
    int     flag123;         /* show mpg123 frame info, OR ISO encoder frame info */
    double  avebits;         /* running average bits per frame */
    int     approxbits;      /* (approx) bits per frame */
    int     maxbits;         /* max bits per frame used so far */
    int     totemph;         /* total of frames with de-emphasis */
    int     totms;           /* total frames with ms_stereo */
    int     totis;           /* total frames with i_stereo */
    int     totshort;        /* total granules with short blocks */
    int     totmix;          /* total granules with mixed blocks */
    int     totpreflag;      /* total granules with preflag */
    int     pupdate;         /* plot while processing, or only when needed */
    int     sfblines;        /* plot scalefactor bands in MDCT plot */
    int     difference;      /* plot original - decoded instead of orig vs. decoded */
    int     totalframes;
} gtkinfo;


static lame_global_flags *gfp;
lame_internal_flags *gfc;
hip_t hip;

/**********************************************************************
 * read one frame and encode it
 **********************************************************************/
int
gtkmakeframe(void)
{
    int     iread = 0;
    static int init = 0;
    static int mpglag;
    static short int Buffer[2][1152];
    short int mpg123pcm[2][1152];
    int     ch, j;
    int     mp3count = 0;
    int     mp3out = 0;
    int     channels_out;
    unsigned char mp3buffer[LAME_MAXMP3BUFFER];
    static int frameNum = 0;
    int     framesize = lame_get_framesize(gfp);

    channels_out = (lame_get_mode(gfp) == MONO) ? 1 : 2;

    pinfo->frameNum = frameNum;
    pinfo->sampfreq = lame_get_out_samplerate(gfp);
    pinfo->framesize = framesize;
    pinfo->stereo = channels_out;

    /* If the analsys code is enabled, lame will writes data into gfc->pinfo,
     * and mpg123 will write data into pinfo.  Set these so
     * the libraries put this data in the right place: */
    gfc->pinfo = pinfo;
    hip_set_pinfo(hip, pinfo);

    if (is_mpeg_file_format(input_format)) {
        iread = get_audio16(gfp, Buffer);


        /* add a delay of framesize-DECDELAY, which will make the total delay
         * exactly one frame, so we can sync MP3 output with WAV input */
        for (ch = 0; ch < channels_out; ch++) {
            for (j = 0; j < framesize - DECDELAY; j++)
                pinfo->pcmdata2[ch][j] = pinfo->pcmdata2[ch][j + framesize];
            for (j = 0; j < framesize; j++) /*rescale from int to short int */
                pinfo->pcmdata2[ch][j + framesize - DECDELAY] = Buffer[ch][j];
        }

        pinfo->frameNum123 = frameNum - 1;
        ++frameNum;

    }
    else {

        /* feed data to encoder until encoder produces some output */
        while (lame_get_frameNum(gfp) == pinfo->frameNum) {

            if (!init) {
                init = 1;
                mpglag = 1;
                if (hip) {
                    hip_decode_exit(hip);
                }
                hip = hip_decode_init();
                hip_set_pinfo(hip, pinfo);
            }

            iread = get_audio16(gfp, Buffer);
            if (iread > framesize) {
                /* NOTE: frame analyzer requires that we encode one frame
                 * for each pass through this loop.  If lame_encode_buffer()
                 * is feed data too quickly, it will sometimes encode multiple frames
                 * breaking this loop.
                 */
                error_printf("Warning: get_audio is returning too much data.\n");
            }
            if (iread <= 0)
                break;  /* eof */

            mp3count = lame_encode_buffer(gfp, Buffer[0], Buffer[1], iread,
                                          mp3buffer, sizeof(mp3buffer));

            assert(!(mp3count > 0 && lame_get_frameNum(gfp) == pinfo->frameNum));
            /* not possible to produce mp3 data without encoding at least
             * one frame of data which would increment frameNum */
        }
        frameNum = lame_get_frameNum(gfp); /* use the internal MP3 frame counter */


        /* decode one frame of output */
        mp3out = hip_decode1(hip, mp3buffer, mp3count, mpg123pcm[0], mpg123pcm[1]); /* re-synthesis to pcm */
        /* mp3out = 0:  need more data to decode */
        /* mp3out = -1:  error.  Lets assume 0 pcm output */
        /* mp3out = number of samples output */
        if (mp3out > 0)
            assert(mp3out == pinfo->framesize);
        if (mp3out != 0) {
            /* decoded output is for frame pinfo->frameNum123
             * add a delay of framesize-DECDELAY, which will make the total delay
             * exactly one frame */
            pinfo->frameNum123 = pinfo->frameNum - mpglag;
            for (ch = 0; ch < pinfo->stereo; ch++) {
                for (j = 0; j < pinfo->framesize - DECDELAY; j++)
                    pinfo->pcmdata2[ch][j] = pinfo->pcmdata2[ch][j + pinfo->framesize];
                for (j = 0; j < pinfo->framesize; j++) {
                    pinfo->pcmdata2[ch][j + pinfo->framesize - DECDELAY] =
                        (mp3out == -1) ? 0 : mpg123pcm[ch][j];
                }
            }
        }
        else {
            if (mpglag == MAXMPGLAG) {
                error_printf("READ_AHEAD set too low - not enough frame buffering.\n"
                             "MP3x display of input and output PCM data out of sync.\n");
                error_flush();
            }
            else
                mpglag++;
            pinfo->frameNum123 = -1; /* no frame output */
        }
    }
    return iread;
}


void
plot_frame(void)
{
    int     i, j, n, ch, gr;
    gdouble *xcord, *ycord;
    gdouble xmx, xmn, ymx, ymn;
    double *data, *data2, *data3;
    char    title2[80];
    char    label[80], label2[80];
    char   *title;
    plotting_data *pplot1;
    plotting_data *pplot2 = NULL;

    double  en, samp;
    int     sampindex, version = 0;
    int     barthick;
    static int firstcall = 1;
    static GdkColor *barcolor, *color, *grcolor[2];
    static GdkColor yellow, gray, cyan, magenta, orange, pink, red, green, blue, black, oncolor,
        offcolor;
    int     blocktype[2][2];
    int     headbits;
    int     mode_gr = 2;

    /* find the frame where mpg123 produced output coming from input frame
     * pinfo.  i.e.:   out_frame + out_frame_lag = input_frame  */
    for (i = 1; i <= MAXMPGLAG; i++) {
        if ((pplot - i)->frameNum123 == pplot->frameNum) {
            pplot2 = pplot - i;
            break;
        }
    }
    if (i > MAXMPGLAG) {
        error_printf("input/output pcm syncing problem.  should not happen!\n");
        pplot2 = pplot - 1;
    }


    /* however, the PCM data is delayed by 528 samples in the encoder filterbanks.
     * We added another 1152-528 delay to this so the PCM data is *exactly* one
     * frame behind the header & MDCT information */
    pplot1 = pplot2 + 1; /* back one frame for header info, MDCT */

    /* allocate these GC's only once */
    if (firstcall) {
        firstcall = 0;
        /*    grcolor[0] = &magenta; */
        grcolor[0] = &blue;
        grcolor[1] = &green;
        barcolor = &gray;

        setcolor(headerbox, &oncolor, 255, 0, 0);
        setcolor(headerbox, &offcolor, 175, 175, 175);
        setcolor(pcmbox, &red, 255, 0, 0);
        setcolor(pcmbox, &pink, 255, 0, 255);
        setcolor(pcmbox, &magenta, 255, 0, 100);
        setcolor(pcmbox, &orange, 255, 127, 0);
        setcolor(pcmbox, &cyan, 0, 255, 255);
        setcolor(pcmbox, &green, 0, 255, 0);
        setcolor(pcmbox, &blue, 0, 0, 255);
        setcolor(pcmbox, &black, 0, 0, 0);
        setcolor(pcmbox, &gray, 100, 100, 100);
        setcolor(pcmbox, &yellow, 255, 255, 0);

    }

  /*******************************************************************
   * frame header info
   *******************************************************************/
    if (pplot1->sampfreq)
        samp = pplot1->sampfreq;
    else
        samp = 1;
    sampindex = SmpFrqIndex((long) samp, &version);

    ch = gtkinfo.chflag;

    headbits = 32 + ((pplot1->stereo == 2) ? 256 : 136);
    gtkinfo.approxbits = (pplot1->bitrate * 1000 * 1152.0 / samp) - headbits;
    sprintf(title2, "%3.1fkHz %ikbs ", samp / 1000, pplot1->bitrate);
    gtk_text_freeze(GTK_TEXT(headerbox));
    gtk_text_backward_delete(GTK_TEXT(headerbox), gtk_text_get_length(GTK_TEXT(headerbox)));
    gtk_text_set_point(GTK_TEXT(headerbox), 0);
    gtk_text_insert(GTK_TEXT(headerbox), NULL, &oncolor, NULL, title2, -1);
    title = " mono ";
    if (2 == pplot1->stereo)
        title = pplot1->js ? " js " : " s ";
    gtk_text_insert(GTK_TEXT(headerbox), NULL, &oncolor, NULL, title, -1);
    color = pplot1->ms_stereo ? &oncolor : &offcolor;
    gtk_text_insert(GTK_TEXT(headerbox), NULL, color, NULL, "ms ", -1);
    color = pplot1->i_stereo ? &oncolor : &offcolor;
    gtk_text_insert(GTK_TEXT(headerbox), NULL, color, NULL, "is ", -1);

    color = pplot1->crc ? &oncolor : &offcolor;
    gtk_text_insert(GTK_TEXT(headerbox), NULL, color, NULL, "crc ", -1);
    color = pplot1->padding ? &oncolor : &offcolor;
    gtk_text_insert(GTK_TEXT(headerbox), NULL, color, NULL, "pad ", -1);

    color = pplot1->emph ? &oncolor : &offcolor;
    gtk_text_insert(GTK_TEXT(headerbox), NULL, color, NULL, "em ", -1);

    sprintf(title2, "bv=%i,%i ", pplot1->big_values[0][ch], pplot1->big_values[1][ch]);
    gtk_text_insert(GTK_TEXT(headerbox), NULL, &black, NULL, title2, -1);

    color = pplot1->scfsi[ch] ? &oncolor : &offcolor;
    sprintf(title2, "scfsi=%i            ", pplot1->scfsi[ch]);
    gtk_text_insert(GTK_TEXT(headerbox), NULL, color, NULL, title2, -1);
    if (gtkinfo.filetype)
        sprintf(title2, " mdb=%i %i/NA", pplot1->maindata, pplot1->totbits);
    else
        sprintf(title2, " mdb=%i   %i/%i",
                pplot1->maindata, pplot1->totbits, pplot1->totbits + pplot->resvsize);
    gtk_text_insert(GTK_TEXT(headerbox), NULL, &oncolor, NULL, title2, -1);
    gtk_text_thaw(GTK_TEXT(headerbox));



  /*******************************************************************
   * block type
   *******************************************************************/
    for (gr = 0; gr < mode_gr; gr++)
        if (gtkinfo.flag123)
            blocktype[gr][ch] = pplot1->mpg123blocktype[gr][ch];
        else
            blocktype[gr][ch] = pplot->blocktype[gr][ch];


  /*******************************************************************
   * draw the PCM data *
   *******************************************************************/
    n = 1600;           /* PCM frame + FFT window:   224 + 1152 + 224  */
    xcord = g_malloc(n * sizeof(gdouble));
    ycord = g_malloc(n * sizeof(gdouble));


    if (gtkinfo.msflag)
        title = ch ? "Side Channel" : "Mid Channel";
    else
        title = ch ? "Right Channel" : "Left Channel";

    sprintf(title2, "%s  mask_ratio=%3.2f  %3.2f  ener_ratio=%3.2f  %3.2f",
            title,
            pplot->ms_ratio[0], pplot->ms_ratio[1],
            pplot->ms_ener_ratio[0], pplot->ms_ener_ratio[1]);


    ymn = -32767;
    ymx = 32767;
    xmn = 0;
    xmx = 1600 - 1;

    /*  0  ... 224      draw in black, connecting to 224 pixel
     * 1375 .. 1599     draw in black  connecting to 1375 pixel
     * 224 ... 1375     MP3 frame.  draw in blue
     */

    /* draw the title */
    gpk_graph_draw(pcmbox, 0, xcord, ycord, xmn, ymn, xmx, ymx, 1, title2, &black);


    /* draw some hash marks dividing the frames */
    ycord[0] = ymx * .8;
    ycord[1] = ymn * .8;
    for (gr = 0; gr <= 2; gr++) {
        xcord[0] = 223.5 + gr * 576;
        xcord[1] = 223.5 + gr * 576;
        gpk_rectangle_draw(pcmbox, xcord, ycord, xmn, ymn, xmx, ymx, &yellow);
    }
    for (gr = 0; gr < mode_gr; gr++) {
        if (blocktype[gr][ch] == 2)
            for (i = 1; i <= 2; i++) {
                xcord[0] = 223.5 + gr * 576 + i * 192;
                xcord[1] = 223.5 + gr * 576 + i * 192;
                gpk_rectangle_draw(pcmbox, xcord, ycord, xmn, ymn, xmx, ymx, &yellow);
            }
    }
    /* bars representing FFT windows */
    xcord[0] = 0;
    ycord[0] = ymn + 3000;
    xcord[1] = 1024 - 1;
    ycord[1] = ymn + 1000;
    gpk_rectangle_draw(pcmbox, xcord, ycord, xmn, ymn, xmx, ymx, grcolor[0]);
    xcord[0] = 576;
    ycord[0] = ymn + 2000;
    xcord[1] = 576 + 1024 - 1;
    ycord[1] = ymn;
    gpk_rectangle_draw(pcmbox, xcord, ycord, xmn, ymn, xmx, ymx, grcolor[1]);


    /* plot PCM data */
    for (i = 0; i < n; i++) {
        xcord[i] = i;
        if (gtkinfo.msflag)
            ycord[i] = ch ? .5 * (pplot->pcmdata[0][i] - pplot->pcmdata[1][i]) :
                .5 * (pplot->pcmdata[0][i] + pplot->pcmdata[1][i]);
        else
            ycord[i] = pplot->pcmdata[ch][i];
    }

    /* skip plot if we are doing an mp3 file */
    if (!gtkinfo.filetype) {
        gpk_graph_draw(pcmbox, n, xcord, ycord, xmn, ymn, xmx, ymx, 0, title2, &black);
    }


  /*******************************************************************/
    /* draw the PCM re-synthesis data */
  /*******************************************************************/
    n = 1152;
    /*
       sprintf(title2,"Re-synthesis  mask_ratio=%3.2f  %3.2f  ener_ratio=%3.2f  %3.2f",
       pplot->ms_ratio[0],pplot->ms_ratio[1],
       pplot->ms_ener_ratio[0],pplot->ms_ener_ratio[1]);
     */
    title = "Re-synthesis";
    if (gtkinfo.difference)
        title = "Re-synthesis difference (amplified 20db)";


    ymn = -32767;
    ymx = 32767;
    xmn = 0;
    xmx = 1600 - 1;
    gpk_graph_draw(winbox, 0, xcord, ycord, xmn, ymn, xmx, ymx, 1, title, &black);
    /* draw some hash marks dividing the frames */
    ycord[0] = ymx * .8;
    ycord[1] = ymn * .8;
    for (gr = 0; gr <= 2; gr++) {
        xcord[0] = 223.5 + gr * 576;
        xcord[1] = 223.5 + gr * 576;
        gpk_rectangle_draw(winbox, xcord, ycord, xmn, ymn, xmx, ymx, &yellow);
    }
    for (gr = 0; gr < 2; gr++) {
        if (blocktype[gr][ch] == 2)
            for (i = 1; i <= 2; i++) {
                xcord[0] = 223.5 + gr * 576 + i * 192;
                xcord[1] = 223.5 + gr * 576 + i * 192;
                gpk_rectangle_draw(winbox, xcord, ycord, xmn, ymn, xmx, ymx, &yellow);
            }
    }

    /* this piece of PCM data from previous frame */
    n = 224;
    for (j = 1152 - n, i = 0; i < n; i++, j++) {
        xcord[i] = i;
        if (gtkinfo.msflag)
            ycord[i] = ch ? .5 * (pplot1->pcmdata2[0][j] -
                                  pplot1->pcmdata2[1][j]) :
                .5 * (pplot1->pcmdata2[0][j] + pplot1->pcmdata2[1][j]);
        else
            ycord[i] = pplot1->pcmdata2[ch][j];
    }

    /* this piece of PCM data from current frame */
    n = 1152;
    for (i = 0; i < n; i++) {
        xcord[i + 224] = i + 224;
        if (gtkinfo.msflag)
            ycord[i + 224] = ch ? .5 * (pplot2->pcmdata2[0][i] - pplot2->pcmdata2[1][i]) :
                .5 * (pplot2->pcmdata2[0][i] + pplot2->pcmdata2[1][i]);
        else
            ycord[i + 224] = pplot2->pcmdata2[ch][i];
    }

    n = 1152 + 224;
    if (gtkinfo.difference) {
        for (i = 0; i < n; i++) {
            if (gtkinfo.msflag)
                ycord[i] -= ch ? .5 * (pplot->pcmdata[0][i] - pplot->pcmdata[1][i]) :
                    .5 * (pplot->pcmdata[0][i] + pplot->pcmdata[1][i]);
            else
                ycord[i] -= pplot->pcmdata[ch][i];
        }
        ycord[i] *= 100;
    }


    gpk_graph_draw(winbox, n, xcord, ycord, xmn, ymn, xmx, ymx, 0, title, &black);





  /*******************************************************************/
    /* draw the MDCT energy spectrum */
  /*******************************************************************/
    for (gr = 0; gr < mode_gr; gr++) {
        int     bits, bits2;
        char   *blockname = "";
        switch (blocktype[gr][ch]) {
        case 0:
            blockname = "normal";
            break;
        case 1:
            blockname = "start";
            break;
        case 2:
            blockname = "short";
            break;
        case 3:
            blockname = "end";
            break;
        }
        strcpy(label, blockname);
        if (pplot1->mixed[gr][ch])
            strcat(label, "(mixed)");




        n = 576;
        if (gtkinfo.flag123) {
            data = pplot1->mpg123xr[gr][0];
            data2 = pplot1->mpg123xr[gr][1];
        }
        else {
            data = pplot->xr[gr][0];
            data2 = pplot->xr[gr][1];
        }


        xmn = 0;
        xmx = n - 1;
        ymn = 0;
        ymx = 11;

        /* draw title, erase old plot */
        if (gtkinfo.flag123) {
            bits = pplot1->mainbits[gr][ch];
            bits2 = pplot1->sfbits[gr][ch];
        }
        else {
            bits = pplot->LAMEmainbits[gr][ch];
            bits2 = pplot->LAMEsfbits[gr][ch];
        }
        sprintf(title2, "MDCT%1i(%s) bits=%i/%i ", gr, label, bits, bits2);
        gpk_bargraph_draw(mdctbox[gr], 0, xcord, ycord, xmn, ymn, xmx, ymx, 1, title2, 0, barcolor);

        /* draw some hash marks showing scalefactor bands */
        if (gtkinfo.sfblines) {
            int     fac, nsfb, *scalefac;
            if (blocktype[gr][ch] == SHORT_TYPE) {
                nsfb = SBMAX_s;
                i = nsfb - 7;
                fac = 3;
                scalefac = gfc->scalefac_band.s;
            }
            else {
                nsfb = SBMAX_l;
                i = nsfb - 10;
                fac = 1;
                scalefac = gfc->scalefac_band.l;
            }
            for (; i < nsfb; i++) {
                ycord[0] = .8 * ymx;
                ycord[1] = ymn;
                xcord[0] = fac * scalefac[i];
                xcord[1] = xcord[0];
                gpk_rectangle_draw(mdctbox[gr], xcord, ycord, xmn, ymn, xmx, ymx, &yellow);
            }
        }



        ymn = 9e20;
        ymx = -9e20;
        for (i = 0; i < n; i++) {
            double  coeff;
            xcord[i] = i;
            if (gtkinfo.msflag) {
                coeff = ch ? .5 * (data[i] - data2[i]) : .5 * (data[i] + data2[i]);
            }
            else {
                coeff = ch ? data2[i] : data[i];
            }
            if (blocktype[gr][ch] == SHORT_TYPE && !subblock_draw[i % 3])
                coeff = 0;
            ycord[i] = coeff * coeff * 1e10;
            ycord[i] = log10(MAX(ycord[i], (double) 1));

#if 0
            if (ch == 0)
                if (i == 26)
                    if (data[i] != 0)
                        console_printf("%i %i i=%i  mdct: (db) %f  %f \n", pplot->frameNum, gr, i,
                                       10 * log10(data[i] * data[i]),
                                       10 * log10(.33 *
                                                  (data[i - 1] * data[i - 1] + data[i] * data[i] +
                                                   data[i + 1] * data[i + 1]))
                            );
#endif

            ymx = (ycord[i] > ymx) ? ycord[i] : ymx;
            ymn = (ycord[i] < ymn) ? ycord[i] : ymn;
        }
        /*  print the min/max
           sprintf(title2,"MDCT%1i %5.2f %5.2f  bits=%i",gr,ymn,ymx,
           pplot1->mainbits[gr][ch]);
         */
        if (gtkinfo.flag123)
            bits = pplot1->mainbits[gr][ch];
        else
            bits = pplot->LAMEmainbits[gr][ch];


        sprintf(title2, "MDCT%1i(%s) bits=%i ", gr, label, bits);

        xmn = 0;
        xmx = n - 1;
        ymn = 0;
        ymx = 11;
        gpk_bargraph_draw(mdctbox[gr], n, xcord, ycord, xmn, ymn, xmx, ymx, 0, title2, 0, barcolor);
    }




  /*******************************************************************
   * draw the psy model energy spectrum (k space)
   * l3psy.c computes pe, en, thm for THIS granule.
   *******************************************************************/
    if (gtkinfo.kbflag) {
        for (gr = 0; gr < mode_gr; gr++) {
            n = HBLKSIZE; /* only show half the spectrum */

            data = &pplot->energy[gr][ch][0];

            ymn = 9e20;
            ymx = -9e20;
            for (i = 0; i < n; i++) {
                xcord[i] = i + 1;
                if (blocktype[gr][ch] == SHORT_TYPE && !subblock_draw[i % 3])
                    ycord[i] = 0;
                else
                    ycord[i] = log10(MAX(data[i], (double) 1));
                ymx = (ycord[i] > ymx) ? ycord[i] : ymx;
                ymn = (ycord[i] < ymn) ? ycord[i] : ymn;
            }
            for (en = 0, j = 0; j < BLKSIZE; j++)
                en += pplot->energy[gr][ch][j];

            sprintf(title2, "FFT%1i  pe=%5.2fK  en=%5.2e ", gr, pplot->pe[gr][ch] / 1000, en);

            ymn = 3;
            ymx = 15;
            xmn = 1;
            xmx = n;
            gpk_bargraph_draw(enerbox[gr], n, xcord, ycord,
                              xmn, ymn, xmx, ymx, 1, title2, 0, barcolor);

        }
    }
    else {
    /*******************************************************************
     * draw the psy model energy spectrum (scalefactor bands)
     *******************************************************************/
        for (gr = 0; gr < mode_gr; gr++) {

            if (blocktype[gr][ch] == 2) {
                n = 3 * SBMAX_s;
                data = &pplot->en_s[gr][ch][0];
                data2 = &pplot->thr_s[gr][ch][0];
                data3 = &pplot->xfsf_s[gr][ch][0];
            }
            else {
                n = SBMAX_l;
                data = &pplot->en[gr][ch][0];
                data2 = &pplot->thr[gr][ch][0];
                data3 = &pplot->xfsf[gr][ch][0];
            }
            ymn = 9e20;
            ymx = -9e20;
            for (i = 0; i < n; i++) {
                xcord[i] = i + 1;
                if (blocktype[gr][ch] == SHORT_TYPE && !subblock_draw[i % 3])
                    ycord[i] = 0;
                else
                    ycord[i] = log10(MAX(data[i], (double) 1));
                /*
                   ymx=(ycord[i] > ymx) ? ycord[i] : ymx;
                   ymn=(ycord[i] < ymn) ? ycord[i] : ymn;
                 */
            }



            /* en = max energy difference amoung the 3 short FFTs for this granule */
            en = pplot->ers[gr][ch];
            if (en > 999)
                en = 999;
            sprintf(title2,
                    "FFT%1i pe=%5.2fK/%3.1f \nnoise ovr_b:%i/max:%3.1f/ovr:%3.1f/tot:%3.1f/ssd:%i",
                    gr, pplot->pe[gr][ch] / 1000, en, pplot->over[gr][ch], pplot->max_noise[gr][ch],
                    pplot->over_noise[gr][ch], pplot->tot_noise[gr][ch], pplot->over_SSD[gr][ch]);

            barthick = 3;
            if (blocktype[gr][ch] == SHORT_TYPE)
                barthick = 2;
            if (!(subblock_draw[0] && subblock_draw[1] && subblock_draw[2]))
                barthick = 3;

            ymn = 3;
            ymx = 15;
            xmn = 1;
            xmx = n + 1; /* a little extra because of the bar thickness */
            gpk_bargraph_draw(enerbox[gr], n, xcord, ycord,
                              xmn, ymn, xmx, ymx, 1, title2, barthick, barcolor);

            for (i = 0; i < n; i++) {
                xcord[i] = i + 1 + .20;
                if (blocktype[gr][ch] == SHORT_TYPE && !subblock_draw[i % 3])
                    ycord[i] = 0;
                else
                    ycord[i] = log10(MAX(data2[i], (double) 1));
            }

            gpk_bargraph_draw(enerbox[gr], n, xcord, ycord,
                              xmn, ymn, xmx, ymx, 0, title2, barthick, grcolor[gr]);

            for (i = 0; i < n; i++) {
                xcord[i] = i + 1 + .40;
                if (blocktype[gr][ch] == SHORT_TYPE && !subblock_draw[i % 3])
                    ycord[i] = 0;
                else
                    ycord[i] = log10(MAX(data3[i], (double) 1));
            }
            gpk_bargraph_draw(enerbox[gr], n, xcord, ycord,
                              xmn, ymn, xmx, ymx, 0, title2, barthick, &red);

        }
    }

  /*******************************************************************
   * draw scalefactors
   *******************************************************************/
    for (gr = 0; gr < mode_gr; gr++) {
        int     ggain;
        if (blocktype[gr][ch] == 2) {
            n = 3 * SBMAX_s;
            if (gtkinfo.flag123)
                data = pplot1->sfb_s[gr][ch];
            else
                data = pplot->LAMEsfb_s[gr][ch];
        }
        else {
            n = SBMAX_l;
            if (gtkinfo.flag123)
                data = pplot1->sfb[gr][ch];
            else
                data = pplot->LAMEsfb[gr][ch];
        }

        ymn = -1;
        ymx = 10;
        for (i = 0; i < n; i++) {
            xcord[i] = i + 1;
            if (blocktype[gr][ch] == SHORT_TYPE && !subblock_draw[i % 3])
                ycord[i] = 0;
            else
                ycord[i] = -data[i];

            ymx = (ycord[i] > ymx - 2) ? ycord[i] + 2 : ymx;
            ymn = (ycord[i] < ymn) ? ycord[i] - 1 : ymn;
        }

        if (blocktype[gr][ch] == 2) {
            sprintf(label2,
                    "SFB scale=%i preflag=%i  %i%i%i",
                    pplot1->scalefac_scale[gr][ch],
                    pplot1->preflag[gr][ch],
                    pplot1->sub_gain[gr][ch][0],
                    pplot1->sub_gain[gr][ch][1], pplot1->sub_gain[gr][ch][2]);
        }
        else {
            sprintf(label2, "SFB scale=%i preflag=%i", pplot1->scalefac_scale[gr][ch],
                    pplot1->preflag[gr][ch]);
        }

        if (gtkinfo.flag123)
            ggain = (pplot1->qss[gr][ch]);
        else
            ggain = (pplot->LAMEqss[gr][ch]);

        sprintf(title2, " ggain=%i", ggain);
        strcat(label2, title2);

        xmn = 1;
        xmx = n + 1;
        gpk_bargraph_draw(sfbbox[gr], n, xcord, ycord,
                          xmn, ymn, xmx, ymx, 1, label2, 0, grcolor[gr]);

        ycord[0] = ycord[1] = 0;
        xcord[0] = 1;
        xcord[1] = n + 1;
        gpk_rectangle_draw(sfbbox[gr], xcord, ycord, xmn, ymn, xmx, ymx, &yellow);


    }


}



static void
update_progress(void)
{
    char    label[80];

    int     tf = lame_get_totalframes(gfp);
    if (gtkinfo.totalframes > 0)
        tf = gtkinfo.totalframes;

    sprintf(label, "Frame:%4i/%4i  %6.2fs", pplot->frameNum, (int) tf - 1, pplot->frametime);
    gtk_progress_set_value(GTK_PROGRESS(frameprogress), (gdouble) pplot->frameNum);
    gtk_label_set_text(GTK_LABEL(framecounter), label);
}



static void
analyze(void)
{
    if (idle_keepgoing) {
        idle_count = 0;
        idle_count_max = 0;
        idle_keepgoing = 0;
        idle_end = 0;
    }
    plot_frame();
    update_progress();
}

static void
plotclick(GtkWidget * widget, gpointer data)
{
    analyze();
}




static int
frameadv1(GtkWidget * widget, gpointer data)
{
    int     i;
    if (idle_keepgoing) {
        if (idle_back) {
            /* frame displayed is the old frame.  to advance, just swap in new frame */
            idle_back--;
            pplot = &Pinfo[READ_AHEAD + idle_back];
        }
        else {
            /* advance the frame by reading in a new frame */
            pplot = &Pinfo[READ_AHEAD];
            if (mp3done) {
                /* dont try to read any more frames, and quit if "finish MP3" was selected */
                /* if (idle_finish) gtk_main_quit(); */
                idle_count_max = 0;
                idle_end = 0;
            }
            else {
                /* read in the next frame */
                for (i = NUMPINFO - 1; i > 0; i--)
                    memcpy(&Pinfo[i], &Pinfo[i - 1], sizeof(plotting_data));
                pinfo = &Pinfo[0];
                pinfo->num_samples = gtkmakeframe();
                if (pinfo->num_samples == 0 && gtkinfo.totalframes == 0)
                    /* allow an extra frame to flush decoder buffers */
                    gtkinfo.totalframes = pinfo->frameNum + 2;

                if (pinfo->sampfreq)
                    pinfo->frametime = (pinfo->frameNum) * 1152.0 / pinfo->sampfreq;
                else
                    pinfo->frametime = 0;

                /* eof?
                   if (!pinfo->num_samples) if (idle_finish) gtk_main_quit();
                 */

                pinfo->totbits = 0;
                {
                    int     gr, ch;
                    for (gr = 0; gr < 2; gr++)
                        for (ch = 0; ch < 2; ch++) {
                            gtkinfo.totshort += (pinfo->mpg123blocktype[gr][ch] == 2);
                            gtkinfo.totmix += !(pinfo->mixed[gr][ch] == 0);
                            gtkinfo.totpreflag += (pinfo->preflag[gr][ch] == 1);
                            pinfo->totbits += pinfo->mainbits[gr][ch];
                        }
                }
                if (pinfo->frameNum > 0) /* start averaging at second frame */
                    gtkinfo.avebits = (gtkinfo.avebits * ((pinfo->frameNum) - 1)
                                       + pinfo->totbits) / (pinfo->frameNum);

                gtkinfo.maxbits = MAX(gtkinfo.maxbits, pinfo->totbits);
                gtkinfo.totemph += !(pinfo->emph == 0);
                gtkinfo.totms += !(pinfo->ms_stereo == 0);
                gtkinfo.totis += !(pinfo->i_stereo == 0);

                if (gtkinfo.totalframes > 0)
                    if (pplot->frameNum >= gtkinfo.totalframes - 1)
                        mp3done = 1;
            }
        }

        idle_count++;
        if (gtkinfo.pupdate)
            plot_frame();
        update_progress();
        if ((idle_count >= idle_count_max) && (!idle_end))
            analyze();
    }
    else {
        /*no processing to do, sleep in order to not monopolize CPU */
        msleep(10);
    }
    return 1;
}


static void
frameadv(GtkWidget * widget, gpointer data)
{
    int     adv;

    if (!strcmp((char *) data, "-1")) {
        /* ignore if we've already gone back as far as possible */
        if (pplot->frameNum == 0 || (idle_back == NUMBACK))
            return;
        idle_back++;
        pplot = &Pinfo[READ_AHEAD + idle_back];
        analyze();
        return;
    }


    adv = 1;
    if (!strcmp((char *) data, "1"))
        adv = 1;
    if (!strcmp((char *) data, "10"))
        adv = 10;
    if (!strcmp((char *) data, "100"))
        adv = 100;
    if (!strcmp((char *) data, "finish"))
        idle_end = 1;


    if (idle_keepgoing) {
        /* already running - que up additional frame advance requests */
        idle_count_max += adv;
    }
    else {
        /* turn on idleing */
        idle_count_max = adv;
        idle_count = 0;
        idle_keepgoing = 1;
    }
}




/* another callback */
static void
delete_event(GtkWidget * widget, GdkEvent * event, gpointer data)
{
    /* set MP3 done flag in case the File/Quit menu item has been selected */
    mp3done = 1;

    gtk_main_quit();
}







static void
channel_option(GtkWidget * widget, gpointer data)
{
    long    option;
    option = (long) data;
    switch (option) {
    case 1:
        gtkinfo.msflag = 0;
        gtkinfo.chflag = 0;
        break;
    case 2:
        gtkinfo.msflag = 0;
        gtkinfo.chflag = 1;
        break;
    case 3:
        gtkinfo.msflag = 1;
        gtkinfo.chflag = 0;
        break;
    case 4:
        gtkinfo.msflag = 1;
        gtkinfo.chflag = 1;
    }
    analyze();
}
static void
spec_option(GtkWidget * widget, gpointer data)
{
    long    option;
    option = (long) data;
    switch (option) {
    case 1:
        gtkinfo.kbflag = 0;
        break;
    case 2:
        gtkinfo.kbflag = 1;
        break;
    case 3:
        gtkinfo.flag123 = 0;
        break;
    case 4:
        gtkinfo.flag123 = 1;
        break;
    case 5:
        gtkinfo.pupdate = 1;
        break;
    case 6:
        gtkinfo.pupdate = 0;
        break;
    case 7:
        gtkinfo.sfblines = !gtkinfo.sfblines;
        break;
    case 8:
        gtkinfo.difference = !gtkinfo.difference;
        break;
    }
    analyze();
}

static  gint
key_press_event(GtkWidget * widget, GdkEventKey * event)
{
    /* is a switch() statement in lame forbidden? */
    if (event->keyval == '1') {
        subblock_draw[0] = 1;
        subblock_draw[1] = 0;
        subblock_draw[2] = 0;
        analyze();
    }
    else if (event->keyval == '2') {
        subblock_draw[0] = 0;
        subblock_draw[1] = 1;
        subblock_draw[2] = 0;
        analyze();
    }
    else if (event->keyval == '3') {
        subblock_draw[0] = 0;
        subblock_draw[1] = 0;
        subblock_draw[2] = 1;
        analyze();
    }
    else if (event->keyval == '0') {
        subblock_draw[0] = 1;
        subblock_draw[1] = 1;
        subblock_draw[2] = 1;
        analyze();
    }
    /* analyze(); */ /* dont redraw entire window for every key! */
    return 0;
}


/*! Get the mp3x version string. */
/*!
  \param void
  \return a pointer to a string which describes the version of mp3x.
*/
const char *
get_mp3x_version(void)
{
#if   MP3X_ALPHA_VERSION > 0
    static /*@observer@ */ const char *const str =
        XSTR(MP3X_MAJOR_VERSION) "." XSTR(MP3X_MINOR_VERSION)
        " (alpha " XSTR(MP3X_ALPHA_VERSION) ", " __DATE__ " " __TIME__ ")";
#elif MP3X_BETA_VERSION > 0
    static /*@observer@ */ const char *const str =
        XSTR(MP3X_MAJOR_VERSION) "." XSTR(MP3X_MINOR_VERSION)
        " (beta " XSTR(MP3X_BETA_VERSION) ", " __DATE__ ")";
#else
    static /*@observer@ */ const char *const str =
        XSTR(MP3X_MAJOR_VERSION) "." XSTR(MP3X_MINOR_VERSION);
#endif

    return str;
}


static void
text_window(GtkWidget * widget, gpointer data)
{
    long    option;
    GtkWidget *hbox, *vbox, *button, *box;
    GtkWidget *textwindow, *vscrollbar;
    char    text[256];

    option = (long) data;

    textwindow = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(textwindow));

    gtk_container_set_border_width(GTK_CONTAINER(textwindow), 0);
    vbox = gtk_vbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);

    button = gtk_button_new_with_label("close");
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
                              GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(textwindow));

    box = gtk_text_new(NULL, NULL);
    gtk_text_set_editable(GTK_TEXT(box), FALSE);
    vscrollbar = gtk_vscrollbar_new(GTK_TEXT(box)->vadj);


    switch (option) {
    case 0:
        gtk_window_set_title(GTK_WINDOW(textwindow), "Documentation");
        gtk_widget_set_usize(box, 450, 500);
        gtk_text_set_word_wrap(GTK_TEXT(box), TRUE);
        /* text should be moved outside this function, may be in a separate file */
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL,
                        "Frame header information: "
                        "First the bitrate, sampling frequency and mono, stereo or jstereo "
                        "indicators are displayed .  If the bitstream is jstereo, then mid/side "
                        "stereo or intensity stereo may be on (indicated in red).  If "
                        "de-emphasis is used, this is also indicated in red.  The mdb value is "
                        "main_data_begin.  The encoded data starts this many bytes *before* the "
                        "frame header.  A large value of mdb means the bitstream has saved some "
                        "bits into the reservoir, which it may allocate for some future frame. "
                        "The two numbers after mdb are the size (in bits) used to encode the "
                        "MDCT coefficients for this frame, followed byt the size of the bit "
                        "resevoir before encoding this frame.  The maximum frame size and a "
                        "running average are given in the Stats pull down menu.  A large "
                        "maximum frame size indicates the bitstream has made use of the bit "
                        "reservoir. \n\n", -1);

        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL,
                        "PCM data (top graph): "
                        "The PCM data is plotted in black.  The layer3 frame is divided into 2 "
                        "granules of 576 samples (marked with yellow vertical lines).  In the "
                        "case of normal, start and stop blocks, the MDCT coefficients for each "
                        "granule are computed using a 1152 sample window centered over the "
                        "granule.  In the case of short blocks, the granule is further divided "
                        "into 3 blocks of 192 samples (also marked with yellow vertical lines)."
                        "The MDCT coefficients for these blocks are computed using 384 sample "
                        "windows centered over the 192 sample window.  (This info not available "
                        "when analyzing .mp3 files.)  For the psycho-acoustic model, a windowed "
                        "FFT is computed for each granule.  The range of these windows "
                        "is denoted by the blue and green bars.\n\n", -1);

        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL,
                        "PCM re-synthesis data (second graph): "
                        "Same as the PCM window described above.  The data displayed is the "
                        "result of encoding and then decoding the original sample. \n\n", -1);

        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL,
                        "MDCT windows: "
                        "Shows the energy in the MDCT spectrum for granule 0 (left window) "
                        "and granule 1 (right window).  The text also shows the blocktype "
                        "used, the number of bits used to encode the coefficients and the "
                        "number of extra bits allocated from the reservoir.  The MDCT pull down "
                        "window will toggle between the original unquantized MDCT coefficients "
                        "and the compressed (quantized) coefficients.\n\n", -1);

        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL,
                        "FFT window: "
                        "The gray bars show the energy in the FFT spectrum used by the "
                        "psycho-acoustic model.  Granule 0 is in the left window, granule 1 in "
                        "the right window.  The green and blue bars show how much distortion is "
                        "allowable, as computed by the psycho-acoustic model. The red bars show "
                        "the actual distortion after encoding.  There is one FFT for each "
                        "granule, computed with a 1024 Hann window centered over the "
                        "appropriate granule.  (the range of this 1024 sample window is shown "
                        "by the blue and green bars in the PCM data window).  The Spectrum pull "
                        "down window will toggle between showing the energy in equally spaced "
                        "frequency domain and the scale factor bands used by layer3.  Finally, "
                        "the perceptual entropy, total energy and number of scalefactor bands "
                        "with audible distortion is shown.  (This info not available when "
                        "analyzing .mp3 files.)", -1);

        break;
    case 1:
        /* Set the about box information */
        gtk_window_set_title(GTK_WINDOW(textwindow), "About");
        gtk_widget_set_usize(box, 350, 260);

        sprintf(text, "LAME version %s \n%s\n\n", get_lame_version(), get_lame_url());
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);

        sprintf(text, "psycho-acoustic model:  GPSYCHO version %s\n", get_psy_version());
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);

        sprintf(text, "frame analyzer: MP3x version %s\n\n", get_mp3x_version());
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);

        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL,
                        "decoder:  mpg123/mpglib  .59q  \nMichael Hipp (www.mpg123.de)\n\n", -1);

        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL,
                        "Encoder, decoder & psy-models based on ISO\ndemonstration source. ", -1);
        break;

    case 2:
        gtk_window_set_title(GTK_WINDOW(textwindow), "Statistics");
        gtk_widget_set_usize(box, 350, 260);
        sprintf(text, "frames processed so far: %i \n", Pinfo[0].frameNum + 1);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "granules processed so far: %i \n\n", 4 * (Pinfo[0].frameNum + 1));
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "mean bits/frame (approximate): %i\n", gtkinfo.approxbits);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "mean bits/frame (from LAME): %i\n", 4 * Pinfo[0].mean_bits);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "bitsize of largest frame: %i \n", gtkinfo.maxbits);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "average bits/frame: %3.1f \n\n", gtkinfo.avebits);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "ms_stereo frames: %i \n", gtkinfo.totms);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "i_stereo frames: %i \n", gtkinfo.totis);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "de-emphasis frames: %i \n", gtkinfo.totemph);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "short block granules: %i \n", gtkinfo.totshort);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "mixed block granules: %i \n", gtkinfo.totmix);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        sprintf(text, "preflag granules: %i \n", gtkinfo.totpreflag);
        gtk_text_insert(GTK_TEXT(box), NULL, NULL, NULL, text, -1);
        break;
    }



    gtk_widget_show(vscrollbar);
    gtk_widget_show(box);
    gtk_widget_show(vbox);
    gtk_widget_show(hbox);
    gtk_widget_show(button);

    gtk_box_pack_start(GTK_BOX(hbox), box, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vscrollbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(textwindow), vbox);
    gtk_widget_show(textwindow);

}




/* #include <strings.h>*/


/* This is the GtkItemFactoryEntry structure used to generate new menus.
   Item 1: The menu path. The letter after the underscore indicates an
           accelerator key once the menu is open.
   Item 2: The accelerator key for the entry
   Item 3: The callback function.
   Item 4: The callback action.  This changes the parameters with
           which the function is called.  The default is 0.
   Item 5: The item type, used to define what kind of an item it is.
           Here are the possible values:

           NULL               -> "<Item>"
           ""                 -> "<Item>"
           "<Title>"          -> create a title item
           "<Item>"           -> create a simple item
           "<CheckItem>"      -> create a check item
           "<ToggleItem>"     -> create a toggle item
           "<RadioItem>"      -> create a radio item
           <path>             -> path of a radio item to link against
           "<Separator>"      -> create a separator
           "<Branch>"         -> create an item to hold sub items
           "<LastBranch>"     -> create a right justified branch
*/


#define C(chr)       "<control>" #chr
#define func(name)   (GtkItemFactoryCallback) (name)

static const GtkItemFactoryEntry menu_items[] = {
    {"/_File", NULL, NULL, 0, "<Branch>"},
#if 0
    {"/File/_New", C(N), func(print_hello), 0, NULL},
    {"/File/_Open", C(O), func(print_hello), 0, NULL},
    {"/File/_Save", C(S), func(print_hello), 0, NULL},
    {"/File/Save _As", NULL, NULL, 0, NULL},
    {"/File/sep1", NULL, NULL, 0, "<Separator>"},
    {"/File/Quit", C(Q), func(gtk_main_quit), 0, NULL},
#endif
    {"/File/_Quit", C(Q), func(delete_event), 0, NULL},

    {"/_Plotting", NULL, NULL, 0, "<Branch>"},
    {"/Plotting/_While advancing", NULL, func(spec_option), 5, NULL},
    {"/Plotting/_After advancing", NULL, func(spec_option), 6, NULL},
    {"/Plotting/Toggle SFB lines", NULL, func(spec_option), 7, NULL},
    {"/Plotting/Toggle orig-diff", NULL, func(spec_option), 8, NULL},

    {"/_Channel", NULL, NULL, 0, "<Branch>"},
    {"/Channel/show _Left", NULL, func(channel_option), 1, NULL},
    {"/Channel/show _Right", NULL, func(channel_option), 2, NULL},
    {"/Channel/show _Mid", NULL, func(channel_option), 3, NULL},
    {"/Channel/show _Side", NULL, func(channel_option), 4, NULL},

    {"/_Spectrum", NULL, NULL, 0, "<Branch>"},
    {"/Spectrum/_Scalefactor bands", NULL, func(spec_option), 1, NULL},
    {"/Spectrum/_Wave number", NULL, func(spec_option), 2, NULL},

    {"/_MDCT", NULL, NULL, 0, "<Branch>"},
    {"/MDCT/_Original", NULL, func(spec_option), 3, NULL},
    {"/MDCT/_Compressed", NULL, func(spec_option), 4, NULL},
    {"/MDCT/_Toggle SFB lines", NULL, func(spec_option), 7, NULL},

    {"/_Stats", NULL, NULL, 0, "<Branch>"},
    {"/Stats/_Show", NULL, func(text_window), 2, NULL},

    {"/_Help", NULL, NULL, 0, "<LastBranch>"},
    {"/_Help/_Documentation", NULL, func(text_window), 0, NULL},
    {"/_Help/_About", NULL, func(text_window), 1, NULL},
};

#undef C
#undef func


static void
get_main_menu(GtkWidget * windows, GtkWidget ** menubar)
{
    unsigned int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
    GtkItemFactory *item_factory;
    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new();

    /* This function initializes the item factory.
       Param 1: The type of menu - can be GTK_TYPE_MENU_BAR, GTK_TYPE_MENU,
       or GTK_TYPE_OPTION_MENU.
       Param 2: The path of the menu.
       Param 3: A pointer to a gtk_accel_group.  The item factory sets up
       the accelerator table while generating menus.
     */

    item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

    /* This function generates the menu items. Pass the item factory,
       the number of items in the array, the array itself, and any
       callback data for the the menu items. */
    gtk_item_factory_create_items(item_factory, nmenu_items, (GtkItemFactoryEntry *) menu_items,
                                  NULL);

    /* Attach the new accelerator group to the window. */
    gtk_accel_group_attach(accel_group, GTK_OBJECT(windows));

    if (menubar)
        /* Finally, return the actual menu bar created by the item factory. */
        *menubar = gtk_item_factory_get_widget(item_factory, "<main>");
}
















int
gtkcontrol(lame_global_flags * gfp2, char *inPath)
{
    /* GtkWidget is the storage type for widgets */
    GtkWidget *button;
    GtkAdjustment *adj;
    GtkWidget *mbox;         /* main box */
    GtkWidget *box1;         /* frame control buttons go */
    GtkWidget *box2;         /* frame counters */
    GtkWidget *box3;         /* frame header info */
    GtkWidget *table;        /* table for all the plotting areas */
    GtkWidget *menubar;

    gint    tableops, graphx, graphy;
    char    frameinfo[80];

    graphx = 600;       /* minimum allowed size of pixmap */
    graphy = 95;

    gfp = gfp2;
    gfc = gfp->internal_flags;

    /* set some global defaults/variables */
    gtkinfo.filetype = is_mpeg_file_format(input_format) ? 1 : 0;
    gtkinfo.msflag = 0;
    gtkinfo.chflag = 0;
    gtkinfo.kbflag = 0;
    gtkinfo.flag123 = is_mpeg_file_format(input_format) ? 1 : 0; /* MP3 file=use mpg123 output */
    gtkinfo.pupdate = 0;
    gtkinfo.avebits = 0;
    gtkinfo.maxbits = 0;
    gtkinfo.approxbits = 0;
    gtkinfo.totemph = 0;
    gtkinfo.totms = 0;
    gtkinfo.totis = 0;
    gtkinfo.totshort = 0;
    gtkinfo.totmix = 0;
    gtkinfo.sfblines = 1;
    gtkinfo.difference = 0;
    gtkinfo.totalframes = 0;

    memset((char *) Pinfo, 0, sizeof(Pinfo));
    pplot = &Pinfo[READ_AHEAD];

    strcpy(frameinfo, "MP3x: ");
    strncat(frameinfo, inPath, 70);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), frameinfo);
    gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(delete_event), NULL);

    gtk_signal_connect_object(GTK_OBJECT(window), "key_press_event",
                              GTK_SIGNAL_FUNC(key_press_event), GTK_OBJECT(window));

    gtk_container_set_border_width(GTK_CONTAINER(window), 0);


    mbox = gtk_vbox_new(FALSE, 0);


    /* layout of mbox */
    box1 = gtk_hbox_new(FALSE, 0);
    box2 = gtk_hbox_new(FALSE, 0);
    box3 = gtk_hbox_new(FALSE, 0);
    table = gtk_table_new(5, 2, FALSE);
    tableops = GTK_FILL | GTK_EXPAND | GTK_SHRINK;
    get_main_menu(window, &menubar);

    gtk_box_pack_start(GTK_BOX(mbox), menubar, FALSE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(mbox), box1, FALSE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(mbox), box2, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mbox), box3, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mbox), table, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(window), mbox);


    /*********************************************************************/
    /* stuff in box3  frame header info */
    /*********************************************************************/
    /*
       headerbox = gtk_label_new(" ");
       gtk_label_set_justify(GTK_LABEL(headerbox),GTK_JUSTIFY_LEFT);
     */
    headerbox = gtk_text_new(NULL, NULL);
    gtk_text_set_editable(GTK_TEXT(headerbox), FALSE);
    gtk_widget_set_usize(headerbox, 200, 20);
    gtk_widget_show(headerbox);
    gtk_box_pack_start(GTK_BOX(box3), headerbox, TRUE, TRUE, 0);



    /*********************************************************************/
    /* stuff in box2   frame counters  */
    /*********************************************************************/
    framecounter = gtk_label_new("");
    gtk_widget_show(framecounter);
    gtk_box_pack_start(GTK_BOX(box2), framecounter, FALSE, TRUE, 0);

    adj = (GtkAdjustment *) gtk_adjustment_new(0, 0, (gint) lame_get_totalframes(gfp) - 1, 0, 0, 0);
    frameprogress = gtk_progress_bar_new_with_adjustment(adj);
    /* Set the format of the string that can be displayed in the
     * trough of the progress bar:
     * %p - percentage
     * %v - value
     * %l - lower range value
     * %u - upper range value */
    gtk_progress_set_format_string(GTK_PROGRESS(frameprogress), "%p%%");
    gtk_progress_set_value(GTK_PROGRESS(frameprogress), (gdouble) 0);
    gtk_progress_set_show_text(GTK_PROGRESS(frameprogress), TRUE);
    gtk_widget_show(frameprogress);
    gtk_box_pack_end(GTK_BOX(box2), frameprogress, FALSE, TRUE, 0);



    /*********************************************************************/
    /* stuff in box1  buttons along bottom */
    /*********************************************************************/
    button = gtk_button_new_with_label("-1");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(frameadv), (gpointer) "-1");
    gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("+1");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(frameadv), (gpointer) "1");
    gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("+10");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(frameadv), (gpointer) "10");
    gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("+100");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(frameadv), (gpointer) "100");
    gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("last frame");
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
                       GTK_SIGNAL_FUNC(frameadv), (gpointer) "finish");
    gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
    gtk_widget_show(button);

    button = gtk_button_new_with_label("stop/plot");
    gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(plotclick), NULL);
    gtk_box_pack_start(GTK_BOX(box1), button, TRUE, TRUE, 0);
    gtk_widget_show(button);


    /*********************************************************************/
    /* stuff in table.  all the plotting windows */
    /*********************************************************************/
    pcmbox = gpk_plot_new(graphx, graphy);
    gtk_table_attach(GTK_TABLE(table), pcmbox, 0, 2, 0, 1, tableops, tableops, 2, 2);
    gtk_widget_show(pcmbox);

    winbox = gpk_plot_new(graphy, graphy);
    gtk_table_attach(GTK_TABLE(table), winbox, 0, 2, 1, 2, tableops, tableops, 2, 2);
    gtk_widget_show(winbox);


    mdctbox[0] = gpk_plot_new(graphy, graphy);
    gtk_table_attach(GTK_TABLE(table), mdctbox[0], 0, 1, 2, 3, tableops, tableops, 2, 2);
    gtk_widget_show(mdctbox[0]);

    mdctbox[1] = gpk_plot_new(graphy, graphy);
    gtk_table_attach(GTK_TABLE(table), mdctbox[1], 1, 2, 2, 3, tableops, tableops, 2, 2);
    gtk_widget_show(mdctbox[1]);

    enerbox[0] = gpk_plot_new(graphy, graphy);
    gtk_table_attach(GTK_TABLE(table), enerbox[0], 0, 1, 3, 4, tableops, tableops, 2, 2);
    gtk_widget_show(enerbox[0]);

    enerbox[1] = gpk_plot_new(graphy, graphy);
    gtk_table_attach(GTK_TABLE(table), enerbox[1], 1, 2, 3, 4, tableops, tableops, 2, 2);
    gtk_widget_show(enerbox[1]);

    sfbbox[0] = gpk_plot_new(graphy, graphy);
    gtk_table_attach(GTK_TABLE(table), sfbbox[0], 0, 1, 4, 5, tableops, tableops, 2, 2);
    gtk_widget_show(sfbbox[0]);

    sfbbox[1] = gpk_plot_new(graphy, graphy);
    gtk_table_attach(GTK_TABLE(table), sfbbox[1], 1, 2, 4, 5, tableops, tableops, 2, 2);
    gtk_widget_show(sfbbox[1]);




    gtk_idle_add((GtkFunction) frameadv1, NULL);
    gtk_widget_show(menubar);
    gtk_widget_show(box2);
    gtk_widget_show(box3);
    gtk_widget_show(table);
    gtk_widget_show(box1);
    gtk_widget_show(mbox);
    gtk_widget_show(window); /* show smallest allowed window */

    /* make window bigger.   */
    /* now the user will be able to shrink it, if desired */
    /* gtk_widget_set_usize(mbox,500,500);  */
    /* gtk_widget_show (window); */ /* show smallest allowed window */



    idle_keepgoing = 1; /* processing of frames is ON */
    idle_count_max = READ_AHEAD + 1; /* number of frames to process before plotting */
    idle_count = 0;     /* pause & plot when idle_count=idle_count_max */

    gtk_main();
    assert(mp3done);
    return (0);
}

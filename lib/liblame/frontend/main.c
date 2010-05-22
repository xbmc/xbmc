/*
 *      Command line frontend program
 *
 *      Copyright (c) 1999 Mark Taylor
 *                    2000 Takehiro TOMINAGA
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

/* $Id: main.c,v 1.107.2.1 2009/01/18 15:44:28 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char   *strchr(), *strrchr();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef __sun__
/* woraround for SunOS 4.x, it has SEEK_* defined here */
#include <unistd.h>
#endif

#if defined(_WIN32)
# include <windows.h>
#endif


/*
 main.c is example code for how to use libmp3lame.a.  To use this library,
 you only need the library and lame.h.  All other .h files are private
 to the library.
*/
#include "lame.h"

#include "console.h"
#include "brhist.h"
#include "parse.h"
#include "main.h"
#include "get_audio.h"
#include "portableio.h"
#include "timestatus.h"

/* PLL 14/04/2000 */
#if macintosh
#include <console.h>
#endif

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif




/************************************************************************
*
* main
*
* PURPOSE:  MPEG-1,2 Layer III encoder with GPSYCHO
* psychoacoustic model.
*
************************************************************************/

static int
parse_args_from_string(lame_global_flags * const gfp, const char *p, char *inPath, char *outPath)
{                       /* Quick & very Dirty */
    char   *q;
    char   *f;
    char   *r[128];
    int     c = 0;
    int     ret;

    if (p == NULL || *p == '\0')
        return 0;

    f = q = malloc(strlen(p) + 1);
    strcpy(q, p);

    r[c++] = "lhama";
    while (1) {
        r[c++] = q;
        while (*q != ' ' && *q != '\0')
            q++;
        if (*q == '\0')
            break;
        *q++ = '\0';
    }
    r[c] = NULL;

    ret = parse_args(gfp, c, r, inPath, outPath, NULL, NULL);
    free(f);
    return ret;
}





static FILE *
init_files(lame_global_flags * gf, char *inPath, char *outPath, int *enc_delay, int *enc_padding)
{
    FILE   *outf;
    /* Mostly it is not useful to use the same input and output name.
       This test is very easy and buggy and don't recognize different names
       assigning the same file
     */
    if (0 != strcmp("-", outPath) && 0 == strcmp(inPath, outPath)) {
        error_printf("Input file and Output file are the same. Abort.\n");
        return NULL;
    }

    /* open the wav/aiff/raw pcm or mp3 input file.  This call will
     * open the file, try to parse the headers and
     * set gf.samplerate, gf.num_channels, gf.num_samples.
     * if you want to do your own file input, skip this call and set
     * samplerate, num_channels and num_samples yourself.
     */
    init_infile(gf, inPath, enc_delay, enc_padding);
    if ((outf = init_outfile(outPath, lame_get_decode_only(gf))) == NULL) {
        error_printf("Can't init outfile '%s'\n", outPath);
        return NULL;
    }

    return outf;
}






/* the simple lame decoder */
/* After calling lame_init(), lame_init_params() and
 * init_infile(), call this routine to read the input MP3 file
 * and output .wav data to the specified file pointer*/
/* lame_decoder will ignore the first 528 samples, since these samples
 * represent the mpglib delay (and are all 0).  skip = number of additional
 * samples to skip, to (for example) compensate for the encoder delay */

int
lame_decoder(lame_global_flags * gfp, FILE * outf, int skip_start, char *inPath, char *outPath,
             int *enc_delay, int *enc_padding)
{
    short int Buffer[2][1152];
    int     iread;
    int     skip_end = 0;
    double  wavsize;
    int     i;
    void    (*WriteFunction) (FILE * fp, char *p, int n);
    int     tmp_num_channels = lame_get_num_channels(gfp);



    if (silent < 10)
        console_printf("\rinput:  %s%s(%g kHz, %i channel%s, ",
                       strcmp(inPath, "-") ? inPath : "<stdin>",
                       strlen(inPath) > 26 ? "\n\t" : "  ",
                       lame_get_in_samplerate(gfp) / 1.e3,
                       tmp_num_channels, tmp_num_channels != 1 ? "s" : "");

    switch (input_format) {
    case sf_mp123: /* FIXME: !!! */
      error_printf("Internal error.  Aborting.");
      exit(-1);

    case sf_mp3:
        if (skip_start == 0) {
            if (*enc_delay > -1 || *enc_padding > -1) {
                if (*enc_delay > -1)
                    skip_start = *enc_delay + 528 + 1;
                if (*enc_padding > -1)
                    skip_end = *enc_padding - (528 + 1);
            }
            else
                skip_start = lame_get_encoder_delay(gfp) + 528 + 1;
        }
        else {
            /* user specified a value of skip. just add for decoder */
            skip_start += 528 + 1; /* mp3 decoder has a 528 sample delay, plus user supplied "skip" */
        }

        if (silent < 10)
            console_printf("MPEG-%u%s Layer %s", 2 - lame_get_version(gfp),
                           lame_get_out_samplerate(gfp) < 16000 ? ".5" : "", "III");
        break;
    case sf_mp2:
        skip_start += 240 + 1;
        if (silent < 10)
            console_printf("MPEG-%u%s Layer %s", 2 - lame_get_version(gfp),
                           lame_get_out_samplerate(gfp) < 16000 ? ".5" : "", "II");
        break;
    case sf_mp1:
        skip_start += 240 + 1;
        if (silent < 10)
            console_printf("MPEG-%u%s Layer %s", 2 - lame_get_version(gfp),
                           lame_get_out_samplerate(gfp) < 16000 ? ".5" : "", "I");
        break;
    case sf_raw:
        if (silent < 10)
            console_printf("raw PCM data");
        mp3input_data.nsamp = lame_get_num_samples(gfp);
        mp3input_data.framesize = 1152;
        skip_start = 0; /* other formats have no delay */ /* is += 0 not better ??? */
        break;
    case sf_wave:
        if (silent < 10)
            console_printf("Microsoft WAVE");
        mp3input_data.nsamp = lame_get_num_samples(gfp);
        mp3input_data.framesize = 1152;
        skip_start = 0; /* other formats have no delay */ /* is += 0 not better ??? */
        break;
    case sf_aiff:
        if (silent < 10)
            console_printf("SGI/Apple AIFF");
        mp3input_data.nsamp = lame_get_num_samples(gfp);
        mp3input_data.framesize = 1152;
        skip_start = 0; /* other formats have no delay */ /* is += 0 not better ??? */
        break;
    default:
        if (silent < 10)
            console_printf("unknown");
        mp3input_data.nsamp = lame_get_num_samples(gfp);
        mp3input_data.framesize = 1152;
        skip_start = 0; /* other formats have no delay */ /* is += 0 not better ??? */
        assert(0);
        break;
    }

    if (silent < 10) {
        console_printf(")\noutput: %s%s(16 bit, Microsoft WAVE)\n",
                       strcmp(outPath, "-") ? outPath : "<stdout>",
                       strlen(outPath) > 45 ? "\n\t" : "  ");

        if (skip_start > 0)
            console_printf("skipping initial %i samples (encoder+decoder delay)\n", skip_start);
        if (skip_end > 0)
            console_printf("skipping final %i samples (encoder padding-decoder delay)\n", skip_end);
    }

    if (0 == disable_wav_header)
        WriteWaveHeader(outf, 0x7FFFFFFF, lame_get_in_samplerate(gfp), tmp_num_channels, 16);
    /* unknown size, so write maximum 32 bit signed value */

    wavsize = -(skip_start + skip_end);
    WriteFunction = swapbytes ? WriteBytesSwapped : WriteBytes;
    mp3input_data.totalframes = mp3input_data.nsamp / mp3input_data.framesize;

    assert(tmp_num_channels >= 1 && tmp_num_channels <= 2);

    do {
        iread = get_audio16(gfp, Buffer); /* read in 'iread' samples */
        if (iread >= 0) {
            mp3input_data.framenum += iread / mp3input_data.framesize;
            wavsize += iread;

            if (silent <= 0) {
                decoder_progress(&mp3input_data);
                console_flush();
            }

            skip_start -= (i = skip_start < iread ? skip_start : iread); /* 'i' samples are to skip in this frame */

            if (skip_end > 1152 && mp3input_data.framenum + 2 > mp3input_data.totalframes) {
                iread -= (skip_end - 1152);
                skip_end = 1152;
            }
            else if (mp3input_data.framenum == mp3input_data.totalframes && iread != 0)
                iread -= skip_end;

            for (; i < iread; i++) {
                if (disable_wav_header) {
                    WriteFunction(outf, (char *) &Buffer[0][i], sizeof(short));
                    if (tmp_num_channels == 2)
                        WriteFunction(outf, (char *) &Buffer[1][i], sizeof(short));
                }
                else {
                    Write16BitsLowHigh(outf, Buffer[0][i]);
                    if (tmp_num_channels == 2)
                        Write16BitsLowHigh(outf, Buffer[1][i]);
                }
            }
            if (flush_write == 1) {
                fflush(outf);
            }
        }
    } while (iread > 0);

    i = (16 / 8) * tmp_num_channels;
    assert(i > 0);
    if (wavsize <= 0) {
        if (silent < 10)
            error_printf("WAVE file contains 0 PCM samples\n");
        wavsize = 0;
    }
    else if (wavsize > 0xFFFFFFD0 / i) {
        if (silent < 10)
            error_printf("Very huge WAVE file, can't set filesize accordingly\n");
        wavsize = 0xFFFFFFD0;
    }
    else {
        wavsize *= i;
    }

    /* if outf is seekable, rewind and adjust length */
    if (!disable_wav_header && strcmp("-", outPath)
	&& !fseek(outf, 0l, SEEK_SET))
	WriteWaveHeader(outf, (int) wavsize, lame_get_in_samplerate(gfp),
			tmp_num_channels, 16);
    fclose(outf);

    if (silent <= 0)
        decoder_progress_finish();
    return 0;
}



static void
print_lame_tag_leading_info(lame_global_flags * gf)
{
    if (lame_get_bWriteVbrTag(gf))
        console_printf("Writing LAME Tag...");
}

static void
print_trailing_info(lame_global_flags * gf)
{
    if (lame_get_bWriteVbrTag(gf))
        console_printf("done\n");

    if (lame_get_findReplayGain(gf)) {
        int     RadioGain = lame_get_RadioGain(gf);
        console_printf("ReplayGain: %s%.1fdB\n", RadioGain > 0 ? "+" : "",
                       ((float) RadioGain) / 10.0);
        if (RadioGain > 0x1FE || RadioGain < -0x1FE)
            error_printf
                    ("WARNING: ReplayGain exceeds the -51dB to +51dB range. Such a result is too\n"
                    "         high to be stored in the header.\n");
    }

    /* if (the user requested printing info about clipping) and (decoding
    on the fly has actually been performed) */
    if (print_clipping_info && lame_get_decode_on_the_fly(gf)) {
        float   noclipGainChange = (float) lame_get_noclipGainChange(gf) / 10.0f;
        float   noclipScale = lame_get_noclipScale(gf);

        if (noclipGainChange > 0.0) { /* clipping occurs */
            console_printf
                    ("WARNING: clipping occurs at the current gain. Set your decoder to decrease\n"
                    "         the  gain  by  at least %.1fdB or encode again ", noclipGainChange);

            /* advice the user on the scale factor */
            if (noclipScale > 0) {
                console_printf("using  --scale %.2f\n", noclipScale);
                console_printf("         or less (the value under --scale is approximate).\n");
            }
            else {
                /* the user specified his own scale factor. We could suggest
                * the scale factor of (32767.0/gfp->PeakSample)*(gfp->scale)
                * but it's usually very inaccurate. So we'd rather advice him to
                * disable scaling first and see our suggestion on the scale factor then. */
                console_printf("using --scale <arg>\n"
                        "         (For   a   suggestion  on  the  optimal  value  of  <arg>  encode\n"
                        "         with  --scale 1  first)\n");
            }

        }
        else {          /* no clipping */
            if (noclipGainChange > -0.1)
                console_printf
                        ("\nThe waveform does not clip and is less than 0.1dB away from full scale.\n");
            else
                console_printf
                        ("\nThe waveform does not clip and is at least %.1fdB away from full scale.\n",
                         -noclipGainChange);
        }
    }

}




static int
write_xing_frame(lame_global_flags * gf, FILE * outf)
{
    unsigned char mp3buffer[LAME_MAXMP3BUFFER];
    size_t  imp3, owrite;
    
    imp3 = lame_get_lametag_frame(gf, mp3buffer, sizeof(mp3buffer));
    if (imp3 > sizeof(mp3buffer)) {
        error_printf("Error writing LAME-tag frame: buffer too small: buffer size=%d  frame size=%d\n"
                    , sizeof(mp3buffer)
                    , imp3
                    );
        return -1;
    }
    if (imp3 <= 0) {
        return 0;
    }
    owrite = (int) fwrite(mp3buffer, 1, imp3, outf);
    if (owrite != imp3) {
        error_printf("Error writing LAME-tag \n");
        return -1;
    }
    if (flush_write == 1) {
        fflush(outf);
    }
    return imp3;
}



static int
lame_encoder(lame_global_flags * gf, FILE * outf, int nogap, char *inPath, char *outPath)
{
    unsigned char mp3buffer[LAME_MAXMP3BUFFER];
    int     Buffer[2][1152];
    int     iread, imp3, owrite, id3v2_size;

    encoder_progress_begin(gf, inPath, outPath);

    imp3 = lame_get_id3v2_tag(gf, mp3buffer, sizeof(mp3buffer));
    if ((size_t)imp3 > sizeof(mp3buffer)) {
        encoder_progress_end(gf);
        error_printf("Error writing ID3v2 tag: buffer too small: buffer size=%d  ID3v2 size=%d\n"
                , sizeof(mp3buffer)
                , imp3
                    );
        return 1;
    }
    owrite = (int) fwrite(mp3buffer, 1, imp3, outf);
    if (owrite != imp3) {
        encoder_progress_end(gf);
        error_printf("Error writing ID3v2 tag \n");
        return 1;
    }
    if (flush_write == 1) {
        fflush(outf);
    }    
    id3v2_size = imp3;
    
    /* encode until we hit eof */
    do {
        /* read in 'iread' samples */
        iread = get_audio(gf, Buffer);

        if (iread >= 0) {
            encoder_progress(gf);

            /* encode */
            imp3 = lame_encode_buffer_int(gf, Buffer[0], Buffer[1], iread,
                                          mp3buffer, sizeof(mp3buffer));

            /* was our output buffer big enough? */
            if (imp3 < 0) {
                if (imp3 == -1)
                    error_printf("mp3 buffer is not big enough... \n");
                else
                    error_printf("mp3 internal error:  error code=%i\n", imp3);
                return 1;
            }
            owrite = (int) fwrite(mp3buffer, 1, imp3, outf);
            if (owrite != imp3) {
                error_printf("Error writing mp3 output \n");
                return 1;
            }
        }
        if (flush_write == 1) {
            fflush(outf);
        }
    } while (iread > 0);

    if (nogap)
        imp3 = lame_encode_flush_nogap(gf, mp3buffer, sizeof(mp3buffer)); /* may return one more mp3 frame */
    else
        imp3 = lame_encode_flush(gf, mp3buffer, sizeof(mp3buffer)); /* may return one more mp3 frame */

    if (imp3 < 0) {
        if (imp3 == -1)
            error_printf("mp3 buffer is not big enough... \n");
        else
            error_printf("mp3 internal error:  error code=%i\n", imp3);
        return 1;

    }

    encoder_progress_end(gf);
    
    owrite = (int) fwrite(mp3buffer, 1, imp3, outf);
    if (owrite != imp3) {
        error_printf("Error writing mp3 output \n");
        return 1;
    }
    if (flush_write == 1) {
        fflush(outf);
    }

    
    imp3 = lame_get_id3v1_tag(gf, mp3buffer, sizeof(mp3buffer));
    if ((size_t)imp3 > sizeof(mp3buffer)) {
        error_printf("Error writing ID3v1 tag: buffer too small: buffer size=%d  ID3v1 size=%d\n"
                , sizeof(mp3buffer)
                , imp3
                    );
    }
    else {
        if (imp3 > 0) {
            owrite = (int) fwrite(mp3buffer, 1, imp3, outf);
            if (owrite != imp3) {
                error_printf("Error writing ID3v1 tag \n");
                return 1;
            }
            if (flush_write == 1) {
                fflush(outf);
            }
        }
    }
    
    if (silent <= 0) {
        print_lame_tag_leading_info(gf);
    }
    if (fseek(outf, id3v2_size, SEEK_SET) != 0) {
        error_printf("fatal error: can't update LAME-tag frame!\n");                    
    }
    else {
        write_xing_frame(gf, outf);
    }

    if (silent <= 0) {
        print_trailing_info(gf);
    }    
    return 0;
}






static void
brhist_init_package(lame_global_flags * gf)
{
#ifdef BRHIST
    if (brhist) {
        if (brhist_init(gf, lame_get_VBR_min_bitrate_kbps(gf), lame_get_VBR_max_bitrate_kbps(gf))) {
            /* fail to initialize */
            brhist = 0;
        }
    }
    else {
        brhist_init(gf, 128, 128); /* Dirty hack */
    }
#endif
}



static
    void
parse_nogap_filenames(int nogapout, char *inPath, char *outPath, char *outdir)
{

    char   *slasher;
    size_t  n;

    strcpy(outPath, outdir);
    if (!nogapout) {
        strncpy(outPath, inPath, PATH_MAX + 1 - 4);
        n = strlen(outPath);
        /* nuke old extension, if one  */
        if (outPath[n - 3] == 'w'
            && outPath[n - 2] == 'a' && outPath[n - 1] == 'v' && outPath[n - 4] == '.') {
            outPath[n - 3] = 'm';
            outPath[n - 2] = 'p';
            outPath[n - 1] = '3';
        }
        else {
            outPath[n + 0] = '.';
            outPath[n + 1] = 'm';
            outPath[n + 2] = 'p';
            outPath[n + 3] = '3';
            outPath[n + 4] = 0;
        }
    }
    else {
        slasher = inPath;
        slasher += PATH_MAX + 1 - 4;

        /* backseek to last dir delemiter */
        while (*slasher != '/' && *slasher != '\\' && slasher != inPath && *slasher != ':') {
            slasher--;
        }

        /* skip one foward if needed */
        if (slasher != inPath
            && (outPath[strlen(outPath) - 1] == '/'
                || outPath[strlen(outPath) - 1] == '\\' || outPath[strlen(outPath) - 1] == ':'))
            slasher++;
        else if (slasher == inPath
                 && (outPath[strlen(outPath) - 1] != '/'
                     &&
                     outPath[strlen(outPath) - 1] != '\\' && outPath[strlen(outPath) - 1] != ':'))
#ifdef _WIN32
            strcat(outPath, "\\");
#elif __OS2__
            strcat(outPath, "\\");
#else
            strcat(outPath, "/");
#endif

        strncat(outPath, slasher, PATH_MAX + 1 - 4);
        n = strlen(outPath);
        /* nuke old extension  */
        if (outPath[n - 3] == 'w'
            && outPath[n - 2] == 'a' && outPath[n - 1] == 'v' && outPath[n - 4] == '.') {
            outPath[n - 3] = 'm';
            outPath[n - 2] = 'p';
            outPath[n - 1] = '3';
        }
        else {
            outPath[n + 0] = '.';
            outPath[n + 1] = 'm';
            outPath[n + 2] = 'p';
            outPath[n + 3] = '3';
            outPath[n + 4] = 0;
        }
    }
}




/***********************************************************************
*
*  Message Output
*
***********************************************************************/


int
main(int argc, char **argv)
{
    int     ret;
    lame_global_flags *gf;
    char    outPath[PATH_MAX + 1];
    char    nogapdir[PATH_MAX + 1];
    char    inPath[PATH_MAX + 1];

    /* add variables for encoder delay/padding */
    int     enc_delay = -1;
    int     enc_padding = -1;

    /* support for "nogap" encoding of up to 200 .wav files */
#define MAX_NOGAP 200
    int     nogapout = 0;
    int     max_nogap = MAX_NOGAP;
    char    nogap_inPath_[MAX_NOGAP][PATH_MAX+1];
    char*   nogap_inPath[MAX_NOGAP];

    int     i;
    FILE   *outf;

#if macintosh
    argc = ccommand(&argv);
#endif
#if 0
    /* rh 061207
       the following fix seems to be a workaround for a problem in the
       parent process calling LAME. It would be better to fix the broken
       application => code disabled.
     */
#if defined(_WIN32)
    /* set affinity back to all CPUs.  Fix for EAC/lame on SMP systems from
       "Todd Richmond" <todd.richmond@openwave.com> */
    typedef BOOL(WINAPI * SPAMFunc) (HANDLE, DWORD_PTR);
    SPAMFunc func;
    SYSTEM_INFO si;

    if ((func = (SPAMFunc) GetProcAddress(GetModuleHandleW(L"KERNEL32.DLL"),
                                          "SetProcessAffinityMask")) != NULL) {
        GetSystemInfo(&si);
        func(GetCurrentProcess(), si.dwActiveProcessorMask);
    }
#endif
#endif

#ifdef __EMX__
    /* This gives wildcard expansion on Non-POSIX shells with OS/2 */
    _wildcard(&argc, &argv);
#endif

    memset(nogap_inPath_, 0, sizeof(nogap_inPath_));
    for (i = 0; i < MAX_NOGAP; ++i) {
        nogap_inPath[i] = &nogap_inPath_[i][0];
    }

    memset(inPath, 0, sizeof(inPath));

    frontend_open_console();

    /* initialize libmp3lame */
    input_format = sf_unknown;
    if (NULL == (gf = lame_init())) {
        error_printf("fatal error during initialization\n");
        frontend_close_console();
        return 1;
    }
    lame_set_errorf(gf, &frontend_errorf);
    lame_set_debugf(gf, &frontend_debugf);
    lame_set_msgf(gf, &frontend_msgf);
    if (argc <= 1) {
        usage(stderr, argv[0]); /* no command-line args, print usage, exit  */
        lame_close(gf);
        frontend_close_console();
        return 1;
    }

    /* parse the command line arguments, setting various flags in the
     * struct 'gf'.  If you want to parse your own arguments,
     * or call libmp3lame from a program which uses a GUI to set arguments,
     * skip this call and set the values of interest in the gf struct.
     * (see the file API and lame.h for documentation about these parameters)
     */
    parse_args_from_string(gf, getenv("LAMEOPT"), inPath, outPath);
    ret = parse_args(gf, argc, argv, inPath, outPath, nogap_inPath, &max_nogap);
    if (ret < 0) {
        lame_close(gf);
        frontend_close_console();
        return ret == -2 ? 0 : 1;
    }
    if (update_interval < 0.)
        update_interval = 2.;

    if (outPath[0] != '\0' && max_nogap > 0) {
        strncpy(nogapdir, outPath, PATH_MAX + 1);
        nogapout = 1;
    }

    /* initialize input file.  This also sets samplerate and as much
       other data on the input file as available in the headers */
    if (max_nogap > 0) {
        /* for nogap encoding of multiple input files, it is not possible to
         * specify the output file name, only an optional output directory. */
        parse_nogap_filenames(nogapout, nogap_inPath[0], outPath, nogapdir);
        outf = init_files(gf, nogap_inPath[0], outPath, &enc_delay, &enc_padding);
    }
    else {
        outf = init_files(gf, inPath, outPath, &enc_delay, &enc_padding);
    }
    if (outf == NULL) {
        lame_close(gf);
        frontend_close_console();
        return -1;
    }
    /* turn off automatic writing of ID3 tag data into mp3 stream 
     * we have to call it before 'lame_init_params', because that
     * function would spit out ID3v2 tag data.
     */
    lame_set_write_id3tag_automatic(gf, 0);

    /* Now that all the options are set, lame needs to analyze them and
     * set some more internal options and check for problems
     */
    i = lame_init_params(gf);
    if (i < 0) {
        if (i == -1) {
            display_bitrates(stderr);
        }
        error_printf("fatal error during initialization\n");
        lame_close(gf);
        frontend_close_console();
        return i;
    }

    if (silent > 0) {
        brhist = 0;     /* turn off VBR histogram */
    }


    if (lame_get_decode_only(gf)) {
        /* decode an mp3 file to a .wav */
        if (mp3_delay_set)
            lame_decoder(gf, outf, mp3_delay, inPath, outPath, &enc_delay, &enc_padding);
        else
            lame_decoder(gf, outf, 0, inPath, outPath, &enc_delay, &enc_padding);

    }
    else {
        if (max_nogap > 0) {
            /*
             * encode multiple input files using nogap option
             */
            for (i = 0; i < max_nogap; ++i) {
                int     use_flush_nogap = (i != (max_nogap - 1));
                if (i > 0) {
                    parse_nogap_filenames(nogapout, nogap_inPath[i], outPath, nogapdir);
                    /* note: if init_files changes anything, like
                       samplerate, num_channels, etc, we are screwed */
                    outf = init_files(gf, nogap_inPath[i], outPath, &enc_delay, &enc_padding);
                    /* reinitialize bitstream for next encoding.  this is normally done
                     * by lame_init_params(), but we cannot call that routine twice */
                    lame_init_bitstream(gf);
                }
                brhist_init_package(gf);
                lame_set_nogap_total(gf, max_nogap);
                lame_set_nogap_currentindex(gf, i);
                
                ret = lame_encoder(gf, outf, use_flush_nogap, nogap_inPath[i], outPath);

                fclose(outf); /* close the output file */
                close_infile(); /* close the input file */

            }
        }
        else {
            /*
             * encode a single input file
             */
            brhist_init_package(gf);
            
            ret = lame_encoder(gf, outf, 0, inPath, outPath);

            fclose(outf); /* close the output file */
            close_infile(); /* close the input file */
        }
    }
    lame_close(gf);
    frontend_close_console();
    return ret;
}

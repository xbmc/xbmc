/*
 *      Command line parsing related functions
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

/* $Id: parse.c,v 1.247.2.8 2009/12/11 22:44:25 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <ctype.h>

#ifdef STDC_HEADERS
# include <stdio.h>
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

#ifdef __OS2__
#include <os2.h>
#define PRTYC_IDLE 1
#define PRTYC_REGULAR 2
#define PRTYD_MINIMUM -31
#define PRTYD_MAXIMUM 31
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#include "lame.h"
#include "set_get.h"

#include "brhist.h"
#include "parse.h"
#include "main.h"
#include "get_audio.h"
#include "version.h"
#include "console.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

                 
#ifdef HAVE_ICONV
#include <iconv.h>
#include <errno.h>
#endif

#if defined DEBUG || _DEBUG || _ALLOW_INTERNAL_OPTIONS
#define INTERNAL_OPTS 1
#else
#define INTERNAL_OPTS LAME_ALPHA_VERSION
#endif

#if (INTERNAL_OPTS!=0)
#define DEV_HELP(a) a
#else
#define DEV_HELP(a)
#endif



/* GLOBAL VARIABLES.  set by parse_args() */
/* we need to clean this up */
sound_file_format input_format;
int     swapbytes = 0;       /* force byte swapping   default=0 */
int     silent;              /* Verbosity */
int     ignore_tag_errors;   /* Ignore errors in values passed for tags */
int     brhist;
float   update_interval;     /* to use Frank's time status display */
int     mp3_delay;           /* to adjust the number of samples truncated
                                during decode */
int     mp3_delay_set;       /* user specified the value of the mp3 encoder
                                delay to assume for decoding */

int     disable_wav_header;
mp3data_struct mp3input_data; /* used by MP3 */
int     print_clipping_info; /* print info whether waveform clips */


int     in_signed = -1;

enum ByteOrder in_endian = ByteOrderLittleEndian;

int     in_bitwidth = 16;

int     flush_write = 0;



/**
 *  Long Filename support for the WIN32 platform
 *
 */
#ifdef WIN32
#include <winbase.h>
static void
dosToLongFileName(char *fn)
{
    const int MSIZE = PATH_MAX + 1 - 4; /*  we wanna add ".mp3" later */
    WIN32_FIND_DATAA lpFindFileData;
    HANDLE  h = FindFirstFileA(fn, &lpFindFileData);
    if (h != INVALID_HANDLE_VALUE) {
        int     a;
        char   *q, *p;
        FindClose(h);
        for (a = 0; a < MSIZE; a++) {
            if ('\0' == lpFindFileData.cFileName[a])
                break;
        }
        if (a >= MSIZE || a == 0)
            return;
        q = strrchr(fn, '\\');
        p = strrchr(fn, '/');
        if (p - q > 0)
            q = p;
        if (q == NULL)
            q = strrchr(fn, ':');
        if (q == NULL)
            strncpy(fn, lpFindFileData.cFileName, a);
        else {
            a += q - fn + 1;
            if (a >= MSIZE)
                return;
            strncpy(++q, lpFindFileData.cFileName, MSIZE - a);
        }
    }
}
#endif
#if defined(WIN32)
#include <windows.h>
BOOL
SetPriorityClassMacro(DWORD p)
{
    HANDLE  op = GetCurrentProcess();
    return SetPriorityClass(op, p);
}

static void
setWin32Priority(lame_global_flags * gfp, int Priority)
{
    switch (Priority) {
    case 0:
    case 1:
        SetPriorityClassMacro(IDLE_PRIORITY_CLASS);
        console_printf("==> Priority set to Low.\n");
        break;
    default:
    case 2:
        SetPriorityClassMacro(NORMAL_PRIORITY_CLASS);
        console_printf("==> Priority set to Normal.\n");
        break;
    case 3:
    case 4:
        SetPriorityClassMacro(HIGH_PRIORITY_CLASS);
        console_printf("==> Priority set to High.\n");
        break;
    }
}
#endif


#if defined(__OS2__)
/* OS/2 priority functions */
static int
setOS2Priority(lame_global_flags * gfp, int Priority)
{
    int     rc;

    switch (Priority) {

    case 0:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_IDLE, /* select priority class (idle, regular, etc) */
                            0, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 0 (Low priority).\n");
        break;

    case 1:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_IDLE, /* select priority class (idle, regular, etc) */
                            PRTYD_MAXIMUM, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 1 (Medium priority).\n");
        break;

    case 2:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_REGULAR, /* select priority class (idle, regular, etc) */
                            PRTYD_MINIMUM, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 2 (Regular priority).\n");
        break;

    case 3:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_REGULAR, /* select priority class (idle, regular, etc) */
                            0, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 3 (High priority).\n");
        break;

    case 4:
        rc = DosSetPriority(0, /* Scope: only one process */
                            PRTYC_REGULAR, /* select priority class (idle, regular, etc) */
                            PRTYD_MAXIMUM, /* set delta */
                            0); /* Assume current process */
        console_printf("==> Priority set to 4 (Maximum priority). I hope you enjoy it :)\n");
        break;

    default:
        console_printf("==> Invalid priority specified! Assuming idle priority.\n");
    }


    return 0;
}
#endif


extern int
id3tag_set_textinfo_ucs2(lame_global_flags* gfp, char const* id, unsigned short const* text);

extern int
id3tag_set_comment_ucs2(lame_global_flags* gfp, char const* lng, unsigned short const* desc, unsigned short const* text);

/* possible text encodings */
typedef enum TextEncoding
{ TENC_RAW     /* bytes will be stored as-is into ID3 tags, which are Latin1/UCS2 per definition */
, TENC_LATIN1  /* text will be converted from local encoding to Latin1, as ID3 needs it */
, TENC_UCS2    /* text will be converted from local encoding to UCS-2, as ID3v2 wants it */
} TextEncoding;

#ifdef HAVE_ICONV

/* search for Zero termination in multi-byte strings */
static size_t
strlenMultiByte(char const* str, size_t w)
{    
    size_t n = 0;
    if (str != 0) {
        size_t i, x = 0;
        for (n = 0; ; ++n) {
            x = 0;
            for (i = 0; i < w; ++i) {
                x += *str++ == 0 ? 1 : 0;
            }
            if (x == w) {
                break;
            }
        }
    }
    return n;
}


static size_t
currCharCodeSize(void)
{
    size_t n = 1;
    char dst[32];
    char* src = "A";
    char* env_lang = getenv("LANG");
    char* xxx_code = env_lang == NULL ? NULL : strrchr(env_lang, '.');
    char* cur_code = xxx_code == NULL ? "" : xxx_code+1;
    iconv_t xiconv = iconv_open(cur_code, "ISO_8859-1");
    if (xiconv != (iconv_t)-1) {
        for (n = 0; n < 32; ++n) {
            char* i_ptr = src;
            char* o_ptr = dst;
            size_t srcln = 1;
            size_t avail = n;
            size_t rc = iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
            if (rc != (size_t)-1) {
                break;
            }
        }
        iconv_close(xiconv);
    }
    return n;
}

#if 0
static
char* fromLatin1( char* src )
{
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlen(src);
        size_t const n = l*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* env_lang = getenv("LANG");
            char* xxx_code = env_lang == NULL ? NULL : strrchr(env_lang, '.');
            char* cur_code = xxx_code == NULL ? "" : xxx_code+1;
            iconv_t xiconv = iconv_open(cur_code, "ISO_8859-1");
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = src;
                char* o_ptr = dst;
                size_t srcln = l;
                size_t avail = n;                
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}

static
char* fromUcs2( char* src )
{
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlenMultiByte(src, 2);
        size_t const n = l*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* env_lang = getenv("LANG");
            char* xxx_code = env_lang == NULL ? NULL : strrchr(env_lang, '.');
            char* cur_code = xxx_code == NULL ? "" : xxx_code+1;
            iconv_t xiconv = iconv_open(cur_code, "UCS-2LE");
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = (char*)src;
                char* o_ptr = dst;
                size_t srcln = l*2;
                size_t avail = n;                
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}
#endif

static
char* toLatin1( char* src )
{
    size_t w = currCharCodeSize();
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlenMultiByte(src, w);
        size_t const n = l*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* env_lang = getenv("LANG");
            char* xxx_code = env_lang == NULL ? NULL : strrchr(env_lang, '.');
            char* cur_code = xxx_code == NULL ? "" : xxx_code+1;
            iconv_t xiconv = iconv_open("ISO_8859-1", cur_code);
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = (char*)src;
                char* o_ptr = dst;
                size_t srcln = l*w;
                size_t avail = n;                
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}


static
char* toUcs2( char* src )
{
    size_t w = currCharCodeSize();
    char* dst = 0;
    if (src != 0) {
        size_t const l = strlenMultiByte(src, w);
        size_t const n = (l+1)*4;
        dst = calloc(n+4, 4);
        if (dst != 0) {
            char* env_lang = getenv("LANG");
            char* xxx_code = env_lang == NULL ? NULL : strrchr(env_lang, '.');
            char* cur_code = xxx_code == NULL ? "" : xxx_code+1;
            iconv_t xiconv = iconv_open("UCS-2LE", cur_code);
            dst[0] = 0xff;
            dst[1] = 0xfe;
            if (xiconv != (iconv_t)-1) {
                char* i_ptr = (char*)src;
                char* o_ptr = &dst[2];
                size_t srcln = l*w;
                size_t avail = n;                
                iconv(xiconv, &i_ptr, &srcln, &o_ptr, &avail);
                iconv_close(xiconv);
            }
        }
    }
    return dst;
}


static int
set_id3v2tag(lame_global_flags* gfp, int type, unsigned short const* str)
{
    switch (type)
    {
        case 'a': return id3tag_set_textinfo_ucs2(gfp, "TPE1", str);
        case 't': return id3tag_set_textinfo_ucs2(gfp, "TIT2", str);
        case 'l': return id3tag_set_textinfo_ucs2(gfp, "TALB", str);
        case 'g': return id3tag_set_textinfo_ucs2(gfp, "TCON", str);
        case 'c': return id3tag_set_comment_ucs2(gfp, 0, 0, str);
        case 'n': return id3tag_set_textinfo_ucs2(gfp, "TRCK", str);
    }
    return 0;
}
#endif

static int
set_id3tag(lame_global_flags* gfp, int type, char const* str)
{
    switch (type)
    {
        case 'a': return id3tag_set_artist(gfp, str), 0;
        case 't': return id3tag_set_title(gfp, str), 0;
        case 'l': return id3tag_set_album(gfp, str), 0;
        case 'g': return id3tag_set_genre(gfp, str);
        case 'c': return id3tag_set_comment(gfp, str), 0;
        case 'n': return id3tag_set_track(gfp, str);
        case 'y': return id3tag_set_year(gfp, str), 0;
        case 'v': return id3tag_set_fieldvalue(gfp, str);
    }
    return 0;
}

static int
id3_tag(lame_global_flags* gfp, int type, TextEncoding enc, char* str)
{
    void* x = 0;
    int result;
    switch (enc) 
    {
        default:
        case TENC_RAW:    x = strdup(str);   break;
#ifdef HAVE_ICONV
        case TENC_LATIN1: x = toLatin1(str); break;
        case TENC_UCS2:   x = toUcs2(str);   break;
#endif
    }
    switch (enc)
    {
        default:
        case TENC_RAW:
        case TENC_LATIN1: result = set_id3tag(gfp, type, x);   break;
#ifdef HAVE_ICONV
        case TENC_UCS2:   result = set_id3v2tag(gfp, type, x); break;
#endif
    }
    free(x);
    return result;
}




/************************************************************************
*
* license
*
* PURPOSE:  Writes version and license to the file specified by fp
*
************************************************************************/

static int
lame_version_print(FILE * const fp)
{
    const char *b = get_lame_os_bitness();
    const char *v = get_lame_version();
    const char *u = get_lame_url();
    const size_t lenb = strlen(b);
    const size_t lenv = strlen(v);
    const size_t lenu = strlen(u);
    const size_t lw = 80;       /* line width of terminal in characters */
    const size_t sw = 16;       /* static width of text */

    if (lw >= lenb + lenv + lenu + sw || lw < lenu + 2)
        /* text fits in 80 chars per line, or line even too small for url */
        if (lenb > 0)
            fprintf(fp, "LAME %s version %s (%s)\n\n", b, v, u);
        else
            fprintf(fp, "LAME version %s (%s)\n\n", v, u);
    else
        /* text too long, wrap url into next line, right aligned */
    if (lenb > 0)
        fprintf(fp, "LAME %s version %s\n%*s(%s)\n\n", b, v, lw - 2 - lenu, "", u);
    else
        fprintf(fp, "LAME version %s\n%*s(%s)\n\n", v, lw - 2 - lenu, "", u);

    if (LAME_ALPHA_VERSION)
        fprintf(fp, "warning: alpha versions should be used for testing only\n\n");


    return 0;
}

static int
print_license(FILE * const fp)
{                       /* print version & license */
    lame_version_print(fp);
    fprintf(fp,
            "Can I use LAME in my commercial program?\n"
            "\n"
            "Yes, you can, under the restrictions of the LGPL.  In particular, you\n"
            "can include a compiled version of the LAME library (for example,\n"
            "lame.dll) with a commercial program.  Some notable requirements of\n"
            "the LGPL:\n" "\n");
    fprintf(fp,
            "1. In your program, you cannot include any source code from LAME, with\n"
            "   the exception of files whose only purpose is to describe the library\n"
            "   interface (such as lame.h).\n" "\n");
    fprintf(fp,
            "2. Any modifications of LAME must be released under the LGPL.\n"
            "   The LAME project (www.mp3dev.org) would appreciate being\n"
            "   notified of any modifications.\n" "\n");
    fprintf(fp,
            "3. You must give prominent notice that your program is:\n"
            "      A. using LAME (including version number)\n"
            "      B. LAME is under the LGPL\n"
            "      C. Provide a copy of the LGPL.  (the file COPYING contains the LGPL)\n"
            "      D. Provide a copy of LAME source, or a pointer where the LAME\n"
            "         source can be obtained (such as www.mp3dev.org)\n"
            "   An example of prominent notice would be an \"About the LAME encoding engine\"\n"
            "   button in some pull down menu within the executable of your program.\n" "\n");
    fprintf(fp,
            "4. If you determine that distribution of LAME requires a patent license,\n"
            "   you must obtain such license.\n" "\n" "\n");
    fprintf(fp,
            "*** IMPORTANT NOTE ***\n"
            "\n"
            "The decoding functions provided in LAME use the mpglib decoding engine which\n"
            "is under the GPL.  They may not be used by any program not released under the\n"
            "GPL unless you obtain such permission from the MPG123 project (www.mpg123.de).\n"
            "\n");
    return 0;
}


/************************************************************************
*
* usage
*
* PURPOSE:  Writes command line syntax to the file specified by fp
*
************************************************************************/

int
usage(FILE * const fp, const char *ProgramName)
{                       /* print general syntax */
    lame_version_print(fp);
    fprintf(fp,
            "usage: %s [options] <infile> [outfile]\n"
            "\n"
            "    <infile> and/or <outfile> can be \"-\", which means stdin/stdout.\n"
            "\n"
            "Try:\n"
            "     \"%s --help\"           for general usage information\n"
            " or:\n"
            "     \"%s --preset help\"    for information on suggested predefined settings\n"
            " or:\n"
            "     \"%s --longhelp\"\n"
            "  or \"%s -?\"              for a complete options list\n\n",
            ProgramName, ProgramName, ProgramName, ProgramName, ProgramName);
    return 0;
}


/************************************************************************
*
* usage
*
* PURPOSE:  Writes command line syntax to the file specified by fp
*           but only the most important ones, to fit on a vt100 terminal
*
************************************************************************/

int
short_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName)
{                       /* print short syntax help */
    lame_version_print(fp);
    fprintf(fp,
            "usage: %s [options] <infile> [outfile]\n"
            "\n"
            "    <infile> and/or <outfile> can be \"-\", which means stdin/stdout.\n"
            "\n" "RECOMMENDED:\n" "    lame -V2 input.wav output.mp3\n" "\n", ProgramName);
    fprintf(fp,
            "OPTIONS:\n"
            "    -b bitrate      set the bitrate, default 128 kbps\n"
            "    -h              higher quality, but a little slower.  Recommended.\n"
            "    -f              fast mode (lower quality)\n"
            "    -V n            quality setting for VBR.  default n=%i\n"
            "                    0=high quality,bigger files. 9=smaller files\n",
            lame_get_VBR_q(gfp));
    fprintf(fp,
            "    --preset type   type must be \"medium\", \"standard\", \"extreme\", \"insane\",\n"
            "                    or a value for an average desired bitrate and depending\n"
            "                    on the value specified, appropriate quality settings will\n"
            "                    be used.\n"
            "                    \"--preset help\" gives more info on these\n" "\n");
    fprintf(fp,
#if defined(WIN32)
            "    --priority type  sets the process priority\n"
            "                     0,1 = Low priority\n"
            "                     2   = normal priority\n"
            "                     3,4 = High priority\n" "\n"
#endif
#if defined(__OS2__)
            "    --priority type  sets the process priority\n"
            "                     0 = Low priority\n"
            "                     1 = Medium priority\n"
            "                     2 = Regular priority\n"
            "                     3 = High priority\n"
            "                     4 = Maximum priority\n" "\n"
#endif
            "    --longhelp      full list of options\n" "\n"
            "    --license       print License information\n\n"
            );

    return 0;
}

/************************************************************************
*
* usage
*
* PURPOSE:  Writes command line syntax to the file specified by fp
*
************************************************************************/

static void
wait_for(FILE * const fp, int lessmode)
{
    if (lessmode) {
        fflush(fp);
        getchar();
    }
    else {
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

int
long_help(const lame_global_flags * gfp, FILE * const fp, const char *ProgramName, int lessmode)
{                       /* print long syntax help */
    lame_version_print(fp);
    fprintf(fp,
            "usage: %s [options] <infile> [outfile]\n"
            "\n"
            "    <infile> and/or <outfile> can be \"-\", which means stdin/stdout.\n"
            "\n" "RECOMMENDED:\n" "    lame -V2 input.wav output.mp3\n" "\n", ProgramName);
    fprintf(fp,
            "OPTIONS:\n"
            "  Input options:\n"
            "    --scale <arg>   scale input (multiply PCM data) by <arg>\n"
            "    --scale-l <arg> scale channel 0 (left) input (multiply PCM data) by <arg>\n"
            "    --scale-r <arg> scale channel 1 (right) input (multiply PCM data) by <arg>\n"
#if (defined HAVE_MPGLIB || defined AMIGA_MPEGA)
            "    --mp1input      input file is a MPEG Layer I   file\n"
            "    --mp2input      input file is a MPEG Layer II  file\n"
            "    --mp3input      input file is a MPEG Layer III file\n"
#endif
            "    --nogap <file1> <file2> <...>\n"
            "                    gapless encoding for a set of contiguous files\n"
            "    --nogapout <dir>\n"
            "                    output dir for gapless encoding (must precede --nogap)\n"
            "    --nogaptags     allow the use of VBR tags in gapless encoding\n"
           );
    fprintf(fp,
            "\n"
            "  Input options for RAW PCM:\n"
            "    -r              input is raw pcm\n"
            "    -x              force byte-swapping of input\n"
            "    -s sfreq        sampling frequency of input file (kHz) - default 44.1 kHz\n"
            "    --bitwidth w    input bit width is w (default 16)\n"
            "    --signed        input is signed (default)\n"
            "    --unsigned      input is unsigned\n"
            "    --little-endian input is little-endian (default)\n"
            "    --big-endian    input is big-endian\n"
           );

    wait_for(fp, lessmode);
    fprintf(fp,
            "  Operational options:\n"
            "    -a              downmix from stereo to mono file for mono encoding\n"
            "    -m <mode>       (j)oint, (s)imple, (f)orce, (d)dual-mono, (m)ono\n"
            "                    default is (j) or (s) depending on bitrate\n"
            "                    joint  = joins the best possible of MS and LR stereo\n"
            "                    simple = force LR stereo on all frames\n"
            "                    force  = force MS stereo on all frames.\n"
            "    --preset type   type must be \"medium\", \"standard\", \"extreme\", \"insane\",\n"
            "                    or a value for an average desired bitrate and depending\n"
            "                    on the value specified, appropriate quality settings will\n"
            "                    be used.\n"
            "                    \"--preset help\" gives more info on these\n"
            "    --comp  <arg>   choose bitrate to achive a compression ratio of <arg>\n");
    fprintf(fp, "    --replaygain-fast   compute RG fast but slightly inaccurately (default)\n"
#ifdef DECODE_ON_THE_FLY
            "    --replaygain-accurate   compute RG more accurately and find the peak sample\n"
#endif
            "    --noreplaygain  disable ReplayGain analysis\n"
#ifdef DECODE_ON_THE_FLY
            "    --clipdetect    enable --replaygain-accurate and print a message whether\n"
            "                    clipping occurs and how far the waveform is from full scale\n"
#endif
        );
    fprintf(fp,
            "    --flush         flush output stream as soon as possible\n"
            "    --freeformat    produce a free format bitstream\n"
            "    --decode        input=mp3 file, output=wav\n"
            "    -t              disable writing wav header when using --decode\n");

    wait_for(fp, lessmode);
    fprintf(fp,
            "  Verbosity:\n"
            "    --disptime <arg>print progress report every arg seconds\n"
            "    -S              don't print progress report, VBR histograms\n"
            "    --nohist        disable VBR histogram display\n"
            "    --silent        don't print anything on screen\n"
            "    --quiet         don't print anything on screen\n"
            "    --brief         print more useful information\n"
            "    --verbose       print a lot of useful information\n" "\n");
    fprintf(fp,
            "  Noise shaping & psycho acoustic algorithms:\n"
            "    -q <arg>        <arg> = 0...9.  Default  -q 5 \n"
            "                    -q 0:  Highest quality, very slow \n"
            "                    -q 9:  Poor quality, but fast \n"
            "    -h              Same as -q 2.   Recommended.\n"
            "    -f              Same as -q 7.   Fast, ok quality\n");

    wait_for(fp, lessmode);
    fprintf(fp,
            "  CBR (constant bitrate, the default) options:\n"
            "    -b <bitrate>    set the bitrate in kbps, default 128 kbps\n"
            "    --cbr           enforce use of constant bitrate\n"
            "\n"
            "  ABR options:\n"
            "    --abr <bitrate> specify average bitrate desired (instead of quality)\n" "\n");
    fprintf(fp,
            "  VBR options:\n"
            "    -V n            quality setting for VBR.  default n=%i\n"
            "                    0=high quality,bigger files. 9=smaller files\n"
            "    -v              the same as -V 4\n"
            "    --vbr-old       use old variable bitrate (VBR) routine\n"
            "    --vbr-new       use new variable bitrate (VBR) routine (default)\n"
            ,
            lame_get_VBR_q(gfp));
    fprintf(fp,
            "    -b <bitrate>    specify minimum allowed bitrate, default  32 kbps\n"
            "    -B <bitrate>    specify maximum allowed bitrate, default 320 kbps\n"
            "    -F              strictly enforce the -b option, for use with players that\n"
            "                    do not support low bitrate mp3\n"
            "    -t              disable writing LAME Tag\n"
            "    -T              enable and force writing LAME Tag\n");

    wait_for(fp, lessmode);
    DEV_HELP(fprintf(fp,
                     "  ATH related:\n"
                     "    --noath         turns ATH down to a flat noise floor\n"
                     "    --athshort      ignore GPSYCHO for short blocks, use ATH only\n"
                     "    --athonly       ignore GPSYCHO completely, use ATH only\n"
                     "    --athtype n     selects between different ATH types [0-4]\n"
                     "    --athlower x    lowers ATH by x dB\n");
             fprintf(fp, "    --athaa-type n  ATH auto adjust: 0 'no' else 'loudness based'\n"
/** OBSOLETE "    --athaa-loudapprox n   n=1 total energy or n=2 equal loudness curve\n"*/
                     "    --athaa-sensitivity x  activation offset in -/+ dB for ATH auto-adjustment\n"
                     "\n");
        )
        fprintf(fp,
                "  PSY related:\n"
                DEV_HELP(
                "    --short         use short blocks when appropriate\n"
                "    --noshort       do not use short blocks\n"
                "    --allshort      use only short blocks\n"
                )
        );
    fprintf(fp,
            "    --temporal-masking x   x=0 disables, x=1 enables temporal masking effect\n"
            "    --nssafejoint   M/S switching criterion\n"
            "    --nsmsfix <arg> M/S switching tuning [effective 0-3.5]\n"
            "    --interch x     adjust inter-channel masking ratio\n"
            "    --ns-bass x     adjust masking for sfbs  0 -  6 (long)  0 -  5 (short)\n"
            "    --ns-alto x     adjust masking for sfbs  7 - 13 (long)  6 - 10 (short)\n"
            "    --ns-treble x   adjust masking for sfbs 14 - 21 (long) 11 - 12 (short)\n");
    fprintf(fp,
            "    --ns-sfb21 x    change ns-treble by x dB for sfb21\n"
            DEV_HELP("    --shortthreshold x,y  short block switching threshold,\n"
                     "                          x for L/R/M channel, y for S channel\n"
                     "  Noise Shaping related:\n"
                     "    --substep n     use pseudo substep noise shaping method types 0-2\n")
        );

    wait_for(fp, lessmode);

    fprintf(fp,
            "  experimental switches:\n"
            DEV_HELP(
            "    -X n[,m]        selects between different noise measurements\n"
            "                    n for long block, m for short. if m is omitted, m = n\n"
            )
            "    -Y              lets LAME ignore noise in sfb21, like in CBR\n"
            DEV_HELP(
            "    -Z [n]          currently no effects\n"
            )
            );

    wait_for(fp, lessmode);

    fprintf(fp,
            "  MP3 header/stream options:\n"
            "    -e <emp>        de-emphasis n/5/c  (obsolete)\n"
            "    -c              mark as copyright\n"
            "    -o              mark as non-original\n"
            "    -p              error protection.  adds 16 bit checksum to every frame\n"
            "                    (the checksum is computed correctly)\n"
            "    --nores         disable the bit reservoir\n"
            "    --strictly-enforce-ISO   comply as much as possible to ISO MPEG spec\n" "\n");
    fprintf(fp,
            "  Filter options:\n"
            "  --lowpass <freq>        frequency(kHz), lowpass filter cutoff above freq\n"
            "  --lowpass-width <freq>  frequency(kHz) - default 15%% of lowpass freq\n"
            "  --highpass <freq>       frequency(kHz), highpass filter cutoff below freq\n"
            "  --highpass-width <freq> frequency(kHz) - default 15%% of highpass freq\n");
    fprintf(fp,
            "  --resample <sfreq>  sampling frequency of output file(kHz)- default=automatic\n");

    wait_for(fp, lessmode);
    fprintf(fp,
            "  ID3 tag options:\n"
            "    --tt <title>    audio/song title (max 30 chars for version 1 tag)\n"
            "    --ta <artist>   audio/song artist (max 30 chars for version 1 tag)\n"
            "    --tl <album>    audio/song album (max 30 chars for version 1 tag)\n"
            "    --ty <year>     audio/song year of issue (1 to 9999)\n"
            "    --tc <comment>  user-defined text (max 30 chars for v1 tag, 28 for v1.1)\n"
            "    --tn <track[/total]>   audio/song track number and (optionally) the total\n"
            "                           number of tracks on the original recording. (track\n"
            "                           and total each 1 to 255. just the track number\n"
            "                           creates v1.1 tag, providing a total forces v2.0).\n"
            "    --tg <genre>    audio/song genre (name or number in list)\n"
            "    --ti <file>     audio/song albumArt (jpeg/png/gif file, 128KB max, v2.3)\n"
            "    --tv <id=value> user-defined frame specified by id and value (v2.3 tag)\n");
    fprintf(fp,
            "    --add-id3v2     force addition of version 2 tag\n"
            "    --id3v1-only    add only a version 1 tag\n"
            "    --id3v2-only    add only a version 2 tag\n"
            "    --space-id3v1   pad version 1 tag with spaces instead of nulls\n"
            "    --pad-id3v2     same as '--pad-id3v2-size 128'\n"
            "    --pad-id3v2-size <value> adds version 2 tag, pad with extra <value> bytes\n"
            "    --genre-list    print alphabetically sorted ID3 genre list and exit\n"
            "    --ignore-tag-errors  ignore errors in values passed for tags\n" "\n");
    fprintf(fp,
            "    Note: A version 2 tag will NOT be added unless one of the input fields\n"
            "    won't fit in a version 1 tag (e.g. the title string is longer than 30\n"
            "    characters), or the '--add-id3v2' or '--id3v2-only' options are used,\n"
            "    or output is redirected to stdout.\n"
#if defined(WIN32)
            "\n\nMS-Windows-specific options:\n"
            "    --priority <type>  sets the process priority:\n"
            "                         0,1 = Low priority (IDLE_PRIORITY_CLASS)\n"
            "                         2 = normal priority (NORMAL_PRIORITY_CLASS, default)\n"
            "                         3,4 = High priority (HIGH_PRIORITY_CLASS))\n"
            "    Note: Calling '--priority' without a parameter will select priority 0.\n"
#endif
#if defined(__OS2__)
            "\n\nOS/2-specific options:\n"
            "    --priority <type>  sets the process priority:\n"
            "                         0 = Low priority (IDLE, delta = 0)\n"
            "                         1 = Medium priority (IDLE, delta = +31)\n"
            "                         2 = Regular priority (REGULAR, delta = -31)\n"
            "                         3 = High priority (REGULAR, delta = 0)\n"
            "                         4 = Maximum priority (REGULAR, delta = +31)\n"
            "    Note: Calling '--priority' without a parameter will select priority 0.\n"
#endif
            "\nMisc:\n    --license       print License information\n\n"
        );

#if defined(HAVE_NASM)
    wait_for(fp, lessmode);
    fprintf(fp,
            "  Platform specific:\n"
            "    --noasm <instructions> disable assembly optimizations for mmx/3dnow/sse\n");
    wait_for(fp, lessmode);
#endif

    display_bitrates(fp);

    return 0;
}

static void
display_bitrate(FILE * const fp, const char *const version, const int d, const int indx)
{
    int     i;
    int nBitrates = 14;
    if (d == 4)
        nBitrates = 8;


    fprintf(fp,
            "\nMPEG-%-3s layer III sample frequencies (kHz):  %2d  %2d  %g\n"
            "bitrates (kbps):", version, 32 / d, 48 / d, 44.1 / d);
    for (i = 1; i <= nBitrates; i++)
        fprintf(fp, " %2i", bitrate_table[indx][i]);
    fprintf(fp, "\n");
}

int
display_bitrates(FILE * const fp)
{
    display_bitrate(fp, "1", 1, 1);
    display_bitrate(fp, "2", 2, 0);
    display_bitrate(fp, "2.5", 4, 0);
    fprintf(fp, "\n");
    fflush(fp);
    return 0;
}


/*  note: for presets it would be better to externalize them in a file.
    suggestion:  lame --preset <file-name> ...
            or:  lame --preset my-setting  ... and my-setting is defined in lame.ini
 */

/*
Note from GB on 08/25/2002:
I am merging --presets and --alt-presets. Old presets are now aliases for
corresponding abr values from old alt-presets. This way we now have a
unified preset system, and I hope than more people will use the new tuned
presets instead of the old unmaintained ones.
*/



/************************************************************************
*
* usage
*
* PURPOSE:  Writes presetting info to #stdout#
*
************************************************************************/


static void
presets_longinfo_dm(FILE * msgfp)
{
    fprintf(msgfp,
            "\n"
            "The --preset switches are aliases over LAME settings.\n"
            "\n" "\n");
    fprintf(msgfp,
            "To activate these presets:\n"
            "\n" "   For VBR modes (generally highest quality):\n" "\n");
    fprintf(msgfp,
            "     \"--preset medium\" This preset should provide near transparency\n"
            "                             to most people on most music.\n"
            "\n"
            "     \"--preset standard\" This preset should generally be transparent\n"
            "                             to most people on most music and is already\n"
            "                             quite high in quality.\n" "\n");
    fprintf(msgfp,
            "     \"--preset extreme\" If you have extremely good hearing and similar\n"
            "                             equipment, this preset will generally provide\n"
            "                             slightly higher quality than the \"standard\"\n"
            "                             mode.\n" "\n");
    fprintf(msgfp,
            "   For CBR 320kbps (highest quality possible from the --preset switches):\n"
            "\n"
            "     \"--preset insane\"  This preset will usually be overkill for most\n"
            "                             people and most situations, but if you must\n"
            "                             have the absolute highest quality with no\n"
            "                             regard to filesize, this is the way to go.\n" "\n");
    fprintf(msgfp,
            "   For ABR modes (high quality per given bitrate but not as high as VBR):\n"
            "\n"
            "     \"--preset <kbps>\"  Using this preset will usually give you good\n"
            "                             quality at a specified bitrate. Depending on the\n"
            "                             bitrate entered, this preset will determine the\n");
    fprintf(msgfp,
            "                             optimal settings for that particular situation.\n"
            "                             While this approach works, it is not nearly as\n"
            "                             flexible as VBR, and usually will not attain the\n"
            "                             same level of quality as VBR at higher bitrates.\n" "\n");
    fprintf(msgfp,
            "The following options are also available for the corresponding profiles:\n"
            "\n"
            "   <fast>        standard\n"
            "   <fast>        extreme\n"
            "                 insane\n"
            "   <cbr> (ABR Mode) - The ABR Mode is implied. To use it,\n"
            "                      simply specify a bitrate. For example:\n"
            "                      \"--preset 185\" activates this\n"
            "                      preset and uses 185 as an average kbps.\n" "\n");
    fprintf(msgfp,
            "   \"fast\" - Enables the fast VBR mode for a particular profile.\n" "\n");
    fprintf(msgfp,
            "   \"cbr\"  - If you use the ABR mode (read above) with a significant\n"
            "            bitrate such as 80, 96, 112, 128, 160, 192, 224, 256, 320,\n"
            "            you can use the \"cbr\" option to force CBR mode encoding\n"
            "            instead of the standard abr mode. ABR does provide higher\n"
            "            quality but CBR may be useful in situations such as when\n"
            "            streaming an mp3 over the internet may be important.\n" "\n");
    fprintf(msgfp,
            "    For example:\n"
            "\n"
            "    \"--preset fast standard <input file> <output file>\"\n"
            " or \"--preset cbr 192 <input file> <output file>\"\n"
            " or \"--preset 172 <input file> <output file>\"\n"
            " or \"--preset extreme <input file> <output file>\"\n" "\n" "\n");
    fprintf(msgfp,
            "A few aliases are also available for ABR mode:\n"
            "phone => 16kbps/mono        phon+/lw/mw-eu/sw => 24kbps/mono\n"
            "mw-us => 40kbps/mono        voice => 56kbps/mono\n"
            "fm/radio/tape => 112kbps    hifi => 160kbps\n"
            "cd => 192kbps               studio => 256kbps\n");
}


extern void lame_set_msfix(lame_t gfp, double msfix);



static int
presets_set(lame_t gfp, int fast, int cbr, const char *preset_name, const char *ProgramName)
{
    int     mono = 0;

    if ((strcmp(preset_name, "help") == 0) && (fast < 1)
        && (cbr < 1)) {
        lame_version_print(stdout);
        presets_longinfo_dm(stdout);
        return -1;
    }



    /*aliases for compatibility with old presets */

    if (strcmp(preset_name, "phone") == 0) {
        preset_name = "16";
        mono = 1;
    }
    if ((strcmp(preset_name, "phon+") == 0) ||
        (strcmp(preset_name, "lw") == 0) ||
        (strcmp(preset_name, "mw-eu") == 0) || (strcmp(preset_name, "sw") == 0)) {
        preset_name = "24";
        mono = 1;
    }
    if (strcmp(preset_name, "mw-us") == 0) {
        preset_name = "40";
        mono = 1;
    }
    if (strcmp(preset_name, "voice") == 0) {
        preset_name = "56";
        mono = 1;
    }
    if (strcmp(preset_name, "fm") == 0) {
        preset_name = "112";
    }
    if ((strcmp(preset_name, "radio") == 0) || (strcmp(preset_name, "tape") == 0)) {
        preset_name = "112";
    }
    if (strcmp(preset_name, "hifi") == 0) {
        preset_name = "160";
    }
    if (strcmp(preset_name, "cd") == 0) {
        preset_name = "192";
    }
    if (strcmp(preset_name, "studio") == 0) {
        preset_name = "256";
    }

    if (strcmp(preset_name, "medium") == 0) {
        lame_set_VBR_q(gfp, 4);
        if (fast > 0) {
            lame_set_VBR(gfp, vbr_mtrh);
        }
        else {
            lame_set_VBR(gfp, vbr_rh);
        }
        return 0;
    }

    if (strcmp(preset_name, "standard") == 0) {
        lame_set_VBR_q(gfp, 2);
        if (fast > 0) {
            lame_set_VBR(gfp, vbr_mtrh);
        }
        else {
            lame_set_VBR(gfp, vbr_rh);
        }
        return 0;
    }

    else if (strcmp(preset_name, "extreme") == 0) {
        lame_set_VBR_q(gfp, 0);
        if (fast > 0) {
            lame_set_VBR(gfp, vbr_mtrh);
        }
        else {
            lame_set_VBR(gfp, vbr_rh);
        }
        return 0;
    }

    else if ((strcmp(preset_name, "insane") == 0) && (fast < 1)) {

        lame_set_preset(gfp, INSANE);

        return 0;
    }

    /* Generic ABR Preset */
    if (((atoi(preset_name)) > 0) && (fast < 1)) {
        if ((atoi(preset_name)) >= 8 && (atoi(preset_name)) <= 320) {
            lame_set_preset(gfp, atoi(preset_name));

            if (cbr == 1)
                lame_set_VBR(gfp, vbr_off);

            if (mono == 1) {
                lame_set_mode(gfp, MONO);
            }

            return 0;

        }
        else {
            lame_version_print(Console_IO.Error_fp);
            error_printf("Error: The bitrate specified is out of the valid range for this preset\n"
                         "\n"
                         "When using this mode you must enter a value between \"32\" and \"320\"\n"
                         "\n" "For further information try: \"%s --preset help\"\n", ProgramName);
            return -1;
        }
    }

    lame_version_print(Console_IO.Error_fp);
    error_printf("Error: You did not enter a valid profile and/or options with --preset\n"
                 "\n"
                 "Available profiles are:\n"
                 "\n"
                 "   <fast>        medium\n"
                 "   <fast>        standard\n"
                 "   <fast>        extreme\n"
                 "                 insane\n"
                 "          <cbr> (ABR Mode) - The ABR Mode is implied. To use it,\n"
                 "                             simply specify a bitrate. For example:\n"
                 "                             \"--preset 185\" activates this\n"
                 "                             preset and uses 185 as an average kbps.\n" "\n");
    error_printf("    Some examples:\n"
                 "\n"
                 " or \"%s --preset fast standard <input file> <output file>\"\n"
                 " or \"%s --preset cbr 192 <input file> <output file>\"\n"
                 " or \"%s --preset 172 <input file> <output file>\"\n"
                 " or \"%s --preset extreme <input file> <output file>\"\n"
                 "\n"
                 "For further information try: \"%s --preset help\"\n", ProgramName, ProgramName,
                 ProgramName, ProgramName, ProgramName);
    return -1;
}

static void
genre_list_handler(int num, const char *name, void *cookie)
{
    (void) cookie;
    console_printf("%3d %s\n", num, name);
}


/************************************************************************
*
* parse_args
*
* PURPOSE:  Sets encoding parameters to the specifications of the
* command line.  Default settings are used for parameters
* not specified in the command line.
*
* If the input file is in WAVE or AIFF format, the sampling frequency is read
* from the AIFF header.
*
* The input and output filenames are read into #inpath# and #outpath#.
*
************************************************************************/

/* would use real "strcasecmp" but it isn't portable */
static int
local_strcasecmp(const char *s1, const char *s2)
{
    unsigned char c1;
    unsigned char c2;

    do {
        c1 = tolower(*s1);
        c2 = tolower(*s2);
        if (!c1) {
            break;
        }
        ++s1;
        ++s2;
    } while (c1 == c2);
    return c1 - c2;
}



/* LAME is a simple frontend which just uses the file extension */
/* to determine the file type.  Trying to analyze the file */
/* contents is well beyond the scope of LAME and should not be added. */
static int
filename_to_type(const char *FileName)
{
    size_t  len = strlen(FileName);

    if (len < 4)
        return sf_unknown;

    FileName += len - 4;
    if (0 == local_strcasecmp(FileName, ".mpg"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".mp1"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".mp2"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".mp3"))
        return sf_mp123;
    if (0 == local_strcasecmp(FileName, ".wav"))
        return sf_wave;
    if (0 == local_strcasecmp(FileName, ".aif"))
        return sf_aiff;
    if (0 == local_strcasecmp(FileName, ".raw"))
        return sf_raw;
    if (0 == local_strcasecmp(FileName, ".ogg"))
        return sf_ogg;
    return sf_unknown;
}

static int
resample_rate(double freq)
{
    if (freq >= 1.e3)
        freq *= 1.e-3;

    switch ((int) freq) {
    case 8:
        return 8000;
    case 11:
        return 11025;
    case 12:
        return 12000;
    case 16:
        return 16000;
    case 22:
        return 22050;
    case 24:
        return 24000;
    case 32:
        return 32000;
    case 44:
        return 44100;
    case 48:
        return 48000;
    default:
        error_printf("Illegal resample frequency: %.3f kHz\n", freq);
        return 0;
    }
}


static int
set_id3_albumart(lame_t gfp, char const* file_name)
{
    int ret = -1;
    FILE *fpi = 0;
    char *albumart = 0;

    if (file_name == 0) {
        return 0;
    }
    fpi = fopen(file_name, "rb");
    if (!fpi) {
        ret = 1;
    }
    else {
        size_t size;

        fseek(fpi, 0, SEEK_END);
        size = ftell(fpi);
        fseek(fpi, 0, SEEK_SET);
        albumart = (char *)malloc(size);
        if (!albumart) {
            ret = 2;            
        }
        else {
            if (fread(albumart, 1, size, fpi) != size) {
                ret = 3;
            }
            else {
                ret = id3tag_set_albumart(gfp, albumart, size) ? 4 : 0;
            }
            free(albumart);
        }
        fclose(fpi);
    }
    switch (ret) {
    case 1: error_printf("Could not find: '%s'.\n", file_name); break;
    case 2: error_printf("Insufficient memory for reading the albumart.\n"); break;
    case 3: error_printf("Read error: '%s'.\n", file_name); break;
    case 4: error_printf("Unsupported image: '%s'.\nSpecify JPEG/PNG/GIF image (128KB maximum)\n", file_name); break;
    default: break;
    }
    return ret;
}


enum ID3TAG_MODE 
{ ID3TAG_MODE_DEFAULT
, ID3TAG_MODE_V1_ONLY
, ID3TAG_MODE_V2_ONLY
};

/* Ugly, NOT final version */

#define T_IF(str)          if ( 0 == local_strcasecmp (token,str) ) {
#define T_ELIF(str)        } else if ( 0 == local_strcasecmp (token,str) ) {
#define T_ELIF_INTERNAL(str)        } else if (INTERNAL_OPTS && (0 == local_strcasecmp (token,str)) ) {
#define T_ELIF2(str1,str2) } else if ( 0 == local_strcasecmp (token,str1)  ||  0 == local_strcasecmp (token,str2) ) {
#define T_ELSE             } else {
#define T_END              }

int
parse_args(lame_global_flags * gfp, int argc, char **argv,
           char *const inPath, char *const outPath, char **nogap_inPath, int *num_nogap)
{
    int     input_file = 0;  /* set to 1 if we parse an input file name  */
    int     i;
    int     autoconvert = 0;
    double  val;
    int     nogap = 0;
    int     nogap_tags = 0;  /* set to 1 to use VBR tags in NOGAP mode */
    const char *ProgramName = argv[0];
    int     count_nogap = 0;
    int     noreplaygain = 0; /* is RG explicitly disabled by the user */
    int     id3tag_mode = ID3TAG_MODE_DEFAULT; 

    inPath[0] = '\0';
    outPath[0] = '\0';
    /* turn on display options. user settings may turn them off below */
    silent = 0;
    ignore_tag_errors = 0;
    brhist = 1;
    mp3_delay = 0;
    mp3_delay_set = 0;
    print_clipping_info = 0;
    disable_wav_header = 0;
    id3tag_init(gfp);

    /* process args */
    for (i = 0; ++i < argc;) {
        char    c;
        char   *token;
        char   *arg;
        char   *nextArg;
        int     argUsed;

        token = argv[i];
        if (*token++ == '-') {
            argUsed = 0;
            nextArg = i + 1 < argc ? argv[i + 1] : "";

            if (!*token) { /* The user wants to use stdin and/or stdout. */
                input_file = 1;
                if (inPath[0] == '\0')
                    strncpy(inPath, argv[i], PATH_MAX + 1);
                else if (outPath[0] == '\0')
                    strncpy(outPath, argv[i], PATH_MAX + 1);
            }
            if (*token == '-') { /* GNU style */
                token++;

                T_IF("resample")
                    argUsed = 1;
                (void) lame_set_out_samplerate(gfp, resample_rate(atof(nextArg)));

                T_ELIF("vbr-old")
                    lame_set_VBR(gfp, vbr_rh);

                T_ELIF("vbr-new")
                    lame_set_VBR(gfp, vbr_mtrh);

                T_ELIF("vbr-mtrh")
                    lame_set_VBR(gfp, vbr_mtrh);

                T_ELIF("cbr")
                    lame_set_VBR(gfp, vbr_off);

                T_ELIF("abr")
                    argUsed = 1;
                lame_set_VBR(gfp, vbr_abr);
                lame_set_VBR_mean_bitrate_kbps(gfp, atoi(nextArg));
                /* values larger than 8000 are bps (like Fraunhofer), so it's strange to get 320000 bps MP3 when specifying 8000 bps MP3 */
                if (lame_get_VBR_mean_bitrate_kbps(gfp) >= 8000)
                    lame_set_VBR_mean_bitrate_kbps(gfp,
                                                   (lame_get_VBR_mean_bitrate_kbps(gfp) +
                                                    500) / 1000);

                lame_set_VBR_mean_bitrate_kbps(gfp, Min(lame_get_VBR_mean_bitrate_kbps(gfp), 320));
                lame_set_VBR_mean_bitrate_kbps(gfp, Max(lame_get_VBR_mean_bitrate_kbps(gfp), 8));

                T_ELIF("r3mix")
                    lame_set_preset(gfp, R3MIX);

                T_ELIF("bitwidth")
                    argUsed = 1;
                in_bitwidth = atoi(nextArg);
                
                T_ELIF("signed")
                    in_signed = 1;

                T_ELIF("unsigned")
                    in_signed = 0;

                T_ELIF("little-endian")
                    in_endian = ByteOrderLittleEndian;

                T_ELIF("big-endian")
                    in_endian = ByteOrderBigEndian;

                T_ELIF("mp1input")
                    input_format = sf_mp1;

                T_ELIF("mp2input")
                    input_format = sf_mp2;

                T_ELIF("mp3input")
                    input_format = sf_mp3;

                T_ELIF("ogginput")
                    error_printf("sorry, vorbis support in LAME is deprecated.\n");
                return -1;

                T_ELIF("phone")
                    if (presets_set(gfp, 0, 0, token, ProgramName) < 0)
                    return -1;
                error_printf("Warning: --phone is deprecated, use --preset phone instead!");

                T_ELIF("voice")
                    if (presets_set(gfp, 0, 0, token, ProgramName) < 0)
                    return -1;
                error_printf("Warning: --voice is deprecated, use --preset voice instead!");

                T_ELIF_INTERNAL("noshort")
                    (void) lame_set_no_short_blocks(gfp, 1);

                T_ELIF_INTERNAL("short")
                    (void) lame_set_no_short_blocks(gfp, 0);

                T_ELIF_INTERNAL("allshort")
                    (void) lame_set_force_short_blocks(gfp, 1);


                T_ELIF("decode")
                    (void) lame_set_decode_only(gfp, 1);

                T_ELIF("flush")
                    flush_write = 1;

                T_ELIF("decode-mp3delay")
                    mp3_delay = atoi(nextArg);
                mp3_delay_set = 1;
                argUsed = 1;

                T_ELIF("nores")
                    lame_set_disable_reservoir(gfp, 1);

                T_ELIF("strictly-enforce-ISO")
                    lame_set_strict_ISO(gfp, 1);

                T_ELIF("scale")
                    argUsed = 1;
                (void) lame_set_scale(gfp, (float) atof(nextArg));

                T_ELIF("scale-l")
                    argUsed = 1;
                (void) lame_set_scale_left(gfp, (float) atof(nextArg));

                T_ELIF("scale-r")
                    argUsed = 1;
                (void) lame_set_scale_right(gfp, (float) atof(nextArg));

                T_ELIF("noasm")
                    argUsed = 1;
                if (!strcmp(nextArg, "mmx"))
                    (void) lame_set_asm_optimizations(gfp, MMX, 0);
                if (!strcmp(nextArg, "3dnow"))
                    (void) lame_set_asm_optimizations(gfp, AMD_3DNOW, 0);
                if (!strcmp(nextArg, "sse"))
                    (void) lame_set_asm_optimizations(gfp, SSE, 0);

                T_ELIF("freeformat")
                    lame_set_free_format(gfp, 1);

                T_ELIF("replaygain-fast")
                    lame_set_findReplayGain(gfp, 1);

#ifdef DECODE_ON_THE_FLY
                T_ELIF("replaygain-accurate")
                    lame_set_decode_on_the_fly(gfp, 1);
                lame_set_findReplayGain(gfp, 1);
#endif

                T_ELIF("noreplaygain")
                    noreplaygain = 1;
                lame_set_findReplayGain(gfp, 0);


#ifdef DECODE_ON_THE_FLY
                T_ELIF("clipdetect")
                    print_clipping_info = 1;
                lame_set_decode_on_the_fly(gfp, 1);
#endif

                T_ELIF("nohist")
                    brhist = 0;

#if defined(__OS2__) || defined(WIN32)
                T_ELIF("priority")
                char   *endptr;
                int     priority = (int) strtol(nextArg, &endptr, 10);
                if (endptr != nextArg) {
                    argUsed = 1;
                }
# if defined(__OS2__)
                setOS2Priority(gfp, priority);
# else /* WIN32 */
                setWin32Priority(gfp, priority);
# endif
#endif

                /* options for ID3 tag */
                T_ELIF("tt")
                    argUsed = 1;
                    id3_tag(gfp, 't', TENC_RAW, nextArg);

                T_ELIF("ta")
                    argUsed = 1;
                id3_tag(gfp, 'a', TENC_RAW, nextArg);

                T_ELIF("tl")
                    argUsed = 1;
                id3_tag(gfp, 'l', TENC_RAW, nextArg);

                T_ELIF("ty")
                    argUsed = 1;
                id3_tag(gfp, 'y', TENC_RAW, nextArg);

                T_ELIF("tc")
                    argUsed = 1;
                id3_tag(gfp, 'c', TENC_RAW, nextArg);

                T_ELIF("tn")
                    int ret = id3_tag(gfp, 'n', TENC_RAW, nextArg);
                    argUsed = 1;
                    if (ret != 0) {
                        if (0 == ignore_tag_errors) {
                            if (id3tag_mode == ID3TAG_MODE_V1_ONLY) {
                                error_printf("The track number has to be between 1 and 255 for ID3v1.\n");
                                return -1;
                            }
                            else if (id3tag_mode == ID3TAG_MODE_V2_ONLY) {
                                /* track will be stored as-is in ID3v2 case, so no problem here */
                            }
                            else {
                                if (silent < 10) {
                                    error_printf("The track number has to be between 1 and 255 for ID3v1, ignored for ID3v1.\n");
                                }
                            }
                        }
                    }

                T_ELIF("tg")
                    int ret = id3_tag(gfp, 'g', TENC_RAW, nextArg);
                    argUsed = 1;
                    if (ret != 0) {
                        if (0 == ignore_tag_errors) {
                            if (ret == -1) {
                                error_printf("Unknown ID3v1 genre number: '%s'.\n", nextArg);
                                return -1;
                            }
                            else if (ret == -2) {
                                if (id3tag_mode == ID3TAG_MODE_V1_ONLY) {
                                    error_printf("Unknown ID3v1 genre: '%s'.\n", nextArg);
                                    return -1;
                                }
                                else if (id3tag_mode == ID3TAG_MODE_V2_ONLY) {
                                    /* genre will be stored as-is in ID3v2 case, so no problem here */
                                }
                                else {
                                    if (silent < 10) {
                                        error_printf("Unknown ID3v1 genre: '%s'.  Setting ID3v1 genre to 'Other'\n", nextArg);
                                    }
                                }
                            }
                            else {
                                error_printf("Internal error.\n");
                                return -1;
                            }
                        }
                    }

                T_ELIF("tv")
                    argUsed = 1;
                    if (id3_tag(gfp, 'v', TENC_RAW, nextArg)) {
                        if (silent < 10) {
                            error_printf("Invalid field value: '%s'. Ignored\n", nextArg);
                        }
                    }

                T_ELIF("ti")
                    argUsed = 1;
                    if (set_id3_albumart(gfp, nextArg) != 0) {
                        if (! ignore_tag_errors) {
                            return -1;
                        }
                    }

                T_ELIF("ignore-tag-errors")
                        ignore_tag_errors = 1;

                T_ELIF("add-id3v2")
                    id3tag_add_v2(gfp);

                T_ELIF("id3v1-only")
                    id3tag_v1_only(gfp);
                    id3tag_mode = ID3TAG_MODE_V1_ONLY;

                T_ELIF("id3v2-only")
                    id3tag_v2_only(gfp);
                    id3tag_mode = ID3TAG_MODE_V2_ONLY;

                T_ELIF("space-id3v1")
                    id3tag_space_v1(gfp);

                T_ELIF("pad-id3v2")
                    id3tag_pad_v2(gfp);

                T_ELIF("pad-id3v2-size")
                    int n = atoi(nextArg);
                    n = n <= 128000 ? n : 128000;
                    n = n >= 0      ? n : 0;
                    id3tag_set_pad(gfp, n);
                    argUsed = 1;


                T_ELIF("genre-list")
                    id3tag_genre_list(genre_list_handler, NULL);
                    return -2;

#ifdef HAVE_ICONV
                    /* some experimental switches for setting ID3 tags
                     * with proper character encodings
                    */
                T_ELIF("lTitle")  argUsed = 1; id3_tag(gfp, 't', TENC_LATIN1, nextArg);
                T_ELIF("lArtist") argUsed = 1; id3_tag(gfp, 'a', TENC_LATIN1, nextArg);
                T_ELIF("lAlbum")  argUsed = 1; id3_tag(gfp, 'l', TENC_LATIN1, nextArg);
                T_ELIF("lGenre")  argUsed = 1; id3_tag(gfp, 'g', TENC_LATIN1, nextArg);
                T_ELIF("lComment")argUsed = 1; id3_tag(gfp, 'c', TENC_LATIN1, nextArg);
                T_ELIF("lFieldvalue")
                    argUsed = 1;
                    if (id3_tag(gfp, 'v', TENC_LATIN1, nextArg)) {
                        if (silent < 10) {
                            error_printf("Invalid field value: '%s'. Ignored\n", nextArg);
                        }
                    }

                T_ELIF("uTitle")  argUsed = 1; id3_tag(gfp, 't', TENC_UCS2, nextArg);
                T_ELIF("uArtist") argUsed = 1; id3_tag(gfp, 'a', TENC_UCS2, nextArg);
                T_ELIF("uAlbum")  argUsed = 1; id3_tag(gfp, 'l', TENC_UCS2, nextArg);
                T_ELIF("uGenre")  argUsed = 1; id3_tag(gfp, 'g', TENC_UCS2, nextArg);
                T_ELIF("uComment")argUsed = 1; id3_tag(gfp, 'c', TENC_UCS2, nextArg);
                /*
                T_ELIF("uFieldvalue")
                    argUsed = 1;
                    if (id3_tag(gfp, 'v', TENC_UCS2, nextArg)) {
                        if (silent < 10) {
                            error_printf("Invalid field value: '%s'. Ignored\n", nextArg);
                        }
                    }
                */
#endif

                T_ELIF("lowpass")
                    val = atof(nextArg);
                argUsed = 1;
                if (val < 0) {
                    lame_set_lowpassfreq(gfp, -1);
                }
                else {
                    /* useful are 0.001 kHz...50 kHz, 50 Hz...50000 Hz */
                    if (val < 0.001 || val > 50000.) {
                        error_printf("Must specify lowpass with --lowpass freq, freq >= 0.001 kHz\n");
                        return -1;
                    }
                    lame_set_lowpassfreq(gfp, (int) (val * (val < 50. ? 1.e3 : 1.e0) + 0.5));
                }
                
                T_ELIF("lowpass-width")
                    val = atof(nextArg);
                argUsed = 1;
                /* useful are 0.001 kHz...16 kHz, 16 Hz...50000 Hz */
                if (val < 0.001 || val > 50000.) {
                    error_printf
                        ("Must specify lowpass width with --lowpass-width freq, freq >= 0.001 kHz\n");
                    return -1;
                }
                lame_set_lowpasswidth(gfp, (int) (val * (val < 16. ? 1.e3 : 1.e0) + 0.5));

                T_ELIF("highpass")
                    val = atof(nextArg);
                argUsed = 1;
                if (val < 0.0) {
                    lame_set_highpassfreq(gfp, -1);
                }
                else {
                    /* useful are 0.001 kHz...16 kHz, 16 Hz...50000 Hz */
                    if (val < 0.001 || val > 50000.) {
                        error_printf("Must specify highpass with --highpass freq, freq >= 0.001 kHz\n");
                        return -1;
                    }
                    lame_set_highpassfreq(gfp, (int) (val * (val < 16. ? 1.e3 : 1.e0) + 0.5));
                }
                
                T_ELIF("highpass-width")
                    val = atof(nextArg);
                argUsed = 1;
                /* useful are 0.001 kHz...16 kHz, 16 Hz...50000 Hz */
                if (val < 0.001 || val > 50000.) {
                    error_printf
                        ("Must specify highpass width with --highpass-width freq, freq >= 0.001 kHz\n");
                    return -1;
                }
                lame_set_highpasswidth(gfp, (int) val);

                T_ELIF("comp")
                    argUsed = 1;
                val = atof(nextArg);
                if (val < 1.0) {
                    error_printf("Must specify compression ratio >= 1.0\n");
                    return -1;
                }
                lame_set_compression_ratio(gfp, (float) val);

                T_ELIF("notemp")
                    (void) lame_set_useTemporal(gfp, 0);

                T_ELIF("interch")
                    argUsed = 1;
                (void) lame_set_interChRatio(gfp, (float) atof(nextArg));

                T_ELIF("temporal-masking")
                    argUsed = 1;
                (void) lame_set_useTemporal(gfp, atoi(nextArg) ? 1 : 0);

                T_ELIF("nspsytune")
                    ;

                T_ELIF("nssafejoint")
                    lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | 2);

                T_ELIF("nsmsfix")
                    argUsed = 1;
                (void) lame_set_msfix(gfp, atof(nextArg));

                T_ELIF("ns-bass")
                    argUsed = 1;
                {
                    double  d;
                    int     k;
                    d = atof(nextArg);
                    k = (int) (d * 4);
                    if (k < -32)
                        k = -32;
                    if (k > 31)
                        k = 31;
                    if (k < 0)
                        k += 64;
                    lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 2));
                }

                T_ELIF("ns-alto")
                    argUsed = 1;
                {
                    double  d;
                    int     k;
                    d = atof(nextArg);
                    k = (int) (d * 4);
                    if (k < -32)
                        k = -32;
                    if (k > 31)
                        k = 31;
                    if (k < 0)
                        k += 64;
                    lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 8));
                }

                T_ELIF("ns-treble")
                    argUsed = 1;
                {
                    double  d;
                    int     k;
                    d = atof(nextArg);
                    k = (int) (d * 4);
                    if (k < -32)
                        k = -32;
                    if (k > 31)
                        k = 31;
                    if (k < 0)
                        k += 64;
                    lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 14));
                }

                T_ELIF("ns-sfb21")
                    /*  to be compatible with Naoki's original code,
                     *  ns-sfb21 specifies how to change ns-treble for sfb21 */
                    argUsed = 1;
                {
                    double  d;
                    int     k;
                    d = atof(nextArg);
                    k = (int) (d * 4);
                    if (k < -32)
                        k = -32;
                    if (k > 31)
                        k = 31;
                    if (k < 0)
                        k += 64;
                    lame_set_exp_nspsytune(gfp, lame_get_exp_nspsytune(gfp) | (k << 20));
                }

                T_ELIF("nspsytune2") {
                }

                /* some more GNU-ish options could be added
                 * brief         => few messages on screen (name, status report)
                 * o/output file => specifies output filename
                 * O             => stdout
                 * i/input file  => specifies input filename
                 * I             => stdin
                 */
                T_ELIF2("quiet", "silent")
                    silent = 10; /* on a scale from 1 to 10 be very silent */

                T_ELIF("brief")
                    silent = -5; /* print few info on screen */

                T_ELIF("verbose")
                    silent = -10; /* print a lot on screen */

                T_ELIF2("version", "license")
                    print_license(stdout);
                return -2;

                T_ELIF2("help", "usage")
                    short_help(gfp, stdout, ProgramName);
                return -2;

                T_ELIF("longhelp")
                    long_help(gfp, stdout, ProgramName, 0 /* lessmode=NO */ );
                return -2;

                T_ELIF("?")
#ifdef __unix__
                FILE   *fp = popen("less -Mqc", "w");
                long_help(gfp, fp, ProgramName, 0 /* lessmode=NO */ );
                pclose(fp);
#else
                    long_help(gfp, stdout, ProgramName, 1 /* lessmode=YES */ );
#endif
                return -2;

                T_ELIF2("preset", "alt-preset")
                    argUsed = 1;
                {
                    int     fast = 0, cbr = 0;

                    while ((strcmp(nextArg, "fast") == 0) || (strcmp(nextArg, "cbr") == 0)) {

                        if ((strcmp(nextArg, "fast") == 0) && (fast < 1))
                            fast = 1;
                        if ((strcmp(nextArg, "cbr") == 0) && (cbr < 1))
                            cbr = 1;

                        argUsed++;
                        nextArg = i + argUsed < argc ? argv[i + argUsed] : "";
                    }

                    if (presets_set(gfp, fast, cbr, nextArg, ProgramName) < 0)
                        return -1;
                }

                T_ELIF("disptime")
                    argUsed = 1;
                update_interval = (float) atof(nextArg);

                T_ELIF("nogaptags")
                    nogap_tags = 1;

                T_ELIF("nogapout")
                    strcpy(outPath, nextArg);
                argUsed = 1;

                T_ELIF("nogap")
                    nogap = 1;


                T_ELIF_INTERNAL("tune") /*without helptext */
                    argUsed = 1;
                {
                    extern void lame_set_tune(lame_t, float);
                    lame_set_tune(gfp, (float) atof(nextArg));
                }

                T_ELIF_INTERNAL("shortthreshold") {
                    float   x, y;
                    int     n = sscanf(nextArg, "%f,%f", &x, &y);
                    if (n == 1) {
                        y = x;
                    }
                    argUsed = 1;
                    (void) lame_set_short_threshold(gfp, x, y);
                }

                T_ELIF_INTERNAL("maskingadjust") /*without helptext */
                    argUsed = 1;
                (void) lame_set_maskingadjust(gfp, (float) atof(nextArg));

                T_ELIF_INTERNAL("maskingadjustshort") /*without helptext */
                    argUsed = 1;
                (void) lame_set_maskingadjust_short(gfp, (float) atof(nextArg));

                T_ELIF_INTERNAL("athcurve") /*without helptext */
                    argUsed = 1;
                (void) lame_set_ATHcurve(gfp, (float) atof(nextArg));

                T_ELIF_INTERNAL("no-preset-tune") /*without helptext */
                    (void) lame_set_preset_notune(gfp, 0);

                T_ELIF_INTERNAL("substep")
                    argUsed = 1;
                (void) lame_set_substep(gfp, atoi(nextArg));

                T_ELIF_INTERNAL("sbgain") /*without helptext */
                    argUsed = 1;
                (void) lame_set_subblock_gain(gfp, atoi(nextArg));

                T_ELIF_INTERNAL("sfscale") /*without helptext */
                    (void) lame_set_sfscale(gfp, 1);

                T_ELIF_INTERNAL("noath")
                    (void) lame_set_noATH(gfp, 1);

                T_ELIF_INTERNAL("athonly")
                    (void) lame_set_ATHonly(gfp, 1);

                T_ELIF_INTERNAL("athshort")
                    (void) lame_set_ATHshort(gfp, 1);

                T_ELIF_INTERNAL("athlower")
                    argUsed = 1;
                (void) lame_set_ATHlower(gfp, (float) atof(nextArg));

                T_ELIF_INTERNAL("athtype")
                    argUsed = 1;
                (void) lame_set_ATHtype(gfp, atoi(nextArg));

                T_ELIF_INTERNAL("athaa-type") /*  switch for developing, no DOCU */
                    argUsed = 1; /* once was 1:Gaby, 2:Robert, 3:Jon, else:off */
                lame_set_athaa_type(gfp, atoi(nextArg)); /* now: 0:off else:Jon */

                T_ELIF ("athaa-sensitivity")
                    argUsed=1;
                lame_set_athaa_sensitivity(gfp, (float) atof(nextArg));

                T_ELIF_INTERNAL("debug-file") /* switch for developing, no DOCU */
                    argUsed = 1; /* file name to print debug info into */
                {
                    set_debug_file(nextArg);
                }

                T_ELSE {
                    error_printf("%s: unrecognized option --%s\n", ProgramName, token);
                    return -1;
                }
                T_END   i += argUsed;

            }
            else {
                while ((c = *token++) != '\0') {
                    arg = *token ? token : nextArg;
                    switch (c) {
                    case 'm':
                        argUsed = 1;

                        switch (*arg) {
                        case 's':
                            (void) lame_set_mode(gfp, STEREO);
                            break;
                        case 'd':
                            (void) lame_set_mode(gfp, DUAL_CHANNEL);
                            break;
                        case 'f':
                            lame_set_force_ms(gfp, 1);
                            /* FALLTHROUGH */
                        case 'j':
                            (void) lame_set_mode(gfp, JOINT_STEREO);
                            break;
                        case 'm':
                            (void) lame_set_mode(gfp, MONO);
                            break;
                        case 'a':
                            (void) lame_set_mode(gfp, JOINT_STEREO);
                            break;
                        default:
                            error_printf("%s: -m mode must be s/d/j/f/m not %s\n", ProgramName,
                                         arg);
                            return -1;
                        }
                        break;

                    case 'V':
                        argUsed = 1;
                        /* to change VBR default look in lame.h */
                        if (lame_get_VBR(gfp) == vbr_off)
                            lame_set_VBR(gfp, vbr_default);
                        lame_set_VBR_quality(gfp, (float)atof(arg));
                        break;
                    case 'v':
                        /* to change VBR default look in lame.h */
                        if (lame_get_VBR(gfp) == vbr_off)
                            lame_set_VBR(gfp, vbr_default);
                        break;

                    case 'q':
                        argUsed = 1;
                        {
                            int     tmp_quality = atoi(arg);

                            /* XXX should we move this into lame_set_quality()? */
                            if (tmp_quality < 0)
                                tmp_quality = 0;
                            if (tmp_quality > 9)
                                tmp_quality = 9;

                            (void) lame_set_quality(gfp, tmp_quality);
                        }
                        break;
                    case 'f':
                        (void) lame_set_quality(gfp, 7);
                        break;
                    case 'h':
                        (void) lame_set_quality(gfp, 2);
                        break;

                    case 's':
                        argUsed = 1;
                        val = atof(arg);
                        (void) lame_set_in_samplerate(gfp,
                                                      (int) (val * (val <= 192 ? 1.e3 : 1.e0) +
                                                             0.5));
                        break;
                    case 'b':
                        argUsed = 1;
                        lame_set_brate(gfp, atoi(arg));
                        lame_set_VBR_min_bitrate_kbps(gfp, lame_get_brate(gfp));
                        break;
                    case 'B':
                        argUsed = 1;
                        lame_set_VBR_max_bitrate_kbps(gfp, atoi(arg));
                        break;
                    case 'F':
                        lame_set_VBR_hard_min(gfp, 1);
                        break;
                    case 't': /* dont write VBR tag */
                        (void) lame_set_bWriteVbrTag(gfp, 0);
                        disable_wav_header = 1;
                        break;
                    case 'T': /* do write VBR tag */
                        (void) lame_set_bWriteVbrTag(gfp, 1);
                        nogap_tags = 1;
                        disable_wav_header = 0;
                        break;
                    case 'r': /* force raw pcm input file */
#if defined(LIBSNDFILE)
                        error_printf
                            ("WARNING: libsndfile may ignore -r and perform fseek's on the input.\n"
                             "Compile without libsndfile if this is a problem.\n");
#endif
                        input_format = sf_raw;
                        break;
                    case 'x': /* force byte swapping */
                        swapbytes = 1;
                        break;
                    case 'p': /* (jo) error_protection: add crc16 information to stream */
                        lame_set_error_protection(gfp, 1);
                        break;
                    case 'a': /* autoconvert input file from stereo to mono - for mono mp3 encoding */
                        autoconvert = 1;
                        (void) lame_set_mode(gfp, MONO);
                        break;
                    case 'd':   /*(void) lame_set_allow_diff_short( gfp, 1 ); */
                    case 'k':   /*lame_set_lowpassfreq(gfp, -1);
                                  lame_set_highpassfreq(gfp, -1); */
                        error_printf("WARNING: -%c is obsolete.\n", c);
                        break;
                    case 'S':
                        silent = 10;
                        break;
                    case 'X':
                        /*  experimental switch -X:
                            the differnt types of quant compare are tough
                            to communicate to endusers, so they shouldn't
                            bother to toy around with them
                         */
                        {
                            int     x, y;
                            int     n = sscanf(arg, "%d,%d", &x, &y);
                            if (n == 1) {
                                y = x;
                            }
                            argUsed = 1;
                            if (INTERNAL_OPTS) {
                                lame_set_quant_comp(gfp, x);
                                lame_set_quant_comp_short(gfp, y);
                            }
                        }
                        break;
                    case 'Y':
                        lame_set_experimentalY(gfp, 1);
                        break;
                    case 'Z':
                        /*  experimental switch -Z:
                            this switch is obsolete
                         */
                        {
                            int     n = 1;
                            argUsed = sscanf(arg, "%d", &n);
                            if (INTERNAL_OPTS) {
                                lame_set_experimentalZ(gfp, n);
                            }
                        }
                        break;
                    case 'e':
                        argUsed = 1;

                        switch (*arg) {
                        case 'n':
                            lame_set_emphasis(gfp, 0);
                            break;
                        case '5':
                            lame_set_emphasis(gfp, 1);
                            break;
                        case 'c':
                            lame_set_emphasis(gfp, 3);
                            break;
                        default:
                            error_printf("%s: -e emp must be n/5/c not %s\n", ProgramName, arg);
                            return -1;
                        }
                        break;
                    case 'c':
                        lame_set_copyright(gfp, 1);
                        break;
                    case 'o':
                        lame_set_original(gfp, 0);
                        break;

                    case '?':
                        long_help(gfp, stdout, ProgramName, 0 /* LESSMODE=NO */ );
                        return -1;

                    default:
                        error_printf("%s: unrecognized option -%c\n", ProgramName, c);
                        return -1;
                    }
                    if (argUsed) {
                        if (arg == token)
                            token = ""; /* no more from token */
                        else
                            ++i; /* skip arg we used */
                        arg = "";
                        argUsed = 0;
                    }
                }
            }
        }
        else {
            if (nogap) {
                if ((num_nogap != NULL) && (count_nogap < *num_nogap)) {
                    strncpy(nogap_inPath[count_nogap++], argv[i], PATH_MAX + 1);
                    input_file = 1;
                }
                else {
                    /* sorry, calling program did not allocate enough space */
                    error_printf
                        ("Error: 'nogap option'.  Calling program does not allow nogap option, or\n"
                         "you have exceeded maximum number of input files for the nogap option\n");
                    *num_nogap = -1;
                    return -1;
                }
            }
            else {
                /* normal options:   inputfile  [outputfile], and
                   either one can be a '-' for stdin/stdout */
                if (inPath[0] == '\0') {
                    strncpy(inPath, argv[i], PATH_MAX + 1);
                    input_file = 1;
                }
                else {
                    if (outPath[0] == '\0')
                        strncpy(outPath, argv[i], PATH_MAX + 1);
                    else {
                        error_printf("%s: excess arg %s\n", ProgramName, argv[i]);
                        return -1;
                    }
                }
            }
        }
    }                   /* loop over args */

    if (!input_file) {
        usage(Console_IO.Console_fp, ProgramName);
        return -1;
    }

    if (inPath[0] == '-')
        silent = (silent <= 1 ? 1 : silent);
#ifdef WIN32
    else
        dosToLongFileName(inPath);
#endif

    if (outPath[0] == '\0' && count_nogap == 0) {
        if (inPath[0] == '-') {
            /* if input is stdin, default output is stdout */
            strcpy(outPath, "-");
        }
        else {
            strncpy(outPath, inPath, PATH_MAX + 1 - 4);
            if (lame_get_decode_only(gfp)) {
                strncat(outPath, ".wav", 4);
            }
            else {
                strncat(outPath, ".mp3", 4);
            }
        }
    }

    /* RG is enabled by default */
    if (!noreplaygain)
        lame_set_findReplayGain(gfp, 1);

    /* disable VBR tags with nogap unless the VBR tags are forced */
    if (nogap && lame_get_bWriteVbrTag(gfp) && nogap_tags == 0) {
        console_printf("Note: Disabling VBR Xing/Info tag since it interferes with --nogap\n");
        lame_set_bWriteVbrTag(gfp, 0);
    }

    /* some file options not allowed with stdout */
    if (outPath[0] == '-') {
        (void) lame_set_bWriteVbrTag(gfp, 0); /* turn off VBR tag */
    }

    /* if user did not explicitly specify input is mp3, check file name */
    if (input_format == sf_unknown)
        input_format = filename_to_type(inPath);

#if !(defined HAVE_MPGLIB || defined AMIGA_MPEGA)
    if (is_mpeg_file_format(input_format)) {
        error_printf("Error: libmp3lame not compiled with mpg123 *decoding* support \n");
        return -1;
    }
#endif


    if (input_format == sf_ogg) {
        error_printf("sorry, vorbis support in LAME is deprecated.\n");
        return -1;
    }
    /* default guess for number of channels */
    if (autoconvert)
        (void) lame_set_num_channels(gfp, 2);
    else if (MONO == lame_get_mode(gfp))
        (void) lame_set_num_channels(gfp, 1);
    else
        (void) lame_set_num_channels(gfp, 2);

    if (lame_get_free_format(gfp)) {
        if (lame_get_brate(gfp) < 8 || lame_get_brate(gfp) > 640) {
            error_printf("For free format, specify a bitrate between 8 and 640 kbps\n");
            error_printf("with the -b <bitrate> option\n");
            return -1;
        }
    }
    if (num_nogap != NULL)
        *num_nogap = count_nogap;
    return 0;
}


/* end of parse.c */

/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#if !defined (TEST_DLL_INTERFACE)
	#define DEFAULT_PATH	"special://xbmc/system/players/paplayer/timidity/"
	#define CONFIG_FILE		"special://xbmc/system/players/paplayer/timidity/timidity.cfg"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef __W32__
#include <windows.h>
#include <io.h>
#include <shlobj.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* NAVE_SYS_STAT_H */
#include <fcntl.h> /* for open */

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif

#ifndef __bool_true_false_are_defined
# ifdef bool
#  undef bool
# endif
# ifdef ture
#  undef ture
# endif
# ifdef false
#  undef false
# endif
# define bool int
# define false ((bool)0)
# define true (!false)
# define __bool_true_false_are_defined true
#endif /* C99 _Bool hack */

#ifdef BORLANDC_EXCEPTION
#include <excpt.h>
#endif /* BORLANDC_EXCEPTION */
#include <signal.h>

#if defined(__FreeBSD__) && !defined(__alpha__)
#include <floatingpoint.h> /* For FP exceptions */
#endif
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <ieeefp.h> /* For FP exceptions */
#endif

#include "timidity.h"
#include "tmdy_getopt.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "miditrace.h"
#include "reverb.h"
#ifdef SUPPORT_SOUNDSPEC
#include "soundspec.h"
#endif /* SUPPORT_SOUNDSPEC */
#include "resample.h"
#include "recache.h"
#include "strtab.h"
#include "wrd.h"
#define DEFINE_GLOBALS
#include "mid.defs"
#include "aq.h"
#include "mix.h"
//#include "unimod.h"
#include "quantity.h"


#ifndef __GNUC__
#define __attribute__(x) /* ignore */
#endif

#define PKGDATADIR DEFAULT_PATH

//#define TEST_DLL_INTERFACE
//#define ENABLE_MAIN

/* option enums */
enum {
	TIM_OPT_FIRST = 256,
	/* first entry */
	TIM_OPT_VOLUME = TIM_OPT_FIRST,
	TIM_OPT_DRUM_POWER,
	TIM_OPT_VOLUME_COMP,
	TIM_OPT_ANTI_ALIAS,
	TIM_OPT_BUFFER_FRAGS,
	TIM_OPT_CONTROL_RATIO,
	TIM_OPT_CONFIG_FILE,
	TIM_OPT_DRUM_CHANNEL,
	TIM_OPT_IFACE_PATH,
	TIM_OPT_EXT,
	TIM_OPT_MOD_WHEEL,
	TIM_OPT_PORTAMENTO,
	TIM_OPT_VIBRATO,
	TIM_OPT_CH_PRESS,
	TIM_OPT_MOD_ENV,
	TIM_OPT_TRACE_TEXT,
	TIM_OPT_OVERLAP,
	TIM_OPT_TEMPER_CTRL,
	TIM_OPT_DEFAULT_MID,
	TIM_OPT_SYSTEM_MID,
	TIM_OPT_DEFAULT_BANK,
	TIM_OPT_FORCE_BANK,
	TIM_OPT_DEFAULT_PGM,
	TIM_OPT_FORCE_PGM,
	TIM_OPT_DELAY,
	TIM_OPT_CHORUS,
	TIM_OPT_REVERB,
	TIM_OPT_VOICE_LPF,
	TIM_OPT_NS,
	TIM_OPT_RESAMPLE,
	TIM_OPT_EVIL,
	TIM_OPT_FAST_PAN,
	TIM_OPT_FAST_DECAY,
	TIM_OPT_SPECTROGRAM,
	TIM_OPT_KEYSIG,
	TIM_OPT_HELP,
	TIM_OPT_INTERFACE,
	TIM_OPT_VERBOSE,
	TIM_OPT_QUIET,
	TIM_OPT_TRACE,
	TIM_OPT_LOOP,
	TIM_OPT_RANDOM,
	TIM_OPT_SORT,
	TIM_OPT_BACKGROUND,
	TIM_OPT_RT_PRIO,
	TIM_OPT_SEQ_PORTS,
	TIM_OPT_REALTIME_LOAD,
	TIM_OPT_ADJUST_KEY,
	TIM_OPT_VOICE_QUEUE,
	TIM_OPT_PATCH_PATH,
	TIM_OPT_PCM_FILE,
	TIM_OPT_DECAY_TIME,
	TIM_OPT_INTERPOLATION,
	TIM_OPT_OUTPUT_MODE,
	TIM_OPT_OUTPUT_STEREO,
	TIM_OPT_OUTPUT_SIGNED,
	TIM_OPT_OUTPUT_BITWIDTH,
	TIM_OPT_OUTPUT_FORMAT,
	TIM_OPT_OUTPUT_SWAB,
	TIM_OPT_FLAC_VERIFY,
	TIM_OPT_FLAC_PADDING,
	TIM_OPT_FLAC_COMPLEVEL,
	TIM_OPT_FLAC_OGGFLAC,
	TIM_OPT_SPEEX_QUALITY,
	TIM_OPT_SPEEX_VBR,
	TIM_OPT_SPEEX_ABR,
	TIM_OPT_SPEEX_VAD,
	TIM_OPT_SPEEX_DTX,
	TIM_OPT_SPEEX_COMPLEXITY,
	TIM_OPT_SPEEX_NFRAMES,
	TIM_OPT_OUTPUT_FILE,
	TIM_OPT_PATCH_FILE,
	TIM_OPT_POLYPHONY,
	TIM_OPT_POLY_REDUCE,
	TIM_OPT_MUTE,
	TIM_OPT_TEMPER_MUTE,
	TIM_OPT_AUDIO_BUFFER,
	TIM_OPT_CACHE_SIZE,
	TIM_OPT_SAMPLE_FREQ,
	TIM_OPT_ADJUST_TEMPO,
	TIM_OPT_CHARSET,
	TIM_OPT_UNLOAD_INST,
	TIM_OPT_VOLUME_CURVE,
	TIM_OPT_VERSION,
	TIM_OPT_WRD,
	TIM_OPT_RCPCV_DLL,
	TIM_OPT_CONFIG_STR,
	TIM_OPT_FREQ_TABLE,
	TIM_OPT_PURE_INT,
	TIM_OPT_MODULE,
	/* last entry */
	TIM_OPT_LAST = TIM_OPT_PURE_INT
};

static const char *optcommands =
		"4A:aB:b:C:c:D:d:E:eFfg:H:hI:i:jK:k:L:M:m:N:"
		"O:o:P:p:Q:q:R:S:s:T:t:UV:vW:"
#ifdef __W32__
		"w:"
#endif
		"x:Z:";		/* Only GJlnruXYyz are remain... */
static const struct option longopts[] = {
	{ "volume",                 required_argument, NULL, TIM_OPT_VOLUME },
	{ "drum-power",             required_argument, NULL, TIM_OPT_DRUM_POWER },
	{ "no-volume-compensation", no_argument,       NULL, TIM_OPT_DRUM_POWER },
	{ "volume-compensation",    optional_argument, NULL, TIM_OPT_VOLUME_COMP },
	{ "no-anti-alias",          no_argument,       NULL, TIM_OPT_ANTI_ALIAS },
	{ "anti-alias",             optional_argument, NULL, TIM_OPT_ANTI_ALIAS },
	{ "buffer-fragments",       required_argument, NULL, TIM_OPT_BUFFER_FRAGS },
	{ "control-ratio",          required_argument, NULL, TIM_OPT_CONTROL_RATIO },
	{ "config-file",            required_argument, NULL, TIM_OPT_CONFIG_FILE },
	{ "drum-channel",           required_argument, NULL, TIM_OPT_DRUM_CHANNEL },
	{ "interface-path",         required_argument, NULL, TIM_OPT_IFACE_PATH },
	{ "ext",                    required_argument, NULL, TIM_OPT_EXT },
	{ "no-mod-wheel",           no_argument,       NULL, TIM_OPT_MOD_WHEEL },
	{ "mod-wheel",              optional_argument, NULL, TIM_OPT_MOD_WHEEL },
	{ "no-portamento",          no_argument,       NULL, TIM_OPT_PORTAMENTO },
	{ "portamento",             optional_argument, NULL, TIM_OPT_PORTAMENTO },
	{ "no-vibrato",             no_argument,       NULL, TIM_OPT_VIBRATO },
	{ "vibrato",                optional_argument, NULL, TIM_OPT_VIBRATO },
	{ "no-ch-pressure",         no_argument,       NULL, TIM_OPT_CH_PRESS },
	{ "ch-pressure",            optional_argument, NULL, TIM_OPT_CH_PRESS },
	{ "no-mod-envelope",        no_argument,       NULL, TIM_OPT_MOD_ENV },
	{ "mod-envelope",           optional_argument, NULL, TIM_OPT_MOD_ENV },
	{ "no-trace-text-meta",     no_argument,       NULL, TIM_OPT_TRACE_TEXT },
	{ "trace-text-meta",        optional_argument, NULL, TIM_OPT_TRACE_TEXT },
	{ "no-overlap-voice",       no_argument,       NULL, TIM_OPT_OVERLAP },
	{ "overlap-voice",          optional_argument, NULL, TIM_OPT_OVERLAP },
	{ "no-temper-control",      no_argument,       NULL, TIM_OPT_TEMPER_CTRL },
	{ "temper-control",         optional_argument, NULL, TIM_OPT_TEMPER_CTRL },
	{ "default-mid",            required_argument, NULL, TIM_OPT_DEFAULT_MID },
	{ "system-mid",             required_argument, NULL, TIM_OPT_SYSTEM_MID },
	{ "default-bank",           required_argument, NULL, TIM_OPT_DEFAULT_BANK },
	{ "force-bank",             required_argument, NULL, TIM_OPT_FORCE_BANK },
	{ "default-program",        required_argument, NULL, TIM_OPT_DEFAULT_PGM },
	{ "force-program",          required_argument, NULL, TIM_OPT_FORCE_PGM },
	{ "delay",                  required_argument, NULL, TIM_OPT_DELAY },
	{ "chorus",                 required_argument, NULL, TIM_OPT_CHORUS },
	{ "reverb",                 required_argument, NULL, TIM_OPT_REVERB },
	{ "voice-lpf",              required_argument, NULL, TIM_OPT_VOICE_LPF },
	{ "noise-shaping",          required_argument, NULL, TIM_OPT_NS },
#ifndef FIXED_RESAMPLATION
	{ "resample",               required_argument, NULL, TIM_OPT_RESAMPLE },
#endif
	{ "evil",                   required_argument, NULL, TIM_OPT_EVIL },
	{ "no-fast-panning",        no_argument,       NULL, TIM_OPT_FAST_PAN },
	{ "fast-panning",           optional_argument, NULL, TIM_OPT_FAST_PAN },
	{ "no-fast-decay",          no_argument,       NULL, TIM_OPT_FAST_DECAY },
	{ "fast-decay",             optional_argument, NULL, TIM_OPT_FAST_DECAY },
	{ "spectrogram",            required_argument, NULL, TIM_OPT_SPECTROGRAM },
	{ "force-keysig",           required_argument, NULL, TIM_OPT_KEYSIG },
	{ "help",                   optional_argument, NULL, TIM_OPT_HELP },
	{ "interface",              required_argument, NULL, TIM_OPT_INTERFACE },
	{ "verbose",                optional_argument, NULL, TIM_OPT_VERBOSE },
	{ "quiet",                  optional_argument, NULL, TIM_OPT_QUIET },
	{ "no-trace",               no_argument,       NULL, TIM_OPT_TRACE },
	{ "trace",                  optional_argument, NULL, TIM_OPT_TRACE },
	{ "no-loop",                no_argument,       NULL, TIM_OPT_LOOP },
	{ "loop",                   optional_argument, NULL, TIM_OPT_LOOP },
	{ "no-random",              no_argument,       NULL, TIM_OPT_RANDOM },
	{ "random",                 optional_argument, NULL, TIM_OPT_RANDOM },
	{ "no-sort",                no_argument,       NULL, TIM_OPT_SORT },
	{ "sort",                   optional_argument, NULL, TIM_OPT_SORT },
#ifdef IA_ALSASEQ
	{ "no-background",          no_argument,       NULL, TIM_OPT_BACKGROUND },
	{ "background",             optional_argument, NULL, TIM_OPT_BACKGROUND },
	{ "realtime-priority",      required_argument, NULL, TIM_OPT_RT_PRIO },
	{ "sequencer-ports",        required_argument, NULL, TIM_OPT_SEQ_PORTS },
#endif
	{ "no-realtime-load",       no_argument,       NULL, TIM_OPT_REALTIME_LOAD },
	{ "realtime-load",          optional_argument, NULL, TIM_OPT_REALTIME_LOAD },
	{ "adjust-key",             required_argument, NULL, TIM_OPT_ADJUST_KEY },
	{ "voice-queue",            required_argument, NULL, TIM_OPT_VOICE_QUEUE },
	{ "patch-path",             required_argument, NULL, TIM_OPT_PATCH_PATH },
	{ "pcm-file",               required_argument, NULL, TIM_OPT_PCM_FILE },
	{ "decay-time",             required_argument, NULL, TIM_OPT_DECAY_TIME },
	{ "interpolation",          required_argument, NULL, TIM_OPT_INTERPOLATION },
	{ "output-mode",            required_argument, NULL, TIM_OPT_OUTPUT_MODE },
	{ "output-stereo",          no_argument,       NULL, TIM_OPT_OUTPUT_STEREO },
	{ "output-mono",            no_argument,       NULL, TIM_OPT_OUTPUT_STEREO },
	{ "output-signed",          no_argument,       NULL, TIM_OPT_OUTPUT_SIGNED },
	{ "output-unsigned",        no_argument,       NULL, TIM_OPT_OUTPUT_SIGNED },
	{ "output-16bit",           no_argument,       NULL, TIM_OPT_OUTPUT_BITWIDTH },
	{ "output-24bit",           no_argument,       NULL, TIM_OPT_OUTPUT_BITWIDTH },
	{ "output-8bit",            no_argument,       NULL, TIM_OPT_OUTPUT_BITWIDTH },
	{ "output-linear",          no_argument,       NULL, TIM_OPT_OUTPUT_FORMAT },
	{ "output-ulaw",            no_argument,       NULL, TIM_OPT_OUTPUT_FORMAT },
	{ "output-alaw",            no_argument,       NULL, TIM_OPT_OUTPUT_FORMAT },
	{ "no-output-swab",         no_argument,       NULL, TIM_OPT_OUTPUT_SWAB },
	{ "output-swab",            optional_argument, NULL, TIM_OPT_OUTPUT_SWAB },
#ifdef AU_FLAC
	{ "flac-verify",            no_argument,       NULL, TIM_OPT_FLAC_VERIFY },
	{ "flac-padding",           required_argument, NULL, TIM_OPT_FLAC_PADDING },
	{ "flac-complevel",         required_argument, NULL, TIM_OPT_FLAC_COMPLEVEL },
#ifdef AU_OGGFLAC
	{ "oggflac",                no_argument,       NULL, TIM_OPT_FLAC_OGGFLAC },
#endif /* AU_OGGFLAC */
#endif /* AU_FLAC */
#ifdef AU_SPEEX
	{ "speex-quality",          required_argument, NULL, TIM_OPT_SPEEX_QUALITY },
	{ "speex-vbr",              no_argument,       NULL, TIM_OPT_SPEEX_VBR },
	{ "speex-abr",              required_argument, NULL, TIM_OPT_SPEEX_ABR },
	{ "speex-vad",              no_argument,       NULL, TIM_OPT_SPEEX_VAD },
	{ "speex-dtx",              no_argument,       NULL, TIM_OPT_SPEEX_DTX },
	{ "speex-complexity",       required_argument, NULL, TIM_OPT_SPEEX_COMPLEXITY },
	{ "speex-nframes",          required_argument, NULL, TIM_OPT_SPEEX_NFRAMES },
#endif /* AU_SPEEX */
	{ "output-file",            required_argument, NULL, TIM_OPT_OUTPUT_FILE },
	{ "patch-file",             required_argument, NULL, TIM_OPT_PATCH_FILE },
	{ "polyphony",              required_argument, NULL, TIM_OPT_POLYPHONY },
	{ "no-polyphony-reduction", no_argument,       NULL, TIM_OPT_POLY_REDUCE },
	{ "polyphony-reduction",    optional_argument, NULL, TIM_OPT_POLY_REDUCE },
	{ "mute",                   required_argument, NULL, TIM_OPT_MUTE },
	{ "temper-mute",            required_argument, NULL, TIM_OPT_TEMPER_MUTE },
	{ "audio-buffer",           required_argument, NULL, TIM_OPT_AUDIO_BUFFER },
	{ "cache-size",             required_argument, NULL, TIM_OPT_CACHE_SIZE },
	{ "sampling-freq",          required_argument, NULL, TIM_OPT_SAMPLE_FREQ },
	{ "adjust-tempo",           required_argument, NULL, TIM_OPT_ADJUST_TEMPO },
	{ "output-charset",         required_argument, NULL, TIM_OPT_CHARSET },
	{ "no-unload-instruments",  no_argument,       NULL, TIM_OPT_UNLOAD_INST },
	{ "unload-instruments",     optional_argument, NULL, TIM_OPT_UNLOAD_INST },
	{ "volume-curve",           required_argument, NULL, TIM_OPT_VOLUME_CURVE },
	{ "version",                no_argument,       NULL, TIM_OPT_VERSION },
	{ "wrd",                    required_argument, NULL, TIM_OPT_WRD },
#ifdef __W32__
	{ "rcpcv-dll",              required_argument, NULL, TIM_OPT_RCPCV_DLL },
#endif
	{ "config-string",          required_argument, NULL, TIM_OPT_CONFIG_STR },
	{ "freq-table",             required_argument, NULL, TIM_OPT_FREQ_TABLE },
	{ "pure-intonation",        optional_argument, NULL, TIM_OPT_PURE_INT },
	{ "module",                 required_argument, NULL, TIM_OPT_MODULE },
	{ NULL,                     no_argument,       NULL, '\0'     }
};
#define INTERACTIVE_INTERFACE_IDS "kmqagrwAWP"

/* main interfaces (To be used another main) */
#if defined(main) || defined(ANOTHER_MAIN) || defined ( IA_W32GUI ) || defined ( IA_W32G_SYN )
#define MAIN_INTERFACE
#else
#define MAIN_INTERFACE static
#endif /* main */

MAIN_INTERFACE void timidity_start_initialize(void);
MAIN_INTERFACE int timidity_pre_load_configuration(void);
MAIN_INTERFACE int timidity_post_load_configuration(void);
MAIN_INTERFACE void timidity_init_player(void);
MAIN_INTERFACE int timidity_play_main(int nfiles, char **files);
MAIN_INTERFACE int got_a_configuration;
char *wrdt_open_opts = NULL;
char *opt_aq_max_buff = NULL,
     *opt_aq_fill_buff = NULL;
void timidity_init_aq_buff(void);
int opt_control_ratio = 0; /* Save -C option */

int set_extension_modes(char *);
int set_ctl(char *);
int set_play_mode(char *);
int set_wrd(char *);
MAIN_INTERFACE int set_tim_opt_short(int, char *);
MAIN_INTERFACE int set_tim_opt_long(int, char *, int);
static inline int parse_opt_A(const char *);
static inline int parse_opt_drum_power(const char *);
static inline int parse_opt_volume_comp(const char *);
static inline int parse_opt_a(const char *);
static inline int parse_opt_B(const char *);
static inline int parse_opt_C(const char *);
static inline int parse_opt_c(char *);
static inline int parse_opt_D(const char *);
static inline int parse_opt_d(const char *);
static inline int parse_opt_E(char *);
static inline int parse_opt_mod_wheel(const char *);
static inline int parse_opt_portamento(const char *);
static inline int parse_opt_vibrato(const char *);
static inline int parse_opt_ch_pressure(const char *);
static inline int parse_opt_mod_env(const char *);
static inline int parse_opt_trace_text(const char *);
static inline int parse_opt_overlap_voice(const char *);
static inline int parse_opt_temper_control(const char *);
static inline int parse_opt_default_mid(char *);
static inline int parse_opt_system_mid(char *);
static inline int parse_opt_default_bank(const char *);
static inline int parse_opt_force_bank(const char *);
static inline int parse_opt_default_program(const char *);
static inline int parse_opt_force_program(const char *);
static inline int set_default_program(int);
static inline int parse_opt_delay(const char *);
static inline int parse_opt_chorus(const char *);
static inline int parse_opt_reverb(const char *);
static inline int parse_opt_voice_lpf(const char *);
static inline int parse_opt_noise_shaping(const char *);
static inline int parse_opt_resample(const char *);
static inline int parse_opt_e(const char *);
static inline int parse_opt_F(const char *);
static inline int parse_opt_f(const char *);
static inline int parse_opt_g(const char *);
static inline int parse_opt_H(const char *);
__attribute__((noreturn))
static inline int parse_opt_h(const char *);
#ifdef IA_DYNAMIC
static inline void list_dyna_interface(FILE *, char *, char *);
static inline char *dynamic_interface_info(int);
char *dynamic_interface_module(int);
#endif
static inline int parse_opt_i(const char *);
static inline int parse_opt_verbose(const char *);
static inline int parse_opt_quiet(const char *);
static inline int parse_opt_trace(const char *);
static inline int parse_opt_loop(const char *);
static inline int parse_opt_random(const char *);
static inline int parse_opt_sort(const char *);
#ifdef IA_ALSASEQ
static inline int parse_opt_background(const char *);
static inline int parse_opt_rt_prio(const char *);
static inline int parse_opt_seq_ports(const char *);
#endif
static inline int parse_opt_j(const char *);
static inline int parse_opt_K(const char *);
static inline int parse_opt_k(const char *);
static inline int parse_opt_L(char *);
static inline int parse_opt_M(const char *);
static inline int parse_opt_m(const char *);
static inline int parse_opt_N(const char *);
static inline int parse_opt_O(const char *);
static inline int parse_opt_output_stereo(const char *);
static inline int parse_opt_output_signed(const char *);
static inline int parse_opt_output_bitwidth(const char *);
static inline int parse_opt_output_format(const char *);
static inline int parse_opt_output_swab(const char *);
#ifdef AU_FLAC
static inline int parse_opt_flac_verify(const char *);
static inline int parse_opt_flac_padding(const char *);
static inline int parse_opt_flac_complevel(const char *);
#ifdef AU_OGGFLAC
static inline int parse_opt_flac_oggflac(const char *);
#endif /* AU_OGGFLAC */
#endif /* AU_FLAC */
#ifdef AU_SPEEX
static inline int parse_opt_speex_quality(const char *);
static inline int parse_opt_speex_vbr(const char *);
static inline int parse_opt_speex_abr(const char *);
static inline int parse_opt_speex_vad(const char *);
static inline int parse_opt_speex_dtx(const char *);
static inline int parse_opt_speex_complexity(const char *);
static inline int parse_opt_speex_nframes(const char *);
#endif /* AU_SPEEX */
static inline int parse_opt_o(char *);
static inline int parse_opt_P(const char *);
static inline int parse_opt_p(const char *);
static inline int parse_opt_p1(const char *);
static inline int parse_opt_Q(const char *);
static inline int parse_opt_Q1(const char *);
static inline int parse_opt_q(const char *);
static inline int parse_opt_R(const char *);
static inline int parse_opt_S(const char *);
static inline int parse_opt_s(const char *);
static inline int parse_opt_T(const char *);
static inline int parse_opt_t(const char *);
static inline int parse_opt_U(const char *);
static inline int parse_opt_volume_curve(char *);
__attribute__((noreturn))
static inline int parse_opt_v(const char *);
#ifdef __W32__
static inline int parse_opt_w(const char *);
#endif
static inline int parse_opt_x(char *);
static inline void expand_escape_string(char *);
static inline int parse_opt_Z(const char *);
static inline int parse_opt_Z1(const char *);
static inline int parse_opt_default_module(const char *);
__attribute__((noreturn))
static inline int parse_opt_fail(const char *);
static inline int set_value(int *, int, int, int, char *);
static inline int set_val_i32(int32 *, int32, int32, int32, char *);
static inline int set_channel_flag(ChannelBitMask *, int32, char *);
static inline int y_or_n_p(const char *);
static inline int set_flag(int32 *, int32, const char *);
static inline FILE *open_pager(void);
static inline void close_pager(FILE *);
static void interesting_message(void);

// sndfont.c
void free_soundfonts();

#ifdef IA_DYNAMIC
MAIN_INTERFACE char dynamic_interface_id;
#endif /* IA_DYNAMIC */

extern int SecondMode;

extern struct URL_module URL_module_file;

MAIN_INTERFACE struct URL_module *url_module_list[] =
{
    &URL_module_file,
    NULL
};

#ifdef IA_DYNAMIC
#include "dlutils.h"
#ifndef SHARED_LIB_PATH
#define SHARED_LIB_PATH PKGLIBDIR
#endif /* SHARED_LIB_PATH */
static char *dynamic_lib_root = SHARED_LIB_PATH;
#endif /* IA_DYNAMIC */

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif /* MAXPATHLEN */

int free_instruments_afterwards=0;
int def_prog = -1;
char def_instr_name[256]="";
VOLATILE int intr = 0;

#ifdef __W32__
CRITICAL_SECTION critSect;

#pragma argsused
static BOOL WINAPI handler(DWORD dw)
{
#if defined(IA_WINSYN) || defined(IA_PORTMIDISYN)
	if( ctl->id_character == 'W' 
		|| ctl->id_character == 'P' )
	{
    	rtsyn_midiports_close();
	}
#endif	
    printf ("***BREAK" NLS); fflush(stdout);
    intr++;
    return TRUE;
}
#endif


int effect_lr_mode = -1;
/* 0: left delay
 * 1: right delay
 * 2: rotate
 * -1: not use
 */
int effect_lr_delay_msec = 25;

extern char* pcm_alternate_file;
/* NULL, "none": disabled (default)
 * "auto":       automatically selected
 * filename:     use the one.
 */

#ifndef atof
extern double atof(const char *);
#endif

/*! copy bank and, if necessary, map appropriately */
static void copybank(ToneBank *to, ToneBank *from, int mapid, int bankmapfrom, int bankno)
{
	ToneBankElement *toelm, *fromelm;
	int i;

	if (from == NULL)
		return;
	for(i = 0; i < 128; i++)
	{
		toelm = &to->tone[i];
		fromelm = &from->tone[i];
		if (fromelm->name == NULL)
		    continue;
		copy_tone_bank_element(toelm, fromelm);
		toelm->instrument = NULL;
		if (mapid != INST_NO_MAP)
		    set_instrument_map(mapid, bankmapfrom, i, bankno, i);
	}
}

/*! copy the whole mapped bank. returns 0 if no error. */
static int copymap(int mapto, int mapfrom, int isdrum)
{
	ToneBank **tb = isdrum ? drumset : tonebank;
	int i, bankfrom, bankto;
	
	for(i = 0; i < 128; i++)
	{
		bankfrom = find_instrument_map_bank(isdrum, mapfrom, i);
		if (bankfrom <= 0) /* not mapped */
			continue;
		bankto = alloc_instrument_map_bank(isdrum, mapto, i);
		if (bankto == -1) /* failed */
			return 1;
		copybank(tb[bankto], tb[bankfrom], mapto, i, bankto);
	}
	return 0;
}

static float *config_parse_tune(const char *cp, int *num)
{
	const char *p;
	float *tune_list;
	int i;
	
	/* count num */
	*num = 1, p = cp;
	while ((p = strchr(p, ',')) != NULL)
		(*num)++, p++;
	/* alloc */
	tune_list = (float *) safe_malloc((*num) * sizeof(float));
	/* regist */
	for (i = 0, p = cp; i < *num; i++, p++) {
		tune_list[i] = atof(p);
		if (! (p = strchr(p, ',')))
			break;
	}
	return tune_list;
}

static int16 *config_parse_int16(const char *cp, int *num)
{
	const char *p;
	int16 *list;
	int i;
	
	/* count num */
	*num = 1, p = cp;
	while ((p = strchr(p, ',')) != NULL)
		(*num)++, p++;
	/* alloc */
	list = (int16 *) safe_malloc((*num) * sizeof(int16));
	/* regist */
	for (i = 0, p = cp; i < *num; i++, p++) {
		list[i] = atoi(p);
		if (! (p = strchr(p, ',')))
			break;
	}
	return list;
}

static int **config_parse_envelope(const char *cp, int *num)
{
	const char *p, *px;
	int **env_list;
	int i, j;
	
	/* count num */
	*num = 1, p = cp;
	while ((p = strchr(p, ',')) != NULL)
		(*num)++, p++;
	/* alloc */
	env_list = (int **) safe_malloc((*num) * sizeof(int *));
	for (i = 0; i < *num; i++)
		env_list[i] = (int *) safe_malloc(6 * sizeof(int));
	/* init */
	for (i = 0; i < *num; i++)
		for (j = 0; j < 6; j++)
			env_list[i][j] = -1;
	/* regist */
	for (i = 0, p = cp; i < *num; i++, p++) {
		px = strchr(p, ',');
		for (j = 0; j < 6; j++, p++) {
			if (*p == ':')
				continue;
			env_list[i][j] = atoi(p);
			if (! (p = strchr(p, ':')))
				break;
			if (px && p > px)
				break;
		}
		if (! (p = px))
			break;
	}
	return env_list;
}

static Quantity **config_parse_modulation(const char *name, int line, const char *cp, int *num, int mod_type)
{
	const char *p, *px, *err;
	char buf[128], *delim;
	Quantity **mod_list;
	int i, j;
	static const char * qtypestr[] = {"tremolo", "vibrato"};
	static const uint16 qtypes[] = {
		QUANTITY_UNIT_TYPE(TREMOLO_SWEEP), QUANTITY_UNIT_TYPE(TREMOLO_RATE), QUANTITY_UNIT_TYPE(DIRECT_INT),
		QUANTITY_UNIT_TYPE(VIBRATO_SWEEP), QUANTITY_UNIT_TYPE(VIBRATO_RATE), QUANTITY_UNIT_TYPE(DIRECT_INT)
	};
	
	/* count num */
	*num = 1, p = cp;
	while ((p = strchr(p, ',')) != NULL)
		(*num)++, p++;
	/* alloc */
	mod_list = (Quantity **) safe_malloc((*num) * sizeof(Quantity *));
	for (i = 0; i < *num; i++)
		mod_list[i] = (Quantity *) safe_malloc(3 * sizeof(Quantity));
	/* init */
	for (i = 0; i < *num; i++)
		for (j = 0; j < 3; j++)
			INIT_QUANTITY(mod_list[i][j]);
	buf[sizeof buf - 1] = '\0';
	/* regist */
	for (i = 0, p = cp; i < *num; i++, p++) {
		px = strchr(p, ',');
		for (j = 0; j < 3; j++, p++) {
			if (*p == ':')
				continue;
			if ((delim = strpbrk(strncpy(buf, p, sizeof buf - 1), ":,")) != NULL)
				*delim = '\0';
			if (*buf != '\0' && (err = string_to_quantity(buf, &mod_list[i][j], qtypes[mod_type * 3 + j])) != NULL) {
				ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: %s: parameter %d of item %d: %s (%s)",
						name, line, qtypestr[mod_type], j+1, i+1, err, buf);
				free_ptr_list(mod_list, *num);
				mod_list = NULL;
				*num = 0;
				return NULL;
			}
			if (! (p = strchr(p, ':')))
				break;
			if (px && p > px)
				break;
		}
		if (! (p = px))
			break;
	}
	return mod_list;
}

static int set_gus_patchconf_opts(char *name,
		int line, char *opts, ToneBankElement *tone)
{
	char *cp;
	int k;
	
	if (! (cp = strchr(opts, '='))) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"%s: line %d: bad patch option %s", name, line, opts);
		return 1;
	}
	*cp++ = 0;
	if (! strcmp(opts, "amp")) {
		k = atoi(cp);
		if ((k < 0 || k > MAX_AMPLIFICATION) || (*cp < '0' || *cp > '9')) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"%s: line %d: amplification must be between 0 and %d",
					name, line, MAX_AMPLIFICATION);
			return 1;
		}
		tone->amp = k;
	} else if (! strcmp(opts, "note")) {
		k = atoi(cp);
		if ((k < 0 || k > 127) || (*cp < '0' || *cp > '9')) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"%s: line %d: note must be between 0 and 127",
					name, line);
			return 1;
		}
		tone->note = k;
		tone->scltune = config_parse_int16("100", &tone->scltunenum);
	} else if (! strcmp(opts, "pan")) {
		if (! strcmp(cp, "center"))
			k = 64;
		else if (! strcmp(cp, "left"))
			k = 0;
		else if (! strcmp(cp, "right"))
			k = 127;
		else {
			k = ((atoi(cp) + 100) * 100) / 157;
			if ((k < 0 || k > 127)
					|| (k == 0 && *cp != '-' && (*cp < '0' || *cp > '9'))) {
				ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
						"%s: line %d: panning must be left, right, "
						"center, or between -100 and 100",
						name, line);
				return 1;
			}
		}
		tone->pan = k;
	} else if (! strcmp(opts, "tune"))
		tone->tune = config_parse_tune(cp, &tone->tunenum);
	else if (! strcmp(opts, "rate"))
		tone->envrate = config_parse_envelope(cp, &tone->envratenum);
	else if (! strcmp(opts, "offset"))
		tone->envofs = config_parse_envelope(cp, &tone->envofsnum);
	else if (! strcmp(opts, "keep")) {
		if (! strcmp(cp, "env"))
			tone->strip_envelope = 0;
		else if (! strcmp(cp, "loop"))
			tone->strip_loop = 0;
		else {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"%s: line %d: keep must be env or loop", name, line);
			return 1;
		}
	} else if (! strcmp(opts, "strip")) {
		if (! strcmp(cp, "env"))
			tone->strip_envelope = 1;
		else if (! strcmp(cp, "loop"))
			tone->strip_loop = 1;
		else if (! strcmp(cp, "tail"))
			tone->strip_tail = 1;
		else {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"%s: line %d: strip must be env, loop, or tail",
					name, line);
			return 1;
		}
	} else if (! strcmp(opts, "tremolo")) {
		if ((tone->trem = config_parse_modulation(name,
				line, cp, &tone->tremnum, 0)) == NULL)
			return 1;
	} else if (! strcmp(opts, "vibrato")) {
		if ((tone->vib = config_parse_modulation(name,
				line, cp, &tone->vibnum, 1)) == NULL)
			return 1;
	} else if (! strcmp(opts, "sclnote"))
		tone->sclnote = config_parse_int16(cp, &tone->sclnotenum);
	else if (! strcmp(opts, "scltune"))
		tone->scltune = config_parse_int16(cp, &tone->scltunenum);
	else if (! strcmp(opts, "comm")) {
		char *p;
		
		if (tone->comment)
			free(tone->comment);
		p = tone->comment = safe_strdup(cp);
		while (*p) {
			if (*p == ',')
				*p = ' ';
			p++;
		}
	} else if (! strcmp(opts, "modrate"))
		tone->modenvrate = config_parse_envelope(cp, &tone->modenvratenum);
	else if (! strcmp(opts, "modoffset"))
		tone->modenvofs = config_parse_envelope(cp, &tone->modenvofsnum);
	else if (! strcmp(opts, "envkeyf"))
		tone->envkeyf = config_parse_envelope(cp, &tone->envkeyfnum);
	else if (! strcmp(opts, "envvelf"))
		tone->envvelf = config_parse_envelope(cp, &tone->envvelfnum);
	else if (! strcmp(opts, "modkeyf"))
		tone->modenvkeyf = config_parse_envelope(cp, &tone->modenvkeyfnum);
	else if (! strcmp(opts, "modvelf"))
		tone->modenvvelf = config_parse_envelope(cp, &tone->modenvvelfnum);
	else if (! strcmp(opts, "trempitch"))
		tone->trempitch = config_parse_int16(cp, &tone->trempitchnum);
	else if (! strcmp(opts, "tremfc"))
		tone->tremfc = config_parse_int16(cp, &tone->tremfcnum);
	else if (! strcmp(opts, "modpitch"))
		tone->modpitch = config_parse_int16(cp, &tone->modpitchnum);
	else if (! strcmp(opts, "modfc"))
		tone->modfc = config_parse_int16(cp, &tone->modfcnum);
	else if (! strcmp(opts, "fc"))
		tone->fc = config_parse_int16(cp, &tone->fcnum);
	else if (! strcmp(opts, "q"))
		tone->reso = config_parse_int16(cp, &tone->resonum);
	else if (! strcmp(opts, "fckeyf"))		/* filter key-follow */
		tone->key_to_fc = atoi(cp);
	else if (! strcmp(opts, "fcvelf"))		/* filter velocity-follow */
		tone->vel_to_fc = atoi(cp);
	else if (! strcmp(opts, "qvelf"))		/* resonance velocity-follow */
		tone->vel_to_resonance = atoi(cp);
	else {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"%s: line %d: bad patch option %s",
				name, line, opts);
		return 1;
	}
	return 0;
}

static void reinit_tone_bank_element(ToneBankElement *tone)
{
	free_tone_bank_element(tone);
	tone->note = tone->pan = -1;
	tone->strip_loop = tone->strip_envelope = tone->strip_tail = -1;
	tone->amp = -1;
	tone->rnddelay = 0;
	tone->loop_timeout = 0;
	tone->legato = tone->damper_mode = tone->key_to_fc = tone->vel_to_fc = 0;
	tone->reverb_send = tone->chorus_send = tone->delay_send = -1;
	tone->tva_level = -1;
	tone->play_note = -1;
}

#define SET_GUS_PATCHCONF_COMMENT
static int set_gus_patchconf(char *name, int line,
			     ToneBankElement *tone, char *pat, char **opts)
{
    int j;
#ifdef SET_GUS_PATCHCONF_COMMENT
		char *old_name = NULL;

		if(tone != NULL && tone->name != NULL)
			old_name = safe_strdup(tone->name);
#endif
    reinit_tone_bank_element(tone);

    if(strcmp(pat, "%font") == 0) /* Font extention */
    {
	/* %font filename bank prog [note-to-use]
	 * %font filename 128 bank key
	 */

	if(opts[0] == NULL || opts[1] == NULL || opts[2] == NULL ||
	   (atoi(opts[1]) == 128 && opts[3] == NULL))
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: Syntax error", name, line);
	    return 1;
	}
	tone->name = safe_strdup(opts[0]);
	tone->instype = 1;
	if(atoi(opts[1]) == 128) /* drum */
	{
	    tone->font_bank = 128;
	    tone->font_preset = atoi(opts[2]);
	    tone->font_keynote = atoi(opts[3]);
	    opts += 4;
	}
	else
	{
	    tone->font_bank = atoi(opts[1]);
	    tone->font_preset = atoi(opts[2]);

	    if(opts[3] && isdigit(opts[3][0]))
	    {
		tone->font_keynote = atoi(opts[3]);
		opts += 4;
	    }
	    else
	    {
		tone->font_keynote = -1;
		opts += 3;
	    }
	}
    }
    else if(strcmp(pat, "%sample") == 0) /* Sample extention */
    {
	/* %sample filename */

	if(opts[0] == NULL)
	{
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: Syntax error", name, line);
	    return 1;
	}
	tone->name = safe_strdup(opts[0]);
	tone->instype = 2;
	opts++;
    }
    else
    {
	tone->instype = 0;
	tone->name = safe_strdup(pat);
    }

    for(j = 0; opts[j] != NULL; j++)
    {
	int err;
	if((err = set_gus_patchconf_opts(name, line, opts[j], tone)) != 0)
	    return err;
    }
#ifdef SET_GUS_PATCHCONF_COMMENT
		if(tone->comment == NULL ||
			(old_name != NULL && strcmp(old_name,tone->comment) == 0))
		{
			if(tone->comment != NULL )
				free(tone->comment);
			tone->comment = safe_strdup(tone->name);
		}
		if(old_name != NULL)
			free(old_name);
#else
    if(tone->comment == NULL)
	tone->comment = safe_strdup(tone->name);
#endif
    return 0;
}

static int set_patchconf(char *name, int line, ToneBank *bank, char *w[], int dr, int mapid, int bankmapfrom, int bankno)
{
    int i;
    
    i = atoi(w[0]);
    if(!dr)
	i -= progbase;
    if(i < 0 || i > 127)
    {
	if(dr)
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: Drum number must be between "
		      "0 and 127",
		      name, line);
	else
	    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		      "%s: line %d: Program must be between "
		      "%d and %d",
		      name, line, progbase, 127 + progbase);
	return 1;
    }
    if(!bank)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "%s: line %d: Must specify tone bank or drum set "
		  "before assignment", name, line);
	return 1;
    }

    if(set_gus_patchconf(name, line, &bank->tone[i], w[1], w + 2))
	return 1;
    if (mapid != INST_NO_MAP)
	set_instrument_map(mapid, bankmapfrom, i, bankno, i);
    return 0;
}

typedef struct {
	const char *name;
	int mapid, isdrum;
} MapNameEntry;

static int mapnamecompare(const void *name, const void *entry)
{
	return strcmp((const char *)name, ((const MapNameEntry *)entry)->name);
}

static int mapname2id(char *name, int *isdrum)
{
	static const MapNameEntry data[] = {
		/* sorted in alphabetical order */
		{"gm2",         GM2_TONE_MAP, 0},
		{"gm2drum",     GM2_DRUM_MAP, 1},
		{"sc55",        SC_55_TONE_MAP, 0},
		{"sc55drum",    SC_55_DRUM_MAP, 1},
		{"sc88",        SC_88_TONE_MAP, 0},
		{"sc8850",      SC_8850_TONE_MAP, 0},
		{"sc8850drum",  SC_8850_DRUM_MAP, 1},
		{"sc88drum",    SC_88_DRUM_MAP, 1},
		{"sc88pro",     SC_88PRO_TONE_MAP, 0},
		{"sc88prodrum", SC_88PRO_DRUM_MAP, 1},
		{"xg",          XG_NORMAL_MAP, 0},
		{"xgdrum",      XG_DRUM_MAP, 1},
		{"xgsfx126",    XG_SFX126_MAP, 1},
		{"xgsfx64",     XG_SFX64_MAP, 0}
	};
	const MapNameEntry *found;
	
	found = (MapNameEntry *)bsearch(name, data, sizeof data / sizeof data[0], sizeof data[0], mapnamecompare);
	if (found != NULL)
	{
		*isdrum = found->isdrum;
		return found->mapid;
	}
	return -1;
}

/* string[0] should not be '#' */
static int strip_trailing_comment(char *string, int next_token_index)
{
    if (string[next_token_index - 1] == '#'	/* strip \1 in /^\S+(#*[ \t].*)/ */
	&& (string[next_token_index] == ' ' || string[next_token_index] == '\t'))
    {
	string[next_token_index] = '\0';	/* new c-string terminator */
	while(string[--next_token_index - 1] == '#')
	    ;
    }
    return next_token_index;
}

static char *expand_variables(char *string, MBlockList *varbuf, const char *basedir)
{
	char *p, *expstr;
	const char *copystr;
	int limlen, copylen, explen, varlen, braced;
	
	if ((p = strchr(string, '$')) == NULL)
		return string;
	varlen = strlen(basedir);
	explen = limlen = 0;
	expstr = NULL;
	copystr = string;
	copylen = p - string;
	string = p;
	for(;;)
	{
		if (explen + copylen + 1 > limlen)
		{
			limlen += copylen + 128;
			expstr = memcpy(new_segment(varbuf, limlen), expstr, explen);
		}
		memcpy(&expstr[explen], copystr, copylen);
		explen += copylen;
		if (*string == '\0')
			break;
		else if (*string == '$')
		{
			braced = *++string == '{';
			if (braced)
			{
				if ((p = strchr(string + 1, '}')) == NULL)
					p = string;	/* no closing brace */
				else
					string++;
			}
			else
				for(p = string; isalnum(*p) || *p == '_'; p++) ;
			if (p == string)	/* empty */
			{
				copystr = "${";
				copylen = 1 + braced;
			}
			else
			{
				if (p - string == 7 && memcmp(string, "basedir", 7) == 0)
				{
					copystr = basedir;
					copylen = varlen;
				}
				else	/* undefined variable */
					copylen = 0;
				string = p + braced;
			}
		}
		else	/* search next */
		{
			p = strchr(string, '$');
			if (p == NULL)
				copylen = strlen(string);
			else
				copylen = p - string;
			copystr = string;
			string += copylen;
		}
	}
	expstr[explen] = '\0';
	return expstr;
}

#define MAXWORDS 130
#define CHECKERRLIMIT \
  if(++errcnt >= 10) { \
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, \
      "Too many errors... Give up read %s", name); \
    reuse_mblock(&varbuf); \
    close_file(tf); return 1; }

MAIN_INTERFACE int read_config_file(char *name, int self)
{
    struct timidity_file *tf;
    char buf[1024], *tmp, *w[MAXWORDS + 1], *cp;
    ToneBank *bank = NULL;
    int i, j, k, line = 0, words, errcnt = 0;
    static int rcf_count = 0;
    int dr = 0, bankno = 0, mapid = INST_NO_MAP, origbankno = 0x7FFFFFFF;
    int extension_flag, param_parse_err;
    MBlockList varbuf;
    char *basedir, *sep;

    if(rcf_count > 50)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Probable source loop in configuration files");
	return 2;
    }

    if(self)
    {
	tf = open_with_mem(name, (int32)strlen(name), OF_VERBOSE);
	name = "(configuration)";
    }
    else
	tf = open_file(name, 1, OF_VERBOSE);
    if(tf == NULL)
	return 1;

	init_mblock(&varbuf);
	if (!self)
	{
		basedir = strdup_mblock(&varbuf, current_filename);
		if (is_url_prefix(basedir))
			sep = strrchr(basedir, '/');
		else
			sep = pathsep_strrchr(basedir);
	}
	else
		sep = NULL;
	if (sep == NULL)
	{
		#ifndef __MACOS__
		basedir = ".";
		#else
		basedir = "";
		#endif
	}
	else
	{
		if ((cp = strchr(sep, '#')) != NULL)
			sep = cp + 1;	/* inclusive of '#' */
		*sep = '\0';
	}

    errno = 0;
    while(tf_gets(buf, sizeof(buf), tf))
    {
	line++;
	if(strncmp(buf, "#extension", 10) == 0) {
	    extension_flag = 1;
	    i = 10;
	}
	else
	{
	    extension_flag = 0;
	    i = 0;
	}

	while(isspace(buf[i]))			/* skip /^\s*(?#)/ */
	    i++;
	if (buf[i] == '#' || buf[i] == '\0')	/* /^#|^$/ */
	    continue;
	tmp = expand_variables(buf, &varbuf, basedir);
	j = strcspn(tmp + i, " \t\r\n\240");
	if (j == 0)
		j = strlen(tmp + i);
	j = strip_trailing_comment(tmp + i, j);
	tmp[i + j] = '\0';			/* terminate the first token */
	w[0] = tmp + i;
	i += j + 1;
	words = param_parse_err = 0;
	while(words < MAXWORDS - 1)		/* -1 : next arg */
	{
	    char *terminator;

	    while(isspace(tmp[i]))		/* skip /^\s*(?#)/ */
		i++;
	    if (tmp[i] == '\0'
		    || tmp[i] == '#')		/* /\s#/ */
		break;
	    if ((tmp[i] == '"' || tmp[i] == '\'')
		    && (terminator = strchr(tmp + i + 1, tmp[i])) != NULL)
	    {
		if (!isspace(terminator[1]) && terminator[1] != '\0')
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			"%s: line %d: there must be at least one whitespace between "
			"string terminator (%c) and the next parameter", name, line, tmp[i]);
		    CHECKERRLIMIT;
		    param_parse_err = 1;
		    break;
		}
		w[++words] = tmp + i + 1;
		i = terminator - tmp + 1;
		*terminator = '\0';
	    }
	    else	/* not terminated */
	    {
		j = strcspn(tmp + i, " \t\r\n\240");
		if (j > 0)
		    j = strip_trailing_comment(tmp + i, j);
		w[++words] = tmp + i;
		i += j;
		if (tmp[i] != '\0')		/* unless at the end-of-string (i.e. EOF) */
		    tmp[i++] = '\0';		/* terminate the token */
	    }
	}
	if (param_parse_err)
	    continue;
	w[++words] = NULL;

	/*
	 * #extension [something...]
	 */

	/* #extension comm program comment */
	if(strcmp(w[0], "comm") == 0)
	{
	    char *p;

	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum "
			  "set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension comm must be "
			  "between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(bank->tone[i].comment)
		free(bank->tone[i].comment);
	    p = bank->tone[i].comment = safe_strdup(w[2]);
	    while(*p)
	    {
		if(*p == ',') *p = ' ';
		p++;
	    }
	}
	/* #extension timeout program sec */
	else if(strcmp(w[0], "timeout") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension timeout "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    bank->tone[i].loop_timeout = atoi(w[2]);
	}
	/* #extension copydrumset drumset */
	else if(strcmp(w[0], "copydrumset") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No copydrumset number given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension copydrumset "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or "
			  "drum set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    copybank(bank, drumset[i], mapid, origbankno, bankno);
	}
	/* #extension copybank bank */
	else if(strcmp(w[0], "copybank") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No copybank number given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension copybank "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or "
			  "drum set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    copybank(bank, tonebank[i], mapid, origbankno, bankno);
	}
	/* #extension copymap tomapid frommapid */
	else if(strcmp(w[0], "copymap") == 0)
	{
	    int mapto, mapfrom;
	    int toisdrum, fromisdrum;

	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if ((mapto = mapname2id(w[1], &toisdrum)) == -1)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid map name: %s", name, line, w[1]);
		CHECKERRLIMIT;
		continue;
	    }
	    if ((mapfrom = mapname2id(w[2], &fromisdrum)) == -1)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid map name: %s", name, line, w[2]);
		CHECKERRLIMIT;
		continue;
	    }
	    if (toisdrum != fromisdrum)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Map type should be matched", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if (copymap(mapto, mapfrom, toisdrum))
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No free %s available to map",
			  name, line, toisdrum ? "drum set" : "tone bank");
		CHECKERRLIMIT;
		continue;
	    }
	}
	/* #extension HTTPproxy hostname:port */
	else if(strcmp(w[0], "HTTPproxy") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No proxy name given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    /* If network is not supported, this extension is ignored. */
#ifdef SUPPORT_SOCKET
	    url_http_proxy_host = safe_strdup(w[1]);
	    if((cp = strchr(url_http_proxy_host, ':')) == NULL)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    *cp++ = '\0';
	    if((url_http_proxy_port = atoi(cp)) <= 0)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Port number must be "
			  "positive number", name, line);
		CHECKERRLIMIT;
		continue;
	    }
#endif
	}
	/* #extension FTPproxy hostname:port */
	else if(strcmp(w[0], "FTPproxy") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No proxy name given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    /* If network is not supported, this extension is ignored. */
#ifdef SUPPORT_SOCKET
	    url_ftp_proxy_host = safe_strdup(w[1]);
	    if((cp = strchr(url_ftp_proxy_host, ':')) == NULL)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    *cp++ = '\0';
	    if((url_ftp_proxy_port = atoi(cp)) <= 0)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Port number "
			  "must be positive number", name, line);
		CHECKERRLIMIT;
		continue;
	    }
#endif
	}
	/* #extension mailaddr somebody@someware.domain.com */
	else if(strcmp(w[0], "mailaddr") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No mail address given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(strchr(w[1], '@') == NULL) {
		ctl->cmsg(CMSG_WARNING, VERB_NOISY,
			  "%s: line %d: Warning: Mail address %s is not valid",
			  name, line);
	    }

	    /* If network is not supported, this extension is ignored. */
#ifdef SUPPORT_SOCKET
	    user_mailaddr = safe_strdup(w[1]);
#endif /* SUPPORT_SOCKET */
	}
	/* #extension opt [-]{option}[optarg] */
	else if (strcmp(w[0], "opt") == 0) {
		int c, longind, err;
		char *p, *cmd, *arg;
		
		if (words != 2 && words != 3) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"%s: line %d: Syntax error", name, line);
			CHECKERRLIMIT;
			continue;
		}
		if (*w[1] == '-') {
			int optind_save = optind;
			optind = 0;
			c = timidity_getopt_long(words, w, optcommands, longopts, &longind);
			err = set_tim_opt_long(c, optarg, longind);
			optind = optind_save;
		} else {
			/* backward compatibility */
			if ((p = strchr(optcommands, c = *(cmd = w[1]))) == NULL)
				err = 1;
			else {
				if (*(p + 1) == ':')
					arg = (words == 2) ? cmd + 1 : w[2];
				else
					arg = "";
				err = set_tim_opt_short(c, arg);
			}
		}
		if (err) {
			/* error */
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"%s: line %d: Invalid command line option",
					name, line);
			errcnt += err - 1;
			CHECKERRLIMIT;
			continue;
		}
	}
	/* #extension undef program */
	else if(strcmp(w[0], "undef") == 0)
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No undef number given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension undef "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or "
			  "drum set before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    free_tone_bank_element(&bank->tone[i]);
	}
	/* #extension altassign numbers... */
	else if(strcmp(w[0], "altassign") == 0)
	{
	    ToneBank *bk;

	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before altassign", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No alternate assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    if(!dr) {
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
			  "%s: line %d: Warning: Not a drumset altassign"
			  " (ignored)",
			  name, line);
		continue;
	    }

	    bk = drumset[bankno];
	    bk->alt = add_altassign_string(bk->alt, w + 1, words - 1);
	}	/* #extension legato [program] [0 or 1] */
	else if(strcmp(w[0], "legato") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension legato "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    bank->tone[i].legato = atoi(w[2]);
	}	/* #extension damper [program] [0 or 1] */
	else if(strcmp(w[0], "damper") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension damper "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    bank->tone[i].damper_mode = atoi(w[2]);
	}	/* #extension rnddelay [program] [0 or 1] */
	else if(strcmp(w[0], "rnddelay") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension rnddelay "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    bank->tone[i].rnddelay = atoi(w[2]);
	}	/* #extension level program tva_level */
	else if(strcmp(w[0], "level") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
		i = atoi(w[2]);
		if(i < 0 || i > 127)
		{
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension level "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
		}
		cp = w[1];
		do {
			if (string_to_7bit_range(cp, &j, &k))
			{
				while (j <= k)
					bank->tone[j++].tva_level = i;
			}
			cp = strchr(cp, ',');
		} while(cp++ != NULL);
	}	/* #extension reverbsend */
	else if(strcmp(w[0], "reverbsend") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
		i = atoi(w[2]);
		if(i < 0 || i > 127)
		{
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension reverbsend "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
		}
		cp = w[1];
		do {
			if (string_to_7bit_range(cp, &j, &k))
			{
				while (j <= k)
					bank->tone[j++].reverb_send = i;
			}
			cp = strchr(cp, ',');
		} while(cp++ != NULL);
	}	/* #extension chorussend */
	else if(strcmp(w[0], "chorussend") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
		i = atoi(w[2]);
		if(i < 0 || i > 127)
		{
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension chorussend "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
		}
		cp = w[1];
		do {
			if (string_to_7bit_range(cp, &j, &k))
			{
				while (j <= k)
					bank->tone[j++].chorus_send = i;
			}
			cp = strchr(cp, ',');
		} while(cp++ != NULL);
	}	/* #extension delaysend */
	else if(strcmp(w[0], "delaysend") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
		i = atoi(w[2]);
		if(i < 0 || i > 127)
		{
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension delaysend "
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
		}
		cp = w[1];
		do {
			if (string_to_7bit_range(cp, &j, &k))
			{
				while (j <= k)
					bank->tone[j++].delay_send = i;
			}
			cp = strchr(cp, ',');
		} while(cp++ != NULL);
	}	/* #extension playnote */
	else if(strcmp(w[0], "playnote") == 0)
	{
	    if(words != 3)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!bank)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify tone bank or drum set "
			  "before assignment", name, line);
		CHECKERRLIMIT;
		continue;
	    }
		i = atoi(w[2]);
		if(i < 0 || i > 127)
		{
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: extension playnote"
			  "must be between 0 and 127", name, line);
		CHECKERRLIMIT;
		continue;
		}
		cp = w[1];
		do {
			if (string_to_7bit_range(cp, &j, &k))
			{
				while (j <= k)
					bank->tone[j++].play_note = i;
			}
			cp = strchr(cp, ',');
		} while(cp++ != NULL);
	}
	else if(!strcmp(w[0], "soundfont"))
	{
	    int order, cutoff, isremove, reso, amp;
	    char *sf_file;

	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No soundfont file given",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    sf_file = w[1];
	    order = cutoff = reso = amp = -1;
	    isremove = 0;
	    for(j = 2; j < words; j++)
	    {
		if(strcmp(w[j], "remove") == 0)
		{
		    isremove = 1;
		    break;
		}
		if(!(cp = strchr(w[j], '=')))
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: bad patch option %s",
			      name, line, w[j]);
		    CHECKERRLIMIT;
		    break;
		}
		*cp++=0;
		k = atoi(cp);
		if(!strcmp(w[j], "order"))
		{
		    if(k < 0 || (*cp < '0' || *cp > '9'))
		    {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "%s: line %d: order must be a digit",
				  name, line);
			CHECKERRLIMIT;
			break;
		    }
		    order = k;
		}
		else if(!strcmp(w[j], "cutoff"))
		{
		    if(k < 0 || (*cp < '0' || *cp > '9'))
		    {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "%s: line %d: cutoff must be a digit",
				  name, line);
			CHECKERRLIMIT;
			break;
		    }
		    cutoff = k;
		}
		else if(!strcmp(w[j], "reso"))
		{
		    if(k < 0 || (*cp < '0' || *cp > '9'))
		    {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				  "%s: line %d: reso must be a digit",
				  name, line);
			CHECKERRLIMIT;
			break;
		    }
		    reso = k;
		}
		else if(!strcmp(w[j], "amp"))
		{
		    amp = k;
		}
	    }
	    if(isremove)
		remove_soundfont(sf_file);
	    else
		add_soundfont(sf_file, order, cutoff, reso, amp);
	}
	else if(!strcmp(w[0], "font"))
	{
	    int bank, preset, keynote;
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: no font command", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if(!strcmp(w[1], "exclude"))
	    {
		if(words < 3)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No bank/preset/key is given",
			      name, line);
		    CHECKERRLIMIT;
		    continue;
		}
		bank = atoi(w[2]);
		if(words >= 4)
		    preset = atoi(w[3]) - progbase;
		else
		    preset = -1;
		if(words >= 5)
		    keynote = atoi(w[4]);
		else
		    keynote = -1;
		if(exclude_soundfont(bank, preset, keynote))
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No soundfont is given",
			      name, line);
		    CHECKERRLIMIT;
		}
	    }
	    else if(!strcmp(w[1], "order"))
	    {
		int order;
		if(words < 4)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No order/bank is given",
			      name, line);
		    CHECKERRLIMIT;
		    continue;
		}
		order = atoi(w[2]);
		bank = atoi(w[3]);
		if(words >= 5)
		    preset = atoi(w[4]) - progbase;
		else
		    preset = -1;
		if(words >= 6)
		    keynote = atoi(w[5]);
		else
		    keynote = -1;
		if(order_soundfont(bank, preset, keynote, order))
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: No soundfont is given",
			      name, line);
		    CHECKERRLIMIT;
		}
	    }
	}
	else if(!strcmp(w[0], "progbase"))
	{
	    if(words < 2 || *w[1] < '0' || *w[1] > '9')
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    progbase = atoi(w[1]);
	}
	else if(!strcmp(w[0], "map")) /* map <name> set1 elem1 set2 elem2 */
	{
	    int arg[5], isdrum;

	    if(words != 6)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if((arg[0] = mapname2id(w[1], &isdrum)) == -1)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid map name: %s", name, line, w[1]);
		CHECKERRLIMIT;
		continue;
	    }
	    for(i = 2; i < 6; i++)
		arg[i - 1] = atoi(w[i]);
	    if(isdrum)
	    {
		arg[1] -= progbase;
		arg[3] -= progbase;
	    }
	    else
	    {
		arg[2] -= progbase;
		arg[4] -= progbase;
	    }

	    for(i = 1; i < 5; i++)
		if(arg[i] < 0 || arg[i] > 127)
		    break;
	    if(i != 5)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid parameter", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    set_instrument_map(arg[0], arg[1], arg[2], arg[3], arg[4]);
	}

	/*
	 * Standard configurations
	 */
	else if(!strcmp(w[0], "dir"))
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No directory given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    for(i = 1; i < words; i++)
		add_to_pathlist(w[i]);
	}
	else if(!strcmp(w[0], "source"))
	{
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No file name given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    for(i = 1; i < words; i++)
	    {
		int status;
		rcf_count++;
		status = read_config_file(w[i], 0);
		rcf_count--;
		if(status == 2)
		{
		    reuse_mblock(&varbuf);
		    close_file(tf);
		    return 2;
		}
		else if(status != 0)
		{

		    CHECKERRLIMIT;
		    continue;
		}
	    }
	}
	else if(!strcmp(w[0], "default"))
	{
	    if(words != 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Must specify exactly one patch name",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    strncpy(def_instr_name, w[1], 255);
	    def_instr_name[255] = '\0';
	    default_instrument_name = def_instr_name;
	}
	/* drumset [mapid] num */
	else if(!strcmp(w[0], "drumset"))
	{
	    int newmapid, isdrum, newbankno;
	    
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No drum set number given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if (words != 2 && !isdigit(*w[1]))
	    {
		if ((newmapid = mapname2id(w[1], &isdrum)) == -1 || !isdrum)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid drum set map name: %s", name, line, w[1]);
		    CHECKERRLIMIT;
		    continue;
		}
		words--;
		memmove(&w[1], &w[2], sizeof w[0] * words);
	    }
	    else
		newmapid = INST_NO_MAP;
	    i = atoi(w[1]) - progbase;
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Drum set must be between %d and %d",
			  name, line,
			  progbase, progbase + 127);
		CHECKERRLIMIT;
		continue;
	    }

	    newbankno = i;
	    i = alloc_instrument_map_bank(1, newmapid, i);
	    if (i == -1)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No free drum set available to map",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    if(words == 2)
	    {
		bank = drumset[i];
		bankno = i;
		mapid = newmapid;
		origbankno = newbankno;
		dr = 1;
	    }
	    else
	    {
		if(words < 4 || *w[2] < '0' || *w[2] > '9')
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: syntax error", name, line);
		    CHECKERRLIMIT;
		    continue;
		}
		if (set_patchconf(name, line, drumset[i], &w[2], 1, newmapid, newbankno, i))
		{
		    CHECKERRLIMIT;
		    continue;
		}
	    }
	}
	/* bank [mapid] num */
	else if(!strcmp(w[0], "bank"))
	{
	    int newmapid, isdrum, newbankno;
	    
	    if(words < 2)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No bank number given", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if (words != 2 && !isdigit(*w[1]))
	    {
		if ((newmapid = mapname2id(w[1], &isdrum)) == -1 || isdrum)
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Invalid bank map name: %s", name, line, w[1]);
		    CHECKERRLIMIT;
		    continue;
		}
		words--;
		memmove(&w[1], &w[2], sizeof w[0] * words);
	    }
	    else
		newmapid = INST_NO_MAP;
	    i = atoi(w[1]);
	    if(i < 0 || i > 127)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: Tone bank must be between 0 and 127",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    newbankno = i;
	    i = alloc_instrument_map_bank(0, newmapid, i);
	    if (i == -1)
	    {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: No free tone bank available to map",
			  name, line);
		CHECKERRLIMIT;
		continue;
	    }

	    if(words == 2)
	    {
		bank = tonebank[i];
		bankno = i;
		mapid = newmapid;
		origbankno = newbankno;
		dr = 0;
	    }
	    else
	    {
		if(words < 4 || *w[2] < '0' || *w[2] > '9')
		{
		    ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			      "%s: line %d: syntax error", name, line);
		    CHECKERRLIMIT;
		    continue;
		}
		if (set_patchconf(name, line, tonebank[i], &w[2], 0, newmapid, newbankno, i))
		{
		    CHECKERRLIMIT;
		    continue;
		}
	    }
	}
	else
	{
	    if(words < 2 || *w[0] < '0' || *w[0] > '9')
	    {
		if(extension_flag)
		    continue;
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "%s: line %d: syntax error", name, line);
		CHECKERRLIMIT;
		continue;
	    }
	    if (set_patchconf(name, line, bank, w, dr, mapid, origbankno, bankno))
	    {
		CHECKERRLIMIT;
		continue;
	    }
	}
    }
    if(errno)
    {
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
		  "Can't read %s: %s", name, strerror(errno));
	errcnt++;
    }
    reuse_mblock(&varbuf);
    close_file(tf);
    return errcnt != 0;
}

#ifdef SUPPORT_SOCKET

#if defined(__W32__) && !defined(MAIL_NAME)
#define MAIL_NAME "anonymous"
#endif /* __W32__ */

#ifdef MAIL_NAME
#define get_username() MAIL_NAME
#else /* MAIL_NAME */
#include <pwd.h>
static char *get_username(void)
{
    char *p;
    struct passwd *pass;

    /* USER
     * LOGIN
     * LOGNAME
     * getpwnam()
     */

    if((p = getenv("USER")) != NULL)
        return p;
    if((p = getenv("LOGIN")) != NULL)
        return p;
    if((p = getenv("LOGNAME")) != NULL)
        return p;

    pass = getpwuid(getuid());
    if(pass == NULL)
        return "nobody";
    return pass->pw_name;
}
#endif /* MAIL_NAME */

static void init_mail_addr(void)
{
    char addr[BUFSIZ];

    sprintf(addr, "%s%s", get_username(), MAIL_DOMAIN);
    user_mailaddr = safe_strdup(addr);
}
#endif /* SUPPORT_SOCKET */

static int read_user_config_file(void)
{
    char *home;
    char path[BUFSIZ];
    int opencheck;

#ifdef __W32__
/* HOME or home */
    home = getenv("HOME");
    if(home == NULL)
	home = getenv("home");
    if(home == NULL)
    {
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		  "Warning: HOME environment is not defined.");
	return 0;
    }
/* .timidity.cfg or timidity.cfg */
    sprintf(path, "%s" PATH_STRING "timidity.cfg", home);
    if((opencheck = open(path, 0)) < 0)
    {
	sprintf(path, "%s" PATH_STRING "_timidity.cfg", home);
	if((opencheck = open(path, 0)) < 0)
	{
	    sprintf(path, "%s" PATH_STRING ".timidity.cfg", home);
	    if((opencheck = open(path, 0)) < 0)
	    {
		ctl->cmsg(CMSG_INFO, VERB_NOISY, "%s: %s",
			  path, strerror(errno));
		return 0;
	    }
	}
    }

    close(opencheck);
    return read_config_file(path, 0);
#else
    home = getenv("HOME");
    if(home == NULL)
    {
	ctl->cmsg(CMSG_INFO, VERB_NOISY,
		  "Warning: HOME environment is not defined.");
	return 0;
    }
    sprintf(path, "%s" PATH_STRING ".timidity.cfg", home);

    if((opencheck = open(path, 0)) < 0)
    {
	ctl->cmsg(CMSG_INFO, VERB_NOISY, "%s: %s",
		  path, strerror(errno));
	return 0;
    }

    close(opencheck);
    return read_config_file(path, 0);
#endif /* __W32__ */
}

MAIN_INTERFACE void tmdy_free_config(void)
{
	free_tone_bank();
	free_instrument_map();
	clean_up_pathlist();
}

int set_extension_modes(char *flag)
{
	return parse_opt_E(flag);
}

int set_ctl(char *cp)
{
	return parse_opt_i(cp);
}

int set_play_mode(char *cp)
{
	return parse_opt_O(cp);
}

int set_wrd(char *w)
{
    return -1;
}

#ifdef __W32__
int opt_evil_mode = 0;
#ifdef SMFCONV
int opt_rcpcv_dll = 0;
#endif	/* SMFCONV */
#endif	/* __W32__ */
static int   try_config_again = 0;
int32 opt_output_rate = 0;
static char *opt_output_name = NULL;
static StringTable opt_config_string;
#ifdef SUPPORT_SOUNDSPEC
static double spectrogram_update_sec = 0.0;
#endif /* SUPPORT_SOUNDSPEC */
int opt_buffer_fragments = -1;

MAIN_INTERFACE int set_tim_opt_short(int c, char *optarg)
{
	int err = 0;
	
	switch (c) {
	case '4':
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"-4 option is obsoleted.  Please use -N");
		return 1;
	case 'A':
		if (*optarg != ',' && *optarg != 'a')
			err += parse_opt_A(optarg);
		if (strchr(optarg, ','))
			err += parse_opt_drum_power(strchr(optarg, ',') + 1);
		if (strchr(optarg, 'a'))
			opt_amp_compensation = 1;
		return err;
	case 'a':
		antialiasing_allowed = 1;
		break;
	case 'B':
		return parse_opt_B(optarg);
	case 'C':
		return parse_opt_C(optarg);
	case 'c':
		return parse_opt_c(optarg);
	case 'D':
		return parse_opt_D(optarg);
	case 'd':
		return parse_opt_d(optarg);
	case 'E':
		return parse_opt_E(optarg);
	case 'e':
		return parse_opt_e(optarg);
	case 'F':
		adjust_panning_immediately = (adjust_panning_immediately) ? 0 : 1;
		break;
	case 'f':
		fast_decay = (fast_decay) ? 0 : 1;
		break;
	case 'g':
		return parse_opt_g(optarg);
	case 'H':
		return parse_opt_H(optarg);
	case 'h':
		return parse_opt_h(optarg);
	case 'I':
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"-I option is obsoleted.  Please use -Ei");
		return 1;
	case 'i':
		return parse_opt_i(optarg);
	case 'j':
		opt_realtime_playing = (opt_realtime_playing) ? 0 : 1;
		break;
	case 'K':
		return parse_opt_K(optarg);
	case 'k':
		return parse_opt_k(optarg);
	case 'L':
		return parse_opt_L(optarg);
	case 'M':
		return parse_opt_M(optarg);
	case 'm':
		return parse_opt_m(optarg);
	case 'N':
		return parse_opt_N(optarg);
	case 'O':
		return parse_opt_O(optarg);
	case 'o':
		return parse_opt_o(optarg);
	case 'P':
		return parse_opt_P(optarg);
	case 'p':
		if (*optarg != 'a')
			err += parse_opt_p(optarg);
		if (strchr(optarg, 'a'))
			auto_reduce_polyphony = (auto_reduce_polyphony) ? 0 : 1;
		return err;
	case 'Q':
		return parse_opt_Q(optarg);
	case 'q':
		return parse_opt_q(optarg);
	case 'R':
		return parse_opt_R(optarg);
	case 'S':
		return parse_opt_S(optarg);
	case 's':
		return parse_opt_s(optarg);
	case 'T':
		return parse_opt_T(optarg);
	case 't':
		return parse_opt_t(optarg);
	case 'U':
		free_instruments_afterwards = 1;
		break;
	case 'V':
		return parse_opt_volume_curve(optarg);
	case 'v':
		return parse_opt_v(optarg);
#ifdef __W32__
	case 'w':
		return parse_opt_w(optarg);
#endif
	case 'x':
		return parse_opt_x(optarg);
	case 'Z':
		if (strncmp(optarg, "pure", 4))
			return parse_opt_Z(optarg);
		else
			return parse_opt_Z1(optarg + 4);
	default:
		return 1;
	}
	return 0;
}

/* -------- getopt_long -------- */
MAIN_INTERFACE int set_tim_opt_long(int c, char *optarg, int index)
{
	const struct option *the_option = &(longopts[index]);
	char *arg;
	
	if (c == '?')	/* getopt_long failed parsing */
		parse_opt_fail(optarg);
	else if (c < TIM_OPT_FIRST)
		return set_tim_opt_short(c, optarg);
	if (! strncmp(the_option->name, "no-", 3))
		arg = "no";		/* `reverse' switch */
	else
		arg = optarg;
	switch (c) {
	case TIM_OPT_VOLUME:
		return parse_opt_A(arg);
	case TIM_OPT_DRUM_POWER:
		return parse_opt_drum_power(arg);
	case TIM_OPT_VOLUME_COMP:
		return parse_opt_volume_comp(arg);
	case TIM_OPT_ANTI_ALIAS:
		return parse_opt_a(arg);
	case TIM_OPT_BUFFER_FRAGS:
		return parse_opt_B(arg);
	case TIM_OPT_CONTROL_RATIO:
		return parse_opt_C(arg);
	case TIM_OPT_CONFIG_FILE:
		return parse_opt_c(arg);
	case TIM_OPT_DRUM_CHANNEL:
		return parse_opt_D(arg);
	case TIM_OPT_IFACE_PATH:
		return parse_opt_d(arg);
	case TIM_OPT_EXT:
		return parse_opt_E(arg);
	case TIM_OPT_MOD_WHEEL:
		return parse_opt_mod_wheel(arg);
	case TIM_OPT_PORTAMENTO:
		return parse_opt_portamento(arg);
	case TIM_OPT_VIBRATO:
		return parse_opt_vibrato(arg);
	case TIM_OPT_CH_PRESS:
		return parse_opt_ch_pressure(arg);
	case TIM_OPT_MOD_ENV:
		return parse_opt_mod_env(arg);
	case TIM_OPT_TRACE_TEXT:
		return parse_opt_trace_text(arg);
	case TIM_OPT_OVERLAP:
		return parse_opt_overlap_voice(arg);
	case TIM_OPT_TEMPER_CTRL:
		return parse_opt_temper_control(arg);
	case TIM_OPT_DEFAULT_MID:
		return parse_opt_default_mid(arg);
	case TIM_OPT_SYSTEM_MID:
		return parse_opt_system_mid(arg);
	case TIM_OPT_DEFAULT_BANK:
		return parse_opt_default_bank(arg);
	case TIM_OPT_FORCE_BANK:
		return parse_opt_force_bank(arg);
	case TIM_OPT_DEFAULT_PGM:
		return parse_opt_default_program(arg);
	case TIM_OPT_FORCE_PGM:
		return parse_opt_force_program(arg);
	case TIM_OPT_DELAY:
		return parse_opt_delay(arg);
	case TIM_OPT_CHORUS:
		return parse_opt_chorus(arg);
	case TIM_OPT_REVERB:
		return parse_opt_reverb(arg);
	case TIM_OPT_VOICE_LPF:
		return parse_opt_voice_lpf(arg);
	case TIM_OPT_NS:
		return parse_opt_noise_shaping(arg);
#ifndef FIXED_RESAMPLATION
	case TIM_OPT_RESAMPLE:
		return parse_opt_resample(arg);
#endif
	case TIM_OPT_EVIL:
		return parse_opt_e(arg);
	case TIM_OPT_FAST_PAN:
		return parse_opt_F(arg);
	case TIM_OPT_FAST_DECAY:
		return parse_opt_f(arg);
	case TIM_OPT_SPECTROGRAM:
		return parse_opt_g(arg);
	case TIM_OPT_KEYSIG:
		return parse_opt_H(arg);
	case TIM_OPT_HELP:
		return parse_opt_h(arg);
	case TIM_OPT_INTERFACE:
		return parse_opt_i(arg);
	case TIM_OPT_VERBOSE:
		return parse_opt_verbose(arg);
	case TIM_OPT_QUIET:
		return parse_opt_quiet(arg);
	case TIM_OPT_TRACE:
		return parse_opt_trace(arg);
	case TIM_OPT_LOOP:
		return parse_opt_loop(arg);
	case TIM_OPT_RANDOM:
		return parse_opt_random(arg);
	case TIM_OPT_SORT:
		return parse_opt_sort(arg);
#ifdef IA_ALSASEQ
	case TIM_OPT_BACKGROUND:
		return parse_opt_background(arg);
	case TIM_OPT_RT_PRIO:
		return parse_opt_rt_prio(arg);
	case TIM_OPT_SEQ_PORTS:
		return parse_opt_seq_ports(arg);
#endif
	case TIM_OPT_REALTIME_LOAD:
		return parse_opt_j(arg);
	case TIM_OPT_ADJUST_KEY:
		return parse_opt_K(arg);
	case TIM_OPT_VOICE_QUEUE:
		return parse_opt_k(arg);
	case TIM_OPT_PATCH_PATH:
		return parse_opt_L(arg);
	case TIM_OPT_PCM_FILE:
		return parse_opt_M(arg);
	case TIM_OPT_DECAY_TIME:
		return parse_opt_m(arg);
	case TIM_OPT_INTERPOLATION:
		return parse_opt_N(arg);
	case TIM_OPT_OUTPUT_MODE:
		return parse_opt_O(arg);
	case TIM_OPT_OUTPUT_STEREO:
		if (! strcmp(the_option->name, "output-mono"))
			/* --output-mono == --output-stereo=no */
			arg = "no";
		return parse_opt_output_stereo(arg);
	case TIM_OPT_OUTPUT_SIGNED:
		if (! strcmp(the_option->name, "output-unsigned"))
			/* --output-unsigned == --output-signed=no */
			arg = "no";
		return parse_opt_output_signed(arg);
	case TIM_OPT_OUTPUT_BITWIDTH:
		if (! strcmp(the_option->name, "output-16bit"))
			arg = "16bit";
		else if (! strcmp(the_option->name, "output-24bit"))
			arg = "24bit";
		else if (! strcmp(the_option->name, "output-8bit"))
			arg = "8bit";
		return parse_opt_output_bitwidth(arg);
	case TIM_OPT_OUTPUT_FORMAT:
		if (! strcmp(the_option->name, "output-linear"))
			arg = "linear";
		else if (! strcmp(the_option->name, "output-ulaw"))
			arg = "ulaw";
		else if (! strcmp(the_option->name, "output-alaw"))
			arg = "alaw";
		return parse_opt_output_format(arg);
	case TIM_OPT_OUTPUT_SWAB:
		return parse_opt_output_swab(arg);
#ifdef AU_FLAC
	case TIM_OPT_FLAC_VERIFY:
		return parse_opt_flac_verify(arg);
	case TIM_OPT_FLAC_PADDING:
		return parse_opt_flac_padding(arg);
	case TIM_OPT_FLAC_COMPLEVEL:
		return parse_opt_flac_complevel(arg);
#ifdef AU_OGGFLAC
	case TIM_OPT_FLAC_OGGFLAC:
		return parse_opt_flac_oggflac(arg);
#endif /* AU_OGGFLAC */
#endif /* AU_FLAC */
#ifdef AU_SPEEX
	case TIM_OPT_SPEEX_QUALITY:
		return parse_opt_speex_quality(arg);
	case TIM_OPT_SPEEX_VBR:
		return parse_opt_speex_vbr(arg);
	case TIM_OPT_SPEEX_ABR:
		return parse_opt_speex_abr(arg);
	case TIM_OPT_SPEEX_VAD:
		return parse_opt_speex_vad(arg);
	case TIM_OPT_SPEEX_DTX:
		return parse_opt_speex_dtx(arg);
	case TIM_OPT_SPEEX_COMPLEXITY:
		return parse_opt_speex_complexity(arg);
	case TIM_OPT_SPEEX_NFRAMES:
		return parse_opt_speex_nframes(arg);
#endif /* AU_SPEEX */
	case TIM_OPT_OUTPUT_FILE:
		return parse_opt_o(arg);
	case TIM_OPT_PATCH_FILE:
		return parse_opt_P(arg);
	case TIM_OPT_POLYPHONY:
		return parse_opt_p(arg);
	case TIM_OPT_POLY_REDUCE:
		return parse_opt_p1(arg);
	case TIM_OPT_MUTE:
		return parse_opt_Q(arg);
	case TIM_OPT_TEMPER_MUTE:
		return parse_opt_Q1(arg);
	case TIM_OPT_AUDIO_BUFFER:
		return parse_opt_q(arg);
	case TIM_OPT_CACHE_SIZE:
		return parse_opt_S(arg);
	case TIM_OPT_SAMPLE_FREQ:
		return parse_opt_s(arg);
	case TIM_OPT_ADJUST_TEMPO:
		return parse_opt_T(arg);
	case TIM_OPT_CHARSET:
		return parse_opt_t(arg);
	case TIM_OPT_UNLOAD_INST:
		return parse_opt_U(arg);
	case TIM_OPT_VOLUME_CURVE:
		return parse_opt_volume_curve(arg);
	case TIM_OPT_VERSION:
		return parse_opt_v(arg);
#ifdef __W32__
	case TIM_OPT_RCPCV_DLL:
		return parse_opt_w(arg);
#endif
	case TIM_OPT_CONFIG_STR:
		return parse_opt_x(arg);
	case TIM_OPT_FREQ_TABLE:
		return parse_opt_Z(arg);
	case TIM_OPT_PURE_INT:
		return parse_opt_Z1(arg);
	case TIM_OPT_MODULE:
		return parse_opt_default_module(arg);
	default:
		ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
				"[BUG] Inconceivable case branch %d", c);
		abort();
	}
}

static inline int parse_opt_A(const char *arg)
{
	/* amplify volume by n percent */
	return set_val_i32(&amplification, atoi(arg), 0, MAX_AMPLIFICATION,
			"Amplification");
}

static inline int parse_opt_drum_power(const char *arg)
{
	/* --drum-power */
	return set_val_i32(&opt_drum_power, atoi(arg), 0, MAX_AMPLIFICATION,
			"Drum power");
}

static inline int parse_opt_volume_comp(const char *arg)
{
	/* --[no-]volume-compensation */
	opt_amp_compensation = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_a(const char *arg)
{
	antialiasing_allowed = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_B(const char *arg)
{
	/* --buffer-fragments */
	const char *p;
	
	/* num */
	if (*arg != ',') {
		if (set_value(&opt_buffer_fragments, atoi(arg), 0, 1000,
				"Buffer Fragments (num)"))
			return 1;
	}
	/* bits */
	if ((p = strchr(arg, ',')) != NULL) {
		if (set_value(&audio_buffer_bits, atoi(++p), 1, AUDIO_BUFFER_BITS,
				"Buffer Fragments (bit)"))
			return 1;
	}
	return 0;
}

static inline int parse_opt_C(const char *arg)
{
	if (set_val_i32(&control_ratio, atoi(arg), 0, MAX_CONTROL_RATIO,
			"Control ratio"))
		return 1;
	opt_control_ratio = control_ratio;
	return 0;
}

static inline int parse_opt_c(char *arg)
{
	if (read_config_file(arg, 0))
		return 1;
	got_a_configuration = 1;
	return 0;
}

static inline int parse_opt_D(const char *arg)
{
	return set_channel_flag(&default_drumchannels, atoi(arg), "Drum channel");
}

static inline int parse_opt_d(const char *arg)
{
	/* dynamic lib root */
#ifdef IA_DYNAMIC
	if (dynamic_lib_root)
		free(dynamic_lib_root);
	dynamic_lib_root = safe_strdup(arg);
	return 0;
#else
	ctl->cmsg(CMSG_WARNING, VERB_NOISY, "-d option is not supported");
	return 1;
#endif	/* IA_DYNAMIC */
}

static inline int parse_opt_E(char *arg)
{
	/* undocumented option --ext */
	int err = 0;
	
	while (*arg) {
		switch (*arg) {
		case 'w':
			opt_modulation_wheel = 1;
			break;
		case 'W':
			opt_modulation_wheel = 0;
			break;
		case 'p':
			opt_portamento = 1;
			break;
		case 'P':
			opt_portamento = 0;
			break;
		case 'v':
			opt_nrpn_vibrato = 1;
			break;
		case 'V':
			opt_nrpn_vibrato = 0;
			break;
		case 's':
			opt_channel_pressure = 1;
			break;
		case 'S':
			opt_channel_pressure = 0;
			break;
		case 'e':
			opt_modulation_envelope = 1;
			break;
		case 'E':
			opt_modulation_envelope = 0;
			break;
		case 't':
			opt_trace_text_meta_event = 1;
			break;
		case 'T':
			opt_trace_text_meta_event = 0;
			break;
		case 'o':
			opt_overlap_voice_allow = 1;
			break;
		case 'O':
			opt_overlap_voice_allow = 0;
			break;
		case 'z':
			opt_temper_control = 1;
			break;
		case 'Z':
			opt_temper_control = 0;
			break;
		case 'm':
			if (parse_opt_default_mid(arg + 1))
				err++;
			arg += 2;
			break;
		case 'M':
			if (parse_opt_system_mid(arg + 1))
				err++;
			arg += 2;
			break;
		case 'b':
			if (parse_opt_default_bank(arg + 1))
				err++;
			while (isdigit(*(arg + 1)))
				arg++;
			break;
		case 'B':
			if (parse_opt_force_bank(arg + 1))
				err++;
			while (isdigit(*(arg + 1)))
				arg++;
			break;
		case 'i':
			if (parse_opt_default_program(arg + 1))
				err++;
			while (isdigit(*(arg + 1)) || *(arg + 1) == '/')
				arg++;
			break;
		case 'I':
			if (parse_opt_force_program(arg + 1))
				err++;
			while (isdigit(*(arg + 1)) || *(arg + 1) == '/')
				arg++;
			break;
		case 'F':
			if (strncmp(arg + 1, "delay=", 6) == 0) {
				if (parse_opt_delay(arg + 7))
					err++;
			} else if (strncmp(arg + 1, "chorus=", 7) == 0) {
				if (parse_opt_chorus(arg + 8))
					err++;
			} else if (strncmp(arg + 1, "reverb=", 7) == 0) {
				if (parse_opt_reverb(arg + 8))
					err++;
			} else if (strncmp(arg + 1, "ns=", 3) == 0) {
				if (parse_opt_noise_shaping(arg + 4))
					err++;
#ifndef FIXED_RESAMPLATION
			} else if (strncmp(arg + 1, "resamp=", 7) == 0) {
				if (parse_opt_resample(arg + 8))
					err++;
#endif
			}
			if (err) {
				ctl->cmsg(CMSG_ERROR,
						VERB_NORMAL, "-E%s: unsupported effect", arg);
				return err;
			}
			return err;
		default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"-E: Illegal mode `%c'", *arg);
			err++;
			break;
		}
		arg++;
	}
	return err;
}

static inline int parse_opt_mod_wheel(const char *arg)
{
	/* --[no-]mod-wheel */
	opt_modulation_wheel = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_portamento(const char *arg)
{
	/* --[no-]portamento */
	opt_portamento = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_vibrato(const char *arg)
{
	/* --[no-]vibrato */
	opt_nrpn_vibrato = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_ch_pressure(const char *arg)
{
	/* --[no-]ch-pressure */
	opt_channel_pressure = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_mod_env(const char *arg)
{
	/* --[no-]mod-envelope */
	opt_modulation_envelope = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_trace_text(const char *arg)
{
	/* --[no-]trace-text-meta */
	opt_trace_text_meta_event = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_overlap_voice(const char *arg)
{
	/* --[no-]overlap-voice */
	opt_overlap_voice_allow = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_temper_control(const char *arg)
{
	/* --[no-]temper-control */
	opt_temper_control = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_default_mid(char *arg)
{
	/* --default-mid */
	int val = str2mID(arg);
	
	if (! val) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Manufacture ID: Illegal value");
		return 1;
	}
	opt_default_mid = val;
	return 0;
}

static inline int parse_opt_system_mid(char *arg)
{
	/* --system-mid */
	int val = str2mID(arg);
	
	if (! val) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Manufacture ID: Illegal value");
		return 1;
	}
	opt_system_mid = val;
	return 0;
}

static inline int parse_opt_default_bank(const char *arg)
{
	/* --default-bank */
	if (set_value(&default_tonebank, atoi(arg), 0, 0x7f, "Bank number"))
		return 1;
	special_tonebank = -1;
	return 0;
}

static inline int parse_opt_force_bank(const char *arg)
{
	/* --force-bank */
	if (set_value(&special_tonebank, atoi(arg), 0, 0x7f, "Bank number"))
		return 1;
	return 0;
}

static inline int parse_opt_default_program(const char *arg)
{
	/* --default-program */
	int prog, i;
	const char *p;
	
	if (set_value(&prog, atoi(arg), 0, 0x7f, "Program number"))
		return 1;
	if ((p = strchr(arg, '/')) != NULL) {
		if (set_value(&i, atoi(++p), 1, MAX_CHANNELS, "Program channel"))
			return 1;
		default_program[i - 1] = prog;
	} else
		for (i = 0; i < MAX_CHANNELS; i++)
			default_program[i] = prog;
	return 0;
}

static inline int parse_opt_force_program(const char *arg)
{
	/* --force-program */
	const char *p;
	int i;
	
	if (set_value(&def_prog, atoi(arg), 0, 0x7f, "Program number"))
		return 1;
	if (ctl->opened)
		set_default_program(def_prog);
	if ((p = strchr(arg, '/')) != NULL) {
		if (set_value(&i, atoi(++p), 1, MAX_CHANNELS, "Program channel"))
			return 1;
		default_program[i - 1] = SPECIAL_PROGRAM;
	} else
		for (i = 0; i < MAX_CHANNELS; i++)
			default_program[i] = SPECIAL_PROGRAM;
	return 0;
}

static inline int set_default_program(int prog)
{
	int bank;
	Instrument *ip;
	
	bank = (special_tonebank >= 0) ? special_tonebank : default_tonebank;
	if ((ip = play_midi_load_instrument(0, bank, prog)) == NULL)
		return 1;
	default_instrument = ip;
	return 0;
}

static inline int parse_opt_delay(const char *arg)
{
	/* --delay */
	const char *p;
	
	switch (*arg) {
	case '0':
	case 'd':	/* disable */
		effect_lr_mode = -1;
		return 0;
	case 'l':	/* left */
		effect_lr_mode = 0;
		break;
	case 'r':	/* right */
		effect_lr_mode = 1;
		break;
	case 'b':	/* both */
		effect_lr_mode = 2;
		break;
	}
	if ((p = strchr(arg, ',')) != NULL)
		if ((effect_lr_delay_msec = atoi(++p)) < 0) {
			effect_lr_delay_msec = 0;
			effect_lr_mode = -1;
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid delay parameter.");
			return 1;
		}
	return 0;
}

static inline int parse_opt_chorus(const char *arg)
{
	/* --chorus */
	const char *p;
	
	switch (*arg) {
	case '0':
	case 'd':	/* disable */
		opt_chorus_control = 0;
		opt_surround_chorus = 0;
		break;
	case '1':
	case 'n':	/* normal */
	case '2':
	case 's':	/* surround */
		opt_surround_chorus = (*arg == '2' || *arg == 's') ? 1 : 0;
		if ((p = strchr(arg, ',')) != NULL) {
			if (set_value(&opt_chorus_control, atoi(++p), 0, 0x7f,
					"Chorus level"))
				return 1;
			opt_chorus_control = -opt_chorus_control;
		} else
			opt_chorus_control = 1;
		break;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid chorus parameter.");
		return 1;
	}
	return 0;
}

static inline int parse_opt_reverb(const char *arg)
{
	/* --reverb */
	const char *p;
	
	/* option       action                  opt_reverb_control
	 * reverb=0     no reverb                 0
	 * reverb=1     old reverb                1
	 * reverb=1,n   set reverb level to n   (-1 to -127)
	 * reverb=2     "global" old reverb       2
	 * reverb=2,n   set reverb level to n   (-1 to -127) - 128
	 * reverb=3     new reverb                3
	 * reverb=3,n   set reverb level to n   (-1 to -127) - 256
	 * reverb=4     "global" new reverb       4
	 * reverb=4,n   set reverb level to n   (-1 to -127) - 384
	 * 
	 * I think "global" was meant to apply a single global reverb,
	 * without applying any reverb to the channels.  The do_effects()
	 * function in effects.c looks like a good way to do this.
	 * 
	 * This is NOT the "correct" way to implement global reverb, we should
	 * really make a new variable just for that.  But if opt_reverb_control
	 * is already used in a similar fashion, rather than creating a new
	 * variable for setting the channel reverb levels, then I guess
	 * maybe this isn't so bad....  It would be nice to create new
	 * variables for both global reverb and channel reverb level settings
	 * in the future, but this will do for now.
	 */
	
	switch (*arg) {
	case '0':
	case 'd':	/* disable */
		opt_reverb_control = 0;
		break;
	case '1':
	case 'n':	/* normal */
		if ((p = strchr(arg, ',')) != NULL) {
			if (set_value(&opt_reverb_control, atoi(++p), 1, 0x7f,
					"Reverb level"))
				return 1;
			opt_reverb_control = -opt_reverb_control;
		} else
			opt_reverb_control = 1;
		break;
	case '2':
	case 'g':	/* global */
		if ((p = strchr(arg, ',')) != NULL) {
			if (set_value(&opt_reverb_control, atoi(++p), 1, 0x7f,
					"Reverb level"))
				return 1;
			opt_reverb_control = -opt_reverb_control - 128;
		} else
			opt_reverb_control = 2;
		break;
	case '3':
	case 'f':	/* freeverb */
		if ((p = strchr(arg, ',')) != NULL) {
			if (set_value(&opt_reverb_control, atoi(++p), 1, 0x7f,
					"Reverb level"))
				return 1;
			opt_reverb_control = -opt_reverb_control - 256;
		} else
			opt_reverb_control = 3;
		break;
	case '4':
	case 'G':	/* global freeverb */
		if ((p = strchr(arg, ',')) != NULL) {
			if (set_value(&opt_reverb_control, atoi(++p), 1, 0x7f,
					"Reverb level"))
				return 1;
			opt_reverb_control = -opt_reverb_control - 384;
		} else
			opt_reverb_control = 4;
		break;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid reverb parameter.");
		return 1;
	}
	return 0;
}

static inline int parse_opt_voice_lpf(const char *arg)
{
	/* --voice-lpf */
	switch (*arg) {
	case '0':
	case 'd':	/* disable */
		opt_lpf_def = 0;
		break;
	case '1':
	case 'c':	/* chamberlin */
		opt_lpf_def = 1;
		break;
	case '2':
	case 'm':	/* moog */
		opt_lpf_def = 2;
		break;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid voice LPF type %s", arg);
		return 1;
	}
	return 0;
}

/* Noise Shaping filter from
 * Kunihiko IMAI <imai@leo.ec.t.kanazawa-u.ac.jp>
 */
static inline int parse_opt_noise_shaping(const char *arg)
{
	/* --noise-shaping */
	if (set_value(&noise_sharp_type, atoi(arg), 0, 4, "Noise shaping type"))
		return 1;
	return 0;
}

static inline int parse_opt_resample(const char *arg)
{
	/* --resample */
	switch (*arg) {
	case '0':
	case 'd':	/* disable */
		set_current_resampler(RESAMPLE_NONE);
		break;
	case '1':
	case 'l':	/* linear */
		set_current_resampler(RESAMPLE_LINEAR);
		break;
	case '2':
	case 'c':	/* cspline */
		set_current_resampler(RESAMPLE_CSPLINE);
		break;
	case '3':
	case 'L':	/* lagrange */
		set_current_resampler(RESAMPLE_LAGRANGE);
		break;
	case '4':
	case 'n':	/* newton */
		set_current_resampler(RESAMPLE_NEWTON);
		break;
	case '5':
	case 'g':	/* guass */
		set_current_resampler(RESAMPLE_GAUSS);
		break;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid resample type %s", arg);
		return 1;
	}
	return 0;
}

static inline int parse_opt_e(const char *arg)
{
	/* evil */
#ifdef __W32__
	opt_evil_mode = 1;
	return 0;
#else
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-e option is not supported");
	return 1;
#endif /* __W32__ */
}

static inline int parse_opt_F(const char *arg)
{
	adjust_panning_immediately = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_f(const char *arg)
{
	fast_decay = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_g(const char *arg)
{
#ifdef SUPPORT_SOUNDSPEC
	spectrogram_update_sec = atof(arg);
	if (spectrogram_update_sec <= 0) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"Invalid -g argument: `%s'", arg);
		return 1;
	}
	view_soundspec_flag = 1;
	return 0;
#else
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-g option is not supported");
	return 1;
#endif	/* SUPPORT_SOUNDSPEC */
}

static inline int parse_opt_H(const char *arg)
{
	/* force keysig (number of sharp/flat) */
	int keysig;
	
	if (set_value(&keysig, atoi(arg), -7, 7,
			"Force keysig (number of sHarp(+)/flat(-))"))
		return 1;
	opt_force_keysig = keysig;
	return 0;
}

__attribute__((noreturn))
static inline int parse_opt_h(const char *arg)
{
	static char *help_list[] = {
"TiMidity++ %s (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>",
"The original version (C) 1995 Tuukka Toivonen <tt@cgs.fi>",
"TiMidity is free software and comes with ABSOLUTELY NO WARRANTY.",
"",
#ifdef __W32__
"Win32 version by Davide Moretti <dave@rimini.com>",
"              and Daisuke Aoki <dai@y7.net>",
"",
#endif /* __W32__ */
"Usage:",
"  %s [options] filename [...]",
"",
#ifndef __W32__		/*does not work in Windows */
"  Use \"-\" as filename to read a MIDI file from stdin",
"",
#endif
"Options:",
"  -A n,m     --volume=n, --drum-power=m",
"               Amplify volume by n percent (may cause clipping),",
"                 and amplify drum power by m percent",
"     (a)     --[no-]volume-compensation",
"               Toggle amplify compensation (disabled by default)",
"  -a         --[no-]anti-alias",
"               Enable the anti-aliasing filter",
"  -B n,m     --buffer-fragments=n,m",
"               Set number of buffer fragments(n), and buffer size(2^m)",
"  -C n       --control-ratio=n",
"               Set ratio of sampling and control frequencies",
"  -c file    --config-file=file",
"               Read extra configuration file",
"  -D n       --drum-channel=n",
"               Play drums on channel n",
#ifdef IA_DYNAMIC
"  -d path    --interface-path=path",
"               Set dynamic interface module directory",
#endif /* IA_DYNAMIC */
"  -E mode    --ext=mode",
"               TiMidity sequencer extensional modes:",
"                 mode = w/W : Enable/Disable Modulation wheel",
"                        p/P : Enable/Disable Portamento",
"                        v/V : Enable/Disable NRPN Vibrato",
"                        s/S : Enable/Disable Channel pressure",
"                        e/E : Enable/Disable Modulation Envelope",
"                        t/T : Enable/Disable Trace Text Meta Event at playing",
"                        o/O : Enable/Disable Overlapped voice",
"                        z/Z : Enable/Disable Temperament control",
"                        m<HH>: Define default Manufacture ID <HH> in two hex",
"                        M<HH>: Define system Manufacture ID <HH> in two hex",
"                        b<n>: Use tone bank <n> as the default",
"                        B<n>: Always use tone bank <n>",
"                        i<n/m>: Use program <n> on channel <m> as the default",
"                        I<n/m>: Always use program <n> on channel <m>",
"                        F<args>: For effect.  See below for effect options",
"                   default: -E "
#ifdef MODULATION_WHEEL_ALLOW
"w"
#else
"W"
#endif /* MODULATION_WHEEL_ALLOW */
#ifdef PORTAMENTO_ALLOW
"p"
#else
"P"
#endif /* PORTAMENTO_ALLOW */
#ifdef NRPN_VIBRATO_ALLOW
"v"
#else
"V"
#endif /* NRPN_VIBRATO_ALLOW */
#ifdef GM_CHANNEL_PRESSURE_ALLOW
"s"
#else
"S"
#endif /* GM_CHANNEL_PRESSURE_ALLOW */
#ifdef MODULATION_ENVELOPE_ALLOW
"e"
#else
"E"
#endif /* MODULATION_ENVELOPE_ALLOW */
#ifdef ALWAYS_TRACE_TEXT_META_EVENT
"t"
#else
"T"
#endif /* ALWAYS_TRACE_TEXT_META_EVENT */
#ifdef OVERLAP_VOICE_ALLOW
"o"
#else
"O"
#endif /* OVERLAP_VOICE_ALLOW */
#ifdef TEMPER_CONTROL_ALLOW
"z"
#else
"Z"
#endif /* TEMPER_CONTROL_ALLOW */
,
#ifdef __W32__
"  -e         --evil",
"               Increase thread priority (evil) - be careful!",
#endif
"  -F         --[no-]fast-panning",
"               Disable/Enable fast panning (toggle on/off, default is on)",
"  -f         --[no-]fast-decay",
"               "
#ifdef FAST_DECAY
"Disable "
#else
"Enable "
#endif
"fast decay mode (toggle)",
#ifdef SUPPORT_SOUNDSPEC
"  -g sec     --spectrogram=sec",
"               Open Sound-Spectrogram Window",
#endif /* SUPPORT_SOUNDSPEC */
"  -H n       --force-keysig=n",
"               Force keysig number of sHarp(+)/flat(-) (-7..7)",
"  -h         --help",
"               Display this help message",
"  -i mode    --interface=mode",
"               Select user interface (see below for list)",
#ifdef IA_ALSASEQ
"             --realtime-priority=n (for alsaseq only)",
"               Set the realtime priority (0-100)",
"             --sequencer-ports=n (for alsaseq only)",
"               Set the number of opened sequencer ports (default is 4)",
#endif
"  -j         --[no-]realtime-load",
"               Realtime load instrument (toggle on/off)",
"  -K n       --adjust-key=n",
"               Adjust key by n half tone (-24..24)",
"  -k msec    --voice-queue=msec",
"               Specify audio queue time limit to reduce voice",
"  -L path    --patch-path=path",
"               Append dir to search path",
"  -M name    --pcm-file=name",
"               Specify PCM filename (*.wav or *.aiff) to be played or:",
"                 \"auto\" : Play *.mid.wav or *.mid.aiff",
"                 \"none\" : Disable this feature (default)",
"  -m msec    --decay-time=msec",
"               Minimum time for a full volume sustained note to decay,",
"                 0 disables",
"  -N n       --interpolation=n",
"               Set the interpolation parameter (depends on -EFresamp option)",
"                 Linear interpolation is used if audio queue < 99%%",
"                 cspline, lagrange:",
"                   Toggle 4-point interpolation (default on)",
"                 newton:",
"                   n'th order Newton polynomial interpolation, n=1-57 odd",
"                 gauss:",
"                   n+1 point Gauss-like interpolation, n=1-34 (default 25)",
"  -O mode    --output-mode=mode",
"               Select output mode and format (see below for list)",
#ifdef AU_FLAC
"             --flac-verify (for Ogg FLAC only)",
"               Verify a correct encoding",
"             --flac-padding=n (for Ogg FLAC only)",
"               Write a PADDING block of length n",
"             --flac-complevel=n (for Ogg FLAC only)",
"               Set compression level n:[0..8]",
#ifdef AU_OGGFLAC
"             --oggflac (for Ogg FLAC only)",
"               Output OggFLAC stream (experimental)",
#endif /* AU_OGGFLAC */
#endif /* AU_FLAC */
#ifdef AU_SPEEX
"             --speex-quality=n (for Ogg Speex only)",
"               Encoding quality n:[0..10]",
"             --speex-vbr (for Ogg Speex only)",
"               Enable variable bit-rate (VBR)",
"             --speex-abr=n (for Ogg Speex only)",
"               Enable average bit-rate (ABR) at rate bps",
"             --speex-vad (for Ogg Speex only)",
"               Enable voice activity detection (VAD)",
"             --speex-dtx (for Ogg Speex only)",
"               Enable file-based discontinuous transmission (DTX)",
"             --speex-complexity=n (for Ogg Speex only)",
"               Set encoding complexity n:[0-10]",
"             --speex-nframes=n (for Ogg Speex only)",
"               Number of frames per Ogg packet n:[0-10]",
#endif
"  -o file    --output-file=file",
"               Output to another file (or device/server) (Use \"-\" for stdout)",
"  -P file    --patch-file=file",
"               Use patch file for all programs",
"  -p n       --polyphony=n",
"               Allow n-voice polyphony.  Optional auto polyphony reduction",
"     (a)     --[no-]polyphony-reduction",
"               Toggle automatic polyphony reduction.  Enabled by default",
"  -Q n[,...] --mute=n[,...]",
"               Ignore channel n (0: ignore all, -n: resume channel n)",
"     (t)     --temper-mute=n[,...]",
"               Quiet temperament type n (0..3: preset, 4..7: user-defined)",
"  -q sec/n   --audio-buffer=sec/n",
"               Specify audio buffer in seconds",
"                 sec: Maxmum buffer, n: Filled to start (default is 5.0/100%%)",
"                 (size of 100%% equals device buffer size)",
"  -R msec      Pseudo reveb effect (set every instrument's release to msec)",
"                 if n=0, n is set to 800",
"  -S n       --cache-size=n",
"               Cache size (0 means no cache)",
"  -s freq    --sampling-freq=freq",
"               Set sampling frequency to freq (Hz or kHz)",
"  -T n       --adjust-tempo=n",
"               Adjust tempo to n%%,",
"                 120=play MOD files with an NTSC Amiga's timing",
"  -t code    --output-charset=code",
"               Output text language code:",
"                 code=auto  : Auto conversion by `LANG' environment variable",
"                              (UNIX only)",
"                      ascii : Convert unreadable characters to '.' (0x2e)",
"                      nocnv : No conversion",
"                      1251  : Convert from windows-1251 to koi8-r",
#ifdef JAPANESE
"                      euc   : EUC-japan",
"                      jis   : JIS",
"                      sjis  : shift JIS",
#endif /* JAPANESE */
"  -U         --[no-]unload-instruments",
"               Unload instruments from memory between MIDI files",
"  -V power   --volume-curve=power",
"               Define the velocity/volume/expression curve",
"                 amp = vol^power (auto: 0, linear: 1, ideal: ~1.661, GS: ~2)",
"  -v         --version",
"               Display TiMidity version information",
"  -W mode    --wrd=mode",
"               Select WRD interface (see below for list)",
#ifdef __W32__
"  -w mode    --rcpcv-dll=mode",
"               Windows extensional modes:",
"                 mode=r/R : Enable/Disable rcpcv.dll",
#endif /* __W32__ */
"  -x str     --config-string=str",
"               Read configuration str from command line argument",
"  -Z file    --freq-table=file",
"               Load frequency table (Use \"pure\" for pure intonation)",
"  pure<n>(m) --pure-intonation=n(m)",
"               Initial keysig number <n> of sharp(+)/flat(-) (-7..7)",
"                 'm' stands for minor mode",
"  --module=n",
"               Simulate behavior of specific synthesizer module by n",
"                 n=0       : TiMidity++ Default (default)",
"                   1-15    : GS family",
"                   16-31   : XG family",
"                   32-111  : SoundBlaster and other systhesizer modules",
"                   112-127 : TiMidity++ specification purposes",
		NULL
	};
	void show_ao_device_info(FILE *fp);
	FILE *fp;
	char version[32], *help_args[3];
	int i, j;
	char *h;
	ControlMode *cmp, **cmpp;
	char mark[128];
	PlayMode *pmp, **pmpp;
	WRDTracer *wlp, **wlpp;
	
	fp = open_pager();
	strcpy(version, (strcmp(timidity_version, "current")) ? "version " : "");
	strcat(version, timidity_version);
	help_args[0] = version;
	help_args[1] = program_name;
	help_args[2] = NULL;
	for (i = 0, j = 0; (h = help_list[i]) != NULL; i++) {
		if (strchr(h, '%')) {
			if (*(strchr(h, '%') + 1) != '%')
				fprintf(fp, h, help_args[j++]);
			else
				fprintf(fp, "%s", h);
		} else
			fputs(h, fp);
		fputs(NLS, fp);
	}
	fputs(NLS, fp);
	fputs("Effect options (-EF, --ext=F option):" NLS
"  -EFdelay=d   Disable delay effect (default)" NLS
"  -EFdelay=l   Enable Left delay" NLS
"    [,msec]      `msec' is optional to specify left-right delay time" NLS
"  -EFdelay=r   Enable Right delay" NLS
"    [,msec]      `msec' is optional to specify left-right delay time" NLS
"  -EFdelay=b   Enable rotate Both left and right" NLS
"    [,msec]      `msec' is optional to specify left-right delay time" NLS
"  -EFchorus=d  Disable MIDI chorus effect control" NLS
"  -EFchorus=n  Enable Normal MIDI chorus effect control" NLS
"    [,level]     `level' is optional to specify chorus level [0..127]" NLS
"                 (default)" NLS
"  -EFchorus=s  Surround sound, chorus detuned to a lesser degree" NLS
"    [,level]     `level' is optional to specify chorus level [0..127]" NLS
"  -EFreverb=d  Disable MIDI reverb effect control" NLS
"  -EFreverb=n  Enable Normal MIDI reverb effect control" NLS
"    [,level]     `level' is optional to specify reverb level [1..127]" NLS
"  -EFreverb=g  Global reverb effect" NLS
"    [,level]     `level' is optional to specify reverb level [1..127]" NLS
"  -EFreverb=f  Enable Freeverb MIDI reverb effect control (default)" NLS
"    [,level]     `level' is optional to specify reverb level [1..127]" NLS
"  -EFreverb=G  Global Freeverb effect" NLS
"    [,level]     `level' is optional to specify reverb level [1..127]" NLS
"  -EFvlpf=d    Disable voice LPF" NLS
"  -EFvlpf=c    Enable Chamberlin resonant LPF (12dB/oct) (default)" NLS
"  -EFvlpf=m    Enable Moog resonant lowpass VCF (24dB/oct)" NLS
"  -EFns=n      Enable the n th degree (type) noise shaping filter" NLS
"                 n:[0..4] (for 8-bit linear encoding, default is 4)" NLS
"                 n:[0..4] (for 16-bit linear encoding, default is 4)" NLS, fp);
#ifndef FIXED_RESAMPLATION
#ifdef HAVE_STRINGIZE
#define tim_str_internal(x) #x
#define tim_str(x) tim_str_internal(x)
#else
#define tim_str(x) "x"
#endif
	fputs("  -EFresamp=d  Disable resamplation", fp);
	if (! strcmp(tim_str(DEFAULT_RESAMPLATION), "resample_none"))
		fputs(" (default)", fp);
	fputs(NLS, fp);
	fputs("  -EFresamp=l  Enable Linear resample algorithm", fp);
	if (! strcmp(tim_str(DEFAULT_RESAMPLATION), "resample_linear"))
		fputs(" (default)", fp);
	fputs(NLS, fp);
	fputs("  -EFresamp=c  Enable C-spline resample algorithm", fp);
	if (! strcmp(tim_str(DEFAULT_RESAMPLATION), "resample_cspline"))
		fputs(" (default)", fp);
	fputs(NLS, fp);
	fputs("  -EFresamp=L  Enable Lagrange resample algorithm", fp);
	if (! strcmp(tim_str(DEFAULT_RESAMPLATION), "resample_lagrange"))
		fputs(" (default)", fp);
	fputs(NLS, fp);
	fputs("  -EFresamp=n  Enable Newton resample algorithm", fp);
	if (! strcmp(tim_str(DEFAULT_RESAMPLATION), "resample_newton"))
		fputs(" (default)", fp);
	fputs(NLS, fp);
	fputs("  -EFresamp=g  Enable Gauss-like resample algorithm", fp);
	if (! strcmp(tim_str(DEFAULT_RESAMPLATION), "resample_gauss"))
		fputs(" (default)", fp);
	fputs(NLS
"                 -EFresamp affects the behavior of -N option" NLS, fp);
#endif
	fputs(NLS, fp);
	fputs("Alternative TiMidity sequencer extensional mode long options:" NLS
"  --[no-]mod-wheel" NLS
"  --[no-]portamento" NLS
"  --[no-]vibrato" NLS
"  --[no-]ch-pressure" NLS
"  --[no-]mod-envelope" NLS
"  --[no-]trace-text-meta" NLS
"  --[no-]overlap-voice" NLS
"  --[no-]temper-control" NLS
"  --default-mid=<HH>" NLS
"  --system-mid=<HH>" NLS
"  --default-bank=n" NLS
"  --force-bank=n" NLS
"  --default-program=n/m" NLS
"  --force-program=n/m" NLS
"  --delay=(d|l|r|b)[,msec]" NLS
"  --chorus=(d|n|s)[,level]" NLS
"  --reverb=(d|n|g|f|G)[,level]" NLS
"  --voice-lpf=(d|c|m)" NLS
"  --noise-shaping=n" NLS, fp);
#ifndef FIXED_RESAMPLATION
	fputs("  --resample=(d|l|c|L|n|g)" NLS, fp);
#endif
	fputs(NLS, fp);
	fputs("Available interfaces (-i, --interface option):" NLS, fp);
	for (cmpp = ctl_list; (cmp = *cmpp) != NULL; cmpp++)
#ifdef IA_DYNAMIC
		if (cmp->id_character != dynamic_interface_id)
			fprintf(fp, "  -i%c          %s" NLS,
					cmp->id_character, cmp->id_name);
#else
		fprintf(fp, "  -i%c          %s" NLS,
				cmp->id_character, cmp->id_name);
#endif	/* IA_DYNAMIC */
#ifdef IA_DYNAMIC
	fprintf(fp, "Supported dynamic load interfaces (%s):" NLS,
			dynamic_lib_root);
	memset(mark, 0, sizeof(mark));
	for (cmpp = ctl_list; (cmp = *cmpp) != NULL; cmpp++)
		mark[(int) cmp->id_character] = 1;
	if (dynamic_interface_id != 0)
		mark[(int) dynamic_interface_id] = 0;
	list_dyna_interface(fp, dynamic_lib_root, mark);
#endif	/* IA_DYNAMIC */
	fputs(NLS, fp);
	fputs("Interface options (append to -i? option):" NLS
"  `v'          more verbose (cumulative)" NLS
"  `q'          quieter (cumulative)" NLS
"  `t'          trace playing" NLS
"  `l'          loop playing (some interface ignore this option)" NLS
"  `r'          randomize file list arguments before playing" NLS
"  `s'          sorting file list arguments before playing" NLS, fp);
#ifdef IA_ALSASEQ
	fputs("  `D'          daemonize TiMidity++ in background "
			"(for alsaseq only)" NLS, fp);
#endif
	fputs(NLS, fp);
	fputs("Alternative interface long options:" NLS
"  --verbose=n" NLS
"  --quiet=n" NLS
"  --[no-]trace" NLS
"  --[no-]loop" NLS
"  --[no-]random" NLS
"  --[no-]sort" NLS, fp);
#ifdef IA_ALSASEQ
	fputs("  --[no-]background" NLS, fp);
#endif
	fputs(NLS, fp);
	fputs("Available output modes (-O, --output-mode option):" NLS, fp);
	for (pmpp = play_mode_list; (pmp = *pmpp) != NULL; pmpp++)
		fprintf(fp, "  -O%c          %s" NLS,
				pmp->id_character, pmp->id_name);
#ifdef AU_AO
	show_ao_device_info(fp);
#endif /* AU_AO */
	fputs(NLS, fp);
	fputs("Output format options (append to -O? option):" NLS
"  `S'          stereo" NLS
"  `M'          monophonic" NLS
"  `s'          signed output" NLS
"  `u'          unsigned output" NLS
"  `1'          16-bit sample width" NLS
"  `2'          24-bit sample width" NLS
"  `8'          8-bit sample width" NLS
"  `l'          linear encoding" NLS
"  `U'          U-Law encoding" NLS
"  `A'          A-Law encoding" NLS
"  `x'          byte-swapped output" NLS, fp);
	fputs(NLS, fp);
	fputs("Alternative output format long options:" NLS
"  --output-stereo" NLS
"  --output-mono" NLS
"  --output-signed" NLS
"  --output-unsigned" NLS
"  --output-16bit" NLS
"  --output-24bit" NLS
"  --output-8bit" NLS
"  --output-linear" NLS
"  --output-ulaw" NLS
"  --output-alaw" NLS
"  --[no-]output-swab" NLS, fp);
	fputs(NLS, fp);
	fputs("Available WRD interfaces (-W, --wrd option):" NLS, fp);
	for (wlpp = wrdt_list; (wlp = *wlpp) != NULL; wlpp++)
		fprintf(fp, "  -W%c          %s" NLS, wlp->id, wlp->name);
	fputs(NLS, fp);
	close_pager(fp);
	exit(EXIT_SUCCESS);
}

#ifdef IA_DYNAMIC
static inline void list_dyna_interface(FILE *fp, char *path, char *mark)
{
	URL url;
	char fname[BUFSIZ], *info;
	int id;
	
	if ((url = url_dir_open(path)) == NULL)
		return;
	while (url_gets(url, fname, sizeof(fname)) != NULL)
		if (strncmp(fname, "interface_", 10) == 0) {
			id = fname[10];
			if (mark[id])
				continue;
			mark[id] = 1;
			if ((info = dynamic_interface_info(id)) == NULL)
				info = dynamic_interface_module(id);
			if (info != NULL)
				fprintf(fp, "  -i%c          %s" NLS, id, info);
		}
	url_close(url);
}

static inline char *dynamic_interface_info(int id)
{
	static char libinfo[MAXPATHLEN];
	int fd, n;
	char *nl;
	
	sprintf(libinfo, "%s" PATH_STRING "interface_%c.txt",
			dynamic_lib_root, id);
	if ((fd = open(libinfo, 0)) < 0)
		return NULL;
	n = read(fd, libinfo, sizeof(libinfo) - 1);
	close(fd);
	if (n <= 0)
		return NULL;
	libinfo[n] = '\0';
	if ((nl = strchr(libinfo, '\n')) == libinfo)
		return NULL;
	if (nl != NULL) {
		*nl = '\0';
		if (*(nl - 1) == '\r')
			*(nl - 1) = '\0';
	}
	return libinfo;
}

char *dynamic_interface_module(int id)
{
	static char shared_library[MAXPATHLEN];
	int fd;
	
	sprintf(shared_library, "%s" PATH_STRING "interface_%c%s",
			dynamic_lib_root, id, SHARED_LIB_EXT);
	if ((fd = open(shared_library, 0)) < 0)
		return NULL;
	close(fd);
	return shared_library;
}
#endif	/* IA_DYNAMIC */

static inline int parse_opt_i(const char *arg)
{
	/* interface mode */
	ControlMode *cmp, **cmpp;
	int found = 0;
	
	for (cmpp = ctl_list; (cmp = *cmpp) != NULL; cmpp++) {
		if (cmp->id_character == *arg) {
			found = 1;
			ctl = cmp;
#if defined(IA_W32GUI) || defined(IA_W32G_SYN)
			cmp->verbosity = 1;
			cmp->trace_playing = 0;
			cmp->flags = 0;
#endif	/* IA_W32GUI */
			break;
		}
#ifdef IA_DYNAMIC
		if (cmp->id_character == dynamic_interface_id
				&& dynamic_interface_module(*arg)) {
			/* Dynamic interface loader */
			found = 1;
			ctl = cmp;
			if (dynamic_interface_id != *arg) {
				cmp->id_character = dynamic_interface_id = *arg;
				cmp->verbosity = 1;
				cmp->trace_playing = 0;
				cmp->flags = 0;
			}
			break;
		}
#endif	/* IA_DYNAMIC */
	}
	if (! found) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"Interface `%c' is not compiled in.", *arg);
		return 1;
	}
	while (*(++arg))
		switch (*arg) {
		case 'v':
			cmp->verbosity++;
			break;
		case 'q':
			cmp->verbosity--;
			break;
		case 't':	/* toggle */
			cmp->trace_playing = (cmp->trace_playing) ? 0 : 1;
			break;
		case 'l':
			cmp->flags ^= CTLF_LIST_LOOP;
			break;
		case 'r':
			cmp->flags ^= CTLF_LIST_RANDOM;
			break;
		case 's':
			cmp->flags ^= CTLF_LIST_SORT;
			break;
		case 'a':
			cmp->flags ^= CTLF_AUTOSTART;
			break;
		case 'x':
			cmp->flags ^= CTLF_AUTOEXIT;
			break;
		case 'd':
			cmp->flags ^= CTLF_DRAG_START;
			break;
		case 'u':
			cmp->flags ^= CTLF_AUTOUNIQ;
			break;
		case 'R':
			cmp->flags ^= CTLF_AUTOREFINE;
			break;
		case 'C':
			cmp->flags ^= CTLF_NOT_CONTINUE;
			break;
#ifdef IA_ALSASEQ
		case 'D':
			cmp->flags ^= CTLF_DAEMONIZE;
			break;
#endif
		default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Unknown interface option `%c'", *arg);
			return 1;
		}
	return 0;
}

static inline int parse_opt_verbose(const char *arg)
{
	/* --verbose */
	ctl->verbosity += (arg) ? atoi(arg) : 1;
	return 0;
}

static inline int parse_opt_quiet(const char *arg)
{
	/* --quiet */
	ctl->verbosity -= (arg) ? atoi(arg) : 1;
	return 0;
}

static inline int parse_opt_trace(const char *arg)
{
	/* --[no-]trace */
	ctl->trace_playing = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_loop(const char *arg)
{
	/* --[no-]loop */
	return set_flag(&(ctl->flags), CTLF_LIST_LOOP, arg);
}

static inline int parse_opt_random(const char *arg)
{
	/* --[no-]random */
	return set_flag(&(ctl->flags), CTLF_LIST_RANDOM, arg);
}

static inline int parse_opt_sort(const char *arg)
{
	/* --[no-]sort */
	return set_flag(&(ctl->flags), CTLF_LIST_SORT, arg);
}

#ifdef IA_ALSASEQ
static inline int parse_opt_background(const char *arg)
{
	/* --[no-]background */
	return set_flag(&(ctl->flags), CTLF_DAEMONIZE, arg);
}

static inline int parse_opt_rt_prio(const char *arg)
{
	/* --realtime-priority */
	if (set_value(&opt_realtime_priority, atoi(arg), 0, 100,
			"Realtime priority"))
		return 1;
	return 0;
}

static inline int parse_opt_seq_ports(const char *arg)
{
	/* --sequencer-ports */
	if (set_value(&opt_sequencer_ports, atoi(arg), 1, 16,
			"Number of sequencer ports"))
		return 1;
	return 0;
}
#endif

static inline int parse_opt_j(const char *arg)
{
	opt_realtime_playing = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_K(const char *arg)
{
	/* key adjust */
	if (set_value(&key_adjust, atoi(arg), -24, 24, "Key adjust"))
		return 1;
	return 0;
}

static inline int parse_opt_k(const char *arg)
{
	reduce_voice_threshold = atoi(arg);
	return 0;
}

static inline int parse_opt_L(char *arg)
{
	add_to_pathlist(arg);
	try_config_again = 1;
	return 0;
}

static inline int parse_opt_M(const char *arg)
{
	if (pcm_alternate_file)
		free(pcm_alternate_file);
	pcm_alternate_file = safe_strdup(arg);
	return 0;
}

static inline int parse_opt_m(const char *arg)
{
	min_sustain_time = atoi(arg);
	if (min_sustain_time < 0)
		min_sustain_time = 0;
	return 0;
}

static inline int parse_opt_N(const char *arg)
{
	int val;
	
	switch (get_current_resampler()) {
	case RESAMPLE_CSPLINE:
	case RESAMPLE_LAGRANGE:
		no_4point_interpolation = y_or_n_p(arg);
		break;
	case RESAMPLE_NEWTON:
	case RESAMPLE_GAUSS:
		if (! (val = atoi(arg)))
			/* set to linear interpolation for compatibility */
			set_current_resampler(RESAMPLE_LINEAR);
		else if (set_resampler_parm(val)) {
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid -N value");
			return 1;
		}
		break;
	}
	return 0;
}

static inline int parse_opt_O(const char *arg)
{
	/* output mode */
	PlayMode *pmp, **pmpp;
	int found = 0;
	
	for (pmpp = play_mode_list; (pmp = *pmpp) != NULL; pmpp++)
		if (pmp->id_character == *arg) {
			found = 1;
			play_mode = pmp;
			break;
		}
	if (! found) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"Playmode `%c' is not compiled in.", *arg);
		return 1;
	}
	while (*(++arg))
		switch (*arg) {
		case 'S':	/* stereo */
			pmp->encoding &= ~PE_MONO;
			break;
		case 'M':
			pmp->encoding |= PE_MONO;
			break;
		case 's':
			pmp->encoding |= PE_SIGNED;
			pmp->encoding &= ~(PE_ULAW | PE_ALAW);
			break;
		case 'u':
			pmp->encoding &= ~PE_SIGNED;
			pmp->encoding &= ~(PE_ULAW | PE_ALAW);
			break;
		case '1':	/* 1 for 16-bit */
			pmp->encoding |= PE_16BIT;
			pmp->encoding &= ~(PE_24BIT | PE_ULAW | PE_ALAW);
			break;
		case '2':	/* 2 for 24-bit */
			pmp->encoding |= PE_24BIT;
			pmp->encoding &= ~(PE_16BIT | PE_ULAW | PE_ALAW);
			break;
		case '8':
			pmp->encoding &= ~(PE_16BIT | PE_24BIT);
			break;
		case 'l':	/* linear */
			pmp->encoding &= ~(PE_ULAW | PE_ALAW);
			break;
		case 'U':	/* uLaw */
			pmp->encoding |= PE_ULAW;
			pmp->encoding &= ~(PE_SIGNED
					| PE_16BIT | PE_24BIT | PE_ALAW | PE_BYTESWAP);
			break;
		case 'A':	/* aLaw */
			pmp->encoding |= PE_ALAW;
			pmp->encoding &= ~(PE_SIGNED
					| PE_16BIT | PE_24BIT | PE_ULAW | PE_BYTESWAP);
			break;
		case 'x':
			pmp->encoding ^= PE_BYTESWAP;	/* toggle */
			pmp->encoding &= ~(PE_ULAW | PE_ALAW);
			break;
		default:
			ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
					"Unknown format modifier `%c'", *arg);
			return 1;
		}
	return 0;
}

static inline int parse_opt_output_stereo(const char *arg)
{
	/* --output-stereo, --output-mono */
	if (y_or_n_p(arg))
		/* I first thought --mono should be the syntax sugar to
		 * --stereo=no, but the source said stereo should be !PE_MONO,
		 * not mono should be !PE_STEREO.  Perhaps I took a wrong
		 * choice? -- mput
		 */
		play_mode->encoding &= ~PE_MONO;
	else
		play_mode->encoding |= PE_MONO;
	return 0;
}

static inline int parse_opt_output_signed(const char *arg)
{
	/* --output-singed, --output-unsigned */
	if (set_flag(&(play_mode->encoding), PE_SIGNED, arg))
		return 1;
	play_mode->encoding &= ~(PE_ULAW | PE_ALAW);
	return 0;
}

static inline int parse_opt_output_bitwidth(const char *arg)
{
	/* --output-16bit, --output-24bit, --output-8bit */
	switch (*arg) {
	case '1':	/* 16bit */
		play_mode->encoding |= PE_16BIT;
		play_mode->encoding &= ~(PE_24BIT | PE_ULAW | PE_ALAW);
		return 0;
	case '2':	/* 24bit */
		play_mode->encoding |= PE_24BIT;
		play_mode->encoding &= ~(PE_16BIT | PE_ULAW | PE_ALAW);
		return 0;
	case '8':	/* 8bit */
		play_mode->encoding &= ~(PE_16BIT | PE_24BIT);
		return 0;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid output bitwidth %s", arg);
		return 1;
	}
}

static inline int parse_opt_output_format(const char *arg)
{
	/* --output-linear, --output-ulaw, --output-alaw */
	switch (*arg) {
	case 'l':	/* linear */
		play_mode->encoding &= ~(PE_ULAW | PE_ALAW);
		return 0;
	case 'u':	/* uLaw */
		play_mode->encoding |= PE_ULAW;
		play_mode->encoding &=
				~(PE_SIGNED | PE_16BIT | PE_24BIT | PE_ALAW | PE_BYTESWAP);
		return 0;
	case 'a':	/* aLaw */
		play_mode->encoding |= PE_ALAW;
		play_mode->encoding &=
				~(PE_SIGNED | PE_16BIT | PE_24BIT | PE_ULAW | PE_BYTESWAP);
		return 0;
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Invalid output format %s", arg);
		return 1;
	}
}

static inline int parse_opt_output_swab(const char *arg)
{
	/* --[no-]output-swab */
	if (set_flag(&(play_mode->encoding), PE_BYTESWAP, arg))
		return 1;
	play_mode->encoding &= ~(PE_ULAW | PE_ALAW);
	return 0;
}

#ifdef AU_FLAC
extern void flac_set_option_verify(int);
extern void flac_set_option_padding(int);
extern void flac_set_compression_level(int);

static inline int parse_opt_flac_verify(const char *arg)
{
	flac_set_option_verify(1);
	return 0;
}

static inline int parse_opt_flac_padding(const char *arg)
{
	flac_set_option_padding(atoi(arg));
	return 0;
}

static inline int parse_opt_flac_complevel(const char *arg)
{
	flac_set_compression_level(atoi(arg));
	return 0;
}

#ifdef AU_OGGFLAC
extern void flac_set_option_oggflac(int);

static inline int parse_opt_flac_oggflac(const char *arg)
{
	flac_set_option_oggflac(1);
	return 0;
}
#endif /* AU_OGGFLAC */
#endif /* AU_FLAC */

#ifdef AU_SPEEX
extern void speex_set_option_quality(int);
extern void speex_set_option_vbr(int);
extern void speex_set_option_abr(int);
extern void speex_set_option_vad(int);
extern void speex_set_option_dtx(int);
extern void speex_set_option_complexity(int);
extern void speex_set_option_nframes(int);

static inline int parse_opt_speex_quality(const char *arg)
{
	speex_set_option_quality(atoi(arg));
	return 0;
}

static inline int parse_opt_speex_vbr(const char *arg)
{
	speex_set_option_vbr(1);
	return 0;
}

static inline int parse_opt_speex_abr(const char *arg)
{
	speex_set_option_abr(atoi(arg));
	return 0;
}

static inline int parse_opt_speex_vad(const char *arg)
{
	speex_set_option_vad(1);
	return 0;
}

static inline int parse_opt_speex_dtx(const char *arg)
{
	speex_set_option_dtx(1);
	return 0;
}

static inline int parse_opt_speex_complexity(const char *arg)
{
	speex_set_option_complexity(atoi(arg));
	return 0;
}

static inline int parse_opt_speex_nframes(const char *arg)
{
	speex_set_option_nframes(atoi(arg));
	return 0;
}
#endif /* AU_SPEEX */

static inline int parse_opt_o(char *arg)
{
	if (opt_output_name)
		free(opt_output_name);
	opt_output_name = safe_strdup(url_expand_home_dir(arg));
	return 0;
}

static inline int parse_opt_P(const char *arg)
{
	/* set overriding instrument */
	strncpy(def_instr_name, arg, sizeof(def_instr_name) - 1);
	def_instr_name[sizeof(def_instr_name) - 1] = '\0';
	return 0;
}

static inline int parse_opt_p(const char *arg)
{
	if (set_value(&voices, atoi(arg), 1,
			MAX_SAFE_MALLOC_SIZE / sizeof(Voice), "Polyphony"))
		return 1;
	max_voices = voices;
	return 0;
}

static inline int parse_opt_p1(const char *arg)
{
	/* --[no-]polyphony-reduction */
	auto_reduce_polyphony = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_Q(const char *arg)
{
	const char *p = arg;
	
	if (strchr(arg, 't'))
		/* backward compatibility */
		return parse_opt_Q1(arg);
	if (set_channel_flag(&quietchannels, atoi(arg), "Quiet channel"))
		return 1;
	while ((p = strchr(p, ',')) != NULL)
		if (set_channel_flag(&quietchannels, atoi(++p), "Quiet channel"))
			return 1;
	return 0;
}

static inline int parse_opt_Q1(const char *arg)
{
	/* --temper-mute */
	int prog;
	const char *p = arg;
	
	if (set_value(&prog, atoi(arg), 0, 7, "Temperament program number"))
		return 1;
	temper_type_mute |= 1 << prog;
	while ((p = strchr(p, ',')) != NULL) {
		if (set_value(&prog, atoi(++p), 0, 7, "Temperament program number"))
			return 1;
		temper_type_mute |= 1 << prog;
	}
	return 0;
}

static inline int parse_opt_q(const char *arg)
{
	char *max_buff = safe_strdup(arg);
	char *fill_buff = strchr(max_buff, '/');
	
	if (fill_buff != max_buff) {
		if (opt_aq_max_buff)
			free(opt_aq_max_buff);
		opt_aq_max_buff = max_buff;
	}
	if (fill_buff) {
		*fill_buff = '\0';
		if (opt_aq_fill_buff)
			free(opt_aq_fill_buff);
		opt_aq_fill_buff = ++fill_buff;
	}
	return 0;
}

static inline int parse_opt_R(const char *arg)
{
	/* I think pseudo reverb can now be retired... Computers are
	 * enough fast to do a full reverb, don't they?
	 */
	if (atoi(arg) == -1)	/* reset */
		modify_release = 0;
	else {
		if (set_val_i32(&modify_release, atoi(arg), 0, MAX_MREL,
				"Modify Release"))
			return 1;
		if (modify_release == 0)
			modify_release = DEFAULT_MREL;
	}
	return 0;
}

static inline int parse_opt_S(const char *arg)
{
	int suffix = arg[strlen(arg) - 1];
	int32 figure;
	
	switch (suffix) {
	case 'M':
	case 'm':
		figure = 1 << 20;
		break;
	case 'K':
	case 'k':
		figure = 1 << 10;
		break;
	default:
		figure = 1;
		break;
	}
	allocate_cache_size = atof(arg) * figure;
	return 0;
}

static inline int parse_opt_s(const char *arg)
{
	/* sampling rate */
	int32 freq;

	if ((freq = atoi(arg)) < 100)
		freq = atof(arg) * 1000 + 0.5;
	return set_val_i32(&opt_output_rate, freq,
			MIN_OUTPUT_RATE, MAX_OUTPUT_RATE, "Resampling frequency");
}

static inline int parse_opt_T(const char *arg)
{
	/* tempo adjust */
	int adjust;
	
	if (set_value(&adjust, atoi(arg), 10, 400, "Tempo adjust"))
		return 1;
	tempo_adjust = 100.0 / adjust;
	return 0;
}

static inline int parse_opt_t(const char *arg)
{
	if (output_text_code)
		free(output_text_code);
	output_text_code = safe_strdup(arg);
	return 0;
}

static inline int parse_opt_U(const char *arg)
{
	free_instruments_afterwards = y_or_n_p(arg);
	return 0;
}

static inline int parse_opt_volume_curve(char *arg)
{
	if (atof(arg) < 0) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"Volume curve power must be >= 0", *arg);
		return 1;
	}
	if (atof(arg) != 0) {
		init_user_vol_table(atof(arg));
		opt_user_volume_curve = 1;
	}
	return 0;
}

__attribute__((noreturn))
static inline int parse_opt_v(const char *arg)
{
	const char *version_list[] = {
#if defined(__BORLANDC__) || defined(__MRC__) || defined(__WATCOMC__)
		"TiMidity++ ",
				"",
				NULL, NLS,
		NLS,
#else
		"TiMidity++ ",
				(strcmp(timidity_version, "current")) ? "version " : "",
				timidity_version, NLS,
		NLS,
#endif
		"Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>", NLS,
		"Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>", NLS,
		NLS,
#ifdef __W32__
		"Win32 version by Davide Moretti <dmoretti@iper.net>", NLS,
		"              and Daisuke Aoki <dai@y7.net>", NLS,
		NLS,
#endif	/* __W32__ */
		"This program is distributed in the hope that it will be useful,", NLS,
		"but WITHOUT ANY WARRANTY; without even the implied warranty of", NLS,
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the", NLS,
		"GNU General Public License for more details.", NLS,
	};
	FILE *fp = open_pager();
	int i;

#if defined(__BORLANDC__) || defined(__MRC__)
	if (strcmp(timidity_version, "current"))
		version_list[1] = "version ";
	version_list[2] = timidity_version;
#endif
	for (i = 0; i < sizeof(version_list) / sizeof(char *); i++)
		fputs(version_list[i], fp);
	close_pager(fp);
	exit(EXIT_SUCCESS);
}


#ifdef __W32__
static inline int parse_opt_w(const char *arg)
{
	switch (*arg) {
#ifdef SMFCONV
	case 'r':
		opt_rcpcv_dll = 1;
		return 0:
	case 'R':
		opt_rcpcv_dll = 0;
		return 0;
#else
	case 'r':
	case 'R':
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"-w%c option is not supported", *arg);
		return 1;
#endif	/* SMFCONV */
	default:
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "-w: Illegal mode `%c'", *arg);
		return 1;
	}
}
#endif	/* __W32__ */

static inline int parse_opt_x(char *arg)
{
	StringTableNode *st;
	
	if ((st = put_string_table(&opt_config_string,
			arg, strlen(arg))) != NULL)
		expand_escape_string(st->string);
	return 0;
}

static inline void expand_escape_string(char *s)
{
	char *t = s;
	
	if (s == NULL)
		return;
	for (t = s; *s; s++)
		if (*s == '\\') {
			switch (*++s) {
			case 'a':
				*t++ = '\a';
				break;
			case 'b':
				*t++ = '\b';
				break;
			case 't':
				*t++ = '\t';
				break;
			case 'n':
				*t++ = '\n';
				break;
			case 'f':
				*t++ = '\f';
				break;
			case 'v':
				*t++ = '\v';
				break;
			case 'r':
				*t++ = '\r';
				break;
			case '\\':
				*t++ = '\\';
				break;
			default:
				if (! (*t++ = *s))
					return;
				break;
			}
		} else
			*t++ = *s;
	*t = *s;
}

static inline int parse_opt_Z(const char *arg)
{
	/* load frequency table */
//	return load_table(arg);
	return 1;
}

static inline int parse_opt_Z1(const char *arg)
{
	/* --pure-intonation */
	int keysig;
	
	opt_pure_intonation = 1;
	if (*arg) {
		if (set_value(&keysig, atoi(arg), -7, 7,
				"Initial keysig (number of #(+)/b(-)[m(minor)])"))
			return 1;
		opt_init_keysig = keysig;
		if (strchr(arg, 'm'))
			opt_init_keysig += 16;
	}
	return 0;
}

static inline int parse_opt_default_module(const char *arg)
{
	opt_default_module = atoi(arg);
	if (opt_default_module < 0)
		opt_default_module = 0;
	return 0;
}

__attribute__((noreturn))
static inline int parse_opt_fail(const char *arg)
{
	/* getopt_long failed to recognize any options */
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			"Could not understand option : try --help");
	exit(EXIT_FAILURE);
}

static inline int set_value(int *param, int i, int low, int high, char *name)
{
	int32 val;
	
	if (set_val_i32(&val, i, low, high, name))
		return 1;
	*param = val;
	return 0;
}

static inline int set_val_i32(int32 *param,
		int32 i, int32 low, int32 high, char *name)
{
	if (i < low || i > high) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"%s must be between %ld and %ld", name, low, high);
		return 1;
	}
	*param = i;
	return 0;
}

static inline int set_channel_flag(ChannelBitMask *flags, int32 i, char *name)
{
	if (i == 0) {
		FILL_CHANNELMASK(*flags);
		return 0;
	} else if (abs(i) > MAX_CHANNELS) {
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
				"%s must be between (-)1 and (-)%d, or 0",
						name, MAX_CHANNELS);
		return 1;
	}
	if (i > 0)
		SET_CHANNELMASK(*flags, i - 1);
	else
		UNSET_CHANNELMASK(*flags, -i - 1);
	return 0;
}

static inline int y_or_n_p(const char *arg)
{
	if (arg) {
		switch (arg[0]) {
		case 'y':
		case 'Y':
		case 't':
		case 'T':
			return 1;
		case 'n':
		case 'N':
		case 'f':
		case 'F':
		default:
			return 0;
		}
	} else
		return 1;
}

static inline int set_flag(int32 *fields, int32 bitmask, const char *arg)
{
	if (y_or_n_p(arg))
		*fields |= bitmask;
	else
		*fields &= ~bitmask;
	return 0;
}

static inline FILE *open_pager(void)
{
#if ! defined(__MACOS__) && defined(HAVE_POPEN) && defined(HAVE_ISATTY) \
		&& ! defined(IA_W32GUI) && ! defined(IA_W32G_SYN)
	char *pager;
	
	if (isatty(1) && (pager = getenv("PAGER")) != NULL)
		return popen(pager, "w");
#endif
	return stdout;
}

static inline void close_pager(FILE *fp)
{
#if ! defined(__MACOS__) && defined(HAVE_POPEN) && defined(HAVE_ISATTY) \
		&& ! defined(IA_W32GUI) && ! defined(IA_W32G_SYN)
	if (fp != stdout)
		pclose(fp);
#endif
}

static void interesting_message(void)
{
	printf(
"TiMidity++ %s%s -- MIDI to WAVE converter and player" NLS
"Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>" NLS
"Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>" NLS
			NLS
#ifdef __W32__
"Win32 version by Davide Moretti <dmoretti@iper.net>" NLS
"              and Daisuke Aoki <dai@y7.net>" NLS
			NLS
#endif /* __W32__ */
"This program is free software; you can redistribute it and/or modify" NLS
"it under the terms of the GNU General Public License as published by" NLS
"the Free Software Foundation; either version 2 of the License, or" NLS
"(at your option) any later version." NLS
			NLS
"This program is distributed in the hope that it will be useful," NLS
"but WITHOUT ANY WARRANTY; without even the implied warranty of" NLS
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" NLS
"GNU General Public License for more details." NLS
			NLS
"You should have received a copy of the GNU General Public License" NLS
"along with this program; if not, write to the Free Software" NLS
"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA" NLS
			NLS, (strcmp(timidity_version, "current")) ? "version " : "",
			timidity_version);
}

/* -------- functions for getopt_long ends here --------- */

#ifdef HAVE_SIGNAL
static RETSIGTYPE sigterm_exit(int sig)
{
    char s[4];

    /* NOTE: Here, fprintf is dangerous because it is not re-enterance
     * function.  It is possible coredump if the signal is called in printf's.
     */

    write(2, "Terminated sig=0x", 17);
    s[0] = "0123456789abcdef"[(sig >> 4) & 0xf];
    s[1] = "0123456789abcdef"[sig & 0xf];
    s[2] = '\n';
    write(2, s, 3);

    safe_exit(1);
}
#endif /* HAVE_SIGNAL */

static void timidity_arc_error_handler(char *error_message)
{
    extern int open_file_noise_mode;
    if(open_file_noise_mode)
	ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "%s", error_message);
}

static PlayMode null_play_mode = {
    0,                          /* rate */
    0,                          /* encoding */
    0,                          /* flag */
    -1,                         /* fd */
    {0,0,0,0,0},                /* extra_param */
    "Null output device",       /* id_name */
    '\0',                       /* id_character */
    NULL,                       /* open_output */
    NULL,                       /* close_output */
    NULL,                       /* output_data */
    NULL,                       /* acntl */
    NULL                        /* detect */
};

MAIN_INTERFACE void timidity_start_initialize(void)
{
    int i;
    static int drums[] = DEFAULT_DRUMCHANNELS;
    static int is_first = 1;
#if defined(__FreeBSD__) && !defined(__alpha__)
    fp_except_t fpexp;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
    fp_except fpexp;
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    fpexp = fpgetmask();
    fpsetmask(fpexp & ~(FP_X_INV|FP_X_DZ));
#endif

    if(!output_text_code)
	output_text_code = safe_strdup(OUTPUT_TEXT_CODE);
    if(!opt_aq_max_buff)
	opt_aq_max_buff = safe_strdup("5.0");
    if(!opt_aq_fill_buff)
	opt_aq_fill_buff = safe_strdup("100%");

    /* Check the byte order */
    i = 1;
#ifdef LITTLE_ENDIAN
    if(*(char *)&i != 1)
#else
    if(*(char *)&i == 1)
#endif
    {
	fprintf(stderr, "Byte order is miss configured.\n");
	exit(1);
    }

    for(i = 0; i < MAX_CHANNELS; i++)
    {
	memset(&(channel[i]), 0, sizeof(Channel));
    }

    CLEAR_CHANNELMASK(quietchannels);
    CLEAR_CHANNELMASK(default_drumchannels);

    for(i = 0; drums[i] > 0; i++)
	SET_CHANNELMASK(default_drumchannels, drums[i] - 1);
#if MAX_CHANNELS > 16
    for(i = 16; i < MAX_CHANNELS; i++)
	if(IS_SET_CHANNELMASK(default_drumchannels, i & 0xF))
	    SET_CHANNELMASK(default_drumchannels, i);
#endif

    if(program_name == NULL)
	program_name = "TiMidity";
    uudecode_unquote_html = 1;
    for(i = 0; i < MAX_CHANNELS; i++)
    {
	default_program[i] = DEFAULT_PROGRAM;
	memset(channel[i].drums, 0, sizeof(channel[i].drums));
    }
    //arc_error_handler = timidity_arc_error_handler;

    if(play_mode == NULL) play_mode = &null_play_mode;

    if(is_first) /* initialize once time */
    {
	got_a_configuration = 0;

	for(i = 0; url_module_list[i]; i++)
	    url_add_module(url_module_list[i]);
	init_string_table(&opt_config_string);
	init_freq_table();
	init_freq_table_tuning();
	init_freq_table_pytha();
	init_freq_table_meantone();
	init_freq_table_pureint();
	init_freq_table_user();
	init_bend_fine();
	init_bend_coarse();
	init_tables();
	init_gm2_pan_table();
	init_attack_vol_table();
	init_sb_vol_table();
	init_modenv_vol_table();
	init_def_vol_table();
	init_gs_vol_table();
	init_perceived_vol_table();
	init_gm2_vol_table();

	for(i = 0; i < NSPECIAL_PATCH; i++)
	    special_patch[i] = NULL;
	init_midi_trace();
	int_rand(-1);	/* initialize random seed */
	int_rand(42);	/* the 1st number generated is not very random */
	// unimod stuff: removed by oldnemesis
	//ML_RegisterAllLoaders ();
    }

    is_first = 0;
}

MAIN_INTERFACE int timidity_pre_load_configuration(void)
{
    /* UNIX */
    if(!read_config_file(CONFIG_FILE, 0))
		got_a_configuration = 1;

    return 0;
}

MAIN_INTERFACE int timidity_post_load_configuration(void)
{
    int i, cmderr = 0;

    if(play_mode == &null_play_mode)
    {
	char *output_id;

	output_id = getenv("TIMIDITY_OUTPUT_ID");
#ifdef TIMIDITY_OUTPUT_ID
	if(output_id == NULL)
	    output_id = TIMIDITY_OUTPUT_ID;
#endif /* TIMIDITY_OUTPUT_ID */
	if(output_id != NULL)
	{
	    for(i = 0; play_mode_list[i]; i++)
		if(play_mode_list[i]->id_character == *output_id)
		{
		    if (! play_mode_list[i]->detect ||
			play_mode_list[i]->detect()) {
			play_mode = play_mode_list[i];
			break;
		    }
		}
	}
    }

    if (play_mode == &null_play_mode) {
	/* try to detect the first available device */
	for(i = 0; play_mode_list[i]; i++) {
	    /* check only the devices with detect callback */
	    if (play_mode_list[i]->detect) {
		if (play_mode_list[i]->detect()) {
		    play_mode = play_mode_list[i];
		    break;
		}
	    }
	}
    }

    if (play_mode == &null_play_mode) {
	fprintf(stderr, "Couldn't open output device" NLS);
	exit(1);
    }
    else {
        /* apply changes made for null play mode to actual play mode */
        if(null_play_mode.encoding != 0) {
            play_mode->encoding |= null_play_mode.encoding;
        }
        if(null_play_mode.rate != 0) {
            play_mode->rate = null_play_mode.rate;
        }
    }

    if(!got_a_configuration)
    {
	if(try_config_again && !read_config_file(CONFIG_FILE, 0))
	    got_a_configuration = 1;
    }

    if(opt_config_string.nstring > 0)
    {
	char **config_string_list;

	config_string_list = make_string_array(&opt_config_string);
	if(config_string_list != NULL)
	{
	    for(i = 0; config_string_list[i]; i++)
	    {
		if(!read_config_file(config_string_list[i], 1))
		    got_a_configuration = 1;
		else
		    cmderr++;
	    }
	    free(config_string_list[0]);
	    free(config_string_list);
	}
    }

    if(!got_a_configuration)
	cmderr++;
    return cmderr;
}

MAIN_INTERFACE void timidity_init_player(void)
{
    initialize_resampler_coeffs();

    /* Allocate voice[] */
    voice = (Voice *) safe_realloc(voice, max_voices * sizeof(Voice));
	memset(voice, 0, max_voices * sizeof(Voice));

    /* Set play mode parameters */
    if(opt_output_rate != 0)
	play_mode->rate = opt_output_rate;
    else if(play_mode->rate == 0)
	play_mode->rate = DEFAULT_RATE;

    /* save defaults */
    COPY_CHANNELMASK(drumchannels, default_drumchannels);
    COPY_CHANNELMASK(drumchannel_mask, default_drumchannel_mask);

    if(opt_buffer_fragments != -1)
    {
	if(play_mode->flag & PF_BUFF_FRAGM_OPT)
	    play_mode->extra_param[0] = opt_buffer_fragments;
	else
	    ctl->cmsg(CMSG_WARNING, VERB_NORMAL,
		      "%s: -B option is ignored", play_mode->id_name);
    }

#ifdef SUPPORT_SOUNDSPEC
    if(view_soundspec_flag)
    {
	open_soundspec();
	soundspec_setinterval(spectrogram_update_sec);
    }
#endif /* SOUNDSPEC */
}

void timidity_init_aq_buff(void)
{
    double time1, /* max buffer */
	   time2, /* init filled */
	   base;  /* buffer of device driver */

    if(!IS_STREAM_TRACE)
	return; /* Ignore */

    time1 = atof(opt_aq_max_buff);
    time2 = atof(opt_aq_fill_buff);
    base  = (double)aq_get_dev_queuesize() / play_mode->rate;
    if(strchr(opt_aq_max_buff, '%'))
    {
	time1 = base * (time1 - 100) / 100.0;
	if(time1 < 0)
	    time1 = 0;
    }
    if(strchr(opt_aq_fill_buff, '%'))
	time2 = base * time2 / 100.0;
    aq_set_soft_queue(time1, time2);
}

MAIN_INTERFACE int timidity_play_main(int nfiles, char **files)
{
    int need_stdin = 0, need_stdout = 0;
    int i;
    int output_fail = 0;

    if(nfiles == 0 && !strchr(INTERACTIVE_INTERFACE_IDS, ctl->id_character))
	return 0;

    if(opt_output_name)
    {
	play_mode->name = opt_output_name;
	if(!strcmp(opt_output_name, "-"))
	    need_stdout = 1;
    }

    for(i = 0; i < nfiles; i++)
	if (!strcmp(files[i], "-"))
	    need_stdin = 1;

    if(ctl->open(need_stdin, need_stdout))
    {
	fprintf(stderr, "Couldn't open %s (`%c')" NLS,
		ctl->id_name, ctl->id_character);
	play_mode->close_output();
	return 3;
    }

    if(wrdt->open(wrdt_open_opts))
    {
	fprintf(stderr, "Couldn't open WRD Tracer: %s (`%c')" NLS,
		wrdt->name, wrdt->id);
	play_mode->close_output();
	ctl->close();
	return 1;
    }

#ifdef BORLANDC_EXCEPTION
    __try
    {
#endif /* BORLANDC_EXCEPTION */
#ifdef __W32__

#ifdef HAVE_SIGNAL
	signal(SIGTERM, sigterm_exit);
#endif
	SetConsoleCtrlHandler(handler, TRUE);

	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		  "Initialize for Critical Section");
	InitializeCriticalSection(&critSect);
	if(opt_evil_mode)
	    if(!SetThreadPriority(GetCurrentThread(),
				  THREAD_PRIORITY_ABOVE_NORMAL))
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
			  "Error raising process priority");

#else
	/* UNIX */
#ifdef HAVE_SIGNAL
	signal(SIGINT, sigterm_exit);
	signal(SIGTERM, sigterm_exit);
#ifdef SIGPIPE
	signal(SIGPIPE, sigterm_exit);    /* Handle broken pipe */
#endif /* SIGPIPE */
#endif /* HAVE_SIGNAL */

#endif

	/* Open output device */
	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		  "Open output: %c, %s",
		  play_mode->id_character,
		  play_mode->id_name);

	if (play_mode->flag & PF_PCM_STREAM) {
	    play_mode->extra_param[1] = aq_calc_fragsize();
	    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		      "requesting fragment size: %d",
		      play_mode->extra_param[1]);
	}
#if !defined ( IA_W32GUI ) && !defined ( IA_W32G_SYN )
	if(play_mode->open_output() < 0)
	{
	    ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		      "Couldn't open %s (`%c')",
		      play_mode->id_name, play_mode->id_character);
	    output_fail = 1;
	    ctl->close();
	    return 2;
	}
#endif /* IA_W32GUI */
	if(!control_ratio)
	{
	    control_ratio = play_mode->rate / CONTROLS_PER_SECOND;
	    if(control_ratio < 1)
		control_ratio = 1;
	    else if (control_ratio > MAX_CONTROL_RATIO)
		control_ratio = MAX_CONTROL_RATIO;
	}

	init_load_soundfont();
	if(!output_fail)
	{
	    aq_setup();
	    timidity_init_aq_buff();
	}
	if(allocate_cache_size > 0)
	    resamp_cache_reset();

	if (def_prog >= 0)
		set_default_program(def_prog);
	if (*def_instr_name)
		set_default_instrument(def_instr_name);

	if(ctl->flags & CTLF_LIST_RANDOM)
	    randomize_string_list(files, nfiles);
	else if(ctl->flags & CTLF_LIST_SORT)
	    sort_pathname(files, nfiles);

	/* Return only when quitting */
	ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		  "pass_playing_list() nfiles=%d", nfiles);

	ctl->pass_playing_list(nfiles, files);

	if(intr)
	    aq_flush(1);

#ifdef XP_UNIX
	return 0;
#endif /* XP_UNIX */

	play_mode->close_output();
	ctl->close();
	wrdt->close();
#ifdef __W32__
	DeleteCriticalSection (&critSect);
#endif

#ifdef BORLANDC_EXCEPTION
    } __except(1) {
	fprintf(stderr, "\nError!!!\nUnexpected Exception Occured!\n");
	if(play_mode->fd != -1)
	{
		play_mode->purge_output();
		play_mode->close_output();
	}
	ctl->close();
	wrdt->close();
	DeleteCriticalSection (&critSect);
	exit(EXIT_FAILURE);
    }
#endif /* BORLANDC_EXCEPTION */

#ifdef SUPPORT_SOUNDSPEC
    if(view_soundspec_flag)
	close_soundspec();
#endif /* SUPPORT_SOUNDSPEC */

    //free_archive_files();
#ifdef SUPPORT_SOCKET
    url_news_connection_cache(URL_NEWS_CLOSE_CACHE);
#endif /* SUPPORT_SOCKET */

    return 0;
}


static inline bool directory_p(const char* path)
{
    struct stat st;
    if(stat(path, &st) != -1) return S_ISDIR(st.st_mode);
    return false;
}

static inline void canonicalize_path(char* path)
{
    int len = strlen(path);
    if(!len || path[len-1]==PATH_SEP) return;
    path[len] = PATH_SEP;
    path[len+1] = '\0';
}


#if defined (ENABLE_MAIN)
int main(int argc, char **argv)
{
    int c, err, i;
    int nfiles;
    char **files;
    int main_ret;
    int longind;

    timidity_start_initialize();

	opt_output_name = safe_strdup( "za_togo_parna.tmp" );

    if((err = timidity_pre_load_configuration()) != 0)
		return err;

    err += timidity_post_load_configuration();

    // If there were problems, give up now
    if( err )
    {
		if(!got_a_configuration)
		{
	    	ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		    	  "%s: Can't read any configuration file.\nPlease check "
		      	CONFIG_FILE, program_name);
		}
	}

    timidity_init_player();

	// Set play mode parameters
	play_mode->rate = 48000;
	play_mode->encoding = 0;
	play_mode->encoding |= PE_16BIT;
	play_mode->encoding |= PE_SIGNED;

    main_ret = timidity_play_main( 1, argv + 1 );

    free_instruments(0);
    free_global_mblock();
    free_all_midi_file_info();
	free_userdrum();
	free_userinst();
    tmdy_free_config();
	free_effect_buffers();
	
	for (i = 0; i < MAX_CHANNELS; i++)
		free_drum_effect(i);

    return main_ret;
}
#endif // ENABLE_MAIN


//
// Timidity library external interface.
// Has to be here because calls static functions
//
extern PlayMode buffer_play_mode; // defined in buffer_a.c


int Timidity_Init(int rate, int bits_per_sample, int channels, const char * soundfont_file )
{
    int c, err, i;

	play_mode = &buffer_play_mode;

    timidity_start_initialize();

	// If soundfont_file is specified, we do not need to load configuration
	if ( soundfont_file )
	{
		// Check if it exists and could be opened to provide a reasonable error message,
		// since add_soundfont does not return a value
		int fd = open( soundfont_file, O_RDONLY );

		if ( fd >= 0 )
		{
			close( fd );

			add_soundfont( soundfont_file, 0, -1, -1, -1 );
			
			// most soundfounts I've seen are too quiet; change the amplification
			amplification = 200;
			got_a_configuration = 1;
			err = 0;
		}
	}

	if( !got_a_configuration )
	{
    	if((err = timidity_pre_load_configuration()) != 0)
			return err;

    	err += timidity_post_load_configuration();
	}

    // If there were problems, give up now
    if( err )
    {
		if(!got_a_configuration)
		{
	    	ctl->cmsg(CMSG_FATAL, VERB_NORMAL,
		    	  "%s: Can't read any configuration file.\nPlease check "
		      	CONFIG_FILE, program_name);
		}
		
		return err;
	}

    timidity_init_player();

	// Set play mode parameters
	play_mode->rate = rate;

	switch ( bits_per_sample )
	{
		case 16:
			play_mode->encoding |= PE_16BIT;
			play_mode->encoding &= ~(PE_24BIT | PE_ULAW | PE_ALAW);
			break;

		case 24:
			play_mode->encoding |= PE_24BIT;
			play_mode->encoding &= ~(PE_16BIT | PE_ULAW | PE_ALAW);
			break;

		case 8:
			play_mode->encoding &= ~(PE_16BIT | PE_24BIT);
			break;
	}

	if ( channels == 1 )
		play_mode->encoding |= PE_MONO;

	if (play_mode->flag & PF_PCM_STREAM) {
	    play_mode->extra_param[1] = aq_calc_fragsize();
	    ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		      "requesting fragment size: %d",
		      play_mode->extra_param[1]);
	}

	if(!control_ratio)
	{
	    control_ratio = play_mode->rate / CONTROLS_PER_SECOND;
	    if(control_ratio < 1)
		control_ratio = 1;
	    else if (control_ratio > MAX_CONTROL_RATIO)
		control_ratio = MAX_CONTROL_RATIO;
	}

	init_load_soundfont();

    aq_setup();
    timidity_init_aq_buff();

	if(allocate_cache_size > 0)
	    resamp_cache_reset();

	if (def_prog >= 0)
		set_default_program(def_prog);

	if (*def_instr_name)
		set_default_instrument(def_instr_name);

	return 0;
}

void Timidity_Cleanup()
{
	int i;

#ifdef SUPPORT_SOUNDSPEC
    if(view_soundspec_flag)
	close_soundspec();
#endif /* SUPPORT_SOUNDSPEC */

    free_instruments(0);
    free_all_midi_file_info();
	free_userdrum();
	free_userinst();
    tmdy_free_config();
	free_effect_buffers();
	
	for ( i = 0; i < MAX_CHANNELS; i++)
		free_drum_effect(i);

	if ( output_text_code )
		free( output_text_code );

    if ( opt_aq_max_buff )
		free( opt_aq_max_buff );
    
	if ( opt_aq_fill_buff )
		free( opt_aq_fill_buff );

	resamp_cache_free();
	delete_string_table( &opt_config_string );

	free_soundfonts();
	free_gauss_table();
	free_tone_bank();
	free_midi_file_data();
	resamp_cache_free_completely();

	free( voice );

	// Must be called last
	free_global();
}

int Timidity_GetLength( MidiSong *song )
{
	return song->samples / 48000 * 1000;
}

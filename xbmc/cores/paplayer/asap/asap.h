/*
 * asap.h - public interface of the ASAP engine
 *
 * Copyright (C) 2005-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _ASAP_H_
#define _ASAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ASAP version. */
#define ASAP_VERSION_MAJOR   2
#define ASAP_VERSION_MINOR   0
#define ASAP_VERSION_MICRO   0
#define ASAP_VERSION         "2.0.0"

/* Short credits of the ASAP engine. */
#define ASAP_YEARS           "2005-2009"
#define ASAP_CREDITS \
	"Another Slight Atari Player (C) 2005-2009 Piotr Fusik\n" \
	"CMC, MPT, TMC, TM2 players (C) 1994-2005 Marcin Lewandowski\n" \
	"RMT player (C) 2002-2005 Radek Sterba\n" \
	"DLT player (C) 2009 Marek Konopka\n" \
	"CMS player (C) 1999 David Spilka\n"

/* Short GPL notice.
   Display after the credits. */
#define ASAP_COPYRIGHT \
	"This program is free software; you can redistribute it and/or modify\n" \
	"it under the terms of the GNU General Public License as published\n" \
	"by the Free Software Foundation; either version 2 of the License,\n" \
	"or (at your option) any later version."

/* Maximum length of a "mm:ss.xxx" string including the terminator. */
#define ASAP_DURATION_CHARS     10

/* Maximum length of a supported input file.
   You can assume that files longer than this are not supported by ASAP. */
#define ASAP_MODULE_MAX         65000

/* Maximum number of songs in a file. */
#define ASAP_SONGS_MAX          32

/* Output sample rate. */
#define ASAP_SAMPLE_RATE        44100

/* WAV file header length. */
#define ASAP_WAV_HEADER_BYTES   44

/* Output formats. */
typedef enum {
	ASAP_FORMAT_U8 = 8,       /* unsigned char */
	ASAP_FORMAT_S16_LE = 16,  /* signed short, little-endian */
	ASAP_FORMAT_S16_BE = -16  /* signed short, big-endian */
} ASAP_SampleFormat;

/* Useful type definitions. */
#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif
typedef int abool;
typedef unsigned char byte;

/* Information about a file. */
typedef struct {
	char author[128];    /* author's name */
	char name[128];      /* title */
	char date[128];      /* creation date */
	int channels;        /* 1 for mono or 2 for stereo */
	int songs;           /* number of subsongs */
	int default_song;    /* 0-based index of the "main" subsong */
	int durations[ASAP_SONGS_MAX]; /* lengths of songs, in milliseconds, -1 = unspecified */
	abool loops[ASAP_SONGS_MAX];   /* whether songs repeat or not */
	/* the following technical information should not be used outside ASAP. */
	int type;
	int fastplay;
	int music;
	int init;
	int player;
	int covox_addr;
	int header_len;
	byte song_pos[128];
} ASAP_ModuleInfo;

/* POKEY state.
   Not for use outside the ASAP engine. */
typedef struct {
	int audctl;
	abool init;
	int poly_index;
	int div_cycles;
	int mute1;
	int mute2;
	int mute3;
	int mute4;
	int audf1;
	int audf2;
	int audf3;
	int audf4;
	int audc1;
	int audc2;
	int audc3;
	int audc4;
	int tick_cycle1;
	int tick_cycle2;
	int tick_cycle3;
	int tick_cycle4;
	int period_cycles1;
	int period_cycles2;
	int period_cycles3;
	int period_cycles4;
	int reload_cycles1;
	int reload_cycles3;
	int out1;
	int out2;
	int out3;
	int out4;
	int delta1;
	int delta2;
	int delta3;
	int delta4;
	int skctl;
	int delta_buffer[888];
} PokeyState;

/* Player state.
   Only module_info is meant to be read outside the ASAP engine. */
typedef struct {
	int cycle;
	int cpu_pc;
	int cpu_a;
	int cpu_x;
	int cpu_y;
	int cpu_s;
	int cpu_nz;
	int cpu_c;
	int cpu_vdi;
	int scanline_number;
	int nearest_event_cycle;
	int next_scanline_cycle;
	int timer1_cycle;
	int timer2_cycle;
	int timer4_cycle;
	int irqst;
	int extra_pokey_mask;
	int consol;
	byte covox[4];
	PokeyState base_pokey;
	PokeyState extra_pokey;
	int sample_offset;
	int sample_index;
	int samples;
	int iir_acc_left;
	int iir_acc_right;
	ASAP_ModuleInfo module_info;
	int tmc_per_frame;
	int tmc_per_frame_counter;
	int current_song;
	int current_duration;
	int blocks_played;
	int silence_cycles;
	int silence_cycles_counter;
	byte poly9_lookup[511];
	byte poly17_lookup[16385];
	byte memory[65536];
} ASAP_State;

/* Parses the string in the "mm:ss.xxx" format
   and returns the number of milliseconds or -1 if an error occurs. */
int ASAP_ParseDuration(const char *s);

/* Converts number of milliseconds to a string in the "mm:ss.xxx" format. */
void ASAP_DurationToString(char *s, int duration);

/* Checks whether the extension of the passed filename is known to ASAP. */
abool ASAP_IsOurFile(const char *filename);

/* Checks whether the filename extension is known to ASAP. */
abool ASAP_IsOurExt(const char *ext);

/* Changes the filename extension, returns true on success. */
abool ASAP_ChangeExt(char *filename, const char *ext);

/* Gets information about a module.
   "module_info" is the structure where the information is returned.
   "filename" determines file format.
   "module" is the music data (contents of the file).
   "module_len" is the number of data bytes.
   ASAP_GetModuleInfo() returns true on success. */
abool ASAP_GetModuleInfo(ASAP_ModuleInfo *module_info, const char *filename,
                         const byte module[], int module_len);

/* Loads music data.
   "ast" is the destination structure.
   "filename" determines file format.
   "module" is the music data (contents of the file).
   "module_len" is the number of data bytes.
   ASAP does not make copies of the passed pointers. You can overwrite
   or free "filename" and "module" once this function returns.
   ASAP_Load() returns true on success.
   If false is returned, the structure is invalid and you cannot
   call the following functions. */
abool ASAP_Load(ASAP_State *ast, const char *filename,
                const byte module[], int module_len);

/* Enables silence detection.
   Makes ASAP finish playing after the specified period of silence.
   "ast" is ASAP state initialized by ASAP_Load().
   "seconds" is the minimum length of silence that ends playback. */
void ASAP_DetectSilence(ASAP_State *ast, int seconds);

/* Prepares ASAP to play the specified song of the loaded module.
   "ast" is ASAP state initialized by ASAP_Load().
   "song" is a zero-based index which must be less than the "songs" field
   of the ASAP_ModuleInfo structure.
   "duration" is playback time in milliseconds - use durations[song]
   unless you want to override it. -1 means indefinitely. */
void ASAP_PlaySong(ASAP_State *ast, int song, int duration);

/* Mutes the selected POKEY channels.
   This is only useful for people who want to grab samples of individual
   instruments.
   "ast" is ASAP state after calling ASAP_PlaySong().
   "mask" is a bit mask which selects POKEY channels to be muted.
   Bits 0-3 control the base POKEY channels,
   bits 4-7 control the extra POKEY channels. */
void ASAP_MutePokeyChannels(ASAP_State *ast, int mask);

/* Returns current position in milliseconds.
   "ast" is ASAP state initialized by ASAP_PlaySong(). */
int ASAP_GetPosition(const ASAP_State *ast);

/* Rewinds the current song.
   "ast" is ASAP state initialized by ASAP_PlaySong().
   "position" is the requested absolute position in milliseconds. */
void ASAP_Seek(ASAP_State *ast, int position);

/* Fills the specified buffer with WAV file header.
   "ast" is ASAP state initialized by ASAP_PlaySong() with a positive "duration".
   "buffer" is buffer of ASAP_WAV_HEADER_BYTES bytes.
   "format" is the format of samples. */
void ASAP_GetWavHeader(const ASAP_State *ast, byte buffer[],
                       ASAP_SampleFormat format);

/* Fills the specified buffer with generated samples.
   "ast" is ASAP state initialized by ASAP_PlaySong().
   "buffer" is the destination buffer.
   "buffer_len" is the length of this buffer in bytes.
   "format" is the format of samples.
   ASAP_Generate() returns number of bytes actually written
   (less than buffer_len if reached the end of the song).
   Normally you use a buffer of a few kilobytes or less,
   and call ASAP_Generate() in a loop or via a callback. */
int ASAP_Generate(ASAP_State *ast, void *buffer, int buffer_len,
                  ASAP_SampleFormat format);

/* Checks whether information in the specified file can be edited. */
abool ASAP_CanSetModuleInfo(const char *filename);

/* Updates the specified module with author, name, date, stereo
   and song durations as specified in "module_info".
   "module_info" contains the new module information.
   "module" is the source file contents.
   "module_len" is the source file length.
   "out_module" is the destination buffer of size ASAP_MODULE_MAX.
   ASAP_SetModuleInfo() returns the resulting file length (number of bytes
   written to "out_module") or -1 if illegal characters were found. */
int ASAP_SetModuleInfo(const ASAP_ModuleInfo *module_info, const byte module[],
                       int module_len, byte out_module[]);

/* Checks whether the specified module can be converted to another format.
   "filename" determines the source format.
   "module_info" contains the information about the source module,
   with possibly modified public fields.
   "module" is the source file contents.
   "module_len" is the source file length.
   ASAP_CanConvert() returns the extension of the target format
   or NULL if there's no possible conversion. */
const char *ASAP_CanConvert(const char *filename, const ASAP_ModuleInfo *module_info,
                            const byte module[], int module_len);

/* Converts the specified module to the format returned by ASAP_CanConvert().
   "filename" determines the source format.
   "module_info" contains the information about the source module,
   with possibly modified public fields.
   "module" is the source file contents.
   "module_len" is the source file length.
   "out_module" is the destination buffer of size ASAP_MODULE_MAX.
   ASAP_Convert() returns the resulting file length (number of bytes
   written to "out_module") or -1 on error. */
int ASAP_Convert(const char *filename, const ASAP_ModuleInfo *module_info,
                 const byte module[], int module_len, byte out_module[]);

#ifdef __cplusplus
}
#endif

#endif

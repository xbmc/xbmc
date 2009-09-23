/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    output.h

*/

#ifndef ___OUTPUT_H_
#define ___OUTPUT_H_
#include "output.h"

/* Data format encoding bits */
/* {PE_16BIT,PE_ULAW,PE_ALAW} is alternative flag */
/* {PE_SIGNED,PE_ULAW,PE_ALAW} is alternative flag */
/* {PE_BYTESWAP,PE_ULAW,PE_ALAW} is alternative flag */
/* {PE_16BIT,PE_24BIT} is alternative flag */
#define PE_MONO 	(1u<<0)  /* versus stereo */
#define PE_SIGNED	(1u<<1)  /* versus unsigned */
#define PE_16BIT 	(1u<<2)  /* versus 8-bit */
#define PE_ULAW 	(1u<<3)  /* versus linear */
#define PE_ALAW 	(1u<<4)  /* versus linear */
#define PE_BYTESWAP	(1u<<5)  /* versus the other way */
#define PE_24BIT	(1u<<6)  /* versus 8-bit, 16-bit */

/* for play_mode->acntl() */
enum {
    PM_REQ_MIDI,	/* ARG: MidiEvent
			 * Send MIDI event.
			 * If PF_MIDI_EVENT is setted, acntl() is called
			 * with this request.
			 */

    PM_REQ_INST_NAME,	/* ARG: char**
			 * Get Instrument name of channel.
			 */

    PM_REQ_DISCARD,	/* ARG: not-used
			 * Discard the audio device buffer and returns
			 * immediatly.
			 */

    PM_REQ_FLUSH,	/* ARG: not-used
			 * Wait until all audio data is out.
			 */

    PM_REQ_GETQSIZ,	/* ARG: int
			 * Get maxmum device queue size in bytes.
			 * If acntl() returns -1,
			 * timidity automatically estimate the size
			 * using adhoc implementation.
			 * This request is used for trace mode.
			 */

    PM_REQ_SETQSIZ,	/* ARG: int (in-out)
			 * Set maxmum device queue size in bytes.
			 * The specified ARG is updated new queue size.
			 */

    PM_REQ_GETFRAGSIZ,	/* ARG: int
			 * Get device fragment size in bytes.
			 */

    PM_REQ_RATE,	/* ARG: int
			 * Change the sample rate.
			 */

    PM_REQ_GETSAMPLES,	/* ARG: int
			 * Get the current play samples.
			 * Play samples must be initialized to zero if
			 * PM_REQ_DISCARD/PM_REQ_FLUSH/PM_REQ_PLAY_START
			 * request is receved.
			 */

    PM_REQ_PLAY_START,	/* ARG: not-used
			 * PM_REQ_PLAY_START is called just before playing.
			 */

    PM_REQ_PLAY_END,	/* ARG: not-used
			 * PM_REQ_PLAY_END is called just after playing.
			 */

    PM_REQ_GETFILLABLE,	/* ARG: int
			 * Get fillable device queue size
			 */

    PM_REQ_GETFILLED,	/* ARG: int
			 * Get filled device queue size
			 */

    PM_REQ_OUTPUT_FINISH /* ARG: not-used
			  * PM_REQ_OUTPUT_FINISH calls just after the last
			  * output_data(), and TiMidity would into
			  * waiting to flush the audio buffer.
			  */
};


/* Flag bits */
#define PF_PCM_STREAM	(1u<<0)	/* Enable output PCM data */
#define PF_MIDI_EVENT	(1u<<1)	/* Enable send MIDI event via acntl() */
#define PF_CAN_TRACE	(1u<<2)	/* Enable realtime tracing */
#define PF_BUFF_FRAGM_OPT (1u<<3) /* Enable set extra_param[0] to specify
				   the number of audio buffer fragments */
#define PF_AUTO_SPLIT_FILE (1u<<4) /* Split PCM files automatically */
#define IS_STREAM_TRACE	((play_mode->flag & (PF_PCM_STREAM|PF_CAN_TRACE)) == (PF_PCM_STREAM|PF_CAN_TRACE))

typedef struct {
    int32 rate, encoding, flag;
    int fd; /* file descriptor for the audio device
	       -1 means closed otherwise opened. It must be -1 by default. */
    int32 extra_param[5]; /* System depended parameters
			     e.g. buffer fragments, ... */
    char *id_name, id_character;
    char *name; /* default device or file name */
    int (* open_output)(void); /* 0=success, 1=warning, -1=fatal error */
    void (* close_output)(void);

    int (* output_data)(char *buf, int32 bytes);
    /* return: -1=error, otherwise success */

    int (* acntl)(int request, void *arg); /* see PM_REQ_* above
					    * return: 0=success, -1=fail
					    */
    int (* detect)(void); /* 0=not available, 1=available */
} PlayMode;

extern PlayMode *play_mode_list[], *play_mode;
extern PlayMode *target_play_mode;
extern int audio_buffer_bits;
#define audio_buffer_size	(1<<audio_buffer_bits)

/* Conversion functions -- These overwrite the int32 data in *lp with
   data in another format */

/* 8-bit signed and unsigned*/
extern void s32tos8(int32 *lp, int32 c);
extern void s32tou8(int32 *lp, int32 c);

/* 16-bit */
extern void s32tos16(int32 *lp, int32 c);
extern void s32tou16(int32 *lp, int32 c);

/* 24-bit */
extern void s32tos24(int32 *lp, int32 c);
extern void s32tou24(int32 *lp, int32 c);

/* byte-exchanged 16-bit */
extern void s32tos16x(int32 *lp, int32 c);
extern void s32tou16x(int32 *lp, int32 c);

/* uLaw (8 bits) */
extern void s32toulaw(int32 *lp, int32 c);

/* aLaw (8 bits) */
extern void s32toalaw(int32 *lp, int32 c);

extern int32 general_output_convert(int32 *buf, int32 count);
extern int validate_encoding(int enc, int include_enc, int exclude_enc);
extern const char *output_encoding_string(int enc);

extern char *create_auto_output_name(const char *input_filename, char *ext_str, char *output_dir, int mode);

#if defined(__W32__)
#define FILE_OUTPUT_MODE	O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644
#elif defined(__MACOS__)
#define FILE_OUTPUT_MODE	O_WRONLY|O_CREAT|O_TRUNC
#else /* UNIX */
#define FILE_OUTPUT_MODE	O_WRONLY|O_CREAT|O_TRUNC, 0644
#endif

#endif /* ___OUTPUT_H_ */


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
	
	mfi.c (Melody Format for i-mode)
	by Kentaro Sato	<kentaro@ranvis.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include <stdlib.h>

#define SETMIDIEVENT(e, at, t, ch, pa, pb) \
    { (e).time = (at); (e).type = (t); \
      (e).channel = (uint8)(ch); (e).a = (uint8)(pa); (e).b = (uint8)(pb); }

#define MIDIEVENT(at, t, ch, pa, pb) \
    { MidiEvent event; SETMIDIEVENT(event, at, t, ch, pa, pb); \
      readmidi_add_event(&event); }

#ifdef LITTLE_ENDIAN
#define BE_FCC(type)		((uint32)XCHG_LONG(type))
#else /* BIG ENDIAN */
#define BE_FCC(type)		((uint32)BE_LONG(type))
#endif
	/*
		Note: Comparing MFi FCC (Four Character Code)
		In MFi, FCC has the same bit length as 4-byte-integer such as dataLength,
		so loading and comparing it like an integer should be portable enough.
	*/
#define MAPBITS(effInt, effBits, mapBits)		(((effInt) << ((mapBits) - (effBits))) | ((effInt) >> ((effBits) - ((mapBits) - (effBits)))))
#define MAPBITS2(effInt, effBits, mapBits)		(((effInt) << ((mapBits) - (effBits))) | (((effInt) << ((mapBits) - (effBits))) >> (effBits)) | (((effInt) << ((mapBits) - (effBits))) >> ((effBits) * 2)))

#define MFI_DEBUG_PREFIX			"MFi "
#define MFI_DEBUG_MORE_VERBOSE		0
#define MFI_DEBUG_NOTE_EVENT		0
#define MFI_DEBUG_NOTE_EVENT_S		0
#define MFI_DEBUG_CTL_DATA			0
#define MFI_DEBUG_UNKNOWN_CTL_DATA	1

#define POS_DS(pos)	((pos) / 48), ((pos) % 48)
#if MFI_DEBUG_MORE_VERBOSE
	#undef VERB_DEBUG
	#undef VERB_NOISY
	#define VERB_DEBUG	VERB_NORMAL
	#define VERB_NOISY	VERB_NORMAL
#endif
#if MFI_DEBUG_NOTE_EVENT
	#define NOTE_EVENT_DEBUGSTR(channel, note, octave, velocity, duration)	\
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, MFI_DEBUG_PREFIX "%d.%02d [%02d] %s%d@%d %d(%d.%02d)", POS_DS(pos), (channel)+1, note, octave, velocity, duration, POS_DS(pos + duration))
	#define NOTE_EVENT_POSSIBLE_SLUR_DEBUGSTR()								/* empty */
#else
	#define NOTE_EVENT_DEBUGSTR(channel, note, octave, velocity, duration)	/* empty */
	#define NOTE_EVENT_POSSIBLE_SLUR_DEBUGSTR()								/* empty */
#endif
#if MFI_DEBUG_NOTE_EVENT_S
	#define NOTE_BUF_EV_DEBUGSTR(channel, time, note, octave, velocity, offtime)	\
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, MFI_DEBUG_PREFIX "<< %d [%02d] %s%d@%d %d", time, (channel)+1, note, octave, velocity, offtime)
#else
	#define NOTE_BUF_EV_DEBUGSTR(channel, time, note, octave, velocity, offtime)	/* empty */
#endif
#if MFI_DEBUG_CTL_DATA
	#define EX_DATA_DEBUGSTR1(exname, channel, paramstr, p1)	\
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, MFI_DEBUG_PREFIX "%d.%02d [%02d] %02X(%s) : " paramstr, POS_DS(pos), (channel)+1, data[2], exname, p1)
	#define EX_NCDATA_DEBUGSTR1(exname, paramstr, p1)	\
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, MFI_DEBUG_PREFIX "%d.%02d [--] %02X(%s) : " paramstr, POS_DS(pos), data[2], exname, p1)
	#define EX_DATA_DEBUGSTR2(exname, channel, paramstr, p1, p2)	\
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, MFI_DEBUG_PREFIX "%d.%02d [%02d] %02X(%s) : " paramstr, POS_DS(pos), (channel)+1, data[2], exname, p1, p2)
#else
	#define EX_DATA_DEBUGSTR1(exname, channel, paramstr, p1)		/* empty */
	#define EX_NCDATA_DEBUGSTR1(exname, channel, p1)				/* empty */
	#define EX_DATA_DEBUGSTR2(exname, channel, paramstr, p1, p2)	/* empty */
#endif
#if MFI_DEBUG_UNKNOWN_CTL_DATA
	#define EX_UNKNOWN_DATA_DEBUGSTR()	\
			ctl->cmsg(CMSG_WARNING, VERB_NOISY, MFI_DEBUG_PREFIX "%d.%02d %02X (not implemented) : %02X", POS_DS(pos), data[2], data[3])
	#define EX_UNKNOWN_EXT_DATA_DEBUGSTR(len)	\
			ctl->cmsg(CMSG_WARNING, VERB_NOISY, MFI_DEBUG_PREFIX "%d.%02d %02X (not implemented) : %04X", POS_DS(pos), data[2], len)
#else
	#define EX_UNKNOWN_DATA_DEBUGSTR()								/* empty */
	#define EX_UNKNOWN_EXT_DATA_DEBUGSTR(len)						/* empty */
#endif

typedef struct timidity_file	timidity_file;

static int tf_read_beint16(int *, timidity_file *);
static int tf_read_beint32(int *, timidity_file *);
static int read_mfi_information(int, int *, int *, int *, timidity_file *);
static int read_mfi_track(int, int, int, int, int, timidity_file *);

int read_mfi_file(timidity_file *tf)
{
	int					length, dataLength, infoLength, dataType, mfiVersion, noteType, extStDLength;
	uint32				type;
	uint8				numTracks;
	int					i;
	
	if (tf_read_beint32(&dataLength, tf) != 1
			|| tf_read_beint16(&infoLength, tf) != 1
			|| tf_read_beint16(&dataType, tf) != 1
			|| tf_read(&numTracks, 1, 1, tf) != 1)
		return 1;
	infoLength -= 2 + 1;
	if (dataType == 0x0202)
	{
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "MFi Type 2.2 may not be playable.");	/* I'm not sure :-) */
		return 1;
	}
	if (numTracks == 0)
	{
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "MFi contains no track.");
		return 1;
	}
	ctl->cmsg(CMSG_INFO, VERB_VERBOSE, "MFi Tracks: %d", numTracks);
	if (numTracks > MAX_CHANNELS / 4)
	{
		ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Too many tracks, last %d track(s) are ignored.", numTracks - (MAX_CHANNELS / 4));
		numTracks = MAX_CHANNELS / 4;
	}
	current_file_info->divisions = 48;
	current_file_info->format = 1;
	current_file_info->tracks = numTracks;
	current_file_info->file_type = IS_MFI_FILE;
	noteType = extStDLength = 0;
	if (read_mfi_information(infoLength, &mfiVersion, &noteType, &extStDLength, tf) != 0)
		return 1;
	for(i = 0; i < numTracks; i++)
	{
		if (tf_read(&type, 4, 1, tf) != 1
				|| tf_read_beint32(&length, tf) != 1)
			return 1;
		if (type != BE_FCC(0x74726163 /*trac*/))
		{
			ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Unknown track signature.");
			return 1;
		}
		if (read_mfi_track(i, length, mfiVersion, noteType, extStDLength, tf) != 0)
			return 1;
	}
	return 0;
}

static int read_mfi_information(int infoLength, int *mfiVersion, int *noteType, int *extStDLength, timidity_file *tf)
{
	int				length;
	uint32			type;
	uint8			byteData;
	char			buf[512];
	
	*mfiVersion = 1;
	while (infoLength > 0)
	{
		if (infoLength < 4 + 2)
		{
			ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Odd information length.");
			return 1;
		}
		infoLength -= 6;
		if (tf_read(&type, 4, 1, tf) != 1
				|| tf_read_beint16(&length, tf) != 1)
			return 1;
		if (length == 0)
			continue;
		if (length > infoLength)
		{
			ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Total information length was too small.");
			return 1;
		}
		infoLength -= length;
		switch(type)
		{
			case BE_FCC(0x7469746C /*titl*/): {	/* title */
				char			*title;
				
				if (current_file_info->seq_name == NULL)
					goto skip_info_data;
				title = safe_malloc(length + 1);
				if (tf_read(title, length, 1, tf) != 1)
				{
					free(title);
					return 1;
				}
				title[length] = '\0';
				current_file_info->seq_name = title;
				ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "Title: %s", title);
			}	break;
			case BE_FCC(0x736F7263 /*sorc*/): {	/* source */
				const char		*srcInfo;
				
				if (length != 1)
					goto skip_info_data;
				if (tf_read(&byteData, 1, 1, tf) != 1)
					return 1;
				switch (byteData >> 1)
				{
					case 0x00:	srcInfo = "network";	break;
					case 0x01:	srcInfo = "manual";	break;
					case 0x02:	srcInfo = "external";	break;
					default: srcInfo = "unknown";
				}
				ctl->cmsg(CMSG_INFO, VERB_NOISY, "Source: %s%s", srcInfo, (byteData & 1) ? ", copyrighted" : "");
			}	break;
			case BE_FCC(0x76657273 /*vers*/):	/* version (unused) */
				if (tf_read(&type, 4, 1, tf) != 1)
					return 1;
				switch(type)
				{
					case BE_FCC(0x30313030 /*0100*/):	*mfiVersion = 1;	break;
					case BE_FCC(0x30323030 /*0200*/):	*mfiVersion = 2;	break;
					case BE_FCC(0x30333030 /*0300*/):	*mfiVersion = 3;	break;
					default:
						ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Unknown MFi version.");
						return 1;
				}
				ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "MFi Version: %d", *mfiVersion);
				/*
					info/controls which are marked as '(MFi*)' are only executable when
					mfiVersion == * or mfiVersion >= *.
				*/
				break;
			case BE_FCC(0x64617465 /*date*/): {	/* created date */
				char			created[9];
				
				if (length != 8)
					goto skip_info_data;
				if (tf_read(created, 8, 1, tf) != 1)
					return 1;
				created[8] = '\0';
				ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "Date: %.4s-%.2s-%.2s", created, &created[4], &created[6]);
			}	break;
			case BE_FCC(0x636F7079 /*copy*/): {	/* copyright */
				int				lengthToRead;
				
				lengthToRead = length >= sizeof buf ? sizeof buf - 1 : length;
				if (lengthToRead > 0 && tf_read(buf, lengthToRead, 1, tf) != 1)
					return 1;
				buf[lengthToRead] = '\0';
				ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "Copyright: %s", buf);
				if (lengthToRead < length && tf_seek(tf, length - lengthToRead, SEEK_CUR) == -1)
					return 1;
			}	break;
			/* case BE_FCC(0x70726F74 /-*prot*-/):
				goto skip_info_data; */
			case BE_FCC(0x6E6F7465 /*note*/):	/* note (MFi2) */
				if (length != 2 || tf_read_beint16(noteType, tf) != 1)
					return 1;
				if (*noteType > 1)
				{
					ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Unknown note information.");
					return 1;
				}
				ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Note Type: %d", *noteType);
				break;
			case BE_FCC(0x65787374 /*exst*/):	/* extended status (MFi2) */
				if (length != 2 || tf_read_beint16(extStDLength, tf) != 1)
					return 1;
				if (*extStDLength != 0)
				{
					ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Unknown extended status information. (%d)");
					return 1;
				}
				break;
			default:
				skip_info_data:
				if (tf_seek(tf, length, SEEK_CUR) == -1)
					return 1;
		}
	}
	return 0;
}

char *get_mfi_file_title(timidity_file *tf)
{
	int				length, dataLength, infoLength, dataType;
	uint32			type;
	uint8			numTracks;
	
	if (tf_read_beint32(&dataLength, tf) != 1
			|| tf_read_beint16(&infoLength, tf) != 1
			|| tf_read_beint16(&dataType, tf) != 1
					|| dataType == 0x0202
			|| tf_read(&numTracks, 1, 1, tf) != 1)
		return NULL;
	infoLength -= 2 + 1;
	while (infoLength >= 4 + 2)
	{
		infoLength -= 6;
		if (tf_read(&type, 4, 1, tf) != 1
				|| tf_read_beint16(&length, tf) != 1)
			return NULL;
		if (length > infoLength)
			break;
		infoLength -= length;
		if (type == BE_FCC(0x7469746C /*titl*/))
		{
			char				*title;
			
			if (length == 0)
				return NULL;
			if ((title = malloc(length + 1)) == NULL)
				break;
			if (tf_read(title, length, 1, tf) != 1)
			{
				free(title);
				break;
			}
			title[length] = '\0';
			return title;
		}
		else if (length != 0 && tf_seek(tf, length, SEEK_CUR) == -1)
			break;
	}
	return NULL;
}

typedef struct LastNoteInfo {
	int				on, off, note, velocity;
} LastNoteInfo;

#define NO_LAST_NOTE_INFO	-1
#define LASTNOTEINFO_HAS_DATA(lni)	((lni).on != NO_LAST_NOTE_INFO)
#define SEND_LASTNOTEINFO(lni, ch)				if (LASTNOTEINFO_HAS_DATA((lni)[ch])) SendLastNoteInfo(lni, ch);
#define SEND_AND_CLEAR_LASTNOTEINFO(lni, ch)	if (LASTNOTEINFO_HAS_DATA((lni)[ch])) { SendLastNoteInfo(lni, ch); (lni)[ch].on = NO_LAST_NOTE_INFO; }

inline void StoreLastNoteInfo(LastNoteInfo *info, int channel, int time, int duration, int note, int velocity)
{
	info[channel].on = time;
	info[channel].off = time + duration;
	info[channel].note = note;
	info[channel].velocity = velocity;
}

inline void SendLastNoteInfo(const LastNoteInfo *info, int channel)
{
	NOTE_BUF_EV_DEBUGSTR(channel, info[channel].on, note_name[info[channel].note % 12], info[channel].note / 12, info[channel].velocity, info[channel].off);
	MIDIEVENT(info[channel].on, ME_NOTEON, channel, info[channel].note, info[channel].velocity);
	MIDIEVENT(info[channel].off, ME_NOTEOFF, channel, info[channel].note, 0);
}

#define CHECK_AND_READ_FROM_FILE(ptr, readLen)		do {	\
						if ((length) < (readLen) || tf_read(ptr, readLen, 1, tf) != 1) {	\
							ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Odd track length.");	\
							return 1;	\
						}	\
						length -= readLen;	\
					} while(0)
static int read_mfi_track(int trackNo, int length, int mfiVersion, int noteType, int extStDLength, timidity_file *tf)
{
	uint8				data[5];
	int					i, pos, note, velocity, dataLength;
	uint8				instruments[MAX_CHANNELS];
	int					channelMap[4];
	LastNoteInfo		lastNotes[MAX_CHANNELS];
	
	readmidi_set_track(trackNo, 1);
	pos = 0;
	velocity = 0x7F;
	for(i = 0; i < 4; i++)
		channelMap[i] = (trackNo * 4) + i;
	dataLength = (noteType == 1) ? 4 : 3;
	data[3] = 0;	/* initialize for debugging purpose */
	for(i = 0; i < MAX_CHANNELS; i++)
		lastNotes[i].on = NO_LAST_NOTE_INFO;
	while (length > 0)
	{
		CHECK_AND_READ_FROM_FILE(data, dataLength);
		pos += data[0];
		if (data[1] != 0xFF)	/* note */
		{
			int					channel;
			
			channel = channelMap[data[1] >> 6];
			note = 0x48 - 0x1B + (data[1] & 0x3F);
			if (dataLength >= 4)
			{
				velocity = ((data[3] & 0xFC) >> 1) | (data[3] >> 7);	/* abcdefgh -> 0abcdefa */
				if (data[3] & 0x2)		/* sign */
					data[3] |= ~0x01;
				else
					data[3] &= 0x01;
				note += ((int8)data[3]) * 12;	/* octave shift */
			}
			if (LASTNOTEINFO_HAS_DATA(lastNotes[channel]))
			{
				if (lastNotes[channel].off <= pos
						|| note != lastNotes[channel].note)
				{
					SendLastNoteInfo(lastNotes, channel);
					StoreLastNoteInfo(lastNotes, channel, pos, data[2], note, velocity);
				}
				#if 0
				else if (note != lastNotes[channel].note)	/* possible slur */
				{
					if (lastNotes[channel].on == pos)	/* may be a chord */
					{
						SendLastNoteInfo(lastNotes, channel);
						StoreLastNoteInfo(lastNotes, channel, pos, data[2], note, velocity);
					}
					else	/* slur */
					{
						/* not implemented */
					}
				}
				#endif
				else	/* tie, what if the velocity isn't the same? :-) */
					lastNotes[channel].off = pos + data[2];
			}
			else
				StoreLastNoteInfo(lastNotes, channel, pos, data[2], note, velocity);
			NOTE_EVENT_DEBUGSTR(channel, note_name[note % 12], note / 12, velocity, data[2]);
		}
		else	/* controls */
		{
			if (dataLength == 3)
				CHECK_AND_READ_FROM_FILE(&data[3], 1);
			if ((data[2] & 0xF0) == 0xC0)	/* tempo */
			{
				int				timebase, tempo;
				
				timebase = data[2] & 0xF;
				if ((timebase & 0x7) == 0x7)
					ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Undefined tempo timebase.");
				else
				{
					if (timebase & 0x8)
						timebase = 15 << (timebase & 0x7);
					else
						timebase = 6 << (timebase & 0x7);
					tempo = 48 * (1000000 * 60 / data[3] / timebase);
					MIDIEVENT(pos, ME_TEMPO, tempo & 0xFF, (tempo >> 16) & 0xFF, (tempo >> 8) & 0xFF);
				}
			}
			else if ((data[2] & 0xF0) == 0xF0)	/* extended controls */
			{
				int				extLength, channel;
				uint8			extData[512];
				
				CHECK_AND_READ_FROM_FILE(&data[4], 1);
				extLength = (data[3] << 8) | data[4];
				if (extLength <= sizeof extData)
				{
					CHECK_AND_READ_FROM_FILE(extData, extLength);
					switch(data[2] & 0xF)
					{
						/* case 0x0:	/-* modify envelope (MFi1) */
						case 0x1:	/* vibrato (MFi1) */
							if (mfiVersion == 1 && extData[0] == 0x01 && extData[1] & 0x01)
							{
								channel = channelMap[extData[1] >> 5];
								MIDIEVENT(pos, ME_MODULATION_WHEEL, channel, (extData[2] & 0xC0) ? 64 : 0, 0)
							}
							break;
						/*case 0xF:	/-* system exclusive (MFi2 or MFi3) */
						default:
							EX_UNKNOWN_EXT_DATA_DEBUGSTR(extLength);
					}
				}
				else
				{
					if (tf_seek(tf, extLength, SEEK_CUR) == -1)
						return 1;
					EX_UNKNOWN_EXT_DATA_DEBUGSTR(extLength);
					length -= extLength;
				}
			}
			else
			{
				int				part, channel, value;
				#define GET_PART_AND_CHANNEL(p, c)		(p) = data[3] >> 6; (c) = channelMap[p]
				
				switch(data[2])
				{
					case 0xB0:	/* master volume */
						value = MAPBITS2(data[3] & 0x7F, 7, 16);
						MIDIEVENT(pos, ME_MASTER_VOLUME, 0, value & 0xFF, value >> 8);
						EX_NCDATA_DEBUGSTR1("Master Volume", "%d", value);
						break;
					case 0xBA:	/* set drum channel flag */
						channel = (data[3] >> 3) & 0xF;
						value = data[3] & 0x1;
						MIDIEVENT(pos, ME_DRUMPART, channel, value, 0);
						EX_DATA_DEBUGSTR1("Drum Flag", channel, "%d", value);
						break;
					case 0xD0:	/* music begin/end */
						/* ignored */
						break;
					/* case 0xDD:	/-* loop begin/end */
					case 0xDE:	/* nop */
						break;
					case 0xDF:	/* end-of-track */
						if (length != 0)
						{
							ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Premature end-of-track (%d)", length);
							length = 0;
						}
						break;
					case 0xE0:	/* program change */
						GET_PART_AND_CHANNEL(part, channel);
						SEND_AND_CLEAR_LASTNOTEINFO(lastNotes, channel);
						/*MIDIEVENT(pos, ME_DRUMPART, channel, 0, 0);*/
						instruments[channel] = (instruments[channel] & 0x40) | (data[3] & 0x3F);
						MIDIEVENT(pos, ME_PROGRAM, channel, instruments[channel], 0);
						EX_DATA_DEBUGSTR1("Program Change", channel, "%d", instruments[channel]);
						break;
					case 0xE1:	/* pre program change */
						GET_PART_AND_CHANNEL(part, channel);
						instruments[channel] = (data[3] & 0x1) << 6;
						EX_DATA_DEBUGSTR1("Pre Program Change", channel, "%d", instruments[channel]);
						break;
					case 0xE2:	/* volume */
						GET_PART_AND_CHANNEL(part, channel);
						value = MAPBITS(data[3] & 0x3F, 6, 7);
						MIDIEVENT(pos, ME_MAINVOLUME, channel, value, 0);
						EX_DATA_DEBUGSTR1("Volume", channel, "%d", value);
						break;
					case 0xE3:	/* pan */
						GET_PART_AND_CHANNEL(part, channel);
						value = MAPBITS(data[3] & 0x3F, 6, 7);
						MIDIEVENT(pos, ME_PAN, channel, value, 0);
						EX_DATA_DEBUGSTR1("Pan", channel, "%d", value);
						break;
					case 0xE4:	/* pitch bend (MFi3) */
						if (mfiVersion >= 3)
						{
							GET_PART_AND_CHANNEL(part, channel);
							value = MAPBITS2(data[3] & 0x3F, 6, 14);
							MIDIEVENT(pos, ME_PITCHWHEEL, channel, value & 0x7F, value >> 7);
							EX_DATA_DEBUGSTR1("Pitch Bend", channel, "%d", value);
						}
						break;
					case 0xE5:	/* map part to channel */
						part = data[3] >> 6;
						SEND_AND_CLEAR_LASTNOTEINFO(lastNotes, channelMap[part]);
						channelMap[part] = data[3] & 0xF;
						SEND_AND_CLEAR_LASTNOTEINFO(lastNotes, channelMap[part]);
						EX_DATA_DEBUGSTR1("Map Channel", part, "%d", channelMap[part]);
						break;
					case 0xE6:	/* expression */
						GET_PART_AND_CHANNEL(part, channel);
						value = data[3] & 0x3F;
						if (value & 0x20)
							value = 64 - (((value ^ 0x3F) + 1) << 1);
						else if (value & 0x10)
							value = 64 + 1 + (value << 1);
						else
							value = 64 + (value << 1);
						MIDIEVENT(pos, ME_EXPRESSION, channel, value, 0);
						EX_DATA_DEBUGSTR1("Expression", channel, "%d", value);
						break;
					case 0xE7:	/* pitch bend range (MFi3) */
						if (mfiVersion >= 3)
						{
							GET_PART_AND_CHANNEL(part, channel);
							value = data[3] & 0x3F;
							MIDIEVENT(pos, ME_RPN_MSB, channel, 0, 0);
							MIDIEVENT(pos, ME_RPN_LSB, channel, 0, 0);
							MIDIEVENT(pos, ME_DATA_ENTRY_MSB, channel, value, 0);
							EX_DATA_DEBUGSTR1("Pitch Bend Range", channel, "%d", value);
						}
						break;
					case 0xEA:	/* vibrato (MFi3) */
						if (mfiVersion >= 3)
						{
							GET_PART_AND_CHANNEL(part, channel);
							value = MAPBITS(data[3] & 0x3F, 6, 7);
							MIDIEVENT(pos, ME_MODULATION_WHEEL, channel, value, 0);
							EX_DATA_DEBUGSTR1("Vibrato", channel, "%d", value);
						}
						break;
					default:
						EX_UNKNOWN_DATA_DEBUGSTR();
				}
			}
		}
	}
	for(i = 0; i < MAX_CHANNELS; i++)
	{
		SEND_LASTNOTEINFO(lastNotes, i);
	}
	return 0;
}

static int tf_read_beint16(int *value, timidity_file *tf)
{
	uint16				value_;
	
	if (tf_read(&value_, 2, 1, tf) != 1)
		return 0;
	*value = BE_SHORT(value_);
	return 1;
}

static int tf_read_beint32(int *value, timidity_file *tf)
{
	uint32				value_;
	
	if (tf_read(&value_, 4, 1, tf) != 1)
		return 0;
	*value = BE_LONG(value_);
	return 1;
}

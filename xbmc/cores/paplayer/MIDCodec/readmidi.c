/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdio.h>
#include <stdlib.h>
//#include <errno.h>
#include <string.h>

#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"

int32 quietchannels=0;

/* to avoid some unnecessary parameter passing */
static MidiEventList *evlist;
static int32 event_count;
static FILE *fp;
static int32 at;

/* These would both fit into 32 bits, but they are often added in
   large multiples, so it's simpler to have two roomy ints */
static int32 sample_increment, sample_correction; /*samples per MIDI delta-t*/

/* Computes how many (fractional) samples one MIDI delta-time unit contains */
static void compute_sample_increment(int32 tempo, int32 divisions)
{
  double a;
  a = (double) (tempo) * (double) (play_mode->rate) * (65536.0/1000000.0) /
    (double)(divisions);

  sample_correction = (int32)(a) & 0xFFFF;
  sample_increment = (int32)(a) >> 16;

  ctl->cmsg(CMSG_INFO, VERB_DEBUG, "Samples per delta-t: %d (correction %d)",
       sample_increment, sample_correction);
}

/* Read variable-length number (7 bits per byte, MSB first) */
static int32 getvl(void)
{
  int32 l=0;
  uint8 c;
  for (;;)
    {
      fread(&c,1,1,fp);
      l += (c & 0x7f);
      if (!(c & 0x80)) return l;
      l<<=7;
    }
}

/* Print a string from the file, followed by a newline. Any non-ASCII
   or unprintable characters will be converted to periods. */
static int dumpstring(int32 len, char *label)
{
  signed char *s=safe_malloc(len+1);
  if (len != (int32)fread(s, 1, len, fp))
    {
      free(s);
      return -1;
    }
  s[len]='\0';
  while (len--)
    {
      if (s[len]<32)
	s[len]='.';
    }
  ctl->cmsg(CMSG_TEXT, VERB_VERBOSE, "%s%s", label, s);
  free(s);
  return 0;
}

#define MIDIEVENT(at,t,ch,pa,pb) \
  new=safe_malloc(sizeof(MidiEventList)); \
  new->event.time=at; new->event.type=t; new->event.channel=ch; \
  new->event.a=pa; new->event.b=pb; new->next=0;\
  return new;

#define MAGIC_EOT ((MidiEventList *)(-1))

/* Read a MIDI event, returning a freshly allocated element that can
   be linked to the event list */
static MidiEventList *read_midi_event(void)
{
  static uint8 laststatus, lastchan;
  static uint8 nrpn=0, rpn_msb[16], rpn_lsb[16]; /* one per channel */
  uint8 me, type, a,b,c;
  int32 len;
  MidiEventList *new;

  for (;;)
    {
      at+=getvl();
      if (fread(&me,1,1,fp)!=1)
	{
	  //ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: read_midi_event: %s", 
	  //     current_filename, strerror(errno));
	  return 0;
	}
      
      if(me==0xF0 || me == 0xF7) /* SysEx event */
	{
	  len=getvl();
	  skip(fp, len);
	}
      else if(me==0xFF) /* Meta event */
	{
	  fread(&type,1,1,fp);
	  len=getvl();
	  if (type>0 && type<16)
	    {
	      static char *label[]={
		"Text event: ", "Text: ", "Copyright: ", "Track name: ",
		"Instrument: ", "Lyric: ", "Marker: ", "Cue point: "};
	      dumpstring(len, label[(type>7) ? 0 : type]);
	    }
	  else
	    switch(type)
	      {
	      case 0x2F: /* End of Track */
		return MAGIC_EOT;

	      case 0x51: /* Tempo */
		fread(&a,1,1,fp); fread(&b,1,1,fp); fread(&c,1,1,fp);
		MIDIEVENT(at, ME_TEMPO, c, a, b);
		
	      default:
		ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
		     "(Meta event type 0x%02x, length %ld)", type, len);
		skip(fp, len);
		break;
	      }
	}
      else
	{
	  a=me;
	  if (a & 0x80) /* status byte */
	    {
	      lastchan=a & 0x0F;
	      laststatus=(a>>4) & 0x07;
	      fread(&a, 1,1, fp);
	      a &= 0x7F;
	    }
	  switch(laststatus)
	    {
	    case 0: /* Note off */
	      fread(&b, 1,1, fp);
	      b &= 0x7F;
	      MIDIEVENT(at, ME_NOTEOFF, lastchan, a,b);

	    case 1: /* Note on */
	      fread(&b, 1,1, fp);
	      b &= 0x7F;
	      MIDIEVENT(at, ME_NOTEON, lastchan, a,b);

	    case 2: /* Key Pressure */
	      fread(&b, 1,1, fp);
	      b &= 0x7F;
	      MIDIEVENT(at, ME_KEYPRESSURE, lastchan, a, b);

	    case 3: /* Control change */
	      fread(&b, 1,1, fp);
	      b &= 0x7F;
	      {
		int control=255;
		switch(a)
		  {
		  case 7: control=ME_MAINVOLUME; break;
		  case 10: control=ME_PAN; break;
		  case 11: control=ME_EXPRESSION; break;
		  case 64: control=ME_SUSTAIN; break;
		  case 120: control=ME_ALL_SOUNDS_OFF; break;
		  case 121: control=ME_RESET_CONTROLLERS; break;
		  case 123: control=ME_ALL_NOTES_OFF; break;

		    /* These should be the SCC-1 tone bank switch
		       commands. I don't know why there are two, or
		       why the latter only allows switching to bank 0.
		       Also, some MIDI files use 0 as some sort of
		       continuous controller. This will cause lots of
		       warnings about undefined tone banks. */
		  case 0: control=ME_TONE_BANK; break;
		  case 32: 
		    if (b!=0)
		      ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
				"(Strange: tone bank change 0x20%02x)", b);
		    else
		      control=ME_TONE_BANK;
		    break;

		  case 100: nrpn=0; rpn_msb[lastchan]=b; break;
		  case 101: nrpn=0; rpn_lsb[lastchan]=b; break;
		  case 99: nrpn=1; rpn_msb[lastchan]=b; break;
		  case 98: nrpn=1; rpn_lsb[lastchan]=b; break;
		    
		  case 6:
		    if (nrpn)
		      {
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
				  "(Data entry (MSB) for NRPN %02x,%02x: %ld)",
				  rpn_msb[lastchan], rpn_lsb[lastchan],
				  b);
			break;
		      }
		    
		    switch((rpn_msb[lastchan]<<8) | rpn_lsb[lastchan])
		      {
		      case 0x0000: /* Pitch bend sensitivity */
			control=ME_PITCH_SENS;
			break;

		      case 0x7F7F: /* RPN reset */
			/* reset pitch bend sensitivity to 2 */
			MIDIEVENT(at, ME_PITCH_SENS, lastchan, 2, 0);

		      default:
			ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
				  "(Data entry (MSB) for RPN %02x,%02x: %ld)",
				  rpn_msb[lastchan], rpn_lsb[lastchan],
				  b);
			break;
		      }
		    break;
		    
		  default:
		    ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
			      "(Control %d: %d)", a, b);
		    break;
		  }
		if (control != 255)
		  { 
		    MIDIEVENT(at, control, lastchan, b, 0); 
		  }
	      }
	      break;

	    case 4: /* Program change */
	      a &= 0x7f;
	      MIDIEVENT(at, ME_PROGRAM, lastchan, a, 0);

	    case 5: /* Channel pressure - NOT IMPLEMENTED */
	      break;

	    case 6: /* Pitch wheel */
	      fread(&b, 1,1, fp);
	      b &= 0x7F;
	      MIDIEVENT(at, ME_PITCHWHEEL, lastchan, a, b);

	    default: 
	      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
		   "*** Can't happen: status 0x%02X, channel 0x%02X",
		   laststatus, lastchan);
	      break;
	    }
	}
    }
  
  return new;
}

#undef MIDIEVENT

/* Read a midi track into the linked list, either merging with any previous
   tracks or appending to them. */
static int read_track(int append)
{
  MidiEventList *meep;
  MidiEventList *next, *new;
  int32 len;
  char tmp[4];

  meep=evlist;
  if (append && meep)
    {
      /* find the last event in the list */
      for (; meep->next; meep=meep->next)
	;
      at=meep->event.time;
    }
  else
    at=0;

  /* Check the formalities */
  
  if ((fread(tmp,1,4,fp) != 4) || (fread(&len,4,1,fp) != 1))
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	   "%s: Can't read track header.", current_filename);
      return -1;
    }
  len=BE_LONG(len);
  if (memcmp(tmp, "MTrk", 4))
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	   "%s: Corrupt MIDI file.", current_filename);
      return -2;
    }

  for (;;)
    {
      if (!(new=read_midi_event())) /* Some kind of error  */
	return -2;

      if (new==MAGIC_EOT) /* End-of-track Hack. */
	{
	  return 0;
	}

      next=meep->next;
      while (next && (next->event.time < new->event.time))
	{
	  meep=next;
	  next=meep->next;
	}
	  
      new->next=next;
      meep->next=new;

      event_count++; /* Count the event. (About one?) */
      meep=new;
    }
}

/* Free the linked event list from memory. */
static void free_midi_list(void)
{
  MidiEventList *meep, *next;
  if (!(meep=evlist)) return;
  while (meep)
    {
      next=meep->next;
      free(meep);
      meep=next;
    }
  evlist=0;
}

/* Allocate an array of MidiEvents and fill it from the linked list of
   events, marking used instruments for loading. Convert event times to
   samples: handle tempo changes. Strip unnecessary events from the list.
   Free the linked list. */
static MidiEvent *groom_list(int32 divisions,int32 *eventsp,int32 *samplesp)
{
  MidiEvent *groomed_list, *lp;
  MidiEventList *meep;
  int32 i, our_event_count, tempo, skip_this_event, new_value;
  int32 sample_cum, samples_to_do, at, st, dt, counting_time;

  int current_bank[16], current_set[16], current_program[16]; 
  /* Or should each bank have its own current program? */

  for (i=0; i<16; i++)
    {
      current_bank[i]=0;
      current_set[i]=0;
      current_program[i]=default_program;
    }

  tempo=500000;
  compute_sample_increment(tempo, divisions);

  /* This may allocate a bit more than we need */
  groomed_list=lp=safe_malloc(sizeof(MidiEvent) * (event_count+1));
  meep=evlist;

  our_event_count=0;
  st=at=sample_cum=0;
  counting_time=2; /* We strip any silence before the first NOTE ON. */

  for (i=0; i<event_count; i++)
    {
      skip_this_event=0;
      ctl->cmsg(CMSG_INFO, VERB_DEBUG_SILLY,
		"%6d: ch %2d: event %d (%d,%d)",
		meep->event.time, meep->event.channel + 1,
		meep->event.type, meep->event.a, meep->event.b);

      if (meep->event.type==ME_TEMPO)
	{
	  tempo=
	    meep->event.channel + meep->event.b * 256 + meep->event.a * 65536;
	  compute_sample_increment(tempo, divisions);
	  skip_this_event=1;
	}
      else if ((quietchannels & (1<<meep->event.channel)))
	skip_this_event=1;
      else switch (meep->event.type)
	{
	case ME_PROGRAM:
	  if (ISDRUMCHANNEL(meep->event.channel))
	    {
	      if (drumset[meep->event.a]) /* Is this a defined drumset? */
		new_value=meep->event.a;
	      else
		{
		  ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		       "Drum set %d is undefined", meep->event.a);
		  new_value=meep->event.a=0;
		}
	      if (current_set[meep->event.channel] != new_value)
		current_set[meep->event.channel]=new_value;
	      else 
		skip_this_event=1;
	    }
	  else
	    {
	      new_value=meep->event.a;
	      if ((current_program[meep->event.channel] != SPECIAL_PROGRAM)
		  && (current_program[meep->event.channel] != new_value))
		current_program[meep->event.channel] = new_value;
	      else
		skip_this_event=1;
	    }
	  break;

	case ME_NOTEON:
	  if (counting_time)
	    counting_time=1;
	  if (ISDRUMCHANNEL(meep->event.channel))
	    {
	      /* Mark this instrument to be loaded */
	      if (!(drumset[current_set[meep->event.channel]]
		    ->tone[meep->event.a].instrument))
		drumset[current_set[meep->event.channel]]
		  ->tone[meep->event.a].instrument=
		    MAGIC_LOAD_INSTRUMENT;
	    }
	  else
	    {
	      if (current_program[meep->event.channel]==SPECIAL_PROGRAM)
		break;
	      /* Mark this instrument to be loaded */
	      if (!(tonebank[current_bank[meep->event.channel]]
		    ->tone[current_program[meep->event.channel]].instrument))
		tonebank[current_bank[meep->event.channel]]
		  ->tone[current_program[meep->event.channel]].instrument=
		    MAGIC_LOAD_INSTRUMENT;
	    }
	  break;

	case ME_TONE_BANK:
	  if (ISDRUMCHANNEL(meep->event.channel))
	    {
	      skip_this_event=1;
	      break;
	    }
	  if (tonebank[meep->event.a]) /* Is this a defined tone bank? */
	    new_value=meep->event.a;
	  else 
	    {
	      ctl->cmsg(CMSG_WARNING, VERB_VERBOSE,
		   "Tone bank %d is undefined", meep->event.a);
	      new_value=meep->event.a=0;
	    }
	  if (current_bank[meep->event.channel]!=new_value)
	    current_bank[meep->event.channel]=new_value;
	  else
	    skip_this_event=1;
	  break;
	}

      /* Recompute time in samples*/
      if ((dt=meep->event.time - at) && !counting_time)
	{
	  samples_to_do=sample_increment * dt;
	  sample_cum += sample_correction * dt;
	  if (sample_cum & 0xFFFF0000)
	    {
	      samples_to_do += ((sample_cum >> 16) & 0xFFFF);
	      sample_cum &= 0x0000FFFF;
	    }
	  st += samples_to_do;
	}
      else if (counting_time==1) counting_time=0;
      if (!skip_this_event)
	{
	  /* Add the event to the list */
	  *lp=meep->event;
	  lp->time=st;
	  lp++;
	  our_event_count++;
	}
      at=meep->event.time;
      meep=meep->next;
    }
  /* Add an End-of-Track event */
  lp->time=st;
  lp->type=ME_EOT;
  our_event_count++;
  free_midi_list();
  
  *eventsp=our_event_count;
  *samplesp=st;
  return groomed_list;
}

MidiEvent *read_midi_file(FILE *mfp, int32 *count, int32 *sp)
{
  int32 len, divisions;
  int16 format, tracks, divisions_tmp;
  int i;
  char tmp[4];

  fp=mfp;
  event_count=0;
  at=0;
  evlist=0;

  if ((fread(tmp,1,4,fp) != 4) || (fread(&len,4,1,fp) != 1))
    {
      if (ferror(fp))
	{
	  //ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: %s", current_filename, 
	   //    strerror(errno));
	}
      else
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
	     "%s: Not a MIDI file!", current_filename);
      return 0;
    }
  len=BE_LONG(len);
  if (memcmp(tmp, "MThd", 4) || len < 6)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL,
	   "%s: Not a MIDI file!", current_filename);
      return 0;
    }

  fread(&format, 2, 1, fp);
  fread(&tracks, 2, 1, fp);
  fread(&divisions_tmp, 2, 1, fp);
  format=BE_SHORT(format);
  tracks=BE_SHORT(tracks);
  divisions_tmp=BE_SHORT(divisions_tmp);

  if (divisions_tmp<0)
    {
      /* SMPTE time -- totally untested. Got a MIDI file that uses this? */
      divisions=
	(int32)(-(divisions_tmp/256)) * (int32)(divisions_tmp & 0xFF);
    }
  else divisions=(int32)(divisions_tmp);

  if (len > 6)
    {
      ctl->cmsg(CMSG_WARNING, VERB_NORMAL, 
	   "%s: MIDI file header size %ld bytes", 
	   current_filename, len);
      skip(fp, len-6); /* skip the excess */
    }
  if (format<0 || format >2)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
	   "%s: Unknown MIDI file format %d", current_filename, format);
      return 0;
    }
  ctl->cmsg(CMSG_INFO, VERB_VERBOSE, 
       "Format: %d  Tracks: %d  Divisions: %d", format, tracks, divisions);

  /* Put a do-nothing event first in the list for easier processing */
  evlist=safe_malloc(sizeof(MidiEventList));
  evlist->event.time=0;
  evlist->event.type=ME_NONE;
  evlist->next=0;
  event_count++;

  switch(format)
    {
    case 0:
      if (read_track(0))
	{
	  free_midi_list();
	  return 0;
	}
      break;

    case 1:
      for (i=0; i<tracks; i++)
	if (read_track(0))
	  {
	    free_midi_list();
	    return 0;
	  }
      break;

    case 2: /* We simply play the tracks sequentially */
      for (i=0; i<tracks; i++)
	if (read_track(1))
	  {
	    free_midi_list();
	    return 0;
	  }
      break;
    }
  return groom_list(divisions, count, sp);
}

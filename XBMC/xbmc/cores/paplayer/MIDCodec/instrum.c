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

   instrum.c 
   
   Code to load and unload GUS-compatible instrument patches.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "resample.h"
#include "tables.h"
#include "filter.h"

/* Some functions get aggravated if not even the standard banks are 
   available. */
static ToneBank standard_tonebank, standard_drumset;
ToneBank 
  *tonebank[128]={&standard_tonebank},
  *drumset[128]={&standard_drumset};

/* This is a special instrument, used for all melodic programs */
Instrument *default_instrument=0;

/* This is only used for tracks that don't specify a program */
int default_program=DEFAULT_PROGRAM;

int antialiasing_allowed=0;
#ifdef FAST_DECAY
int fast_decay=1;
#else
int fast_decay=0;
#endif

static void free_instrument(Instrument *ip)
{
  Sample *sp;
  int i;
  if (!ip) return;
  for (i=0; i<ip->samples; i++)
    {
      sp=&(ip->sample[i]);
      free(sp->data);
    }
  free(ip->sample);
  free(ip);
}

static void free_bank(int dr, int b)
{
  int i;
  ToneBank *bank=((dr) ? drumset[b] : tonebank[b]);
  for (i=0; i<128; i++)
    if (bank->tone[i].instrument)
      {
	/* Not that this could ever happen, of course */
	if (bank->tone[i].instrument != MAGIC_LOAD_INSTRUMENT)
	  free_instrument(bank->tone[i].instrument);
	bank->tone[i].instrument=0;
      }
}

static int32 convert_envelope_rate(uint8 rate)
{
  int32 r;

  r=3-((rate>>6) & 0x3);
  r*=3;
  r = (int32)(rate & 0x3f) << r; /* 6.9 fixed point */

  /* 15.15 fixed point. */
  return (((r * 44100) / play_mode->rate) * control_ratio) 
    << ((fast_decay) ? 10 : 9);
}

static int32 convert_envelope_offset(uint8 offset)
{
  /* This is not too good... Can anyone tell me what these values mean?
     Are they GUS-style "exponential" volumes? And what does that mean? */

  /* 15.15 fixed point */
  return offset << (7+15);
}

static int32 convert_tremolo_sweep(uint8 sweep)
{
  if (!sweep)
    return 0;

  return
    ((control_ratio * SWEEP_TUNING) << SWEEP_SHIFT) /
      (play_mode->rate * sweep);
}

static int32 convert_vibrato_sweep(uint8 sweep, int32 vib_control_ratio)
{
  if (!sweep)
    return 0;

  return
    (int32) (FSCALE((double) (vib_control_ratio) * SWEEP_TUNING, SWEEP_SHIFT)
	     / (double)(play_mode->rate * sweep));

  /* this was overflowing with seashore.pat

      ((vib_control_ratio * SWEEP_TUNING) << SWEEP_SHIFT) /
      (play_mode->rate * sweep); */
}

static int32 convert_tremolo_rate(uint8 rate)
{
  return
    ((SINE_CYCLE_LENGTH * control_ratio * rate) << RATE_SHIFT) /
      (TREMOLO_RATE_TUNING * play_mode->rate);
}

static int32 convert_vibrato_rate(uint8 rate)
{
  /* Return a suitable vibrato_control_ratio value */
  return
    (VIBRATO_RATE_TUNING * play_mode->rate) / 
      (rate * 2 * VIBRATO_SAMPLE_INCREMENTS);
}

static void reverse_data(int16 *sp, int32 ls, int32 le)
{
  int16 s, *ep=sp+le;
  sp+=ls;
  le-=ls;
  le/=2;
  while (le--)
    {
      s=*sp;
      *sp++=*ep;
      *ep--=s;
    }
}

/* 
   If panning or note_to_use != -1, it will be used for all samples,
   instead of the sample-specific values in the instrument file. 

   For note_to_use, any value <0 or >127 will be forced to 0.
 
   For other parameters, 1 means yes, 0 means no, other values are
   undefined.

   TODO: do reverse loops right */
static Instrument *load_instrument(char *name, int percussion,
				   int panning, int amp, int note_to_use,
				   int strip_loop, int strip_envelope,
				   int strip_tail)
{
  Instrument *ip;
  Sample *sp;
  FILE *fp;
  uint8 tmp[1024];
  int i,j,noluck=0;
#ifdef PATCH_EXT_LIST
  static char *patch_ext[] = PATCH_EXT_LIST;
#endif

  if (!name) return 0;
    
  /* Open patch file */
  if ((fp=open_file(name, 1, OF_NORMAL)) == NULL)
    {
      noluck=1;
#ifdef PATCH_EXT_LIST
      /* Try with various extensions */
      for (i=0; patch_ext[i]; i++)
	{
	  if (strlen(name)+strlen(patch_ext[i])<1024)
	    {
        char path[1024];
        strcpy(path,DEFAULT_PATH);
		#ifdef __linux__
		strcat(path,"/");
		#else
        strcat(path,"\\");
		#endif
	    strcat(path, name);
	    strcat(path, patch_ext[i]);
        if ((fp=open_file(path, 1, OF_NORMAL)) != NULL)
		{
      noluck=0;
		  break;
		}
	    }
	}
#endif
    }
  
  if (noluck)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
		"Instrument `%s' can't be found.", name);
      return 0;
    }
      
  ctl->cmsg(CMSG_INFO, VERB_NOISY, "Loading instrument %s", current_filename);
  
  /* Read some headers and do cursory sanity checks. There are loads
     of magic offsets. This could be rewritten... */

  if ((239 != fread(tmp, 1, 239, fp)) ||
      (memcmp(tmp, "GF1PATCH110\0ID#000002", 22) &&
       memcmp(tmp, "GF1PATCH100\0ID#000002", 22))) /* don't know what the
						      differences are */
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "%s: not an instrument", name);
      return 0;
    }
  
  if (tmp[82] != 1 && tmp[82] != 0) /* instruments. To some patch makers, 
				       0 means 1 */
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
	   "Can't handle patches with %d instruments", tmp[82]);
      return 0;
    }

  if (tmp[151] != 1 && tmp[151] != 0) /* layers. What's a layer? */
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
	   "Can't handle instruments with %d layers", tmp[151]);
      return 0;
    }
  
  ip=safe_malloc(sizeof(Instrument));
  ip->samples = tmp[198];
  ip->sample = safe_malloc(sizeof(Sample) * ip->samples);
  for (i=0; i<ip->samples; i++)
    {

      uint8 fractions;
      int32 tmplong;
      uint16 tmpshort;
      uint8 tmpchar;

#define READ_CHAR(thing) \
      if (1 != fread(&tmpchar, 1, 1, fp)) goto fail; \
      thing = tmpchar;
#define READ_SHORT(thing) \
      if (1 != fread(&tmpshort, 2, 1, fp)) goto fail; \
      thing = LE_SHORT(tmpshort);
#define READ_LONG(thing) \
      if (1 != fread(&tmplong, 4, 1, fp)) goto fail; \
      thing = LE_LONG(tmplong);

      skip(fp, 7); /* Skip the wave name */

      if (1 != fread(&fractions, 1, 1, fp))
	{
	fail:
	  ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Error reading sample %d", i);
	  for (j=0; j<i; j++)
	    free(ip->sample[j].data);
	  free(ip->sample);
	  free(ip);
	  return 0;
	}

      sp=&(ip->sample[i]);
      
      READ_LONG(sp->data_length);
      READ_LONG(sp->loop_start);
      READ_LONG(sp->loop_end);
      READ_SHORT(sp->sample_rate);
      READ_LONG(sp->low_freq);
      READ_LONG(sp->high_freq);
      READ_LONG(sp->root_freq);
      skip(fp, 2); /* Why have a "root frequency" and then "tuning"?? */
      
      READ_CHAR(tmp[0]);

      if (panning==-1)
	sp->panning = (tmp[0] * 8 + 4) & 0x7f;
      else
	sp->panning=(uint8)(panning & 0x7F);

      /* envelope, tremolo, and vibrato */
      if (18 != fread(tmp, 1, 18, fp)) goto fail; 

      if (!tmp[13] || !tmp[14])
	{
	  sp->tremolo_sweep_increment=
	    sp->tremolo_phase_increment=sp->tremolo_depth=0;
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG, " * no tremolo");
	}
      else
	{
	  sp->tremolo_sweep_increment=convert_tremolo_sweep(tmp[12]);
	  sp->tremolo_phase_increment=convert_tremolo_rate(tmp[13]);
	  sp->tremolo_depth=tmp[14];
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG,
	       " * tremolo: sweep %d, phase %d, depth %d",
	       sp->tremolo_sweep_increment, sp->tremolo_phase_increment,
	       sp->tremolo_depth);
	}

      if (!tmp[16] || !tmp[17])
	{
	  sp->vibrato_sweep_increment=
	    sp->vibrato_control_ratio=sp->vibrato_depth=0;
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG, " * no vibrato");
	}
      else
	{
	  sp->vibrato_control_ratio=convert_vibrato_rate(tmp[16]);
	  sp->vibrato_sweep_increment=
	    convert_vibrato_sweep(tmp[15], sp->vibrato_control_ratio);
	  sp->vibrato_depth=tmp[17];
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG,
	       " * vibrato: sweep %d, ctl %d, depth %d",
	       sp->vibrato_sweep_increment, sp->vibrato_control_ratio,
	       sp->vibrato_depth);

	}

      READ_CHAR(sp->modes);

      skip(fp, 40); /* skip the useless scale frequency, scale factor
		       (what's it mean?), and reserved space */

      /* Mark this as a fixed-pitch instrument if such a deed is desired. */
      if (note_to_use!=-1)
	sp->note_to_use=(uint8)(note_to_use);
      else
	sp->note_to_use=0;
      
      /* seashore.pat in the Midia patch set has no Sustain. I don't
         understand why, and fixing it by adding the Sustain flag to
         all looped patches probably breaks something else. We do it
         anyway. */
	 
      if (sp->modes & MODES_LOOPING) 
	sp->modes |= MODES_SUSTAIN;

      /* Strip any loops and envelopes we're permitted to */
      if ((strip_loop==1) && 
	  (sp->modes & (MODES_SUSTAIN | MODES_LOOPING | 
			MODES_PINGPONG | MODES_REVERSE)))
	{
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG, " - Removing loop and/or sustain");
	  sp->modes &=~(MODES_SUSTAIN | MODES_LOOPING | 
			MODES_PINGPONG | MODES_REVERSE);
	}

      if (strip_envelope==1)
	{
	  if (sp->modes & MODES_ENVELOPE)
	    ctl->cmsg(CMSG_INFO, VERB_DEBUG, " - Removing envelope");
	  sp->modes &= ~MODES_ENVELOPE;
	}
      else if (strip_envelope != 0)
	{
	  /* Have to make a guess. */
	  if (!(sp->modes & (MODES_LOOPING | MODES_PINGPONG | MODES_REVERSE)))
	    {
	      /* No loop? Then what's there to sustain? No envelope needed
		 either... */
	      sp->modes &= ~(MODES_SUSTAIN|MODES_ENVELOPE);
	      ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
			" - No loop, removing sustain and envelope");
	    }
	  else if (!memcmp(tmp, "??????", 6) || tmp[11] >= 100) 
	    {
	      /* Envelope rates all maxed out? Envelope end at a high "offset"?
		 That's a weird envelope. Take it out. */
	      sp->modes &= ~MODES_ENVELOPE;
	      ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
			" - Weirdness, removing envelope");
	    }
	  else if (!(sp->modes & MODES_SUSTAIN))
	    {
	      /* No sustain? Then no envelope.  I don't know if this is
		 justified, but patches without sustain usually don't need the
		 envelope either... at least the Gravis ones. They're mostly
		 drums.  I think. */
	      sp->modes &= ~MODES_ENVELOPE;
	      ctl->cmsg(CMSG_INFO, VERB_DEBUG, 
			" - No sustain, removing envelope");
	    }
	}

      for (j=0; j<6; j++)
	{
	  sp->envelope_rate[j]=
	    convert_envelope_rate(tmp[j]);
	  sp->envelope_offset[j]= 
	    convert_envelope_offset(tmp[6+j]);
	}

      /* Then read the sample data */
      sp->data = safe_malloc(sp->data_length);
      if (1 != fread(sp->data, sp->data_length, 1, fp))
	goto fail;
      
      if (!(sp->modes & MODES_16BIT)) /* convert to 16-bit data */
	{
	  int32 i=sp->data_length;
	  uint8 *cp=(uint8 *)(sp->data);
	  uint16 *tmp,*new;
	  tmp=new=safe_malloc(sp->data_length*2);
	  while (i--)
	    *tmp++ = (uint16)(*cp++) << 8;
	  cp=(uint8 *)(sp->data);
	  sp->data = (sample_t *)new;
	  free(cp);
	  sp->data_length *= 2;
	  sp->loop_start *= 2;
	  sp->loop_end *= 2;
	}
#ifndef LITTLE_ENDIAN
      else
	/* convert to machine byte order */
	{
	  int32 i=sp->data_length/2;
	  int16 *tmp=(int16 *)sp->data,s;
	  while (i--)
	    { 
	      s=LE_SHORT(*tmp);
	      *tmp++=s;
	    }
	}
#endif
      
      if (sp->modes & MODES_UNSIGNED) /* convert to signed data */
	{
	  int32 i=sp->data_length/2;
	  int16 *tmp=(int16 *)sp->data;
	  while (i--)
	    *tmp++ ^= 0x8000;
	}

      /* Reverse reverse loops and pass them off as normal loops */
      if (sp->modes & MODES_REVERSE)
	{
	  int32 t;
	  /* The GUS apparently plays reverse loops by reversing the
	     whole sample. We do the same because the GUS does not SUCK. */

	  ctl->cmsg(CMSG_WARNING, VERB_NORMAL, "Reverse loop in %s", name);
	  reverse_data((int16 *)sp->data, 0, sp->data_length/2);

	  t=sp->loop_start;
	  sp->loop_start=sp->data_length - sp->loop_end;
	  sp->loop_end=sp->data_length - t;

	  sp->modes &= ~MODES_REVERSE;
	  sp->modes |= MODES_LOOPING; /* just in case */
	}

      /* If necessary do some anti-aliasing filtering  */

      if (antialiasing_allowed)
	  antialiasing(sp,play_mode->rate);

#ifdef ADJUST_SAMPLE_VOLUMES
      if (amp!=-1)
	sp->volume=(float)((amp) / 100.0);
      else
	{
	  /* Try to determine a volume scaling factor for the sample.
	     This is a very crude adjustment, but things sound more
	     balanced with it. Still, this should be a runtime option. */
	  int32 i=sp->data_length/2;
	  int16 maxamp=0,a;
	  int16 *tmp=(int16 *)sp->data;
	  while (i--)
	    {
	      a=*tmp++;
	      if (a<0) a=-a;
	      if (a>maxamp)
		maxamp=a;
	    }
	  sp->volume=(float)(32768.0 / maxamp);
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG, " * volume comp: %f", sp->volume);
	}
#else
      if (amp!=-1)
	sp->volume=(double)(amp) / 100.0;
      else
	sp->volume=1.0;
#endif

      sp->data_length /= 2; /* These are in bytes. Convert into samples. */
      sp->loop_start /= 2;
      sp->loop_end /= 2;

      /* Then fractional samples */
      sp->data_length <<= FRACTION_BITS;
      sp->loop_start <<= FRACTION_BITS;
      sp->loop_end <<= FRACTION_BITS;

      /* Adjust for fractional loop points. This is a guess. Does anyone
	 know what "fractions" really stands for? */
      sp->loop_start |=
	(fractions & 0x0F) << (FRACTION_BITS-4);
      sp->loop_end |=
	((fractions>>4) & 0x0F) << (FRACTION_BITS-4);

      /* If this instrument will always be played on the same note,
	 and it's not looped, we can resample it now. */
      if (sp->note_to_use && !(sp->modes & MODES_LOOPING))
	pre_resample(sp);

#ifdef LOOKUP_HACK
      /* Squash the 16-bit data into 8 bits. */
      {
	uint8 *gulp,*ulp;
	int16 *swp;
	int l=sp->data_length >> FRACTION_BITS;
	gulp=ulp=safe_malloc(l+1);
	swp=(int16 *)sp->data;
	while(l--)
	  *ulp++ = (*swp++ >> 8) & 0xFF;
	free(sp->data);
	sp->data=(sample_t *)gulp;
      }
#endif
      
      if (strip_tail==1)
	{
	  /* Let's not really, just say we did. */
	  ctl->cmsg(CMSG_INFO, VERB_DEBUG, " - Stripping tail");
	  sp->data_length = sp->loop_end;
	}
    }

  close_file(fp);
  return ip;
}

static int fill_bank(int dr, int b)
{
  int i, errors=0;
  ToneBank *bank=((dr) ? drumset[b] : tonebank[b]);
  if (!bank)
    {
      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
	   "Huh. Tried to load instruments in non-existent %s %d",
	   (dr) ? "drumset" : "tone bank", b);
      return 0;
    }
  for (i=0; i<128; i++)
    {
      if (bank->tone[i].instrument==MAGIC_LOAD_INSTRUMENT)
	{
	  if (!(bank->tone[i].name))
	    {
	      ctl->cmsg(CMSG_WARNING, (b!=0) ? VERB_VERBOSE : VERB_NORMAL,
		   "No instrument mapped to %s %d, program %d%s",
		   (dr)? "drum set" : "tone bank", b, i, 
		   (b!=0) ? "" : " - this instrument will not be heard");
	      if (b!=0)
		{
		  /* Mark the corresponding instrument in the default
		     bank / drumset for loading (if it isn't already) */
		  if (!dr)
		    {
		      if (!(standard_tonebank.tone[i].instrument))
			standard_tonebank.tone[i].instrument=
			  MAGIC_LOAD_INSTRUMENT;
		    }
		  else
		    {
		      if (!(standard_drumset.tone[i].instrument))
			standard_drumset.tone[i].instrument=
			  MAGIC_LOAD_INSTRUMENT;
		    }
		}
	      bank->tone[i].instrument=0;
	      errors++;
	    }
	  else if (!(bank->tone[i].instrument=
		     load_instrument(bank->tone[i].name, 
				     (dr) ? 1 : 0,
				     bank->tone[i].pan,
				     bank->tone[i].amp,
				     (bank->tone[i].note!=-1) ? 
				     bank->tone[i].note :
				     ((dr) ? i : -1),
				     (bank->tone[i].strip_loop!=-1) ?
				     bank->tone[i].strip_loop :
				     ((dr) ? 1 : -1),
				     (bank->tone[i].strip_envelope != -1) ? 
				     bank->tone[i].strip_envelope :
				     ((dr) ? 1 : -1),
				     bank->tone[i].strip_tail )))
	    {
	      ctl->cmsg(CMSG_ERROR, VERB_NORMAL, 
		   "Couldn't load instrument %s (%s %d, program %d)",
		   bank->tone[i].name,
		   (dr)? "drum set" : "tone bank", b, i);
	      errors++;
	    }
	}
    }
  return errors;
}

int load_missing_instruments(void)
{
  int i=128,errors=0;
  while (i--)
    {
      if (tonebank[i])
	errors+=fill_bank(0,i);
      if (drumset[i])
	errors+=fill_bank(1,i);
    }
  return errors;
}

void free_instruments(void)
{
  int i=128;
  while(i--)
    {
      if (tonebank[i])
	free_bank(0,i);
      if (drumset[i])
	free_bank(1,i);
    }
}

int set_default_instrument(char *name)
{
  Instrument *ip;
  if (!(ip=load_instrument(name, 0, -1, -1, -1, 0, 0, 0)))
    return -1;
  if (default_instrument)
    free_instrument(default_instrument);
  default_instrument=ip;
  default_program=SPECIAL_PROGRAM;
  return 0;
}

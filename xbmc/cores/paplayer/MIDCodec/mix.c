/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    Suddenly, you realize that this program is free software; you get
    an overwhelming urge to redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received another copy of the GNU General Public
    License along with this program; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    I bet they'll be amazed.

    mix.c */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "resample.h"
#include "mix.h"

/* Returns 1 if envelope runs out */
int recompute_envelope(int v)
{
  int stage;

  stage = voice[v].envelope_stage;

  if (stage>5)
    {
      /* Envelope ran out. */
      int tmp=(voice[v].status == VOICE_DIE); /* Already displayed as dead */
      voice[v].status = VOICE_FREE;
      if(!tmp)
	ctl->note(v);
      return 1;
    }

  if (voice[v].sample->modes & MODES_ENVELOPE)
    {
      if (voice[v].status==VOICE_ON || voice[v].status==VOICE_SUSTAINED)
	{
	  if (stage>2)
	    {
	      /* Freeze envelope until note turns off. Trumpets want this. */
	      voice[v].envelope_increment=0;
	      return 0;
	    }
	}
    }
  voice[v].envelope_stage=stage+1;

  if (voice[v].envelope_volume==voice[v].sample->envelope_offset[stage])
    return recompute_envelope(v);
  voice[v].envelope_target=voice[v].sample->envelope_offset[stage];
  voice[v].envelope_increment = voice[v].sample->envelope_rate[stage];
  if (voice[v].envelope_target<voice[v].envelope_volume)
    voice[v].envelope_increment = -voice[v].envelope_increment;
  return 0;
}

void apply_envelope_to_amp(int v)
{
  float lamp=voice[v].left_amp, ramp;
  int32 la,ra;
  if (voice[v].panned == PANNED_MYSTERY)
    {
      ramp=voice[v].right_amp;
      if (voice[v].tremolo_phase_increment)
	{
	  lamp *= voice[v].tremolo_volume;
	  ramp *= voice[v].tremolo_volume;
	}
      if (voice[v].sample->modes & MODES_ENVELOPE)
	{
	  lamp *= (float)vol_table[voice[v].envelope_volume>>23];
	  ramp *= (float)vol_table[voice[v].envelope_volume>>23];
	}

      la = (int32)FSCALE(lamp,AMP_BITS);
      
      if (la>MAX_AMP_VALUE)
	la=MAX_AMP_VALUE;

      ra = (int32)FSCALE(ramp,AMP_BITS);
      if (ra>MAX_AMP_VALUE)
	ra=MAX_AMP_VALUE;

      
      voice[v].left_mix=FINAL_VOLUME(la);
      voice[v].right_mix=FINAL_VOLUME(ra);
    }
  else
    {
      if (voice[v].tremolo_phase_increment)
	lamp *= voice[v].tremolo_volume;
      if (voice[v].sample->modes & MODES_ENVELOPE)
	lamp *= (float)vol_table[voice[v].envelope_volume>>23];

      la = (int32)FSCALE(lamp,AMP_BITS);

      if (la>MAX_AMP_VALUE)
	la=MAX_AMP_VALUE;

      voice[v].left_mix=FINAL_VOLUME(la);
    }
}

static int update_envelope(int v)
{
  voice[v].envelope_volume += voice[v].envelope_increment;
  /* Why is there no ^^ operator?? */
  if (((voice[v].envelope_increment < 0) &&
       (voice[v].envelope_volume <= voice[v].envelope_target)) ||
      ((voice[v].envelope_increment > 0) &&
	   (voice[v].envelope_volume >= voice[v].envelope_target)))
    {
      voice[v].envelope_volume = voice[v].envelope_target;
      if (recompute_envelope(v))
	return 1;
    }
  return 0;
}

static void update_tremolo(int v)
{
  int32 depth=voice[v].sample->tremolo_depth<<7;

  if (voice[v].tremolo_sweep)
    {
      /* Update sweep position */

      voice[v].tremolo_sweep_position += voice[v].tremolo_sweep;
      if (voice[v].tremolo_sweep_position>=(1<<SWEEP_SHIFT))
	voice[v].tremolo_sweep=0; /* Swept to max amplitude */
      else
	{
	  /* Need to adjust depth */
	  depth *= voice[v].tremolo_sweep_position;
	  depth >>= SWEEP_SHIFT;
	}
    }

  voice[v].tremolo_phase += voice[v].tremolo_phase_increment;

  /* if (voice[v].tremolo_phase >= (SINE_CYCLE_LENGTH<<RATE_SHIFT))
     voice[v].tremolo_phase -= SINE_CYCLE_LENGTH<<RATE_SHIFT;  */

  voice[v].tremolo_volume = (float) 
    (1.0 - FSCALENEG((sine(voice[v].tremolo_phase >> RATE_SHIFT) + 1.0)
		    * depth * TREMOLO_AMPLITUDE_TUNING,
		    17));

  /* I'm not sure about the +1.0 there -- it makes tremoloed voices'
     volumes on average the lower the higher the tremolo amplitude. */
}

/* Returns 1 if the note died */
static int update_signal(int v)
{
  if (voice[v].envelope_increment && update_envelope(v))
    return 1;

  if (voice[v].tremolo_phase_increment)
    update_tremolo(v);

  apply_envelope_to_amp(v);
  return 0;
}

#ifdef LOOKUP_HACK
#  define MIXATION(a)	*lp++ += mixup[(a<<8) | (uint8)s];
#else
#  define MIXATION(a)	*lp++ += (a)*s;
#endif

static void mix_mystery_signal(sample_t *sp, int32 *lp, int v, int count)
{
  Voice *vp = voice + v;
  final_volume_t 
    left=vp->left_mix, 
    right=vp->right_mix;
  int cc;
  sample_t s;

  if (!(cc = vp->control_counter))
    {
      cc = control_ratio;
      if (update_signal(v))
	return;	/* Envelope ran out */
      left = vp->left_mix;
      right = vp->right_mix;
    }
  
  while (count)
    if (cc < count)
      {
	count -= cc;
	while (cc--)
	  {
	    s = *sp++;
	    MIXATION(left);
	    MIXATION(right);
	  }
	cc = control_ratio;
	if (update_signal(v))
	  return;	/* Envelope ran out */
	left = vp->left_mix;
	right = vp->right_mix;
      }
    else
      {
	vp->control_counter = cc - count;
	while (count--)
	  {
	    s = *sp++;
	    MIXATION(left);
	    MIXATION(right);
	  }
	return;
      }
}

static void mix_center_signal(sample_t *sp, int32 *lp, int v, int count)
{
  Voice *vp = voice + v;
  final_volume_t 
    left=vp->left_mix;
  int cc;
  sample_t s;

  if (!(cc = vp->control_counter))
    {
      cc = control_ratio;
      if (update_signal(v))
	return;	/* Envelope ran out */
      left = vp->left_mix;
    }
  
  while (count)
    if (cc < count)
      {
	count -= cc;
	while (cc--)
	  {
	    s = *sp++;
	    MIXATION(left);
	    MIXATION(left);
	  }
	cc = control_ratio;
	if (update_signal(v))
	  return;	/* Envelope ran out */
	left = vp->left_mix;
      }
    else
      {
	vp->control_counter = cc - count;
	while (count--)
	  {
	    s = *sp++;
	    MIXATION(left);
	    MIXATION(left);
	  }
	return;
      }
}

static void mix_single_signal(sample_t *sp, int32 *lp, int v, int count)
{
  Voice *vp = voice + v;
  final_volume_t 
    left=vp->left_mix;
  int cc;
  sample_t s;
  
  if (!(cc = vp->control_counter))
    {
      cc = control_ratio;
      if (update_signal(v))
	return;	/* Envelope ran out */
      left = vp->left_mix;
    }
  
  while (count)
    if (cc < count)
      {
	count -= cc;
	while (cc--)
	  {
	    s = *sp++;
	    MIXATION(left);
	    lp++;
	  }
	cc = control_ratio;
	if (update_signal(v))
	  return;	/* Envelope ran out */
	left = vp->left_mix;
      }
    else
      {
	vp->control_counter = cc - count;
	while (count--)
	  {
	    s = *sp++;
	    MIXATION(left);
	    lp++;
	  }
	return;
      }
}

static void mix_mono_signal(sample_t *sp, int32 *lp, int v, int count)
{
  Voice *vp = voice + v;
  final_volume_t 
    left=vp->left_mix;
  int cc;
  sample_t s;
  
  if (!(cc = vp->control_counter))
    {
      cc = control_ratio;
      if (update_signal(v))
	return;	/* Envelope ran out */
      left = vp->left_mix;
    }
  
  while (count)
    if (cc < count)
      {
	count -= cc;
	while (cc--)
	  {
	    s = *sp++;
	    MIXATION(left);
	  }
	cc = control_ratio;
	if (update_signal(v))
	  return;	/* Envelope ran out */
	left = vp->left_mix;
      }
    else
      {
	vp->control_counter = cc - count;
	while (count--)
	  {
	    s = *sp++;
	    MIXATION(left);
	  }
	return;
      }
}

static void mix_mystery(sample_t *sp, int32 *lp, int v, int count)
{
  final_volume_t 
    left=voice[v].left_mix, 
    right=voice[v].right_mix;
  sample_t s;
  
  while (count--)
    {
      s = *sp++;
      MIXATION(left);
      MIXATION(right);
    }
}

static void mix_center(sample_t *sp, int32 *lp, int v, int count)
{
  final_volume_t 
    left=voice[v].left_mix;
  sample_t s;
  
  while (count--)
    {
      s = *sp++;
      MIXATION(left);
      MIXATION(left);
    }
}

static void mix_single(sample_t *sp, int32 *lp, int v, int count)
{
  final_volume_t 
    left=voice[v].left_mix;
  sample_t s;
  
  while (count--)
    {
      s = *sp++;
      MIXATION(left);
      lp++;
    }
}

static void mix_mono(sample_t *sp, int32 *lp, int v, int count)
{
  final_volume_t 
    left=voice[v].left_mix;
  sample_t s;
  
  while (count--)
    {
      s = *sp++;
      MIXATION(left);
    }
}

/* Ramp a note out in c samples */
static void ramp_out(sample_t *sp, int32 *lp, int v, int32 c)
{

  /* should be final_volume_t, but uint8 gives trouble. */
  int32 left, right, li, ri;

  sample_t s=0; /* silly warning about uninitialized s */

  /* Fix by James Caldwell */
  if ( c == 0 ) c = 1;

  left=voice[v].left_mix;
  li=-(left/c);
  if (!li) li=-1;

  /* printf("Ramping out: left=%d, c=%d, li=%d\n", left, c, li); */

  if (!(play_mode->encoding & PE_MONO))
    {
      if (voice[v].panned==PANNED_MYSTERY)
	{
	  right=voice[v].right_mix;
	  ri=-(right/c);
	  while (c--)
	    {
	      left += li;
	      if (left<0)
		left=0;
	      right += ri;
	      if (right<0)
		right=0;
	      s=*sp++;
	      MIXATION(left);
	      MIXATION(right);
	    }
	}
      else if (voice[v].panned==PANNED_CENTER)
	{
	  while (c--)
	    {
	      left += li;
	      if (left<0)
		return;
	      s=*sp++;	
	      MIXATION(left);
	      MIXATION(left);
	    }
	}
      else if (voice[v].panned==PANNED_LEFT)
	{
	  while (c--)
	    {
	      left += li;
	      if (left<0)
		return;
	      s=*sp++;
	      MIXATION(left);
	      lp++;
	    }
	}
      else if (voice[v].panned==PANNED_RIGHT)
	{
	  while (c--)
	    {
	      left += li;
	      if (left<0)
		return;
	      s=*sp++;
	      lp++;
	      MIXATION(left);
	    }
	}
    }
  else
    {
      /* Mono output.  */
      while (c--)
	{
	  left += li;
	  if (left<0)
	    return;
	  s=*sp++;
	  MIXATION(left);
	}
    }
}


/**************** interface function ******************/

void mix_voice(int32 *buf, int v, int32 c)
{
  Voice *vp=voice+v;
  sample_t *sp;
  if (vp->status==VOICE_DIE)
    {
      if (c>=MAX_DIE_TIME)
	c=MAX_DIE_TIME;
      sp=resample_voice(v, &c);
      ramp_out(sp, buf, v, c);
      vp->status=VOICE_FREE;
    }
  else
    {
      sp=resample_voice(v, &c);
      if (play_mode->encoding & PE_MONO)
	{
	  /* Mono output. */
	  if (vp->envelope_increment || vp->tremolo_phase_increment)
	    mix_mono_signal(sp, buf, v, c);
	  else
	    mix_mono(sp, buf, v, c);
	}
      else
	{
	  if (vp->panned == PANNED_MYSTERY)
	    {
	      if (vp->envelope_increment || vp->tremolo_phase_increment)
		mix_mystery_signal(sp, buf, v, c);
	      else
		mix_mystery(sp, buf, v, c);
	    }
	  else if (vp->panned == PANNED_CENTER)
	    {
	      if (vp->envelope_increment || vp->tremolo_phase_increment)
		mix_center_signal(sp, buf, v, c);
	      else
		mix_center(sp, buf, v, c);
	    }
	  else
	    { 
	      /* It's either full left or full right. In either case,
		 every other sample is 0. Just get the offset right: */
	      if (vp->panned == PANNED_RIGHT) buf++;
	      
	      if (vp->envelope_increment || vp->tremolo_phase_increment)
		mix_single_signal(sp, buf, v, c);
	      else 
		mix_single(sp, buf, v, c);
	    }
	}
    }
}

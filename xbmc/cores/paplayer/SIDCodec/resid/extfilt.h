//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#ifndef __EXTFILT_H__
#define __EXTFILT_H__

#include "siddefs.h"

RESID_NAMESPACE_START

// ----------------------------------------------------------------------------
// The audio output stage in a Commodore 64 consists of two STC networks,
// a low-pass filter with 3-dB frequency 16kHz followed by a high-pass
// filter with 3-dB frequency 16Hz (the latter provided an audio equipment
// input impedance of 1kOhm).
// The STC networks are connected with a BJT supposedly meant to act as
// a unity gain buffer, which is not really how it works. A more elaborate
// model would include the BJT, however DC circuit analysis yields BJT
// base-emitter and emitter-base impedances sufficiently low to produce
// additional low-pass and high-pass 3dB-frequencies in the order of hundreds
// of kHz. This calls for a sampling frequency of several MHz, which is far
// too high for practical use.
// ----------------------------------------------------------------------------
class ExternalFilter
{
public:
  ExternalFilter();

  void enable_filter(bool enable);
  void set_sampling_parameter(double pass_freq);
  void set_chip_model(chip_model model);

  RESID_INLINE void clock(sound_sample Vi);
  RESID_INLINE void clock(cycle_count delta_t, sound_sample Vi);
  void reset();

  // Audio output (20 bits).
  RESID_INLINE sound_sample output();

protected:
  // Filter enabled.
  bool enabled;

  // Maximum mixer DC offset.
  sound_sample mixer_DC;

  // State of filters.
  sound_sample Vlp; // lowpass
  sound_sample Vhp; // highpass
  sound_sample Vo;

  // Cutoff frequencies.
  sound_sample w0lp;
  sound_sample w0hp;

friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__EXTFILT_CC__)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void ExternalFilter::clock(sound_sample Vi)
{
  // This is handy for testing.
  if (!enabled) {
    // Remove maximum DC level since there is no filter to do it.
    Vlp = Vhp = 0;
    Vo = Vi - mixer_DC;
    return;
  }

  // delta_t is converted to seconds given a 1MHz clock by dividing
  // with 1 000 000.

  // Calculate filter outputs.
  // Vo  = Vlp - Vhp;
  // Vlp = Vlp + w0lp*(Vi - Vlp)*delta_t;
  // Vhp = Vhp + w0hp*(Vlp - Vhp)*delta_t;

  sound_sample dVlp = (w0lp >> 8)*(Vi - Vlp) >> 12;
  sound_sample dVhp = w0hp*(Vlp - Vhp) >> 20;
  Vo = Vlp - Vhp;
  Vlp += dVlp;
  Vhp += dVhp;
}

// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
RESID_INLINE
void ExternalFilter::clock(cycle_count delta_t,
			   sound_sample Vi)
{
  // This is handy for testing.
  if (!enabled) {
    // Remove maximum DC level since there is no filter to do it.
    Vlp = Vhp = 0;
    Vo = Vi - mixer_DC;
    return;
  }

  // Maximum delta cycles for the external filter to work satisfactorily
  // is approximately 8.
  cycle_count delta_t_flt = 8;

  while (delta_t) {
    if (delta_t < delta_t_flt) {
      delta_t_flt = delta_t;
    }

    // delta_t is converted to seconds given a 1MHz clock by dividing
    // with 1 000 000.

    // Calculate filter outputs.
    // Vo  = Vlp - Vhp;
    // Vlp = Vlp + w0lp*(Vi - Vlp)*delta_t;
    // Vhp = Vhp + w0hp*(Vlp - Vhp)*delta_t;

    sound_sample dVlp = (w0lp*delta_t_flt >> 8)*(Vi - Vlp) >> 12;
    sound_sample dVhp = w0hp*delta_t_flt*(Vlp - Vhp) >> 20;
    Vo = Vlp - Vhp;
    Vlp += dVlp;
    Vhp += dVhp;

    delta_t -= delta_t_flt;
  }
}


// ----------------------------------------------------------------------------
// Audio output (19.5 bits).
// ----------------------------------------------------------------------------
RESID_INLINE
sound_sample ExternalFilter::output()
{
  return Vo;
}

#endif // RESID_INLINING || defined(__EXTFILT_CC__)

RESID_NAMESPACE_STOP

#endif // not __EXTFILT_H__

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

#ifndef __FILTER_H__
#define __FILTER_H__

#include "siddefs.h"
#include "spline.h"

RESID_NAMESPACE_START

// ----------------------------------------------------------------------------
// The SID filter is modeled with a two-integrator-loop biquadratic filter,
// which has been confirmed by Bob Yannes to be the actual circuit used in
// the SID chip.
//
// Measurements show that excellent emulation of the SID filter is achieved,
// except when high resonance is combined with high sustain levels.
// In this case the SID op-amps are performing less than ideally and are
// causing some peculiar behavior of the SID filter. This however seems to
// have more effect on the overall amplitude than on the color of the sound.
//
// The theory for the filter circuit can be found in "Microelectric Circuits"
// by Adel S. Sedra and Kenneth C. Smith.
// The circuit is modeled based on the explanation found there except that
// an additional inverter is used in the feedback from the bandpass output,
// allowing the summer op-amp to operate in single-ended mode. This yields
// inverted filter outputs with levels independent of Q, which corresponds with
// the results obtained from a real SID.
//
// We have been able to model the summer and the two integrators of the circuit
// to form components of an IIR filter.
// Vhp is the output of the summer, Vbp is the output of the first integrator,
// and Vlp is the output of the second integrator in the filter circuit.
//
// According to Bob Yannes, the active stages of the SID filter are not really
// op-amps. Rather, simple NMOS inverters are used. By biasing an inverter
// into its region of quasi-linear operation using a feedback resistor from
// input to output, a MOS inverter can be made to act like an op-amp for
// small signals centered around the switching threshold.
//
// Qualified guesses at SID filter schematics are depicted below.
//
// SID filter
// ----------
// 
//     -----------------------------------------------
//    |                                               |
//    |            ---Rq--                            |
//    |           |       |                           |
//    |  ------------<A]-----R1---------              |
//    | |                               |             |
//    | |                        ---C---|      ---C---|
//    | |                       |       |     |       |
//    |  --R1--    ---R1--      |---Rs--|     |---Rs--| 
//    |        |  |       |     |       |     |       |
//     ----R1--|-----[A>--|--R-----[A>--|--R-----[A>--|
//             |          |             |             |
// vi -----R1--           |             |             |
// 
//                       vhp           vbp           vlp
// 
// 
// vi  - input voltage
// vhp - highpass output
// vbp - bandpass output
// vlp - lowpass output
// [A> - op-amp
// R1  - summer resistor
// Rq  - resistor array controlling resonance (4 resistors)
// R   - NMOS FET voltage controlled resistor controlling cutoff frequency
// Rs  - shunt resitor
// C   - capacitor
// 
// 
// 
// SID integrator
// --------------
// 
//                                   V+
// 
//                                   |
//                                   |
//                              -----|
//                             |     |
//                             | ||--
//                              -||
//                   ---C---     ||->
//                  |       |        |
//                  |---Rs-----------|---- vo
//                  |                |
//                  |            ||--
// vi ----     -----|------------||
//        |   ^     |            ||->
//        |___|     |                |
//        -----     |                |
//          |       |                |
//          |---R2--                 |
//          |
//          R1                       V-
//          |
//          |
// 
//          Vw
//
// ----------------------------------------------------------------------------
class Filter
{
public:
  Filter();

  void enable_filter(bool enable);
  void set_chip_model(chip_model model);

  RESID_INLINE
  void clock(sound_sample voice1, sound_sample voice2, sound_sample voice3,
	     sound_sample ext_in);
  RESID_INLINE
  void clock(cycle_count delta_t,
  	     sound_sample voice1, sound_sample voice2, sound_sample voice3,
	     sound_sample ext_in);
  void reset();

  // Write registers.
  void writeFC_LO(reg8);
  void writeFC_HI(reg8);
  void writeRES_FILT(reg8);
  void writeMODE_VOL(reg8);

  // SID audio output (16 bits).
  sound_sample output();

  // Spline functions.
  void fc_default(const fc_point*& points, int& count);
  PointPlotter<sound_sample> fc_plotter();

protected:
  void set_w0();
  void set_Q();

  // Filter enabled.
  bool enabled;

  // Filter cutoff frequency.
  reg12 fc;

  // Filter resonance.
  reg8 res;

  // Selects which inputs to route through filter.
  reg8 filt;

  // Switch voice 3 off.
  reg8 voice3off;

  // Highpass, bandpass, and lowpass filter modes.
  reg8 hp_bp_lp;

  // Output master volume.
  reg4 vol;

  // Mixer DC offset.
  sound_sample mixer_DC;

  // State of filter.
  sound_sample Vhp; // highpass
  sound_sample Vbp; // bandpass
  sound_sample Vlp; // lowpass
  sound_sample Vnf; // not filtered

  // Cutoff frequency, resonance.
  sound_sample w0, w0_ceil_1, w0_ceil_dt;
  sound_sample _1024_div_Q;

  // Cutoff frequency tables.
  // FC is an 11 bit register.
  sound_sample f0_6581[2048];
  sound_sample f0_8580[2048];
  sound_sample* f0;
  static fc_point f0_points_6581[];
  static fc_point f0_points_8580[];
  fc_point* f0_points;
  int f0_count;

friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__FILTER_CC__)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void Filter::clock(sound_sample voice1,
		   sound_sample voice2,
		   sound_sample voice3,
		   sound_sample ext_in)
{
  // Scale each voice down from 20 to 13 bits.
  voice1 >>= 7;
  voice2 >>= 7;

  // NB! Voice 3 is not silenced by voice3off if it is routed through
  // the filter.
  if (voice3off && !(filt & 0x04)) {
    voice3 = 0;
  }
  else {
    voice3 >>= 7;
  }

  ext_in >>= 7;

  // This is handy for testing.
  if (!enabled) {
    Vnf = voice1 + voice2 + voice3 + ext_in;
    Vhp = Vbp = Vlp = 0;
    return;
  }

  // Route voices into or around filter.
  // The code below is expanded to a switch for faster execution.
  // (filt1 ? Vi : Vnf) += voice1;
  // (filt2 ? Vi : Vnf) += voice2;
  // (filt3 ? Vi : Vnf) += voice3;

  sound_sample Vi;

  switch (filt) {
  default:
  case 0x0:
    Vi = 0;
    Vnf = voice1 + voice2 + voice3 + ext_in;
    break;
  case 0x1:
    Vi = voice1;
    Vnf = voice2 + voice3 + ext_in;
    break;
  case 0x2:
    Vi = voice2;
    Vnf = voice1 + voice3 + ext_in;
    break;
  case 0x3:
    Vi = voice1 + voice2;
    Vnf = voice3 + ext_in;
    break;
  case 0x4:
    Vi = voice3;
    Vnf = voice1 + voice2 + ext_in;
    break;
  case 0x5:
    Vi = voice1 + voice3;
    Vnf = voice2 + ext_in;
    break;
  case 0x6:
    Vi = voice2 + voice3;
    Vnf = voice1 + ext_in;
    break;
  case 0x7:
    Vi = voice1 + voice2 + voice3;
    Vnf = ext_in;
    break;
  case 0x8:
    Vi = ext_in;
    Vnf = voice1 + voice2 + voice3;
    break;
  case 0x9:
    Vi = voice1 + ext_in;
    Vnf = voice2 + voice3;
    break;
  case 0xa:
    Vi = voice2 + ext_in;
    Vnf = voice1 + voice3;
    break;
  case 0xb:
    Vi = voice1 + voice2 + ext_in;
    Vnf = voice3;
    break;
  case 0xc:
    Vi = voice3 + ext_in;
    Vnf = voice1 + voice2;
    break;
  case 0xd:
    Vi = voice1 + voice3 + ext_in;
    Vnf = voice2;
    break;
  case 0xe:
    Vi = voice2 + voice3 + ext_in;
    Vnf = voice1;
    break;
  case 0xf:
    Vi = voice1 + voice2 + voice3 + ext_in;
    Vnf = 0;
    break;
  }
    
  // delta_t = 1 is converted to seconds given a 1MHz clock by dividing
  // with 1 000 000.

  // Calculate filter outputs.
  // Vhp = Vbp/Q - Vlp - Vi;
  // dVbp = -w0*Vhp*dt;
  // dVlp = -w0*Vbp*dt;

  sound_sample dVbp = (w0_ceil_1*Vhp >> 20);
  sound_sample dVlp = (w0_ceil_1*Vbp >> 20);
  Vbp -= dVbp;
  Vlp -= dVlp;
  Vhp = (Vbp*_1024_div_Q >> 10) - Vlp - Vi;
}

// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
RESID_INLINE
void Filter::clock(cycle_count delta_t,
		   sound_sample voice1,
		   sound_sample voice2,
		   sound_sample voice3,
		   sound_sample ext_in)
{
  // Scale each voice down from 20 to 13 bits.
  voice1 >>= 7;
  voice2 >>= 7;

  // NB! Voice 3 is not silenced by voice3off if it is routed through
  // the filter.
  if (voice3off && !(filt & 0x04)) {
    voice3 = 0;
  }
  else {
    voice3 >>= 7;
  }

  ext_in >>= 7;

  // Enable filter on/off.
  // This is not really part of SID, but is useful for testing.
  // On slow CPUs it may be necessary to bypass the filter to lower the CPU
  // load.
  if (!enabled) {
    Vnf = voice1 + voice2 + voice3 + ext_in;
    Vhp = Vbp = Vlp = 0;
    return;
  }

  // Route voices into or around filter.
  // The code below is expanded to a switch for faster execution.
  // (filt1 ? Vi : Vnf) += voice1;
  // (filt2 ? Vi : Vnf) += voice2;
  // (filt3 ? Vi : Vnf) += voice3;

  sound_sample Vi;

  switch (filt) {
  default:
  case 0x0:
    Vi = 0;
    Vnf = voice1 + voice2 + voice3 + ext_in;
    break;
  case 0x1:
    Vi = voice1;
    Vnf = voice2 + voice3 + ext_in;
    break;
  case 0x2:
    Vi = voice2;
    Vnf = voice1 + voice3 + ext_in;
    break;
  case 0x3:
    Vi = voice1 + voice2;
    Vnf = voice3 + ext_in;
    break;
  case 0x4:
    Vi = voice3;
    Vnf = voice1 + voice2 + ext_in;
    break;
  case 0x5:
    Vi = voice1 + voice3;
    Vnf = voice2 + ext_in;
    break;
  case 0x6:
    Vi = voice2 + voice3;
    Vnf = voice1 + ext_in;
    break;
  case 0x7:
    Vi = voice1 + voice2 + voice3;
    Vnf = ext_in;
    break;
  case 0x8:
    Vi = ext_in;
    Vnf = voice1 + voice2 + voice3;
    break;
  case 0x9:
    Vi = voice1 + ext_in;
    Vnf = voice2 + voice3;
    break;
  case 0xa:
    Vi = voice2 + ext_in;
    Vnf = voice1 + voice3;
    break;
  case 0xb:
    Vi = voice1 + voice2 + ext_in;
    Vnf = voice3;
    break;
  case 0xc:
    Vi = voice3 + ext_in;
    Vnf = voice1 + voice2;
    break;
  case 0xd:
    Vi = voice1 + voice3 + ext_in;
    Vnf = voice2;
    break;
  case 0xe:
    Vi = voice2 + voice3 + ext_in;
    Vnf = voice1;
    break;
  case 0xf:
    Vi = voice1 + voice2 + voice3 + ext_in;
    Vnf = 0;
    break;
  }

  // Maximum delta cycles for the filter to work satisfactorily under current
  // cutoff frequency and resonance constraints is approximately 8.
  cycle_count delta_t_flt = 8;

  while (delta_t) {
    if (delta_t < delta_t_flt) {
      delta_t_flt = delta_t;
    }

    // delta_t is converted to seconds given a 1MHz clock by dividing
    // with 1 000 000. This is done in two operations to avoid integer
    // multiplication overflow.

    // Calculate filter outputs.
    // Vhp = Vbp/Q - Vlp - Vi;
    // dVbp = -w0*Vhp*dt;
    // dVlp = -w0*Vbp*dt;
    sound_sample w0_delta_t = w0_ceil_dt*delta_t_flt >> 6;

    sound_sample dVbp = (w0_delta_t*Vhp >> 14);
    sound_sample dVlp = (w0_delta_t*Vbp >> 14);
    Vbp -= dVbp;
    Vlp -= dVlp;
    Vhp = (Vbp*_1024_div_Q >> 10) - Vlp - Vi;

    delta_t -= delta_t_flt;
  }
}


// ----------------------------------------------------------------------------
// SID audio output (20 bits).
// ----------------------------------------------------------------------------
RESID_INLINE
sound_sample Filter::output()
{
  // This is handy for testing.
  if (!enabled) {
    return (Vnf + mixer_DC)*static_cast<sound_sample>(vol);
  }

  // Mix highpass, bandpass, and lowpass outputs. The sum is not
  // weighted, this can be confirmed by sampling sound output for
  // e.g. bandpass, lowpass, and bandpass+lowpass from a SID chip.

  // The code below is expanded to a switch for faster execution.
  // if (hp) Vf += Vhp;
  // if (bp) Vf += Vbp;
  // if (lp) Vf += Vlp;

  sound_sample Vf;

  switch (hp_bp_lp) {
  default:
  case 0x0:
    Vf = 0;
    break;
  case 0x1:
    Vf = Vlp;
    break;
  case 0x2:
    Vf = Vbp;
    break;
  case 0x3:
    Vf = Vlp + Vbp;
    break;
  case 0x4:
    Vf = Vhp;
    break;
  case 0x5:
    Vf = Vlp + Vhp;
    break;
  case 0x6:
    Vf = Vbp + Vhp;
    break;
  case 0x7:
    Vf = Vlp + Vbp + Vhp;
    break;
  }

  // Sum non-filtered and filtered output.
  // Multiply the sum with volume.
  return (Vnf + Vf + mixer_DC)*static_cast<sound_sample>(vol);
}

#endif // RESID_INLINING || defined(__FILTER_CC__)

RESID_NAMESPACE_STOP

#endif // not __FILTER_H__

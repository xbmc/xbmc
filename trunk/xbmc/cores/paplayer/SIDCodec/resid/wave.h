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

#ifndef __WAVE_H__
#define __WAVE_H__

#include "siddefs.h"

RESID_NAMESPACE_START

// ----------------------------------------------------------------------------
// A 24 bit accumulator is the basis for waveform generation. FREQ is added to
// the lower 16 bits of the accumulator each cycle.
// The accumulator is set to zero when TEST is set, and starts counting
// when TEST is cleared.
// The noise waveform is taken from intermediate bits of a 23 bit shift
// register. This register is clocked by bit 19 of the accumulator.
// ----------------------------------------------------------------------------
class WaveformGenerator
{
public:
  WaveformGenerator();

  void set_sync_source(WaveformGenerator*);
  void set_chip_model(chip_model model);

  RESID_INLINE void clock();
  RESID_INLINE void clock(cycle_count delta_t);
  RESID_INLINE void synchronize();
  void reset();

  void writeFREQ_LO(reg8);
  void writeFREQ_HI(reg8);
  void writePW_LO(reg8);
  void writePW_HI(reg8);
  void writeCONTROL_REG(reg8);
  reg8 readOSC();

  // 12-bit waveform output.
  RESID_INLINE reg12 output();

protected:
  const WaveformGenerator* sync_source;
  WaveformGenerator* sync_dest;

  // Tell whether the accumulator MSB was set high on this cycle.
  bool msb_rising;

  reg24 accumulator;
  reg24 shift_register;

  // Fout  = (Fn*Fclk/16777216)Hz
  reg16 freq;
  // PWout = (PWn/40.95)%
  reg12 pw;

  // The control register right-shifted 4 bits; used for output function
  // table lookup.
  reg8 waveform;

  // The remaining control register bits.
  reg8 test;
  reg8 ring_mod;
  reg8 sync;
  // The gate bit is handled by the EnvelopeGenerator.

  // 16 possible combinations of waveforms.
  RESID_INLINE reg12 output____();
  RESID_INLINE reg12 output___T();
  RESID_INLINE reg12 output__S_();
  RESID_INLINE reg12 output__ST();
  RESID_INLINE reg12 output_P__();
  RESID_INLINE reg12 output_P_T();
  RESID_INLINE reg12 output_PS_();
  RESID_INLINE reg12 output_PST();
  RESID_INLINE reg12 outputN___();
  RESID_INLINE reg12 outputN__T();
  RESID_INLINE reg12 outputN_S_();
  RESID_INLINE reg12 outputN_ST();
  RESID_INLINE reg12 outputNP__();
  RESID_INLINE reg12 outputNP_T();
  RESID_INLINE reg12 outputNPS_();
  RESID_INLINE reg12 outputNPST();

  // Sample data for combinations of waveforms.
  static reg8 wave6581__ST[];
  static reg8 wave6581_P_T[];
  static reg8 wave6581_PS_[];
  static reg8 wave6581_PST[];

  static reg8 wave8580__ST[];
  static reg8 wave8580_P_T[];
  static reg8 wave8580_PS_[];
  static reg8 wave8580_PST[];

  reg8* wave__ST;
  reg8* wave_P_T;
  reg8* wave_PS_;
  reg8* wave_PST;

friend class Voice;
friend class SID;
};


// ----------------------------------------------------------------------------
// Inline functions.
// The following functions are defined inline because they are called every
// time a sample is calculated.
// ----------------------------------------------------------------------------

#if RESID_INLINING || defined(__WAVE_CC__)

// ----------------------------------------------------------------------------
// SID clocking - 1 cycle.
// ----------------------------------------------------------------------------
RESID_INLINE
void WaveformGenerator::clock()
{
  // No operation if test bit is set.
  if (test) {
    return;
  }

  reg24 accumulator_prev = accumulator;

  // Calculate new accumulator value;
  accumulator += freq;
  accumulator &= 0xffffff;

  // Check whether the MSB is set high. This is used for synchronization.
  msb_rising = !(accumulator_prev & 0x800000) && (accumulator & 0x800000);

  // Shift noise register once for each time accumulator bit 19 is set high.
  if (!(accumulator_prev & 0x080000) && (accumulator & 0x080000)) {
    reg24 bit0 = ((shift_register >> 22) ^ (shift_register >> 17)) & 0x1;
    shift_register <<= 1;
    shift_register &= 0x7fffff;
    shift_register |= bit0;
  }
}

// ----------------------------------------------------------------------------
// SID clocking - delta_t cycles.
// ----------------------------------------------------------------------------
RESID_INLINE
void WaveformGenerator::clock(cycle_count delta_t)
{
  // No operation if test bit is set.
  if (test) {
    return;
  }

  reg24 accumulator_prev = accumulator;

  // Calculate new accumulator value;
  reg24 delta_accumulator = delta_t*freq;
  accumulator += delta_accumulator;
  accumulator &= 0xffffff;

  // Check whether the MSB is set high. This is used for synchronization.
  msb_rising = !(accumulator_prev & 0x800000) && (accumulator & 0x800000);

  // Shift noise register once for each time accumulator bit 19 is set high.
  // Bit 19 is set high each time 2^20 (0x100000) is added to the accumulator.
  reg24 shift_period = 0x100000;

  while (delta_accumulator) {
    if (delta_accumulator < shift_period) {
      shift_period = delta_accumulator;
      // Determine whether bit 19 is set on the last period.
      // NB! Requires two's complement integer.
      if (shift_period <= 0x080000) {
	// Check for flip from 0 to 1.
	if (((accumulator - shift_period) & 0x080000) || !(accumulator & 0x080000))
	{
	  break;
	}
      }
      else {
	// Check for flip from 0 (to 1 or via 1 to 0) or from 1 via 0 to 1.
	if (((accumulator - shift_period) & 0x080000) && !(accumulator & 0x080000))
	{
	  break;
	}
      }
    }

    // Shift the noise/random register.
    // NB! The shift is actually delayed 2 cycles, this is not modeled.
    reg24 bit0 = ((shift_register >> 22) ^ (shift_register >> 17)) & 0x1;
    shift_register <<= 1;
    shift_register &= 0x7fffff;
    shift_register |= bit0;

    delta_accumulator -= shift_period;
  }
}


// ----------------------------------------------------------------------------
// Synchronize oscillators.
// This must be done after all the oscillators have been clock()'ed since the
// oscillators operate in parallel.
// Note that the oscillators must be clocked exactly on the cycle when the
// MSB is set high for hard sync to operate correctly. See SID::clock().
// ----------------------------------------------------------------------------
RESID_INLINE
void WaveformGenerator::synchronize()
{
  // A special case occurs when a sync source is synced itself on the same
  // cycle as when its MSB is set high. In this case the destination will
  // not be synced. This has been verified by sampling OSC3.
  if (msb_rising && sync_dest->sync && !(sync && sync_source->msb_rising)) {
    sync_dest->accumulator = 0;
  }
}


// ----------------------------------------------------------------------------
// Output functions.
// NB! The output from SID 8580 is delayed one cycle compared to SID 6581,
// this is not modeled.
// ----------------------------------------------------------------------------

// No waveform:
// Zero output.
//
RESID_INLINE
reg12 WaveformGenerator::output____()
{
  return 0x000;
}

// Triangle:
// The upper 12 bits of the accumulator are used.
// The MSB is used to create the falling edge of the triangle by inverting
// the lower 11 bits. The MSB is thrown away and the lower 11 bits are
// left-shifted (half the resolution, full amplitude).
// Ring modulation substitutes the MSB with MSB EOR sync_source MSB.
//
RESID_INLINE
reg12 WaveformGenerator::output___T()
{
  reg24 msb = (ring_mod ? accumulator ^ sync_source->accumulator : accumulator)
    & 0x800000;
  return ((msb ? ~accumulator : accumulator) >> 11) & 0xfff;
}

// Sawtooth:
// The output is identical to the upper 12 bits of the accumulator.
//
RESID_INLINE
reg12 WaveformGenerator::output__S_()
{
  return accumulator >> 12;
}

// Pulse:
// The upper 12 bits of the accumulator are used.
// These bits are compared to the pulse width register by a 12 bit digital
// comparator; output is either all one or all zero bits.
// NB! The output is actually delayed one cycle after the compare.
// This is not modeled.
//
// The test bit, when set to one, holds the pulse waveform output at 0xfff
// regardless of the pulse width setting.
//
RESID_INLINE
reg12 WaveformGenerator::output_P__()
{
  return (test || (accumulator >> 12) >= pw) ? 0xfff : 0x000;
}

// Noise:
// The noise output is taken from intermediate bits of a 23-bit shift register
// which is clocked by bit 19 of the accumulator.
// NB! The output is actually delayed 2 cycles after bit 19 is set high.
// This is not modeled.
//
// Operation: Calculate EOR result, shift register, set bit 0 = result.
//
//                        ----------------------->---------------------
//                        |                                            |
//                   ----EOR----                                       |
//                   |         |                                       |
//                   2 2 2 1 1 1 1 1 1 1 1 1 1                         |
// Register bits:    2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 <---
//                   |   |       |     |   |       |     |   |
// OSC3 bits  :      7   6       5     4   3       2     1   0
//
// Since waveform output is 12 bits the output is left-shifted 4 times.
//
RESID_INLINE
reg12 WaveformGenerator::outputN___()
{
  return
    ((shift_register & 0x400000) >> 11) |
    ((shift_register & 0x100000) >> 10) |
    ((shift_register & 0x010000) >> 7) |
    ((shift_register & 0x002000) >> 5) |
    ((shift_register & 0x000800) >> 4) |
    ((shift_register & 0x000080) >> 1) |
    ((shift_register & 0x000010) << 1) |
    ((shift_register & 0x000004) << 2);
}

// Combined waveforms:
// By combining waveforms, the bits of each waveform are effectively short
// circuited. A zero bit in one waveform will result in a zero output bit
// (thus the infamous claim that the waveforms are AND'ed).
// However, a zero bit in one waveform will also affect the neighboring bits
// in the output. The reason for this has not been determined.
//
// Example:
// 
//             1 1
// Bit #       1 0 9 8 7 6 5 4 3 2 1 0
//             -----------------------
// Sawtooth    0 0 0 1 1 1 1 1 1 0 0 0
//
// Triangle    0 0 1 1 1 1 1 1 0 0 0 0
//
// AND         0 0 0 1 1 1 1 1 0 0 0 0
//
// Output      0 0 0 0 1 1 1 0 0 0 0 0
//
//
// This behavior would be quite difficult to model exactly, since the SID
// in this case does not act as a digital state machine. Tests show that minor
// (1 bit)  differences can actually occur in the output from otherwise
// identical samples from OSC3 when waveforms are combined. To further
// complicate the situation the output changes slightly with time (more
// neighboring bits are successively set) when the 12-bit waveform
// registers are kept unchanged.
//
// It is probably possible to come up with a valid model for the
// behavior, however this would be far too slow for practical use since it
// would have to be based on the mutual influence of individual bits.
//
// The output is instead approximated by using the upper bits of the
// accumulator as an index to look up the combined output in a table
// containing actual combined waveform samples from OSC3.
// These samples are 8 bit, so 4 bits of waveform resolution is lost.
// All OSC3 samples are taken with FREQ=0x1000, adding a 1 to the upper 12
// bits of the accumulator each cycle for a sample period of 4096 cycles.
//
// Sawtooth+Triangle:
// The sawtooth output is used to look up an OSC3 sample.
// 
// Pulse+Triangle:
// The triangle output is right-shifted and used to look up an OSC3 sample.
// The sample is output if the pulse output is on.
// The reason for using the triangle output as the index is to handle ring
// modulation. Only the first half of the sample is used, which should be OK
// since the triangle waveform has half the resolution of the accumulator.
// 
// Pulse+Sawtooth:
// The sawtooth output is used to look up an OSC3 sample.
// The sample is output if the pulse output is on.
//
// Pulse+Sawtooth+Triangle:
// The sawtooth output is used to look up an OSC3 sample.
// The sample is output if the pulse output is on.
// 
RESID_INLINE
reg12 WaveformGenerator::output__ST()
{
  return wave__ST[output__S_()] << 4;
}

RESID_INLINE
reg12 WaveformGenerator::output_P_T()
{
  return (wave_P_T[output___T() >> 1] << 4) & output_P__();
}

RESID_INLINE
reg12 WaveformGenerator::output_PS_()
{
  return (wave_PS_[output__S_()] << 4) & output_P__();
}

RESID_INLINE
reg12 WaveformGenerator::output_PST()
{
  return (wave_PST[output__S_()] << 4) & output_P__();
}

// Combined waveforms including noise:
// All waveform combinations including noise output zero after a few cycles.
// NB! The effects of such combinations are not fully explored. It is claimed
// that the shift register may be filled with zeroes and locked up, which
// seems to be true.
// We have not attempted to model this behavior, suffice to say that
// there is very little audible output from waveform combinations including
// noise. We hope that nobody is actually using it.
//
RESID_INLINE
reg12 WaveformGenerator::outputN__T()
{
  return 0;
}

RESID_INLINE
reg12 WaveformGenerator::outputN_S_()
{
  return 0;
}

RESID_INLINE
reg12 WaveformGenerator::outputN_ST()
{
  return 0;
}

RESID_INLINE
reg12 WaveformGenerator::outputNP__()
{
  return 0;
}

RESID_INLINE
reg12 WaveformGenerator::outputNP_T()
{
  return 0;
}

RESID_INLINE
reg12 WaveformGenerator::outputNPS_()
{
  return 0;
}

RESID_INLINE
reg12 WaveformGenerator::outputNPST()
{
  return 0;
}

// ----------------------------------------------------------------------------
// Select one of 16 possible combinations of waveforms.
// ----------------------------------------------------------------------------
RESID_INLINE
reg12 WaveformGenerator::output()
{
  // It may seem cleaner to use an array of member functions to return
  // waveform output; however a switch with inline functions is faster.

  switch (waveform) {
  default:
  case 0x0:
    return output____();
  case 0x1:
    return output___T();
  case 0x2:
    return output__S_();
  case 0x3:
    return output__ST();
  case 0x4:
    return output_P__();
  case 0x5:
    return output_P_T();
  case 0x6:
    return output_PS_();
  case 0x7:
    return output_PST();
  case 0x8:
    return outputN___();
  case 0x9:
    return outputN__T();
  case 0xa:
    return outputN_S_();
  case 0xb:
    return outputN_ST();
  case 0xc:
    return outputNP__();
  case 0xd:
    return outputNP_T();
  case 0xe:
    return outputNPS_();
  case 0xf:
    return outputNPST();
  }
}

#endif // RESID_INLINING || defined(__WAVE_CC__)

RESID_NAMESPACE_STOP

#endif // not __WAVE_H__

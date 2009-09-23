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

#ifndef __SID_H__
#define __SID_H__

#include "siddefs.h"
#include "voice.h"
#include "filter.h"
#include "extfilt.h"
#include "pot.h"

RESID_NAMESPACE_START

class SID
{
public:
  SID();
  ~SID();

  void set_chip_model(chip_model model);
  void enable_filter(bool enable);
  void enable_external_filter(bool enable);
  bool set_sampling_parameters(double clock_freq, sampling_method method,
			       double sample_freq, double pass_freq = -1,
			       double filter_scale = 0.97);
  void adjust_sampling_frequency(double sample_freq);

  void fc_default(const fc_point*& points, int& count);
  PointPlotter<sound_sample> fc_plotter();

  void clock();
  void clock(cycle_count delta_t);
  int clock(cycle_count& delta_t, short* buf, int n, int interleave = 1);
  void reset();
  
  // Read/write registers.
  reg8 read(reg8 offset);
  void write(reg8 offset, reg8 value);
  void mute(reg8 channel, bool enable);

  // Read/write state.
  class State
  {
  public:
    State();

    char sid_register[0x20];

    reg8 bus_value;
    cycle_count bus_value_ttl;

    reg24 accumulator[3];
    reg24 shift_register[3];
    reg16 rate_counter[3];
    reg16 rate_counter_period[3];
    reg16 exponential_counter[3];
    reg16 exponential_counter_period[3];
    reg8 envelope_counter[3];
    EnvelopeGenerator::State envelope_state[3];
    bool hold_zero[3];
  };
    
  State read_state();
  void write_state(const State& state);

  // 16-bit input (EXT IN).
  void input(int sample);

  // 16-bit output (AUDIO OUT).
  int output();
  // n-bit output.
  int output(int bits);

protected:
  static double I0(double x);
  RESID_INLINE int clock_fast(cycle_count& delta_t, short* buf, int n,
			      int interleave);
  RESID_INLINE int clock_interpolate(cycle_count& delta_t, short* buf, int n,
				     int interleave);
  RESID_INLINE int clock_resample_interpolate(cycle_count& delta_t, short* buf,
					      int n, int interleave);
  RESID_INLINE int clock_resample_fast(cycle_count& delta_t, short* buf,
				       int n, int interleave);

  Voice voice[3];
  Filter filter;
  ExternalFilter extfilt;
  Potentiometer potx;
  Potentiometer poty;

  reg8 bus_value;
  cycle_count bus_value_ttl;

  double clock_frequency;

  // External audio input.
  int ext_in;

  // Resampling constants.
  static const int FIR_N;
  static const int FIR_RES_INTERPOLATE;
  static const int FIR_RES_FAST;
  static const int FIR_SHIFT;
  static const int RINGSIZE;

  // Fixpoint constants.
  static const int FIXP_SHIFT;
  static const int FIXP_MASK;

  // Sampling variables.
  sampling_method sampling;
  cycle_count cycles_per_sample;
  cycle_count sample_offset;
  int sample_index;
  short sample_prev;
  int fir_N;
  int fir_RES;

  // Ring buffer with overflow for contiguous storage of RINGSIZE samples.
  short* sample;

  // FIR_RES filter tables (FIR_N*FIR_RES).
  short* fir;
};

RESID_NAMESPACE_STOP

#endif // not __SID_H__

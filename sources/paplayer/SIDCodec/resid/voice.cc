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

#define __VOICE_CC__
#include "voice.h"

RESID_NAMESPACE_START

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
Voice::Voice()
  : muted(false)
{
  set_chip_model(MOS6581);
}

// ----------------------------------------------------------------------------
// Set chip model.
// ----------------------------------------------------------------------------
void Voice::set_chip_model(chip_model model)
{
  wave.set_chip_model(model);

  if (model == MOS6581) {
    // The waveform D/A converter introduces a DC offset in the signal
    // to the envelope multiplying D/A converter. The "zero" level of
    // the waveform D/A converter can be found as follows:
    //
    // Measure the "zero" voltage of voice 3 on the SID audio output
    // pin, routing only voice 3 to the mixer ($d417 = $0b, $d418 =
    // $0f, all other registers zeroed).
    //
    // Then set the sustain level for voice 3 to maximum and search for
    // the waveform output value yielding the same voltage as found
    // above. This is done by trying out different waveform output
    // values until the correct value is found, e.g. with the following
    // program:
    //
    //	lda #$08
    //	sta $d412
    //	lda #$0b
    //	sta $d417
    //	lda #$0f
    //	sta $d418
    //	lda #$f0
    //	sta $d414
    //	lda #$21
    //	sta $d412
    //	lda #$01
    //	sta $d40e
    //
    //	ldx #$00
    //	lda #$38	; Tweak this to find the "zero" level
    //l	cmp $d41b
    //	bne l
    //	stx $d40e	; Stop frequency counter - freeze waveform output
    //	brk
    //
    // The waveform output range is 0x000 to 0xfff, so the "zero"
    // level should ideally have been 0x800. In the measured chip, the
    // waveform output "zero" level was found to be 0x380 (i.e. $d41b
    // = 0x38) at 5.94V.

    wave_zero = 0x380;

    // The envelope multiplying D/A converter introduces another DC
    // offset. This is isolated by the following measurements:
    //
    // * The "zero" output level of the mixer at full volume is 5.44V.
    // * Routing one voice to the mixer at full volume yields
    //     6.75V at maximum voice output (wave = 0xfff, sustain = 0xf)
    //     5.94V at "zero" voice output  (wave = any,   sustain = 0x0)
    //     5.70V at minimum voice output (wave = 0x000, sustain = 0xf)
    // * The DC offset of one voice is (5.94V - 5.44V) = 0.50V
    // * The dynamic range of one voice is |6.75V - 5.70V| = 1.05V
    // * The DC offset is thus 0.50V/1.05V ~ 1/2 of the dynamic range.
    //
    // Note that by removing the DC offset, we get the following ranges for
    // one voice:
    //     y > 0: (6.75V - 5.44V) - 0.50V =  0.81V
    //     y < 0: (5.70V - 5.44V) - 0.50V = -0.24V
    // The scaling of the voice amplitude is not symmetric about y = 0;
    // this follows from the DC level in the waveform output.

    voice_DC = 0x800*0xff;
  }
  else {
    // No DC offsets in the MOS8580.
    wave_zero = 0x800;
    voice_DC = 0;
  }
}

// ----------------------------------------------------------------------------
// Set sync source.
// ----------------------------------------------------------------------------
void Voice::set_sync_source(Voice* source)
{
  wave.set_sync_source(&source->wave);
}

// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void Voice::writeCONTROL_REG(reg8 control)
{
  wave.writeCONTROL_REG(control);
  envelope.writeCONTROL_REG(control);
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void Voice::reset()
{
  wave.reset();
  envelope.reset();
}


// ----------------------------------------------------------------------------
// Voice mute.
// ----------------------------------------------------------------------------
void Voice::mute(bool enable)
{
  // enable = true (means voice is muted)
  muted = enable;
}

RESID_NAMESPACE_STOP

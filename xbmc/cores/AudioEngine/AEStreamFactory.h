#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING. If not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 */

#include "AEAudioFormat.h"

class IAEStream;

class CAEStreamFactory
{
public:
  /**
   * Creates and returns a new IAEStream for the currently active engine.
   * 
   * @returns A valid stream or NULL if the supplied parameters were invalid
   * 
   * @see IAE::MakeStream for parameter documentation
   */
  static IAEStream *MakeStream(enum AEDataFormat dataFormat, unsigned int sampleRate, unsigned int encodedSampleRate, CAEChannelInfo channelLayout, unsigned int options = 0);
  
  /**
   * Remove the specified stream from the engine and free it.
   */
  static void FreeStream(IAEStream *stream);
};

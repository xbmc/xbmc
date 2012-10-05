#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"
#include "AEAudioFormat.h"

class CAEWAVLoader
{
public:
  CAEWAVLoader();
  ~CAEWAVLoader();

  /**
   * Load a WAV file into memory
   * @param filename The filename to load
   * @return         true on success
   */
  bool Load(const std::string &filename);

  /**
   * Unload and release the samples loaded by CAWEAVLoader::Load
   */
  void UnLoad();

  /**
   * Initialize the loaded file for the required output format
   * @param sampleRate    The desired output sample rate
   * @param channelLayout The desired output channel layout
   * @param stdChLayout   The channels that are actually used in the layout
   */
  bool Initialize(unsigned int resampleRate, CAEChannelInfo channelLayout, enum AEStdChLayout stdChLayout = AE_CH_LAYOUT_INVALID);

  /**
   * DeInitialize the remapped/resampled output
   */
  void DeInitialize();

  /**
   * Returns if the current sample buffer is valid
   * @return true if the current sample buffer is valid
   */
  bool IsValid() { return m_valid; }

  CAEChannelInfo GetChannelLayout();
  unsigned int   GetSampleRate();
  unsigned int   GetSampleCount();
  unsigned int   GetFrameCount();
  float*         GetSamples();
  bool           IsCompatible(const unsigned int sampleRate, const CAEChannelInfo &channelInfo);

private:
  std::string  m_filename;
  bool         m_valid;

  CAEChannelInfo m_channels    ,  m_outputChannels;
  unsigned int   m_sampleRate  ,  m_outputSampleRate;
  unsigned int   m_frameCount  ,  m_outputFrameCount;
  unsigned int   m_sampleCount ,  m_outputSampleCount;
  float         *m_samples     , *m_outputSamples;
};


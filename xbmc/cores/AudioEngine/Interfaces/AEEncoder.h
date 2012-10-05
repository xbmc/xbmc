#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DllAvCodec.h"
#include "AEAudioFormat.h"

/**
 * IAEEncoder interface for on the fly audio compression
 */
class IAEEncoder
{
public:
  /**
   * Constructor
   */
  IAEEncoder() {};

  /**
   * Destructor
   */
  virtual ~IAEEncoder() {};

  /**
   * Return true if the supplied format is compatible with the current open encoder.
   * @param format the format to compare
   * @return true if compatible, false if not
   */
  virtual bool IsCompatible(AEAudioFormat format) = 0;

  /**
   * Called to setup the encoder to accept data in the specified format
   * @param format the desired audio format, may be changed to suit the encoder
   * @return true on success, false on failure
   */
  virtual bool Initialize(AEAudioFormat &format) = 0;

  /**
   * Reset the encoder for new data
   */
  virtual void Reset() = 0;

  /**
   * Returns the bitrate of the encoder
   * @return bit rate in bits per second
   */
  virtual unsigned int GetBitRate() = 0;

  /**
   * Returns the CodecID of the encoder
   * @return the ffmpeg codec id
   */
  virtual CodecID GetCodecID() = 0;

  /**
   * Return the number of frames needed to encode
   * @return number of frames (frames * channels = samples * bits per sample = bytes)
   */
  virtual unsigned int GetFrames() = 0;

  /**
   * Encodes the supplied samples
   * @param data the PCM samples in float format
   * @param frames the number of audio frames in data (bytes / bits per sample = samples / channels = frames)
   * @return the number of samples consumed
   */
  virtual int Encode(float *data, unsigned int frames) = 0;

  /**
   * Get the encoded data
   * @param data return pointer to the buffer with the current encoded block
   * @return the size in bytes of *data
   */
  virtual int GetData(uint8_t **data) = 0;

  /**
   * Get the delay in seconds
   * @param bufferSize how much encoded data the caller has buffered to add to the delay
   * @return the delay in seconds including any un-fetched encoded data
   */
  virtual double GetDelay(unsigned int bufferSize) = 0;
};


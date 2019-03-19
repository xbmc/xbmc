/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AEAudioFormat.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

/**
 * IAEEncoder interface for on the fly audio compression
 */
class IAEEncoder
{
public:
  /**
   * Constructor
   */
  IAEEncoder() = default;

  /**
   * Destructor
   */
  virtual ~IAEEncoder() = default;

  /**
   * Return true if the supplied format is compatible with the current open encoder.
   * @param format the format to compare
   * @return true if compatible, false if not
   */
  virtual bool IsCompatible(const AEAudioFormat& format) = 0;

  /**
   * Called to setup the encoder to accept data in the specified format
   * @param format the desired audio format, may be changed to suit the encoder
   * @param allow_planar_input allow engine to use with planar formats
   * @return true on success, false on failure
   */
  virtual bool Initialize(AEAudioFormat &format, bool allow_planar_input = false) = 0;

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
   * Returns the AVCodecID of the encoder
   * @return the ffmpeg codec id
   */
  virtual AVCodecID GetCodecID() = 0;

  /**
   * Return the number of frames needed to encode
   * @return number of frames (frames * channels = samples * bits per sample = bytes)
   */
  virtual unsigned int GetFrames() = 0;

  /**
   * Encodes the supplied samples into a provided buffer
   * @param in the PCM samples encoder requested format
   * @param in_size input buffer size
   * @param output buffer
   * @param out_size output buffer size
   * @return size of encoded data
   */
  virtual int Encode (uint8_t *in, int in_size, uint8_t *out, int out_size) = 0;

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


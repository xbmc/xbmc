#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "DVDClock.h"


#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif
#include <vector>

extern "C" {
#include "libavcodec/avcodec.h"
}

struct AVStream;

class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

typedef struct stDVDAudioFrame
{
  uint8_t*          data[16];
  double            pts;
  double            duration;
  unsigned int      nb_frames;
  unsigned int      framesize;
  unsigned int      planes;

  int               channel_count;
  int               encoded_channel_count;
  CAEChannelInfo    channel_layout;
  enum AEDataFormat data_format;
  int               bits_per_sample;
  int               sample_rate;
  int               encoded_sample_rate;
  bool              passthrough;
} DVDAudioFrame;

class CDVDAudioCodec
{
public:

  CDVDAudioCodec() {}
  virtual ~CDVDAudioCodec() {}

  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;

  /*
   * returns bytes used or -1 on error
   *
   */
  virtual int Decode(uint8_t* pData, int iSize) = 0;

  /*
   * returns nr of bytes in decode buffer
   * the data is valid until the next Decode call
   */
  virtual int GetData(uint8_t** dst) = 0;

  /*
   * the data is valid until the next Decode call
   */
  virtual void GetData(DVDAudioFrame &frame)
  {
    frame.nb_frames = 0;
    frame.data_format           = GetDataFormat();
    frame.channel_count         = GetChannels();
    frame.framesize             = (CAEUtil::DataFormatToBits(frame.data_format) >> 3) * frame.channel_count;
    if(frame.framesize == 0)
      return;
    frame.nb_frames             = GetData(frame.data)/frame.framesize;
    frame.channel_layout        = GetChannelMap();
    frame.channel_count         = GetChannels();
    frame.planes                = AE_IS_PLANAR(frame.data_format) ? frame.channel_count : 1;
    frame.encoded_channel_count = GetEncodedChannels();
    frame.bits_per_sample       = CAEUtil::DataFormatToBits(frame.data_format);
    frame.sample_rate           = GetSampleRate();
    frame.encoded_sample_rate   = GetEncodedSampleRate();
    frame.passthrough           = NeedPassthrough();
    frame.pts                   = DVD_NOPTS_VALUE;
    // compute duration.
    if (frame.sample_rate)
      frame.duration = ((double)frame.nb_frames * DVD_TIME_BASE) / frame.sample_rate;
    else
      frame.duration = 0.0;
  }

  /*
   * resets the decoder
   */
  virtual void Reset() = 0;

  /*
   * returns the nr of channels for the decoded audio stream
   */
  virtual int GetChannels() = 0;

  /*
   * returns the nr of channels for the encoded audio stream
   */
  virtual int GetEncodedChannels() { return GetChannels(); }

  /*
   * returns the channel mapping
   */
  virtual CAEChannelInfo GetChannelMap() = 0;

  /*
   * returns the samplerate for the decoded audio stream
   */
  virtual int GetSampleRate() = 0;

  /*
   * returns the samplerate for the encoded audio stream
   */
  virtual int GetEncodedSampleRate() { return GetSampleRate(); }

  /*
   * returns the data format for the decoded audio stream (eg AE_FMT_S16LE)
   */
  virtual enum AEDataFormat GetDataFormat() = 0;

  /*
   * should return the average input bit rate 
   */
  virtual int GetBitRate() { return 0; }

  /*
   * returns if the codec requests to use passtrough
   */
  virtual bool NeedPassthrough() { return false; }

  /*
   * should return codecs name
   */
  virtual const char* GetName() = 0;

  /*
   * should return amount of data decoded has buffered in preparation for next audio frame
   */
  virtual int GetBufferSize() { return 0; }
};

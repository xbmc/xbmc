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
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#include "TimingConstants.h"

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
  uint8_t* data[16];
  double pts;
  bool hasTimestamp;
  double duration;
  unsigned int nb_frames;
  unsigned int framesize;
  unsigned int planes;

  AEAudioFormat format;
  int bits_per_sample;
  bool passthrough;
  AEAudioFormat audioFormat;
  enum AVAudioServiceType audio_service_type;
  enum AVMatrixEncoding matrix_encoding;
  int               profile;
} DVDAudioFrame;

class CDVDAudioCodec
{
public:

  CDVDAudioCodec(CProcessInfo &processInfo) : m_processInfo(processInfo) {}
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
   * returns -1 on error
   *
   */
  virtual int AddData(const DemuxPacket &packet) = 0;

  /*
   * returns nr of bytes in decode buffer
   * the data is valid until the next call
   */
  virtual int GetData(uint8_t** dst) = 0;

  /*
   * the data is valid until the next call
   */
  virtual void GetData(DVDAudioFrame &frame) = 0;

  /*
   * resets the decoder
   */
  virtual void Reset() = 0;

  /*
   * returns the format for the audio stream
   */
  virtual AEAudioFormat GetFormat() = 0;

  /*
   * should return the average input bit rate
   */
  virtual int GetBitRate() { return 0; }

  /*
   * returns if the codec requests to use passthrough
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

  /*
   * should return the ffmpeg matrix encoding type
   */
  virtual enum AVMatrixEncoding GetMatrixEncoding() { return AV_MATRIX_ENCODING_NONE; }

  /*
   * should return the ffmpeg audio service type
   */
  virtual enum AVAudioServiceType GetAudioServiceType() { return AV_AUDIO_SERVICE_TYPE_MAIN; }

  /*
   * should return the ffmpeg profile value
   */
  virtual int GetProfile() { return 0; }

protected:
  CProcessInfo &m_processInfo;
};

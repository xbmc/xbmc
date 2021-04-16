/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
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
  unsigned int framesOut;
  unsigned int framesize;
  unsigned int planes;

  AEAudioFormat format;
  int bits_per_sample;
  bool passthrough;
  enum AVAudioServiceType audio_service_type;
  enum AVMatrixEncoding matrix_encoding;
  int profile;
  bool hasDownmix;
  double centerMixLevel;
} DVDAudioFrame;

class CDVDAudioCodec
{
public:

  explicit CDVDAudioCodec(CProcessInfo &processInfo) : m_processInfo(processInfo) {}
  virtual ~CDVDAudioCodec() = default;

  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;

  /*
   * returns false on error
   *
   */
  virtual bool AddData(const DemuxPacket &packet) = 0;

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
  virtual std::string GetName() = 0;

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

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "utils/RingBuffer.h"

struct AEAudioFormat;
class CFileItem;
class ICodec;

#define PACKET_SIZE 3840    // audio packet size - we keep 1 in reserve for gapless playback
                            // using a multiple of 1, 2, 3, 4, 5, 6 to guarantee track alignment
                            // note that 7 or higher channels won't work too well.

#define INPUT_SIZE PACKET_SIZE * 3      // input data size we read from the codecs at a time
                                        // * 3 to allow 24 bit audio

#define OUTPUT_SAMPLES PACKET_SIZE      // max number of output samples
#define INPUT_SAMPLES  PACKET_SIZE      // number of input samples (distributed over channels)

#define STATUS_NO_FILE  0
#define STATUS_QUEUING  1
#define STATUS_QUEUED   2
#define STATUS_PLAYING  3
#define STATUS_ENDING   4
#define STATUS_ENDED    5

// return codes from decoders
#define RET_ERROR -1
#define RET_SUCCESS 0
#define RET_SLEEP 1

class CAudioDecoder
{
public:
  CAudioDecoder();
  ~CAudioDecoder();

  bool Create(const CFileItem &file, int64_t seekOffset);
  void Destroy();

  int ReadSamples(int numsamples);

  bool CanSeek();
  int64_t Seek(int64_t time);
  int64_t TotalTime();
  void SetTotalTime(int64_t time);
  void Start() { m_canPlay = true;}; // cause a pre-buffered stream to start.
  int GetStatus() { return m_status; }
  void SetStatus(int status) { m_status = status; }

  AEAudioFormat GetFormat();
  unsigned int GetChannels();
  // Data management
  unsigned int GetDataSize(bool checkPktSize);
  void *GetData(unsigned int samples);
  uint8_t* GetRawData(int &size);
  ICodec *GetCodec() const { return m_codec; }
  float GetReplayGain(float &peakVal);

private:
  // pcm buffer
  CRingBuffer m_pcmBuffer;

  // output buffer (for transferring data from the Pcm Buffer to the rest of the audio chain)
  float m_outputBuffer[OUTPUT_SAMPLES];

  // input buffer (for transferring data from the Codecs to our Pcm Ringbuffer
  uint8_t m_pcmInputBuffer[INPUT_SIZE];
  float m_inputBuffer[INPUT_SAMPLES];

  uint8_t *m_rawBuffer;
  int m_rawBufferSize;

  // status
  bool m_eof;
  int m_status;
  bool m_canPlay;

  // the codec we're using
  ICodec* m_codec;

  CCriticalSection m_critSection;
};

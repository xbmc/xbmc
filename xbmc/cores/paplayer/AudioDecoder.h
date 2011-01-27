#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "threads/Thread.h"
#include "ICodec.h"
#include "threads/CriticalSection.h"
#include "utils/RingBuffer.h"

class CFileItem;

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

  bool Create(const CFileItem &file, __int64 seekOffset, unsigned int nBufferSize);
  void Destroy();

  int ReadSamples(int numsamples);

  bool CanSeek() { if (m_codec) return m_codec->CanSeek(); else return false; };
  __int64 Seek(__int64 time);
  __int64 TotalTime();
  void Start() { m_canPlay = true;}; // cause a pre-buffered stream to start.
  int GetStatus() { return m_status; };
  void SetStatus(int status) { m_status = status; };

  void GetDataFormat(unsigned int *channels, unsigned int *samplerate, unsigned int *bitspersample);
  unsigned int GetChannels() { if (m_codec) return m_codec->m_Channels; else return 0; };
  // Data management
  unsigned int GetDataSize();
  void *GetData(unsigned int size);
  void PrefixData(void *data, unsigned int size);
  ICodec *GetCodec() const { return m_codec; }

private:
  void ProcessAudio(float *data, int numsamples);
  // ReadPCMSamples() - helper to convert PCM (short/byte) to float
  int ReadPCMSamples(float *buffer, int numsamples, int *actualsamples);
  float GetReplayGain();

  // block size (number of bytes per sample * number of channels)
  int m_blockSize;
  // pcm buffer
  CRingBuffer m_pcmBuffer;

  // output buffer (for transferring data from the Pcm Buffer to the rest of the audio chain)
  float m_outputBuffer[OUTPUT_SAMPLES];

  // gapless buffer (left over samples from the previous audio decoder)
  float m_gaplessBuffer[OUTPUT_SAMPLES];
  unsigned int m_gaplessBufferSize;

  // input buffer (for transferring data from the Codecs to our Pcm Ringbuffer
  BYTE m_pcmInputBuffer[INPUT_SIZE];
  float m_inputBuffer[INPUT_SAMPLES];

  // status
  bool    m_eof;
  int     m_status;
  bool    m_canPlay;

  // the codec we're using
  ICodec*          m_codec;

  CCriticalSection m_critSection;
};

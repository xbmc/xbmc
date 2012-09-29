/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DVDAudioCodecLibMad.h"
#include "DVDStreamInfo.h"

CDVDAudioCodecLibMad::CDVDAudioCodecLibMad() : CDVDAudioCodec()
{
  m_bInitialized = false;
  
  memset(&m_synth, 0, sizeof(m_synth));
  memset(&m_stream, 0, sizeof(m_stream));
  memset(&m_frame, 0, sizeof(m_frame));
  
  m_iDecodedDataSize = 0;
  
  m_iSourceSampleRate = 0;
  m_iSourceChannels = 0;
  m_iSourceBitrate = 0;
  
  m_iInputBufferSize = 0;
}

CDVDAudioCodecLibMad::~CDVDAudioCodecLibMad()
{
  Dispose();
}

bool CDVDAudioCodecLibMad::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (m_bInitialized) Dispose();

  if (!m_dll.Load())
    return false;

  memset(&m_synth, 0, sizeof(m_synth));
  memset(&m_stream, 0, sizeof(m_stream));
  memset(&m_frame, 0, sizeof(m_frame));

  m_dll.mad_synth_init(&m_synth);
  m_dll.mad_stream_init(&m_stream);
  m_dll.mad_frame_init(&m_frame);
  m_stream.options = MAD_OPTION_IGNORECRC;

  m_iDecodedDataSize = 0;

  m_iSourceSampleRate = 0;
  m_iSourceChannels = 0;
  m_iSourceBitrate = 0;

  m_iInputBufferSize = 0;

  m_bInitialized = true;
  return true;
}

void CDVDAudioCodecLibMad::Dispose()
{
  if (m_bInitialized)
  {
    mad_synth_finish (&m_synth);
    m_dll.mad_stream_finish(&m_stream);
    m_dll.mad_frame_finish (&m_frame);
    m_bInitialized = false;
  }
}

int CDVDAudioCodecLibMad::Decode(BYTE* pData, int iSize)
{
  BYTE* pBuffer = m_inputBuffer;
  //int iBufferSize = iSize;
  bool bFullOutputBuffer = false;

  m_iDecodedDataSize = 0;

  // m_inputBuffer should always have room here for extra bytes
  int iBytesFree = MAD_INPUT_SIZE - m_iInputBufferSize;
  int iBytesUsed = std::min(iSize, iBytesFree);

  // copy data into our buffer for decoding
  memcpy(m_inputBuffer + m_iInputBufferSize, pData, iBytesUsed);
  m_iInputBufferSize += iBytesUsed;

  if (m_bInitialized)
  {
    m_dll.mad_stream_buffer(&m_stream, pBuffer, m_iInputBufferSize);

    while (true)
    {
      if (bFullOutputBuffer || m_dll.mad_frame_decode(&m_frame, &m_stream) != MAD_ERROR_NONE)
      {
        if (m_stream.error == MAD_ERROR_BUFLEN || bFullOutputBuffer)
        {
          // need more data
          if (m_stream.next_frame)
          {
            // we have some data left, move remaining data to beginning of buffer
            m_iInputBufferSize = m_stream.bufend - m_stream.next_frame;
            memmove(m_inputBuffer, m_stream.next_frame, m_iInputBufferSize);
          }
          else
          {
            m_iInputBufferSize = 0;
          }

          return iBytesUsed;
        }

        // sync stream
        if (m_stream.next_frame)
        {
          m_iInputBufferSize = m_stream.bufend - m_stream.next_frame;
          pBuffer = (BYTE*)m_stream.next_frame;
        }

        if (m_iInputBufferSize <= 0)
        {
          return iBytesUsed;
        }

        // buffer again after a sync
        m_dll.mad_stream_buffer(&m_stream, pBuffer, m_iInputBufferSize);
      }
      else
      {
        // decoded some data

        m_iSourceSampleRate = m_frame.header.samplerate;
        m_iSourceChannels   = (m_frame.header.mode == MAD_MODE_SINGLE_CHANNEL) ? 1 : 2;
        m_iSourceBitrate    = m_frame.header.bitrate;
        m_dll.mad_synth_frame(&m_synth, &m_frame);

        {
          unsigned int nchannels, nsamples;
          mad_fixed_t const* left_ch, *right_ch;
          struct mad_pcm* pcm = &m_synth.pcm;
          float* output = (float*)(m_decodedData + m_iDecodedDataSize);

          nchannels = pcm->channels;
          nsamples  = pcm->length;
          left_ch   = pcm->samples[0];
          right_ch  = pcm->samples[1];

          unsigned int iSize = nsamples * sizeof(float) * nchannels;
          if (iSize < (MAD_DECODED_SIZE - m_iDecodedDataSize))
          {
            while (nsamples--)
            {
              // output sample(s) in float
              *output++ = mad_f_todouble(*left_ch++);   	
              if (nchannels == 2)
              {
                *output++ = mad_f_todouble(*right_ch++);
              }
            }
            m_iDecodedDataSize += iSize;
          }
	
          if (iSize > (MAD_DECODED_SIZE - m_iDecodedDataSize))
          {
            // next audio frame is not going to fit
            bFullOutputBuffer = true;
          }
        }
      }
    }
  }
  return 0;
}

int CDVDAudioCodecLibMad::GetData(BYTE** dst)
{
  *dst = m_decodedData;
  return m_iDecodedDataSize;
}

void CDVDAudioCodecLibMad::Reset()
{
  if (m_bInitialized)
  {
    mad_synth_finish(&m_synth);
    m_dll.mad_stream_finish(&m_stream);
    m_dll.mad_frame_finish(&m_frame);

    m_dll.mad_synth_init(&m_synth);
    m_dll.mad_stream_init(&m_stream);
    m_dll.mad_frame_init(&m_frame);
    m_stream.options = MAD_OPTION_IGNORECRC;

    m_iDecodedDataSize = 0;
    m_iSourceSampleRate = 0;
    m_iSourceChannels = 0;
    m_iSourceBitrate = 0;

    m_iInputBufferSize = 0;
  }
}

CAEChannelInfo CDVDAudioCodecLibMad::GetChannelMap()
{
  assert(m_iSourceChannels > 0 && m_iSourceChannels < 3);

  static enum AEChannel map[2][3] = {
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL}
  };

  return CAEChannelInfo(map[m_iSourceChannels -1]);
}


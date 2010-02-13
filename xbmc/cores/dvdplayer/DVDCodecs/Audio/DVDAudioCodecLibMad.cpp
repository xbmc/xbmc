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

#include "DVDAudioCodecLibMad.h"
#include "DVDStreamInfo.h"

static inline signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

CDVDAudioCodecLibMad::CDVDAudioCodecLibMad() : CDVDAudioCodec()
{
  m_bInitialized = false;
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
  int iBytesUsed = iBytesFree;
  if (iBytesUsed > iSize) iBytesUsed = iSize;

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
	      m_iSourceChannels = (m_frame.header.mode == MAD_MODE_SINGLE_CHANNEL) ? 1 : 2;
	      m_iSourceBitrate = m_frame.header.bitrate;

  /*
              switch (this->frame.header.layer) {
              case MAD_LAYER_I:
                _x_meta_info_set_utf8(this->xstream, XINE_META_INFO_AUDIOCODEC,
                  "MPEG audio layer 1 (lib: MAD)");
                break;
              case MAD_LAYER_II:
                _x_meta_info_set_utf8(this->xstream, XINE_META_INFO_AUDIOCODEC,
                  "MPEG audio layer 2 (lib: MAD)");
                break;
              case MAD_LAYER_III:
                _x_meta_info_set_utf8(this->xstream, XINE_META_INFO_AUDIOCODEC,
                  "MPEG audio layer 3 (lib: MAD)");
                break;
              default:
                _x_meta_info_set_utf8(this->xstream, XINE_META_INFO_AUDIOCODEC,
                  "MPEG audio (lib: MAD)");
              }
  */
        m_dll.mad_synth_frame(&m_synth, &m_frame);

        {
          unsigned int nchannels, nsamples;
	        mad_fixed_t const* left_ch, *right_ch;
	        struct mad_pcm* pcm = &m_synth.pcm;
	        unsigned __int16* output = (unsigned __int16*)(m_decodedData + m_iDecodedDataSize);

          nchannels = pcm->channels;
          nsamples  = pcm->length;
	        left_ch   = pcm->samples[0];
	        right_ch  = pcm->samples[1];

          int iSize = nsamples * 2;
          if (nchannels == 2) iSize += nsamples * 2;

          if (iSize < (MAD_DECODED_SIZE - m_iDecodedDataSize))
          {
	          while (nsamples--)
	          {
	            // output sample(s) in 16-bit signed little-endian PCM
	            *output++ = scale(*left_ch++);
        	
	            if (nchannels == 2)
	            {
	              *output++ = scale(*right_ch++);
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

enum PCMChannels* CDVDAudioCodecLibMad::GetChannelMap()
{
  static enum PCMChannels map[2][2] = {
    {PCM_FRONT_CENTER},
    {PCM_FRONT_LEFT, PCM_FRONT_RIGHT}
  };

  return map[m_iSourceChannels - 1];
}


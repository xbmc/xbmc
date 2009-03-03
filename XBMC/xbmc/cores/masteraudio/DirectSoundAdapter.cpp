/*
 *      Copyright (C) 2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DirectSoundAdapter.h"
// TODO: Remove WaveOut test code
#include "../AudioRenderers/WaveFileRenderer.h"
#include "Settings.h"

CWaveFileRenderer waveOut;
bool writeWave = false;

CDirectSoundAdapter::CDirectSoundAdapter() : 
  m_pRenderer(NULL),
  m_TotalBytesReceived(0),
  m_pInputSlice(NULL)
{

}

CDirectSoundAdapter::~CDirectSoundAdapter()
{
  Close();
}

// IAudioSink
MA_RESULT CDirectSoundAdapter::TestInputFormat(CStreamDescriptor* pDesc)
{
  // TODO: Implement properly

  return MA_SUCCESS;
}

MA_RESULT CDirectSoundAdapter::SetInputFormat(CStreamDescriptor* pDesc)
{
  if(m_pRenderer)
    Close();

  if(!pDesc)  // NULL indicates 'Clear'
    return MA_SUCCESS;

  CStreamAttributeCollection* pAtts = pDesc->GetAttributes();
  if(!pAtts)
    return MA_ERROR;

  unsigned int format;
  unsigned int channels;
  unsigned int bitsPerSample;
  unsigned int samplesPerSecond;
  
  // Require PCM stream for now
  // TODO: Write helper method to fetch attributes
  if((MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_STREAM_FORMAT,(int*)&format)) || (format != MA_STREAM_FORMAT_PCM))
    return MA_ERROR;

  if(MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_CHANNELS,(int*)&channels))
    return MA_ERROR;

  if(MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_BITDEPTH,(int*)&bitsPerSample))
    return MA_ERROR;

  if(MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_SAMPLESPERSEC,(int*)&samplesPerSecond))
    return MA_ERROR;

  m_pRenderer = CAudioRendererFactory::Create(NULL,channels, samplesPerSecond, bitsPerSample, false,"",false,false);
  if(!m_pRenderer)
    return MA_ERROR;

  if (writeWave)
    waveOut.Open(g_stSettings.m_logFolder + "\\waveout.wav",m_pRenderer->GetChunkLen(),samplesPerSecond);

  return m_OutputBuffer.Initialize(m_pRenderer->GetChunkLen()*2) ? MA_SUCCESS : MA_ERROR; // TODO: This is wasteful.
}

MA_RESULT CDirectSoundAdapter::AddSlice(audio_slice* pSlice)
{
  // CLog::Log(LOGDEBUG,"<------MASTER_AUDIO:DirectSoundAdapter - trying slice %I64d, len=%d", pSlice->header.id, pSlice->header.data_len);

  // Get some sizes to prevent duplicate calls
  size_t newLen = pSlice->header.data_len;
  size_t bufferLen = m_OutputBuffer.GetLen();
  size_t chunkLen = m_pRenderer->GetChunkLen();

  // See if we can send some data to the renderer
  if (bufferLen > chunkLen)
  {
    size_t bytesWritten = m_pRenderer->AddPackets((unsigned char*)m_OutputBuffer.GetData(NULL), bufferLen);
    size_t bytesLeft = bufferLen - bytesWritten;
    if (bytesWritten) // The renderer took some data
    {
      if (writeWave)
      {
        waveOut.WriteData((short*)m_OutputBuffer.GetData(NULL), bufferLen);
        waveOut.Save();
      }

      if (bytesLeft)
        m_OutputBuffer.ShiftUp(bytesWritten);
      else
        m_OutputBuffer.Empty();
      bufferLen = bytesLeft; 
    }
    else  // The renderer was already full and we have some data to give. No need to accept more data.
      return MA_BUSYORFULL;
  }

  // If we still have at least one chunk, don't accept any more data
  if (bufferLen > chunkLen)
    return MA_BUSYORFULL;

  // See if we have space to store the incoming data
  if (bufferLen + newLen > m_OutputBuffer.GetMaxLen())  // TODO: Should we dynamically grow the buffer? Split it up and make multiple calls?
    return MA_ERROR;  // This will likely cause us to clog up and hang...

  // Save the new slice data
  m_OutputBuffer.Write(pSlice->get_data(), newLen);

  // CLog::Log(LOGDEBUG,"vv MASTER_AUDIO:DirectSoundAdapter - RENDERING (slice %I64d, len=%d)", pSlice->header.id, pSlice->header.data_len);
  m_TotalBytesReceived += pSlice->header.data_len;

  delete pSlice;  // We are the end of the road for this slice

  return MA_SUCCESS;
}

// IRenderingControl
void CDirectSoundAdapter::Play()
{
  if(m_pRenderer)
    m_pRenderer->Resume();
}

void CDirectSoundAdapter::Stop()
{
  if(m_pRenderer)
    m_pRenderer->Stop();

  CLog::Log(LOGINFO, "MasterAudio:DirectSoundAdapter: Stopped - Total Bytes Received = %I64d",m_TotalBytesReceived);
}

void CDirectSoundAdapter::Pause()
{
  if(m_pRenderer)
    m_pRenderer->Pause();
}

void CDirectSoundAdapter::Resume()
{
  if(m_pRenderer)
    m_pRenderer->Resume();
}

void CDirectSoundAdapter::SetVolume(long vol)
{
  if(m_pRenderer)
    m_pRenderer->SetCurrentVolume(vol);
}

// IMixerChannel
void CDirectSoundAdapter::Close()
{
  // Reset the state of the channel

  delete m_pInputSlice; // We don't need it and can't give it away

  if(m_pRenderer)
    m_pRenderer->Deinitialize();

  delete m_pRenderer;
  m_pRenderer = NULL;

  m_OutputBuffer.Empty();
}

bool CDirectSoundAdapter::IsIdle()
{
  return (m_pRenderer == NULL);
}
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
#include "Util/WaveFileRenderer.h"
#include "Settings.h"

CWaveFileRenderer waveOut;
bool writeWave = false;

CDirectSoundAdapter::CDirectSoundAdapter() : 
  m_pRenderer(NULL),
  m_TotalBytesReceived(0),
  m_pInputSlice(NULL),
  m_ChunkLen(0),
  m_BufferOffset(0)
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
  if (m_pRenderer)
    Close();

  // pDesc == NULL indicates 'Clear'
  if (!pDesc)
  {
    Flush();
    return MA_SUCCESS;
  }

  CStreamAttributeCollection* pAtts = pDesc->GetAttributes();
  if (!pAtts)
    return MA_ERROR;

  unsigned int format = 0;
  unsigned int channels = 0;
  unsigned int bitsPerSample = 0;
  unsigned int samplesPerSecond = 0;
  unsigned int encoding = 0;
 
  // TODO: Write helper method to fetch attributes
  if (MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_STREAM_FORMAT,(int*)&format))
    return MA_ERROR;

  // TODO: Find a more elegant way to configure the renderer
  m_pRenderer = NULL;
  switch (format)
  {
  case MA_STREAM_FORMAT_PCM:
    if (MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_CHANNELS,(int*)&channels))
      return MA_ERROR;
    if (MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_BITDEPTH,(int*)&bitsPerSample))
      return MA_ERROR;
    if (MA_SUCCESS != pAtts->GetInt(MA_ATT_TYPE_SAMPLESPERSEC,(int*)&samplesPerSecond))
      return MA_ERROR;
    m_pRenderer = CAudioRendererFactory::Create(NULL,channels, samplesPerSecond, bitsPerSample, false,"",false,false);
    break;
  case MA_STREAM_FORMAT_ENCODED:
    if ((MA_SUCCESS == pAtts->GetInt(MA_ATT_TYPE_ENCODING,(int*)&encoding)) && (encoding == MA_STREAM_ENCODING_AC3 || encoding == MA_STREAM_ENCODING_DTS))
    {
      m_pRenderer = CAudioRendererFactory::Create(NULL, 2, 48000, 16, false, "", false, true);
      break;
    }
  default:
    break;  // Unsupported format
  }
  if (!m_pRenderer)
    return MA_ERROR;

  m_ChunkLen = m_pRenderer->GetChunkLen();

  if (writeWave)
    waveOut.Open(g_stSettings.m_logFolder + "\\waveout.wav",m_pRenderer->GetChunkLen(),samplesPerSecond);

  return MA_SUCCESS;
}

MA_RESULT CDirectSoundAdapter::GetInputProperties(audio_data_transfer_props* pProps)
{
  if (!pProps || ! m_pRenderer)
    return MA_ERROR;

  pProps->transfer_alignment = m_ChunkLen; // This is the smallest size we can write to the renderer, so we don't want anything smaller
  pProps->max_transfer_size = 0;  // Anything goes
  pProps->preferred_transfer_size = m_ChunkLen; // We prefer one renderer chunk at a time

  return MA_SUCCESS;
}

MA_RESULT CDirectSoundAdapter::AddSlice(audio_slice* pSlice)
{
  // TODO: manage cache/buffer level to prevent underruns
  if (!m_pRenderer || !pSlice)
    return MA_ERROR;

  if (pSlice->header.data_len % m_ChunkLen)
    return MA_ERROR; // Data is misaligned

  size_t newLen = pSlice->header.data_len;

  // Try send some data to the renderer
  if (m_pInputSlice && m_BufferOffset) // We have a leftover slice from last time
  {
    size_t bytesWritten = m_pRenderer->AddPackets(m_pInputSlice->get_data() + m_BufferOffset, m_pInputSlice->header.data_len - m_BufferOffset);
    m_BufferOffset += bytesWritten;
    // TODO: How should we handle misalignment caused by reads by the renderer? Can we?
    if (m_BufferOffset >= m_pInputSlice->header.data_len) // We are done with this one
    {
      delete m_pInputSlice;
      m_pInputSlice = NULL;
      m_BufferOffset = 0;
    }
    else
      return MA_BUSYORFULL; // We still have data to transfer
  }

  size_t bytesWritten = m_pRenderer->AddPackets(pSlice->get_data(), pSlice->header.data_len);
  if (!bytesWritten) // The renderer is not accepting data
    return MA_BUSYORFULL;

  m_TotalBytesReceived += pSlice->header.data_len;
  if (bytesWritten < pSlice->header.data_len) // Save it for later
  {
    m_BufferOffset = bytesWritten;
    m_pInputSlice = pSlice; // Store the provided slice
  }
  else // We are done with this one
  {
    delete pSlice;
  }

  return MA_SUCCESS;
}

float CDirectSoundAdapter::GetMaxLatency()
{
  if (!m_pRenderer)
    return 0;

  // TODO: Include internal buffer

  return m_pRenderer->GetDelay();
}

void CDirectSoundAdapter::Flush()
{
  if (m_pRenderer)
    m_pRenderer->Stop();

  delete m_pInputSlice; // We don't need it and can't give it away
  m_pInputSlice = NULL;
  m_BufferOffset = 0;
}

bool CDirectSoundAdapter::Drain(unsigned int timeout)
{
  // TODO: Find a way to honor the timeout
  // TODO: Push remaining data into the renderer

  if (m_pRenderer)
    m_pRenderer->WaitCompletion();

  return true;
}

// IRenderingControl
void CDirectSoundAdapter::Play()
{
  if (m_pRenderer)
    m_pRenderer->Resume();
}

void CDirectSoundAdapter::Stop()
{
  if (writeWave)
    waveOut.Close(true);

  if (m_pRenderer)
    m_pRenderer->Stop();

  delete m_pInputSlice;
  m_pInputSlice = NULL;
  m_BufferOffset = 0;

  CLog::Log(LOGINFO, "MasterAudio:DirectSoundAdapter: Stopped - Total Bytes Received = %I64d",m_TotalBytesReceived);
}

void CDirectSoundAdapter::Pause()
{
  if (m_pRenderer)
    m_pRenderer->Pause();
}

void CDirectSoundAdapter::Resume()
{
  if (m_pRenderer)
    m_pRenderer->Resume();
}

void CDirectSoundAdapter::SetVolume(long vol)
{
  if (m_pRenderer)
    m_pRenderer->SetCurrentVolume(vol);
}

// IMixerChannel
void CDirectSoundAdapter::Close()
{
  Flush();

  if (m_pRenderer)
    m_pRenderer->Deinitialize();

  delete m_pRenderer;
  m_pRenderer = NULL;
}

bool CDirectSoundAdapter::IsIdle()
{
  return (m_pRenderer == NULL);
}
/*
 *      Copyright (C) 2009 Team XBMC
 *      http://www.xbmc.org
 *      Portions of WAV IO based on code by Evan Merz (www.thisisnotalabel.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "stdafx.h"
#include "WaveFileRenderer.h"


CWaveFileRenderer::CWaveFileRenderer() :
  m_pOutputBuffer(NULL),
  m_OutputBufferSize(0),
  m_OutputBufferPos(0)
{
}

CWaveFileRenderer::~CWaveFileRenderer()
{
  // Free data buffer
  if (m_pOutputBuffer)
    delete m_pOutputBuffer;

  // Close output file
  if (m_OutputStream.is_open())
    m_OutputStream.close();
}

bool CWaveFileRenderer::Open(const char* filePath, unsigned int bufferSize, unsigned int sampleRate)
{
  if(m_OutputStream.is_open())
    return false;

  // Open an output filestream
  m_OutputStream.open(filePath, ios::out | ios::binary);

  m_FileHeader.chunkId = 0x46464952; // "RIFF"  
  m_FileHeader.chunkSize = 0x00000000; // Placeholder.  Should equal 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
  m_FileHeader.chunkFormat = 0x45564157 ; // "WAVE"
  m_FileHeader.subChunk1Id = 0x20746D66 ; // "fmt "
  m_FileHeader.subChunk1Size = 16;
  m_FileHeader.audioFormat = 1; // PCM w/linear quantization (no compression)
  m_FileHeader.numChannels = 2;
  m_FileHeader.samplesPerSec = sampleRate;
  m_FileHeader.bitsPerSample = 16;
  m_FileHeader.bytesPerSec = m_FileHeader.samplesPerSec * m_FileHeader.numChannels * (m_FileHeader.bitsPerSample >> 3);
  m_FileHeader.blockAlign = m_FileHeader.numChannels * (m_FileHeader.bitsPerSample >> 3);
  m_FileHeader.subChunk2Id = 0x61746164; // "data"
  m_FileHeader.subChunk2Size = 0x00000000; // Placeholder.  Should equal the total 'data' size (NumSamples * NumChannels * BitsPerSample/8)
  
  // Write the RIFF Header
  m_OutputStream.write ((const char*)&m_FileHeader, sizeof(m_FileHeader));

  // Setup output byffer
  m_OutputBufferSize = bufferSize;
  m_pOutputBuffer = new BYTE[m_OutputBufferSize];

  return true;
}

unsigned int CWaveFileRenderer::GetSpace()
{
  return m_OutputBufferSize - m_OutputBufferPos;
}

unsigned int CWaveFileRenderer::WriteData(short* pSamples, size_t len)
{
  if (!m_OutputStream.is_open())
    return 0;

  if (m_OutputBufferPos + len > m_OutputBufferSize)
    len = m_OutputBufferSize - m_OutputBufferPos;

  memcpy(m_pOutputBuffer + m_OutputBufferPos, pSamples, len);

  m_OutputBufferPos += len;
  m_FileHeader.subChunk2Size += len;  // Update file header information
  CLog::DebugLog("WaveFileRenderer: Wrote %d bytes to buffer. Buffer pos = %d, 'data' chunklen = %d.",len, m_OutputBufferPos, m_FileHeader.subChunk2Size);

  return len;
}

// The file should be playable after this is called
bool CWaveFileRenderer::Save()
{
  if (!m_OutputStream.is_open())
    return false;

  // Write the RIFF Header
  UpdateHeader();
  CLog::DebugLog("WaveFileRenderer: Saving %d bytes to file. 'data' chunklen = %d.",m_OutputBufferPos, m_FileHeader.subChunk2Size);

  // Write cached data to file
  m_OutputStream.write((const char*)m_pOutputBuffer, m_OutputBufferPos);

  m_OutputBufferPos = 0;

  return true;
}

void CWaveFileRenderer::Close(bool saveData)
{
  if (!m_OutputStream.is_open())
    return;
    
  if (saveData)
    Save();

  m_OutputStream.close();

  m_OutputBufferSize = 0;
  m_OutputBufferPos = 0;

  if (m_pOutputBuffer)
    delete m_pOutputBuffer;

  m_pOutputBuffer = NULL;
}

// Private

void CWaveFileRenderer::UpdateHeader()
{
  // Update file header
  m_FileHeader.chunkSize = 4 + (8 + m_FileHeader.subChunk1Size) + (8 + m_FileHeader.subChunk2Size);

  long pos = m_OutputStream.tellp();
  m_OutputStream.seekp (0, ios::beg); 
  m_OutputStream.write ((const char*)&m_FileHeader, sizeof(m_FileHeader));

  m_OutputStream.seekp(pos);
}
/*
 *      Copyright (C) 2009 phi2039
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

#ifndef __WAVE_FILE_RENDERER_H__
#define __WAVE_FILE_RENDERER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <iostream>
#include <fstream>

using namespace std;

typedef struct tag_WAVE_FILE_HEADER
{
  // Chunk Header
  unsigned int chunkId;
  unsigned int chunkSize;
  unsigned int chunkFormat;
  // Sub-Chunk 1 (Format)
  unsigned int subChunk1Id;
  unsigned int subChunk1Size;
  unsigned short audioFormat;
  unsigned short numChannels;
  unsigned int samplesPerSec;
  unsigned int bytesPerSec;  
  unsigned short blockAlign; 
  unsigned short bitsPerSample;
  // Sub-Chunk 2 (Data)
  unsigned int subChunk2Id;
  unsigned int subChunk2Size;
} WAVEFILEHEADER, *LPWAVEFILEHEADER;

class CWaveFileRenderer
{
public:
  CWaveFileRenderer();
  virtual ~CWaveFileRenderer();
  bool Open(const char* filePath, unsigned int bufferSize, unsigned int sampleRate);
  unsigned int GetSpace();
  unsigned int WriteData(short* pSamples, size_t len);
  bool Save();
  void Close(bool saveData);

private:
  fstream m_OutputStream;
  BYTE* m_pOutputBuffer;
  unsigned int m_OutputBufferSize;
  unsigned int m_OutputBufferPos;

  WAVEFILEHEADER m_FileHeader;

  void UpdateHeader();
};

#endif __WAVE_FILE_RENDERER_H__
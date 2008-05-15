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

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "FileSystem/File.h"
#include "RingHoldBuffer.h"

// A threaded file reader class that reads ahead of the current file position
// using a separate thread.
class CFileReader : public CThread
{
public:
  CFileReader();
  virtual ~CFileReader();

  void Initialize(unsigned int bufferSize);
  virtual bool Open(const CStdString &strFile, bool autoBuffer = true, bool preBuffer = false);
  virtual void Close();
  virtual void StartBuffering();
  virtual int Read(void *out, __int64 size);
  virtual __int64 GetPosition();
  virtual __int64 Seek(__int64 pos, int whence = SEEK_SET);
  virtual __int64 GetLength();
  virtual bool SkipNext();
  unsigned int GetChunkSize() {return m_chunk_size;}
  int GetCacheLevel();
  bool CanSeek();

  EventHandler OnClear;
protected:
  // thread functions
  virtual void OnStartup() {}
  virtual void Process();
  virtual void OnExit() {};

private:
  int BufferChunk();
  void OnClearEvent();

  XFILE::CFile   m_file;
  __int64 m_bufferedDataPos;      // the position our client thinks we're at

  bool    m_readError;

  CCriticalSection m_fileLock;
  
  __int64 m_FileLength;
  unsigned int m_chunk_size;
  char* m_chunkBuffer;          // buffer that we read chunks of our file into.
  CRingHoldBuffer m_ringBuffer; // ring buffer that holds our read-in data
};

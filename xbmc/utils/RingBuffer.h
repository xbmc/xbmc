#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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

#include "threads/CriticalSection.h"
#include <sstream>

class CRingBuffer
{
  CCriticalSection m_critSection;
  char *m_buffer;
  uint64_t m_size;
  uint64_t m_readPtr;
  uint64_t m_writePtr;
  unsigned int m_fillCount;
public:
  CRingBuffer();
  ~CRingBuffer();
  bool Create(uint64_t size);
  void Destroy();
  void Clear();
  bool ReadData(char *buf, unsigned int size);
  bool ReadData(CRingBuffer &rBuf, unsigned int size);
  bool WriteData(const char *buf, unsigned int size);
  bool WriteData(CRingBuffer &rBuf, unsigned int size);
  bool SkipBytes(int skipSize);
  bool Append(CRingBuffer &rBuf);
  bool Copy(CRingBuffer &rBuf);
  char *getBuffer();
  uint64_t getSize();
  uint64_t getReadPtr() const;
  uint64_t getWritePtr();
  unsigned int getMaxReadSize();
  unsigned int getMaxWriteSize();
};

#pragma once
/*
 *      Copyright (C) 2010-2012 Team XBMC
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

#include "threads/CriticalSection.h"

class CRingBuffer
{
  CCriticalSection m_critSection;
  char *m_buffer;
  unsigned int m_size;
  unsigned int m_readPtr;
  unsigned int m_writePtr;
  unsigned int m_fillCount;
public:
  CRingBuffer();
  ~CRingBuffer();
  bool Create(unsigned int size);
  void Destroy();
  void Clear();
  bool ReadData(char *buf, unsigned int size);
  bool ReadData(CRingBuffer &rBuf, unsigned int size);
  bool WriteData(char *buf, unsigned int size);
  bool WriteData(CRingBuffer &rBuf, unsigned int size);
  bool SkipBytes(int skipSize);
  bool Append(CRingBuffer &rBuf);
  bool Copy(CRingBuffer &rBuf);
  char *getBuffer();
  unsigned int getSize();
  unsigned int getReadPtr();
  unsigned int getWritePtr();
  unsigned int getMaxReadSize();
  unsigned int getMaxWriteSize();
};

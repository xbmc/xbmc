/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  bool WriteData(const char *buf, unsigned int size);
  bool WriteData(CRingBuffer &rBuf, unsigned int size);
  bool SkipBytes(int skipSize);
  bool Append(CRingBuffer &rBuf);
  bool Copy(CRingBuffer &rBuf);
  char *getBuffer();
  unsigned int getSize();
  unsigned int getReadPtr() const;
  unsigned int getWritePtr();
  unsigned int getMaxReadSize();
  unsigned int getMaxWriteSize();
};

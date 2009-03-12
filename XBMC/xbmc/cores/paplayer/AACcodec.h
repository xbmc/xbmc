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

#include "CachingCodec.h"
#include "DllAACCodec.h"

class AACCodec : public CachingCodec
{
public:
  AACCodec();
  virtual ~AACCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
private:
  static unsigned __int32 AACOpenCallback(const char *pName, const char *mode, void *userData);
  static void AACCloseCallback(void *userData);
  static unsigned __int32 AACReadCallback(void *userData, void *pBuffer, unsigned long nBytesToRead);
  static __int32 AACSeekCallback(void *userData, unsigned __int64 pos);
  static __int64 AACFilesizeCallback(void *userData);

  AACHandle m_Handle;
  BYTE*     m_Buffer;
  int       m_BufferSize;
  int       m_BufferPos;

  // Our dll
  DllAACCodec m_dll;
};

#ifndef NSF_CODEC_H_
#define NSF_CODEC_H_

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "ICodec.h"
#include "DllNosefart.h"

class NSFCodec : public ICodec
{
public:
  NSFCodec();
  virtual ~NSFCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual int64_t Seek(int64_t iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();

private:
  int m_iTrack;
  void* m_nsf;
  bool m_bIsPlaying;

  DllNosefart m_dll;
  char* m_szBuffer;
  char* m_szStartOfBuffer; // never allocated
  int m_iDataInBuffer;
  int64_t m_iDataPos;
};

#endif


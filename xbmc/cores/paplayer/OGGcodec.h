#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef PAPLAYER_CACHINGCODEC_H_INCLUDED
#define PAPLAYER_CACHINGCODEC_H_INCLUDED
#include "CachingCodec.h"
#endif

#ifndef PAPLAYER_DLLVORBISFILE_H_INCLUDED
#define PAPLAYER_DLLVORBISFILE_H_INCLUDED
#include "DllVorbisfile.h"
#endif

#ifndef PAPLAYER_OGGCALLBACK_H_INCLUDED
#define PAPLAYER_OGGCALLBACK_H_INCLUDED
#include "OggCallback.h"
#endif


class OGGCodec : public CachingCodec
{
public:
  OGGCodec();
  virtual ~OGGCodec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual int64_t Seek(int64_t iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual bool CanInit();
  virtual CAEChannelInfo GetChannelInfo();

private:
  COggCallback m_callback;

  DllVorbisfile m_dll;
  OggVorbis_File m_VorbisFile;
  double m_TimeOffset;
  int m_CurrentStream;
  bool m_inited;
};

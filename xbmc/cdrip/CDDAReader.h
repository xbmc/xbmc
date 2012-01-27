#ifndef _CCDDAREADER_H
#define _CCDDAREADER_H

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

#define CDDARIP_OK    0
#define CDDARIP_ERR   1
#define CDDARIP_DONE  2

#include "threads/Thread.h"

#include "FileItem.h"
#include "filesystem/File.h"
#include "threads/Event.h"

struct RipBuffer
{
  int iRipError;
  long lBytesRead;
  BYTE* pbtStream;
  HANDLE hEvent;
};

class CCDDAReader : public CThread
{
public:
  CCDDAReader();
  virtual ~CCDDAReader();
  int GetData(BYTE** stream, long& lBytes);
  bool Init(const char* strFileName);
  bool DeInit();
  int GetPercent();
  int GetTrackLength();
protected:
  void Process();
  int ReadChunk();

  long m_lBufferSize;

  RipBuffer m_sRipBuffer[2]; // hold space for 2 buffers
  int m_iCurrentBuffer;   // 0 or 1

  CEvent m_hReadEvent;       // data is fetched
  CEvent m_hDataReadyEvent;  // data is ready to be fetched

  bool m_iInitialized;

  XFILE::CFile m_fileCdda;
};

#endif // _CCDDAREADER_H

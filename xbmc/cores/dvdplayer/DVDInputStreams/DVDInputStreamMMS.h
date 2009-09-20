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

#include "system.h"

#ifdef HAS_FILESYSTEM_MMS

#include "DVDInputStream.h"
#include "mms/mmsx.h"

class CDVDInputStreamMMS : public CDVDInputStream
{
public:
  CDVDInputStreamMMS();
  virtual ~CDVDInputStreamMMS();
  virtual bool    Open(const char* strFile, const std::string &content);
  virtual bool    Pause(double) { return true; }
  virtual void    Close();
  virtual int     Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  bool            SeekTime(int iTimeInMsec);
  virtual bool    IsEOF();
  virtual __int64 GetLength();
  virtual bool    NextStream();
protected:
  mmsx_t* m_mms;
};

#endif

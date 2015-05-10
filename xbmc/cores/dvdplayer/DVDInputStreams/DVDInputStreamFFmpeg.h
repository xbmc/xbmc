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

#include "DVDInputStream.h"

class CDVDInputStreamFFmpeg
  : public CDVDInputStream
  , public CDVDInputStream::ISeekable
{
public:
  CDVDInputStreamFFmpeg();
  virtual ~CDVDInputStreamFFmpeg();
  virtual bool Open(const char* strFile, const std::string &content, bool contentLookup);
  virtual void Close();
  virtual int Read(uint8_t* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual bool Pause(double dTime) { return false; };
  virtual bool IsEOF();
  virtual int64_t GetLength();

  virtual void    Abort()    { m_aborted = true;  }
  bool            Aborted()  { return m_aborted;  }

  bool            CanSeek()  { return m_can_seek; }
  bool            CanPause() { return m_can_pause; }

protected:
  bool m_can_pause;
  bool m_can_seek;
  bool m_aborted;
};

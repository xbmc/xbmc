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

#ifdef HAS_LIBRTMP

#include "DVDInputStream.h"
#include "DllLibRTMP.h"

class CDVDInputStreamRTMP 
  : public CDVDInputStream
  , public CDVDInputStream::ISeekTime
  , public CDVDInputStream::ISeekable
{
public:
  CDVDInputStreamRTMP();
  virtual ~CDVDInputStreamRTMP();
  virtual bool    Open(const char* strFile, const std::string &content);
  virtual void    Close();
  virtual int     Read(uint8_t* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  bool            SeekTime(int iTimeInMsec);
  bool            CanSeek()  { return m_canSeek; }
  bool            CanPause() { return m_canPause; }
  virtual bool    Pause(double dTime);
  virtual bool    IsEOF();
  virtual int64_t GetLength();

  CCriticalSection m_RTMPSection;

protected:
  bool       m_eof;
  bool       m_bPaused;
  bool       m_canSeek;
  bool       m_canPause;
  char*      m_sStreamPlaying;
  std::vector<std::string> m_optionvalues;

  RTMP       *m_rtmp;
  DllLibRTMP m_libRTMP;
};

#endif

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

#pragma once
#include "DVDInputStream.h"
#include "FileSystem/HTSPSession.h"

class CDVDInputStreamHTSP
  : public CDVDInputStream
  , public CDVDInputStream::IChannel
{
public:
  CDVDInputStreamHTSP();
  virtual ~CDVDInputStreamHTSP();
  virtual bool    Open(const char* file, const std::string &content);
  virtual void    Close();
  virtual int     Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence) { return -1; }
  virtual bool    IsEOF();
  virtual __int64 GetLength()                      { return -1; }

  virtual bool    NextStream()                     { return m_startup; }


  bool            NextChannel();
  bool            PrevChannel();
  bool            SelectChannel(unsigned int channel) { return false; }
  int             GetTotalTime() { return 0; }
  int             GetStartTime() { return 0; }
  bool            UpdateItem(CFileItem& item);

  bool            CanRecord()         { return false; }
  bool            IsRecording()       { return false; }
  bool            Record(bool bOnOff) { return false; }

  htsmsg_t* ReadStream();

private:

  bool      SetChannel(int channel);
  unsigned           m_subs;
  bool               m_startup;
  HTSP::CHTSPSession m_session;
  int                m_channel;
  int                m_tag;
  HTSP::SChannels    m_channels;
  HTSP::SEvent       m_event;
};

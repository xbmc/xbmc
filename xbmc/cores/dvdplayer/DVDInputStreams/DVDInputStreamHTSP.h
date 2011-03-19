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
#include "filesystem/HTSPSession.h"

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
  virtual bool Pause(double dTime) { return false; };
  virtual bool    IsEOF();
  virtual __int64 GetLength()                      { return -1; }

  virtual bool    NextStream()                     { return m_startup; }

  virtual void    Abort();

  bool            NextChannel(bool preview = false);
  bool            PrevChannel(bool preview = false);
  bool            SelectChannelByNumber(unsigned int channel);
  bool            SelectChannel(const CPVRChannel &channel) { return false; }
  bool            GetSelectedChannel(CPVRChannel *channel) {return false; }
  bool            UpdateItem(CFileItem& item);

  bool            CanRecord()         { return false; }
  bool            IsRecording()       { return false; }
  bool            Record(bool bOnOff) { return false; }

  int             GetTotalTime();
  int             GetStartTime();

  htsmsg_t* ReadStream();

private:
  typedef std::vector<HTSP::SChannel> SChannelV;
  typedef HTSP::const_circular_iter<SChannelV::iterator> SChannelC;
  bool      GetChannels(SChannelV &channels, SChannelV::iterator &it);
  bool      SetChannel(int channel);
  unsigned           m_subs;
  bool               m_startup;
  HTSP::CHTSPSession m_session;
  int                m_channel;
  int                m_tag;
  HTSP::SChannels    m_channels;
  HTSP::SEvent       m_event;

  struct SRead
  {
    SRead() { buf = NULL; Clear(); }
   ~SRead() { Clear(); }

    int  Size() { return end - cur; }
    void Clear()
    {
      free(buf);
      buf = NULL;
      cur = NULL;
      end = NULL;
    }

    uint8_t* buf;
    uint8_t* cur;
    uint8_t* end;
  } m_read;
};

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


typedef struct htsmsg htsmsg_t;

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

  htsmsg_t* ReadStream();

private:
  bool      SetChannel(int channel);
  htsmsg_t* ReadMessage();
  bool      SendMessage(htsmsg_t* m);
  htsmsg_t* ReadResult (htsmsg_t* m, bool sequence = true);

  SOCKET   m_fd;
  unsigned m_seq;
  unsigned m_subs;
  int      m_channel;
  bool     m_startup;
};

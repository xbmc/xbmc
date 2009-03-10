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
#include "deque"


typedef struct htsmsg htsmsg_t;


class CHTSPSession
{
public:
  struct SChannel
  {
    int         id;
    std::string name;
    std::string icon;
    int         event;

    SChannel() { Clear(); }
    void Clear()
    {
      id    = 0;
      event = 0;
      name.empty();
      icon.empty();
    }
  };

  struct SEvent
  {
    int         id;
    int         next;

    int         start;
    int         stop;
    std::string title;
    std::string descs;

    SEvent() { Clear(); }
    void Clear()
    {
      id    = 0;
      next  = 0;
      start = 0;
      stop  = 0;
      title.empty();
      descs.empty();
    }
  };

  typedef std::map<int, SChannel> SChannels;

  CHTSPSession();
  ~CHTSPSession();

  bool      Connect(const std::string& hostname, int port);
  void      Close();
  bool      Auth(const std::string& username, const std::string& password);

  htsmsg_t* ReadMessage();
  bool      SendMessage(htsmsg_t* m);

  htsmsg_t* ReadResult (htsmsg_t* m, bool sequence = true);
  bool      ReadSuccess(htsmsg_t* m, bool sequence = true, std::string action = "");

  bool      SendSubscribe  (int subscription, int channel);
  bool      SendUnsubscribe(int subscription);
  bool      SendEnableAsync();
  bool      GetEvent(SEvent& event, int id);

  static void OnChannelUpdate(htsmsg_t* msg, SChannels &channels);
  static void OnChannelRemove(htsmsg_t* msg, SChannels &channels);

private:
  SOCKET      m_fd;
  unsigned    m_seq;
  void*       m_challenge;
  int         m_challenge_len;

  std::deque<htsmsg_t*> m_queue;
  const unsigned int    m_queue_size;
};


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
  bool            UpdateItem(CFileItem& item);

  htsmsg_t* ReadStream();

private:

  typedef CHTSPSession::SChannel  SChannel;
  typedef CHTSPSession::SChannels SChannels;
  typedef CHTSPSession::SEvent    SEvent;

  bool      SetChannel(int channel);
  unsigned     m_subs;
  bool         m_startup;
  CHTSPSession m_session;
  int          m_channel;
  SChannels    m_channels;
  SEvent       m_event;
};

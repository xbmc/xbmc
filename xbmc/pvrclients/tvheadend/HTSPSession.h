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
#include <deque>
#include "pvrclient-tvheadend_os.h"
#include "StdString.h"

#include <algorithm>
#include <vector>
#include <map>

typedef struct htsmsg htsmsg_t;

template<typename T>
class const_circular_iter
  : public std::iterator< typename std::iterator_traits<T>::iterator_category
                        , typename std::iterator_traits<T>::value_type
                        , typename std::iterator_traits<T>::difference_type
                        , typename std::iterator_traits<T>::pointer
                        , typename std::iterator_traits<T>::reference>
{
  protected:
    T begin;
    T end;
    T iter;
  public:
    typedef typename std::iterator_traits<T>::value_type      value_type;
    typedef typename std::iterator_traits<T>::difference_type difference_type;
    typedef typename std::iterator_traits<T>::pointer         pointer;
    typedef typename std::iterator_traits<T>::reference       reference;

    const_circular_iter(const const_circular_iter& src)     : begin(src.begin), end(src.end), iter(src.iter) {};
    const_circular_iter(const T& b, const T& e)             : begin(b), end(e), iter(b) {};
    const_circular_iter(const T& b, const T& e, const T& c) : begin(b), end(e), iter(c) {};
    const_circular_iter<T>& operator++()
    {
      if(begin == end)
        return(*this);
      ++iter;
      if (iter == end)
        iter = begin;
      return(*this);
    }

    const_circular_iter<T>& operator--()
    {
      if(begin == end)
        return(*this);
      if (iter == begin)
        iter = end;
      iter--;
      return(*this);
    }

          reference operator*()  const { return (*iter);  }
    const pointer   operator->() const { return &(*iter); }
    bool operator==(const const_circular_iter<T>&  rhs) const { return (iter == rhs.iter); }
    bool operator==(const T&                       rhs) const { return (iter == rhs); }
    bool operator!=(const const_circular_iter<T>&  rhs) const { return ! operator==(rhs); }
    bool operator!=(const T&                       rhs) const { return ! operator==(rhs); }
};

struct STag
{
  int              id;
  std::string      name;
  std::string      icon;
  std::vector<int> channels;

  STag() { Clear(); }
  void Clear()
  {
    id    = 0;
    name.clear();
    icon.clear();
    channels.clear();
  }
  bool BelongsTo(int channel) const
  {
    return std::find(channels.begin(), channels.end(), channel) != channels.end();
  }

};

struct SChannel
{
  int              id;
  std::string      name;
  std::string      icon;
  int              event;
  int              num;
  std::vector<int> tags;

  SChannel() { Clear(); }
  void Clear()
  {
    id    = 0;
    event = 0;
    num   = 0;
    name.clear();
    icon.clear();
    tags.clear();
  }
  bool MemberOf(int tag) const
  {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
  }
  bool operator<(const SChannel &right) const
  {
    return num < right.num;
  }
};

struct SEvent
{
  int         id;
  int         next;
  int         chan_id;

  int         content;
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
    title.clear();
    descs.clear();
  }
};

struct SQueueStatus
{
  uint32_t packets; // Number of data packets in queue.
  uint32_t bytes;   // Number of bytes in queue.
  uint32_t delay;   // Estimated delay of queue (in µs)
  uint32_t bdrops;  // Number of B-frames dropped
  uint32_t pdrops;  // Number of P-frames dropped
  uint32_t idrops;  // Number of I-frames dropped

  SQueueStatus() { Clear(); }
  void Clear()
  {
    packets = 0;
    bytes   = 0;
    delay   = 0;
    bdrops  = 0;
    pdrops  = 0;
    idrops  = 0;
  }
};

struct SQuality
{
  CStdString  fe_name;
  CStdString  fe_status;
  uint32_t    fe_snr;
  uint32_t    fe_signal;
  uint32_t    fe_ber;
  uint32_t    fe_unc;
};

typedef std::map<int, SChannel> SChannels;
typedef std::map<int, STag>     STags;
typedef std::map<int, SEvent>   SEvents;


class cHTSPSession
{
public:
  cHTSPSession();
  ~cHTSPSession();

  bool      Connect(const std::string& hostname, int port);
  void      Close();
  void      Abort();
  bool      Auth(const std::string& username, const std::string& password);

  htsmsg_t* ReadMessage(int timeout = 10000);
  bool      SendMessage(htsmsg_t* m);

  htsmsg_t* ReadResult (htsmsg_t* m, bool sequence = true);
  bool      ReadSuccess(htsmsg_t* m, bool sequence = true, std::string action = "");

  bool      SendSubscribe  (int subscription, int channel);
  bool      SendUnsubscribe(int subscription);
  bool      SendEnableAsync();
  bool      GetEvent(SEvent& event, uint32_t id);

  int         GetProtocol()   { return m_protocol; }
  CStdString  GetServerName() { return m_server; }
  CStdString  GetVersion()    { return m_version; }
  unsigned    AddSequence()   { return ++m_seq; }

  static bool ParseEvent        (htsmsg_t* msg, uint32_t id, SEvent &event);
  static void ParseChannelUpdate(htsmsg_t* msg, SChannels &channels);
  static void ParseChannelRemove(htsmsg_t* msg, SChannels &channels);
  static void ParseTagUpdate    (htsmsg_t* msg, STags &tags);
  static void ParseTagRemove    (htsmsg_t* msg, STags &tags);
  static bool ParseQueueStatus  (htsmsg_t* msg, SQueueStatus &queue, SQuality &quality);

private:
  SOCKET      m_fd;
  unsigned    m_seq;
  void*       m_challenge;
  int         m_challenge_len;
  int         m_protocol;
  CStdString  m_server;
  CStdString  m_version;

  std::deque<htsmsg_t*> m_queue;
  const unsigned int    m_queue_size;

};

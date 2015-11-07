/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *		Copyright (C) 2010-2013 Eduard Kytmanov
 *		http://www.avmedia.su
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "threads/Event.h"

class CDSMsg
{
public:
  enum Message
  {
    NONE = 0,

    // General messages
    GENERAL_SET_WINDOW_POS,

    // MADVR
    MADVR_SET_WINDOW_POS,

    // Player messages
    PLAYER_SEEK_TIME,
    PLAYER_SEEK_PERCENT,
    PLAYER_SEEK,
    PLAYER_PLAY,
    PLAYER_PAUSE,
    PLAYER_STOP,
    PLAYER_UPDATE_TIME,

    // TODO: DVD needs some refactoring
    PLAYER_DVD_MOUSE_MOVE,
    PLAYER_DVD_MOUSE_CLICK,
    PLAYER_DVD_NAV_UP,
    PLAYER_DVD_NAV_DOWN,
    PLAYER_DVD_NAV_LEFT,
    PLAYER_DVD_NAV_RIGHT,
    PLAYER_DVD_MENU_ROOT,
    PLAYER_DVD_MENU_EXIT,
    PLAYER_DVD_MENU_BACK,
    PLAYER_DVD_MENU_SELECT,
    PLAYER_DVD_MENU_TITLE,
    PLAYER_DVD_MENU_SUBTITLE,
    PLAYER_DVD_MENU_AUDIO,
    PLAYER_DVD_MENU_ANGLE,

    RESET_DEVICE
  };

  CDSMsg(Message msg)
    : m_message(msg)
  {
    m_event.Reset();
    m_references = 1;
  }

  virtual ~CDSMsg()
  {
    assert(m_references == 0);
  }

  inline Message GetMessageType()
  {
    return m_message;
  }

  inline bool IsType(Message msg)
  {
    return (m_message == msg);
  }

  void Set()
  {
    m_event.Set();
  }

  void Wait()
  {
    m_event.Wait();
  }

  /**
   * increase the reference counter by one.
   */
  CDSMsg* Acquire()
  {
    InterlockedIncrement(&m_references);
    return this;
  }

  /**
   * decrease the reference counter by one.
   */
  long Release()
  {
    long count = InterlockedDecrement(&m_references);
    if (count == 0) delete this;
    return count;
  }

private:
  Message m_message;
  CEvent m_event;
  bool m_wait;
  long m_references;
};

template <typename T>
class CDSMsgType : public CDSMsg
{
public:
  CDSMsgType(Message type, T value)
    : CDSMsg(type)
    , m_value(value)
  {}
  operator T() { return m_value; }
  T m_value;
};

typedef CDSMsgType<bool>   CDSMsgBool;
typedef CDSMsgType<int>    CDSMsgInt;
typedef CDSMsgType<double> CDSMsgDouble;

#define AM_SEEKING_AbsolutePositioning 1
class CDSMsgPlayerSeekTime : public CDSMsg
{
public:
  CDSMsgPlayerSeekTime(uint64_t time, uint32_t flags = AM_SEEKING_AbsolutePositioning, bool showPopup = true)
    : CDSMsg(PLAYER_SEEK_TIME), m_time(time), m_flags(flags), m_showPopup(showPopup)
  {
  }

  uint64_t GetTime() const
  {
    return m_time;
  }

  uint32_t GetFlags() const
  {
    return m_flags;
  }

  bool ShowPopup() const
  {
    return m_showPopup;
  }

private:
  uint64_t m_time;
  uint32_t m_flags;
  bool m_showPopup;
};

class CDSMsgPlayerSeek : public CDSMsg
{
public:
  CDSMsgPlayerSeek(bool forward, bool largeStep)
    : CDSMsg(PLAYER_SEEK), m_forward(forward), m_largeStep(largeStep)
  {
  }

  bool Forward() const
  {
    return m_forward;
  }

  bool LargeStep() const
  {
    return m_largeStep;
  }

private:
  bool m_forward;
  bool m_largeStep;
};
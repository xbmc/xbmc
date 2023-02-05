/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDMessage.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"

#include <algorithm>
#include <atomic>
#include <list>
#include <string>

struct DVDMessageListItem
{
  DVDMessageListItem(std::shared_ptr<CDVDMsg> msg, int prio) : message(std::move(msg))
  {
    priority = prio;
  }
  DVDMessageListItem() { priority = 0; }
  DVDMessageListItem(const DVDMessageListItem&) = delete;
  ~DVDMessageListItem() = default;

  DVDMessageListItem& operator=(const DVDMessageListItem&) = delete;

  std::shared_ptr<CDVDMsg> message;
  int priority;
};

enum MsgQueueReturnCode
{
  MSGQ_OK = 1,
  MSGQ_TIMEOUT = 0,
  MSGQ_ABORT = -1, // negative for legacy, not an error actually
  MSGQ_NOT_INITIALIZED = -2,
  MSGQ_INVALID_MSG = -3,
  MSGQ_OUT_OF_MEMORY = -4
};

#define MSGQ_IS_ERROR(c)    (c < 0)

class CDVDMessageQueue
{
public:
  explicit CDVDMessageQueue(const std::string &owner);
  virtual ~CDVDMessageQueue();

  void Init();
  void Flush(CDVDMsg::Message message = CDVDMsg::DEMUXER_PACKET);
  void Abort();
  void End();

  MsgQueueReturnCode Put(const std::shared_ptr<CDVDMsg>& pMsg, int priority = 0);
  MsgQueueReturnCode PutBack(const std::shared_ptr<CDVDMsg>& pMsg, int priority = 0);

  /**
   * msg,       message type from DVDMessage.h
   * timeout,   timeout in msec
   * priority,  minimum priority to get, outputs returned packets priority
   */
  MsgQueueReturnCode Get(std::shared_ptr<CDVDMsg>& pMsg,
                         std::chrono::milliseconds timeout,
                         int& priority);
  MsgQueueReturnCode Get(std::shared_ptr<CDVDMsg>& pMsg, std::chrono::milliseconds timeout)
  {
    int priority = 0;
    return Get(pMsg, timeout, priority);
  }

  int GetDataSize() const { return m_iDataSize; }
  int GetTimeSize() const;
  unsigned GetPacketCount(CDVDMsg::Message type);
  bool ReceivedAbortRequest() { return m_bAbortRequest; }
  void WaitUntilEmpty();

  // non messagequeue related functions
  bool IsFull() const { return GetLevel() == 100; }
  int GetLevel() const;

  void SetMaxDataSize(int iMaxDataSize) { m_iMaxDataSize = iMaxDataSize; }
  void SetMaxTimeSize(double sec) { m_TimeSize  = 1.0 / std::max(1.0, sec); }
  int GetMaxDataSize() const { return m_iMaxDataSize; }
  double GetMaxTimeSize() const { return m_TimeSize; }
  bool IsInited() const { return m_bInitialized; }
  bool IsDataBased() const;

private:
  MsgQueueReturnCode Put(const std::shared_ptr<CDVDMsg>& pMsg, int priority, bool front);
  void UpdateTimeFront();
  void UpdateTimeBack();

  CEvent m_hEvent;
  mutable CCriticalSection m_section;

  std::atomic<bool> m_bAbortRequest = false;
  bool m_bInitialized;
  bool m_drain = false;

  int m_iDataSize;
  double m_TimeFront;
  double m_TimeBack;
  double m_TimeSize;

  int m_iMaxDataSize;
  std::string m_owner;

  std::list<DVDMessageListItem> m_messages;
  std::list<DVDMessageListItem> m_prioMessages;
};


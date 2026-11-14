/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationMessenger.h"

#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "messaging/IMessageTarget.h"
#include "threads/Event.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace KODI
{
namespace MESSAGING
{

class CDelayedMessage : public CThread
{
  public:
    CDelayedMessage(const ThreadMessage& msg, unsigned int delay);
    void Process() override;

  private:
    unsigned int   m_delay;
    ThreadMessage  m_msg;
};

CDelayedMessage::CDelayedMessage(const ThreadMessage& msg, unsigned int delay)
  : CThread("DelayedMessage"), m_msg(msg)
{
  m_delay = delay;
}

void CDelayedMessage::Process()
{
  CThread::Sleep(std::chrono::milliseconds(m_delay));

  if (!m_bStop)
    CServiceBroker::GetAppMessenger()->PostMsg(m_msg.dwMessage, m_msg.param1, m_msg.param1,
                                               m_msg.lpVoid, m_msg.strParam, m_msg.params);
}

CApplicationMessenger::CApplicationMessenger() = default;

CApplicationMessenger::~CApplicationMessenger()
{
  Cleanup();
}

void CApplicationMessenger::Cleanup()
{
  std::unique_lock lock(m_critSection);

  while (!m_vecMessages.empty())
  {
    ThreadMessage* pMsg = m_vecMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecMessages.pop();
  }

  while (!m_vecWindowMessages.empty())
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();

    if (pMsg->waitEvent)
      pMsg->waitEvent->Set();

    delete pMsg;
    m_vecWindowMessages.pop();
  }
}

int CApplicationMessenger::SendMsg(ThreadMessage&& message, bool wait)
{
  std::shared_ptr<CEvent> waitEvent;
  std::shared_ptr<int> result;

  if (wait)
  {
    //Initialize result here as it's not needed for posted messages
    message.result = std::make_shared<int>(-1);
    // check that we're not being called from our application thread, else we'll be waiting
    // forever!
    if (m_guiThreadId != CThread::GetCurrentThreadId())
    {
      message.waitEvent = std::make_shared<CEvent>(true);
      waitEvent = message.waitEvent;
      result = message.result;
    }
    else
    {
      //OutputDebugString("Attempting to wait on a SendMessage() from our application thread will cause lockup!\n");
      //OutputDebugString("Sending immediately\n");
      ProcessMessage(&message);
      return *message.result;
    }
  }


  if (m_bStop)
    return -1;

  ThreadMessage* msg = new ThreadMessage(std::move(message));

  // once queued the app thread may process and delete msg, so touching it after
  // this is a race (yarc - "yet another race condition")
  Enqueue(msg);

  if (waitEvent) // ... it just so happens we have a spare reference to the
                 //  waitEvent ... just for such contingencies :)
  {
    // ensure the thread doesn't hold the graphics lock
    CWinSystemBase* winSystem = CServiceBroker::GetWinSystem();
    //! @todo This won't really help as winSystem can die every single
    // moment on shutdown. A shared ptr would be a more valid solution
    // depending on the design dependencies.
    if (winSystem)
    {
      CSingleExit exit(winSystem->GetGfxContext());
      waitEvent->Wait();
    }
    return *result;
  }

  return -1;
}

void CApplicationMessenger::Enqueue(ThreadMessage* msg)
{
  std::unique_lock lock(m_critSection);
  if (msg->dwMessage == TMSG_GUI_MESSAGE)
    m_vecWindowMessages.push(msg);
  else
    m_vecMessages.push(msg);
}

int CApplicationMessenger::SendMsg(uint32_t messageId)
{
   return SendMsg(ThreadMessage{ messageId }, true);
}

int CApplicationMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload)
{
  return SendMsg(ThreadMessage{ messageId, param1, param2, payload }, true);
}

bool CApplicationMessenger::SendMsgAbortable(
    uint32_t messageId, int param1, int param2, void* payload, CEvent& abortEvent)
{
  if (m_guiThreadId == CThread::GetCurrentThreadId())
  {
    SendMsg(messageId, param1, param2, payload);
    return true;
  }

  if (m_bStop || abortEvent.Signaled())
    return false;

  auto waitEvent = std::make_shared<CEvent>(true);
  auto claimed = std::make_shared<std::atomic<bool>>(false);
  ThreadMessage* msg = new ThreadMessage{messageId, param1, param2, payload};
  msg->waitEvent = waitEvent;
  msg->claimed = claimed;
  Enqueue(msg); // msg may be deleted from here on

  // ensure the thread doesn't hold the graphics lock
  std::optional<CSingleExit> exit;
  if (CWinSystemBase* winSystem = CServiceBroker::GetWinSystem(); winSystem)
    exit.emplace(winSystem->GetGfxContext());

  {
    XbmcThreads::CEventGroup group{waitEvent.get(), &abortEvent};
    // drop it, unless the app thread has already taken it
    if (group.wait() == &abortEvent && !claimed->exchange(true))
      return false;
  }
  waitEvent->Wait();
  // not claimed if Cleanup() released it at shutdown
  return claimed->load();
}

int CApplicationMessenger::SendMsgAbortable(uint32_t messageId,
                                            int param1,
                                            int param2,
                                            std::shared_ptr<void> payload,
                                            CEvent& abortEvent,
                                            std::string strParam)
{
  if (m_guiThreadId == CThread::GetCurrentThreadId())
    return SendMsg(messageId, param1, param2, payload.get(), std::move(strParam));

  if (m_bStop || abortEvent.Signaled())
    return -1;

  auto waitEvent = std::make_shared<CEvent>(true);
  auto result = std::make_shared<int>(-1);
  auto claimed = std::make_shared<std::atomic<bool>>(false);
  ThreadMessage* msg =
      new ThreadMessage{messageId, param1, param2, payload.get(), std::move(strParam), {}};
  msg->payloadOwner = std::move(payload);
  msg->waitEvent = waitEvent;
  msg->result = result;
  msg->claimed = claimed;
  Enqueue(msg); // msg may be deleted from here on

  // ensure the thread doesn't hold the graphics lock
  std::optional<CSingleExit> exit;
  if (CWinSystemBase* winSystem = CServiceBroker::GetWinSystem(); winSystem)
    exit.emplace(winSystem->GetGfxContext());

  XbmcThreads::CEventGroup group{waitEvent.get(), &abortEvent};
  if (group.wait() == &abortEvent)
  {
    // drop it, unless the app thread has already taken it
    claimed->exchange(true);
    return -1;
  }
  return *result;
}

int CApplicationMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam)
{
  return SendMsg(ThreadMessage{messageId, param1, param2, payload, std::move(strParam),
                               std::vector<std::string>{}},
                 true);
}

int CApplicationMessenger::SendMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params)
{
  return SendMsg(
      ThreadMessage{messageId, param1, param2, payload, std::move(strParam), std::move(params)},
      true);
}

void CApplicationMessenger::PostMsg(uint32_t messageId)
{
  SendMsg(ThreadMessage{ messageId }, false);
}

void CApplicationMessenger::PostMsg(uint32_t messageId, int64_t param3)
{
  SendMsg(ThreadMessage{ messageId, param3 }, false);
}

void CApplicationMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload)
{
  SendMsg(ThreadMessage{ messageId, param1, param2, payload }, false);
}

void CApplicationMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam)
{
  SendMsg(ThreadMessage{messageId, param1, param2, payload, std::move(strParam),
                        std::vector<std::string>{}},
          false);
}

void CApplicationMessenger::PostMsg(uint32_t messageId, int param1, int param2, void* payload, std::string strParam, std::vector<std::string> params)
{
  SendMsg(ThreadMessage{messageId, param1, param2, payload, std::move(strParam), std::move(params)},
          false);
}

void CApplicationMessenger::ProcessMessages()
{
  // process threadmessages
  std::unique_lock lock(m_critSection);
  while (!m_vecMessages.empty())
  {
    ThreadMessage* pMsg = m_vecMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecMessages.pop();

    //Leave here as the message might make another
    //thread call processmessages or sendmessage

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.unlock(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);

    if (waitEvent)
      waitEvent->Set();
    delete pMsg;

    lock.lock();
  }
}

void CApplicationMessenger::ProcessMessage(ThreadMessage *pMsg)
{
  // the sender gave up waiting and dropped it
  if (pMsg->claimed && pMsg->claimed->exchange(true))
    return;

  //special case for this that we handle ourselves
  if (pMsg->dwMessage == TMSG_CALLBACK)
  {
    ThreadMessageCallback *callback = static_cast<ThreadMessageCallback*>(pMsg->lpVoid);
    callback->callback(callback->userptr);
    return;
  }

  std::unique_lock lock(m_critSection);
  int mask = pMsg->dwMessage & TMSG_MASK_MESSAGE;

  const auto it = m_mapTargets.find(mask);
  if (it != m_mapTargets.end())
  {
    CSingleExit exit(m_critSection);
    it->second->OnApplicationMessage(pMsg);
  }
  else
    CLog::LogF(LOGERROR, "receiver {} is not defined", mask);
}

void CApplicationMessenger::ProcessWindowMessages()
{
  std::unique_lock lock(m_critSection);
  //message type is window, process window messages
  while (!m_vecWindowMessages.empty())
  {
    ThreadMessage* pMsg = m_vecWindowMessages.front();
    //first remove the message from the queue, else the message could be processed more then once
    m_vecWindowMessages.pop();

    // leave here in case we make more thread messages from this one

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.unlock(); // <- see the large comment in SendMessage ^

    ProcessMessage(pMsg);
    if (waitEvent)
      waitEvent->Set();
    delete pMsg;

    lock.lock();
  }
}

void CApplicationMessenger::SendGUIMessage(const CGUIMessage &message, int windowID, bool waitResult)
{
  ThreadMessage tMsg(TMSG_GUI_MESSAGE);
  tMsg.param1 = windowID == WINDOW_INVALID ? 0 : windowID;
  tMsg.lpVoid = new CGUIMessage(message);
  SendMsg(std::move(tMsg), waitResult);
}

void CApplicationMessenger::RegisterReceiver(IMessageTarget* target)
{
  std::unique_lock lock(m_critSection);
  m_mapTargets.insert(std::make_pair(target->GetMessageMask(), target));
}

bool CApplicationMessenger::IsProcessThread() const
{
  return m_processThreadId == CThread::GetCurrentThreadId();
}
}
}

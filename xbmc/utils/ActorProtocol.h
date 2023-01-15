/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <cstddef>
#include <memory>
#include <queue>
#include <string>
#include <utility>

class CEvent;

namespace Actor
{

class CPayloadWrapBase
{
public:
  virtual ~CPayloadWrapBase() = default;
};

template<typename Payload>
class CPayloadWrap : public CPayloadWrapBase
{
public:
  ~CPayloadWrap() override = default;
  CPayloadWrap(Payload* data) { m_pPayload.reset(data); }
  CPayloadWrap(Payload& data) { m_pPayload.reset(new Payload(data)); }
  Payload* GetPlayload() { return m_pPayload.get(); }

protected:
  std::unique_ptr<Payload> m_pPayload;
};

class Protocol;

class Message
{
  friend class Protocol;

  static constexpr size_t MSG_INTERNAL_BUFFER_SIZE = 32;

public:
  int signal;
  bool isSync = false;
  bool isSyncFini;
  bool isOut;
  bool isSyncTimeout;
  size_t payloadSize;
  uint8_t buffer[MSG_INTERNAL_BUFFER_SIZE];
  uint8_t *data = nullptr;
  std::unique_ptr<CPayloadWrapBase> payloadObj;
  Message *replyMessage = nullptr;
  Protocol &origin;
  CEvent *event = nullptr;

  void Release();
  bool Reply(int sig, void *data = nullptr, size_t size = 0);

private:
  explicit Message(Protocol &_origin) noexcept
    :origin(_origin) {}
};

class Protocol
{
public:
  Protocol(std::string name, CEvent* inEvent, CEvent* outEvent)
    : portName(std::move(name)), containerInEvent(inEvent), containerOutEvent(outEvent)
  {
  }
  Protocol(std::string name) : Protocol(std::move(name), nullptr, nullptr) {}
  ~Protocol();
  Message *GetMessage();
  void ReturnMessage(Message *msg);
  bool SendOutMessage(int signal,
                      const void* data = nullptr,
                      size_t size = 0,
                      Message* outMsg = nullptr);
  bool SendOutMessage(int signal, CPayloadWrapBase *payload, Message *outMsg = nullptr);
  bool SendInMessage(int signal,
                     const void* data = nullptr,
                     size_t size = 0,
                     Message* outMsg = nullptr);
  bool SendInMessage(int signal, CPayloadWrapBase *payload, Message *outMsg = nullptr);
  bool SendOutMessageSync(int signal,
                          Message** retMsg,
                          std::chrono::milliseconds timeout,
                          const void* data = nullptr,
                          size_t size = 0);
  bool SendOutMessageSync(int signal,
                          Message** retMsg,
                          std::chrono::milliseconds timeout,
                          CPayloadWrapBase* payload);
  bool ReceiveOutMessage(Message **msg);
  bool ReceiveInMessage(Message **msg);
  void Purge();
  void PurgeIn(int signal);
  void PurgeOut(int signal);
  void DeferIn(bool value) { inDefered = value; }
  void DeferOut(bool value) { outDefered = value; }
  void Lock() { criticalSection.lock(); }
  void Unlock() { criticalSection.unlock(); }
  std::string portName;

protected:
  CEvent *containerInEvent, *containerOutEvent;
  CCriticalSection criticalSection;
  std::queue<Message*> outMessages;
  std::queue<Message*> inMessages;
  std::queue<Message*> freeMessageQueue;
  bool inDefered = false, outDefered = false;
};

}

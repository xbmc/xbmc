/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include "threads/CriticalSection.h"

#include <cstddef>
#include <queue>
#include <memory>
#include <string>

class CEvent;

namespace Actor
{

class CPayloadWrapBase
{
public:
  virtual ~CPayloadWrapBase() {};
};

template<typename Payload>
class CPayloadWrap : public CPayloadWrapBase
{
public:
  ~CPayloadWrap() override {};
  CPayloadWrap(Payload *data) {m_pPayload.reset(data);};
  CPayloadWrap(Payload &data) {m_pPayload.reset(new Payload(data));};
  Payload *GetPlayload() {return m_pPayload.get();};
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
  Protocol(std::string name, CEvent* inEvent, CEvent *outEvent)
    :portName(name), containerInEvent(inEvent), containerOutEvent(outEvent) {}
  Protocol(std::string name)
    : Protocol(name, nullptr, nullptr) {}
  ~Protocol();
  Message *GetMessage();
  void ReturnMessage(Message *msg);
  bool SendOutMessage(int signal, void *data = nullptr, size_t size = 0, Message *outMsg = nullptr);
  bool SendOutMessage(int signal, CPayloadWrapBase *payload, Message *outMsg = nullptr);
  bool SendInMessage(int signal, void *data = nullptr, size_t size = 0, Message *outMsg = nullptr);
  bool SendInMessage(int signal, CPayloadWrapBase *payload, Message *outMsg = nullptr);
  bool SendOutMessageSync(int signal, Message **retMsg, int timeout, void *data = nullptr, size_t size = 0);
  bool SendOutMessageSync(int signal, Message **retMsg, int timeout, CPayloadWrapBase *payload);
  bool ReceiveOutMessage(Message **msg);
  bool ReceiveInMessage(Message **msg);
  void Purge();
  void PurgeIn(int signal);
  void PurgeOut(int signal);
  void DeferIn(bool value) {inDefered = value;};
  void DeferOut(bool value) {outDefered = value;};
  void Lock() {criticalSection.lock();};
  void Unlock() {criticalSection.unlock();};
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

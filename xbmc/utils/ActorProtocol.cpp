/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ActorProtocol.h"

#include "threads/Event.h"

#include <cstring>
#include <mutex>

using namespace Actor;

void Message::Release()
{
  bool skip;
  origin.Lock();
  skip = isSync ? !isSyncFini : false;
  isSyncFini = true;
  origin.Unlock();

  if (skip)
    return;

  // free data buffer
  if (data != buffer)
    delete [] data;

  payloadObj.reset();

  // delete event in case of sync message
  delete event;

  origin.ReturnMessage(this);
}

bool Message::Reply(int sig, void *data /* = NULL*/, size_t size /* = 0 */)
{
  if (!isSync)
  {
    if (isOut)
      return origin.SendInMessage(sig, data, size);
    else
      return origin.SendOutMessage(sig, data, size);
  }

  origin.Lock();

  if (!isSyncTimeout)
  {
    Message *msg = origin.GetMessage();
    msg->signal = sig;
    msg->isOut = !isOut;
    replyMessage = msg;
    if (data)
    {
      if (size > sizeof(msg->buffer))
        msg->data = new uint8_t[size];
      else
        msg->data = msg->buffer;
      memcpy(msg->data, data, size);
    }
  }

  origin.Unlock();

  if (event)
    event->Set();

  return true;
}

Protocol::~Protocol()
{
  Message *msg;
  Purge();
  while (!freeMessageQueue.empty())
  {
    msg = freeMessageQueue.front();
    freeMessageQueue.pop();
    delete msg;
  }
}

Message *Protocol::GetMessage()
{
  Message *msg;

  std::unique_lock<CCriticalSection> lock(criticalSection);

  if (!freeMessageQueue.empty())
  {
    msg = freeMessageQueue.front();
    freeMessageQueue.pop();
  }
  else
    msg = new Message(*this);

  msg->isSync = false;
  msg->isSyncFini = false;
  msg->isSyncTimeout = false;
  msg->event = NULL;
  msg->data = NULL;
  msg->payloadSize = 0;
  msg->replyMessage = NULL;

  return msg;
}

void Protocol::ReturnMessage(Message *msg)
{
  std::unique_lock<CCriticalSection> lock(criticalSection);

  freeMessageQueue.push(msg);
}

bool Protocol::SendOutMessage(int signal,
                              const void* data /* = NULL */,
                              size_t size /* = 0 */,
                              Message* outMsg /* = NULL */)
{
  Message *msg;
  if (outMsg)
    msg = outMsg;
  else
    msg = GetMessage();

  msg->signal = signal;
  msg->isOut = true;

  if (data)
  {
    if (size > sizeof(msg->buffer))
      msg->data = new uint8_t[size];
    else
      msg->data = msg->buffer;
    memcpy(msg->data, data, size);
  }

  {
    std::unique_lock<CCriticalSection> lock(criticalSection);
    outMessages.push(msg);
  }
  if (containerOutEvent)
    containerOutEvent->Set();

  return true;
}

bool Protocol::SendOutMessage(int signal, CPayloadWrapBase *payload, Message *outMsg)
{
  Message *msg;
  if (outMsg)
    msg = outMsg;
  else
    msg = GetMessage();

  msg->signal = signal;
  msg->isOut = true;

  msg->payloadObj.reset(payload);

  {
    std::unique_lock<CCriticalSection> lock(criticalSection);
    outMessages.push(msg);
  }
  if (containerOutEvent)
    containerOutEvent->Set();

  return true;
}

bool Protocol::SendInMessage(int signal,
                             const void* data /* = NULL */,
                             size_t size /* = 0 */,
                             Message* outMsg /* = NULL */)
{
  Message *msg;
  if (outMsg)
    msg = outMsg;
  else
    msg = GetMessage();

  msg->signal = signal;
  msg->isOut = false;

  if (data)
  {
    if (size > sizeof(msg->data))
      msg->data = new uint8_t[size];
    else
      msg->data = msg->buffer;
    memcpy(msg->data, data, size);
  }

  {
    std::unique_lock<CCriticalSection> lock(criticalSection);
    inMessages.push(msg);
  }
  if (containerInEvent)
    containerInEvent->Set();

  return true;
}

bool Protocol::SendInMessage(int signal, CPayloadWrapBase *payload, Message *outMsg)
{
  Message *msg;
  if (outMsg)
    msg = outMsg;
  else
    msg = GetMessage();

  msg->signal = signal;
  msg->isOut = false;

  msg->payloadObj.reset(payload);

  {
    std::unique_lock<CCriticalSection> lock(criticalSection);
    inMessages.push(msg);
  }
  if (containerInEvent)
    containerInEvent->Set();

  return true;
}

bool Protocol::SendOutMessageSync(int signal,
                                  Message** retMsg,
                                  std::chrono::milliseconds timeout,
                                  const void* data /* = NULL */,
                                  size_t size /* = 0 */)
{
  Message *msg = GetMessage();
  msg->isOut = true;
  msg->isSync = true;
  msg->event = new CEvent;
  msg->event->Reset();
  SendOutMessage(signal, data, size, msg);

  if (!msg->event->Wait(timeout))
  {
    const std::unique_lock<CCriticalSection> lock(criticalSection);
    if (msg->replyMessage)
      *retMsg = msg->replyMessage;
    else
    {
      *retMsg = NULL;
      msg->isSyncTimeout = true;
    }
  }
  else
    *retMsg = msg->replyMessage;

  msg->Release();

  if (*retMsg)
    return true;
  else
    return false;
}

bool Protocol::SendOutMessageSync(int signal,
                                  Message** retMsg,
                                  std::chrono::milliseconds timeout,
                                  CPayloadWrapBase* payload)
{
  Message *msg = GetMessage();
  msg->isOut = true;
  msg->isSync = true;
  msg->event = new CEvent;
  msg->event->Reset();
  SendOutMessage(signal, payload, msg);

  if (!msg->event->Wait(timeout))
  {
    const std::unique_lock<CCriticalSection> lock(criticalSection);
    if (msg->replyMessage)
      *retMsg = msg->replyMessage;
    else
    {
      *retMsg = NULL;
      msg->isSyncTimeout = true;
    }
  }
  else
    *retMsg = msg->replyMessage;

  msg->Release();

  if (*retMsg)
    return true;
  else
    return false;
}

bool Protocol::ReceiveOutMessage(Message **msg)
{
  std::unique_lock<CCriticalSection> lock(criticalSection);

  if (outMessages.empty() || outDefered)
    return false;

  *msg = outMessages.front();
  outMessages.pop();

  return true;
}

bool Protocol::ReceiveInMessage(Message **msg)
{
  std::unique_lock<CCriticalSection> lock(criticalSection);

  if (inMessages.empty() || inDefered)
    return false;

  *msg = inMessages.front();
  inMessages.pop();

  return true;
}


void Protocol::Purge()
{
  Message *msg;

  while (ReceiveInMessage(&msg))
    msg->Release();

  while (ReceiveOutMessage(&msg))
    msg->Release();
}

void Protocol::PurgeIn(int signal)
{
  Message *msg;
  std::queue<Message*> msgs;

  std::unique_lock<CCriticalSection> lock(criticalSection);

  while (!inMessages.empty())
  {
    msg = inMessages.front();
    inMessages.pop();
    if (msg->signal != signal)
      msgs.push(msg);
  }
  while (!msgs.empty())
  {
    msg = msgs.front();
    msgs.pop();
    inMessages.push(msg);
  }
}

void Protocol::PurgeOut(int signal)
{
  Message *msg;
  std::queue<Message*> msgs;

  std::unique_lock<CCriticalSection> lock(criticalSection);

  while (!outMessages.empty())
  {
    msg = outMessages.front();
    outMessages.pop();
    if (msg->signal != signal)
      msgs.push(msg);
  }
  while (!msgs.empty())
  {
    msg = msgs.front();
    msgs.pop();
    outMessages.push(msg);
  }
}

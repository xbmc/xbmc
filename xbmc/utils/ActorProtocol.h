/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "threads/Thread.h"
#include "utils/log.h"
#include <queue>
#include "memory.h"

#define MSG_INTERNAL_BUFFER_SIZE 32

namespace Actor
{

class Protocol;

class Message
{
  friend class Protocol;
public:
  int signal;
  bool isSync;
  bool isSyncFini;
  bool isOut;
  bool isSyncTimeout;
  int payloadSize;
  uint8_t buffer[MSG_INTERNAL_BUFFER_SIZE];
  uint8_t *data;
  Message *replyMessage;
  Protocol *origin;
  CEvent *event;

  void Release();
  bool Reply(int sig, void *data = NULL, int size = 0);

private:
  Message() {isSync = false; data = NULL; event = NULL; replyMessage = NULL;};
};

class Protocol
{
public:
  Protocol(std::string name, CEvent* inEvent, CEvent *outEvent)
    : portName(name), inDefered(false), outDefered(false) {containerInEvent = inEvent; containerOutEvent = outEvent;};
  virtual ~Protocol();
  Message *GetMessage();
  void ReturnMessage(Message *msg);
  bool SendOutMessage(int signal, void *data = NULL, int size = 0, Message *outMsg = NULL);
  bool SendInMessage(int signal, void *data = NULL, int size = 0, Message *outMsg = NULL);
  bool SendOutMessageSync(int signal, Message **retMsg, int timeout, void *data = NULL, int size = 0);
  bool ReceiveOutMessage(Message **msg);
  bool ReceiveInMessage(Message **msg);
  void Purge();
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
  bool inDefered, outDefered;
};

}

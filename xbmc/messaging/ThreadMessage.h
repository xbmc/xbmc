#pragma once
/*
*      Copyright (C) 2005-2015 Team XBMC
*      http://xbmc.org
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

#include <memory>
#include <string>
#include <vector>

class CEvent;

namespace KODI
{
namespace MESSAGING
{

class CApplicationMessenger;

class ThreadMessage
{
  friend CApplicationMessenger;
public:
  ThreadMessage()
    : ThreadMessage{ 0, -1, -1, nullptr }
  {
  }

  explicit ThreadMessage(uint32_t messageId)
    : ThreadMessage{ messageId, -1, -1, nullptr }
  {
  }

  ThreadMessage(uint32_t messageId, int p1, int p2, void* payload)
    : dwMessage{ messageId }
    , param1{ p1 }
    , param2{ p2 }
    , lpVoid{ payload }
  {
  }

  ThreadMessage(uint32_t messageId, int p1, int p2, void* payload, std::string param, std::vector<std::string> vecParams)
    : dwMessage{ messageId }
    , param1{ p1 }
    , param2{ p2 }
    , lpVoid{ payload }
    , strParam( param )
    , params( vecParams )
  {
  }

  ThreadMessage(const ThreadMessage& other)
    : dwMessage(other.dwMessage),
    param1(other.param1),
    param2(other.param2),
    lpVoid(other.lpVoid),
    strParam(other.strParam),
    params(other.params),
    waitEvent(other.waitEvent),
    result(other.result)
  {
  }

  ThreadMessage(ThreadMessage&& other)
    : dwMessage(other.dwMessage),
    param1(other.param1),
    param2(other.param2),
    lpVoid(other.lpVoid),
    strParam(std::move(other.strParam)),
    params(std::move(other.params)),
    waitEvent(std::move(other.waitEvent)),
    result(std::move(other.result))
  {
  }

  ThreadMessage& operator=(const ThreadMessage& other)
  {
    if (this == &other)
      return *this;
    dwMessage = other.dwMessage;
    param1 = other.param1;
    param2 = other.param2;
    lpVoid = other.lpVoid;
    strParam = other.strParam;
    params = other.params;
    waitEvent = other.waitEvent;
    result = other.result;
    return *this;
  }

  ThreadMessage& operator=(ThreadMessage&& other)
  {
    if (this == &other)
      return *this;
    dwMessage = other.dwMessage;
    param1 = other.param1;
    param2 = other.param2;
    lpVoid = other.lpVoid;
    strParam = std::move(other.strParam);
    params = std::move(other.params);
    waitEvent = std::move(other.waitEvent);
    result = std::move(other.result);
    return *this;
  }

  uint32_t dwMessage;
  int param1;
  int param2;
  void* lpVoid;
  std::string strParam;
  std::vector<std::string> params;

  void SetResult(int res)
  {
    //On posted messages result will be zero, since they can't
    //retreive the response we silently ignore this to let message
    //handlers not have to worry about it
    if (result)
      *result = res;
  }
protected:
  std::shared_ptr<CEvent> waitEvent;
  std::shared_ptr<int> result;
};
}
}

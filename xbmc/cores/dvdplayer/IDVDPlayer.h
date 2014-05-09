#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "DVDStreamInfo.h"
#include "DVDMessageQueue.h"

class DVDNavResult;

class IDVDPlayer
{
public:
  virtual int OnDVDNavResult(void* pData, int iMessage) = 0;
  virtual ~IDVDPlayer() { }
};

class IDVDStreamPlayer
{
public:
  virtual bool OpenStream(CDVDStreamInfo &hint) = 0;
  virtual void CloseStream(bool bWaitForBuffers) = 0;
  virtual void SendMessage(CDVDMsg* pMsg, int priority) = 0;
  virtual bool IsInited() const = 0;
  virtual bool AcceptsData() const = 0;
  virtual bool IsStalled() const = 0;
};

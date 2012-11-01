#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "threads/Thread.h"
#include "DVDMessageQueue.h"
#include "video/TeletextDefines.h"

class CDVDStreamInfo;

class CDVDTeletextData : public CThread
{
public:
  CDVDTeletextData();
  ~CDVDTeletextData();

  bool CheckStream(CDVDStreamInfo &hints);
  bool OpenStream(CDVDStreamInfo &hints);
  void CloseStream(bool bWaitForBuffers);
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers() { m_messageQueue.WaitUntilEmpty(); }
  bool AcceptsData() const { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg) { if(m_messageQueue.IsInited()) m_messageQueue.Put(pMsg); }

  TextCacheStruct_t* GetTeletextCache() { return &m_TXTCache; }
  void LoadPage(int p, int sp, unsigned char* buffer);

protected:
  virtual void OnExit();
  virtual void Process();

private:
  void ResetTeletextCache();
  void Decode_p2829(unsigned char *vtxt_row, TextExtData_t **ptExtData);
  void SavePage(int p, int sp, unsigned char* buffer);
  void ErasePage(int magazine);
  void AllocateCache(int magazine);

  int m_speed;
  TextCacheStruct_t  m_TXTCache;
  CCriticalSection m_critSection;
  CDVDMessageQueue m_messageQueue;
};


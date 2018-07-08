/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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
#include "DVDMessageQueue.h"
#include "video/TeletextDefines.h"
#include "IVideoPlayer.h"

class CDVDStreamInfo;

class CDVDTeletextData : public CThread, public IDVDStreamPlayer
{
public:
  explicit CDVDTeletextData(CProcessInfo &processInfo);
  ~CDVDTeletextData() override;

  bool CheckStream(CDVDStreamInfo &hints);
  bool OpenStream(CDVDStreamInfo hints) override;
  void CloseStream(bool bWaitForBuffers) override;
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers() { m_messageQueue.WaitUntilEmpty(); }
  bool AcceptsData() const override { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0) override { if(m_messageQueue.IsInited()) m_messageQueue.Put(pMsg, priority); }
  void FlushMessages() override { m_messageQueue.Flush(); }
  bool IsInited() const override { return true; }
  bool IsStalled() const override { return true; }

  std::shared_ptr<TextCacheStruct_t> GetTeletextCache() { return m_TXTCache; }
  void LoadPage(int p, int sp, unsigned char* buffer);

protected:
  void OnExit() override;
  void Process() override;

private:
  void ResetTeletextCache();
  void Decode_p2829(unsigned char *vtxt_row, TextExtData_t **ptExtData);
  void SavePage(int p, int sp, unsigned char* buffer);
  void ErasePage(int magazine);
  void AllocateCache(int magazine);

  int m_speed;
  std::shared_ptr<TextCacheStruct_t> m_TXTCache = std::make_shared<TextCacheStruct_t>();
  CCriticalSection m_critSection;
  CDVDMessageQueue m_messageQueue;
};


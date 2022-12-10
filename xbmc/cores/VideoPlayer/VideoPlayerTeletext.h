/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDMessageQueue.h"
#include "IVideoPlayer.h"
#include "threads/Thread.h"
#include "video/TeletextDefines.h"

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
  void SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority = 0) override
  {
    if (m_messageQueue.IsInited())
      m_messageQueue.Put(pMsg, priority);
  }
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
  CDVDMessageQueue m_messageQueue;
};


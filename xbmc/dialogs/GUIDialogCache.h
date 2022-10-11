/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IFileTypes.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"

#include <string>

class CGUIDialogProgress;

class CGUIDialogCache : public CThread, public XFILE::IFileCallback
{
public:
  CGUIDialogCache(std::chrono::milliseconds delay = std::chrono::milliseconds(100),
                  const std::string& strHeader = "",
                  const std::string& strMsg = "");
  ~CGUIDialogCache() override;
  void SetHeader(const std::string& strHeader);
  void SetHeader(int nHeader);
  void SetMessage(const std::string& strMessage);
  bool IsCanceled() const;
  void ShowProgressBar(bool bOnOff);
  void SetPercentage(int iPercentage);

  void Close(bool bForceClose = false);

  void Process() override;
  bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) override;

protected:

  void OpenDialog();

  XbmcThreads::EndTime<> m_endtime;
  CGUIDialogProgress* m_pDlg;
  std::string m_strHeader;
  std::string m_strLinePrev;
  std::string m_strLinePrev2;
  bool bSentCancel;
};

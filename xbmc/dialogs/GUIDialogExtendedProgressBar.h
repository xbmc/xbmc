/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <string>
#include <vector>

class CGUIDialogProgressBarHandle
{
public:
  explicit CGUIDialogProgressBarHandle(const std::string& strTitle) : m_strTitle(strTitle) {}
  virtual ~CGUIDialogProgressBarHandle(void) = default;

  const std::string &Title(void) { return m_strTitle; }
  void SetTitle(const std::string &strTitle);

  std::string Text(void) const;
  void SetText(const std::string &strText);

  bool IsFinished(void) const { return m_bFinished; }
  void MarkFinished(void)     { m_bFinished = true; }

  float Percentage(void) const          { return m_fPercentage;}
  void SetPercentage(float fPercentage) { m_fPercentage = fPercentage; }
  void SetProgress(int currentItem, int itemCount);

private:
  mutable CCriticalSection m_critSection;
  float m_fPercentage = 0;
  std::string       m_strTitle;
  std::string       m_strText;
  bool m_bFinished = false;
};

class CGUIDialogExtendedProgressBar : public CGUIDialog
{
public:
  CGUIDialogExtendedProgressBar(void);
  ~CGUIDialogExtendedProgressBar(void) override = default;
  bool OnMessage(CGUIMessage& message) override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;

  CGUIDialogProgressBarHandle *GetHandle(const std::string &strTitle);

protected:
  void UpdateState(unsigned int currentTime);

  CCriticalSection                           m_critSection;
  unsigned int                               m_iCurrentItem;
  unsigned int                               m_iLastSwitchTime;
  std::vector<CGUIDialogProgressBarHandle *> m_handles;
};

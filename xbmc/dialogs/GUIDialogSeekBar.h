/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

class CGUIDialogSeekBar : public CGUIDialog
{
public:
  CGUIDialogSeekBar(void);
  ~CGUIDialogSeekBar(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void FrameMove() override;
private:
  int GetProgress() const;
  int GetEpgEventProgress() const;
  int GetTimeshiftProgress() const;

  int m_lastProgress = 0;
  int m_lastEpgEventProgress = 0;
  int m_lastTimeshiftProgress = 0;
};

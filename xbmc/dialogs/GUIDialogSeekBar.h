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
  float GetSeekPercent() const;
  int GetEpgEventProgress() const;
  int GetEpgEventSeekPercent() const;

  unsigned int m_lastPercent = ~0U;
  unsigned int m_lastEpgEventPercent = ~0U;
};

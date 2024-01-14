/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/Stopwatch.h"

class CGUIWindowVisualisation :
      public CGUIWindow
{
public:
  CGUIWindowVisualisation(void);
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void FrameMove() override;
protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;

  CStopWatch m_initTimer;
  CStopWatch m_lockedTimer;
  bool m_bShowPreset;
  MUSIC_INFO::CMusicInfoTag m_tag;    // current tag info, for finding when the info manager updates
};


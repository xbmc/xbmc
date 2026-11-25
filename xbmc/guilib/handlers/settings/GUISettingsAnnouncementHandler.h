/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"

class CGUISettingsAnnouncementHandler : public ANNOUNCEMENT::IAnnouncer
{
public:
  CGUISettingsAnnouncementHandler();
  ~CGUISettingsAnnouncementHandler() override;
  void OnSettingsEvent(const ANNOUNCEMENT::SettingsEvent& event) override;
};

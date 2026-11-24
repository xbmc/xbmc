/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/AnnouncementManager.h"

class CAnnounceReceiver : public ANNOUNCEMENT::IAnnouncer
{
public:
  static CAnnounceReceiver* GetInstance();

  void Initialize();
  void DeInitialize();

  void OnAnnouncement(const ANNOUNCEMENT::Announcement& announcement) override;

private:
  CAnnounceReceiver() = default;
};

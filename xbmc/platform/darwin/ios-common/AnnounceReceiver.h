/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/AnnouncementManager.h"

class CVariant;

class CAnnounceReceiver : public ANNOUNCEMENT::IAnnouncer
{
public:
  static CAnnounceReceiver* GetInstance();

  void Initialize();
  void DeInitialize();

  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const char* sender,
                const char* message,
                const CVariant& data) override;

private:
  CAnnounceReceiver() = default;
};

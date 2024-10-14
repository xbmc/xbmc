/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"

/*!
\brief Handler for announcements of type sources
*/
class CGUISourcesAnnouncementHandler : public ANNOUNCEMENT::IAnnouncer
{
public:
  CGUISourcesAnnouncementHandler();
  ~CGUISourcesAnnouncementHandler();

  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override;
};

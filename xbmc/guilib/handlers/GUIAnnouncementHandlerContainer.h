/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"

#include <memory>
#include <vector>

/*!
\brief This class is a container of announcement handlers per application component. It allows the GUI Layer
to execute GUI Actions upon receiving announcements from other components effectively decoupling GUI
from other components.
*/
class CGUIAnnouncementHandlerContainer final
{
public:
  CGUIAnnouncementHandlerContainer();
  ~CGUIAnnouncementHandlerContainer() = default;

private:
  std::vector<std::unique_ptr<ANNOUNCEMENT::IAnnouncer>> m_announcementHandlers;
};

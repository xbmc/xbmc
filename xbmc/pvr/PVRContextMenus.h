/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

class IContextMenuItem;

namespace PVR
{
  class CPVRContextMenuManager
  {
  public:
    static CPVRContextMenuManager& GetInstance();

    std::vector<std::shared_ptr<IContextMenuItem>> GetMenuItems() const { return m_items; }

  private:
    CPVRContextMenuManager();
    CPVRContextMenuManager(const CPVRContextMenuManager&) = delete;
    CPVRContextMenuManager const& operator=(CPVRContextMenuManager const&) = delete;
    virtual ~CPVRContextMenuManager() = default;

    std::vector<std::shared_ptr<IContextMenuItem>> m_items;
  };

} // namespace PVR

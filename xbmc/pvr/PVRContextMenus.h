/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/EventStream.h"

#include <memory>
#include <vector>

class IContextMenuItem;

namespace PVR
{
enum class PVRContextMenuEventAction
{
  ADD_ITEM,
  REMOVE_ITEM
};

struct PVRContextMenuEvent
{
  PVRContextMenuEvent(const PVRContextMenuEventAction& a,
                      const std::shared_ptr<IContextMenuItem>& i)
    : action(a), item(i)
  {
  }

  PVRContextMenuEventAction action;
  std::shared_ptr<IContextMenuItem> item;
};

class CPVRClientMenuHook;

class CPVRContextMenuManager
{
public:
  static CPVRContextMenuManager& GetInstance();

  std::vector<std::shared_ptr<IContextMenuItem>> GetMenuItems() const { return m_items; }

  void AddMenuHook(const CPVRClientMenuHook& hook);
  void RemoveMenuHook(const CPVRClientMenuHook& hook);

  /*!
   * @brief Query the events available for CEventStream
   */
  CEventStream<PVRContextMenuEvent>& Events() { return m_events; }

private:
  CPVRContextMenuManager();
  CPVRContextMenuManager(const CPVRContextMenuManager&) = delete;
  CPVRContextMenuManager const& operator=(CPVRContextMenuManager const&) = delete;
  virtual ~CPVRContextMenuManager() = default;

  std::vector<std::shared_ptr<IContextMenuItem>> m_items;
  CEventSource<PVRContextMenuEvent> m_events;
};

} // namespace PVR

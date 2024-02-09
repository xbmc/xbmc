/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameControllerProvider.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/GameServices.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/ports/windows/GUIPortDefines.h"
#include "guilib/GUIFont.h"
#include "guilib/GUIListItem.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace GAME;

namespace
{
// Prepend a "disconnected" icon to the list of ports
constexpr unsigned int ITEM_COUNT = MAX_PORT_COUNT + 1;
} // namespace

CGUIGameControllerProvider::CGUIGameControllerProvider(unsigned int portCount,
                                                       int portIndex,
                                                       const std::string& peripheralLocation,
                                                       uint32_t alignment,
                                                       int parentID)
  : IListProvider(parentID),
    m_portCount(portCount),
    m_portIndex(portIndex),
    m_peripheralLocation(peripheralLocation),
    m_alignment(alignment)
{
  InitializeItems();
}

CGUIGameControllerProvider::CGUIGameControllerProvider(const CGUIGameControllerProvider& other)
  : IListProvider(other.m_parentID),
    m_portCount(other.m_portCount),
    m_portIndex(other.m_portIndex),
    m_peripheralLocation(other.m_peripheralLocation),
    m_alignment(other.m_alignment)
{
  InitializeItems();
}

CGUIGameControllerProvider::~CGUIGameControllerProvider() = default;

std::unique_ptr<IListProvider> CGUIGameControllerProvider::Clone()
{
  return std::make_unique<CGUIGameControllerProvider>(*this);
}

bool CGUIGameControllerProvider::Update(bool forceRefresh)
{
  bool bDirty = false;
  std::swap(bDirty, m_bDirty);
  return bDirty;
}

void CGUIGameControllerProvider::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  items = m_items;
}

void CGUIGameControllerProvider::SetControllerProfile(ControllerPtr controllerProfile)
{
  const std::string oldControllerId = m_controllerProfile ? m_controllerProfile->ID() : "";
  const std::string newControllerId = controllerProfile ? controllerProfile->ID() : "";

  if (oldControllerId != newControllerId)
  {
    m_controllerProfile = std::move(controllerProfile);
    UpdateItems();
  }
}

void CGUIGameControllerProvider::SetPortCount(unsigned int portCount)
{
  if (m_portCount != portCount)
  {
    m_portCount = portCount;
    UpdateItems();
  }
}

void CGUIGameControllerProvider::SetPortIndex(int portIndex)
{
  if (m_portIndex != portIndex)
  {
    m_portIndex = portIndex;
    UpdateItems();
  }
}

void CGUIGameControllerProvider::SetPeripheralLocation(const std::string& peripheralLocation)
{
  if (m_peripheralLocation != peripheralLocation)
  {
    m_peripheralLocation = peripheralLocation;
    UpdateItems();
  }
}

void CGUIGameControllerProvider::InitializeItems()
{
  m_items.resize(ITEM_COUNT);
  for (auto& item : m_items)
    item = std::make_shared<CGUIListItem>();

  UpdateItems();
}

void CGUIGameControllerProvider::UpdateItems()
{
  ControllerPtr controller = m_controllerProfile;

  int portIndex = -1;
  for (unsigned int i = 0; i < static_cast<unsigned int>(m_items.size()); ++i)
  {
    std::shared_ptr<CGUIListItem>& guiItem = m_items.at(i);

    // Pad list if aligning to the right
    if (m_alignment == XBFONT_RIGHT && i + m_portCount < MAX_PORT_COUNT)
    {
      // Fully reset item state. Simply resetting the individual properties
      // is not enough, as other properties may also be set.
      guiItem = std::make_shared<CGUIListItem>();
      continue;
    }

    CFileItemPtr fileItem = std::make_shared<CFileItem>();

    // Set the item state for the current port index
    if (portIndex++ == m_portIndex && controller)
    {
      fileItem->SetLabel(controller->Layout().Label());
      fileItem->SetPath(m_peripheralLocation);
      fileItem->SetProperty("Addon.ID", controller->ID());
      fileItem->SetArt("icon", controller->Layout().ImagePath());
    }

    guiItem = std::move(fileItem);
  }

  m_bDirty = true;
}

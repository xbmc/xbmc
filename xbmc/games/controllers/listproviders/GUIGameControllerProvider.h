/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "guilib/listproviders/IListProvider.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Controller list provider for the \ref IAgentList control in the
 *        Player Viewer (<b>`GameAgents`</b>) window
 *
 * This list provider populates a game controller list with items that show
 * which emulator port a player's controller is connected to. Most items are
 * empty to pad the controller to its correct position in the list.
 *
 * The number of list items is determined by \ref MAX_PORT_COUNT, plus an item
 * for the "disconnected" icon. The list items are updated when the port count
 * or port index changes.
 *
 * An alignment can be specified to align the available ports to the left or
 * right side of the list.
 */
class CGUIGameControllerProvider : public IListProvider
{
public:
  /*!
   * \brief Construct a game controller provider for the player's controller
   *        list in the Player Viewer dialog
   *
   * \param portCount The number of open ports for the emulator
   * \param portIndex The index of the port the controller is connected to
   * \param peripheralLocation The location of the underlying peripheral
   *        providing input
   * \param alignment The alignment of the list items (<b>`XBFONT_LEFT`</b>
   *        or <b>`XBFONT_RIGHT`</b>)
   * \param parentID The ID of the parent window
   */
  CGUIGameControllerProvider(unsigned int portCount,
                             int portIndex,
                             const std::string& peripheralLocation,
                             uint32_t alignment,
                             int parentID);
  explicit CGUIGameControllerProvider(const CGUIGameControllerProvider& other);

  ~CGUIGameControllerProvider() override;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) override;
  bool OnClick(const std::shared_ptr<CGUIListItem>& item) override { return false; }
  bool OnInfo(const std::shared_ptr<CGUIListItem>& item) override { return false; }
  bool OnContextMenu(const std::shared_ptr<CGUIListItem>& item) override { return false; }
  void SetDefaultItem(int item, bool always) override {}
  int GetDefaultItem() const override { return -1; }
  bool AlwaysFocusDefaultItem() const override { return false; }

  // Game functions
  ControllerPtr GetControllerProfile() const { return m_controllerProfile; }
  void SetControllerProfile(ControllerPtr controllerProfile);
  unsigned int GetPortCount() const { return m_portCount; }
  void SetPortCount(unsigned int portCount);
  int GetPortIndex() const { return m_portIndex; }
  void SetPortIndex(int portIndex);
  const std::string& GetPeripheralLocation() const { return m_peripheralLocation; }
  void SetPeripheralLocation(const std::string& peripheralLocation);

private:
  // GUI functions
  void InitializeItems();
  void UpdateItems();

  // Game parameters
  ControllerPtr m_controllerProfile;
  unsigned int m_portCount{0};
  int m_portIndex{-1}; // Not connected
  std::string m_peripheralLocation;

  // GUI parameters
  const uint32_t m_alignment;
  std::vector<std::shared_ptr<CGUIListItem>> m_items;
  bool m_bDirty{true};
};
} // namespace GAME
} // namespace KODI

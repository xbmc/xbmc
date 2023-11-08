/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IPortList.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "games/controllers/dialogs/ControllerSelect.h"
#include "games/controllers/types/ControllerTree.h"

#include <map>
#include <memory>
#include <string>

class CFileItemList;
class CGUIViewControl;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup game
 */
class CGUIPortList : public IPortList
{
public:
  CGUIPortList(CGUIWindow& window);
  ~CGUIPortList() override;

  // Implementation of IPortList
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  bool Initialize(GameClientPtr gameClient) override;
  void Deinitialize() override;
  bool HasControl(int controlId) override;
  int GetCurrentControl() override;
  void Refresh() override;
  void FrameMove() override;
  void SetFocused() override;
  bool OnSelect() override;
  void ResetPorts() override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  bool AddItems(const CPortNode& port, unsigned int& itemId, const std::string& itemLabel);
  void CleanupItems();
  void OnItemFocus(unsigned int itemIndex);
  void OnItemSelect(unsigned int itemIndex);

  // Controller selection callback
  void OnControllerSelected(const CPortNode& port, const ControllerPtr& controller);

  static std::string GetLabel(const CPortNode& port);

  // Construction parameters
  CGUIWindow& m_guiWindow;

  // GUI parameters
  CControllerSelect m_controllerSelectDialog;
  std::string m_focusedPort; // Address of focused port
  int m_currentItem{-1}; // Index of the selected item, or -1 if no item is selected
  std::unique_ptr<CGUIViewControl> m_viewControl;
  std::unique_ptr<CFileItemList> m_vecItems;

  // Game parameters
  GameClientPtr m_gameClient;
  std::map<unsigned int, std::string> m_itemToAddress; // item index -> port address
  std::map<std::string, unsigned int> m_addressToItem; // port address -> item index
};
} // namespace GAME
} // namespace KODI

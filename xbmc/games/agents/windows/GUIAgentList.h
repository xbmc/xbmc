/*
 *  Copyright (C) 2022-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAgentList.h"
#include "addons/AddonEvents.h"
#include "games/GameTypes.h"
#include "games/controllers/ControllerTypes.h"
#include "utils/Observer.h"

#include <map>
#include <memory>
#include <set>
#include <string>

class CFileItem;
class CFileItemList;
class CGUIViewControl;
class CGUIWindow;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIAgentList : public IAgentList, public Observer
{
public:
  CGUIAgentList(CGUIWindow& window);
  ~CGUIAgentList() override;

  // Implementation of IAgentList
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  bool Initialize(GameClientPtr gameClient) override;
  void Deinitialize() override;
  bool HasControl(int controlId) const override;
  int GetCurrentControl() const override;
  void FrameMove() override;
  void Refresh() override;
  void SetFocused() override;
  void OnSelect() override;

  // Implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  // Add-on API
  void OnEvent(const ADDON::AddonEvent& event);

  // GUI functions
  void AddItem(const CGameAgent& agent);
  void CleanupItems();
  void OnItemFocus(unsigned int itemIndex);
  void OnAgentFocus(const std::string& focusedAgent);
  void OnItemSelect(unsigned int itemIndex);
  void OnAgentSelect(const CFileItem& selectedAgentItem);
  void ShowAgentDialog(const CGameAgent& agent);

  // Construction parameters
  CGUIWindow& m_guiWindow;

  // GUI parameters
  std::unique_ptr<CGUIViewControl> m_viewControl;
  std::unique_ptr<CFileItemList> m_vecItems;
  unsigned int m_currentItem{0};
  std::string m_currentAgent;

  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI

/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerActions.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/dialogs/disc/DialogGameDiscManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <cstddef>
#include <optional>
#include <set>

using namespace KODI;
using namespace GAME;

CDiscManagerActions::CDiscManagerActions(CDialogGameDiscManager& discManager)
  : m_discManager(discManager)
{
}

void CDiscManagerActions::Initialize(GameClientPtr gameClient)
{
  m_gameClient = std::move(gameClient);
}

void CDiscManagerActions::Deinitialize()
{
  m_gameClient.reset();
}

void CDiscManagerActions::OnSelectDisc()
{
  if (!m_gameClient)
    return;

  CGameClientDiscs& discs = m_gameClient->Discs();

  // Do nothing if the disc isn't ejected
  if (!discs.IsEjected())
    return;

  // Get currently-selected disc
  const CGameClientDiscModel& discList = discs.GetDiscs();
  const std::optional<size_t> selectedIndex = discList.GetSelectedDiscIndex();

  m_discManager.SelectDiscToInsert(selectedIndex,
                                   [this, selectedIndex](std::optional<size_t> discIndex)
                                   {
                                     // Do nothing if the selection didn't change
                                     if (selectedIndex == discIndex)
                                       return;

                                     if (discIndex.has_value())
                                     {
                                       if (!m_gameClient->Discs().InsertDiscByIndex(*discIndex))
                                         ShowInternalError();
                                     }
                                     else if (!m_gameClient->Discs().InsertDisc(""))
                                     {
                                       ShowInternalError();
                                     }

                                     m_discManager.UpdateMenu();
                                   });
}

void CDiscManagerActions::OnEjectInsert()
{
  if (!m_gameClient)
    return;

  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  CGameClientDiscs& discs = m_gameClient->Discs();

  const bool wasEjected = discs.IsEjected();

  const bool success = discs.SetEjected(!wasEjected);

  if (!success)
  {
    if (wasEjected)
    {
      // "Error"
      // "The disc can't be inserted right now."
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{strings.Get(257)},
                                           CVariant{strings.Get(35279)});
    }
    else
    {
      // "Error"
      // "The disc can't be ejected right now."
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{strings.Get(257)},
                                           CVariant{strings.Get(35278)});
    }
  }

  m_discManager.UpdateMenu();
}

void CDiscManagerActions::OnAdd()
{
  if (!m_gameClient)
    return;

  CGameClientDiscs& discs = m_gameClient->Discs();

  // Do nothing if the disc isn't ejected
  if (!discs.IsEjected())
    return;

  const CGameClientDiscModel& discModel = discs.GetDiscs();

  std::string startingPath = discModel.GetSelectedDiscPath();

  // Fall back to first valid disc
  if (startingPath.empty())
  {
    for (const GameClientDiscEntry& disc : discModel.GetDiscs())
    {
      if (!disc.path.empty())
      {
        startingPath = disc.path;
        break;
      }
    }
  }

  // Fall back to currently playing game
  if (startingPath.empty())
    startingPath = m_gameClient->GetGamePath();

  std::string filePath;
  if (!BrowseForDiscImage(startingPath, filePath) || filePath.empty())
    return;

  if (!discs.AddDisc(filePath))
    ShowInternalError();

  m_discManager.UpdateMenu();
}

void CDiscManagerActions::OnRemove()
{
  if (!m_gameClient)
    return;

  CGameClientDiscs& discs = m_gameClient->Discs();

  // Do nothing if the disc isn't ejected
  if (!discs.IsEjected())
    return;

  m_discManager.SelectDiscToRemove(
      [this](size_t discIndex)
      {
        if (!m_gameClient->Discs().RemoveDiscByIndex(discIndex))
          ShowInternalError();

        m_discManager.UpdateMenu();
      });
}

void CDiscManagerActions::OnApplyDiscChange()
{
  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_GAME_DISC_CHANGER);
}

void CDiscManagerActions::OnResumeGame()
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                             "PlayerControl(Play)");
}

bool CDiscManagerActions::BrowseForDiscImage(const std::string& startingPath, std::string& filePath)
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  std::set<std::string> extensions = m_gameClient->GetExtensions();
  if (extensions.empty())
    extensions = CGameUtils::GetGameExtensions();

  const std::string strExtensions = StringUtils::Join(extensions, "|");

  return CGUIDialogFileBrowser::ShowAndGetFile(startingPath, strExtensions,
                                               strings.Get(35280), // "Select disc"
                                               filePath);
}

void CDiscManagerActions::ShowInternalError()
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  // "Error"
  // "The emulator \"{0:s}\" had an internal error."
  MESSAGING::HELPERS::ShowOKDialogText(
      CVariant{strings.Get(257)},
      CVariant{StringUtils::Format(strings.Get(35213), m_gameClient->Name())});
}

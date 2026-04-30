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
#include "games/dialogs/disc/DiscManagerIDs.h"
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
                                   [this](std::optional<size_t> discIndex)
                                   {
                                     bool success = false;
                                     if (discIndex.has_value())
                                     {
                                       success =
                                           m_gameClient->Discs().InsertDiscByIndex(*discIndex);
                                       if (!success)
                                         ShowInternalError();
                                     }
                                     else
                                     {
                                       success = m_gameClient->Discs().InsertDisc("");
                                       if (!success)
                                         ShowInternalError();
                                     }

                                     m_discManager.UpdateMenu();

                                     // Returning from a successful selection of a disc should land on Resume
                                     if (success)
                                       m_discManager.FocusMainMenuItem(MENU_INDEX_RESUME_GAME);
                                     else
                                       m_discManager.FocusMainMenuItem(MENU_INDEX_SELECT_DISC);
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
  {
    m_discManager.FocusMainMenuItem(MENU_INDEX_ADD_DISC);
    return;
  }

  const bool success = discs.AddDisc(filePath);
  if (!success)
    ShowInternalError();

  m_discManager.UpdateMenu();

  if (success)
    m_discManager.FocusMainMenuItem(MENU_INDEX_SELECT_DISC);
  else
    m_discManager.FocusMainMenuItem(MENU_INDEX_ADD_DISC);
}

void CDiscManagerActions::OnDelete()
{
  if (!m_gameClient)
    return;

  CGameClientDiscs& discs = m_gameClient->Discs();

  // Do nothing if the disc isn't ejected
  if (!discs.IsEjected())
    return;

  m_discManager.SelectDiscToDelete(
      [this](size_t discIndex)
      {
        const bool success = m_gameClient->Discs().RemoveDiscByIndex(discIndex);
        if (!success)
          ShowInternalError();

        m_discManager.UpdateMenu();

        if (success)
          m_discManager.FocusMainMenuItem(MENU_INDEX_SELECT_DISC);
        else
          m_discManager.FocusMainMenuItem(MENU_INDEX_DELETE_DISC);
      });
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

  // Disc Manager "Add disc" should accept concrete disc images only.
  // .m3u is a playlist/container used for launch-time disc sets, not a
  // single disc image.
  extensions.erase(".m3u");

  const std::string strExtensions = StringUtils::Join(extensions, "|");

  return CGUIDialogFileBrowser::ShowAndGetFile(startingPath, strExtensions,
                                               strings.Get(35280), // "Disc selection"
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

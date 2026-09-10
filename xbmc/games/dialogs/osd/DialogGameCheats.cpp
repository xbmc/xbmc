/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameCheats.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/cheats/GameClientCheats.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMacros.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "jobs/JobManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/StringUtils.h"

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace KODI::GAME;

namespace
{
constexpr auto SETTING_CHEAT_PREFIX = "cheat";
constexpr auto SETTING_GET_MORE = "getmore";

//! "Get more..."
constexpr int HEADING_GET_MORE = 21452;

//! "Cheats"
constexpr int HEADING_CHEATS = 35320;

//! "Enabled"
constexpr int HEADING_ENABLED = 305;

//! "No cheats found for this game"
constexpr int HEADING_NO_CHEATS = 35323;

//! The label above the list of what is switched on, a size larger than the
//! names below it, which a single control could not do: the text markup
//! carries no size of its own
constexpr int CONTROL_ENABLED_HEADING = 6001;

//! Cheat files are written by hand and a description is sometimes a sentence
//! of instructions rather than a name. Past this the text runs under the switch.
constexpr size_t MAX_LABEL_LENGTH = 55;

//! Cuts a label to a width the dialog can show. Counted in characters and cut
//! on a character boundary: a byte count both cuts short text that happens to
//! be multibyte and can split a character into invalid UTF-8.
std::string TruncateCharacters(const std::string& text, size_t characters)
{
  size_t pos = 0;
  for (size_t seen = 0; seen < characters && pos < text.size(); ++seen)
  {
    // Step over the character's continuation bytes, which are 10xxxxxx and do
    // not begin one, the same way utf8_strlen counts them
    do
      ++pos;
    while (pos < text.size() && (static_cast<unsigned char>(text[pos]) & 0xC0) == 0x80);
  }

  return text.substr(0, pos);
}

std::string SettingId(size_t index)
{
  return StringUtils::Format("{}{}", SETTING_CHEAT_PREFIX, index);
}
} // namespace

CDialogGameCheats::CDialogGameCheats()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_GAME_CHEATS, "DialogGameCheats.xml")
{
}

CDialogGameCheats::~CDialogGameCheats() = default;

void CDialogGameCheats::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(HEADING_CHEATS);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_OKAY_BUTTON);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 15067); // "Close"
}

std::string CDialogGameCheats::GetSettingsLabel(const std::shared_ptr<ISetting>& setting)
{
  const auto label = m_labels.find(setting->GetId());
  if (label != m_labels.end())
    return label->second;

  return CGUIDialogSettingsManualBase::GetSettingsLabel(setting);
}

std::string CDialogGameCheats::EnabledSummary() const
{
  const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
  if (!gameClient)
    return "";

  std::vector<std::string> names;
  for (const Cheat& cheat : gameClient->Cheats().GetCheats())
  {
    if (cheat.enabled)
      names.emplace_back(!cheat.description.empty() ? cheat.description : cheat.code);
  }

  if (names.empty())
    return "";

  // Centred in a narrow column, so the names stand alone under the heading
  // rather than carrying bullets that would sit raggedly against them
  return StringUtils::Join(names, "[CR]");
}

void CDialogGameCheats::SetDescription(const CVariant& label)
{
  // What is switched on is listed under the focused cheat's explanation, so
  // the player can see it without scrolling the list back
  const std::string summary = EnabledSummary();

  SET_CONTROL_LABEL(CONTROL_ENABLED_HEADING,
                    summary.empty()
                        ? ""
                        : CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                              HEADING_ENABLED));

  const BaseSettingControlPtr control = GetSettingControl(m_iSetting);
  if (control != nullptr && control->GetSetting() != nullptr)
  {
    const auto description = m_descriptions.find(control->GetSetting()->GetId());
    if (description != m_descriptions.end())
    {
      CGUIDialogSettingsManualBase::SetDescription(
          CVariant{summary.empty() ? description->second
                                   : description->second + "[CR][CR]" + summary});
      return;
    }

    // A cheat the file said nothing more about: the summary alone rather than
    // the heading the settings framework falls back to
    if (m_labels.find(control->GetSetting()->GetId()) != m_labels.end())
    {
      CGUIDialogSettingsManualBase::SetDescription(CVariant{summary});
      return;
    }
  }

  CGUIDialogSettingsManualBase::SetDescription(label);
}

void CDialogGameCheats::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("gamecheats", HEADING_CHEATS);
  if (category == nullptr)
    return;

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == nullptr)
    return;

  m_labels.clear();
  m_descriptions.clear();

  const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
  if (!gameClient)
    return;

  const std::vector<Cheat> cheats = gameClient->Cheats().GetCheats();
  for (size_t index = 0; index < cheats.size(); ++index)
  {
    const Cheat& cheat = cheats[index];
    const std::string id = SettingId(index);

    // A cheat file is allowed to leave a cheat unnamed, and a switch with no
    // label cannot be told apart from the ones around it
    std::string label = !cheat.description.empty() ? cheat.description : cheat.code;
    if (StringUtils::utf8_strlen(label) > MAX_LABEL_LENGTH)
      label = TruncateCharacters(label, MAX_LABEL_LENGTH - 1) + "\u2026";

    m_labels[id] = std::move(label);

    if (!cheat.longDescription.empty())
      m_descriptions[id] = cheat.longDescription;
    else if (StringUtils::utf8_strlen(cheat.description) > MAX_LABEL_LENGTH)
      m_descriptions[id] = cheat.description;

    // GetSettingsLabel() supplies the real label; a setting still has to be
    // given a string ID to be created at all
    AddToggle(group, id, HEADING_CHEATS, SettingLevel::Basic, cheat.enabled);
  }

  // The player has no other way of learning the cheat database exists
  if (gameClient->Cheats().CanInstallCheats())
    AddButton(group, SETTING_GET_MORE, HEADING_GET_MORE, SettingLevel::Basic);
}

void CDialogGameCheats::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  if (setting->GetId() != SETTING_GET_MORE)
    return;

  const std::shared_ptr<CJobManager> jobManager = CServiceBroker::GetJobManager();
  if (!jobManager)
    return;

  // The download blocks, and the dialog is rebuilt on the GUI thread once the
  // add-on is in place and the game has been looked up again.
  //
  // An update rather than an init: the player can close the dialog while the
  // download runs, and initialising a closed one marks it active without the
  // window manager knowing, after which it never opens again.
  jobManager->Submit(
      []()
      {
        const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
        if (gameClient && !gameClient->Cheats().InstallCheats())
        {
          // The add-on is in place but holds nothing for this game, which a
          // renamed or homebrew title will hit. Rebuilding would leave the
          // dialog with no switches and no offer, and no word of why.
          CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info,
                                                CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(HEADING_CHEATS),
                                                CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(HEADING_NO_CHEATS));
        }

        CGUIMessage message(GUI_MSG_UPDATE, WINDOW_DIALOG_GAME_CHEATS,
                            WINDOW_DIALOG_GAME_CHEATS);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message,
                                                                       WINDOW_DIALOG_GAME_CHEATS);
      });
}

void CDialogGameCheats::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string& id = setting->GetId();
  if (!StringUtils::StartsWith(id, SETTING_CHEAT_PREFIX))
    return;

  const std::string index = id.substr(std::strlen(SETTING_CHEAT_PREFIX));
  if (index.empty() || !StringUtils::IsNaturalNumber(index))
    return;

  const GameClientPtr gameClient = CGameUtils::GetPlayingGameClient();
  if (!gameClient)
    return;

  gameClient->Cheats().SetEnabled(
      static_cast<unsigned int>(std::stoul(index)),
      std::static_pointer_cast<const CSettingBool>(setting)->GetValue());

  // The panel lists what is on, so it follows every toggle
  SetDescription(CVariant{""});
}

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAddonInfo.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/AddonRepos.h"
#include "addons/AddonSystemSettings.h"
#include "addons/AudioDecoder.h"
#include "addons/ExtsMimeSupportList.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/gui/GUIHelpers.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "games/GameUtils.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/builtins/Builtins.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pictures/GUIWindowSlideShow.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <functional>
#include <memory>
#include <sstream>
#include <utility>

#define CONTROL_BTN_INSTALL 6
#define CONTROL_BTN_ENABLE 7
#define CONTROL_BTN_UPDATE 8
#define CONTROL_BTN_SETTINGS 9
#define CONTROL_BTN_DEPENDENCIES 10
#define CONTROL_BTN_SELECT 12
#define CONTROL_BTN_AUTOUPDATE 13
#define CONTROL_BTN_VERSIONS 14
#define CONTROL_LIST_SCREENSHOTS 50

using namespace KODI;
using namespace ADDON;
using namespace KODI::ADDONS;
using namespace XFILE;
using namespace KODI::MESSAGING;

CGUIDialogAddonInfo::CGUIDialogAddonInfo(void)
  : CGUIDialog(WINDOW_DIALOG_ADDON_INFO, "DialogAddonInfo.xml"),
    m_item(std::make_shared<CFileItem>())
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogAddonInfo::~CGUIDialogAddonInfo(void) = default;

bool CGUIDialogAddonInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_UPDATE)
      {
        OnUpdate();
        return true;
      }
      else if (iControl == CONTROL_BTN_INSTALL)
      {
        const auto& itemAddonInfo = m_item->GetAddonInfo();
        if (!CServiceBroker::GetAddonMgr().IsAddonInstalled(
                itemAddonInfo->ID(), itemAddonInfo->Origin(), itemAddonInfo->Version()))
        {
          OnInstall();
          return true;
        }
        else
        {
          m_silentUninstall = false;
          OnUninstall();
          return true;
        }
      }
      else if (iControl == CONTROL_BTN_SELECT)
      {
        OnSelect();
        return true;
      }
      else if (iControl == CONTROL_BTN_ENABLE)
      {
        OnEnableDisable();
        return true;
      }
      else if (iControl == CONTROL_BTN_SETTINGS)
      {
        OnSettings();
        return true;
      }
      else if (iControl == CONTROL_BTN_DEPENDENCIES)
      {
        ShowDependencyList(Reactivate::CHOICE_YES, EntryPoint::SHOW_DEPENDENCIES);
        return true;
      }
      else if (iControl == CONTROL_BTN_AUTOUPDATE)
      {
        OnToggleAutoUpdates();
        return true;
      }
      else if (iControl == CONTROL_LIST_SCREENSHOTS)
      {
        if (message.GetParam1() == ACTION_SELECT_ITEM ||
            message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          OnMessage(msg);
          int start = msg.GetParam1();
          if (start >= 0 && start < static_cast<int>(m_item->GetAddonInfo()->Screenshots().size()))
            CGUIWindowSlideShow::RunSlideShow(m_item->GetAddonInfo()->Screenshots(), start);
        }
      }
      else if (iControl == CONTROL_BTN_VERSIONS)
      {
        OnSelectVersion();
        return true;
      }
    }
    break;
    default:
      break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogAddonInfo::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_SHOW_INFO)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogAddonInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  BuildDependencyList();
  UpdateControls(PerformButtonFocus::CHOICE_YES);
}

void CGUIDialogAddonInfo::UpdateControls(PerformButtonFocus performButtonFocus)
{
  if (!m_item)
    return;

  const auto& itemAddonInfo = m_item->GetAddonInfo();
  bool isInstalled = CServiceBroker::GetAddonMgr().IsAddonInstalled(
      itemAddonInfo->ID(), itemAddonInfo->Origin(), itemAddonInfo->Version());
  m_addonEnabled =
      m_localAddon && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_localAddon->ID());
  bool canDisable =
      isInstalled && CServiceBroker::GetAddonMgr().CanAddonBeDisabled(m_localAddon->ID());
  bool canInstall = !isInstalled && itemAddonInfo->LifecycleState() != AddonLifecycleState::BROKEN;
  bool canUninstall = m_localAddon && CServiceBroker::GetAddonMgr().CanUninstall(m_localAddon);

  bool isUpdate = (!isInstalled && CServiceBroker::GetAddonMgr().IsAddonInstalled(
                                       itemAddonInfo->ID(), itemAddonInfo->Origin()));

  bool showUpdateButton = m_localAddon &&
                          CServiceBroker::GetAddonMgr().IsAutoUpdateable(m_localAddon->ID()) &&
                          m_item->GetProperty("Addon.HasUpdate").asBoolean();

  if (isInstalled)
  {
    SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, 24037); // uninstall
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, canUninstall);
  }
  else
  {
    if (isUpdate)
    {
      SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, 24138); // update
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, 24038); // install
    }

    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, canInstall);
    if (canInstall && performButtonFocus == PerformButtonFocus::CHOICE_YES)
    {
      SET_CONTROL_FOCUS(CONTROL_BTN_INSTALL, 0);
    }
  }

  if (showUpdateButton)
  {
    SET_CONTROL_VISIBLE(CONTROL_BTN_UPDATE);
    SET_CONTROL_HIDDEN(CONTROL_BTN_VERSIONS);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_BTN_VERSIONS);
    SET_CONTROL_HIDDEN(CONTROL_BTN_UPDATE);
  }

  if (m_addonEnabled)
  {
    SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, 24021);
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ENABLE, canDisable);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTN_ENABLE, 24022);
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_ENABLE, isInstalled);
  }

  bool autoUpdatesOn = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                           CSettings::SETTING_ADDONS_AUTOUPDATES) == AUTO_UPDATES_ON;
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_AUTOUPDATE, isInstalled && autoUpdatesOn);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_AUTOUPDATE,
                       isInstalled && autoUpdatesOn &&
                           CServiceBroker::GetAddonMgr().IsAutoUpdateable(m_localAddon->ID()));
  SET_CONTROL_LABEL(CONTROL_BTN_AUTOUPDATE, 21340);

  const bool active = m_localAddon && CAddonSystemSettings::GetInstance().IsActive(*m_localAddon);
  CONTROL_ENABLE_ON_CONDITION(
      CONTROL_BTN_SELECT,
      m_addonEnabled && (CanShowSupportList() || CanOpen() || CanRun() || (CanUse() && !active)));

  int label;
  if (CanShowSupportList())
    label = 21484;
  else if (CanUse())
    label = 21480;
  else if (CanOpen())
    label = 21478;
  else
    label = 21479;
  SET_CONTROL_LABEL(CONTROL_BTN_SELECT, label);

  const bool hasSettings = m_localAddon && m_localAddon->CanHaveAddonOrInstanceSettings();
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, isInstalled && hasSettings);
  if (isInstalled && hasSettings && performButtonFocus == PerformButtonFocus::CHOICE_YES)
  {
    SET_CONTROL_FOCUS(CONTROL_BTN_SETTINGS, 0);
  }

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_DEPENDENCIES, !m_depsInstalledWithAvailable.empty());

  CFileItemList items;
  for (const auto& screenshot : m_item->GetAddonInfo()->Screenshots())
  {
    auto item = std::make_shared<CFileItem>("");
    item->SetArt("thumb", screenshot);
    items.Add(std::move(item));
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_SCREENSHOTS, 0, 0, &items);
  OnMessage(msg);
}

static const std::string LOCAL_CACHE =
    "\\0_local_cache"; // \0 to give it the lowest priority when sorting

int CGUIDialogAddonInfo::AskForVersion(std::vector<std::pair<CAddonVersion, std::string>>& versions)
{
  auto dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT);
  dialog->Reset();
  dialog->SetHeading(CVariant{21338});
  dialog->SetUseDetails(true);

  for (const auto& versionInfo : versions)
  {
    CFileItem item(StringUtils::Format(g_localizeStrings.Get(21339), versionInfo.first.asString()));
    if (m_localAddon && m_localAddon->Version() == versionInfo.first &&
        m_item->GetAddonInfo()->Origin() == versionInfo.second)
      item.Select(true);

    AddonPtr repo;
    if (versionInfo.second == LOCAL_CACHE)
    {
      item.SetLabel2(g_localizeStrings.Get(24095));
      item.SetArt("icon", "DefaultAddonRepository.png");
      dialog->Add(item);
    }
    else if (CServiceBroker::GetAddonMgr().GetAddon(versionInfo.second, repo, AddonType::REPOSITORY,
                                                    OnlyEnabled::CHOICE_YES))
    {
      item.SetLabel2(repo->Name());
      item.SetArt("icon", repo->Icon());
      dialog->Add(item);
    }
  }

  dialog->Open();
  return dialog->IsConfirmed() ? dialog->GetSelectedItem() : -1;
}

void CGUIDialogAddonInfo::OnUpdate()
{
  const auto& itemAddonInfo = m_item->GetAddonInfo();
  const std::string& addonId = itemAddonInfo->ID();
  const std::string& origin = m_item->GetProperty("Addon.ValidUpdateOrigin").asString();
  const CAddonVersion& version =
      static_cast<CAddonVersion>(m_item->GetProperty("Addon.ValidUpdateVersion").asString());

  Close();
  if (!m_depsInstalledWithAvailable.empty() &&
      !ShowDependencyList(Reactivate::CHOICE_NO, EntryPoint::UPDATE))
    return;

  CAddonInstaller::GetInstance().Install(addonId, version, origin);
}

void CGUIDialogAddonInfo::OnSelectVersion()
{
  if (!m_item->HasAddonInfo())
    return;

  const std::string& processAddonId = m_item->GetAddonInfo()->ID();
  EntryPoint entryPoint = m_localAddon ? EntryPoint::UPDATE : EntryPoint::INSTALL;

  // get all compatible versions of an addon-id regardless of their origin
  std::vector<std::shared_ptr<IAddon>> compatibleVersions =
      CServiceBroker::GetAddonMgr().GetCompatibleVersions(processAddonId);

  std::vector<std::pair<CAddonVersion, std::string>> versions;
  versions.reserve(compatibleVersions.size());

  for (const auto& compatibleVersion : compatibleVersions)
    versions.emplace_back(compatibleVersion->Version(), compatibleVersion->Origin());

  CAddonDatabase database;
  database.Open();

  CFileItemList items;
  if (XFILE::CDirectory::GetDirectory("special://home/addons/packages/", items, ".zip",
                                      DIR_FLAG_NO_FILE_DIRS))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      std::string packageId;
      std::string versionString;
      if (CAddonVersion::SplitFileName(packageId, versionString, items[i]->GetLabel()))
      {
        if (packageId == processAddonId)
        {
          std::string hash;
          std::string path(items[i]->GetPath());
          if (database.GetPackageHash(processAddonId, items[i]->GetPath(), hash))
          {
            std::string sha256 = CUtil::GetFileDigest(path, KODI::UTILITY::CDigest::Type::SHA256);

            // don't offer locally cached packages that result in an invalid version.
            // usually this happens when the package filename gets malformed on the fs
            // e.g. downloading "http://localhost/a+b.zip" ends up in "a b.zip"
            const CAddonVersion version(versionString);
            if (StringUtils::EqualsNoCase(sha256, hash) && !version.empty())
              versions.emplace_back(version, LOCAL_CACHE);
          }
        }
      }
    }
  }

  if (versions.empty())
    HELPERS::ShowOKDialogText(CVariant{21341}, CVariant{21342});
  else
  {
    int i = AskForVersion(versions);
    if (i != -1)
    {
      Close();

      if (versions[i].second == LOCAL_CACHE)
      {
        CAddonInstaller::GetInstance().InstallFromZip(
            StringUtils::Format("special://home/addons/packages/{}-{}.zip", processAddonId,
                                versions[i].first.asString()));
      }
      else
      {
        if (!m_depsInstalledWithAvailable.empty() &&
            !ShowDependencyList(Reactivate::CHOICE_NO, entryPoint))
          return;
        CAddonInstaller::GetInstance().Install(processAddonId, versions[i].first,
                                               versions[i].second);
      }
    }
  }
}

void CGUIDialogAddonInfo::OnToggleAutoUpdates()
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), CONTROL_BTN_AUTOUPDATE);
  if (OnMessage(msg))
  {
    bool selected = msg.GetParam1() == 1;
    if (selected)
      CServiceBroker::GetAddonMgr().RemoveAllUpdateRulesFromList(m_localAddon->ID());
    else
      CServiceBroker::GetAddonMgr().AddUpdateRuleToList(m_localAddon->ID(),
                                                        AddonUpdateRule::USER_DISABLED_AUTO_UPDATE);

    bool showUpdateButton = (selected && m_item->GetProperty("Addon.HasUpdate").asBoolean());

    if (showUpdateButton)
    {
      SET_CONTROL_VISIBLE(CONTROL_BTN_UPDATE);
      SET_CONTROL_HIDDEN(CONTROL_BTN_VERSIONS);
    }
    else
    {
      SET_CONTROL_VISIBLE(CONTROL_BTN_VERSIONS);
      SET_CONTROL_HIDDEN(CONTROL_BTN_UPDATE);
    }

    CServiceBroker::GetAddonMgr().PublishEventAutoUpdateStateChanged(m_localAddon->ID());
  }
}

void CGUIDialogAddonInfo::OnInstall()
{
  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  if (!m_item->HasAddonInfo())
    return;

  const auto& itemAddonInfo = m_item->GetAddonInfo();
  const std::string& origin = itemAddonInfo->Origin();

  if (m_localAddon && CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode() !=
                          AddonRepoUpdateMode::ANY_REPOSITORY)
  {
    if (m_localAddon->Origin() != origin && m_localAddon->Origin() != ORIGIN_SYSTEM)
    {
      const std::string& header = g_localizeStrings.Get(19098); // Warning!
      const std::string origin =
          !m_localAddon->Origin().empty() ? m_localAddon->Origin() : g_localizeStrings.Get(39029);
      const std::string text =
          StringUtils::Format(g_localizeStrings.Get(39028), m_localAddon->Name(), origin,
                              m_localAddon->Version().asString());

      if (CGUIDialogYesNo::ShowAndGetInput(header, text))
      {
        m_silentUninstall = true;
        OnUninstall();
      }
      else
      {
        return;
      }
    }
  }

  const std::string& addonId = itemAddonInfo->ID();
  const CAddonVersion& version = itemAddonInfo->Version();

  Close();
  if (!m_depsInstalledWithAvailable.empty() &&
      !ShowDependencyList(Reactivate::CHOICE_NO, EntryPoint::INSTALL))
    return;

  CAddonInstaller::GetInstance().Install(addonId, version, origin);
}

void CGUIDialogAddonInfo::OnSelect()
{
  if (!m_localAddon)
    return;

  if (CanShowSupportList())
  {
    ShowSupportList();
    return;
  }

  Close();

  if (CanOpen() || CanRun())
    CBuiltins::GetInstance().Execute("RunAddon(" + m_localAddon->ID() + ")");
  else if (CanUse())
    CAddonSystemSettings::GetInstance().SetActive(m_localAddon->Type(), m_localAddon->ID());
}

bool CGUIDialogAddonInfo::CanOpen() const
{
  return m_localAddon && m_localAddon->Type() == AddonType::PLUGIN;
}

bool CGUIDialogAddonInfo::CanRun() const
{
  if (m_localAddon)
  {
    if (m_localAddon->Type() == AddonType::SCRIPT)
      return true;

    if (GAME::CGameUtils::IsStandaloneGame(m_localAddon))
      return true;
  }

  return false;
}

bool CGUIDialogAddonInfo::CanUse() const
{
  return m_localAddon && (m_localAddon->Type() == AddonType::SKIN ||
                          m_localAddon->Type() == AddonType::SCREENSAVER ||
                          m_localAddon->Type() == AddonType::VISUALIZATION ||
                          m_localAddon->Type() == AddonType::SCRIPT_WEATHER ||
                          m_localAddon->Type() == AddonType::RESOURCE_LANGUAGE ||
                          m_localAddon->Type() == AddonType::RESOURCE_UISOUNDS ||
                          m_localAddon->Type() == AddonType::AUDIOENCODER);
}

bool CGUIDialogAddonInfo::CanShowSupportList() const
{
  return m_localAddon && (m_localAddon->Type() == AddonType::AUDIODECODER ||
                          m_localAddon->Type() == AddonType::IMAGEDECODER);
}

bool CGUIDialogAddonInfo::PromptIfDependency(int heading, int line2)
{
  if (!m_localAddon)
    return false;

  VECADDONS addons;
  std::vector<std::string> deps;
  CServiceBroker::GetAddonMgr().GetAddons(addons);
  for (VECADDONS::const_iterator it = addons.begin(); it != addons.end(); ++it)
  {
    auto i =
        std::find_if((*it)->GetDependencies().begin(), (*it)->GetDependencies().end(),
                     [&](const DependencyInfo& other) { return other.id == m_localAddon->ID(); });
    if (i != (*it)->GetDependencies().end() && !i->optional) // non-optional dependency
      deps.push_back((*it)->Name());
  }

  if (!deps.empty())
  {
    std::string line0 = StringUtils::Format(g_localizeStrings.Get(24046), m_localAddon->Name());
    std::string line1 = StringUtils::Join(deps, ", ");
    HELPERS::ShowOKDialogLines(CVariant{heading}, CVariant{std::move(line0)},
                               CVariant{std::move(line1)}, CVariant{line2});
    return true;
  }
  return false;
}

void CGUIDialogAddonInfo::OnUninstall()
{
  if (!m_localAddon.get())
    return;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  // ensure the addon is not a dependency of other installed addons
  if (PromptIfDependency(24037, 24047))
    return;

  // prompt user to be sure
  if (!m_silentUninstall && !CGUIDialogYesNo::ShowAndGetInput(CVariant{24037}, CVariant{750}))
    return;

  bool removeData = false;
  if (CDirectory::Exists(m_localAddon->Profile()))
    removeData = CGUIDialogYesNo::ShowAndGetInput(CVariant{24037}, CVariant{39014});

  CAddonInstaller::GetInstance().UnInstall(m_localAddon, removeData);

  Close();
}

void CGUIDialogAddonInfo::OnEnableDisable()
{
  if (!m_localAddon)
    return;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  if (m_addonEnabled)
  {
    if (PromptIfDependency(24075, 24091))
      return; //required. can't disable

    CServiceBroker::GetAddonMgr().DisableAddon(m_localAddon->ID(), AddonDisabledReason::USER);
  }
  else
  {
    // Check user want to enable if lifecycle not normal
    if (!ADDON::GUI::CHelpers::DialogAddonLifecycleUseAsk(m_localAddon))
      return;

    CServiceBroker::GetAddonMgr().EnableAddon(m_localAddon->ID());
  }

  UpdateControls(PerformButtonFocus::CHOICE_NO);
}

void CGUIDialogAddonInfo::OnSettings()
{
  CGUIDialogAddonSettings::ShowForAddon(m_localAddon);
}

bool CGUIDialogAddonInfo::ShowDependencyList(Reactivate reactivate, EntryPoint entryPoint)
{
  if (entryPoint != EntryPoint::INSTALL || m_showDepDialogOnInstall)
  {
    auto pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
        WINDOW_DIALOG_SELECT);
    CFileItemList items;

    for (const auto& it : m_depsInstalledWithAvailable)
    {
      // All combinations of depAddon and localAddon validity are possible and information
      // must be displayed even when there is no depAddon.
      // info_addon is the add-on to take the information to display (name, icon) from. The
      // version in the repository is preferred because it might contain more recent data.

      std::shared_ptr<IAddon> infoAddon = it.m_available ? it.m_available : it.m_installed;

      if (infoAddon)
      {
        if (entryPoint != EntryPoint::UPDATE || !it.IsInstalledUpToDate())
        {
          const CFileItemPtr item = std::make_shared<CFileItem>(infoAddon->Name());
          int messageId = 24180; // minversion only

          // dep not installed locally, but it is available from a repo!
          // make sure only non-optional add-ons that meet versionMin are
          // announced for installation

          if (!it.m_installed)
          {
            if (entryPoint != EntryPoint::SHOW_DEPENDENCIES && !it.m_depInfo.optional)
            {
              if (it.m_depInfo.versionMin <= it.m_available->Version())
              {
                messageId = 24181; // => install
              }
              else
              {
                messageId = 24185; // => not available, only lower versions available in the repos
              }
            }
          }
          else // dep is installed locally
          {
            messageId = 24182; // => installed

            if (!it.IsInstalledUpToDate())
            {
              messageId = 24183; // => update to
            }
          }

          if (entryPoint == EntryPoint::SHOW_DEPENDENCIES ||
              infoAddon->MainType() != AddonType::SCRIPT_MODULE ||
              !CAddonRepos::IsFromOfficialRepo(infoAddon, CheckAddonPath::CHOICE_NO))
          {
            item->SetLabel2(StringUtils::Format(
                g_localizeStrings.Get(messageId), it.m_depInfo.versionMin.asString(),
                it.m_installed ? it.m_installed->Version().asString() : "",
                it.m_available ? it.m_available->Version().asString() : "",
                it.m_depInfo.optional ? g_localizeStrings.Get(24184) : ""));

            item->SetArt("icon", infoAddon->Icon());
            item->SetProperty("addon_id", it.m_depInfo.id);
            items.Add(item);
          }
        }
      }
      else
      {
        const CFileItemPtr item = std::make_shared<CFileItem>(it.m_depInfo.id);
        item->SetLabel2(g_localizeStrings.Get(10005)); // Not available
        items.Add(item);
      }
    }

    if (!items.IsEmpty())
    {
      CFileItemPtr backup_item = GetCurrentListItem();
      while (true)
      {
        pDialog->Reset();
        pDialog->SetHeading(reactivate == Reactivate::CHOICE_YES ? 39024 : 39020);
        pDialog->SetUseDetails(true);
        for (auto& it : items)
          pDialog->Add(*it);
        pDialog->EnableButton(reactivate == Reactivate::CHOICE_NO, 186);
        pDialog->SetButtonFocus(true);
        pDialog->Open();

        if (pDialog->IsButtonPressed())
          return true;

        if (pDialog->IsConfirmed())
        {
          const CFileItemPtr& item = pDialog->GetSelectedFileItem();
          std::string addon_id = item->GetProperty("addon_id").asString();
          std::shared_ptr<IAddon> depAddon;
          if (CServiceBroker::GetAddonMgr().FindInstallableById(addon_id, depAddon))
          {
            Close();
            ShowForItem(std::make_shared<CFileItem>(depAddon));
          }
        }
        else
          break;
      }
      SetItem(backup_item);
      if (reactivate == Reactivate::CHOICE_YES)
        Open();

      return false;
    }
  }

  return true;
}

void CGUIDialogAddonInfo::ShowSupportList()
{
  std::vector<KODI::ADDONS::AddonSupportEntry> list;
  if (CanShowSupportList())
    list =
        CServiceBroker::GetExtsMimeSupportList().GetSupportedExtsAndMimeTypes(m_localAddon->ID());

  auto pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT);
  CFileItemList items;
  for (const auto& entry : list)
  {
    // Ignore included extension about track support
    if (StringUtils::EndsWith(entry.m_name, KODI_ADDON_AUDIODECODER_TRACK_EXT))
      continue;

    std::string label;
    if (entry.m_type == AddonSupportType::Extension)
      label = StringUtils::Format(g_localizeStrings.Get(21346), entry.m_name);
    else if (entry.m_type == AddonSupportType::Mimetype)
      label = StringUtils::Format(g_localizeStrings.Get(21347), entry.m_name);
    else
      label = entry.m_name;

    const CFileItemPtr item = std::make_shared<CFileItem>(label);
    item->SetLabel2(entry.m_description);
    if (!entry.m_icon.empty())
      item->SetArt("icon", entry.m_icon);
    else if (entry.m_type == AddonSupportType::Extension)
      item->SetArt("icon", "DefaultExtensionInfo.png");
    else if (entry.m_type == AddonSupportType::Mimetype)
      item->SetArt("icon", "DefaultMimetypeInfo.png");
    item->SetProperty("addon_id", m_localAddon->ID());
    items.Add(item);
  }

  pDialog->Reset();
  pDialog->SetHeading(21485);
  pDialog->SetUseDetails(true);
  for (auto& it : items)
    pDialog->Add(*it);
  pDialog->SetButtonFocus(true);
  pDialog->Open();
}

bool CGUIDialogAddonInfo::ShowForItem(const CFileItemPtr& item)
{
  if (!item)
    return false;

  CGUIDialogAddonInfo* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogAddonInfo>(
          WINDOW_DIALOG_ADDON_INFO);
  if (!dialog)
    return false;
  if (!dialog->SetItem(item))
    return false;

  dialog->Open();
  return true;
}

bool CGUIDialogAddonInfo::SetItem(const CFileItemPtr& item)
{
  if (!item || !item->HasAddonInfo())
    return false;

  m_item = std::make_shared<CFileItem>(*item);
  m_localAddon.reset();
  if (CServiceBroker::GetAddonMgr().GetAddon(item->GetAddonInfo()->ID(), m_localAddon,
                                             OnlyEnabled::CHOICE_NO))
  {
    CLog::Log(LOGDEBUG, "{} - Addon with id {} not found locally.", __FUNCTION__,
              item->GetAddonInfo()->ID());
  }
  return true;
}

void CGUIDialogAddonInfo::BuildDependencyList()
{
  if (!m_item)
    return;

  m_showDepDialogOnInstall = false;
  m_depsInstalledWithAvailable.clear();
  m_deps = CServiceBroker::GetAddonMgr().GetDepsRecursive(m_item->GetAddonInfo()->ID(),
                                                          OnlyEnabledRootAddon::CHOICE_NO);

  for (const auto& dep : m_deps)
  {
    std::shared_ptr<IAddon> addonInstalled;
    std::shared_ptr<IAddon> addonAvailable;

    // Find add-on in local installation
    if (!CServiceBroker::GetAddonMgr().GetAddon(dep.id, addonInstalled, OnlyEnabled::CHOICE_YES))
    {
      addonInstalled = nullptr;
    }

    // Find add-on in repositories
    if (!CServiceBroker::GetAddonMgr().FindInstallableById(dep.id, addonAvailable))
    {
      addonAvailable = nullptr;
    }

    if (!addonInstalled)
    {

      // after pushing the install button the dependency install dialog
      // will be opened only if...
      // - dependencies are unavailable (for informational purposes) OR
      // - the dependency is not a script/module                     OR
      // - the script/module is not available at an official repo
      if (!addonAvailable || addonAvailable->MainType() != AddonType::SCRIPT_MODULE ||
          !CAddonRepos::IsFromOfficialRepo(addonAvailable, CheckAddonPath::CHOICE_NO))
      {
        m_showDepDialogOnInstall = true;
      }
    }
    else
    {

      // only display dialog if updates for already installed dependencies will install
      if (addonAvailable && addonAvailable->Version() > addonInstalled->Version())
      {
        m_showDepDialogOnInstall = true;
      }
    }

    m_depsInstalledWithAvailable.emplace_back(dep, addonInstalled, addonAvailable);
  }

  std::sort(m_depsInstalledWithAvailable.begin(), m_depsInstalledWithAvailable.end(),
            [](const auto& a, const auto& b) {
              // 1. "not installed/available" go to the bottom first
              const bool depAInstalledOrAvailable =
                  a.m_installed != nullptr || a.m_available != nullptr;
              const bool depBInstalledOrAvailable =
                  b.m_installed != nullptr || b.m_available != nullptr;

              if (depAInstalledOrAvailable != depBInstalledOrAvailable)
              {
                return !depAInstalledOrAvailable;
              }

              // 2. then optional add-ons to top
              if (a.m_depInfo.optional != b.m_depInfo.optional)
              {
                return a.m_depInfo.optional;
              }

              // 3. addon type asc, except scripts/modules at the bottom
              const std::shared_ptr<IAddon>& depA = a.m_installed ? a.m_installed : a.m_available;
              const std::shared_ptr<IAddon>& depB = b.m_installed ? b.m_installed : b.m_available;

              if (depA && depB)
              {
                const AddonType typeA = depA->MainType();
                const AddonType typeB = depB->MainType();
                if (typeA != typeB)
                {
                  if ((typeA == AddonType::SCRIPT_MODULE) == (typeB == AddonType::SCRIPT_MODULE))
                  {
                    // both are scripts/modules or neither one is => sort by addon type asc
                    return typeA < typeB;
                  }
                  else
                  {
                    // At this point, either:
                    // A is script/module and B is not, or A is not script/module and B is.
                    // the script/module goes to the bottom
                    return typeA != AddonType::SCRIPT_MODULE;
                  }
                }
              }

              // 4. finally order by addon-id
              return a.m_depInfo.id < b.m_depInfo.id;
            });
}

bool CInstalledWithAvailable::IsInstalledUpToDate() const
{
  if (m_installed)
  {
    if (!m_available || m_available->Version() == m_installed->Version())
    {
      return true;
    }
  }

  return false;
}

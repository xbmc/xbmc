/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAddonInfo.h"

#include "FileItem.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "games/GameUtils.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "interfaces/builtins/Builtins.h"
#include "messaging/helpers/DialogHelper.h"
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
#include <sstream>
#include <utility>

#define CONTROL_BTN_INSTALL 6
#define CONTROL_BTN_ENABLE 7
#define CONTROL_BTN_UPDATE 8
#define CONTROL_BTN_SETTINGS 9
#define CONTROL_BTN_DEPENDENCIES 10
#define CONTROL_BTN_SELECT 12
#define CONTROL_BTN_AUTOUPDATE 13
#define CONTROL_LIST_SCREENSHOTS 50

using namespace KODI;
using namespace ADDON;
using namespace XFILE;
using namespace KODI::MESSAGING;

CGUIDialogAddonInfo::CGUIDialogAddonInfo(void)
  : CGUIDialog(WINDOW_DIALOG_ADDON_INFO, "DialogAddonInfo.xml")
{
  m_item = CFileItemPtr(new CFileItem);
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
      if (iControl == CONTROL_BTN_INSTALL)
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
        auto deps = CServiceBroker::GetAddonMgr().GetDepsRecursive(m_item->GetAddonInfo()->ID());
        ShowDependencyList(deps, true);
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
  UpdateControls();
  CGUIDialog::OnInitWindow();
}

void CGUIDialogAddonInfo::UpdateControls()
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
  bool canInstall = !isInstalled && !m_item->GetAddonInfo()->IsBroken();
  bool canUninstall = m_localAddon && CServiceBroker::GetAddonMgr().CanUninstall(m_localAddon);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_INSTALL, canInstall || canUninstall);
  SET_CONTROL_LABEL(CONTROL_BTN_INSTALL, isInstalled ? 24037 : 24038);

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

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_UPDATE, isInstalled);

  bool autoUpdatesOn = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
                           CSettings::SETTING_ADDONS_AUTOUPDATES) == AUTO_UPDATES_ON;
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_AUTOUPDATE, isInstalled && autoUpdatesOn);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_AUTOUPDATE,
                       isInstalled && autoUpdatesOn &&
                           !CServiceBroker::GetAddonMgr().IsBlacklisted(m_localAddon->ID()));
  SET_CONTROL_LABEL(CONTROL_BTN_AUTOUPDATE, 21340);

  CONTROL_ENABLE_ON_CONDITION(
      CONTROL_BTN_SELECT,
      m_addonEnabled && (CanOpen() || CanRun() || (CanUse() && !m_localAddon->IsInUse())));
  SET_CONTROL_LABEL(CONTROL_BTN_SELECT, CanUse() ? 21480 : (CanOpen() ? 21478 : 21479));

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_SETTINGS, isInstalled && m_localAddon->HasSettings());

  auto deps = CServiceBroker::GetAddonMgr().GetDepsRecursive(m_item->GetAddonInfo()->ID());
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_DEPENDENCIES, !deps.empty());

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


int CGUIDialogAddonInfo::AskForVersion(std::vector<std::pair<AddonVersion, std::string>>& versions)
{
  auto dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT);
  dialog->Reset();
  dialog->SetHeading(CVariant{21338});
  dialog->SetUseDetails(true);

  std::sort(versions.begin(), versions.end(), std::greater<std::pair<AddonVersion, std::string>>());

  for (const auto& versionInfo : versions)
  {
    CFileItem item(StringUtils::Format(g_localizeStrings.Get(21339).c_str(),
                                       versionInfo.first.asString().c_str()));
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
    else if (CServiceBroker::GetAddonMgr().GetAddon(versionInfo.second, repo, ADDON_REPOSITORY))
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
  if (!m_localAddon)
    return;

  std::vector<std::pair<AddonVersion, std::string>> versions;

  CAddonDatabase database;
  database.Open();
  database.GetAvailableVersions(m_localAddon->ID(), versions);

  CFileItemList items;
  if (XFILE::CDirectory::GetDirectory("special://home/addons/packages/", items, ".zip",
                                      DIR_FLAG_NO_FILE_DIRS))
  {
    for (int i = 0; i < items.Size(); ++i)
    {
      std::string packageId;
      std::string versionString;
      if (AddonVersion::SplitFileName(packageId, versionString, items[i]->GetLabel()))
      {
        if (packageId == m_localAddon->ID())
        {
          std::string hash;
          std::string path(items[i]->GetPath());
          if (database.GetPackageHash(m_localAddon->ID(), items[i]->GetPath(), hash))
          {
            std::string md5 = CUtil::GetFileDigest(path, KODI::UTILITY::CDigest::Type::MD5);
            if (StringUtils::EqualsNoCase(md5, hash))
              versions.emplace_back(AddonVersion(versionString), LOCAL_CACHE);
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
      //turn auto updating off if downgrading
      if (m_localAddon->Version() > versions[i].first)
        CServiceBroker::GetAddonMgr().AddToUpdateBlacklist(m_localAddon->ID());

      if (versions[i].second == LOCAL_CACHE)
        CAddonInstaller::GetInstance().InstallFromZip(
            StringUtils::Format("special://home/addons/packages/%s-%s.zip",
                                m_localAddon->ID().c_str(), versions[i].first.asString().c_str()));
      else
        CAddonInstaller::GetInstance().Install(m_localAddon->ID(), versions[i].first,
                                               versions[i].second);
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
      CServiceBroker::GetAddonMgr().RemoveFromUpdateBlacklist(m_localAddon->ID());
    else
      CServiceBroker::GetAddonMgr().AddToUpdateBlacklist(m_localAddon->ID());
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

  if (m_localAddon && (m_localAddon->Origin() != origin) &&
      (CAddonSystemSettings::GetInstance().GetAddonRepoUpdateMode() !=
       AddonRepoUpdateMode::ANY_REPOSITORY))
  {
    const std::string& header = g_localizeStrings.Get(19098); // Warning!
    const std::string text =
        StringUtils::Format(g_localizeStrings.Get(39028), m_localAddon->ID(),
                            m_localAddon->Origin(), m_localAddon->Version().asString());

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

  const std::string& addonId = itemAddonInfo->ID();
  const AddonVersion& version = itemAddonInfo->Version();

  Close();
  const auto& deps = CServiceBroker::GetAddonMgr().GetDepsRecursive(addonId);
  if (!deps.empty() && !ShowDependencyList(deps, false))
    return;

  CAddonInstaller::GetInstance().Install(addonId, version, origin);
}

void CGUIDialogAddonInfo::OnSelect()
{
  if (!m_localAddon)
    return;

  Close();

  if (CanOpen() || CanRun())
    CBuiltins::GetInstance().Execute("RunAddon(" + m_localAddon->ID() + ")");
  else if (CanUse())
    CAddonSystemSettings::GetInstance().SetActive(m_localAddon->Type(), m_localAddon->ID());
}

bool CGUIDialogAddonInfo::CanOpen() const
{
  return m_localAddon && m_localAddon->Type() == ADDON_PLUGIN;
}

bool CGUIDialogAddonInfo::CanRun() const
{
  if (m_localAddon)
  {
    if (m_localAddon->Type() == ADDON_SCRIPT)
      return true;

    if (GAME::CGameUtils::IsStandaloneGame(m_localAddon))
      return true;
  }

  return false;
}

bool CGUIDialogAddonInfo::CanUse() const
{
  return m_localAddon &&
         (m_localAddon->Type() == ADDON_SKIN || m_localAddon->Type() == ADDON_SCREENSAVER ||
          m_localAddon->Type() == ADDON_VIZ || m_localAddon->Type() == ADDON_SCRIPT_WEATHER ||
          m_localAddon->Type() == ADDON_RESOURCE_LANGUAGE ||
          m_localAddon->Type() == ADDON_RESOURCE_UISOUNDS);
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
    std::string line0 =
        StringUtils::Format(g_localizeStrings.Get(24046).c_str(), m_localAddon->Name().c_str());
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
  if (CDirectory::Exists("special://profile/addon_data/" + m_localAddon->ID()))
    removeData = CGUIDialogYesNo::ShowAndGetInput(CVariant{24037}, CVariant{39014});

  CJobManager::GetInstance().AddJob(new CAddonUnInstallJob(m_localAddon, removeData),
                                    &CAddonInstaller::GetInstance());
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
    CServiceBroker::GetAddonMgr().EnableAddon(m_localAddon->ID());

  UpdateControls();
}

void CGUIDialogAddonInfo::OnSettings()
{
  CGUIDialogAddonSettings::ShowForAddon(m_localAddon);
}

bool CGUIDialogAddonInfo::ShowDependencyList(const std::vector<ADDON::DependencyInfo>& deps,
                                             bool reactivate)
{
  auto pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT);
  CFileItemList items;
  for (auto& it : deps)
  {
    AddonPtr dep_addon, local_addon, info_addon;
    // Find add-on in repositories
    CServiceBroker::GetAddonMgr().FindInstallableById(it.id, dep_addon);
    // Find add-on in local installation
    CServiceBroker::GetAddonMgr().GetAddon(it.id, local_addon);

    // All combinations of dep_addon and local_addon validity are possible and information
    // must be displayed even when there is no dep_addon.
    // info_addon is the add-on to take the information to display (name, icon) from. The
    // version in the repository is preferred because it might contain more recent data.
    info_addon = dep_addon ? dep_addon : local_addon;

    if (info_addon)
    {
      CFileItemPtr item(new CFileItem(info_addon->Name()));
      std::stringstream str;
      str << it.id << " " << it.versionMin.asString() << " -> " << it.version.asString();
      if ((it.optional && !local_addon) || (!it.optional && local_addon))
        str << " "
            << StringUtils::Format(g_localizeStrings.Get(39022).c_str(),
                                   local_addon ? g_localizeStrings.Get(39019).c_str()
                                               : g_localizeStrings.Get(39018).c_str());
      else if (it.optional && local_addon)
        str << " "
            << StringUtils::Format(g_localizeStrings.Get(39023).c_str(),
                                   g_localizeStrings.Get(39019).c_str(),
                                   g_localizeStrings.Get(39018).c_str());

      item->SetLabel2(str.str());
      item->SetArt("icon", info_addon->Icon());
      item->SetProperty("addon_id", it.id);
      items.Add(item);
    }
    else
    {
      CFileItemPtr item(new CFileItem(it.id));
      item->SetLabel2(g_localizeStrings.Get(10005)); // Not available
      items.Add(item);
    }
  }

  CFileItemPtr backup_item = GetCurrentListItem();
  while (true)
  {
    pDialog->Reset();
    pDialog->SetHeading(reactivate ? 39024 : 39020);
    pDialog->SetUseDetails(true);
    for (auto& it : items)
      pDialog->Add(*it);
    pDialog->EnableButton(!reactivate, 186);
    pDialog->SetButtonFocus(true);
    pDialog->Open();

    if (pDialog->IsButtonPressed())
      return true;

    if (pDialog->IsConfirmed())
    {

      const CFileItemPtr& item = pDialog->GetSelectedFileItem();
      std::string addon_id = item->GetProperty("addon_id").asString();
      AddonPtr dep_addon;
      if (CServiceBroker::GetAddonMgr().FindInstallableById(addon_id, dep_addon))
      {
        Close();
        ShowForItem(CFileItemPtr(new CFileItem(dep_addon)));
      }
    }
    else
      break;
  }
  SetItem(backup_item);
  if (reactivate)
    Open();

  return false;
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
  CServiceBroker::GetAddonMgr().GetAddon(item->GetAddonInfo()->ID(), m_localAddon, ADDON_UNKNOWN,
                                         false);
  return true;
}

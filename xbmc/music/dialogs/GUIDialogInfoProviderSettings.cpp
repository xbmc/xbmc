/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogInfoProviderSettings.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "addons/gui/GUIWindowAddonBrowser.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/builtins/Builtins.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "storage/MediaManager.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <limits.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace ADDON;

const std::string SETTING_ALBUMSCRAPER_SETTINGS = "albumscrapersettings";
const std::string SETTING_ARTISTSCRAPER_SETTINGS = "artistscrapersettings";
const std::string SETTING_APPLYTOITEMS = "applysettingstoitems";

CGUIDialogInfoProviderSettings::CGUIDialogInfoProviderSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_INFOPROVIDER_SETTINGS, "DialogSettings.xml")
{ }

bool CGUIDialogInfoProviderSettings::Show()
{
  CGUIDialogInfoProviderSettings *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogInfoProviderSettings>(WINDOW_DIALOG_INFOPROVIDER_SETTINGS);
  if (!dialog)
    return false;

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  dialog->m_showSingleScraper = false;

  // Get current default info provider settings from service broker
  dialog->m_fetchInfo = settings->GetBool(CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO);

  ADDON::AddonPtr defaultScraper;
  // Get default album scraper (when enabled - can default scraper be disabled??)
  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::AddonType::SCRAPER_ALBUMS,
                                                           defaultScraper))
  {
    ADDON::ScraperPtr scraper = std::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper);
    dialog->SetAlbumScraper(scraper);
  }

  // Get default artist scraper
  if (ADDON::CAddonSystemSettings::GetInstance().GetActive(ADDON::AddonType::SCRAPER_ARTISTS,
                                                           defaultScraper))
  {
    ADDON::ScraperPtr scraper = std::dynamic_pointer_cast<ADDON::CScraper>(defaultScraper);
    dialog->SetArtistScraper(scraper);
  }

  dialog->m_strArtistInfoPath = settings->GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);

  dialog->Open();

  dialog->ResetDefaults();
  return dialog->IsConfirmed();
}

int CGUIDialogInfoProviderSettings::Show(ADDON::ScraperPtr& scraper)
{
  CGUIDialogInfoProviderSettings *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogInfoProviderSettings>(WINDOW_DIALOG_INFOPROVIDER_SETTINGS);
  if (!dialog || !scraper)
    return -1;
  if (scraper->Content() != CONTENT_ARTISTS && scraper->Content() != CONTENT_ALBUMS)
    return -1;

  dialog->m_showSingleScraper = true;
  dialog->m_singleScraperType = scraper->Content();

  if (dialog->m_singleScraperType == CONTENT_ALBUMS)
    dialog->SetAlbumScraper(scraper);
  else
    dialog->SetArtistScraper(scraper);
  // toast selected but disabled scrapers
  if (CServiceBroker::GetAddonMgr().IsAddonDisabled(scraper->ID()))
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24024), scraper->Name(), 2000, true);

  dialog->Open();

  bool confirmed = dialog->IsConfirmed();
  unsigned int applyToItems = dialog->m_applyToItems;
  if (confirmed)
  {
    if (dialog->m_singleScraperType == CONTENT_ALBUMS)
      scraper = dialog->GetAlbumScraper();
    else
    {
      scraper = dialog->GetArtistScraper();
      // Save artist information folder (here not in the caller) when applying setting as default for all artists
      if (applyToItems == INFOPROVIDERAPPLYOPTIONS::INFOPROVIDER_DEFAULT)
        CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, dialog->m_strArtistInfoPath);
    }
    if (scraper)
      scraper->SetPathSettings(dialog->m_singleScraperType, "");
  }

  dialog->ResetDefaults();

  if (confirmed)
    return applyToItems;
  else
    return -1;
}

void CGUIDialogInfoProviderSettings::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
}

void CGUIDialogInfoProviderSettings::OnSettingChanged(
    const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO)
  {
    m_fetchInfo = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    SetupView();
    SetFocus(CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO);
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER)
    m_strArtistInfoPath = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  else if (settingId == SETTING_APPLYTOITEMS)
  {
    m_applyToItems = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    SetupView();
    SetFocus(SETTING_APPLYTOITEMS);
  }
}

void CGUIDialogInfoProviderSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER)
  {
    std::string currentScraperId;
    if (m_albumscraper)
      currentScraperId = m_albumscraper->ID();
    std::string selectedAddonId = currentScraperId;

    if (CGUIWindowAddonBrowser::SelectAddonID(AddonType::SCRAPER_ALBUMS, selectedAddonId, false) ==
            1 &&
        selectedAddonId != currentScraperId)
    {
      AddonPtr scraperAddon;
      if (CServiceBroker::GetAddonMgr().GetAddon(selectedAddonId, scraperAddon,
                                                 OnlyEnabled::CHOICE_YES))
      {
        m_albumscraper = std::dynamic_pointer_cast<CScraper>(scraperAddon);
        SetupView();
        SetFocus(settingId);
      }
      else
      {
        CLog::Log(LOGERROR, "{} - Could not get settings for addon: {}", __FUNCTION__,
                  selectedAddonId);
      }
    }
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER)
  {
    std::string currentScraperId;
    if (m_artistscraper)
      currentScraperId = m_artistscraper->ID();
    std::string selectedAddonId = currentScraperId;

    if (CGUIWindowAddonBrowser::SelectAddonID(AddonType::SCRAPER_ARTISTS, selectedAddonId, false) ==
            1 &&
        selectedAddonId != currentScraperId)
    {
      AddonPtr scraperAddon;
      if (CServiceBroker::GetAddonMgr().GetAddon(selectedAddonId, scraperAddon,
                                                 OnlyEnabled::CHOICE_YES))
      {
        m_artistscraper = std::dynamic_pointer_cast<CScraper>(scraperAddon);
        SetupView();
        SetFocus(settingId);
      }
      else
      {
        CLog::Log(LOGERROR, "{} - Could not get addon: {}", __FUNCTION__, selectedAddonId);
      }
    }
  }
  else if (settingId == SETTING_ALBUMSCRAPER_SETTINGS)
    CGUIDialogAddonSettings::ShowForAddon(m_albumscraper, false);
  else if (settingId == SETTING_ARTISTSCRAPER_SETTINGS)
    CGUIDialogAddonSettings::ShowForAddon(m_artistscraper, false);
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER)
  {
    VECSOURCES shares;
    CServiceBroker::GetMediaManager().GetLocalDrives(shares);
    CServiceBroker::GetMediaManager().GetNetworkLocations(shares);
    CServiceBroker::GetMediaManager().GetRemovableDrives(shares);
    std::string strDirectory = m_strArtistInfoPath;
    if (!strDirectory.empty())
    {
      URIUtils::AddSlashAtEnd(strDirectory);
      bool bIsSource;
      if (CUtil::GetMatchingSource(strDirectory, shares, bIsSource) < 0) // path is outside shares - add it as a separate one
      {
        CMediaSource share;
        share.strName = g_localizeStrings.Get(13278);
        share.strPath = strDirectory;
        shares.push_back(share);
      }
    }
    else
      strDirectory = "default location";

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(20223), strDirectory, true))
    {
      if (!strDirectory.empty())
      {
        m_strArtistInfoPath = strDirectory;
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, strDirectory);
        SetFocus(CSettings::CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
      }
    }
  }
}

bool CGUIDialogInfoProviderSettings::Save()
{
  if (m_showSingleScraper)
    return true; //Save done by caller of ::Show

  // Save default settings for fetching additional information and art
  CLog::Log(LOGINFO, "{} called", __FUNCTION__);
  // Save Fetch addiitional info during update
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->SetBool(CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO, m_fetchInfo);
  // Save default scrapers and addon setting values
  settings->SetString(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, m_albumscraper->ID());
  m_albumscraper->SaveSettings();
  settings->SetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, m_artistscraper->ID());
  m_artistscraper->SaveSettings();
  // Save artist information folder
  settings->SetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, m_strArtistInfoPath);
  settings->Save();

  return true;
}

void CGUIDialogInfoProviderSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);

  SetLabel2(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, m_strArtistInfoPath);

  if (!m_showSingleScraper)
  {
    SetHeading(38330);
    if (!m_fetchInfo)
    {
      ToggleState(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, false);
      ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
      ToggleState(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, false);
      ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
      ToggleState(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, false);
    }
    else
    {  // Album scraper
      ToggleState(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, true);
      if (m_albumscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_albumscraper->ID()))
      {
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, m_albumscraper->Name());
        if (m_albumscraper && m_albumscraper->HasSettings())
          ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, true);
        else
          ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
      }
      else
      {
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, g_localizeStrings.Get(231)); //Set label2 to "None"
        ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
      }
      // Artist scraper
      ToggleState(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, true);
      if (m_artistscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_artistscraper->ID()))
      {
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, m_artistscraper->Name());
        if (m_artistscraper && m_artistscraper->HasSettings())
          ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, true);
        else
          ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
      }
      else
      {
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, g_localizeStrings.Get(231)); //Set label2 to "None"
        ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
      }
      // Artist Information Folder
      ToggleState(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, true);
    }
  }
  else if (m_singleScraperType == CONTENT_ALBUMS)
  {
    SetHeading(38331);
    // Album scraper
    ToggleState(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, true);
    if (m_albumscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_albumscraper->ID()))
    {
      SetLabel2(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, m_albumscraper->Name());
      if (m_albumscraper && m_albumscraper->HasSettings())
        ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, true);
      else
        ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
    }
    else
    {
      SetLabel2(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, g_localizeStrings.Get(231)); //Set label2 to "None"
      ToggleState(SETTING_ALBUMSCRAPER_SETTINGS, false);
    }
  }
  else
  {
    SetHeading(38332);
    // Artist scraper
    ToggleState(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, true);
    if (m_artistscraper && !CServiceBroker::GetAddonMgr().IsAddonDisabled(m_artistscraper->ID()))
    {
      SetLabel2(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, m_artistscraper->Name());
      if (m_artistscraper && m_artistscraper->HasSettings())
        ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, true);
      else
        ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
    }
    else
    {
      SetLabel2(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, g_localizeStrings.Get(231)); //Set label2 to "None"
      ToggleState(SETTING_ARTISTSCRAPER_SETTINGS, false);
    }
    // Artist Information Folder when default settings
    ToggleState(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, m_applyToItems == INFOPROVIDER_DEFAULT);
  }
}

void CGUIDialogInfoProviderSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  std::shared_ptr<CSettingCategory> category = AddCategory("infoprovidersettings", -1);
  if (category == nullptr)
  {
    CLog::Log(LOGERROR, "{}: unable to setup settings", __FUNCTION__);
    return;
  }
  std::shared_ptr<CSettingGroup> group1 = AddGroup(category);
  if (group1 == nullptr)
  {
    CLog::Log(LOGERROR, "{}: unable to setup settings", __FUNCTION__);
    return;
  }

  if (!m_showSingleScraper)
  {
    AddToggle(group1, CSettings::SETTING_MUSICLIBRARY_DOWNLOADINFO, 38333, SettingLevel::Basic, m_fetchInfo); // "Fetch additional information during scan"
  }
  else
  {
    TranslatableIntegerSettingOptions entries;
    entries.clear();
    if (m_singleScraperType == CONTENT_ALBUMS)
    {
      entries.emplace_back(38066, INFOPROVIDER_THISITEM);
      entries.emplace_back(38067, INFOPROVIDER_ALLVIEW);
    }
    else
    {
      entries.emplace_back(38064, INFOPROVIDER_THISITEM);
      entries.emplace_back(38065, INFOPROVIDER_ALLVIEW);
    }
    entries.emplace_back(38063, INFOPROVIDER_DEFAULT);
    AddList(group1, SETTING_APPLYTOITEMS, 38338, SettingLevel::Basic, m_applyToItems, entries, 38339); // "Apply settings to"
  }

  std::shared_ptr<CSettingGroup> group = AddGroup(category, 38337);
  if (group == nullptr)
  {
    CLog::Log(LOGERROR, "{}: unable to setup settings", __FUNCTION__);
    return;
  }
  std::shared_ptr<CSettingAction> subsetting;
  if (!m_showSingleScraper || m_singleScraperType == CONTENT_ALBUMS)
  {
    AddButton(group, CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, 38334, SettingLevel::Basic); //Provider for album information
    subsetting = AddButton(group, SETTING_ALBUMSCRAPER_SETTINGS, 10004, SettingLevel::Basic); //"settings"
    if (subsetting)
      subsetting->SetParent(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER);
  }
  if (!m_showSingleScraper || m_singleScraperType == CONTENT_ARTISTS)
  {
    AddButton(group, CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, 38335, SettingLevel::Basic); //Provider for artist information
    subsetting = AddButton(group, SETTING_ARTISTSCRAPER_SETTINGS, 10004, SettingLevel::Basic); //"settings"
    if (subsetting)
      subsetting->SetParent(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER);

    AddButton(group, CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER, 38336, SettingLevel::Basic);
  }
}

void CGUIDialogInfoProviderSettings::SetLabel2(const std::string &settingid, const std::string &label)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), label);
}

void CGUIDialogInfoProviderSettings::ToggleState(const std::string &settingid, bool enabled)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    if (enabled)
      CONTROL_ENABLE(settingControl->GetID());
    else
      CONTROL_DISABLE(settingControl->GetID());
  }
}

void CGUIDialogInfoProviderSettings::SetFocus(const std::string &settingid)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_FOCUS(settingControl->GetID(), 0);
}

void CGUIDialogInfoProviderSettings::ResetDefaults()
{
  m_showSingleScraper = false;
  m_singleScraperType = CONTENT_NONE;
  m_applyToItems = INFOPROVIDER_THISITEM;
}

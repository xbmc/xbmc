/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogLibExportSettings.h"

#include "ServiceBroker.h"
#include "Util.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/SettingUtils.h"
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
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

CGUIDialogLibExportSettings::CGUIDialogLibExportSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_LIBEXPORT_SETTINGS, "DialogSettings.xml")
{ }

bool CGUIDialogLibExportSettings::Show(CLibExportSettings& settings)
{
  CGUIDialogLibExportSettings *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogLibExportSettings>(WINDOW_DIALOG_LIBEXPORT_SETTINGS);
  if (!dialog)
    return false;

  // Get current export settings from service broker
  const std::shared_ptr<CSettings> pSettings = CServiceBroker::GetSettingsComponent()->GetSettings();
  dialog->m_settings.SetExportType(pSettings->GetInt(CSettings::SETTING_MUSICLIBRARY_EXPORT_FILETYPE));
  dialog->m_settings.m_strPath = pSettings->GetString(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER);
  dialog->m_settings.SetItemsToExport(pSettings->GetInt(CSettings::SETTING_MUSICLIBRARY_EXPORT_ITEMS));
  dialog->m_settings.m_unscraped = pSettings->GetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED);
  dialog->m_settings.m_artwork = pSettings->GetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_ARTWORK);
  dialog->m_settings.m_skipnfo = pSettings->GetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO);
  dialog->m_settings.m_overwrite = pSettings->GetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE);

  // Ensure NFO or art output enabled when albums exported (adjust old saved settings)
  if (dialog->m_settings.IsItemExported(ELIBEXPORT_ALBUMS) && (dialog->m_settings.m_skipnfo && !dialog->m_settings.m_artwork))
    dialog->m_settings.m_skipnfo = false;

  dialog->m_destinationChecked = false;
  dialog->Open();

  bool confirmed = dialog->IsConfirmed();
  if (confirmed)
  {
    // Return the new settings (saved by service broker but avoids re-reading)
    settings = dialog->m_settings;
  }
  return confirmed;
}

void CGUIDialogLibExportSettings::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
}

void CGUIDialogLibExportSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_FILETYPE)
  {
    m_settings.SetExportType(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
    SetupView();
    SetFocus(CSettings::SETTING_MUSICLIBRARY_EXPORT_FILETYPE);
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER)
  {
    m_settings.m_strPath = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
    UpdateButtons();
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE)
    m_settings.m_overwrite = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_ITEMS)
  {
    m_settings.SetItemsToExport(GetExportItemsFromSetting(setting));
    if (m_settings.IsItemExported(ELIBEXPORT_ALBUMS) && (m_settings.m_skipnfo && !m_settings.m_artwork))
    {
      m_settings.m_skipnfo = false;
      m_settingNFO->SetValue(true);
      UpdateToggles();
    }
    UpdateDescription();
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_ARTWORK)
  {
    m_settings.m_artwork = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    if (m_settings.IsItemExported(ELIBEXPORT_ALBUMS) && (m_settings.m_skipnfo && !m_settings.m_artwork))
    {
      m_settings.m_skipnfo = false;
      m_settingNFO->SetValue(true);
    }
    UpdateToggles();
  }
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED)
    m_settings.m_unscraped = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO)
  {
    m_settings.m_skipnfo = !std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    if (m_settings.IsItemExported(ELIBEXPORT_ALBUMS) && (m_settings.m_skipnfo && !m_settings.m_artwork))
    {
      m_settings.m_artwork = true;
      m_settingArt->SetValue(true);
    }
    UpdateToggles();
  }
}

void CGUIDialogLibExportSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER && !m_settings.IsToLibFolders() &&
      !m_settings.IsArtistFoldersOnly())
  {
    VECSOURCES shares;
    CServiceBroker::GetMediaManager().GetLocalDrives(shares);
    CServiceBroker::GetMediaManager().GetNetworkLocations(shares);
    CServiceBroker::GetMediaManager().GetRemovableDrives(shares);
    std::string strDirectory = m_settings.m_strPath;
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

    if (CGUIDialogFileBrowser::ShowAndGetDirectory(shares, g_localizeStrings.Get(661), strDirectory, true))
    {
      if (!strDirectory.empty())
      {
        m_destinationChecked = true;
        m_settings.m_strPath = strDirectory;
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, strDirectory);
        SetFocus(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER);
      }
    }
    UpdateButtons();
  }
}

bool CGUIDialogLibExportSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_SETTINGS_OKAY_BUTTON)
      {
        OnOK();
        return true;
      }
    }
    break;
  }
  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogLibExportSettings::OnOK()
{
  // Validate destination folder
  if (m_settings.IsToLibFolders() || m_settings.IsArtistFoldersOnly())
  {
    // Check artist info folder setting
    std::string path = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
    if (path.empty())
    {
      //"Unable to export to library folders as the system artist information folder setting is empty"
      //Settings (YES) button takes user to enter the artist info folder setting
      if (HELPERS::ShowYesNoDialogText(20223, 38317, 186, 10004) == DialogResponse::CHOICE_YES)
      {
        m_confirmed = false;
        Close();
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SETTINGS_MEDIA, CSettings::SETTING_MUSICLIBRARY_ARTISTSFOLDER);
      }
      return;
    }
  }
  else if (!m_destinationChecked)
  {
    // ELIBEXPORT_SINGLEFILE or LIBEXPORT_SEPARATEFILES
    // Check that destination folder exists
    if (!XFILE::CDirectory::Exists(m_settings.m_strPath))
    {
      HELPERS::ShowOKDialogText(CVariant{ 38300 }, CVariant{ 38318 });
      return;
    }
  }
  m_confirmed = true;
  Save();
  Close();
}

bool CGUIDialogLibExportSettings::Save()
{
  CLog::Log(LOGINFO, "CGUIDialogMusicExportSettings: Save() called");
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  settings->SetInt(CSettings::SETTING_MUSICLIBRARY_EXPORT_FILETYPE, m_settings.GetExportType());
  settings->SetString(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, m_settings.m_strPath);
  settings->SetInt(CSettings::SETTING_MUSICLIBRARY_EXPORT_ITEMS, m_settings.GetItemsToExport());
  settings->SetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED, m_settings.m_unscraped);
  settings->SetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE, m_settings.m_overwrite);
  settings->SetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_ARTWORK, m_settings.m_artwork);
  settings->SetBool(CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO, m_settings.m_skipnfo);
  settings->Save();

  return true;
}

void CGUIDialogLibExportSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(38300);

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 38319);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);

  UpdateButtons();
  UpdateToggles();
  UpdateDescription();
}

void CGUIDialogLibExportSettings::UpdateButtons()
{
  // Enable Export button when destination folder has a path (but may not exist)
  bool enableExport(true);
  if (m_settings.IsSingleFile() ||
      m_settings.IsSeparateFiles())
    enableExport = !m_settings.m_strPath.empty();

  CONTROL_ENABLE_ON_CONDITION(CONTROL_SETTINGS_OKAY_BUTTON, enableExport);
  if (!enableExport)
    SetFocus(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER);
}

void CGUIDialogLibExportSettings::UpdateToggles()
{
  if (m_settings.IsSeparateFiles())
    ToggleState(CSettings::SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED, !m_settings.m_skipnfo);

  if (!m_settings.IsItemExported(ELIBEXPORT_ALBUMS) && m_settings.m_skipnfo && !m_settings.m_artwork)
  {
    //"Output information to NFO files (currently exporting artist folders only)"
    SetLabel(CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO, g_localizeStrings.Get(38310));
    ToggleState(CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE, false);
  }
  else
  {
    //"Output information to NFO files"
    SetLabel(CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO, g_localizeStrings.Get(38309));
    ToggleState(CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE, true);
  }
}

void CGUIDialogLibExportSettings::UpdateDescription()
{
  if (m_settings.IsToLibFolders())
  {
    // Destination button is description of what to library means
    SetLabel(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, "");
    if (m_settings.IsItemExported(ELIBEXPORT_ALBUMS))
      if (m_settings.IsArtists())
        //"Artists exported to Artist Information Folder and albums to music folders"
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, g_localizeStrings.Get(38322));
      else
        //"Albums exported to music folders"
        SetLabel2(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, g_localizeStrings.Get(38323));
    else
      // "Artists exported to Artist Information Folder"
      SetLabel2(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, g_localizeStrings.Get(38324));
  }
  else if (m_settings.IsArtistFoldersOnly())
  {
    // Destination button is description of what artist folders means
    SetLabel(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, "");
    //"Artists folders created in Artist Information Folder"
    SetLabel2(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, g_localizeStrings.Get(38325));
  }
  else
  {
    SetLabel2(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, m_settings.m_strPath);
    SetLabel(CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, g_localizeStrings.Get(38305));
  }
}

void CGUIDialogLibExportSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  std::shared_ptr<CSettingCategory> category = AddCategory("exportsettings", -1);
  if (!category)
  {
    CLog::Log(LOGERROR, "CGUIDialogLibExportSettings: unable to setup settings");
    return;
  }

  std::shared_ptr<CSettingGroup> groupDetails = AddGroup(category);
  if (!groupDetails)
  {
    CLog::Log(LOGERROR, "CGUIDialogLibExportSettings: unable to setup settings");
    return;
  }

  TranslatableIntegerSettingOptions entries;

  entries.emplace_back(38301, ELIBEXPORT_SINGLEFILE);
  entries.emplace_back(38303, ELIBEXPORT_TOLIBRARYFOLDER);
  entries.emplace_back(38302, ELIBEXPORT_SEPARATEFILES);
  entries.emplace_back(38321, ELIBEXPORT_ARTISTFOLDERS);
  AddList(groupDetails, CSettings::SETTING_MUSICLIBRARY_EXPORT_FILETYPE, 38304, SettingLevel::Basic, m_settings.GetExportType(), entries, 38304); // "Choose kind of export output"
  AddButton(groupDetails, CSettings::SETTING_MUSICLIBRARY_EXPORT_FOLDER, 38305, SettingLevel::Basic);

  entries.clear();
  if (!m_settings.IsArtistFoldersOnly())
    entries.emplace_back(132, ELIBEXPORT_ALBUMS); //ablums
  if (m_settings.IsSingleFile())
    entries.emplace_back(134, ELIBEXPORT_SONGS); //songs
  entries.emplace_back(38043, ELIBEXPORT_ALBUMARTISTS); //album artists
  entries.emplace_back(38312, ELIBEXPORT_SONGARTISTS); //song artists
  entries.emplace_back(38313, ELIBEXPORT_OTHERARTISTS); //other artists

  std::vector<int> items;
  if (m_settings.IsArtistFoldersOnly())
  {
    // Only artists, not albums, at least album artists
    items = m_settings.GetLimitedItems(ELIBEXPORT_ALBUMARTISTS + ELIBEXPORT_SONGARTISTS + ELIBEXPORT_OTHERARTISTS);
    if (items.size() == 0)
      items.emplace_back(ELIBEXPORT_ALBUMARTISTS);
  }
  else if (!m_settings.IsSingleFile())
  {
    // No songs unless single file export, at least album artists
    items = m_settings.GetLimitedItems(ELIBEXPORT_ALBUMS + ELIBEXPORT_ALBUMARTISTS + ELIBEXPORT_SONGARTISTS + ELIBEXPORT_OTHERARTISTS);
    if (items.size() == 0)
      items.emplace_back(ELIBEXPORT_ALBUMARTISTS);
  }
  else
   items = m_settings.GetExportItems();

  AddList(groupDetails, CSettings::SETTING_MUSICLIBRARY_EXPORT_ITEMS, 38306, SettingLevel::Basic, items, entries, 133, 1);

  if (m_settings.IsToLibFolders() || m_settings.IsSeparateFiles())
  {
    m_settingNFO = AddToggle(groupDetails, CSettings::SETTING_MUSICLIBRARY_EXPORT_SKIPNFO, 38309, SettingLevel::Basic, !m_settings.m_skipnfo);
    if (m_settings.IsSeparateFiles())
      AddToggle(groupDetails, CSettings::SETTING_MUSICLIBRARY_EXPORT_UNSCRAPED, 38308, SettingLevel::Basic, m_settings.m_unscraped);
    m_settingArt = AddToggle(groupDetails, CSettings::SETTING_MUSICLIBRARY_EXPORT_ARTWORK, 38307, SettingLevel::Basic, m_settings.m_artwork);
    AddToggle(groupDetails, CSettings::SETTING_MUSICLIBRARY_EXPORT_OVERWRITE, 38311, SettingLevel::Basic, m_settings.m_overwrite);
  }
}

void CGUIDialogLibExportSettings::SetLabel2(const std::string &settingid, const std::string &label)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != nullptr && settingControl->GetControl() != nullptr)
    SET_CONTROL_LABEL2(settingControl->GetID(), label);
}

void CGUIDialogLibExportSettings::SetLabel(const std::string &settingid, const std::string &label)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != nullptr && settingControl->GetControl() != nullptr)
    SetControlLabel(settingControl->GetID(), label);
}

void CGUIDialogLibExportSettings::ToggleState(const std::string & settingid, bool enabled)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != nullptr && settingControl->GetControl() != nullptr)
  {
    if (enabled)
      CONTROL_ENABLE(settingControl->GetID());
    else
      CONTROL_DISABLE(settingControl->GetID());
  }
}

void CGUIDialogLibExportSettings::SetFocus(const std::string &settingid)
{
  BaseSettingControlPtr settingControl = GetSettingControl(settingid);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_FOCUS(settingControl->GetID(), 0);
}

int CGUIDialogLibExportSettings::GetExportItemsFromSetting(const SettingConstPtr& setting)
{
  std::shared_ptr<const CSettingList> settingList = std::static_pointer_cast<const CSettingList>(setting);
  if (settingList->GetElementType() != SettingType::Integer)
  {
    CLog::Log(LOGERROR, "CGUIDialogLibExportSettings::{} - wrong items element type", __FUNCTION__);
    return 0;
  }
  int exportitems = 0;
  std::vector<CVariant> list = CSettingUtils::GetList(settingList);
  for (const auto &value : list)
  {
    if (!value.isInteger())
    {
      CLog::Log(LOGERROR, "CGUIDialogLibExportSettings::{} - wrong items value type", __FUNCTION__);
      return 0;
    }
    exportitems += static_cast<int>(value.asInteger());
  }
  return exportitems;
}

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationSkinHandling.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIInfoManager.h"
#include "GUILargeTextureManager.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "addons/AddonManager.h"
#include "addons/AddonVersion.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonType.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogButtonMenu.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogSubMenu.h"
#include "filesystem/Directory.h"
#include "filesystem/DirectoryCache.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIColorManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIFontManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"
#include "video/dialogs/GUIDialogFullScreenInfo.h"

using namespace KODI::MESSAGING;

CApplicationSkinHandling::CApplicationSkinHandling(IMsgTargetCallback* msgCb,
                                                   IWindowManagerCallback* wCb,
                                                   bool& bInitializing)
  : m_msgCb(msgCb), m_wCb(wCb), m_bInitializing(bInitializing)
{
}

bool CApplicationSkinHandling::LoadSkin(const std::string& skinID)
{
  std::shared_ptr<ADDON::CSkinInfo> skin;
  {
    ADDON::AddonPtr addon;
    if (!CServiceBroker::GetAddonMgr().GetAddon(skinID, addon, ADDON::AddonType::SKIN,
                                                ADDON::OnlyEnabled::CHOICE_YES))
      return false;
    skin = std::static_pointer_cast<ADDON::CSkinInfo>(addon);
  }

  // store player and rendering state
  bool bPreviousPlayingState = false;

  enum class RENDERING_STATE
  {
    NONE,
    VIDEO,
    GAME,
  } previousRenderingState = RENDERING_STATE::NONE;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer && appPlayer->IsPlayingVideo())
  {
    bPreviousPlayingState = !appPlayer->IsPausedPlayback();
    if (bPreviousPlayingState)
      appPlayer->Pause();
    appPlayer->FlushRenderer();
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
      previousRenderingState = RENDERING_STATE::VIDEO;
    }
    else if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() ==
             WINDOW_FULLSCREEN_GAME)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
      previousRenderingState = RENDERING_STATE::GAME;
    }
  }

  std::unique_lock<CCriticalSection> lock(CServiceBroker::GetWinSystem()->GetGfxContext());

  // store current active window with its focused control
  int currentWindowID = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  int currentFocusedControlID = -1;
  if (currentWindowID != WINDOW_INVALID)
  {
    CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(currentWindowID);
    if (pWindow)
      currentFocusedControlID = pWindow->GetFocusedControlID();
  }

  UnloadSkin();

  skin->Start();

  // migrate any skin-specific settings that are still stored in guisettings.xml
  CSkinSettings::GetInstance().MigrateSettings(skin);

  // check if the skin has been properly loaded and if it has a Home.xml
  if (!skin->HasSkinFile("Home.xml"))
  {
    CLog::Log(LOGERROR, "failed to load requested skin '{}'", skin->ID());
    return false;
  }

  CLog::Log(LOGINFO, "  load skin from: {} (version: {})", skin->Path(),
            skin->Version().asString());
  g_SkinInfo = skin;

  CLog::Log(LOGINFO, "  load fonts for skin...");
  CServiceBroker::GetWinSystem()->GetGfxContext().SetMediaDir(skin->Path());
  g_directoryCache.ClearSubPaths(skin->Path());

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  CServiceBroker::GetGUI()->GetColorManager().Load(
      settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS));

  g_SkinInfo->LoadIncludes();

  g_fontManager.LoadFonts(settings->GetString(CSettings::SETTING_LOOKANDFEEL_FONT));

  // load in the skin strings
  std::string langPath = URIUtils::AddFileToFolder(skin->Path(), "language");
  URIUtils::AddSlashAtEnd(langPath);

  g_localizeStrings.LoadSkinStrings(langPath,
                                    settings->GetString(CSettings::SETTING_LOCALE_LANGUAGE));
  g_SkinInfo->LoadTimers();

  const auto start = std::chrono::steady_clock::now();

  CLog::Log(LOGINFO, "  load new skin...");

  // Load custom windows
  LoadCustomWindows();

  const auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> duration = end - start;

  CLog::Log(LOGDEBUG, "Load Skin XML: {:.2f} ms", duration.count());

  CLog::Log(LOGINFO, "  initialize new skin...");
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(m_msgCb);
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(&CServiceBroker::GetPlaylistPlayer());
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(&g_fontManager);
  CServiceBroker::GetGUI()->GetWindowManager().AddMsgTarget(
      &CServiceBroker::GetGUI()->GetStereoscopicsManager());
  CServiceBroker::GetGUI()->GetWindowManager().SetCallback(*m_wCb);

  //@todo should be done by GUIComponents
  CServiceBroker::GetGUI()->GetWindowManager().Initialize();
  CServiceBroker::GetGUI()->GetAudioManager().Enable(true);
  CServiceBroker::GetGUI()->GetAudioManager().Load();
  CServiceBroker::GetTextureCache()->Initialize();

  if (g_SkinInfo->HasSkinFile("DialogFullScreenInfo.xml"))
    CServiceBroker::GetGUI()->GetWindowManager().Add(new CGUIDialogFullScreenInfo);

  CLog::Log(LOGINFO, "  skin loaded...");

  // leave the graphics lock
  lock.unlock();

  // restore active window
  if (currentWindowID != WINDOW_INVALID)
  {
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(currentWindowID);
    if (currentFocusedControlID != -1)
    {
      CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(currentWindowID);
      if (pWindow && pWindow->HasSaveLastControl())
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, currentWindowID, currentFocusedControlID, 0);
        pWindow->OnMessage(msg);
      }
    }
  }

  // restore player and rendering state
  if (appPlayer && appPlayer->IsPlayingVideo())
  {
    if (bPreviousPlayingState)
      appPlayer->Pause();

    switch (previousRenderingState)
    {
      case RENDERING_STATE::VIDEO:
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
        break;
      case RENDERING_STATE::GAME:
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_FULLSCREEN_GAME);
        break;
      default:
        break;
    }
  }

  return true;
}

void CApplicationSkinHandling::UnloadSkin()
{
  if (g_SkinInfo != nullptr && m_saveSkinOnUnloading)
    g_SkinInfo->SaveSettings();
  else if (!m_saveSkinOnUnloading)
    m_saveSkinOnUnloading = true;

  if (g_SkinInfo)
    g_SkinInfo->Unload();

  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
  {
    gui->GetAudioManager().Enable(false);

    gui->GetWindowManager().DeInitialize();
    CServiceBroker::GetTextureCache()->Deinitialize();

    // remove the skin-dependent window
    gui->GetWindowManager().Delete(WINDOW_DIALOG_FULLSCREEN_INFO);

    gui->GetTextureManager().Cleanup();
    gui->GetLargeTextureManager().CleanupUnusedImages(true);

    g_fontManager.Clear();

    gui->GetColorManager().Clear();

    gui->GetInfoManager().Clear();
  }

  //  The g_SkinInfo shared_ptr ought to be reset here
  // but there are too many places it's used without checking for nullptr
  // and as a result a race condition on exit can cause a crash.
  CLog::Log(LOGINFO, "Unloaded skin");
}

bool CApplicationSkinHandling::LoadCustomWindows()
{
  // Start from wherever home.xml is
  std::vector<std::string> vecSkinPath;
  g_SkinInfo->GetSkinPaths(vecSkinPath);

  for (const auto& skinPath : vecSkinPath)
  {
    CLog::Log(LOGINFO, "Loading custom window XMLs from skin path {}", skinPath);

    CFileItemList items;
    if (XFILE::CDirectory::GetDirectory(skinPath, items, ".xml", XFILE::DIR_FLAG_NO_FILE_DIRS))
    {
      for (const auto& item : items)
      {
        if (item->m_bIsFolder)
          continue;

        std::string skinFile = URIUtils::GetFileName(item->GetPath());
        if (StringUtils::StartsWithNoCase(skinFile, "custom"))
        {
          CXBMCTinyXML xmlDoc;
          if (!xmlDoc.LoadFile(item->GetPath()))
          {
            CLog::Log(LOGERROR, "Unable to load custom window XML {}. Line {}\n{}", item->GetPath(),
                      xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
            continue;
          }

          // Root element should be <window>
          TiXmlElement* pRootElement = xmlDoc.RootElement();
          std::string strValue = pRootElement->Value();
          if (!StringUtils::EqualsNoCase(strValue, "window"))
          {
            CLog::Log(LOGERROR, "No <window> root element found for custom window in {}", skinFile);
            continue;
          }

          int id = WINDOW_INVALID;

          // Read the type attribute or element to get the window type to create
          // If no type is specified, create a CGUIWindow as default
          std::string strType;
          if (pRootElement->Attribute("type"))
            strType = pRootElement->Attribute("type");
          else
          {
            const TiXmlNode* pType = pRootElement->FirstChild("type");
            if (pType && pType->FirstChild())
              strType = pType->FirstChild()->Value();
          }

          // Read the id attribute or element to get the window id
          if (!pRootElement->Attribute("id", &id))
          {
            const TiXmlNode* pType = pRootElement->FirstChild("id");
            if (pType && pType->FirstChild())
              id = atol(pType->FirstChild()->Value());
          }

          int windowId = id + WINDOW_HOME;
          if (id == WINDOW_INVALID ||
              CServiceBroker::GetGUI()->GetWindowManager().GetWindow(windowId))
          {
            // No id specified or id already in use
            CLog::Log(LOGERROR, "No id specified or id already in use for custom window in {}",
                      skinFile);
            continue;
          }

          CGUIWindow* pWindow = nullptr;
          bool hasVisibleCondition = false;

          if (StringUtils::EqualsNoCase(strType, "dialog"))
          {
            DialogModalityType modality = DialogModalityType::MODAL;
            hasVisibleCondition = pRootElement->FirstChildElement("visible") != nullptr;
            // By default dialogs that have visible conditions are considered modeless unless explicitly
            // set to "modal" by the skinner using the "modality" attribute in the root XML element of the window
            if (hasVisibleCondition &&
                (!pRootElement->Attribute("modality") ||
                 !StringUtils::EqualsNoCase(pRootElement->Attribute("modality"), "modal")))
              modality = DialogModalityType::MODELESS;

            pWindow = new CGUIDialog(windowId, skinFile, modality);
          }
          else if (StringUtils::EqualsNoCase(strType, "submenu"))
          {
            pWindow = new CGUIDialogSubMenu(windowId, skinFile);
          }
          else if (StringUtils::EqualsNoCase(strType, "buttonmenu"))
          {
            pWindow = new CGUIDialogButtonMenu(windowId, skinFile);
          }
          else
          {
            pWindow = new CGUIWindow(windowId, skinFile);
          }

          if (!pWindow)
          {
            CLog::Log(LOGERROR, "Failed to create custom window from {}", skinFile);
            continue;
          }

          pWindow->SetCustom(true);

          // Determining whether our custom dialog is modeless (visible condition is present)
          // will be done on load. Therefore we need to initialize the custom dialog on gui init.
          pWindow->SetLoadType(hasVisibleCondition ? CGUIWindow::LOAD_ON_GUI_INIT
                                                   : CGUIWindow::KEEP_IN_MEMORY);

          CServiceBroker::GetGUI()->GetWindowManager().AddCustomWindow(pWindow);
        }
      }
    }
  }
  return true;
}

void CApplicationSkinHandling::ReloadSkin(bool confirm)
{
  if (!g_SkinInfo || m_bInitializing)
    return; // Don't allow reload before skin is loaded by system

  std::string oldSkin = g_SkinInfo->ID();

  CGUIMessage msg(GUI_MSG_LOAD_SKIN, -1,
                  CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow());
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  std::string newSkin = settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
  if (LoadSkin(newSkin))
  {
    /* The Reset() or SetString() below will cause recursion, so the m_confirmSkinChange boolean is set so as to not prompt the
       user as to whether they want to keep the current skin. */
    if (confirm && m_confirmSkinChange)
    {
      if (HELPERS::ShowYesNoDialogText(CVariant{13123}, CVariant{13111}, CVariant{""}, CVariant{""},
                                       10000) != HELPERS::DialogResponse::CHOICE_YES)
      {
        m_confirmSkinChange = false;
        settings->SetString(CSettings::SETTING_LOOKANDFEEL_SKIN, oldSkin);
      }
      else
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_STARTUP_ANIM);
    }
  }
  else
  {
    // skin failed to load - we revert to the default only if we didn't fail loading the default
    auto setting = settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKIN);
    if (!setting)
    {
      CLog::Log(LOGFATAL, "Failed to load setting for: {}", CSettings::SETTING_LOOKANDFEEL_SKIN);
      return;
    }

    std::string defaultSkin = std::static_pointer_cast<CSettingString>(setting)->GetDefault();
    if (newSkin != defaultSkin)
    {
      m_confirmSkinChange = false;
      setting->Reset();
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(24102),
                                            g_localizeStrings.Get(24103));
    }
  }
  m_confirmSkinChange = true;
}

bool CApplicationSkinHandling::OnSettingChanged(const CSetting& setting)
{
  const std::string& settingId = setting.GetId();

  if (settingId == CSettings::SETTING_LOOKANDFEEL_SKIN ||
      settingId == CSettings::SETTING_LOOKANDFEEL_FONT ||
      settingId == CSettings::SETTING_LOOKANDFEEL_SKINTHEME ||
      settingId == CSettings::SETTING_LOOKANDFEEL_SKINCOLORS)
  {
    // check if we should ignore this change event due to changing skins in which case we have to
    // change several settings and each one of them could lead to a complete skin reload which would
    // result in multiple skin reloads. Therefore we manually specify to ignore specific settings
    // which are going to be changed.
    if (m_ignoreSkinSettingChanges)
      return true;

    // if the skin changes and the current color/theme/font is not the default one, reset
    // the it to the default value
    if (settingId == CSettings::SETTING_LOOKANDFEEL_SKIN)
    {
      const std::shared_ptr<CSettings> settings =
          CServiceBroker::GetSettingsComponent()->GetSettings();
      SettingPtr skinRelatedSetting =
          settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS);
      if (!skinRelatedSetting->IsDefault())
      {
        m_ignoreSkinSettingChanges = true;
        skinRelatedSetting->Reset();
      }

      skinRelatedSetting = settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
      if (!skinRelatedSetting->IsDefault())
      {
        m_ignoreSkinSettingChanges = true;
        skinRelatedSetting->Reset();
      }

      skinRelatedSetting = settings->GetSetting(CSettings::SETTING_LOOKANDFEEL_FONT);
      if (!skinRelatedSetting->IsDefault())
      {
        m_ignoreSkinSettingChanges = true;
        skinRelatedSetting->Reset();
      }
    }
    else if (settingId == CSettings::SETTING_LOOKANDFEEL_SKINTHEME)
    {
      std::shared_ptr<CSettingString> skinColorsSetting = std::static_pointer_cast<CSettingString>(
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
              CSettings::SETTING_LOOKANDFEEL_SKINCOLORS));
      std::shared_ptr<CSettingString> skinFontSetting = std::static_pointer_cast<CSettingString>(
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
              CSettings::SETTING_LOOKANDFEEL_FONT));
      m_ignoreSkinSettingChanges = true;

      // we also need to adjust the skin color theme and fontset
      std::string theme = static_cast<const CSettingString&>(setting).GetValue();
      if (setting.IsDefault() || StringUtils::EqualsNoCase(theme, "Textures.xbt"))
      {
        skinColorsSetting->Reset();
        skinFontSetting->Reset();
      }
      else
      {
        URIUtils::RemoveExtension(theme);
        skinColorsSetting->SetValue(theme);
        skinFontSetting->SetValue(theme);
      }
    }

    m_ignoreSkinSettingChanges = false;

    if (g_SkinInfo)
    {
      // now we can finally reload skins
      std::string builtin("ReloadSkin");
      if (settingId == CSettings::SETTING_LOOKANDFEEL_SKIN && m_confirmSkinChange)
        builtin += "(confirm)";
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, builtin);
    }
  }
  else if (settingId == CSettings::SETTING_LOOKANDFEEL_SKINZOOM)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WINDOW_RESIZE);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else
    return false;

  return true;
}

void CApplicationSkinHandling::ProcessSkin() const
{
  if (g_SkinInfo != nullptr)
    g_SkinInfo->ProcessTimers();
}

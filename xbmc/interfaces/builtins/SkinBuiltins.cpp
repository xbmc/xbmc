/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SkinBuiltins.h"

#include "MediaSource.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIWindowAddonBrowser.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationSkinHandling.h"
#include "dialogs/GUIDialogColorPicker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/SkinSettings.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace ADDON;

/*! \brief Reload current skin.
 *  \param params The parameters.
 *  \details params[0] = "confirm" to show a confirmation dialog (optional).
 */
static int ReloadSkin(const std::vector<std::string>& params)
{
  //  Reload the skin
  auto& components = CServiceBroker::GetAppComponents();
  const auto appSkin = components.GetComponent<CApplicationSkinHandling>();
  appSkin->ReloadSkin(!params.empty() && StringUtils::EqualsNoCase(params[0], "confirm"));

  return 0;
}

/*! \brief Unload current skin.
 *  \param params (ignored)
 */
static int UnloadSkin(const std::vector<std::string>& params)
{
  auto& components = CServiceBroker::GetAppComponents();
  const auto appSkin = components.GetComponent<CApplicationSkinHandling>();
  appSkin->UnloadSkin();

  return 0;
}

/*! \brief Toggle a skin bool setting.
 *  \param params The parameters.
 *  \details params[0] = Skin setting to toggle.
 */
static int ToggleSetting(const std::vector<std::string>& params)
{
  int setting = CSkinSettings::GetInstance().TranslateBool(params[0]);
  CSkinSettings::GetInstance().SetBool(setting, !CSkinSettings::GetInstance().GetBool(setting));
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return 0;
}

/*! \brief Set an add-on type skin setting.
 *  \param params The parameters.
 *  \details params[0] = Skin setting to store result in.
 *           params[1,...] = Add-on types to allow selecting.
 */
static int SetAddon(const std::vector<std::string>& params)
{
  int string = CSkinSettings::GetInstance().TranslateString(params[0]);
  std::vector<ADDON::AddonType> types;
  for (unsigned int i = 1 ; i < params.size() ; i++)
  {
    ADDON::AddonType type = CAddonInfo::TranslateType(params[i]);
    if (type != AddonType::UNKNOWN)
      types.push_back(type);
  }
  std::string result;
  if (!types.empty() && CGUIWindowAddonBrowser::SelectAddonID(types, result, true) == 1)
  {
    CSkinSettings::GetInstance().SetString(string, result);
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }

  return 0;
}

/*! \brief Select and set a skin bool setting.
 *  \param params The parameters.
 *  \details params[0] = Number of a localized string to display as a header in a select dialog
 *  \details params[1,...] = one or more number|skinbool-setting pairs where number is index of a localized string used as label
 *  \details and skinbool-setting is a string of the skinbool setting name. The pairs are added to the select dialog list.
 *  \details If the users confirms a (single) selection label in the select dialog, the paired skinbool is set to true and all others
 *  \details in the list are set to false. Multi-select is not available.
 */
static int SelectBool(const std::vector<std::string>& params)
{
  std::vector<std::pair<std::string, std::string>> settings;

  CGUIDialogSelect* pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  pDlgSelect->Reset();
  pDlgSelect->SetHeading(CVariant{g_localizeStrings.Get(atoi(params[0].c_str()))});

  for (unsigned int i = 1 ; i < params.size() ; i++)
  {
    if (params[i].find('|') != std::string::npos)
    {
      std::vector<std::string> values = StringUtils::Split(params[i], '|');
      std::string label = g_localizeStrings.Get(atoi(values[0].c_str()));
      settings.emplace_back(label, values[1].c_str());
      pDlgSelect->Add(label);
    }
  }

  pDlgSelect->Open();

  if(pDlgSelect->IsConfirmed())
  {
    unsigned int iItem = pDlgSelect->GetSelectedItem();

    for (unsigned int i = 0 ; i < settings.size() ; i++)
    {
      std::string item = settings[i].second;
      int setting = CSkinSettings::GetInstance().TranslateBool(item);
      if (i == iItem)
        CSkinSettings::GetInstance().SetBool(setting, true);
      else
        CSkinSettings::GetInstance().SetBool(setting, false);
    }
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
  }

  return 0;
}

/*! \brief Set a skin bool setting.
 *  \param params The parameters.
 *  \details params[0] = Name of skin setting.
 *           params[1] = Value to set ("false", or "true") (optional).
 */
static int SetBool(const std::vector<std::string>& params)
{
  if (params.size() > 1)
  {
    int string = CSkinSettings::GetInstance().TranslateBool(params[0]);
    CSkinSettings::GetInstance().SetBool(string, StringUtils::EqualsNoCase(params[1], "true"));
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    return 0;
  }
  // default is to set it to true
  int setting = CSkinSettings::GetInstance().TranslateBool(params[0]);
  CSkinSettings::GetInstance().SetBool(setting, true);
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return 0;
}

/*! \brief Set a numeric skin setting.
 *  \param params The parameters.
 *  \details params[0] = Name of skin setting.
 */
static int SetNumeric(const std::vector<std::string>& params)
{
  int string = CSkinSettings::GetInstance().TranslateString(params[0]);
  std::string value = CSkinSettings::GetInstance().GetString(string);
  if (CGUIDialogNumeric::ShowAndGetNumber(value, g_localizeStrings.Get(611)))
    CSkinSettings::GetInstance().SetString(string, value);

  return 0;
}

/*! \brief Set a path skin setting.
 *  \param params The parameters.
 *  \details params[0] = Name of skin setting.
 *           params[1] = Extra URL to allow selection from (optional).
 */
static int SetPath(const std::vector<std::string>& params)
{
  int string = CSkinSettings::GetInstance().TranslateString(params[0]);
  std::string value = CSkinSettings::GetInstance().GetString(string);
  VECSOURCES localShares;
  CServiceBroker::GetMediaManager().GetLocalDrives(localShares);
  CServiceBroker::GetMediaManager().GetNetworkLocations(localShares);
  if (params.size() > 1)
  {
    value = params[1];
    URIUtils::AddSlashAtEnd(value);
    bool bIsSource;
    if (CUtil::GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
    {
      CMediaSource share;
      share.strName = g_localizeStrings.Get(13278);
      share.strPath = value;
      localShares.push_back(share);
    }
  }

  if (CGUIDialogFileBrowser::ShowAndGetDirectory(localShares, g_localizeStrings.Get(657), value))
    CSkinSettings::GetInstance().SetString(string, value);

  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return 0;
}

/*! \brief Set a skin file setting.
 *  \param params The parameters.
 *  \details params[0] = Name of skin setting.
 *           params[1] = File mask or add-on type (optional).
 *           params[2] = Extra URL to allow selection from or
 *                       content type if mask is an addon-on type (optional).
 */
static int SetFile(const std::vector<std::string>& params)
{
  int string = CSkinSettings::GetInstance().TranslateString(params[0]);
  std::string value = CSkinSettings::GetInstance().GetString(string);
  VECSOURCES localShares;
  CServiceBroker::GetMediaManager().GetLocalDrives(localShares);

  // Note. can only browse one addon type from here
  // if browsing for addons, required param[1] is addontype string, with optional param[2]
  // as contenttype string see IAddon.h & ADDON::TranslateXX
  std::string strMask = (params.size() > 1) ? params[1] : "";
  StringUtils::ToLower(strMask);
  ADDON::AddonType type;
  if ((type = CAddonInfo::TranslateType(strMask)) != AddonType::UNKNOWN)
  {
    CURL url;
    url.SetProtocol("addons");
    url.SetHostName("enabled");
    url.SetFileName(strMask+"/");
    localShares.clear();
    std::string content = (params.size() > 2) ? params[2] : "";
    StringUtils::ToLower(content);
    url.SetPassword(content);
    std::string strMask;
    if (type == AddonType::SCRIPT)
      strMask = ".py";
    std::string replace;
    if (CGUIDialogFileBrowser::ShowAndGetFile(url.Get(), strMask, CAddonInfo::TranslateType(type, true), replace, true, true, true))
    {
      if (StringUtils::StartsWithNoCase(replace, "addons://"))
        CSkinSettings::GetInstance().SetString(string, URIUtils::GetFileName(replace));
      else
        CSkinSettings::GetInstance().SetString(string, replace);
    }
  }
  else
  {
    if (params.size() > 2)
    {
      value = params[2];
      URIUtils::AddSlashAtEnd(value);
      bool bIsSource;
      if (CUtil::GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
      {
        CMediaSource share;
        share.strName = g_localizeStrings.Get(13278);
        share.strPath = value;
        localShares.push_back(share);
      }
    }
    if (CGUIDialogFileBrowser::ShowAndGetFile(localShares, strMask, g_localizeStrings.Get(1033), value))
      CSkinSettings::GetInstance().SetString(string, value);
  }

  return 0;
}

/*! \brief Set a skin image setting.
 *  \param params The parameters.
 *  \details params[0] = Name of skin setting.
 *           params[1] = Extra URL to allow selection from (optional).
 */
static int SetImage(const std::vector<std::string>& params)
{
  int string = CSkinSettings::GetInstance().TranslateString(params[0]);
  std::string value = CSkinSettings::GetInstance().GetString(string);
  VECSOURCES localShares;
  CServiceBroker::GetMediaManager().GetLocalDrives(localShares);
  if (params.size() > 1)
  {
    value = params[1];
    URIUtils::AddSlashAtEnd(value);
    bool bIsSource;
    if (CUtil::GetMatchingSource(value,localShares,bIsSource) < 0) // path is outside shares - add it as a separate one
    {
      CMediaSource share;
      share.strName = g_localizeStrings.Get(13278);
      share.strPath = value;
      localShares.push_back(share);
    }
  }
  if (CGUIDialogFileBrowser::ShowAndGetImage(localShares, g_localizeStrings.Get(1030), value))
    CSkinSettings::GetInstance().SetString(string, value);

  return 0;
}

/*! \brief Set a skin color setting.
 *  \param params The parameters.
 *  \details params[0] = Name of skin setting.
 *           params[1] = Dialog header text.
 *           params[2] = Hex value of the preselected color (optional).
 *           params[3] = XML file containing color definitions (optional).
 */
static int SetColor(const std::vector<std::string>& params)
{
  int string = CSkinSettings::GetInstance().TranslateString(params[0]);
  std::string value = CSkinSettings::GetInstance().GetString(string);

  if (value.empty() && params.size() > 2)
  {
    value = params[2];
  }

  CGUIDialogColorPicker* pDlgColorPicker =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogColorPicker>(
          WINDOW_DIALOG_COLOR_PICKER);
  pDlgColorPicker->Reset();
  pDlgColorPicker->SetHeading(CVariant{g_localizeStrings.Get(atoi(params[1].c_str()))});

  if (params.size() > 3)
  {
    pDlgColorPicker->LoadColors(params[3]);
  }
  else
  {
    pDlgColorPicker->LoadColors();
  }

  pDlgColorPicker->SetSelectedColor(value);

  pDlgColorPicker->Open();

  if (pDlgColorPicker->IsConfirmed())
  {
    value = pDlgColorPicker->GetSelectedColor();
    CSkinSettings::GetInstance().SetString(string, value);
  }

  return 0;
}

/*! \brief Set a string skin setting.
 *  \param params The parameters.
 *  \details params[0] = Name of skin setting.
 *           params[1] = Value of skin setting (optional).
 */
static int SetString(const std::vector<std::string>& params)
{
  // break the parameter up if necessary
  int string = 0;
  if (params.size() > 1)
  {
    string = CSkinSettings::GetInstance().TranslateString(params[0]);
    CSkinSettings::GetInstance().SetString(string, params[1]);
    CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
    return 0;
  }
  else
    string = CSkinSettings::GetInstance().TranslateString(params[0]);

  std::string value = CSkinSettings::GetInstance().GetString(string);
  if (CGUIKeyboardFactory::ShowAndGetInput(value, CVariant{g_localizeStrings.Get(1029)}, true))
    CSkinSettings::GetInstance().SetString(string, value);

  return 0;
}

/*! \brief Select skin theme.
 *  \param params The parameters.
 *  \details params[0] = 0 or 1 to increase theme, -1 to decrease.
 */
static int SetTheme(const std::vector<std::string>& params)
{
  // enumerate themes
  std::vector<std::string> vecTheme;
  CUtil::GetSkinThemes(vecTheme);

  int iTheme = -1;

  // find current theme
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const std::string strTheme = settings->GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME);
  if (!StringUtils::EqualsNoCase(strTheme, "SKINDEFAULT"))
  {
    for (size_t i=0;i<vecTheme.size();++i)
    {
      std::string strTmpTheme(strTheme);
      URIUtils::RemoveExtension(strTmpTheme);
      if (StringUtils::EqualsNoCase(vecTheme[i], strTmpTheme))
      {
        iTheme=i;
        break;
      }
    }
  }

  int iParam = atoi(params[0].c_str());
  if (iParam == 0 || iParam == 1)
    iTheme++;
  else if (iParam == -1)
    iTheme--;
  if (iTheme > (int)vecTheme.size()-1)
    iTheme = -1;
  if (iTheme < -1)
    iTheme = vecTheme.size()-1;

  std::string strSkinTheme = "SKINDEFAULT";
  if (iTheme != -1 && iTheme < (int)vecTheme.size())
    strSkinTheme = vecTheme[iTheme];

  // Because of the way callbacks are implemented, calling  settings->SetString(...)
  // causes ApplicationSkinHandling::OnSettingChanged(...) to be called.
  // The ApplicationSkinHandling::OnSettingChanged method will do all the work of
  // changing to the new theme, including reloading the skin.
  settings->SetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME, strSkinTheme);

  return 0;
}

/*! \brief Reset a skin setting.
 *  \param params The parameters.
 *  \details params[0] = Name of setting to reset.
 */
static int SkinReset(const std::vector<std::string>& params)
{
  CSkinSettings::GetInstance().Reset(params[0]);
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return 0;
}

/*! \brief Reset all skin settings.
 *  \param params (ignored)
 */
static int SkinResetAll(const std::vector<std::string>& params)
{
  CSkinSettings::GetInstance().Reset();
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

  return 0;
}

/*! \brief Toggle skin debug.
 *  \param params (ignored)
 */
static int SkinDebug(const std::vector<std::string>& params)
{
  g_SkinInfo->ToggleDebug();

  return 0;
}

/*! \brief Starts a given skin timer
 *  \param params The parameters.
 *  \details params[0] = Name of the timer.
 *  \return -1 in case of error, 0 in case of success
 */
static int SkinTimerStart(const std::vector<std::string>& params)
{
  if (params.empty())
  {
    return -1;
  }

  g_SkinInfo->TimerStart(params[0]);
  return 0;
}

/*! \brief Stops a given skin timer
 *  \param params The parameters.
 *  \details params[0] = Name of the timer.
 *  \return -1 in case of error, 0 in case of success
 */
static int SkinTimerStop(const std::vector<std::string>& params)
{
  if (params.empty())
  {
    return -1;
  }

  g_SkinInfo->TimerStop(params[0]);
  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_14 Skin built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`ReloadSkin(reload)`</b>
///     ,
///     Reloads the current skin – useful for Skinners to use after they upload
///     modified skin files (saves power cycling)
///     @param[in] reload                <b>"confirm"</b> to show a confirmation dialog (optional).
///   }
///   \table_row2_l{
///     <b>`UnloadSkin()`</b>
///     ,
///     Unloads the current skin
///   }
///   \table_row2_l{
///     <b>`Skin.Reset(setting)`</b>
///     ,
///     Resets the skin `setting`. If `setting` is a bool setting (i.e. set via
///     `SetBool` or `ToggleSetting`) then the setting is reset to false. If
///     `setting`  is a string (Set via <c>SetString</c>\, <c>SetImage</c> or
///     <c>SetPath</c>) then it is set to empty.
///     @param[in] setting               Name of setting to reset.
///   }
///   \table_row2_l{
///     <b>`Skin.ResetSettings()`</b>
///     ,
///     Resets all the above skin settings to their defaults (toggles all set to
///     false\, strings all set to empty.)
///   }
///   \table_row2_l{
///     <b>`Skin.SetAddon(string\,type)`</b>
///     ,
///     Pops up a select dialog and allows the user to select an add-on of the
///     given type to be used elsewhere in the skin via the info tag
///     `Skin.String(string)`. The most common types are xbmc.addon.video\,
///     xbmc.addon.audio\, xbmc.addon.image and xbmc.addon.executable.
///     @param[in] string[0]             Skin setting to store result in.
///     @param[in] type[1\,...]           Add-on types to allow selecting.
///   }
///   \table_row2_l{
///     <b>`Skin.SelectBool(header\, label1|setting1\, label2|setting2\, ...)`</b>
///     \anchor Skin_SelectBool,
///     Pops up select dialog to select between multiple skin setting options.
///     @param[in] header              Localized string to display as dialog select header.
///     @param[in] pairs               One or more number|skinbool-setting pairs where number is index of a localized string used as label and
///     skinbool-setting is a string of the skinbool setting name. The pairs are added to the select dialog list.
///     @details If the users confirms a (single) selection label in the select dialog\, the paired skinbool is set to true and all others
///     in the list are set to false. Multi-select is not available.</p>
///     <b>Example:</b></p>
///     <code>Skin.SelectBool(424\, 31411|RecentWidget\, 31412|RandomWidget\, 31413|InProgressWidget)</code>
///   }
///   \table_row2_l{
///     <b>`Skin.SetBool(setting[\,value])`</b>
///     \anchor Skin_SetBool,
///     Sets the skin `setting` to true\, for use with the conditional visibility
///     tags containing \link Skin_HasSetting `Skin.HasSetting(setting)`\endlink. The settings are saved
///     per-skin in settings.xml just like all the other Kodi settings.
///     @param[in] setting               Name of skin setting.
///     @param[in] value                 Value to set ("false"\, or "true") (optional).
///   }
///   \table_row2_l{
///     <b>`Skin.SetFile(string\,mask\,folderpath)`</b>
///     ,
///     \" minus quotes. If the folderpath parameter is set the file browser will
///     start in that folder.
///     @param[in] string                Name of skin setting.
///     @param[in] mask                  File mask or add-on type (optional).
///     @param[in] folderpath            Extra URL to allow selection from or.
///                                      content type if mask is an addon-on type (optional).
///   }
///   \table_row2_l{
///     <b>`Skin.SetImage(string[\,url])`</b>
///     ,
///     Pops up a file browser and allows the user to select an image file to be
///     used in an image control elsewhere in the skin via the info tag
///     `Skin.String(string)`. If the value parameter is specified\, then the
///     file browser dialog does not pop up\, and the image path is set directly.
///     The path option allows you to open the file browser in the specified
///     folder.
///     @param[in] string                Name of skin setting.
///     @param[in] url                   Extra URL to allow selection from (optional).
///   }
///   \table_row2_l{
///     <b>`Skin.SetColor(string\,header[\,colorfile\,selectedcolor])`</b>
///     \anchor Builtin_SetColor,
///     Pops up a color selection dialog and allows the user to select a color to be
///     used to define the color of a label control or as a colordiffuse value for a texture
///     elsewhere in the skin via the info tag `Skin.String(string)`.
///     Skinners can optionally set the color that needs to be preselected in the
///     dialog by specifying the hex value of this color.
///     Also optionally\, skinners can include their own color definition file. If not specified\,
///     the default colorfile included with Kodi will be used.
///     @param[in] string                Name of skin setting.
///     @param[in] string                Dialog header text.
///     @param[in] string                Hex value of the color to preselect (optional)\,
///                                      example: FF00FF00.
///     @param[in] string                Filepath of the color definition file (optional).
///     <p><hr>
///     @skinning_v20 **[New builtin]** \link Builtin_SetColor `SetColor(string\,header[\,colorfile\,selectedcolor])`\endlink
///     <p>
///   }
///   \table_row2_l{
///     <b>`Skin.SetNumeric(numeric[\,value])`</b>
///     \anchor Skin_SetNumeric,
///     Pops up a keyboard dialog and allows the user to input a numerical.
///     @param[in] numeric               Name of skin setting.
///     @param[in] value                 Value of skin setting (optional).
///   }
///   \table_row2_l{
///     <b>`Skin.SetPath(string[\,value])`</b>
///     ,
///     Pops up a folder browser and allows the user to select a folder of images
///     to be used in a multi image control else where in the skin via the info
///     tag `Skin.String(string)`. If the value parameter is specified\, then the
///     file browser dialog does not pop up\, and the path is set directly.
///     @param[in] string                Name of skin setting.
///     @param[in] value                 Extra URL to allow selection from (optional).
///   }
///   \table_row2_l{
///     <b>`Skin.SetString(string[\,value])`</b>
///     \anchor Skin_SetString,
///     Pops up a keyboard dialog and allows the user to input a string which can
///     be used in a label control elsewhere in the skin via the info tag
///     \link Skin_StringValue `Skin.String(string)`\endlink. The value of the setting
///     can also be compared to another value using the info bool \link Skin_StringCompare `Skin.String(string\, value)`\endlink.
///     If the value parameter is specified\, then the
///     keyboard dialog does not pop up\, and the string is set directly.
///     @param[in] string                Name of skin setting.
///     @param[in] value                 Value of skin setting (optional).
///   }
///   \table_row2_l{
///     <b>`Skin.Theme(cycle)`</b>
///     \anchor Skin_CycleTheme,
///     Cycles the skin theme. Skin.theme(-1) will go backwards.
///     @param[in] cycle                 0 or 1 to increase theme\, -1 to decrease.
///   }
///   \table_row2_l{
///     <b>`Skin.ToggleDebug`</b>
///     ,
///     Toggles skin debug info on/off
///   }
///   \table_row2_l{
///     <b>`Skin.ToggleSetting(setting)`</b>
///     ,
///     Toggles the skin `setting` for use with conditional visibility tags
///     containing `Skin.HasSetting(setting)`.
///     @param[in] setting               Skin setting to toggle
///  }
///   \table_row2_l{
///     <b>`Skin.TimerStart(timer)`</b>
///     \anchor Builtin_SkinStartTimer,
///     Starts the timer with name `timer`
///     @param[in] timer               The name of the timer
///     <p><hr>
///     @skinning_v20 **[New builtin]** \link Builtin_SkinStartTimer `Skin.TimerStart(timer)`\endlink
///     <p>
///  }
///   \table_row2_l{
///     <b>`Skin.TimerStop(timer)`</b>
///     \anchor Builtin_SkinStopTimer,
///     Stops the timer with name `timer`
///     @param[in] timer               The name of the timer
///     <p><hr>
///     @skinning_v20 **[New builtin]** \link Builtin_SkinStopTimer `Skin.TimerStop(timer)`\endlink
///     <p>
///  }
/// \table_end
///

CBuiltins::CommandMap CSkinBuiltins::GetOperations() const
{
  return {{"reloadskin", {"Reload Kodi's skin", 0, ReloadSkin}},
          {"unloadskin", {"Unload Kodi's skin", 0, UnloadSkin}},
          {"skin.reset", {"Resets a skin setting to default", 1, SkinReset}},
          {"skin.resetsettings", {"Resets all skin settings", 0, SkinResetAll}},
          {"skin.setaddon", {"Prompts and set an addon", 2, SetAddon}},
          {"skin.selectbool", {"Prompts and set a skin setting", 2, SelectBool}},
          {"skin.setbool", {"Sets a skin setting on", 1, SetBool}},
          {"skin.setfile", {"Prompts and sets a file", 1, SetFile}},
          {"skin.setimage", {"Prompts and sets a skin image", 1, SetImage}},
          {"skin.setcolor", {"Prompts and sets a skin color", 1, SetColor}},
          {"skin.setnumeric", {"Prompts and sets numeric input", 1, SetNumeric}},
          {"skin.setpath", {"Prompts and sets a skin path", 1, SetPath}},
          {"skin.setstring", {"Prompts and sets skin string", 1, SetString}},
          {"skin.theme", {"Control skin theme", 1, SetTheme}},
          {"skin.toggledebug", {"Toggle skin debug", 0, SkinDebug}},
          {"skin.togglesetting", {"Toggles a skin setting on or off", 1, ToggleSetting}},
          {"skin.timerstart", {"Starts a given skin timer", 1, SkinTimerStart}},
          {"skin.timerstop", {"Stops a given skin timer", 1, SkinTimerStop}}};
}

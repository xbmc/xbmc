/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SkinBuiltins.h"

#include "addons/Addon.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "Application.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "MediaSource.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SkinSettings.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "Util.h"
#include "URL.h"

using namespace ADDON;

/*! \brief Reload current skin.
 *  \param params The parameters.
 *  \details params[0] = "confirm" to show a confirmation dialog (optional).
 */
static int ReloadSkin(const std::vector<std::string>& params)
{
  //  Reload the skin
  g_application.ReloadSkin(!params.empty() &&
                           StringUtils::EqualsNoCase(params[0], "confirm"));

  return 0;
}

/*! \brief Unload current skin.
 *  \param params (ignored)
 */
static int UnloadSkin(const std::vector<std::string>& params)
{
  g_application.UnloadSkin(true); // we're reloading the skin after this

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
  CSettings::GetInstance().Save();

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
  std::vector<ADDON::TYPE> types;
  for (unsigned int i = 1 ; i < params.size() ; i++)
  {
    ADDON::TYPE type = TranslateType(params[i]);
    if (type != ADDON_UNKNOWN)
      types.push_back(type);
  }
  std::string result;
  if (!types.empty() && CGUIWindowAddonBrowser::SelectAddonID(types, result, true) == 1)
  {
    CSkinSettings::GetInstance().SetString(string, result);
    CSettings::GetInstance().Save();
  }

  return 0;
}

/*! \brief Select and set a skin bool setting.
 *  \param params The parameters.
 *  \details params[0] = Names of skin settings.
 */
static int SelectBool(const std::vector<std::string>& params)
{
  std::vector<std::pair<std::string, std::string>> settings;

  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDlgSelect->Reset();
  pDlgSelect->SetHeading(CVariant{g_localizeStrings.Get(atoi(params[0].c_str()))});

  for (unsigned int i = 1 ; i < params.size() ; i++)
  {
    if (params[i].find('|') != std::string::npos)
    {
      std::vector<std::string> values = StringUtils::Split(params[i], '|');
      std::string label = g_localizeStrings.Get(atoi(values[0].c_str()));
      settings.push_back(std::make_pair(label, values[1].c_str()));
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
    CSettings::GetInstance().Save();
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
    CSettings::GetInstance().Save();
    return 0;
  }
  // default is to set it to true
  int setting = CSkinSettings::GetInstance().TranslateBool(params[0]);
  CSkinSettings::GetInstance().SetBool(setting, true);
  CSettings::GetInstance().Save();

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
  g_mediaManager.GetLocalDrives(localShares);
  g_mediaManager.GetNetworkLocations(localShares);
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

  CSettings::GetInstance().Save();

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
  g_mediaManager.GetLocalDrives(localShares);

  // Note. can only browse one addon type from here
  // if browsing for addons, required param[1] is addontype string, with optional param[2]
  // as contenttype string see IAddon.h & ADDON::TranslateXX
  std::string strMask = (params.size() > 1) ? params[1] : "";
  StringUtils::ToLower(strMask);
  ADDON::TYPE type;
  if ((type = TranslateType(strMask)) != ADDON_UNKNOWN)
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
    if (type == ADDON_SCRIPT)
      strMask = ".py";
    std::string replace;
    if (CGUIDialogFileBrowser::ShowAndGetFile(url.Get(), strMask, TranslateType(type, true), replace, true, true, true))
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
  g_mediaManager.GetLocalDrives(localShares);
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
    CSettings::GetInstance().Save();
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
  if (!StringUtils::EqualsNoCase(CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME), "SKINDEFAULT"))
  {
    for (size_t i=0;i<vecTheme.size();++i)
    {
      std::string strTmpTheme(CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME));
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

  CSettings::GetInstance().SetString(CSettings::SETTING_LOOKANDFEEL_SKINTHEME, strSkinTheme);
  // also set the default color theme
  std::string colorTheme(URIUtils::ReplaceExtension(strSkinTheme, ".xml"));
  if (StringUtils::EqualsNoCase(colorTheme, "Textures.xml"))
    colorTheme = "defaults.xml";
  CSettings::GetInstance().SetString(CSettings::SETTING_LOOKANDFEEL_SKINCOLORS, colorTheme);
  g_application.ReloadSkin();

  return 0;
}

/*! \brief Reset a skin setting.
 *  \param params The parameters.
 *  \details params[0] = Name of setting to reset.
 */
static int SkinReset(const std::vector<std::string>& params)
{
  CSkinSettings::GetInstance().Reset(params[0]);
  CSettings::GetInstance().Save();

  return 0;
}

/*! \brief Reset all skin settings.
 *  \param params (ignored)
 */
static int SkinResetAll(const std::vector<std::string>& params)
{
  CSkinSettings::GetInstance().Reset();
  CSettings::GetInstance().Save();

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
///     <b>`Skin.SetBool(setting[\,value)`</b>
///     ,
///     Sets the skin `setting` to true\, for use with the conditional visibility
///     tags containing `Skin.HasSetting(setting)`. The settings are saved
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
///     <b>`Skin.SetNumeric(numeric[\,value])`</b>
///     ,
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
///     ,
///     Pops up a keyboard dialog and allows the user to input a string which can
///     be used in a label control elsewhere in the skin via the info tag
///     `Skin.String(string)`. If the value parameter is specified\, then the
///     keyboard dialog does not pop up\, and the string is set directly.
///     @param[in] string                Name of skin setting.
///     @param[in] value                 Value of skin setting (optional).
///   }
///   \table_row2_l{
///     <b>`Skin.Theme(cycle)`</b>
///     ,
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
/// \table_end
///

CBuiltins::CommandMap CSkinBuiltins::GetOperations() const
{
  return {
           {"reloadskin",         {"Reload Kodi's skin", 0, ReloadSkin}},
           {"unloadskin",         {"Unload Kodi's skin", 0, UnloadSkin}},
           {"skin.reset",         {"Resets a skin setting to default", 1, SkinReset}},
           {"skin.resetsettings", {"Resets all skin settings", 0, SkinResetAll}},
           {"skin.setaddon",      {"Prompts and set an addon", 2, SetAddon}},
           {"skin.selectbool",    {"Prompts and set a skin setting", 2, SelectBool}},
           {"skin.setbool",       {"Sets a skin setting on", 1, SetBool}},
           {"skin.setfile",       {"Prompts and sets a file", 1, SetFile}},
           {"skin.setimage",      {"Prompts and sets a skin image", 1, SetImage}},
           {"skin.setnumeric",    {"Prompts and sets numeric input", 1, SetNumeric}},
           {"skin.setpath",       {"Prompts and sets a skin path", 1, SetPath}},
           {"skin.setstring",     {"Prompts and sets skin string", 1, SetString}},
           {"skin.theme",         {"Control skin theme", 1, SetTheme}},
           {"skin.toggledebug",   {"Toggle skin debug", 0, SkinDebug}},
           {"skin.togglesetting", {"Toggles a skin setting on or off", 1, ToggleSetting}}
  };
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Skin.h"
#include "AddonManager.h"
#include "Util.h"
#include "dialogs/GUIDialogKaiToast.h"
// fallback for new skin resolution code
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/Variant.h"

#define XML_SETTINGS      "settings"
#define XML_SETTING       "setting"
#define XML_ATTR_TYPE     "type"
#define XML_ATTR_NAME     "name"
#define XML_ATTR_ID       "id"

using namespace XFILE;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

std::shared_ptr<ADDON::CSkinInfo> g_SkinInfo;

namespace ADDON
{

bool CSkinSetting::Serialize(TiXmlElement* parent) const
{
  if (parent == nullptr)
    return false;

  TiXmlElement setting(XML_SETTING);
  setting.SetAttribute(XML_ATTR_ID, name.c_str());
  setting.SetAttribute(XML_ATTR_TYPE, GetType());

  if (!SerializeSetting(&setting))
    return false;

  parent->InsertEndChild(setting);

  return true;
}

bool CSkinSetting::Deserialize(const TiXmlElement* element)
{
  if (element == nullptr)
    return false;

  name = XMLUtils::GetAttribute(element, XML_ATTR_ID);

  // backwards compatibility for guisettings.xml
  if (name.empty())
    name = XMLUtils::GetAttribute(element, XML_ATTR_NAME);

  return true;
}

bool CSkinSettingString::Deserialize(const TiXmlElement* element)
{
  value.clear();

  if (!CSkinSetting::Deserialize(element))
    return false;

  if (element->FirstChild() != nullptr)
    value = element->FirstChild()->Value();

  return true;
}

bool CSkinSettingString::SerializeSetting(TiXmlElement* element) const
{
  if (element == nullptr)
    return false;

  TiXmlText xmlValue(value);
  element->InsertEndChild(xmlValue);

  return true;
}

bool CSkinSettingBool::Deserialize(const TiXmlElement* element)
{
  value = false;

  if (!CSkinSetting::Deserialize(element))
    return false;

  if (element->FirstChild() != nullptr)
    value = StringUtils::EqualsNoCase(element->FirstChild()->ValueStr(), "true");

  return true;
}

bool CSkinSettingBool::SerializeSetting(TiXmlElement* element) const
{
  if (element == nullptr)
    return false;

  TiXmlText xmlValue(value ? "true" : "false");
  element->InsertEndChild(xmlValue);

  return true;
}

std::unique_ptr<CSkinInfo> CSkinInfo::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  RESOLUTION_INFO defaultRes = RESOLUTION_INFO();
  std::vector<RESOLUTION_INFO> resolutions;

  ELEMENTS elements;
  if (CAddonMgr::GetInstance().GetExtElements(ext->configuration, "res", elements))
  {
    for (ELEMENTS::iterator i = elements.begin(); i != elements.end(); ++i)
    {
      int width = atoi(CAddonMgr::GetInstance().GetExtValue(*i, "@width").c_str());
      int height = atoi(CAddonMgr::GetInstance().GetExtValue(*i, "@height").c_str());
      bool defRes = CAddonMgr::GetInstance().GetExtValue(*i, "@default") == "true";
      std::string folder = CAddonMgr::GetInstance().GetExtValue(*i, "@folder");
      float aspect = 0;
      std::string strAspect = CAddonMgr::GetInstance().GetExtValue(*i, "@aspect");
      std::vector<std::string> fracs = StringUtils::Split(strAspect, ':');
      if (fracs.size() == 2)
        aspect = (float)(atof(fracs[0].c_str())/atof(fracs[1].c_str()));
      if (width > 0 && height > 0)
      {
        RESOLUTION_INFO res(width, height, aspect, folder);
        res.strId = strAspect; // for skin usage, store aspect string in strId
        if (defRes)
          defaultRes = res;
        resolutions.push_back(res);
      }
    }
  }
  else
  { // no resolutions specified -> backward compatibility
    std::string defaultWide = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@defaultwideresolution");
    if (defaultWide.empty())
      defaultWide = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@defaultresolution");
    TranslateResolution(defaultWide, defaultRes);
  }

  float effectsSlowDown(1.f);
  std::string str = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@effectslowdown");
  if (!str.empty())
    effectsSlowDown = (float)atof(str.c_str());

  bool debugging = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@debugging") == "true";

  return std::unique_ptr<CSkinInfo>(new CSkinInfo(std::move(props), defaultRes, resolutions,
      effectsSlowDown, debugging));
}

CSkinInfo::CSkinInfo(
    AddonProps props,
    const RESOLUTION_INFO& resolution,
    const std::vector<RESOLUTION_INFO>& resolutions,
    float effectsSlowDown,
    bool debugging)
    : CAddon(std::move(props)),
      m_defaultRes(resolution),
      m_resolutions(resolutions),
      m_effectsSlowDown(effectsSlowDown),
      m_debugging(debugging)
{
  LoadStartupWindows(nullptr);
}

struct closestRes
{
  closestRes(const RESOLUTION_INFO &target) : m_target(target) { };
  bool operator()(const RESOLUTION_INFO &i, const RESOLUTION_INFO &j)
  {
    float diff = fabs(i.DisplayRatio() - m_target.DisplayRatio()) - fabs(j.DisplayRatio() - m_target.DisplayRatio());
    if (diff < 0) return true;
    if (diff > 0) return false;
    diff = fabs((float)i.iHeight - m_target.iHeight) - fabs((float)j.iHeight - m_target.iHeight);
    if (diff < 0) return true;
    if (diff > 0) return false;
    return fabs((float)i.iWidth - m_target.iWidth) < fabs((float)j.iWidth - m_target.iWidth);
  }
  RESOLUTION_INFO m_target;
};

void CSkinInfo::Start()
{
  if (!LoadUserSettings())
    CLog::Log(LOGWARNING, "CSkinInfo: failed to load skin settings");

  if (!m_resolutions.size())
  { // try falling back to whatever resolutions exist in the directory
    CFileItemList items;
    CDirectory::GetDirectory(Path(), items, "", DIR_FLAG_NO_FILE_DIRS);
    for (int i = 0; i < items.Size(); i++)
    {
      RESOLUTION_INFO res;
      if (items[i]->m_bIsFolder && TranslateResolution(items[i]->GetLabel(), res))
        m_resolutions.push_back(res);
    }
  }

  if (!m_resolutions.empty())
  {
    // find the closest resolution
    const RESOLUTION_INFO &target = g_graphicsContext.GetResInfo();
    RESOLUTION_INFO& res = *std::min_element(m_resolutions.begin(), m_resolutions.end(), closestRes(target));
    m_currentAspect = res.strId;
  }
}

std::string CSkinInfo::GetSkinPath(const std::string& strFile, RESOLUTION_INFO *res, const std::string& strBaseDir /* = "" */) const
{
  if (m_resolutions.empty())
    return ""; // invalid skin

  std::string strPathToUse = Path();
  if (!strBaseDir.empty())
    strPathToUse = strBaseDir;

  // if the caller doesn't care about the resolution just use a temporary
  RESOLUTION_INFO tempRes;
  if (!res)
    res = &tempRes;

  // find the closest resolution
  const RESOLUTION_INFO &target = g_graphicsContext.GetResInfo();
  *res = *std::min_element(m_resolutions.begin(), m_resolutions.end(), closestRes(target));

  std::string strPath = URIUtils::AddFileToFolder(strPathToUse, res->strMode, strFile);
  if (CFile::Exists(strPath))
    return strPath;

  // use the default resolution
  *res = m_defaultRes;

  return URIUtils::AddFileToFolder(strPathToUse, res->strMode, strFile);
}

bool CSkinInfo::HasSkinFile(const std::string &strFile) const
{
  return CFile::Exists(GetSkinPath(strFile));
}

void CSkinInfo::LoadIncludes()
{
  std::string includesPath = CSpecialProtocol::TranslatePathConvertCase(GetSkinPath("includes.xml"));
  CLog::Log(LOGINFO, "Loading skin includes from %s", includesPath.c_str());
  m_includes.ClearIncludes();
  m_includes.LoadIncludes(includesPath);
}

void CSkinInfo::ResolveIncludes(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions /* = NULL */)
{
  if(xmlIncludeConditions)
    xmlIncludeConditions->clear();

  m_includes.ResolveIncludes(node, xmlIncludeConditions);
}

int CSkinInfo::GetStartWindow() const
{
  int windowID = CSettings::GetInstance().GetInt(CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW);
  assert(m_startupWindows.size());
  for (std::vector<CStartupWindow>::const_iterator it = m_startupWindows.begin(); it != m_startupWindows.end(); ++it)
  {
    if (windowID == (*it).m_id)
      return windowID;
  }
  // return our first one
  return m_startupWindows[0].m_id;
}

bool CSkinInfo::LoadStartupWindows(const cp_extension_t *ext)
{
  m_startupWindows.clear();
  m_startupWindows.emplace_back(WINDOW_HOME, "513");
  m_startupWindows.emplace_back(WINDOW_TV_CHANNELS, "19180");
  m_startupWindows.emplace_back(WINDOW_RADIO_CHANNELS, "19183");
  m_startupWindows.emplace_back(WINDOW_PROGRAMS, "0");
  m_startupWindows.emplace_back(WINDOW_PICTURES, "1");
  m_startupWindows.emplace_back(WINDOW_MUSIC_NAV, "2");
  m_startupWindows.emplace_back(WINDOW_VIDEO_NAV, "3");
  m_startupWindows.emplace_back(WINDOW_FILES, "7");
  m_startupWindows.emplace_back(WINDOW_SETTINGS_MENU, "5");
  m_startupWindows.emplace_back(WINDOW_WEATHER, "8");
  return true;
}

void CSkinInfo::GetSkinPaths(std::vector<std::string> &paths) const
{
  RESOLUTION_INFO res;
  GetSkinPath("Home.xml", &res);
  if (!res.strMode.empty())
    paths.push_back(URIUtils::AddFileToFolder(Path(), res.strMode));
  if (res.strMode != m_defaultRes.strMode)
    paths.push_back(URIUtils::AddFileToFolder(Path(), m_defaultRes.strMode));
}

bool CSkinInfo::TranslateResolution(const std::string &name, RESOLUTION_INFO &res)
{
  std::string lower(name); StringUtils::ToLower(lower);
  if (lower == "pal")
    res = RESOLUTION_INFO(720, 576, 4.0f/3, "pal");
  else if (lower == "pal16x9")
    res = RESOLUTION_INFO(720, 576, 16.0f/9, "pal16x9");
  else if (lower == "ntsc")
    res = RESOLUTION_INFO(720, 480, 4.0f/3, "ntsc");
  else if (lower == "ntsc16x9")
    res = RESOLUTION_INFO(720, 480, 16.0f/9, "ntsc16x9");
  else if (lower == "720p")
    res = RESOLUTION_INFO(1280, 720, 0, "720p");
  else if (lower == "1080i")
    res = RESOLUTION_INFO(1920, 1080, 0, "1080i");
  else
    return false;
  return true;
}

int CSkinInfo::GetFirstWindow() const
{
  int startWindow = GetStartWindow();
  if (HasSkinFile("Startup.xml"))
    startWindow = WINDOW_STARTUP_ANIM;
  return startWindow;
}

bool CSkinInfo::IsInUse() const
{
  // Could extend this to prompt for reverting to the standard skin perhaps
  return CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKIN) == ID();
}

const INFO::CSkinVariableString* CSkinInfo::CreateSkinVariable(const std::string& name, int context)
{
  return m_includes.CreateSkinVariable(name, context);
}

void CSkinInfo::OnPreInstall()
{
  if (IsInUse())
    CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "UnloadSkin");
}

void CSkinInfo::OnPostInstall(bool update, bool modal)
{
  if (IsInUse() || (!update && !modal && 
    HELPERS::ShowYesNoDialogText(CVariant{Name()}, CVariant{24099}) == DialogResponse::YES))
  {
    CGUIDialogKaiToast *toast = (CGUIDialogKaiToast *)g_windowManager.GetWindow(WINDOW_DIALOG_KAI_TOAST);
    if (toast)
    {
      toast->ResetTimer();
      toast->Close(true);
    }
    if (CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SKIN) == ID())
      CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "ReloadSkin");
    else
      CSettings::GetInstance().SetString(CSettings::SETTING_LOOKANDFEEL_SKIN, ID());
  }
}

void CSkinInfo::SettingOptionsSkinColorsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  std::string settingValue = ((const CSettingString*)setting)->GetValue();
  // Remove the .xml extension from the Themes
  if (URIUtils::HasExtension(settingValue, ".xml"))
    URIUtils::RemoveExtension(settingValue);
  current = "SKINDEFAULT";

  // There is a default theme (just defaults.xml)
  // any other *.xml files are additional color themes on top of this one.
  
  // add the default label
  list.push_back(std::make_pair(g_localizeStrings.Get(15109), "SKINDEFAULT")); // the standard defaults.xml will be used!

  // Search for colors in the Current skin!
  std::vector<std::string> vecColors;
  std::string strPath = URIUtils::AddFileToFolder(g_SkinInfo->Path(), "colors");

  CFileItemList items;
  CDirectory::GetDirectory(CSpecialProtocol::TranslatePathConvertCase(strPath), items, ".xml");
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder && !StringUtils::EqualsNoCase(pItem->GetLabel(), "defaults.xml"))
    { // not the default one
      vecColors.push_back(pItem->GetLabel().substr(0, pItem->GetLabel().size() - 4));
    }
  }
  sort(vecColors.begin(), vecColors.end(), sortstringbyname());
  for (int i = 0; i < (int) vecColors.size(); ++i)
    list.push_back(make_pair(vecColors[i], vecColors[i]));

  // try to find the best matching value
  for (std::vector< std::pair<std::string, std::string> >::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(it->second, settingValue))
      current = settingValue;
  }
}

void CSkinInfo::SettingOptionsSkinFontsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  std::string settingValue = ((const CSettingString*)setting)->GetValue();
  bool currentValueSet = false;
  std::string strPath = g_SkinInfo->GetSkinPath("Font.xml");

  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "FillInSkinFonts: Couldn't load %s", strPath.c_str());
    return;
  }

  const TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->ValueStr() != "fonts")
  {
    CLog::Log(LOGERROR, "FillInSkinFonts: file %s doesn't start with <fonts>", strPath.c_str());
    return;
  }

  const TiXmlElement *pChild = pRootElement->FirstChildElement("fontset");
  while (pChild)
  {
    const char* idAttr = pChild->Attribute("id");
    const char* idLocAttr = pChild->Attribute("idloc");
    if (idAttr != NULL)
    {
      if (idLocAttr)
        list.push_back(std::make_pair(g_localizeStrings.Get(atoi(idLocAttr)), idAttr));
      else
        list.push_back(std::make_pair(idAttr, idAttr));

      if (StringUtils::EqualsNoCase(idAttr, settingValue))
        currentValueSet = true;
    }
    pChild = pChild->NextSiblingElement("fontset");
  }

  if (list.empty())
  { // Since no fontset is defined, there is no selection of a fontset, so disable the component
    list.push_back(make_pair(g_localizeStrings.Get(13278), ""));
    current = "";
    currentValueSet = true;
  }

  if (!currentValueSet)
    current = list[0].second;
}

void CSkinInfo::SettingOptionsSkinThemesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  // get the choosen theme and remove the extension from the current theme (backward compat)
  std::string settingValue = ((const CSettingString*)setting)->GetValue();
  URIUtils::RemoveExtension(settingValue);
  current = "SKINDEFAULT";

  // there is a default theme (just Textures.xbt)
  // any other *.xbt files are additional themes on top of this one.

  // add the default Label
  list.push_back(make_pair(g_localizeStrings.Get(15109), "SKINDEFAULT")); // the standard Textures.xbt will be used

  // search for themes in the current skin!
  std::vector<std::string> vecTheme;
  CUtil::GetSkinThemes(vecTheme);

  // sort the themes for GUI and list them
  for (int i = 0; i < (int) vecTheme.size(); ++i)
    list.push_back(make_pair(vecTheme[i], vecTheme[i]));

  // try to find the best matching value
  for (std::vector< std::pair<std::string, std::string> >::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(it->second, settingValue))
      current = settingValue;
  }
}

void CSkinInfo::SettingOptionsStartupWindowsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  int settingValue = ((const CSettingInt *)setting)->GetValue();
  current = -1;

  const std::vector<CStartupWindow> &startupWindows = g_SkinInfo->GetStartupWindows();

  for (std::vector<CStartupWindow>::const_iterator it = startupWindows.begin(); it != startupWindows.end(); ++it)
  {
    std::string windowName = it->m_name;
    if (StringUtils::IsNaturalNumber(windowName))
      windowName = g_localizeStrings.Get(atoi(windowName.c_str()));
    int windowID = it->m_id;

    list.push_back(make_pair(windowName, windowID));

    if (settingValue == windowID)
      current = settingValue;
  }

  // if the current value hasn't been properly set, set it to the first window in the list
  if (current < 0)
    current = list[0].second;
}

void CSkinInfo::ToggleDebug()
{
  m_debugging = !m_debugging;
}

int CSkinInfo::TranslateString(const std::string &setting)
{
  // run through and see if we have this setting
  for (const auto& it : m_strings)
  {
    if (StringUtils::EqualsNoCase(setting, it.second->name))
      return it.first;
  }

  // didn't find it - insert it
  CSkinSettingStringPtr skinString(new CSkinSettingString());
  skinString->name = setting;

  int number = m_bools.size() + m_strings.size();
  m_strings.insert(std::pair<int, CSkinSettingStringPtr>(number, skinString));

  return number;
}

const std::string& CSkinInfo::GetString(int setting) const
{
  const auto& it = m_strings.find(setting);
  if (it != m_strings.end())
    return it->second->value;

  return StringUtils::Empty;
}

void CSkinInfo::SetString(int setting, const std::string &label)
{
  auto&& it = m_strings.find(setting);
  if (it != m_strings.end())
  {
    it->second->value = label;
    return;
  }

  CLog::Log(LOGFATAL, "%s: unknown setting (%d) requested", __FUNCTION__, setting);
  assert(false);
}

int CSkinInfo::TranslateBool(const std::string &setting)
{
  // run through and see if we have this setting
  for (const auto& it : m_bools)
  {
    if (StringUtils::EqualsNoCase(setting, it.second->name))
      return it.first;
  }

  // didn't find it - insert it
  CSkinSettingBoolPtr skinBool(new CSkinSettingBool());
  skinBool->name = setting;

  int number = m_bools.size() + m_strings.size();
  m_bools.insert(std::pair<int, CSkinSettingBoolPtr>(number, skinBool));

  return number;
}

bool CSkinInfo::GetBool(int setting) const
{
  const auto& it = m_bools.find(setting);
  if (it != m_bools.end())
    return it->second->value;

  // default is to return false
  return false;
}

void CSkinInfo::SetBool(int setting, bool set)
{
  auto&& it = m_bools.find(setting);
  if (it != m_bools.end())
  {
    it->second->value = set;
    return;
  }

  CLog::Log(LOGFATAL, "%s: unknown setting (%d) requested", __FUNCTION__, setting);
  assert(false);
}

void CSkinInfo::Reset(const std::string &setting)
{
  // run through and see if we have this setting as a string
  for (auto& it : m_strings)
  {
    if (StringUtils::EqualsNoCase(setting, it.second->name))
    {
      it.second->value.clear();
      return;
    }
  }

  // and now check for the skin bool
  for (auto& it : m_bools)
  {
    if (StringUtils::EqualsNoCase(setting, it.second->name))
    {
      it.second->value = false;
      return;
    }
  }
}

void CSkinInfo::Reset()
{
  // clear all the settings and strings from this skin.
  for (auto& it : m_bools)
    it.second->value = false;

  for (auto& it : m_strings)
    it.second->value.clear();
}

std::set<CSkinSettingPtr> CSkinInfo::ParseSettings(const TiXmlElement* rootElement)
{
  std::set<CSkinSettingPtr> settings;
  if (rootElement == nullptr)
    return settings;

  const TiXmlElement *settingElement = rootElement->FirstChildElement(XML_SETTING);
  while (settingElement != nullptr)
  {
    CSkinSettingPtr setting = ParseSetting(settingElement);
    if (setting != nullptr)
      settings.insert(setting);

    settingElement = settingElement->NextSiblingElement(XML_SETTING);
  }

  return settings;
}

CSkinSettingPtr CSkinInfo::ParseSetting(const TiXmlElement* element)
{
  if (element == nullptr)
    return CSkinSettingPtr();

  std::string settingType = XMLUtils::GetAttribute(element, XML_ATTR_TYPE);
  CSkinSettingPtr setting;
  if (settingType == "string")
    setting = CSkinSettingPtr(new CSkinSettingString());
  else if (settingType == "bool")
    setting = CSkinSettingPtr(new CSkinSettingBool());
  else
    return CSkinSettingPtr();

  if (setting == nullptr)
    return CSkinSettingPtr();

  if (!setting->Deserialize(element))
    return CSkinSettingPtr();

  return setting;
}

bool CSkinInfo::HasSettingsToSave() const
{
  return !m_strings.empty() || !m_bools.empty();
}

bool CSkinInfo::SettingsFromXML(const CXBMCTinyXML &doc, bool loadDefaults /* = false */)
{
  const TiXmlElement *rootElement = doc.RootElement();
  if (rootElement == nullptr || rootElement->ValueStr().compare(XML_SETTINGS) != 0)
  {
    CLog::Log(LOGWARNING, "CSkinInfo: no <settings> tag found");
    return false;
  }

  m_strings.clear();
  m_bools.clear();

  int number = 0;
  std::set<CSkinSettingPtr> settings = ParseSettings(rootElement);
  for (const auto& setting : settings)
  {
    if (setting->GetType() == "string")
      m_strings.insert(std::pair<int, CSkinSettingStringPtr>(number++, std::dynamic_pointer_cast<CSkinSettingString>(setting)));
    else if (setting->GetType() == "bool")
      m_bools.insert(std::pair<int, CSkinSettingBoolPtr>(number++, std::dynamic_pointer_cast<CSkinSettingBool>(setting)));
    else
      CLog::Log(LOGWARNING, "CSkinInfo: ignoring setting of unknwon type \"%s\"", setting->GetType().c_str());
  }

  return true;
}

void CSkinInfo::SettingsToXML(CXBMCTinyXML &doc) const
{
  // add the <skinsettings> tag
  TiXmlElement rootElement(XML_SETTINGS);
  TiXmlNode *settingsNode = doc.InsertEndChild(rootElement);
  if (settingsNode == NULL)
  {
    CLog::Log(LOGWARNING, "CSkinInfo: could not create <settings> tag");
    return;
  }

  TiXmlElement* settingsElement = settingsNode->ToElement();
  for (const auto& it : m_bools)
  {
    if (!it.second->Serialize(settingsElement))
      CLog::Log(LOGWARNING, "CSkinInfo: failed to save string setting \"%s\"", it.second->name.c_str());
  }

  for (const auto& it : m_strings)
  {
    if (!it.second->Serialize(settingsElement))
      CLog::Log(LOGWARNING, "CSkinInfo: failed to save bool setting \"%s\"", it.second->name.c_str());
  }
}

} /*namespace ADDON*/

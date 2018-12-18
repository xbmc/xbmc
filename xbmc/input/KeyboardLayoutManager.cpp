/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeyboardLayoutManager.h"

#include <algorithm>

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "URL.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"

#define KEYBOARD_LAYOUTS_PATH   "special://xbmc/system/keyboardlayouts"

CKeyboardLayoutManager::~CKeyboardLayoutManager()
{
  Unload();
}

CKeyboardLayoutManager& CKeyboardLayoutManager::GetInstance()
{
  static CKeyboardLayoutManager s_instance;
  return s_instance;
}

bool CKeyboardLayoutManager::Load(const std::string& path /* = "" */)
{
  std::string layoutDirectory = path;
  if (layoutDirectory.empty())
    layoutDirectory = KEYBOARD_LAYOUTS_PATH;

  if (!XFILE::CDirectory::Exists(layoutDirectory))
  {
    CLog::Log(LOGWARNING, "CKeyboardLayoutManager: unable to load keyboard layouts from non-existing directory \"%s\"", layoutDirectory.c_str());
    return false;
  }

  CFileItemList layouts;
  if (!XFILE::CDirectory::GetDirectory(CURL(layoutDirectory), layouts, ".xml", XFILE::DIR_FLAG_DEFAULTS) || layouts.IsEmpty())
  {
    CLog::Log(LOGWARNING, "CKeyboardLayoutManager: no keyboard layouts found in %s", layoutDirectory.c_str());
    return false;
  }

  CLog::Log(LOGINFO, "CKeyboardLayoutManager: loading keyboard layouts from %s...", layoutDirectory.c_str());
  size_t oldLayoutCount = m_layouts.size();
  for (int i = 0; i < layouts.Size(); i++)
  {
    std::string layoutPath = layouts[i]->GetPath();
    if (layoutPath.empty())
      continue;

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(layoutPath))
    {
      CLog::Log(LOGWARNING, "CKeyboardLayoutManager: unable to open %s", layoutPath.c_str());
      continue;
    }

    const TiXmlElement* rootElement = xmlDoc.RootElement();
    if (rootElement == NULL)
    {
      CLog::Log(LOGWARNING, "CKeyboardLayoutManager: missing or invalid XML root element in %s", layoutPath.c_str());
      continue;
    }

    if (rootElement->ValueStr() != "keyboardlayouts")
    {
      CLog::Log(LOGWARNING, "CKeyboardLayoutManager: unexpected XML root element \"%s\" in %s", rootElement->Value(), layoutPath.c_str());
      continue;
    }

    const TiXmlElement* layoutElement = rootElement->FirstChildElement("layout");
    while (layoutElement != NULL)
    {
      CKeyboardLayout layout;
      if (!layout.Load(layoutElement))
        CLog::Log(LOGWARNING, "CKeyboardLayoutManager: failed to load %s", layoutPath.c_str());
      else if (m_layouts.find(layout.GetIdentifier()) != m_layouts.end())
        CLog::Log(LOGWARNING, "CKeyboardLayoutManager: duplicate layout with identifier \"%s\" in %s", layout.GetIdentifier().c_str(), layoutPath.c_str());
      else
      {
        CLog::Log(LOGDEBUG, "CKeyboardLayoutManager: keyboard layout \"%s\" successfully loaded", layout.GetIdentifier().c_str());
        m_layouts.insert(std::make_pair(layout.GetIdentifier(), layout));
      }

      layoutElement = layoutElement->NextSiblingElement();
    }
  }

  return m_layouts.size() > oldLayoutCount;
}

void CKeyboardLayoutManager::Unload()
{
  m_layouts.clear();
}

bool CKeyboardLayoutManager::GetLayout(const std::string& name, CKeyboardLayout& layout) const
{
  if (name.empty())
    return false;

  KeyboardLayouts::const_iterator it = m_layouts.find(name);
  if (it == m_layouts.end())
    return false;

  layout = it->second;
  return true;
}

namespace
{
  inline bool LayoutSort(const StringSettingOption& i, const StringSettingOption& j)
  {
    return (i.value > j.value);
  }
}

void CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller(SettingConstPtr setting, std::vector<StringSettingOption> &list, std::string &current, void* data)
{
  for (KeyboardLayouts::const_iterator it = CKeyboardLayoutManager::GetInstance().m_layouts.begin(); it != CKeyboardLayoutManager::GetInstance().m_layouts.end(); ++it)
  {
    std::string name = it->second.GetName();
    list.push_back(StringSettingOption(name, name));
  }

  std::sort(list.begin(), list.end(), LayoutSort);
}

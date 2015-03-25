/*
 *      Copyright (C) 2015 Team XBMC
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

#include <algorithm>

#include "KeyboardLayoutManager.h"
#include "FileItem.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"

#define KEYBOARD_LAYOUTS_PATH   "special://xbmc/system/keyboardlayouts"

CKeyboardLayoutManager::~CKeyboardLayoutManager()
{
  Unload();
}

CKeyboardLayoutManager& CKeyboardLayoutManager::Get()
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
  if (!XFILE::CDirectory::GetDirectory(CURL(layoutDirectory), layouts, ".xml") || layouts.IsEmpty())
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

void CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void* data)
{
  for (KeyboardLayouts::const_iterator it = CKeyboardLayoutManager::Get().m_layouts.begin(); it != CKeyboardLayoutManager::Get().m_layouts.end(); ++it)
  {
    std::string name = it->second.GetName();
    list.push_back(make_pair(name, name));
  }

  std::sort(list.begin(), list.end());
}

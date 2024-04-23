/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeyboardLayoutManager.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>

using namespace KODI;
using namespace KEYBOARD;

#define KEYBOARD_LAYOUTS_PATH "special://xbmc/system/keyboardlayouts"

CKeyboardLayoutManager::~CKeyboardLayoutManager()
{
  Unload();
}

bool CKeyboardLayoutManager::Load(const std::string& path /* = "" */)
{
  std::string layoutDirectory = path;
  if (layoutDirectory.empty())
    layoutDirectory = KEYBOARD_LAYOUTS_PATH;

  if (!XFILE::CDirectory::Exists(layoutDirectory))
  {
    CLog::Log(LOGWARNING,
              "CKeyboardLayoutManager: unable to load keyboard layouts from non-existing directory "
              "\"{}\"",
              layoutDirectory);
    return false;
  }

  CFileItemList layouts;
  if (!XFILE::CDirectory::GetDirectory(CURL(layoutDirectory), layouts, ".xml",
                                       XFILE::DIR_FLAG_DEFAULTS) ||
      layouts.IsEmpty())
  {
    CLog::Log(LOGWARNING, "CKeyboardLayoutManager: no keyboard layouts found in {}",
              layoutDirectory);
    return false;
  }

  CLog::Log(LOGINFO, "CKeyboardLayoutManager: loading keyboard layouts from {}...",
            layoutDirectory);
  size_t oldLayoutCount = m_layouts.size();
  for (int i = 0; i < layouts.Size(); i++)
  {
    std::string layoutPath = layouts[i]->GetPath();
    if (layoutPath.empty())
      continue;

    CXBMCTinyXML2 xmlDoc;
    if (!xmlDoc.LoadFile(layoutPath))
    {
      CLog::Log(LOGWARNING, "CKeyboardLayoutManager: unable to open {}", layoutPath);
      continue;
    }

    const auto* rootElement = xmlDoc.RootElement();
    if (rootElement == nullptr)
    {
      CLog::Log(LOGWARNING, "CKeyboardLayoutManager: missing or invalid XML root element in {}",
                layoutPath);
      continue;
    }

    if (std::strcmp(rootElement->Value(), "keyboardlayouts") != 0)
    {
      CLog::Log(LOGWARNING, "CKeyboardLayoutManager: unexpected XML root element \"{}\" in {}",
                rootElement->Value(), layoutPath);
      continue;
    }

    const auto* layoutElement = rootElement->FirstChildElement("layout");
    while (layoutElement != nullptr)
    {
      CKeyboardLayout layout;
      if (!layout.Load(layoutElement))
        CLog::Log(LOGWARNING, "CKeyboardLayoutManager: failed to load {}", layoutPath);
      else if (m_layouts.find(layout.GetIdentifier()) != m_layouts.end())
        CLog::Log(LOGWARNING,
                  "CKeyboardLayoutManager: duplicate layout with identifier \"{}\" in {}",
                  layout.GetIdentifier(), layoutPath);
      else
      {
        CLog::Log(LOGDEBUG, "CKeyboardLayoutManager: keyboard layout \"{}\" successfully loaded",
                  layout.GetIdentifier());
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
  return (i.value < j.value);
}
} // namespace

void CKeyboardLayoutManager::SettingOptionsKeyboardLayoutsFiller(
    const SettingConstPtr& setting,
    std::vector<StringSettingOption>& list,
    std::string& current,
    void* data)
{
  for (const auto& it : CServiceBroker::GetKeyboardLayoutManager()->m_layouts)
  {
    std::string name = it.second.GetName();
    list.emplace_back(name, name);
  }

  std::sort(list.begin(), list.end(), LayoutSort);
}

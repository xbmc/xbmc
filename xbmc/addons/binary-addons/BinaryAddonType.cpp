/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonType.h"
#include "BinaryAddonBase.h"

#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"

using namespace ADDON;


CBinaryAddonType::CBinaryAddonType(TYPE type, CBinaryAddonBase* info, const TiXmlElement* child)
 : m_type(type),
   m_path(info->Path())
{
  if (child)
  {
    // Get add-on library file name (if present)
    const char* library = child->Attribute("library");
    if (library == nullptr)
      library = GetPlatformLibraryName(child);
    if (library != nullptr)
      m_libname = library;

    if (!ParseExtension(child))
    {
      CLog::Log(LOGERROR, "CBinaryAddonType::%s: addon.xml file doesn't contain a valid add-on extensions (%s)", __FUNCTION__, info->ID().c_str());
      return;
    }
    SetProvides(GetValue("provides").asString());
  }
}

std::string CBinaryAddonType::LibPath() const
{
  if (m_libname.empty())
    return "";
  return URIUtils::AddFileToFolder(m_path, m_libname);
}

const char* CBinaryAddonType::GetPlatformLibraryName(const TiXmlElement* element)
{
  const char* libraryName;
#if defined(TARGET_ANDROID)
  libraryName = element->Attribute("library_android");
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
#if defined(TARGET_FREEBSD)
  libraryName = element->Attribute("library_freebsd");
  if (libraryName == nullptr)
#elif defined(TARGET_RASPBERRY_PI)
  libraryName = element->Attribute("library_rbpi");
  if (libraryName == nullptr)
#endif
  libraryName = element->Attribute("library_linux");
#elif defined(TARGET_WINDOWS_DESKTOP)
  libraryName = element->Attribute("library_windx");
  if (libraryName == nullptr)
    libraryName = element->Attribute("library_windows");
#elif defined(TARGET_WINDOWS_STORE)
  libraryName = element->Attribute("library_windowsstore");
#elif defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_IOS)
  libraryName = element->Attribute("library_ios");
  if (libraryName == nullptr)
#endif
  libraryName = element->Attribute("library_osx");
#endif

  return libraryName;
}

void CBinaryAddonType::SetProvides(const std::string &content)
{
  if (!content.empty())
  {
    for (auto provide : StringUtils::Split(content, ' '))
    {
      TYPE content = CAddonInfo::TranslateSubContent(provide);
      if (content != ADDON_UNKNOWN)
        m_providedSubContent.insert(content);
    }
  }
}

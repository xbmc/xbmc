/*
 *      Copyright (C) 2005-2017 Team Kodi
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

#include "BinaryAddonType.h"
#include "BinaryAddonBase.h"

#include "filesystem/SpecialProtocol.h"
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
#elif defined(TARGET_WINDOWS)
  libraryName = element->Attribute("library_windx");
  if (libraryName == nullptr)
    libraryName = element->Attribute("library_windows");
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

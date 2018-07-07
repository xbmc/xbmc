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

#pragma once

#include "BinaryAddonExtensions.h"

#include "addons/AddonInfo.h"

#include <string>
#include <set>

class TiXmlElement;

namespace ADDON
{

  class CBinaryAddonBase;

  class CBinaryAddonType : public CBinaryAddonExtensions
  {
  public:
    CBinaryAddonType(TYPE type, CBinaryAddonBase* info, const TiXmlElement* child);

    TYPE Type() const { return m_type; }
    std::string LibPath() const;
    const std::string& LibName() const { return m_libname; }

    bool ProvidesSubContent(const TYPE& content) const
    {
      return content == ADDON_UNKNOWN ? false : m_type == content || m_providedSubContent.count(content) > 0;
    }

    bool ProvidesSeveralSubContents() const
    {
      return m_providedSubContent.size() > 1;
    }

    size_t ProvidedSubContents() const
    {
      return m_providedSubContent.size();
    }

    static const char* GetPlatformLibraryName(const TiXmlElement* element);

  private:
    void SetProvides(const std::string &content);

    TYPE m_type;
    std::string m_path;
    std::string m_libname;
    std::set<TYPE> m_providedSubContent;
  };

} /* namespace ADDON */

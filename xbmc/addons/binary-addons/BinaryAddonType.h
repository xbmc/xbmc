/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

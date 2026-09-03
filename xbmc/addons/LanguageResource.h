/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Resource.h"
#include "language/LanguageTag.h"

#include <set>

namespace ADDON
{
class CLanguageResource : public CResource
{
public:
  explicit CLanguageResource(const AddonInfoPtr& addonInfo);

  bool IsInUse() const override;

  void OnPostInstall(bool update, bool modal) override;

  const KODI::LANGUAGE::CLanguageTag& GetLanguage() const { return m_language; }

  const std::string& GetGuiCharset() const { return m_charsetGui; }
  const std::string& GetSubtitleCharset() const { return m_charsetSubtitle; }


  const std::set<std::string, std::less<>>& GetSortTokens() const { return m_sortTokens; }

  static std::string GetAddonId(const std::string& locale);


protected:
  Published PublishedFiles() const override;

private:
  KODI::LANGUAGE::CLanguageTag m_language;

  std::string m_charsetGui;
  std::string m_charsetSubtitle;


  std::set<std::string, std::less<>> m_sortTokens;
};

}

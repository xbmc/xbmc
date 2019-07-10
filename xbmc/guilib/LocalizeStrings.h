/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file LocalizeStrings.h
\brief
*/

#include "threads/SharedSection.h"
#include "utils/ILocalizer.h"

#include <map>
#include <stdint.h>
#include <string>

/*!
 \ingroup strings
 \brief
 */

struct LocStr
{
  std::string strTranslated; // string to be used in xbmc GUI
  std::string strOriginal;   // the original English string the translation is based on
};

// The default fallback language is fixed to be English
const std::string LANGUAGE_DEFAULT = "resource.language.en_gb";
const std::string LANGUAGE_OLD_DEFAULT = "English";

class CLocalizeStrings : public ILocalizer
{
public:
  CLocalizeStrings(void);
  ~CLocalizeStrings(void) override;
  bool Load(const std::string& strPathName, const std::string& strLanguage);
  bool LoadSkinStrings(const std::string& path, const std::string& language);
  bool LoadAddonStrings(const std::string& path, const std::string& language, const std::string& addonId);
  void ClearSkinStrings();
  const std::string& Get(uint32_t code) const;
  std::string GetAddonString(const std::string& addonId, uint32_t code);
  void Clear();

  // implementation of ILocalizer
  std::string Localize(std::uint32_t code) const override { return Get(code); }

protected:
  void Clear(uint32_t start, uint32_t end);

  std::map<uint32_t, LocStr> m_strings;
  std::map<std::string, std::map<uint32_t, LocStr>> m_addonStrings;
  typedef std::map<uint32_t, LocStr>::const_iterator ciStrings;
  typedef std::map<uint32_t, LocStr>::iterator       iStrings;

  mutable CSharedSection m_stringsMutex;
  CSharedSection m_addonStringsMutex;
};

/*!
 \ingroup strings
 \brief
 */
extern CLocalizeStrings g_localizeStrings;
extern CLocalizeStrings g_localizeStringsTemp;


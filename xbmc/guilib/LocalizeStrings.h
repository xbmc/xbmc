/*!
\file LocalizeStrings.h
\brief
*/

#ifndef GUILIB_LOCALIZESTRINGS_H
#define GUILIB_LOCALIZESTRINGS_H

#pragma once

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

#include "threads/CriticalSection.h"
#include "threads/SharedSection.h"

#include <map>
#include <string>
#include <stdint.h>

#include "utils/ILocalizer.h"

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

  CSharedSection m_stringsMutex;
  CSharedSection m_addonStringsMutex;
};

/*!
 \ingroup strings
 \brief
 */
extern CLocalizeStrings g_localizeStrings;
extern CLocalizeStrings g_localizeStringsTemp;
#endif

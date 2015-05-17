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

#include <map>
#include <string>
#include <stdint.h>

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

class CLocalizeStrings
{
public:
  CLocalizeStrings(void);
  virtual ~CLocalizeStrings(void);
  bool Load(const std::string& strPathName, const std::string& strLanguage);
  bool LoadSkinStrings(const std::string& path, const std::string& language);
  void ClearSkinStrings();
  const std::string& Get(uint32_t code) const;
  void Clear();

protected:
  void Clear(uint32_t start, uint32_t end);

  /*! \brief Loads language ids and strings to memory map m_strings.
   * It tries to load a strings.po file first. If doesn't exist, it loads a strings.xml file instead.
   \param pathname The directory name, where we look for the strings file.
   \param language We load the strings for this language. Fallback language is always English.
   \param encoding Encoding of the strings. For PO files we only use utf-8.
   \param offset An offset value to place strings from the id value.
   \return false if no strings.po or strings.xml file was loaded.
   */
  bool LoadStr2Mem(const std::string &pathname, const std::string &language,
                   std::string &encoding, uint32_t offset = 0);

  /*! \brief Tries to load ids and strings from a strings.po file to m_strings map.
   * It should only be called from the LoadStr2Mem function to have a fallback.
   \param pathname The directory name, where we look for the strings file.
   \param encoding Encoding of the strings. For PO files we only use utf-8.
   \param offset An offset value to place strings from the id value.
   \param bSourceLanguage If we are loading the source English strings.po.
   \return false if no strings.po file was loaded.
   */
  bool LoadPO(const std::string &filename, std::string &encoding, uint32_t offset = 0,
              bool bSourceLanguage = false);

  /*! \brief Tries to load ids and strings from a strings.xml file to m_strings map.
   * It should only be called from the LoadStr2Mem function to try a PO file first.
   \param pathname The directory name, where we look for the strings file.
   \param encoding Encoding of the strings.
   \param offset An offset value to place strings from the id value.
   \return false if no strings.xml file was loaded.
   */
  bool LoadXML(const std::string &filename, std::string &encoding, uint32_t offset = 0);

  static std::string ToUTF8(const std::string &encoding, const std::string &str);
  std::map<uint32_t, LocStr> m_strings;
  typedef std::map<uint32_t, LocStr>::const_iterator ciStrings;
  typedef std::map<uint32_t, LocStr>::iterator       iStrings;

  CCriticalSection m_critSection;
};

/*!
 \ingroup strings
 \brief
 */
extern CLocalizeStrings g_localizeStrings;
extern CLocalizeStrings g_localizeStringsTemp;
#endif

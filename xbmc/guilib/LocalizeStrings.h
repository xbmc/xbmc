/*!
\file LocalizeStrings.h
\brief
*/

#ifndef GUILIB_LOCALIZESTRINGS_H
#define GUILIB_LOCALIZESTRINGS_H

#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "utils/StdString.h"

#include <map>

/*!
 \ingroup strings
 \brief
 */

struct LocStr
{
CStdString strTranslated; // string to be used in xbmc GUI
CStdString strOriginal;   // the original English string, the tranlsation is based on
};

// The default fallback language is fixed to be English
const CStdString SOURCE_LANGUAGE = "English";

class CLocalizeStrings
{
public:
  CLocalizeStrings(void);
  virtual ~CLocalizeStrings(void);
  bool Load(const CStdString& strPathName, const CStdString& strLanguage);
  bool LoadSkinStrings(const CStdString& path, const CStdString& language);
  void ClearSkinStrings();
  const CStdString& Get(uint32_t code) const;
  void Clear();
  uint32_t LoadBlock(const CStdString &id, const CStdString &path, const CStdString &language);
  void ClearBlock(const CStdString &id);
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
  bool LoadStr2Mem(const CStdString &pathname, const CStdString &language,
                   CStdString &encoding, uint32_t offset = 0);

  /*! \brief Tries to load ids and strings from a strings.po file to m_strings map.
   * It should only be called from the LoadStr2Mem function to have a fallback.
   \param pathname The directory name, where we look for the strings file.
   \param encoding Encoding of the strings. For PO files we only use utf-8.
   \param offset An offset value to place strings from the id value.
   \param bSourceLanguage If we are loading the source English strings.po.
   \return false if no strings.po file was loaded.
   */
  bool LoadPO(const CStdString &filename, CStdString &encoding, uint32_t offset = 0,
              bool bSourceLanguage = false);

  /*! \brief Tries to load ids and strings from a strings.xml file to m_strings map.
   * It should only be called from the LoadStr2Mem function to try a PO file first.
   \param pathname The directory name, where we look for the strings file.
   \param encoding Encoding of the strings.
   \param offset An offset value to place strings from the id value.
   \return false if no strings.xml file was loaded.
   */
  bool LoadXML(const CStdString &filename, CStdString &encoding, uint32_t offset = 0);

  CStdString ToUTF8(const CStdString &encoding, const CStdString &str);
  std::map<uint32_t, LocStr> m_strings;
  typedef std::map<uint32_t, LocStr>::const_iterator ciStrings;
  typedef std::map<uint32_t, LocStr>::iterator       iStrings;

  static const uint32_t block_start = 0xf000000;
  static const uint32_t block_size = 4096;
  std::map<CStdString, uint32_t> m_blocks;
  typedef std::map<CStdString, uint32_t>::iterator iBlocks;
};

/*!
 \ingroup strings
 \brief
 */
extern CLocalizeStrings g_localizeStrings;
extern CLocalizeStrings g_localizeStringsTemp;
#endif

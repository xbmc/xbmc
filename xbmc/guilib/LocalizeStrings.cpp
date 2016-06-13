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

#include "system.h"
#include "LocalizeStrings.h"
#include "addons/LanguageResource.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/POUtils.h"
#include "filesystem/Directory.h"
#include "threads/SharedSection.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"


/*! \brief Tries to load ids and strings from a strings.xml file to the `strings` map..
 * It should only be called from the LoadStr2Mem function to try a PO file first.
 \param pathname The directory name, where we look for the strings file.
 \param strings [out] The resulting strings map.
 \param encoding Encoding of the strings.
 \param offset An offset value to place strings from the id value.
 \return false if no strings.xml file was loaded.
 */
static bool LoadXML(const std::string &filename, std::map<uint32_t, LocStr>& strings,
    std::string &encoding, uint32_t offset = 0)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(filename))
  {
    CLog::Log(LOGDEBUG, "unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() ||
      pRootElement->ValueStr()!="strings")
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <strings>", filename.c_str());
    return false;
  }

  const auto originalSize = strings.size();
  const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
  while (pChild)
  {
    // Load old style language file with id as attribute
    const char* attrId=pChild->Attribute("id");
    if (attrId && !pChild->NoChildren())
    {
      uint32_t id = atoi(attrId) + offset;
      if (strings.find(id) == strings.end())
        strings[id].strTranslated = pChild->FirstChild()->Value();
    }
    pChild = pChild->NextSiblingElement("string");
  }
  CLog::Log(LOGDEBUG, "LocalizeStrings: loaded %lu strings from file %s", strings.size() - originalSize, filename.c_str());
  return true;
}

/*! \brief Tries to load ids and strings from a strings.po file to the `strings` map.
 * It should only be called from the LoadStr2Mem function to have a fallback.
 \param pathname The directory name, where we look for the strings file.
 \param strings [out] The resulting strings map.
 \param encoding Encoding of the strings. For PO files we only use utf-8.
 \param offset An offset value to place strings from the id value.
 \param bSourceLanguage If we are loading the source English strings.po.
 \return false if no strings.po file was loaded.
 */
static bool LoadPO(const std::string &filename, std::map<uint32_t, LocStr>& strings,
    std::string &encoding, uint32_t offset = 0 , bool bSourceLanguage = false)
{
  CPODocument PODoc;
  if (!PODoc.LoadFile(filename))
    return false;

  int counter = 0;

  while ((PODoc.GetNextEntry()))
  {
    uint32_t id;
    if (PODoc.GetEntryType() == ID_FOUND)
    {
      bool bStrInMem = strings.find((id = PODoc.GetEntryID()) + offset) != strings.end();
      PODoc.ParseEntry(bSourceLanguage);

      if (bSourceLanguage && !PODoc.GetMsgid().empty())
      {
        if (bStrInMem && (strings[id + offset].strOriginal.empty() ||
                          PODoc.GetMsgid() == strings[id + offset].strOriginal))
          continue;
        else if (bStrInMem)
          CLog::Log(LOGDEBUG,
              "POParser: id:%i was recently re-used in the English string file, which is not yet "
                  "changed in the translated file. Using the English string instead", id);
        strings[id + offset].strTranslated = PODoc.GetMsgid();
        counter++;
      }
      else if (!bSourceLanguage && !bStrInMem && !PODoc.GetMsgstr().empty())
      {
        strings[id + offset].strTranslated = PODoc.GetMsgstr();
        strings[id + offset].strOriginal = PODoc.GetMsgid();
        counter++;
      }
    }
    else if (PODoc.GetEntryType() == MSGID_FOUND)
    {
      //! @todo implement reading of non-id based string entries from the PO files.
      //! These entries would go into a separate memory map, using hash codes for fast look-up.
      //! With this memory map we can implement using gettext(), ngettext(), pgettext() calls,
      //! so that we don't have to use new IDs for new strings. Even we can start converting
      //! the ID based calls to normal gettext calls.
    }
    else if (PODoc.GetEntryType() == MSGID_PLURAL_FOUND)
    {
      //! @todo implement reading of non-id based pluralized string entries from the PO files.
      //! We can store the pluralforms for each language, in the langinfo.xml files.
    }
  }

  CLog::Log(LOGDEBUG, "LocalizeStrings: loaded %i strings from file %s", counter, filename.c_str());
  return true;
}

/*! \brief Loads language ids and strings to memory map `strings`.
 * It tries to load a strings.po file first. If doesn't exist, it loads a strings.xml file instead.
 \param pathname The directory name, where we look for the strings file.
 \param language We load the strings for this language. Fallback language is always English.
 \param strings [out] The resulting strings map.
 \param encoding Encoding of the strings. For PO files we only use utf-8.
 \param offset An offset value to place strings from the id value.
 \return false if no strings.po or strings.xml file was loaded.
 */
static bool LoadStr2Mem(const std::string &pathname_in, const std::string &language,
    std::map<uint32_t, LocStr>& strings,  std::string &encoding, uint32_t offset = 0 )
{
  std::string pathname = CSpecialProtocol::TranslatePathConvertCase(pathname_in + language);
  if (!XFILE::CDirectory::Exists(pathname))
  {
    bool exists = false;
    std::string lang;
    // check if there's a language addon using the old language naming convention
    if (ADDON::CLanguageResource::FindLegacyLanguage(language, lang))
    {
      pathname = CSpecialProtocol::TranslatePathConvertCase(pathname_in + lang);
      exists = XFILE::CDirectory::Exists(pathname);
    }

    if (!exists)
      return false;
  }

  bool useSourceLang = StringUtils::EqualsNoCase(language, LANGUAGE_DEFAULT) || StringUtils::EqualsNoCase(language, LANGUAGE_OLD_DEFAULT);
  if (LoadPO(URIUtils::AddFileToFolder(pathname, "strings.po"), strings, encoding, offset, useSourceLang))
    return true;

  return LoadXML(URIUtils::AddFileToFolder(pathname, "strings.xml"), strings, encoding, offset);
}

static bool LoadWithFallback(const std::string& path, const std::string& language, std::map<uint32_t, LocStr>& strings)
{
  std::string encoding;
  if (!LoadStr2Mem(path, language, strings, encoding))
  {
    if (StringUtils::EqualsNoCase(language, LANGUAGE_DEFAULT)) // no fallback, nothing to do
      return false;
  }

  // load the fallback
  if (!StringUtils::EqualsNoCase(language, LANGUAGE_DEFAULT))
    LoadStr2Mem(path, LANGUAGE_DEFAULT, strings, encoding);

  return true;
}

CLocalizeStrings::CLocalizeStrings(void)
{

}

CLocalizeStrings::~CLocalizeStrings(void)
{

}

void CLocalizeStrings::ClearSkinStrings()
{
  // clear the skin strings
  CExclusiveLock lock(m_stringsMutex);
  Clear(31000, 31999);
}

bool CLocalizeStrings::LoadSkinStrings(const std::string& path, const std::string& language)
{
  //! @todo shouldn't hold lock while loading file
  CExclusiveLock lock(m_stringsMutex);
  ClearSkinStrings();
  // load the skin strings in.
  return LoadWithFallback(path, language, m_strings);
}

bool CLocalizeStrings::Load(const std::string& strPathName, const std::string& strLanguage)
{
  std::map<uint32_t, LocStr> strings;
  if (!LoadWithFallback(strPathName, strLanguage, strings))
    return false;

  // fill in the constant strings
  strings[20022].strTranslated = "";
  strings[20027].strTranslated = "°F";
  strings[20028].strTranslated = "K";
  strings[20029].strTranslated = "°C";
  strings[20030].strTranslated = "°Ré";
  strings[20031].strTranslated = "°Ra";
  strings[20032].strTranslated = "°Rø";
  strings[20033].strTranslated = "°De";
  strings[20034].strTranslated = "°N";

  strings[20200].strTranslated = "km/h";
  strings[20201].strTranslated = "m/min";
  strings[20202].strTranslated = "m/s";
  strings[20203].strTranslated = "ft/h";
  strings[20204].strTranslated = "ft/min";
  strings[20205].strTranslated = "ft/s";
  strings[20206].strTranslated = "mph";
  strings[20207].strTranslated = "kts";
  strings[20208].strTranslated = "Beaufort";
  strings[20209].strTranslated = "inch/s";
  strings[20210].strTranslated = "yard/s";
  strings[20211].strTranslated = "Furlong/Fortnight";

  CExclusiveLock lock(m_stringsMutex);
  Clear();
  m_strings = std::move(strings);
  return true;
}

const std::string& CLocalizeStrings::Get(uint32_t dwCode) const
{
  CSharedLock lock(m_stringsMutex);
  ciStrings i = m_strings.find(dwCode);
  if (i == m_strings.end())
  {
    return StringUtils::Empty;
  }
  return i->second.strTranslated;
}

void CLocalizeStrings::Clear()
{
  CExclusiveLock lock(m_stringsMutex);
  m_strings.clear();
}

void CLocalizeStrings::Clear(uint32_t start, uint32_t end)
{
  CExclusiveLock lock(m_stringsMutex);
  iStrings it = m_strings.begin();
  while (it != m_strings.end())
  {
    if (it->first >= start && it->first <= end)
      m_strings.erase(it++);
    else
      ++it;
  }
}

bool CLocalizeStrings::LoadAddonStrings(const std::string& path, const std::string& language, const std::string& addonId)
{
  std::map<uint32_t, LocStr> strings;
  if (!LoadWithFallback(path, language, strings))
    return false;

  CExclusiveLock lock(m_addonStringsMutex);
  auto it = m_addonStrings.find(addonId);
  if (it != m_addonStrings.end())
    m_addonStrings.erase(it);

  return m_addonStrings.emplace(std::string(addonId), std::move(strings)).second;
}

std::string CLocalizeStrings::GetAddonString(const std::string& addonId, uint32_t code)
{
  CSharedLock lock(m_addonStringsMutex);
  auto i = m_addonStrings.find(addonId);
  if (i == m_addonStrings.end())
    return StringUtils::Empty;

  auto j = i->second.find(code);
  if (j == i->second.end())
    return StringUtils::Empty;

  return j->second.strTranslated;
}

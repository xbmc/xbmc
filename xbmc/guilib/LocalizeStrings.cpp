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
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/XMLUtils.h"
#include "utils/URIUtils.h"
#include "utils/POUtils.h"
#include "filesystem/Directory.h"
#include "threads/SingleLock.h"

CLocalizeStrings::CLocalizeStrings(void)
{

}

CLocalizeStrings::~CLocalizeStrings(void)
{

}

CStdString CLocalizeStrings::ToUTF8(const CStdString& strEncoding, const CStdString& str)
{
  if (strEncoding.empty())
    return str;

  CStdString ret;
  g_charsetConverter.ToUtf8(strEncoding, str, ret);
  return ret;
}

void CLocalizeStrings::ClearSkinStrings()
{
  // clear the skin strings
  Clear(31000, 31999);
}

bool CLocalizeStrings::LoadSkinStrings(const CStdString& path, const CStdString& language)
{
  ClearSkinStrings();
  // load the skin strings in.
  CStdString encoding;
  if (!LoadStr2Mem(path, language, encoding))
  {
    if (language.Equals(SOURCE_LANGUAGE)) // no fallback, nothing to do
      return false;
  }

  // load the fallback
  if (!language.Equals(SOURCE_LANGUAGE))
    LoadStr2Mem(path, SOURCE_LANGUAGE, encoding);

  return true;
}

bool CLocalizeStrings::LoadStr2Mem(const CStdString &pathname_in, const CStdString &language,
                                   CStdString &encoding, uint32_t offset /* = 0 */)
{
  CStdString pathname = CSpecialProtocol::TranslatePathConvertCase(pathname_in + language);
  if (!XFILE::CDirectory::Exists(pathname))
  {
    CLog::Log(LOGDEBUG,
              "LocalizeStrings: no translation available in currently set gui language, at path %s",
              pathname.c_str());
    return false;
  }

  if (LoadPO(URIUtils::AddFileToFolder(pathname, "strings.po"), encoding, offset,
      language.Equals(SOURCE_LANGUAGE)))
    return true;

  CLog::Log(LOGDEBUG, "LocalizeStrings: no strings.po file exist at %s, fallback to strings.xml",
            pathname.c_str());
  return LoadXML(URIUtils::AddFileToFolder(pathname, "strings.xml"), encoding, offset);
}

bool CLocalizeStrings::LoadPO(const CStdString &filename, CStdString &encoding,
                              uint32_t offset /* = 0 */, bool bSourceLanguage)
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
      bool bStrInMem = m_strings.find((id = PODoc.GetEntryID()) + offset) != m_strings.end();
      PODoc.ParseEntry(bSourceLanguage);

      if (bSourceLanguage && !PODoc.GetMsgid().empty())
      {
        if (bStrInMem && (m_strings[id + offset].strOriginal.empty() ||
            PODoc.GetMsgid() == m_strings[id + offset].strOriginal))
          continue;
        else if (bStrInMem)
          CLog::Log(LOGDEBUG,
                    "POParser: id:%i was recently re-used in the English string file, which is not yet "
                    "changed in the translated file. Using the English string instead", id);
        m_strings[id + offset].strTranslated = PODoc.GetMsgid();
        counter++;
      }
      else if (!bSourceLanguage && !bStrInMem && !PODoc.GetMsgstr().empty())
      {
        m_strings[id + offset].strTranslated = PODoc.GetMsgstr();
        m_strings[id + offset].strOriginal = PODoc.GetMsgid();
        counter++;
      }
    }
    else if (PODoc.GetEntryType() == MSGID_FOUND)
    {
      // TODO: implement reading of non-id based string entries from the PO files.
      // These entries would go into a separate memory map, using hash codes for fast look-up.
      // With this memory map we can implement using gettext(), ngettext(), pgettext() calls,
      // so that we don't have to use new IDs for new strings. Even we can start converting
      // the ID based calls to normal gettext calls.
    }
    else if (PODoc.GetEntryType() == MSGID_PLURAL_FOUND)
    {
      // TODO: implement reading of non-id based pluralized string entries from the PO files.
      // We can store the pluralforms for each language, in the langinfo.xml files.
    }
  }

  CLog::Log(LOGDEBUG, "POParser: loaded %i strings from file %s", counter, filename.c_str());
  return true;
}

bool CLocalizeStrings::LoadXML(const CStdString &filename, CStdString &encoding, uint32_t offset /* = 0 */)
{
  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(filename))
  {
    CLog::Log(LOGDEBUG, "unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() ||
       pRootElement->ValueStr()!=CStdString("strings"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <strings>", filename.c_str());
    return false;
  }

  const TiXmlElement *pChild = pRootElement->FirstChildElement("string");
  while (pChild)
  {
    // Load old style language file with id as attribute
    const char* attrId=pChild->Attribute("id");
    if (attrId && !pChild->NoChildren())
    {
      uint32_t id = atoi(attrId) + offset;
      if (m_strings.find(id) == m_strings.end())
        m_strings[id].strTranslated = pChild->FirstChild()->Value();
    }
    pChild = pChild->NextSiblingElement("string");
  }
  return true;
}

bool CLocalizeStrings::Load(const CStdString& strPathName, const CStdString& strLanguage)
{
  bool bLoadFallback = !strLanguage.Equals(SOURCE_LANGUAGE);

  CStdString encoding;
  CSingleLock lock(m_critSection);
  Clear();

  if (!LoadStr2Mem(strPathName, strLanguage, encoding))
  {
    // try loading the fallback
    if (!bLoadFallback || !LoadStr2Mem(strPathName, SOURCE_LANGUAGE, encoding))
      return false;

    bLoadFallback = false;
  }

  if (bLoadFallback)
    LoadStr2Mem(strPathName, SOURCE_LANGUAGE, encoding);

  // fill in the constant strings
  m_strings[20022].strTranslated = "";
  m_strings[20027].strTranslated = "°F";
  m_strings[20028].strTranslated = "K";
  m_strings[20029].strTranslated = "°C";
  m_strings[20030].strTranslated = "°Ré";
  m_strings[20031].strTranslated = "°Ra";
  m_strings[20032].strTranslated = "°Rø";
  m_strings[20033].strTranslated = "°De";
  m_strings[20034].strTranslated = "°N";

  m_strings[20200].strTranslated = "km/h";
  m_strings[20201].strTranslated = "m/min";
  m_strings[20202].strTranslated = "m/s";
  m_strings[20203].strTranslated = "ft/h";
  m_strings[20204].strTranslated = "ft/min";
  m_strings[20205].strTranslated = "ft/s";
  m_strings[20206].strTranslated = "mph";
  m_strings[20207].strTranslated = "kts";
  m_strings[20208].strTranslated = "Beaufort";
  m_strings[20209].strTranslated = "inch/s";
  m_strings[20210].strTranslated = "yard/s";
  m_strings[20211].strTranslated = "Furlong/Fortnight";

  return true;
}

static CStdString szEmptyString = "";

const CStdString& CLocalizeStrings::Get(uint32_t dwCode) const
{
  ciStrings i = m_strings.find(dwCode);
  if (i == m_strings.end())
  {
    return szEmptyString;
  }
  return i->second.strTranslated;
}

void CLocalizeStrings::Clear()
{
  m_strings.clear();
}

void CLocalizeStrings::Clear(uint32_t start, uint32_t end)
{
  iStrings it = m_strings.begin();
  while (it != m_strings.end())
  {
    if (it->first >= start && it->first <= end)
      m_strings.erase(it++);
    else
      ++it;
  }
}

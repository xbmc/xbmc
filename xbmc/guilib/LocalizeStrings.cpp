/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "LocalizeStrings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/XMLUtils.h"

CLocalizeStrings::CLocalizeStrings(void)
{

}

CLocalizeStrings::~CLocalizeStrings(void)
{

}

CStdString CLocalizeStrings::ToUTF8(const CStdString& strEncoding, const CStdString& str)
{
  if (strEncoding.IsEmpty())
    return str;

  CStdString ret;
  g_charsetConverter.stringCharsetToUtf8(strEncoding, str, ret);
  return ret;
}

void CLocalizeStrings::ClearSkinStrings()
{
  // clear the skin strings
  Clear(31000, 31999);
}

bool CLocalizeStrings::LoadSkinStrings(const CStdString& path, const CStdString& fallbackPath)
{
  ClearSkinStrings();
  // load the skin strings in.
  CStdString encoding;
  if (!LoadXML(path, encoding))
  {
    if (path == fallbackPath) // no fallback, nothing to do
      return false;
  }

  // load the fallback
  if (path != fallbackPath)
    LoadXML(fallbackPath, encoding);

  return true;
}

bool CLocalizeStrings::LoadXML(const CStdString &filename, CStdString &encoding, uint32_t offset /* = 0 */)
{
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(PTH_IC(filename)))
  {
    CLog::Log(LOGDEBUG, "unable to load %s: %s at line %d", filename.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  XMLUtils::GetEncoding(&xmlDoc, encoding);

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
    // Load new style language file with id as attribute
    const char* attrId=pChild->Attribute("id");
    if (attrId && !pChild->NoChildren())
    {
      int id = atoi(attrId) + offset;
      if (m_strings.find(id) == m_strings.end())
        m_strings[id] = ToUTF8(encoding, pChild->FirstChild()->Value());
    }
    pChild = pChild->NextSiblingElement("string");
  }
  return true;
}

bool CLocalizeStrings::Load(const CStdString& strFileName, const CStdString& strFallbackFileName)
{
  bool bLoadFallback = !strFileName.Equals(strFallbackFileName);

  CStdString encoding;
  Clear();

  if (!LoadXML(strFileName, encoding))
  {
    // try loading the fallback
    if (!bLoadFallback || !LoadXML(strFallbackFileName, encoding))
      return false;

    bLoadFallback = false;
  }

  if (bLoadFallback)
    LoadXML(strFallbackFileName, encoding);

  // fill in the constant strings
  m_strings[20022] = "";
  m_strings[20027] = ToUTF8(encoding, "°F");
  m_strings[20028] = ToUTF8(encoding, "K");
  m_strings[20029] = ToUTF8(encoding, "°C");
  m_strings[20030] = ToUTF8(encoding, "°Ré");
  m_strings[20031] = ToUTF8(encoding, "°Ra");
  m_strings[20032] = ToUTF8(encoding, "°Rø");
  m_strings[20033] = ToUTF8(encoding, "°De");
  m_strings[20034] = ToUTF8(encoding, "°N");

  m_strings[20200] = ToUTF8(encoding, "km/h");
  m_strings[20201] = ToUTF8(encoding, "m/min");
  m_strings[20202] = ToUTF8(encoding, "m/s");
  m_strings[20203] = ToUTF8(encoding, "ft/h");
  m_strings[20204] = ToUTF8(encoding, "ft/min");
  m_strings[20205] = ToUTF8(encoding, "ft/s");
  m_strings[20206] = ToUTF8(encoding, "mph");
  m_strings[20207] = ToUTF8(encoding, "kts");
  m_strings[20208] = ToUTF8(encoding, "Beaufort");
  m_strings[20209] = ToUTF8(encoding, "inch/s");
  m_strings[20210] = ToUTF8(encoding, "yard/s");
  m_strings[20211] = ToUTF8(encoding, "Furlong/Fortnight");

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
  return i->second;
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

uint32_t CLocalizeStrings::LoadBlock(const CStdString &id, const CStdString &path, const CStdString &fallbackPath)
{
  iBlocks it = m_blocks.find(id);
  if (it != m_blocks.end())
    return it->second;  // already loaded

  // grab a new block
  uint32_t offset = block_start + m_blocks.size()*block_size;
  m_blocks.insert(make_pair(id, offset));

  // load the strings
  CStdString encoding;
  bool success = LoadXML(path, encoding, offset);
  if (!success)
  {
    if (path == fallbackPath) // no fallback, nothing to do
      return 0;
  }

  // load the fallback
  if (path != fallbackPath)
    success |= LoadXML(fallbackPath, encoding, offset);

  return success ? offset : 0;
}

void CLocalizeStrings::ClearBlock(const CStdString &id)
{
  iBlocks it = m_blocks.find(id);
  if (it == m_blocks.end())
  {
    CLog::Log(LOGERROR, "%s: Trying to clear non existent block %s", __FUNCTION__, id.c_str());
    return; // doesn't exist
  }

  // clear our block
  Clear(it->second, it->second + block_size);
  m_blocks.erase(it);
}

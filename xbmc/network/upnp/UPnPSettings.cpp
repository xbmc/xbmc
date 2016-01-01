/*
 *      Copyright (C) 2013 Team XBMC
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

#include "UPnPSettings.h"
#include "filesystem/File.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#define XML_UPNP          "upnpserver"
#define XML_SERVER_UUID   "UUID"
#define XML_SERVER_PORT   "Port"
#define XML_MAX_ITEMS     "MaxReturnedItems"
#define XML_RENDERER_UUID "UUIDRenderer"
#define XML_RENDERER_PORT "PortRenderer"

using namespace XFILE;

CUPnPSettings::CUPnPSettings()
{
  Clear();
}

CUPnPSettings::~CUPnPSettings()
{
  Clear();
}

CUPnPSettings& CUPnPSettings::GetInstance()
{
  static CUPnPSettings sUPnPSettings;
  return sUPnPSettings;
}

void CUPnPSettings::OnSettingsUnloaded()
{
  Clear();
}

bool CUPnPSettings::Load(const std::string &file)
{
  CSingleLock lock(m_critical);

  Clear();

  if (!CFile::Exists(file))
    return false;
  
  CXBMCTinyXML doc;
  if (!doc.LoadFile(file))
  {
    CLog::Log(LOGERROR, "CUPnPSettings: error loading %s, Line %d\n%s", file.c_str(), doc.ErrorRow(), doc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = doc.RootElement();
  if (pRootElement == NULL || !StringUtils::EqualsNoCase(pRootElement->Value(), XML_UPNP))
  {
    CLog::Log(LOGERROR, "CUPnPSettings: error loading %s, no <upnpserver> node", file.c_str());
    return false;
  }

  // load settings
  XMLUtils::GetString(pRootElement, XML_SERVER_UUID, m_serverUUID);
  XMLUtils::GetInt(pRootElement, XML_SERVER_PORT, m_serverPort);
  XMLUtils::GetInt(pRootElement, XML_MAX_ITEMS, m_maxReturnedItems);
  XMLUtils::GetString(pRootElement, XML_RENDERER_UUID, m_rendererUUID);
  XMLUtils::GetInt(pRootElement, XML_RENDERER_PORT, m_rendererPort);

  return true;
}

bool CUPnPSettings::Save(const std::string &file) const
{
  CSingleLock lock(m_critical);

  CXBMCTinyXML doc;
  TiXmlElement xmlRootElement(XML_UPNP);
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (pRoot == NULL)
    return false;

  XMLUtils::SetString(pRoot, XML_SERVER_UUID, m_serverUUID);
  XMLUtils::SetInt(pRoot, XML_SERVER_PORT, m_serverPort);
  XMLUtils::SetInt(pRoot, XML_MAX_ITEMS, m_maxReturnedItems);
  XMLUtils::SetString(pRoot, XML_RENDERER_UUID, m_rendererUUID);
  XMLUtils::SetInt(pRoot, XML_RENDERER_PORT, m_rendererPort);

  // save the file
  return doc.SaveFile(file);
}

void CUPnPSettings::Clear()
{
  CSingleLock lock(m_critical);

  m_serverUUID.clear();
  m_serverPort = 0;
  m_maxReturnedItems = 0;
  m_rendererUUID.clear();
  m_rendererPort = 0;
}

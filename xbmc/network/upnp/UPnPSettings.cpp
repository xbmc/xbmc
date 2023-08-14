/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UPnPSettings.h"

#include "ServiceBroker.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <mutex>

#define XML_UPNP          "upnpserver"
#define XML_SERVER_UUID   "UUID"
#define XML_SERVER_PORT   "Port"
#define XML_MAX_ITEMS     "MaxReturnedItems"
#define XML_RENDERER_UUID "UUIDRenderer"
#define XML_RENDERER_PORT "PortRenderer"

CUPnPSettings::CUPnPSettings() : m_logger(CServiceBroker::GetLogging().GetLogger("CUPnPSettings"))
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
  std::unique_lock<CCriticalSection> lock(m_critical);

  Clear();

  if (!CFileUtils::Exists(file))
    return false;

  CXBMCTinyXML2 doc;
  if (!doc.LoadFile(file))
  {
    m_logger->error("error loading {}, Line {}\n{}", file, doc.ErrorLineNum(), doc.ErrorStr());
    return false;
  }

  auto* rootElement = doc.RootElement();
  if (!rootElement || !StringUtils::EqualsNoCase(rootElement->Value(), XML_UPNP))
  {
    m_logger->error("error loading {}, no <upnpserver> node", file);
    return false;
  }

  // load settings
  XMLUtils::GetString(rootElement, XML_SERVER_UUID, m_serverUUID);
  XMLUtils::GetInt(rootElement, XML_SERVER_PORT, m_serverPort);
  XMLUtils::GetInt(rootElement, XML_MAX_ITEMS, m_maxReturnedItems);
  XMLUtils::GetString(rootElement, XML_RENDERER_UUID, m_rendererUUID);
  XMLUtils::GetInt(rootElement, XML_RENDERER_PORT, m_rendererPort);

  return true;
}

bool CUPnPSettings::Save(const std::string &file) const
{
  std::unique_lock<CCriticalSection> lock(m_critical);

  CXBMCTinyXML2 doc;
  auto* element = doc.NewElement(XML_UPNP);
  auto* rootNode = doc.InsertFirstChild(element);
  if (!rootNode)
    return false;

  XMLUtils::SetString(rootNode, XML_SERVER_UUID, m_serverUUID);
  XMLUtils::SetInt(rootNode, XML_SERVER_PORT, m_serverPort);
  XMLUtils::SetInt(rootNode, XML_MAX_ITEMS, m_maxReturnedItems);
  XMLUtils::SetString(rootNode, XML_RENDERER_UUID, m_rendererUUID);
  XMLUtils::SetInt(rootNode, XML_RENDERER_PORT, m_rendererPort);

  // save the file
  return doc.SaveFile(file);
}

void CUPnPSettings::Clear()
{
  std::unique_lock<CCriticalSection> lock(m_critical);

  m_serverUUID.clear();
  m_serverPort = 0;
  m_maxReturnedItems = 0;
  m_rendererUUID.clear();
  m_rendererPort = 0;
}

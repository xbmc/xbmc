/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PhysicalPort.h"

#include "games/controllers/ControllerDefinitions.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

CPhysicalPort::CPhysicalPort(std::string portId, std::vector<std::string> accepts)
  : m_portId(std::move(portId)), m_accepts(std::move(accepts))
{
}

void CPhysicalPort::Reset()
{
  CPhysicalPort defaultPort;
  *this = std::move(defaultPort);
}

bool CPhysicalPort::IsCompatible(const std::string& controllerId) const
{
  return std::find(m_accepts.begin(), m_accepts.end(), controllerId) != m_accepts.end();
}

bool CPhysicalPort::Deserialize(const tinyxml2::XMLElement* pElement)
{
  if (pElement == nullptr)
    return false;

  Reset();

  m_portId = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_PORT_ID);

  for (const auto* pChild = pElement->FirstChildElement(); pChild != nullptr;
       pChild = pChild->NextSiblingElement())
  {
    if (std::strcmp(pChild->Value(), LAYOUT_XML_ELM_ACCEPTS) == 0)
    {
      std::string controller = XMLUtils::GetAttribute(pChild, LAYOUT_XML_ATTR_CONTROLLER);

      if (!controller.empty())
        m_accepts.emplace_back(std::move(controller));
      else
        CLog::Log(LOGWARNING, "<{}> tag is missing \"{}\" attribute", LAYOUT_XML_ELM_ACCEPTS,
                  LAYOUT_XML_ATTR_CONTROLLER);
    }
    else
    {
      CLog::Log(LOGDEBUG, "Unknown physical topology port tag: <{}>", pChild->Value());
    }
  }

  return true;
}

/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PhysicalTopology.h"

#include "games/controllers/ControllerDefinitions.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace GAME;

CPhysicalTopology::CPhysicalTopology(bool bProvidesInput, std::vector<CPhysicalPort> ports)
  : m_bProvidesInput(bProvidesInput), m_ports(std::move(ports))
{
}

void CPhysicalTopology::Reset()
{
  CPhysicalTopology defaultTopology;
  *this = std::move(defaultTopology);
}

bool CPhysicalTopology::Deserialize(const TiXmlElement* pElement)
{
  Reset();

  if (pElement == nullptr)
    return false;

  m_bProvidesInput = (XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_PROVIDES_INPUT) != "false");

  for (const TiXmlElement* pChild = pElement->FirstChildElement(); pChild != nullptr;
       pChild = pChild->NextSiblingElement())
  {
    if (pChild->ValueStr() == LAYOUT_XML_ELM_PORT)
    {
      CPhysicalPort port;
      if (port.Deserialize(pChild))
        m_ports.emplace_back(std::move(port));
    }
    else
    {
      CLog::Log(LOGDEBUG, "Unknown physical topology tag: <{}>", pChild->ValueStr());
    }
  }

  return true;
}

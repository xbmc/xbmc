/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerTopology.h"

#include "ControllerDefinitions.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <utility>

using namespace KODI;
using namespace GAME;

CControllerTopology::CControllerTopology(bool bProvidesInput, std::vector<CControllerPort> ports)
  : m_bProvidesInput(bProvidesInput), m_ports(std::move(ports))
{
}

void CControllerTopology::Reset()
{
  CControllerTopology defaultTopology;
  *this = std::move(defaultTopology);
}

bool CControllerTopology::Deserialize(const TiXmlElement* pElement)
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
      CControllerPort port;
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

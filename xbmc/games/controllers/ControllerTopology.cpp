/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ControllerTopology.h"
#include "ControllerDefinitions.h"
#include "games/controllers/Controller.h"
#include "games/GameServices.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"
#include "ServiceManager.h"
#include <algorithm>

using namespace KODI;
using namespace GAME;

// --- CControllerPort ---------------------------------------------------------

CControllerPort::CControllerPort(std::string portId, std::vector<std::string> accepts) :
  m_portId(std::move(portId)),
  m_accepts(std::move(accepts))
{
}

void CControllerPort::Reset(void)
{
  CControllerPort defaultPort;
  *this = std::move(defaultPort);
}

bool CControllerPort::IsCompatible(const std::string &controllerId) const
{
  return std::find(m_accepts.begin(), m_accepts.end(), controllerId) != m_accepts.end();
}

bool CControllerPort::Deserialize(const TiXmlElement* pElement)
{
  Reset();

  if (pElement == nullptr)
    return false;

  m_portId = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_PORT_ID);

  for (const TiXmlElement* pChild = pElement->FirstChildElement(); pChild != nullptr; pChild = pChild->NextSiblingElement())
  {
    if (pChild->ValueStr() == LAYOUT_XML_ELM_ACCEPTS)
    {
      std::string controller = XMLUtils::GetAttribute(pChild, LAYOUT_XML_ATTR_CONTROLLER);

      if (!controller.empty())
        m_accepts.emplace_back(std::move(controller));
      else
        CLog::Log(LOGWARNING, "<%s> tag is missing \"%s\" attribute", LAYOUT_XML_ELM_ACCEPTS, LAYOUT_XML_ATTR_CONTROLLER);
    }
    else
    {
      CLog::Log(LOGDEBUG, "Unknown physical topology port tag: <%s>", pChild->ValueStr().c_str());
    }
  }

  return true;
}

// --- CControllerTopology -----------------------------------------------------

CControllerTopology::CControllerTopology(bool bProvidesInput, std::vector<CControllerPort> ports) :
  m_bProvidesInput(bProvidesInput),
  m_ports(std::move(ports))
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

  for (const TiXmlElement* pChild = pElement->FirstChildElement(); pChild != nullptr; pChild = pChild->NextSiblingElement())
  {
    if (pChild->ValueStr() == LAYOUT_XML_ELM_PORT)
    {
      CControllerPort port;
      if (port.Deserialize(pChild))
        m_ports.emplace_back(std::move(port));
    }
    else
    {
      CLog::Log(LOGDEBUG, "Unknown physical topology tag: <%s>", pChild->ValueStr().c_str());
    }
  }

  return true;
}

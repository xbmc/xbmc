/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "Controller.h"
#include "ControllerDefinitions.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"

using namespace GAME;

const ControllerPtr CController::EmptyPtr;

std::unique_ptr<CController> CController::FromExtension(ADDON::AddonProps props, const cp_extension_t* ext)
{
  return std::unique_ptr<CController>(new CController(std::move(props)));
}

CController::CController(ADDON::AddonProps addonprops) :
  CAddon(std::move(addonprops)),
  m_bLoaded(false)
{
}

std::string CController::Label(void)
{
  if (m_layout.Label() > 0)
    return g_localizeStrings.GetAddonString(ID(), m_layout.Label());
  return "";
}

std::string CController::ImagePath(void) const
{
  if (!m_layout.Image().empty())
    return URIUtils::AddFileToFolder(URIUtils::GetDirectory(LibPath()), m_layout.Image());
  return "";
}

bool CController::LoadLayout(void)
{
  if (!m_bLoaded)
  {
    std::string strLayoutXmlPath = LibPath();

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(strLayoutXmlPath))
    {
      CLog::Log(LOGDEBUG, "Unable to load %s: %s at line %d", strLayoutXmlPath.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return false;
    }

    TiXmlElement* pRootElement = xmlDoc.RootElement();
    if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueStr() != LAYOUT_XML_ROOT)
    {
      CLog::Log(LOGERROR, "%s: Can't find root <%s> tag", strLayoutXmlPath.c_str(), LAYOUT_XML_ROOT);
      return false;
    }

    CLog::Log(LOGINFO, "Loading controller layout %s", strLayoutXmlPath.c_str());

    if (m_layout.Deserialize(pRootElement, this))
      m_bLoaded = true;
  }

  return m_bLoaded;
}

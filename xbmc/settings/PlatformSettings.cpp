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

#include "PlatformSettings.h"
#include <limits.h>
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

using namespace XFILE;

CPlatformSettings::CPlatformSettings()
{
}

void CPlatformSettings::Initialize()
{
  m_canQuit = true;
  m_canWindowed = true;
}

bool CPlatformSettings::Load()
{
  Initialize();
  CStdString platformSettingsXML = CSpecialProtocol::TranslatePath("special://xbmc/system/platform.xml");
  TiXmlDocument platformXML;
  if (!CFile::Exists(platformSettingsXML))
  {
    CLog::Log(LOGNOTICE, "No platform.xml to load (%s)", platformSettingsXML.c_str());
    return false;
  }

  if (!platformXML.LoadFile(platformSettingsXML))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", platformSettingsXML.c_str(), platformXML.ErrorRow(), platformXML.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = platformXML.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"platform") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <platform> node", platformSettingsXML.c_str());
    return false;
  }

  //Process platform.xml document
  TiXmlElement *pElement = pRootElement->FirstChildElement("system");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "canquit", m_canQuit);
  }

  pElement = pRootElement->FirstChildElement("video");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "canwindowed", m_canWindowed);
  }
  
  return true;
}

void CPlatformSettings::Clear()
{
}
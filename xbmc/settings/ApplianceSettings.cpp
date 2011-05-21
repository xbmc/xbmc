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

#include "ApplianceSettings.h"

#include <limits.h>

#include "system.h"
#include "Application.h"
#include "filesystem/File.h"
#include "utils/LangCodeExpander.h"
#include "LangInfo.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"

using namespace XFILE;

CApplianceSettings::CApplianceSettings()
{
}

void CApplianceSettings::Initialize()
{

}

bool CApplianceSettings::Load(CStdString profileName)
{
  Initialize();
  CStdString applianceSettingsXML = g_settings.GetUserDataItem("Appliance.xml");
  TiXmlDocument applianceXML;
  if (!CFile::Exists(applianceSettingsXML))
  {
    CLog::Log(LOGNOTICE, "No Appliance.xml to load (%s)", applianceSettingsXML.c_str());
    return false;
  }

  if (!applianceXML.LoadFile(applianceSettingsXML))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", applianceSettingsXML.c_str(), applianceXML.ErrorRow(), applianceXML.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = applianceXML.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"appliance") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <appliance> node", applianceSettingsXML.c_str());
    return false;
  }

  //Process Appliance.xml document

  return true;
}

void CApplianceSettings::Clear()
{

}
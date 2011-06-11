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

#include "AdvancedSettings.h"
#include "Application.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

using namespace XFILE;

CAdvancedSettings::CAdvancedSettings()
{
  AudioSettings = new CAudioSettings();
  KaraokeSettings = new CKaraokeSettings();
  LibrarySettings = new CLibrarySettings();
  MediaProviderSettings = new CMediaProviderSettings();
  PictureSettings = new CPictureSettings();
  SystemSettings = new CSystemSettings(g_application.IsStandAlone());
  VideoSettings = new CVideoAdvancedSettings();
}

CAdvancedSettings::~CAdvancedSettings()
{
  delete AudioSettings;
  delete KaraokeSettings;
  delete LibrarySettings;
  delete MediaProviderSettings;
  delete PictureSettings;
  delete SystemSettings;
  delete VideoSettings;
}

void CAdvancedSettings::Initialize()
{
  m_lcdRows = 4;
  m_lcdColumns = 20;
  m_lcdAddress1 = 0;
  m_lcdAddress2 = 0x40;
  m_lcdAddress3 = 0x14;
  m_lcdAddress4 = 0x54;
  m_lcdHeartbeat = false;
  m_lcdDimOnScreenSave = false;
  m_lcdScrolldelay = 1;
  m_lcdHostName = "localhost";

  m_controllerDeadzone = 0.2f;
}

bool CAdvancedSettings::Load()
{
  // NOTE: This routine should NOT set the default of any of these parameters
  //       it should instead use the versions of GetString/Integer/Float that
  //       don't take defaults in.  Defaults are set in the constructor above
  Initialize(); // In case of profile switch.
  ParseSettingsFile("special://xbmc/system/advancedsettings.xml");
  ParseSettingsFile(g_settings.GetUserDataItem("advancedsettings.xml"));
  for (unsigned int i = 0; i < m_settingsFiles.size(); i++)
    ParseSettingsFile(m_settingsFiles[i]);
  return true;
}

void CAdvancedSettings::ParseSettingsFile(CStdString file)
{
  TiXmlDocument advancedXML;
  if (!CFile::Exists(file))
  {
    CLog::Log(LOGNOTICE, "No settings file to load to load (%s)", file.c_str());
    return;
  }

  if (!advancedXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", file.c_str(), advancedXML.ErrorRow(), advancedXML.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = advancedXML.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"advancedsettings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <advancedsettings> node", file.c_str());
    return;
  }

  CLog::Log(LOGNOTICE, "Loaded settings file from %s", file.c_str());

  // Dump contents of AS.xml to debug log
  TiXmlPrinter printer;
  printer.SetLineBreak("\n");
  printer.SetIndent("  ");
  advancedXML.Accept(&printer);
  CLog::Log(LOGNOTICE, "Contents of %s are...\n%s", file.c_str(), printer.CStr());

  delete AudioSettings;
  delete KaraokeSettings;
  delete LibrarySettings;
  delete MediaProviderSettings;
  delete PictureSettings;
  delete SystemSettings;
  delete VideoSettings;

  AudioSettings = new CAudioSettings(pRootElement);
  KaraokeSettings = new CKaraokeSettings(pRootElement);
  LibrarySettings = new CLibrarySettings(pRootElement);
  MediaProviderSettings = new CMediaProviderSettings(pRootElement);
  PictureSettings = new CPictureSettings(pRootElement);
  SystemSettings = new CSystemSettings(g_application.IsStandAlone(), pRootElement);
  VideoSettings = new CVideoAdvancedSettings(pRootElement);

  TiXmlElement *pElement = pRootElement->FirstChildElement("lcd");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "rows", m_lcdRows, 1, 4);
    XMLUtils::GetInt(pElement, "columns", m_lcdColumns, 1, 40);
    XMLUtils::GetInt(pElement, "address1", m_lcdAddress1, 0, 0x100);
    XMLUtils::GetInt(pElement, "address2", m_lcdAddress2, 0, 0x100);
    XMLUtils::GetInt(pElement, "address3", m_lcdAddress3, 0, 0x100);
    XMLUtils::GetInt(pElement, "address4", m_lcdAddress4, 0, 0x100);
    XMLUtils::GetBoolean(pElement, "heartbeat", m_lcdHeartbeat);
    XMLUtils::GetBoolean(pElement, "dimonscreensave", m_lcdDimOnScreenSave);
    XMLUtils::GetInt(pElement, "scrolldelay", m_lcdScrolldelay, -8, 8);
    XMLUtils::GetString(pElement, "hostname", m_lcdHostName);
  }

  XMLUtils::GetFloat(pRootElement, "controllerdeadzone", m_controllerDeadzone, 0.0f, 1.0f);

  // load in the GUISettings overrides:
  g_guiSettings.LoadXML(pRootElement, true);  // true to hide the settings we read in
}

void CAdvancedSettings::Clear()
{
  AudioSettings->Clear();
  LibrarySettings->Clear();
  PictureSettings->Clear();
  VideoSettings->Clear();
}

void CAdvancedSettings::AddSettingsFile(CStdString filename)
{
  m_settingsFiles.push_back("special://xbmc/system/" + filename);
}
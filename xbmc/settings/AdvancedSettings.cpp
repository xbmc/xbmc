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

#include <limits.h>

#include "system.h"
#include "AdvancedSettings.h"
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

CAdvancedSettings::CAdvancedSettings()
{
  AudioSettings = new CAudioSettings();
  KaraokeSettings = new CKaraokeSettings();
  LibrarySettings = new CLibrarySettings();
  PictureSettings = new CPictureSettings();
  SystemSettings = new CSystemSettings(g_application.IsStandAlone());
  VideoSettings = new CVideoAdvancedSettings();
}

CAdvancedSettings::~CAdvancedSettings()
{
  delete AudioSettings;
  delete KaraokeSettings;
  delete LibrarySettings;
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

  m_songInfoDuration = 10;
  m_busyDialogDelay = 2000;

  m_cddbAddress = "freedb.freedb.org";

  m_remoteDelay = 3;
  m_controllerDeadzone = 0.2f;

  m_iTuxBoxStreamtsPort = 31339;
  m_bTuxBoxAudioChannelSelection = false;
  m_bTuxBoxSubMenuSelection = false;
  m_bTuxBoxPictureIcon= true;
  m_bTuxBoxSendAllAPids= false;
  m_iTuxBoxEpgRequestTime = 10; //seconds
  m_iTuxBoxDefaultSubMenu = 4;
  m_iTuxBoxDefaultRootMenu = 0; //default TV Mode
  m_iTuxBoxZapWaitTime = 0; // Time in sec. Default 0:OFF
  m_bTuxBoxZapstream = true;
  m_iTuxBoxZapstreamPort = 31344;

  m_iMythMovieLength = 0; // 0 == Off

  m_bEdlMergeShortCommBreaks = false;      // Off by default
  m_iEdlMaxCommBreakLength = 8 * 30 + 10;  // Just over 8 * 30 second commercial break.
  m_iEdlMinCommBreakLength = 3 * 30;       // 3 * 30 second commercial breaks.
  m_iEdlMaxCommBreakGap = 4 * 30;          // 4 * 30 second commercial breaks.
  m_iEdlMaxStartGap = 5 * 60;              // 5 minutes.
  m_iEdlCommBreakAutowait = 0;             // Off by default
  m_iEdlCommBreakAutowind = 0;             // Off by default



  m_fullScreen = false;


  m_GLRectangleHack = false;
  m_iSkipLoopFilter = 0;
  m_AllowD3D9Ex = true;
  m_ForceD3D9Ex = false;
  m_AllowDynamicTextures = true;

  m_ForcedSwapTime = 0.0;

  m_measureRefreshrate = false;
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

  // succeeded - tell the user it worked
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
  delete PictureSettings;
  delete SystemSettings;
  delete VideoSettings;

  AudioSettings = new CAudioSettings(pRootElement);
  KaraokeSettings = new CKaraokeSettings(pRootElement);
  LibrarySettings = new CLibrarySettings(pRootElement);
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

  XMLUtils::GetString(pRootElement, "cddbaddress", m_cddbAddress);

  XMLUtils::GetInt(pRootElement, "songinfoduration", m_songInfoDuration, 0, INT_MAX);
  XMLUtils::GetInt(pRootElement, "busydialogdelay", m_busyDialogDelay, 0, 5000);


  XMLUtils::GetBoolean(pRootElement,"glrectanglehack", m_GLRectangleHack);
  XMLUtils::GetInt(pRootElement,"skiploopfilter", m_iSkipLoopFilter, -16, 48);
  XMLUtils::GetFloat(pRootElement, "forcedswaptime", m_ForcedSwapTime, 0.0, 100.0);

  XMLUtils::GetBoolean(pRootElement,"allowd3d9ex", m_AllowD3D9Ex);
  XMLUtils::GetBoolean(pRootElement,"forced3d9ex", m_ForceD3D9Ex);
  XMLUtils::GetBoolean(pRootElement,"allowdynamictextures", m_AllowDynamicTextures);


  //Tuxbox
  pElement = pRootElement->FirstChildElement("tuxbox");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "streamtsport", m_iTuxBoxStreamtsPort, 0, 65535);
    XMLUtils::GetBoolean(pElement, "audiochannelselection", m_bTuxBoxAudioChannelSelection);
    XMLUtils::GetBoolean(pElement, "submenuselection", m_bTuxBoxSubMenuSelection);
    XMLUtils::GetBoolean(pElement, "pictureicon", m_bTuxBoxPictureIcon);
    XMLUtils::GetBoolean(pElement, "sendallaudiopids", m_bTuxBoxSendAllAPids);
    XMLUtils::GetInt(pElement, "epgrequesttime", m_iTuxBoxEpgRequestTime, 0, 3600);
    XMLUtils::GetInt(pElement, "defaultsubmenu", m_iTuxBoxDefaultSubMenu, 1, 4);
    XMLUtils::GetInt(pElement, "defaultrootmenu", m_iTuxBoxDefaultRootMenu, 0, 4);
    XMLUtils::GetInt(pElement, "zapwaittime", m_iTuxBoxZapWaitTime, 0, 120);
    XMLUtils::GetBoolean(pElement, "zapstream", m_bTuxBoxZapstream);
    XMLUtils::GetInt(pElement, "zapstreamport", m_iTuxBoxZapstreamPort, 0, 65535);
  }

  // Myth TV
  pElement = pRootElement->FirstChildElement("myth");
  if (pElement)
  {
    XMLUtils::GetInt(pElement, "movielength", m_iMythMovieLength);
  }

  // EDL commercial break handling
  pElement = pRootElement->FirstChildElement("edl");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "mergeshortcommbreaks", m_bEdlMergeShortCommBreaks);
    XMLUtils::GetInt(pElement, "maxcommbreaklength", m_iEdlMaxCommBreakLength, 0, 10 * 60); // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "mincommbreaklength", m_iEdlMinCommBreakLength, 0, 5 * 60);  // Between 0 and 5 minutes
    XMLUtils::GetInt(pElement, "maxcommbreakgap", m_iEdlMaxCommBreakGap, 0, 5 * 60);        // Between 0 and 5 minutes.
    XMLUtils::GetInt(pElement, "maxstartgap", m_iEdlMaxStartGap, 0, 10 * 60);               // Between 0 and 10 minutes
    XMLUtils::GetInt(pElement, "commbreakautowait", m_iEdlCommBreakAutowait, 0, 10);        // Between 0 and 10 seconds
    XMLUtils::GetInt(pElement, "commbreakautowind", m_iEdlCommBreakAutowind, 0, 10);        // Between 0 and 10 seconds
  }

  XMLUtils::GetInt(pRootElement, "remotedelay", m_remoteDelay, 1, 20);
  XMLUtils::GetFloat(pRootElement, "controllerdeadzone", m_controllerDeadzone, 0.0f, 1.0f);
  XMLUtils::GetBoolean(pRootElement, "measurerefreshrate", m_measureRefreshrate);

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
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
#include "stdafx.h"
#include "PluginSettings.h"
#include "PVRClientSettings.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"


CPVRClientSettings::CPVRClientSettings()
{
}

CPVRClientSettings::~CPVRClientSettings()
{
}

bool CPVRClientSettings::Load(const CURL& url)
{
  m_url = url;

  // create the users filepath
  m_userFileName.Format("special://profile/plugin_data/%s/%s", url.GetHostName().c_str(), url.GetFileName().c_str());
  CUtil::RemoveSlashAtEnd(m_userFileName);
  CUtil::AddFileToFolder(m_userFileName, "settings.xml", m_userFileName);

  // Create our final path
  CStdString pluginFileName = "special://home/plugins/";

  CUtil::AddFileToFolder(pluginFileName, url.GetHostName(), pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, url.GetFileName(), pluginFileName);

  CUtil::AddFileToFolder(pluginFileName, "resources", pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, "settings.xml", pluginFileName);

  pluginFileName = pluginFileName;

  if (!m_pluginXmlDoc.LoadFile(pluginFileName))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", pluginFileName.c_str(), m_pluginXmlDoc.ErrorRow(), m_pluginXmlDoc.ErrorDesc());
    return false;
  }

  // Make sure that the plugin XML has the settings element
  TiXmlElement *setting = m_pluginXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", pluginFileName.c_str());
    return false;
  }

  // Load the user saved settings. If it does not exist, create it
  if (!m_userXmlDoc.LoadFile(m_userFileName))
  {
    TiXmlDocument doc;
    TiXmlDeclaration decl("1.0", "UTF-8", "yes");
    doc.InsertEndChild(decl);

    TiXmlElement xmlRootElement("settings");
    doc.InsertEndChild(xmlRootElement);

    m_userXmlDoc = doc;

    // Don't worry about the actual settings, they will be set when the user clicks "Ok"
    // in the settings dialog
  }

  return true;
}

bool CPVRClientSettings::Save(void)
{
  // break down the path into directories
  CStdString strRoot, strType, strPlugin;
  CUtil::GetDirectory(m_userFileName, strPlugin);
  CUtil::RemoveSlashAtEnd(strPlugin);
  CUtil::GetDirectory(strPlugin, strType);
  CUtil::RemoveSlashAtEnd(strType);
  CUtil::GetDirectory(strType, strRoot);
  CUtil::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!DIRECTORY::CDirectory::Exists(strRoot))
    DIRECTORY::CDirectory::Create(strRoot);
  if (!DIRECTORY::CDirectory::Exists(strType))
    DIRECTORY::CDirectory::Create(strType);
  if (!DIRECTORY::CDirectory::Exists(strPlugin))
    DIRECTORY::CDirectory::Create(strPlugin);

  return m_userXmlDoc.SaveFile(m_userFileName);
}

bool CPVRClientSettings::SettingsExist(const CStdString& strPath)
{
  CURL url(strPath);
  CStdString pluginFileName = "special://home/plugins/";

  // Create our final path
  CUtil::AddFileToFolder(pluginFileName, url.GetHostName(), pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, url.GetFileName(), pluginFileName);

  CUtil::AddFileToFolder(pluginFileName, "resources", pluginFileName);
  CUtil::AddFileToFolder(pluginFileName, "settings.xml", pluginFileName);

  // Load the settings file to verify it's valid
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(pluginFileName))
    return false;

  // Make sure that the plugin XML has the settings element
  TiXmlElement *setting = xmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
    return false;

  return true;
}

CPVRClientSettings& CPVRClientSettings::operator=(const CBasicSettings& settings)
{
  *((CBasicSettings*)this) = settings;

  return *this;
}

CPVRClientSettings g_currentPVRClientSettings;

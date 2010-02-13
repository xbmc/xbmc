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

#include "ScriptSettings.h"
#include "Util.h"
#include "FileSystem/Directory.h"
#include "utils/log.h"
#ifdef _WIN32
#include "PlatformDefs.h" //for strcasecmp
#endif

CScriptSettings::CScriptSettings()
{
}

CScriptSettings::~CScriptSettings()
{
}

bool CScriptSettings::Load(const CStdString& strPath)
{
  m_scriptPath = strPath;

  // create the users filepath
  CUtil::RemoveSlashAtEnd(m_scriptPath);
  m_userFileName.Format("special://profile/script_data/%s", CUtil::GetFileName(m_scriptPath));
  CUtil::AddFileToFolder(m_userFileName, "settings.xml", m_userFileName);

  // Create our final settings path
  CStdString scriptFileName;
  CUtil::AddFileToFolder(m_scriptPath, "resources", scriptFileName);
  CUtil::AddFileToFolder(scriptFileName, "settings.xml", scriptFileName);

  if (!m_pluginXmlDoc.LoadFile(scriptFileName))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", scriptFileName.c_str(), m_pluginXmlDoc.ErrorRow(), m_pluginXmlDoc.ErrorDesc());
    return false;
  }

  // Make sure that the script XML has the settings element
  TiXmlElement *setting = m_pluginXmlDoc.RootElement();
  if (!setting || strcasecmp(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", scriptFileName.c_str());
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

bool CScriptSettings::Save(void)
{
  // break down the path into directories
  CStdString strRoot, strType, strScript;
  CUtil::GetDirectory(m_userFileName, strScript);
  CUtil::RemoveSlashAtEnd(strScript);
  CUtil::GetDirectory(strScript, strType);
  CUtil::RemoveSlashAtEnd(strType);
  CUtil::GetDirectory(strType, strRoot);
  CUtil::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!XFILE::CDirectory::Exists(strRoot))
    XFILE::CDirectory::Create(strRoot);
  if (!XFILE::CDirectory::Exists(strType))
    XFILE::CDirectory::Create(strType);
  if (!XFILE::CDirectory::Exists(strScript))
    XFILE::CDirectory::Create(strScript);

  return m_userXmlDoc.SaveFile(m_userFileName);
}

bool CScriptSettings::SettingsExist(const CStdString& strPath)
{
  CStdString scriptFileName = strPath;
  CUtil::RemoveSlashAtEnd(scriptFileName);

  CUtil::AddFileToFolder(scriptFileName, "resources", scriptFileName);
  CUtil::AddFileToFolder(scriptFileName, "settings.xml", scriptFileName);

  // Load the settings file to verify it's valid
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(scriptFileName))
    return false;

  // Make sure that the script XML has the settings element
  TiXmlElement *setting = xmlDoc.RootElement();
  if (!setting || strcasecmp(setting->Value(), "settings") != 0)
    return false;

  return true;
}

CScriptSettings& CScriptSettings::operator=(const CBasicSettings& settings)
{
  *((CBasicSettings*)this) = settings;

  return *this;
}


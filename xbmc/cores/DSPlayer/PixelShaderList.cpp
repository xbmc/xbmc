/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 
#ifdef HAS_DS_PLAYER

#include "PixelShaderList.h"
#include "Settings.h"
#include "FileSystem\File.h"
#include "XMLUtils.h"

CPixelShaderList::~CPixelShaderList()
{
  m_activatedPixelShaders.clear();
  
  for (PixelShaderVector::iterator it = m_pixelShaders.begin();
    it != m_pixelShaders.end(); ++it)
  {
    delete (*it);
  }
  m_pixelShaders.clear();
}

void CPixelShaderList::Load()
{
  LoadXMLFile(g_settings.GetUserDataItem("dsplayer/shaders.xml"));
  LoadXMLFile("special://xbmc/system/players/dsplayer/shaders/shaders.xml");
}

bool CPixelShaderList::LoadXMLFile(const CStdString& xmlFile)
{
  CLog::Log(LOGNOTICE, "Loading pixel shaders list from %s", xmlFile.c_str());
  if (! XFILE::CFile::Exists(xmlFile))
  {
    CLog::Log(LOGNOTICE, "%s does not exist. Skipping.", xmlFile.c_str());
    return false;
  }

  TiXmlDocument xmlDoc;
  if (! xmlDoc.LoadFile(xmlFile))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", xmlFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *rootElement = xmlDoc.RootElement();
  if (!rootElement || strcmpi(rootElement->Value(), "shaders") != 0)
  {
    CLog::Log(LOGERROR, "Error loading shaders list, no <shaders> node");
    return false;
  }

  TiXmlElement* shaders = rootElement->FirstChildElement("shader");
  while (shaders)
  {
    CExternalPixelShader *shader = new CExternalPixelShader(shaders);
    if (shader->IsValid())
      m_pixelShaders.push_back(shader);
    else
      delete shader;

    shaders = shaders->NextSiblingElement("shader");
  }
  return true;
}

#endif
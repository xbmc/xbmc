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
#include "FileSystem\Directory.h"
#include "XMLUtils.h"
#include "SingleLock.h"
#include "Util.h"

CPixelShaderList::~CPixelShaderList()
{
  CSingleLock lock(m_accessLock);

  m_activatedPixelShaders.clear();
  
  for (PixelShaderVector::iterator it = m_pixelShaders.begin();
    it != m_pixelShaders.end(); ++it)
  {
    delete (*it);
  }
  m_pixelShaders.clear();
}

void CPixelShaderList::SaveXML()
{
  CStdString userDataDSPlayer = CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "dsplayer");
  if (! XFILE::CDirectory::Exists(userDataDSPlayer))
  {
    if (! XFILE::CDirectory::Create(userDataDSPlayer))
    {
      CLog::Log(LOGERROR, "%s Failed to create userdata folder (%s). Pixel shaders settings won't be saved.", __FUNCTION__, userDataDSPlayer.c_str());
      return;
    }
  }

  CStdString xmlFile = CUtil::AddFileToFolder(userDataDSPlayer, "shaders.xml");
  if (XFILE::CFile::Exists(xmlFile))
    XFILE::CFile::Delete(xmlFile);

  TiXmlDocument xmlDoc;
  TiXmlNode* pRoot = xmlDoc.InsertEndChild( TiXmlElement("shaders") );

  for (PixelShaderVector::iterator it = m_pixelShaders.begin();
    it != m_pixelShaders.end(); ++it)
  {
    CExternalPixelShader * ps = (*it);
    pRoot->InsertEndChild( ps->ToXML() );
  }

  if (!xmlDoc.SaveFile(xmlFile))
    CLog::Log(LOGERROR, "%s Failed to save shaders.xml file", __FUNCTION__);
}

void CPixelShaderList::Load()
{
  LoadXMLFile(g_settings.GetUserDataItem("dsplayer/shaders.xml"));
  LoadXMLFile("special://xbmc/system/players/dsplayer/shaders/shaders.xml");

  UpdateActivatedList();
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
    std::auto_ptr<CExternalPixelShader> shader(new CExternalPixelShader(shaders));
    if (shader->IsValid())
    {
      if (std::find_if(m_pixelShaders.begin(), m_pixelShaders.end(), std::bind1st(std::ptr_fun(HasSameID),
        shader->GetId())) != m_pixelShaders.end())
      {
        shaders = shaders->NextSiblingElement("shader");
        continue;
      }
      if (std::find_if(m_pixelShaders.begin(), m_pixelShaders.end(), std::bind1st(std::ptr_fun(HasSameIndex),
        shader->GetIndex())) != m_pixelShaders.end())
        CLog::Log(LOGERROR, "%s A pixel shader with the index %d already exists", __FUNCTION__, shader->GetIndex());
      else
      {
        CLog::Log(LOGINFO, "Loaded pixel shader \"%s\", id %d, %s", shader->GetName().c_str(),
          shader->GetId(), shader->IsEnabled() ? "enabled" : "disabled");
        m_pixelShaders.push_back(shader.release());
      }
    }

    shaders = shaders->NextSiblingElement("shader");
  }

  return true;
}

void CPixelShaderList::UpdateActivatedList()
{
  Sort();
  CSingleLock lock(m_accessLock);

  m_activatedPixelShaders.clear();
  for (PixelShaderVector::iterator it = m_pixelShaders.begin();
    it != m_pixelShaders.end(); ++it)
  {
    if ( (*it)->IsEnabled() )
      m_activatedPixelShaders.push_back( *it );
  }
}

void CPixelShaderList::MoveUp(uint32_t index)
{
  if (index > m_pixelShaders.size() || index < 1)
    return;

  // Swap
  {
    CSingleLock lock(m_accessLock);
    m_pixelShaders[index]->SetIndex(index - 1);
    m_pixelShaders[index - 1]->SetIndex(index);
  }

  Sort();
}

void CPixelShaderList::MoveDown(uint32_t index)
{
  if (index > m_pixelShaders.size() - 1)
    return;

  // Swap
  {
    CSingleLock lock(m_accessLock);
    m_pixelShaders[index]->SetIndex(index + 1);
    m_pixelShaders[index + 1]->SetIndex(index);
  }

  Sort();
}

void CPixelShaderList::Sort()
{
  {
    CSingleLock lock(m_accessLock);
    std::sort(m_pixelShaders.begin(), m_pixelShaders.end(), SortPixelShader);

    for (uint32_t i = 0; i < m_pixelShaders.size(); i++)
      m_pixelShaders[i]->SetIndex(i);
  }
}

bool HasSameIndex(uint32_t index, CExternalPixelShader* p2)
{
  return index == p2->GetIndex();
}

bool HasSameID(uint32_t id, CExternalPixelShader* p2)
{
  return id == p2->GetId();
}

#endif
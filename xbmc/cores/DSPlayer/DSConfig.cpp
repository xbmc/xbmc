/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "dsconfig.h"
#include "Utils/log.h"
#include "DShowUtil/DShowUtil.h"
#include "CharsetConverter.h"
#include "Filters/ffdshow_constants.h"
//Required for the gui buttons
#include "XMLUtils.h"
#include "FileSystem/SpecialProtocol.h"

#include "GuiSettings.h"
#include "FGLoader.h"
#include "Filters/VMR9AllocatorPresenter.h"

using namespace std;

class CDSConfig g_dsconfig;
CDSConfig::CDSConfig(void)
{
  m_pGraphBuilder = NULL;
  m_pIMpcDecFilter = NULL;
  m_pSplitter = NULL;
  pIffdshowDecoder = NULL;
  pGraph = NULL;
  pGraph = NULL;
  pQualProp = NULL;
  m_pGuidDxva = GUID_NULL;
}

CDSConfig::~CDSConfig(void)
{
  while (! m_pPropertiesFilters.empty())
    m_pPropertiesFilters.pop_back();
}
void CDSConfig::ClearConfig()
{
  m_pGraphBuilder = NULL;
  m_pIMpcDecFilter = NULL;
  pIffdshowDecoder = NULL;
  pQualProp = NULL;
  pGraph = NULL;
  m_pStdDxva.Format("");
  m_pGuidDxva = GUID_NULL;
  while (! m_pPropertiesFilters.empty())
    m_pPropertiesFilters.pop_back();
}
HRESULT CDSConfig::ConfigureFilters(IFilterGraph2* pGB)
{
  HRESULT hr = S_OK;
  m_pGraphBuilder = pGB;
  pGB = NULL;
  m_pIMpcDecFilter = NULL;
  pIffdshowDecoder = NULL;
  pQualProp = NULL;
  m_pStdDxva = DShowUtil::GetDXVAMode(&m_pGuidDxva);

  while (! m_pPropertiesFilters.empty())
    m_pPropertiesFilters.pop_back();

  IBaseFilter *pBF = NULL;
  if (SUCCEEDED(m_pGraphBuilder->FindFilterByName(L"Xbmc VMR9", &pBF)))
  {
    pBF->QueryInterface(IID_IQualProp,(void **) &pQualProp);
  }
  SAFE_RELEASE(pBF);
  ConfigureFilters();

  return hr;
}

void CDSConfig::ConfigureFilters()
{
  GetDxvaGuid();
  BeginEnumFilters(m_pGraphBuilder, pEF, pBF)
  {
    GetffdshowFilters(pBF);
    LoadPropertiesPage(pBF);
  }
  EndEnumFilters

  CreatePropertiesXml();
}

bool CDSConfig::LoadPropertiesPage(IBaseFilter *pBF)
{
  if ((pBF == CFGLoader::Filters.AudioRenderer.pBF && CFGLoader::Filters.AudioRenderer.guid != CLSID_ReClock) || pBF == CFGLoader::Filters.VideoRenderer.pBF )
    return false;

  ISpecifyPropertyPages *pProp = NULL;
  CAUUID pPages;
  if ( SUCCEEDED( pBF->QueryInterface(IID_ISpecifyPropertyPages, (void **) &pProp) ) )
  {
    pProp->GetPages(&pPages);
    if (pPages.cElems > 0)
    {
      m_pPropertiesFilters.push_back(pBF);
    
      CStdString filterName;
      g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBF), filterName);
      CLog::Log(LOGNOTICE, "%s \"%s\" expose ISpecifyPropertyPages", __FUNCTION__, filterName.c_str());
    }
	  SAFE_RELEASE(pProp);
    CoTaskMemFree(pPages.pElems);
    return true;
    
  } 
  else
    return false;
}

void CDSConfig::CreatePropertiesXml()
{
  //verify we have at least one property page
  if (m_pPropertiesFilters.empty())
    return;

  CStdString pStrName;
  CStdString pStrId;
  int pIntId = 0;
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("strings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) 
    return;
  
  for (std::vector<IBaseFilter*>::const_iterator it = m_pPropertiesFilters.begin() ; it != m_pPropertiesFilters.end(); it++)
  {
    g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(*it), pStrName);
    TiXmlElement newFilterElement("string");
    
    //Set the id of the lang
    pStrId.Format("%i", pIntId);
    newFilterElement.SetAttribute("id", pStrId.c_str());

    //set the name of the filter in the element
    TiXmlNode *pNewNode = pRoot->InsertEndChild(newFilterElement);
    if (! pNewNode)
      break;
    
    TiXmlText value(pStrName.c_str());
    pNewNode->InsertEndChild(value);
    pIntId++;
  }
  xmlDoc.SaveFile("special://temp//dslang.xml");
}

void CDSConfig::ShowPropertyPage(IBaseFilter *pBF)
{
  m_pCurrentProperty = new CDSPropertyPage(g_dsconfig.pGraph, pBF);
  m_pCurrentProperty->Initialize(true);
  return;
  //this is not working yet, calling the switch to fake fullscreen is reseting the video renderer
  //A better handling of the video resolution change by the video renderers is needed
  if (g_graphicsContext.IsFullScreenRoot() && !g_guiSettings.GetBool("videoscreen.fakefullscreen"))
  {
    //True fullscreen cant handle those proprety page
    m_pCurrentProperty->Initialize(false);
    //g_graphicsContext.SetFullScreenVideo();
  }
  else
  {
    m_pCurrentProperty->Initialize(true);
  }
}

void CDSConfig::GetDxvaGuid()
{
  //Get the subtype of the pin this is actually the guid used for dxva description
  IBaseFilter* pBFV = CFGLoader::Filters.VideoRenderer.pBF;
  GUID dxvaGuid = GUID_NULL;
  IPin *pPin = NULL;
  pPin = DShowUtil::GetFirstPin(pBFV);
  if (pPin)
  {
    AM_MEDIA_TYPE pMT;
    HRESULT hr = pPin->ConnectionMediaType(&pMT);
    dxvaGuid = pMT.subtype;
    FreeMediaType(pMT);
  }
  m_pGuidDxva = dxvaGuid;
  if (dxvaGuid == GUID_NULL)
    m_pStdDxva.Format("");
  else
    m_pStdDxva = DShowUtil::GetDXVAMode(&dxvaGuid);
  SAFE_RELEASE(pBFV);    
}

bool CDSConfig::GetffdshowFilters(IBaseFilter* pBF)
{
  HRESULT hr;
  /*if (!pIffdshowDecFilter)
  {
    hr = pBF->QueryInterface(IID_IffdshowDecVideoA, (void **) &pIffdshowDecFilter );
  }
  if (!pIffdshowBase)
  {
    hr = pBF->QueryInterface(IID_IffdshowBaseA,(void **) &pIffdshowBase );
  }*/
  if (!pIffdshowDecoder)
    hr = pBF->QueryInterface(IID_IffDecoder, (void **) &pIffdshowDecoder );
  
  return true;
}

bool CDSConfig::LoadffdshowSubtitles(CStdString filePath)
{
  if (pIffdshowDecoder)
  {
    if (SUCCEEDED(pIffdshowDecoder->compat_putParamStr(IDFF_subFilename,filePath.c_str())))
      return true;
    return false;
  }
  else
    return false;
}
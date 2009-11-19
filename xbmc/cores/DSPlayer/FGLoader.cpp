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

#include "FGLoader.h"
#include "dshowutil/dshowutil.h"

#include "XMLUtils.h"
#include "charsetconverter.h"
#include "Log.h"

#include "filters/xbmcfilesource.h"
#include "filters/asyncflt.h"
#include "filters/asyncio.h"

#include "FileSystem/SpecialProtocol.h"
using namespace std;
CFGLoader::CFGLoader(IGraphBuilder2* gb,CStdString xbmcPath)
:m_pGraphBuilder(gb)
,m_xbmcPath(xbmcPath)
{
}

CFGLoader::~CFGLoader()
{
}




HRESULT CFGLoader::InsertSourceFilter(const CFileItem& pFileItem, TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  if ( ( (CStdString)pRule->Attribute("source")).length() > 0 )
  {
    POSITION pos = m_configFilter.GetHeadPosition();
    CFGFilterFile* pFGF;
    while(pos)
    {
      pFGF = m_configFilter.GetNext(pos);
      if ( ((CStdString)pRule->Attribute("source")).Equals(pFGF->GetXFilterName().c_str(),false) )
      {
        if(SUCCEEDED(pFGF->Create(&m_SourceF, pUnk)))
          break;
	    else
	    {
          CLog::Log(LOGERROR,"DSPlayer %s Failed to create the source filter",__FUNCTION__);
          return E_FAIL;
        }
	  }
    }
    m_pGraphBuilder->AddFilter(m_SourceF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
	IFileSourceFilter *pFS = NULL;
	m_SourceF->QueryInterface(IID_IFileSourceFilter, (void**)&pFS);
    CStdStringW strFileW;
    g_charsetConverter.subtitleCharsetToW(pFileItem.GetAsUrl().GetFileName(),strFileW);
    hr = pFS->Load(strFileW.c_str(), NULL);
    //If the source filter is the splitter set the splitter the same as source
	if (DShowUtil::IsSplitter(m_SourceF,false))
	{
      m_SplitterF = m_SourceF;
	}
	return hr;
  }
  else
  {
    if(m_File.Open(pFileItem.GetAsUrl().GetFileName().c_str(), READ_TRUNCATED | READ_BUFFERED))
    {
    CXBMCFileStream* pXBMCStream = new CXBMCFileStream(&m_File);
    CXBMCFileReader* pXBMCReader = new CXBMCFileReader(pXBMCStream, NULL, &hr);
    if (!pXBMCReader)
      CLog::Log(LOGERROR,"%s Failed Loading XBMC File Source filter",__FUNCTION__);
    m_SourceF = pXBMCReader;
    m_pGraphBuilder->AddFilter(m_SourceF, L"XBMC File Source");
    return hr;
    }
    return E_FAIL;
  }
}
HRESULT CFGLoader::InsertSplitter(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  POSITION pos = m_configFilter.GetHeadPosition();
  m_SplitterF = NULL;
  CFGFilterFile* pFGF;
  while(pos)
  {
    pFGF = m_configFilter.GetNext(pos);
	if ( ( (CStdString)pRule->Attribute("splitter")).Equals(pFGF->GetXFilterName().c_str(),false ) )
    {
      if(SUCCEEDED(pFGF->Create(&m_SplitterF, pUnk)))
        break;
	  else
	  {
        CLog::Log(LOGERROR,"DSPlayer %s Failed to create spliter",__FUNCTION__);
        return E_FAIL;
	  }
	}
  }

  m_pGraphBuilder->AddFilter(m_SplitterF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
  hr = ::ConnectFilters(m_pGraphBuilder,m_SourceF,m_SplitterF);
  //If the splitter successully connected to the source
  if ( SUCCEEDED( hr ) )
    CLog::Log(LOGNOTICE,"DSPlayer %s Connected the source to the spillter",__FUNCTION__);
  else
  {
    CLog::Log(LOGERROR,"DSPlayer %s Failed to connect the source to the spliter",__FUNCTION__);
    return hr;
  }
  return hr;
}

HRESULT CFGLoader::InsertAudioDecoder(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  POSITION pos = m_configFilter.GetHeadPosition();
  CComPtr<IBaseFilter> ppBF;
  CFGFilterFile* pFGF;
  while(pos)
  {
    pFGF = m_configFilter.GetNext(pos);
    if ( ((CStdString)pRule->Attribute("audiodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
    {
      if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
        break;
	  else
	  {
        CLog::Log(LOGERROR,"DSPlayer %s Failed to create the audio decoder filter",__FUNCTION__);
        return E_FAIL;
      }
	}
  }
  m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
  ppBF.Release();
  CLog::Log(LOGDEBUG,"DSPlayer %s Sucessfully added the audio decoder filter",__FUNCTION__);
  return hr;
}

HRESULT CFGLoader::InsertVideoDecoder(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  POSITION pos = m_configFilter.GetHeadPosition();
  CComPtr<IBaseFilter> ppBF;
  CFGFilterFile* pFGF;
  while(pos)
  {
    pFGF = m_configFilter.GetNext(pos);
    if ( ((CStdString)pRule->Attribute("videodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
    {
      if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
        break;
	  else
	  {
        CLog::Log(LOGERROR,"DSPlayer %s Failed to create the video decoder filter",__FUNCTION__);
        return E_FAIL;
      }
    }
  }
  m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(pFGF->GetXFilterName().c_str()).c_str());
  ppBF.Release();
  CLog::Log(LOGDEBUG,"DSPlayer %s Sucessfully added the video decoder filter",__FUNCTION__);
  return hr;
}

HRESULT CFGLoader::LoadFilterRules(const CFileItem& pFileItem)
{
  HRESULT hr;
  //Load the rules from the xml
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(m_xbmcConfigFilePath))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  
  TiXmlElement *pRules = graphConfigRoot->FirstChildElement("rules");
  pRules = pRules->FirstChildElement("rule");
  while (pRules)
  {
    if (((CStdString)pRules->Attribute("filetypes")).Equals(pFileItem.GetAsUrl().GetFileType().c_str(),false))
    {
      if (FAILED(InsertSourceFilter(pFileItem,pRules)))
        hr = E_FAIL;
      if (!m_SplitterF)
        if (FAILED(InsertSplitter(pRules)))
          hr = E_FAIL;
      if (FAILED(InsertVideoDecoder(pRules)))
        hr = E_FAIL;
	  if (FAILED(InsertAudioDecoder(pRules)))
        hr = E_FAIL;
    }
    pRules = pRules->NextSiblingElement();
  }
  
  if ( m_SplitterF )
  {
  hr = m_pGraphBuilder->ConnectFilter(m_SplitterF,NULL);
  if (FAILED(hr))
    CLog::Log(LOGERROR,"DSPlayer %s Failed to connect every filter together",__FUNCTION__);
  else
    CLog::Log(LOGDEBUG,"DSPlayer %s Successfuly connected every filters",__FUNCTION__);
  }
  
  return hr;
}

HRESULT CFGLoader::LoadConfig(CStdString configFile)
{
  HRESULT hr = S_OK;
  m_xbmcConfigFilePath = configFile;
  m_xbmcPath.Replace("\\","\\\\");
  if (!CFile::Exists(configFile))
    return false;
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(configFile))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  CStdString strFPath;
  CStdString strFGuid;
  CStdString strFileType;
  CStdString strTrimedPath;
  CStdString strTmpFilterName;
  CStdString strTmpFilterType;
  CFGFilterFile* pFGF;
  TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  pFilters = pFilters->FirstChildElement("filter");
  while (pFilters)
  {
    strTmpFilterName = pFilters->Attribute("name");
    strTmpFilterType = pFilters->Attribute("type");
    if (XMLUtils::GetString(pFilters,"path",strFPath))
	{

		CStdString strPath2;
		strPath2.Format("special://xbmc/system/players/dsplayer/%s", strFPath.c_str());
		if (!CFile::Exists(strFPath) && CFile::Exists(strPath2))
		  strFPath = _P(strPath2);

		XMLUtils::GetString(pFilters,"guid",strFGuid);
		XMLUtils::GetString(pFilters,"filetype",strFileType);
		pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,L"",MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
		if (strTmpFilterName.Equals("mpcvideodec",false))
		  m_mpcVideoDecGuid = DShowUtil::GUIDFromCString(strFGuid);
	}
	else
	{
      XMLUtils::GetString(pFilters,"guid",strFGuid);
      XMLUtils::GetString(pFilters,"filetype",strFileType);
      
	  strFPath = DShowUtil::GetFilterPath(strFGuid);
	  pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,L"",MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	}
  if (pFGF)
    m_configFilter.AddTail(pFGF);

  pFilters = pFilters->NextSiblingElement();
  
  }
}
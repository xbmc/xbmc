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
#include "FileSystem/SpecialProtocol.h"
#include "XMLUtils.h"
#include "WINDirectShowEnumerator.h"
#include "GuiSettings.h"
#include "boost/foreach.hpp"

using namespace std;
CFGLoader::CFGLoader(IGraphBuilder2* gb)
:m_pGraphBuilder(gb)
{
}

CFGLoader::~CFGLoader()
{
  CAutoLock cAutoLock(this);
  while(!m_configFilter.empty()) 
    m_configFilter.pop_back();
}




HRESULT CFGLoader::InsertSourceFilter(const CFileItem& pFileItem, TiXmlElement *pRule)
{

  HRESULT hr = S_OK;
  bool failedWithFirstSourceFilter = false;
  CStdString pWinFilePath;
  bool pForceXbmcSourceFilter=false;
  pWinFilePath = pFileItem.m_strPath;

  if ((pWinFilePath.Left(6)).Equals("smb://",false))
	  pWinFilePath.Replace("smb://","\\\\");

  if ((pWinFilePath.Left(6)).Equals("rar://",false))
    pForceXbmcSourceFilter = true;

  pWinFilePath.Replace("/","\\");
  if ( (( (CStdString)pRule->Attribute("source")).length() > 0 ) && ( !pForceXbmcSourceFilter ))
  {
    CLog::Log(LOGNOTICE,"%s Starting this file with dsplayer \"%s\"",__FUNCTION__,pWinFilePath.c_str());
    
    BOOST_FOREACH(CFGFilterFile* pFGF,m_configFilter)
    {
      if ( ((CStdString)pRule->Attribute("source")).Equals(pFGF->GetXFilterName().c_str(),false) )
      {
        if(SUCCEEDED(pFGF->Create(&m_SourceF, pUnk)))
        {
          m_pGraphBuilder->AddFilter(m_SourceF,pFGF->GetName().c_str());
          g_charsetConverter.wToUTF8(pFGF->GetName(),m_pStrSource);
          break;
        }
	      else
	      {
        CLog::Log(LOGERROR,"DSPlayer %s Failed to create the source filter",__FUNCTION__);
        return E_FAIL;
        }
	    }
    
    }
    
	  IFileSourceFilter *pFS = NULL;
	  m_SourceF->QueryInterface(IID_IFileSourceFilter, (void**)&pFS);
    CStdStringW strFileW;
    g_charsetConverter.subtitleCharsetToW(pWinFilePath,strFileW);
    hr = pFS->Load(strFileW.c_str(), NULL);
    if (SUCCEEDED(hr))
    {
      failedWithFirstSourceFilter = true;
    //If the source filter is the splitter set the splitter the same as source
	    if (DShowUtil::IsSplitter(m_SourceF,false))
        m_SplitterF = m_SourceF;
      return hr;
    }
    
	  
  }
  
  if (!failedWithFirstSourceFilter)
  {
    CLog::Log(LOGNOTICE,"%s DSplayer: Inserting xbmc source filter for this file \"%s\"",__FUNCTION__,pWinFilePath.c_str());
	
    //if(m_File.Open(pFileItem.GetAsUrl().GetFileName().c_str(), READ_TRUNCATED | READ_BUFFERED))
    if(m_File.Open(pFileItem.m_strPath, READ_TRUNCATED | READ_BUFFERED))
    {
      CComPtr<IBaseFilter> pSrc;
      CXBMCFileStream* pXBMCStream = new CXBMCFileStream(&m_File,&pSrc,&hr);
      m_SourceF = pSrc;
      m_pGraphBuilder->AddFilter(m_SourceF, L"XBMC File Source");
      m_pStrSource = CStdString("XBMC File Source");
      return hr;
    }
    return hr;
  }
}
HRESULT CFGLoader::InsertSplitter(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  m_SplitterF = NULL;
  
  BOOST_FOREACH(CFGFilterFile* pFGF,m_configFilter)
  {
    if ( ( (CStdString)pRule->Attribute("splitter")).Equals(pFGF->GetXFilterName().c_str(),false ) )
    {
      if(SUCCEEDED(pFGF->Create(&m_SplitterF, pUnk)))
      {
        m_pGraphBuilder->AddFilter(m_SplitterF,pFGF->GetName().c_str());
        g_charsetConverter.wToUTF8(pFGF->GetName(),m_pStrSplitter);
        break;
      }else
	    {
        CLog::Log(LOGERROR,"DSPlayer %s Failed to create spliter",__FUNCTION__);
        return E_FAIL;
      }
    }
  }

  
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
  CComPtr<IBaseFilter> ppBF;
  
  BOOST_FOREACH(CFGFilterFile* pFGF, m_configFilter)
  {
    if ( ((CStdString)pRule->Attribute("audiodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
    {
      if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
      {
        m_pGraphBuilder->AddFilter(ppBF,pFGF->GetName().c_str());
        g_charsetConverter.wToUTF8(pFGF->GetName(),m_pStrAudiodec);
        break;
      }
	    else
	    {
        CLog::Log(LOGERROR,"DSPlayer %s Failed to create the audio decoder filter",__FUNCTION__);
        return E_FAIL;
      }
	  }
  }
  

  
  ppBF.Release();
  CLog::Log(LOGDEBUG,"DSPlayer %s Sucessfully added the audio decoder filter",__FUNCTION__);
  return hr;
}

HRESULT CFGLoader::InsertVideoDecoder(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  CComPtr<IBaseFilter> ppBF;
  BOOST_FOREACH(CFGFilterFile* pFGF, m_configFilter)
  {
    if ( ((CStdString)pRule->Attribute("videodec")).Equals(pFGF->GetXFilterName().c_str(),false) )
    {
      if(SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
      {
        m_pGraphBuilder->AddFilter(ppBF,pFGF->GetName().c_str());
        g_charsetConverter.wToUTF8(pFGF->GetName(),m_pStrVideodec);
        break;
      }
	    else
	    {
        CLog::Log(LOGERROR,"DSPlayer %s Failed to create the video decoder filter",__FUNCTION__);
        return E_FAIL;
      }
    }
  }
  
  ppBF.Release();
  CLog::Log(LOGDEBUG,"DSPlayer %s Sucessfully added the video decoder filter",__FUNCTION__);
  return hr;
}

HRESULT CFGLoader::InsertAudioRenderer()
{
  HRESULT hr = S_OK;
  CComPtr<IBaseFilter> ppBF;
  CFGFilterRegistry* pFGF;
  CStdString currentGuid,currentName;

  CDirectShowEnumerator p_dsound;
  std::vector<DSFilterInfo> deviceList = p_dsound.GetAudioRenderers();
  std::vector<DSFilterInfo>::const_iterator iter = deviceList.begin();
  //see if there a config first 
  for (int i=0; iter != deviceList.end(); i++)
  {
    DSFilterInfo dev = *iter;
    if (g_guiSettings.GetString("dsplayer.audiorenderer").Equals(dev.lpstrName))
    {
      currentGuid = dev.lpstrGuid;
      currentName = dev.lpstrName;
    }
    ++iter;
  }
  if (currentName.IsEmpty())
  {
    currentGuid = DShowUtil::CStringFromGUID(CLSID_DSoundRender);
    currentName.Format("Default DirectSound Device");
  }
  m_pStrAudioRenderer = currentName;
  pFGF = new CFGFilterRegistry(DShowUtil::GUIDFromCString(currentGuid));
  hr = pFGF->Create(&ppBF, pUnk);
  hr = m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(currentName));
  ppBF.Release();
  return hr;
}

HRESULT CFGLoader::InsertAutoLoad()
{
  HRESULT hr = S_OK;
  CComPtr<IBaseFilter> ppBF;
  BOOST_FOREACH(CFGFilterFile* pFGF, m_configFilter)
  { 
    if ( pFGF->GetAutoLoad() )
    {
      if (SUCCEEDED(pFGF->Create(&ppBF, pUnk)))
	    {
        m_pGraphBuilder->AddFilter(ppBF,pFGF->GetName().c_str());
        ppBF = NULL;
	    }
	    else
	    {
        CLog::Log(LOGDEBUG,"DSPlayer %s Failed to create the auto loading filter called %s",__FUNCTION__,pFGF->GetXFilterName().c_str());
      }
    }
  }  
  ppBF.Release();
  CLog::Log(LOGDEBUG,"DSPlayer %s Is done adding the autoloading filters",__FUNCTION__);
  return hr;
}

HRESULT CFGLoader::LoadFilterRules(const CFileItem& pFileItem)
{
  HRESULT hr;
  //Load the rules from the xml
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(m_xbmcConfigFilePath))
    return E_FAIL;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return E_FAIL;
  
  TiXmlElement *pRules = graphConfigRoot->FirstChildElement("rules");
  pRules = pRules->FirstChildElement("rule");
  m_SplitterF = NULL;
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
      
      if (FAILED(InsertAudioRenderer()))
      {
        //wont do shit if its not inserted or is inserted correctly
        hr = S_OK;
      }
      //AutoLoad is useless the only filters autoloading is the one that dont work with audio renderers 
      //if (FAILED(InsertAutoLoad()))
      //  hr = E_FAIL;
    }
    pRules = pRules->NextSiblingElement();
  }
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR,"DSPlayer %s Failed when inserting filters",__FUNCTION__);
    return hr;
  }
  if ( m_SplitterF )
  {
  hr = m_pGraphBuilder->ConnectFilter(m_SplitterF,NULL);
  if (FAILED(hr))
    CLog::Log(LOGERROR,"DSPlayer %s Failed to connect every filters together",__FUNCTION__);
  else
    CLog::Log(LOGDEBUG,"DSPlayer %s Successfuly connected every filters",__FUNCTION__);
  }
  
  return hr;
}

HRESULT CFGLoader::LoadConfig(CStdString configFile)
{
  HRESULT hr = S_OK;
  m_xbmcConfigFilePath = configFile;
  if (!CFile::Exists(configFile))
    return false;
  TiXmlDocument graphConfigXml;
  if (!graphConfigXml.LoadFile(configFile))
    return false;
  TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  if ( !graphConfigRoot)
    return false;
  CStdString strFPath, strFGuid, strFAutoLoad, strFileType;
  CStdString strTmpFilterName, strTmpFilterType,strTmpOsdName;
  CStdStringW strTmpOsdNameW;

  CFGFilterFile* pFGF;
  TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  pFilters = pFilters->FirstChildElement("filter");
  while (pFilters)
  {
    strTmpFilterName = pFilters->Attribute("name");
    strTmpFilterType = pFilters->Attribute("type");
    XMLUtils::GetString(pFilters,"osdname",strTmpOsdName);
    g_charsetConverter.subtitleCharsetToW(strTmpOsdName,strTmpOsdNameW);
    if (XMLUtils::GetString(pFilters,"path",strFPath))
	  {
		  CStdString strPath2;
		  strPath2.Format("special://xbmc/system/players/dsplayer/%s", strFPath.c_str());
		  if (!CFile::Exists(strFPath) && CFile::Exists(strPath2))
		    strFPath = _P(strPath2);

		  XMLUtils::GetString(pFilters,"guid",strFGuid);
		  XMLUtils::GetString(pFilters,"filetype",strFileType);
		  pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	  }
	  else
	  {
      XMLUtils::GetString(pFilters,"guid",strFGuid);
      XMLUtils::GetString(pFilters,"filetype",strFileType);
	    strFPath = DShowUtil::GetFilterPath(strFGuid);
	    pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	  }
    if (XMLUtils::GetString(pFilters,"alwaysload",strFAutoLoad))
	  {
      if ( ( strFAutoLoad.Equals("1",false) ) || ( strFAutoLoad.Equals("true",false) ) )
        pFGF->SetAutoLoad(true);
    }
 
    if (pFGF)
      m_configFilter.push_back(pFGF);

    pFilters = pFilters->NextSiblingElement();
  }//end while
}
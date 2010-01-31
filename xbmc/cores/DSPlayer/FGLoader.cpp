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
#include "filters/VMR9AllocatorPresenter.h"
#include "filters/EVRAllocatorPresenter.h"

#include <ks.h>
#include <Codecapi.h>

using namespace std;
CFGLoader::CFGLoader():
  m_pGraphBuilder(NULL),
  m_SourceF(NULL),
  m_SplitterF(NULL),
  m_pFGF(NULL)
{

}

CFGLoader::~CFGLoader()
{
  CAutoLock cAutoLock(this);
  while(!m_configFilter.empty())
  {
    if (m_configFilter.back())
      delete m_configFilter.back();
    m_configFilter.pop_back();
  }
  SAFE_DELETE(m_pFGF);
}




HRESULT CFGLoader::InsertSourceFilter(const CFileItem& pFileItem, TiXmlElement *pRule)
{

  HRESULT hr = S_OK;
  bool failedWithFirstSourceFilter = true;
  CStdString pWinFilePath;
  bool pForceXbmcSourceFilter = false;
  pWinFilePath = pFileItem.m_strPath;

  if ((pWinFilePath.Left(6)).Equals("smb://",false))
	  pWinFilePath.Replace("smb://","\\\\");

  if ((pWinFilePath.Left(6)).Equals("rar://",false))
    pForceXbmcSourceFilter = true;

  pWinFilePath.Replace("/","\\");
  if ( (( (CStdString)pRule->Attribute("source")).length() > 0 ) && ( !pForceXbmcSourceFilter ))
  {
    CLog::Log(LOGNOTICE, "%s Starting this file with dsplayer (%s)", __FUNCTION__, pWinFilePath.c_str());
    for (list<CFGFilterFile*>::iterator it = m_configFilter.begin(); it != m_configFilter.end(); it++)
    {
      if ( ((CStdString)pRule->Attribute("source")).Equals((*it)->GetXFilterName().c_str(),false) )
      {
        if (SUCCEEDED((*it)->Create(&m_SourceF)))
        {
          g_charsetConverter.wToUTF8((*it)->GetName(), m_pStrSource);

          if (SUCCEEDED(m_pGraphBuilder->AddFilter(m_SourceF,(*it)->GetName().c_str())))
            CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrSource.c_str());
          else
            CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, m_pStrSource.c_str());

          break;
        }
	      else
	      {
          CLog::Log(LOGERROR,"%s Failed to create the source filter", __FUNCTION__);
          return E_FAIL;
        }
      }
    }
    
	  IFileSourceFilter *pFS = NULL;
	  m_SourceF->QueryInterface(IID_IFileSourceFilter, (void**)&pFS);
    CStdStringW strFileW;
    
    g_charsetConverter.utf8ToW(pWinFilePath, strFileW);
    hr = pFS->Load(strFileW.c_str(), NULL);
    if (SUCCEEDED(hr))
    {
      failedWithFirstSourceFilter = false;
      
	    if (DShowUtil::IsSplitter(m_SourceF, false)) //If the source filter is the splitter set the splitter the same as source
        m_SplitterF = m_SourceF;

      m_SourceF->Release(); // AddRef added by IFilterGraph
      SAFE_RELEASE(pFS);
      return hr;
    } else {
      CLog::Log(LOGERROR, "%s Source filter \"%s\" can't open file \"%s\" (result: %X)", __FUNCTION__, m_pStrSource.c_str(), pWinFilePath.c_str(), hr);
    }
    SAFE_RELEASE(pFS);
  }
  
  if (failedWithFirstSourceFilter)
  {
    CLog::Log(LOGNOTICE,"%s Inserting xbmc source filter for this file \"%s\"", __FUNCTION__, pWinFilePath.c_str());
	
    //if(m_File.Open(pFileItem.GetAsUrl().GetFileName().c_str(), READ_TRUNCATED | READ_BUFFERED))
    if(m_File.Open(pFileItem.m_strPath, READ_TRUNCATED | READ_BUFFERED))
    {
      IBaseFilter* pSrc;
      CXBMCFileStream* pXBMCStream = new CXBMCFileStream(&m_File,&pSrc,&hr);
      m_pGraphBuilder->RemoveFilter(m_SourceF);

      m_SourceF = pSrc;
      if (SUCCEEDED(m_pGraphBuilder->AddFilter(m_SourceF, L"XBMC File Source")))
      {
        CLog::Log(LOGNOTICE, "%s Successfully added xbmc source filter to the graph", __FUNCTION__);
      } else {
        CLog::Log(LOGERROR, "%s Failted to add xbmc source filter to the graph", __FUNCTION__);
      }
      m_pStrSource = CStdString("XBMC File Source");
      return hr;
    }
    return hr;
  }

  return hr;
}
HRESULT CFGLoader::InsertSplitter(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  m_SplitterF = NULL;
  for (list<CFGFilterFile*>::iterator it = m_configFilter.begin(); it != m_configFilter.end(); it++)
  {
	  if ( ( (CStdString)pRule->Attribute("splitter")).Equals((*it)->GetXFilterName().c_str(),false ) )
    {
      if(SUCCEEDED((*it)->Create(&m_SplitterF)))
      {
        g_charsetConverter.wToUTF8((*it)->GetName(), m_pStrSplitter);
        if (SUCCEEDED(m_pGraphBuilder->AddFilter(m_SplitterF,(*it)->GetName().c_str())))
        {
          CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrSplitter.c_str());
        } else {
          CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, m_pStrSplitter.c_str());
        }
        break;
      }
	    else
	    {
        CLog::Log(LOGERROR,"%s Failed to create the spliter", __FUNCTION__);
        return E_FAIL;
	    }
	  }
  }

  
  hr = ::ConnectFilters(m_pGraphBuilder, m_SourceF, m_SplitterF);
  //If the splitter successully connected to the source
  if ( SUCCEEDED( hr ) )
    CLog::Log(LOGNOTICE, "%s Successfully connected the source to the spillter", __FUNCTION__);
  else
  {
    CLog::Log(LOGERROR, "%s Failed to connect the source to the spliter", __FUNCTION__);
    return hr;
  }
  return hr;
}

HRESULT CFGLoader::InsertAudioDecoder(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  IBaseFilter* ppBF;
  for (list<CFGFilterFile*>::iterator it = m_configFilter.begin(); it != m_configFilter.end(); it++)
  {
    if ( ((CStdString)pRule->Attribute("audiodec")).Equals((*it)->GetXFilterName().c_str(),false) )
    {
      g_charsetConverter.wToUTF8((*it)->GetName(), m_pStrAudiodec);
      if(SUCCEEDED((*it)->Create(&ppBF)))
      {
        if (SUCCEEDED(m_pGraphBuilder->AddFilter(ppBF,(*it)->GetName().c_str())))
        {
          CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrAudiodec.c_str());
        } else {
          CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, m_pStrAudiodec.c_str());
        }
        
        break;
      }
	    else
	    {
        CLog::Log(LOGERROR, "%s Failed to create the audio decoder filter", __FUNCTION__);
        return E_FAIL;
      }
	  }
  }
  

  
  SAFE_RELEASE(ppBF);
  return hr;
}

HRESULT CFGLoader::InsertVideoDecoder(TiXmlElement *pRule)
{
  HRESULT hr = S_OK;
  IBaseFilter* ppBF = NULL;
  for (list<CFGFilterFile*>::iterator it = m_configFilter.begin(); it != m_configFilter.end(); it++)
  {
    if ( ((CStdString)pRule->Attribute("videodec")).Equals((*it)->GetXFilterName().c_str(),false) )
    {
      g_charsetConverter.wToUTF8((*it)->GetName(), m_pStrVideodec);
      if(SUCCEEDED((*it)->Create(&ppBF)))
      {
        if (SUCCEEDED(m_pGraphBuilder->AddFilter(ppBF,(*it)->GetName().c_str())))
        {
          CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrVideodec.c_str());
        } else {
          CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, m_pStrVideodec.c_str());
        }        

        //// TEST //
        //BeginEnumPins(ppBF, pEP, pPin)
        //{
        //  ICodecAPI *codecApi;
        //  hr = pPin->QueryInterface(__uuidof(codecApi), (void **) &codecApi);

        //  if (SUCCEEDED(hr))
        //  {
        //    VARIANT value;
        //    hr = codecApi->GetValue(&CODECAPI_AVDecCommonInputFormat,  &value);
        //    VariantClear(&value);
        //    hr = codecApi->GetValue(&CODECAPI_AVDecCommonOutputFormat, &value);
        //    VariantClear(&value);
        //  }
        //  hr = S_OK;
        //}
        //EndEnumPins
        break;
      }
	    else
	    {
        CLog::Log(LOGERROR,"%s Failed to create video decoder filter \"%s\"", __FUNCTION__, m_pStrVideodec.c_str());
        m_pStrVideodec = "";
        return E_FAIL;
      }
    }
  }
  
  SAFE_RELEASE(ppBF);
  return hr;
}

HRESULT CFGLoader::InsertAudioRenderer()
{
  HRESULT hr = S_OK;
  IBaseFilter* ppBF;
  CFGFilterRegistry* pFGF;
  CStdString currentGuid,currentName;

  CDirectShowEnumerator p_dsound;
  std::vector<DSFilterInfo> deviceList = p_dsound.GetAudioRenderers();
  
  //see if there a config first 
  for (std::vector<DSFilterInfo>::const_iterator iter = deviceList.begin();
    iter != deviceList.end(); ++iter)
  {
    DSFilterInfo dev = *iter;
    if (g_guiSettings.GetString("dsplayer.audiorenderer").Equals(dev.lpstrName))
    {
      currentGuid = dev.lpstrGuid;
      currentName = dev.lpstrName;
      break;
    }
  }
  if (currentName.IsEmpty())
  {
    currentGuid = DShowUtil::CStringFromGUID(CLSID_DSoundRender);
    currentName.Format("Default DirectSound Device");
  }

  m_pStrAudioRenderer = currentName;
  pFGF = new CFGFilterRegistry(DShowUtil::GUIDFromCString(currentGuid));
  hr = pFGF->Create(&ppBF);
  hr = m_pGraphBuilder->AddFilter(ppBF,DShowUtil::AnsiToUTF16(currentName));

  if (SUCCEEDED(hr))
    CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, m_pStrAudioRenderer.c_str());
  else
    CLog::Log(LOGNOTICE, "%s Failed to add \"%s\" to the graph (result: %X)", __FUNCTION__, m_pStrAudioRenderer.c_str(), hr);

  SAFE_RELEASE(ppBF);
  return hr;
}

HRESULT CFGLoader::InsertVideoRenderer()
{
  HRESULT hr = S_OK;
  IBaseFilter* ppBF = NULL;
  
  if (g_sysinfo.IsVistaOrHigher())
  {
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_CurrentRenderer = DIRECTSHOW_RENDERER_VMR9;
    else 
      m_CurrentRenderer = DIRECTSHOW_RENDERER_EVR;
  }
  else
  {
    if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
      m_CurrentRenderer = DIRECTSHOW_RENDERER_EVR;
    else 
      m_CurrentRenderer = DIRECTSHOW_RENDERER_VMR9;
  }
    

  // Renderers
  if (m_CurrentRenderer == DIRECTSHOW_RENDERER_EVR)
    m_pFGF = new CFGFilterVideoRenderer(__uuidof(CEVRAllocatorPresenter), L"Xbmc EVR");
  else
    m_pFGF = new CFGFilterVideoRenderer(__uuidof(CVMR9AllocatorPresenter), L"Xbmc VMR9 (Renderless)");
  hr = m_pFGF->Create(&ppBF);
  hr = m_pGraphBuilder->AddFilter(ppBF, m_pFGF->GetName());
  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG, "%s Allocator presenter successfully added to the graph (Renderer: %d)",  __FUNCTION__, m_CurrentRenderer);
  } else {
    CLog::Log(LOGERROR, "%s Failed to add allocator presenter to the graph (hr = %X)", __FUNCTION__, hr);
  }
  SAFE_RELEASE(ppBF);
  return hr;
  
}

HRESULT CFGLoader::InsertAutoLoad()
{
  HRESULT hr = S_OK;
  IBaseFilter* ppBF;
  for (list<CFGFilterFile*>::iterator it = m_configFilter.begin(); it != m_configFilter.end(); it++)
  { 
    if ( (*it)->GetAutoLoad() )
    {
      if (SUCCEEDED((*it)->Create(&ppBF)))
	    {
        m_pGraphBuilder->AddFilter(ppBF,(*it)->GetName().c_str());
        ppBF = NULL;
	    }
	    else
	    {
        CLog::Log(LOGDEBUG,"DSPlayer %s Failed to create the auto loading filter called %s",__FUNCTION__,(*it)->GetXFilterName().c_str());
      }
    }
  }  
  SAFE_RELEASE(ppBF);
  CLog::Log(LOGDEBUG,"DSPlayer %s Is done adding the autoloading filters",__FUNCTION__);
  return hr;
}

HRESULT CFGLoader::LoadFilterRules(const CFileItem& pFileItem)
{
  HRESULT hr = E_FAIL;
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
      hr = S_OK;

      InsertVideoRenderer();

      if (FAILED(InsertSourceFilter(pFileItem, pRules)))
        hr = E_FAIL;

      if (!m_SplitterF)
      {
        if (FAILED(InsertSplitter(pRules)))
          hr = E_FAIL;
      } else {
        CLog::Log(LOGNOTICE, "%s The source filter is also the splitter filter", __FUNCTION__);
      }

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
      break;
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
    if (FAILED(hr))
      CLog::Log(LOGERROR,"DSPlayer %s Failed to connect every filters together",__FUNCTION__);
    else
      CLog::Log(LOGDEBUG,"DSPlayer %s Successfuly connected every filters",__FUNCTION__);
  }
  
  return hr;
}

HRESULT CFGLoader::LoadConfig(IFilterGraph2* fg,CStdString configFile)
{

  m_pGraphBuilder = fg;
  fg = NULL;
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
  bool p_bGotThisFilter;

  while (pFilters)
  {
    strTmpFilterName = pFilters->Attribute("name");
    strTmpFilterType = pFilters->Attribute("type");
    XMLUtils::GetString(pFilters,"osdname",strTmpOsdName);
    g_charsetConverter.subtitleCharsetToW(strTmpOsdName,strTmpOsdNameW);
    p_bGotThisFilter = false;
    //first look if we got the path in the config file
    if (XMLUtils::GetString(pFilters, "path" , strFPath))
	  {
		  CStdString strPath2;
      XMLUtils::GetString(pFilters,"guid",strFGuid);
		  XMLUtils::GetString(pFilters,"filetype",strFileType);
		  strPath2.Format("special://xbmc/system/players/dsplayer/%s", strFPath.c_str());

      //First verify if we have the full path or just the name of the file
		  if (!CFile::Exists(strFPath))
      {
        if (CFile::Exists(strPath2))
        {
		      strFPath = _P(strPath2);
          p_bGotThisFilter = true;
        }
      }
      else
        p_bGotThisFilter = true;      

		  if (p_bGotThisFilter)
		    pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	  }

	  if (! p_bGotThisFilter)
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

  }
  
  return true;
  //end while

  //m_pGraphBuilder = fg;
  //fg = NULL;
  //HRESULT hr = S_OK;
  //m_xbmcConfigFilePath = configFile;
  //if (!CFile::Exists(configFile))
  //  return false;
  //TiXmlDocument graphConfigXml;
  //if (!graphConfigXml.LoadFile(configFile))
  //  return false;
  //TiXmlElement* graphConfigRoot = graphConfigXml.RootElement();
  //if ( !graphConfigRoot)
  //  return false;
  //CStdString strFPath, strFGuid, strFAutoLoad, strFileType;
  //CStdString strTmpFilterName, strTmpFilterType,strTmpOsdName;
  //CStdStringW strTmpOsdNameW;

  //CFGFilterFile* pFGF;
  //TiXmlElement *pFilters = graphConfigRoot->FirstChildElement("filters");
  //pFilters = pFilters->FirstChildElement("filter");
  //while (pFilters)
  //{
  //  strTmpFilterName = pFilters->Attribute("name");
  //  strTmpFilterType = pFilters->Attribute("type");
  //  XMLUtils::GetString(pFilters,"osdname",strTmpOsdName);
  //  g_charsetConverter.subtitleCharsetToW(strTmpOsdName,strTmpOsdNameW);
  //  if (XMLUtils::GetString(pFilters,"path",strFPath) && !strFPath.empty())
	 // {
		//  CStdString strPath2;
		//  strPath2.Format("special://xbmc/system/players/dsplayer/%s", strFPath.c_str());
		//  if (!CFile::Exists(strFPath) && CFile::Exists(strPath2))
		//    strFPath = _P(strPath2);

		//  XMLUtils::GetString(pFilters,"guid",strFGuid);
		//  XMLUtils::GetString(pFilters,"filetype",strFileType);
		//  pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	 // }
	 // else
	 // {
  //    XMLUtils::GetString(pFilters,"guid",strFGuid);
  //    XMLUtils::GetString(pFilters,"filetype",strFileType);
	 //   strFPath = DShowUtil::GetFilterPath(strFGuid);
	 //   pFGF = new CFGFilterFile(DShowUtil::GUIDFromCString(strFGuid),strFPath,strTmpOsdNameW.c_str(),MERIT64_ABOVE_DSHOW+2,strTmpFilterName,strFileType);
	 // }
  //  if (XMLUtils::GetString(pFilters,"alwaysload",strFAutoLoad))
	 // {
  //    if ( ( strFAutoLoad.Equals("1",false) ) || ( strFAutoLoad.Equals("true",false) ) )
  //      pFGF->SetAutoLoad(true);
  //  }
 
  //  if (pFGF)
  //    m_configFilter.push_back(pFGF);

  //  pFilters = pFilters->NextSiblingElement();
  //}//end while
}
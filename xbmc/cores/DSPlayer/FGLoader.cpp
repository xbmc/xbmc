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
#include "DSPlayer.h"
#include "streamsmanager.h"
#include "dshowutil/dshowutil.h"
#include "DShowUtil/smartptr.h"

#include "charsetconverter.h"
#include "Log.h"
#include "GUIDialogOK.h"
#include "GUIWindowManager.h"
#include "filtercorefactory/filtercorefactory.h"
#include "Settings.h"
#include "GUISettings.h"
#include "utils/SystemInfo.h"

#include "filters/XBMCFileSource.h"
#include "filters/Splitters/AviSplitter.h"
#include "filters/DsVideoDecoder/XBMCVideoDecFilter.h"
#include "filters/VMR9AllocatorPresenter.h"
#include "filters/EVRAllocatorPresenter.h"

#include "WINDirectShowEnumerator.h"

using namespace std;

DIRECTSHOW_RENDERER CFGLoader::m_CurrentRenderer = DIRECTSHOW_RENDERER_UNDEF;

CFGLoader::CFGLoader():
  m_pFGF(NULL)
{
  Filters.Clear();
  m_UsingDXVADecoder = false;
}

CFGLoader::~CFGLoader()
{
  CSingleLock lock(*this);
  
  CFilterCoreFactory::Destroy();
  SAFE_DELETE(m_pFGF);

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);
}

HRESULT CFGLoader::InsertSourceFilter(const CFileItem& pFileItem, const CStdString& filterName)
{

  HRESULT hr = E_FAIL;
  
#if 0
  //Keep that stuff ill remove it when im done with sources filters Ti-BEN
  if (0)
  {
    Com::SmartQIPtr<IFileSourceFilter> pBFSrc;
    Com::SmartPtr<IBaseFilter> pBFF = DNew CMatroskaSourceFilter(NULL, &hr);
    hr = pBFF->QueryInterface(IID_IFileSourceFilter,(void**)&pBFSrc);
    if (SUCCEEDED(hr))
      hr = pBFSrc->Load(DShowUtil::AToW(pFileItem.m_strPath).c_str(), NULL);
    if (SUCCEEDED(hr))
    {
      
      Filters.Source.pBF = pBFF.Detach();
      g_dsGraph->pFilterGraph->AddFilter(Filters.Source.pBF, L"XBMC File Source");
      Filters.Splitter.osdname = "XBMC File Source";
    }
    return hr;
  }

  if (0)
  {
    Com::SmartQIPtr<IFileSourceFilter> pBFSrc;
    Com::SmartPtr<IBaseFilter> pBFF = DNew CAviSplitterFilter(NULL, &hr);
    pBFSrc = pBFF;
    if (SUCCEEDED(hr))
      hr = pBFSrc->Load(DShowUtil::AToW(pFileItem.m_strPath).c_str(), NULL);
    if (SUCCEEDED(hr))
    {
      
      Filters.Source.pBF = pBFF.Detach();
      g_dsGraph->pFilterGraph->AddFilter(Filters.Source.pBF, L"XBMC File Source");
      Filters.Source.osdname = "XBMC File Source";
    }
  }
  if (0)
  {
    //Adding the xbmc source filter
    CXBMCAsyncStream* pXBMCStream = new CXBMCAsyncStream(pFileItem.m_strPath, &Filters.Source.pBF, &hr);
    if (SUCCEEDED(hr))
    {
      hr = g_dsGraph->pFilterGraph->AddFilter(Filters.Source.pBF, L"XBMC File Source");
      if (FAILED(hr))
        return hr;
      Filters.Source.osdname = "XBMC File Source";
    }
    Filters.Splitter.pBF = DNew CMatroskaSplitterFilter(NULL, &hr);
    hr = g_dsGraph->pFilterGraph->AddFilter(Filters.Splitter.pBF, L"XBMC Avi Splitter");
    return hr;
  }
  #endif
  /* XBMC SOURCE FILTER  */
  if (CUtil::IsInArchive(pFileItem.m_strPath))
  {
    CLog::Log(LOGNOTICE,"%s File \"%s\" need a custom source filter", __FUNCTION__, pFileItem.m_strPath.c_str());
    CXBMCAsyncStream* pXBMCStream = new CXBMCAsyncStream(pFileItem.m_strPath, &Filters.Source.pBF, &hr);
    if (SUCCEEDED(hr))
    {
      hr = g_dsGraph->pFilterGraph->AddFilter(Filters.Source.pBF, L"XBMC Source Filter");
      if (FAILED(hr))
      {
        CLog::Log(LOGERROR, "%s Failed to add xbmc source filter to the graph", __FUNCTION__);
        return hr;
      }
      Filters.Source.osdname = "XBMC File Source";
      CLog::Log(LOGNOTICE, "%s Successfully added xbmc source filter to the graph", __FUNCTION__);
    }
    return hr;
  }
  /* DVD NAVIGATOR */
  if (pFileItem.IsDVDFile())
  {
    hr = InsertFilter(filterName, Filters.Splitter);
    if (SUCCEEDED(hr))
    {
      if(!((Filters.DVD.dvdControl = Filters.Splitter.pBF) && (Filters.DVD.dvdInfo = Filters.Splitter.pBF)))
      {
        Filters.DVD.Clear();
        return E_NOINTERFACE;
      }
    }

    Filters.isDVD = true;
    CStdString dirA;
    CStdStringW dirW;
    CUtil::GetDirectory(pFileItem.m_strPath, dirA);
    g_charsetConverter.utf8ToW(dirA, dirW);

    hr = Filters.DVD.dvdControl->SetDVDDirectory(dirW.c_str());
    if (FAILED(hr))
      CLog::Log(LOGERROR, "%s Failed loading dvd directory.", __FUNCTION__);

    Filters.DVD.dvdControl->SetOption(DVD_ResetOnStop, FALSE);
    Filters.DVD.dvdControl->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);
    
    return hr;
  }

  /* INTERNET STREAM */
  if (pFileItem.IsInternetStream())
  {
    //TODO
    //add IAMOpenProgress for requesting the status of stream without this interface the player failed if connection is too slow
    Com::SmartQIPtr<IFileSourceFilter> pSourceUrl;
    Com::SmartPtr<IUnknown> pUnk = NULL;

    pUnk.CoCreateInstance(CLSID_URLReader, NULL);
    hr = pUnk->QueryInterface(IID_IBaseFilter, (void**)&Filters.Source.pBF);
    if (SUCCEEDED(hr))
    {
      hr = g_dsGraph->pFilterGraph->AddFilter(Filters.Source.pBF, L"URLReader");
      Filters.Source.osdname = "URLReader";
      CStdStringW strUrlW; g_charsetConverter.utf8ToW(pFileItem.m_strPath, strUrlW);
      //hr = pUnk->QueryInterface(IID_IFileSourceFilter,(void**) &pSourceUrl);
      if (pSourceUrl = pUnk)
        hr = pSourceUrl->Load(strUrlW.c_str(), NULL);

      if(FAILED(hr))
      {
        g_dsGraph->pFilterGraph->RemoveFilter(Filters.Source.pBF);
        CLog::Log(LOGERROR, "%s Failed to add url source filter to the graph.", __FUNCTION__);
        Filters.Source.pBF = NULL;
      }
      else
      {
        CLog::Log(LOGNOTICE, "%s Successfully added url source filter to the graph", __FUNCTION__);
        return hr;
      }    
    }    
  }
/* This is also adding the splitter with the insert filter function */
  if (SUCCEEDED(hr = InsertFilter(filterName, Filters.Splitter)))
  {
    CStdString pWinFilePath = pFileItem.m_strPath;
    if ( (pWinFilePath.Left(6)).Equals("smb://", false) )
    pWinFilePath.Replace("smb://", "\\\\");
  
    pWinFilePath.Replace("/", "\\");

    Com::SmartQIPtr<IFileSourceFilter> pFS = Filters.Splitter.pBF;
    
    CStdStringW strFileW;  
    g_charsetConverter.utf8ToW(pWinFilePath, strFileW);

    if (SUCCEEDED(hr = pFS->Load(strFileW.c_str(), NULL)))
      CLog::Log(LOGNOTICE, "%s Successfully loaded file in the splitter", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Failed to load file in the splitter", __FUNCTION__);
  }


  return hr;  
}
HRESULT CFGLoader::InsertSplitter(const CFileItem& pFileItem, const CStdString& filterName)
{
  HRESULT hr = InsertFilter(filterName, Filters.Splitter);

  if (SUCCEEDED(hr))
  {
    if (SUCCEEDED(hr = ConnectFilters(g_dsGraph->pFilterGraph, Filters.Source.pBF, Filters.Splitter.pBF)))
      CLog::Log(LOGNOTICE, "%s Successfully connected the source to the spillter", __FUNCTION__);
    else
    {
      CLog::Log(LOGERROR, "%s Failed to connect the source to the spliter", __FUNCTION__);
      // What the point to provide filters customization if we let windows choose filters for us ?!
      /*CLog::Log(LOGNOTICE, "%s Trying to just render the source output pin", __FUNCTION__);
      BeginEnumPins(Filters.Source.pBF,pEP,pPin)
      {
        if (!DShowUtil::IsPinConnected(pPin))
        {
          hr = m_pGraphBuilder->Render(pPin);
        }
      }
      EndEnumPins
      if (FAILED(hr))
      {
        CLog::Log(LOGERROR, "%s Failed the just rendering the output pin", __FUNCTION__);
      }*/
    }
  }
  
  return hr;
}

HRESULT CFGLoader::InsertAudioRenderer(const CStdString& filterName)
{
  HRESULT hr = S_OK;
  CFGFilterRegistry* pFGF;
  CStdString currentGuid, currentName;

  if (! filterName.empty())
  {
    if (SUCCEEDED(InsertFilter(filterName, Filters.AudioRenderer)))
      return S_OK;
    else
      CLog::Log(LOGERROR, "%s Failed to insert custom audio renderer, fallback to default one", __FUNCTION__);
  }

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

  pFGF = new CFGFilterRegistry(DShowUtil::GUIDFromCString(currentGuid));
  hr = pFGF->Create(&Filters.AudioRenderer.pBF);
  delete pFGF;

  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Failed to create the audio renderer (%X)", __FUNCTION__, hr);
    return hr;
  }

  Filters.AudioRenderer.osdname = currentName;
  Filters.AudioRenderer.guid = DShowUtil::GUIDFromCString(currentGuid);

  hr = g_dsGraph->pFilterGraph->AddFilter(Filters.AudioRenderer.pBF, DShowUtil::AnsiToUTF16(currentName));

  if (SUCCEEDED(hr))
    CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, Filters.AudioRenderer.osdname.c_str());
  else
    CLog::Log(LOGNOTICE, "%s Failed to add \"%s\" to the graph (result: %X)", __FUNCTION__, Filters.AudioRenderer.osdname.c_str(), hr);

  return hr;
}

HRESULT CFGLoader::InsertVideoRenderer()
{
  HRESULT hr = S_OK;
  
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
  {
    m_pFGF = new CFGFilterVideoRenderer(CLSID_EVRAllocatorPresenter, L"Xbmc EVR");
    Filters.VideoRenderer.osdname = _T("Enhanced Video Renderer");
  }
  else
  {
    m_pFGF = new CFGFilterVideoRenderer(CLSID_VMR9AllocatorPresenter, L"Xbmc VMR9");
    Filters.VideoRenderer.osdname = _T("VMR9 (Renderless)");
  }

  
  hr = m_pFGF->Create( &Filters.VideoRenderer.pBF);
  if (FAILED(hr))
  {
    CLog::Log(LOGERROR, "%s Failed to create allocator presenter (hr = %X)", __FUNCTION__, hr);
    return hr;
  }
  hr = g_dsGraph->pFilterGraph->AddFilter(Filters.VideoRenderer.pBF, m_pFGF->GetName());

  /* Query IQualProp from the renderer */
  Filters.VideoRenderer.pBF->QueryInterface(IID_IQualProp, (void **) &Filters.VideoRenderer.pQualProp);

  if (SUCCEEDED(hr))
  {
    CLog::Log(LOGDEBUG, "%s Allocator presenter successfully added to the graph (Renderer: %s)",  __FUNCTION__, Filters.VideoRenderer.osdname.c_str());
  } else {
    CLog::Log(LOGERROR, "%s Failed to add allocator presenter to the graph (hr = %X)", __FUNCTION__, hr);
  }

  return hr; 
}

HRESULT CFGLoader::LoadFilterRules(const CFileItem& pFileItem)
{
   
  if (! CFilterCoreFactory::SomethingMatch(pFileItem))
  {

    CLog::Log(LOGERROR, "%s Extension \"%s\" not found. Please check dsfilterconfig.xml", __FUNCTION__, CURL(pFileItem.m_strPath).GetFileType().c_str());
    CGUIDialogOK *dialog = (CGUIDialogOK *)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dialog)
    {
      dialog->SetHeading("Extension not found");
      dialog->SetLine(0, "Impossible to play the media file : the media");
      dialog->SetLine(1, "extension \"" + CURL(pFileItem.m_strPath).GetFileType() + "\" isn't declared in dsfilterconfig.xml.");
      dialog->SetLine(2, "");
      CDSPlayer::errorWindow = dialog;
    }
    return E_FAIL;
  }

  CStdString filter = "";

  CFilterCoreFactory::GetAudioRendererFilter(pFileItem, filter);
  InsertAudioRenderer(filter); // First added, last connected
  InsertVideoRenderer();

  CHECK_HR_RETURN(CFilterCoreFactory::GetSourceFilter(pFileItem, filter),"Failed to get the source filter")
  
  CHECK_HR_RETURN(InsertSourceFilter(pFileItem, filter),"Failed to insert source filter")

  if (! Filters.Splitter.pBF)
  {
    if ( FAILED(CFilterCoreFactory::GetSplitterFilter(pFileItem, filter)) )
      return E_FAIL;
    
    if ( FAILED(InsertSplitter(pFileItem, filter)))
    {
      return E_FAIL;
    }
  }

  /* Ok, the splitter is added to the graph. We need to detect the video stream codec in order to choose
  if we use or not the dxva filter */

  Com::SmartPtr<IBaseFilter> pBF = Filters.Splitter.pBF;
  bool fourccfinded = false;
  BeginEnumPins(pBF, pEP, pPin)
  {
    if (fourccfinded)
      break;

    BeginEnumMediaTypes(pPin, pEMT, pMT)
    {
      if (pMT->majortype != MEDIATYPE_Video)
        break; // Next pin

      SVideoStreamInfos s;
      s.Clear();

      CStreamsManager::Get()->GetStreamInfos(pMT, &s);

      CLog::Log(LOGINFO, "Video stream fourcc : %c%c%c%c", s.fourcc >> 24 & 0xff, s.fourcc >> 16 & 0xff, s.fourcc >> 8 & 0xff, s.fourcc & 0xff);

      fourccfinded = true;
      // CCV1 is a fake fourcc code generated by Haali in order to trick the Microsoft h264
      // decoder included in Windows 7. ffdshow handle that fourcc well, but it may not be
      // the case for others decoders
      if (s.fourcc == 'H264' || s.fourcc == 'AVC1' || s.fourcc == 'WVC1'
        || s.fourcc == 'WMV3' || s.fourcc == 'WMVA' || s.fourcc == 'CCV1')
        m_UsingDXVADecoder = true;

      if (s.fourcc == 'CCV1')
      {
        CLog::Log(LOGINFO, "Haali Media Splitter is configured for using a fake fourcc code 'CCV1'. If you've got an error message, be sure your video decoder handle that fourcc well.");
      }

      break;
    }
    EndEnumMediaTypes(pMT)
  }
  EndEnumPins

  std::vector<CStdString> extras;
  if (FAILED(CFilterCoreFactory::GetExtraFilters(pFileItem, extras, m_UsingDXVADecoder)))
  {
    //Dont want the loading to fail for an error there
    CLog::Log(LOGERROR,"Failed loading extras filters in dxfilterconfig.xml");
  }
    

  // Insert extra first because first added, last connected!
  for (unsigned int i = 0; i < extras.size(); i++)
  {
    SFilterInfos f;
    if (SUCCEEDED(InsertFilter(extras[i], f)))
      Filters.Extras.push_back(f);

  }
  extras.clear();

  if ( FAILED(CFilterCoreFactory::GetVideoFilter(pFileItem, filter, m_UsingDXVADecoder)))
    return E_FAIL;

  if ( FAILED(InsertFilter(filter, Filters.Video)))
  {
    return E_FAIL;
  }

  if ( FAILED(CFilterCoreFactory::GetAudioFilter(pFileItem, filter, m_UsingDXVADecoder)))
    return E_FAIL;

  if ( FAILED(InsertFilter(filter, Filters.Audio)))
  {
    return E_FAIL;
  }

  CLog::Log(LOGDEBUG,"%s All filters added to the graph", __FUNCTION__);
 
  return S_OK;
}

HRESULT CFGLoader::LoadConfig()
{
  LoadFilterCoreFactorySettings(g_settings.GetUserDataItem("dsfilterconfig.xml"), true);
  LoadFilterCoreFactorySettings("special://xbmc/system/players/dsplayer/dsfilterconfig.xml", false);

  return S_OK;
}

HRESULT CFGLoader::InsertFilter(const CStdString& filterName, SFilterInfos& f)
{
  HRESULT hr = S_OK;
  f.pBF = NULL;

  CFGFilterFile *filter = NULL;
  //TODO Add an option to the gui for forcing internal filters when supported
#if 0
  if (filterName.Equals("mpcvideodec"))
  {
    Com::SmartPtr<IBaseFilter> pBF = new CXBMCVideoDecFilter(NULL, &hr);
    Filters.Video.pBF = pBF.Detach();
    hr = g_dsGraph->pFilterGraph->AddFilter(Filters.Video.pBF, L"Internal MpcVideoDec");
    Filters.Video.osdname = "Internal MpcVideoDec";
    return hr;
  }
#endif
  if (! (filter = CFilterCoreFactory::GetFilterFromName(filterName)))
    return E_FAIL;

  if(SUCCEEDED(hr = filter->Create(&f.pBF)))
  {
    g_charsetConverter.wToUTF8(filter->GetName(), f.osdname);
    if (SUCCEEDED(hr = g_dsGraph->pFilterGraph->AddFilter(f.pBF, filter->GetName().c_str())))
      CLog::Log(LOGNOTICE, "%s Successfully added \"%s\" to the graph", __FUNCTION__, f.osdname.c_str());
    else
      CLog::Log(LOGERROR, "%s Failed to add \"%s\" to the graph", __FUNCTION__, f.osdname.c_str());

    f.guid = filter->GetCLSID();
  } 
  else
  {
    CLog::Log(LOGERROR,"%s Failed to create filter \"%s\"", __FUNCTION__, filterName.c_str());
  }

  return hr;
}

bool CFGLoader::LoadFilterCoreFactorySettings( const CStdString& fileStr, bool clear )
{
  CLog::Log(LOGNOTICE, "Loading filter core factory settings from %s.", fileStr.c_str());
  if (!XFILE::CFile::Exists(fileStr))
  { // tell the user it doesn't exist
    CLog::Log(LOGNOTICE, "%s does not exist. Skipping.", fileStr.c_str());
    return false;
  }

  TiXmlDocument filterCoreFactoryXML;
  if (!filterCoreFactoryXML.LoadFile(fileStr))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", fileStr.c_str(), filterCoreFactoryXML.ErrorRow(), filterCoreFactoryXML.ErrorDesc());
    return false;
  }

  return (SUCCEEDED(CFilterCoreFactory::LoadConfiguration(filterCoreFactoryXML.RootElement(), clear)));
}

bool                      CFGLoader::m_UsingDXVADecoder = false;
SFilters                  CFGLoader::Filters;

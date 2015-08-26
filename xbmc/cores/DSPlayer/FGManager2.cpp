/*
 *  Copyright (C) 2010-2013 Eduard Kytmanov
 *  http://www.avmedia.su
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_DS_PLAYER

#include "FGManager2.h"
#include "DSPlayer.h"
#include "FGLoader.h"
#include "DVDFileInfo.h"
#include "utils/XMLUtils.h"
#include "settings/Settings.h"
#include "filtercorefactory/filtercorefactory.h"
#include "Filters/RendererSettings.h"
#include "video/VideoInfoTag.h"
#include "PixelShaderList.h"


HRESULT CFGManager2::RenderFileXbmc(const CFileItem& pFileItem)
{

  CFileItem FileItem = pFileItem;
  bool bIsAutoRender = CSettings::GetInstance().GetInt(CSettings::SETTING_DSPLAYER_FILTERSMANAGEMENT) == DSMERITS;

  if (FileItem.IsDVDFile() || !bIsAutoRender)
    return __super::RenderFileXbmc(FileItem);

  CSingleLock lock(*this);

  HRESULT hr = S_OK;

  // We *need* those informations for filter loading. If the user wants it, be sure it's loaded
  // before using it.
  bool hasStreamDetails = false;
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS) && FileItem.HasVideoInfoTag() && !FileItem.GetVideoInfoTag()->HasStreamDetails())
  {
    CLog::Log(LOGDEBUG, "%s - trying to extract filestream details from video file %s", __FUNCTION__, FileItem.GetPath().c_str());
    hasStreamDetails = CDVDFileInfo::GetFileStreamDetails(&FileItem);
  }
  else{
    hasStreamDetails = FileItem.HasVideoInfoTag() && FileItem.GetVideoInfoTag()->HasStreamDetails();
  }

  CURL url(FileItem.GetPath());

  CStdString pWinFilePath = url.Get();
  if ((pWinFilePath.Left(6)).Equals("smb://", false))
    pWinFilePath.Replace("smb://", "\\\\");

  if (!FileItem.IsInternetStream())
    pWinFilePath.Replace("/", "\\");

  Com::SmartPtr<IBaseFilter> pBF;
  CStdStringW strFileW;
  g_charsetConverter.utf8ToW(pWinFilePath, strFileW);
  if (FAILED(hr = g_dsGraph->pFilterGraph->AddSourceFilter(strFileW.c_str(), NULL, &pBF)))
  {
    return hr;
  }

  CStdString filter = "";

  START_PERFORMANCE_COUNTER
    CFilterCoreFactory::GetAudioRendererFilter(FileItem, filter);
  m_CfgLoader->InsertAudioRenderer(filter); // First added, last connected
  END_PERFORMANCE_COUNTER("Loading audio renderer");

  START_PERFORMANCE_COUNTER
    m_CfgLoader->InsertVideoRenderer();
  END_PERFORMANCE_COUNTER("Loading video renderer");

  START_PERFORMANCE_COUNTER
    hr = ConnectFilter(pBF, NULL);
  END_PERFORMANCE_COUNTER("Render filters");
  if (FAILED(hr))
    return hr;
  RemoveUnconnectedFilters(g_dsGraph->pFilterGraph);

#ifdef _DSPLAYER_DEBUG
  LogFilterGraph();
#endif

  if (!IsSplitter(pBF))
    CGraphFilters::Get()->Source.pBF = pBF;

  do{
    if (IsSplitter(pBF))
    {
      CGraphFilters::Get()->Splitter.pBF = pBF;
      break;
    }
    Com::SmartPtr<IBaseFilter> pNext;
    hr = GetNextFilter(pBF, DOWNSTREAM, &pNext);
    pBF = pNext;
  } while (hr == S_OK);

  // ASSIGN FILTER
  IDirectVobSub *pSub;
  ILAVAudioSettings *pAudioLav;
  IffdshowBaseW *pAudioFFDS;
  CGraphFilters::Get()->SetHasSubFilter(false);
  BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
  {
    if ((pBF == CGraphFilters::Get()->AudioRenderer.pBF && CGraphFilters::Get()->AudioRenderer.guid != CLSID_ReClock) || pBF == CGraphFilters::Get()->VideoRenderer.pBF)
      continue;

    hr = pBF->QueryInterface(__uuidof(pSub), (void **)&pSub);
    if (SUCCEEDED(hr))
    { 
      CGraphFilters::Get()->SetHasSubFilter(true);
      CGraphFilters::Get()->Subs.pBF = pBF;
    } 

    hr = pBF->QueryInterface(__uuidof(pAudioLav), (void **)&pAudioLav);
    if (SUCCEEDED(hr))
      CGraphFilters::Get()->Audio.pBF = pBF;

    hr = pBF->QueryInterface(IID_IffdshowDecAudioW, (void **)&pAudioFFDS);
    if (SUCCEEDED(hr))
      CGraphFilters::Get()->Audio.pBF = pBF;
  }
  EndEnumFilters

  // Init Streams manager, and load streams
  START_PERFORMANCE_COUNTER
    CStreamsManager::Create();
  CStreamsManager::Get()->InitManager();
  CStreamsManager::Get()->LoadStreams();
  END_PERFORMANCE_COUNTER("Loading streams informations");

  if (!hasStreamDetails) {
    if (CSettings::GetInstance().GetBool(CSettings::SETTING_MYVIDEOS_EXTRACTFLAGS)) // Only warn user if the option is enabled
      CLog::Log(LOGWARNING, __FUNCTION__" DVDPlayer failed to fetch streams details. Using DirectShow ones");

    FileItem.GetVideoInfoTag()->m_streamDetails.AddStream(new CDSStreamDetailVideo((const CDSStreamDetailVideo &)(*CStreamsManager::Get()->GetVideoStreamDetail())));

    std::vector<CDSStreamDetailAudio *>& streams = CStreamsManager::Get()->GetAudios();
    for (std::vector<CDSStreamDetailAudio *>::const_iterator it = streams.begin(); it != streams.end(); ++it)
      FileItem.GetVideoInfoTag()->m_streamDetails.AddStream(new CDSStreamDetailAudio((const CDSStreamDetailAudio &)(**it)));

    FileItem.GetVideoInfoTag()->m_streamDetails.Reset();
  }


  // Shaders
    {
      std::vector<uint32_t> shaders;
      std::vector<uint32_t> shadersStages;
      START_PERFORMANCE_COUNTER
        if (SUCCEEDED(CFilterCoreFactory::GetShaders(FileItem, shaders, shadersStages, CGraphFilters::Get()->IsUsingDXVADecoder())))
        {
        for (int i = 0; i < shaders.size(); i++)
        {
          g_dsSettings.pixelShaderList->EnableShader(shaders[i], shadersStages[i]);
        }
        }
      END_PERFORMANCE_COUNTER("Loading shaders");
    }

  CLog::Log(LOGDEBUG, "%s All filters added to the graph", __FUNCTION__);

  return S_OK;
}

#endif
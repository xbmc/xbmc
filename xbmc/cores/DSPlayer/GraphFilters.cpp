/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *		Copyright (C) 2010-2013 Eduard Kytmanov
 *		http://www.avmedia.su
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

#include "GraphFilters.h"
#include "settings/Settings.h"
#include "settings/MediaSettings.h"
#include "Filters/LavSettings.h"
#include "Filters/LAVAudioSettings.h"
#include "Filters/LAVVideoSettings.h"
#include "Filters/LAVSplitterSettings.h"
#include "DSPropertyPage.h"
#include "FGFilter.h"
#include "DSPlayerDatabase.h"
#include "filtercorefactory/filtercorefactory.h"
#include "Utils/DSFilterEnumerator.h"

CGraphFilters *CGraphFilters::m_pSingleton = NULL;

CGraphFilters::CGraphFilters() :
m_isDVD(false), m_UsingDXVADecoder(false), m_hsubfilter(false)
{
  m_isKodiRealFS = false;
  m_defaultRulePriority = "0";
  m_pD3DDevice = NULL;
}

CGraphFilters::~CGraphFilters()
{
  if (m_isKodiRealFS)
  {
    CSettings::GetInstance().SetBool(CSettings::SETTING_VIDEOSCREEN_FAKEFULLSCREEN, false);
    m_isKodiRealFS = false;
  }
}

CGraphFilters* CGraphFilters::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CGraphFilters());
}

void CGraphFilters::ShowInternalPPage(LAVFILTERS_TYPE type, bool showPropertyPage)
{
  m_pBF = NULL;

  IBaseFilter *pBF;
  GetInternalFilter(type, &pBF);

  // If there is not a playback create a filter to show propertypage
  if (pBF == NULL)
  {
    CreateInternalFilter(type, &m_pBF);
    pBF = m_pBF;
  }

  if (showPropertyPage)
  {
    CDSPropertyPage *pDSPropertyPage = DNew CDSPropertyPage(pBF, type);
    pDSPropertyPage->Initialize();
  }
  else
  {
    if (type == LAVVIDEO)
      g_windowManager.ActivateWindow(WINDOW_DIALOG_LAVVIDEO);
    if (type == LAVAUDIO)
      g_windowManager.ActivateWindow(WINDOW_DIALOG_LAVAUDIO);
    if (type == LAVSPLITTER)
      g_windowManager.ActivateWindow(WINDOW_DIALOG_LAVSPLITTER);
  }
}

bool CGraphFilters::ShowOSDPPage(IBaseFilter *pBF)
{
  if (Video.pBF == pBF && Video.internalFilter)
  {
    g_windowManager.ActivateWindow(WINDOW_DIALOG_LAVVIDEO);
    return true;
  }
  if (Audio.pBF == pBF && Audio.internalFilter)
  {
    g_windowManager.ActivateWindow(WINDOW_DIALOG_LAVAUDIO);
    return true;
  }
  if ((Source.pBF == pBF && Source.internalFilter) || (Splitter.pBF == pBF && Splitter.internalFilter))
  {
    g_windowManager.ActivateWindow(WINDOW_DIALOG_LAVSPLITTER);
    return true;
  }

  return false;
}

void CGraphFilters::CreateInternalFilter(LAVFILTERS_TYPE type, IBaseFilter **ppBF)
{
  std::string filterName;
  if (type == LAVVIDEO)
    filterName = "lavvideo_internal";
  if (type == LAVAUDIO)
    filterName = "lavaudio_internal";
  if (type == LAVSPLITTER)
    filterName = "lavsource_internal";
  if (type == XYSUBFILTER)
    CSettings::GetInstance().GetString(CSettings::SETTING_DSPLAYER_VIDEORENDERER) == "EVR" ? filterName = "xyvsfilter_internal" : filterName = "xysubfilter_internal";

  CFGLoader *pLoader = new CFGLoader();
  pLoader->LoadConfig();

  CFGFilter *pFilter = NULL;
  if (!(pFilter = CFilterCoreFactory::GetFilterFromName(filterName)))
    return;

  pFilter->Create(ppBF);

  // Init LavFilters settings
  SetupLavSettings(type, *ppBF);
}

void CGraphFilters::GetInternalFilter(LAVFILTERS_TYPE type, IBaseFilter **ppBF)
{
  *ppBF = m_pBF;

  if (type == LAVVIDEO && Video.pBF && Video.internalFilter)
    *ppBF = Video.pBF;

  if (type == LAVAUDIO && Audio.pBF && Audio.internalFilter)
    *ppBF = Audio.pBF;

  if (type == LAVSPLITTER && Splitter.pBF && Splitter.internalFilter)
    *ppBF = Splitter.pBF;

  if (type == LAVSPLITTER && Source.pBF && Source.internalFilter)
    *ppBF = Source.pBF;

  if (type == XYSUBFILTER && Subs.pBF && Subs.internalFilter)
    *ppBF = Subs.pBF;
}

LAVFILTERS_TYPE CGraphFilters::GetInternalType(IBaseFilter *pBF)
{
  if (Video.pBF == pBF && Video.internalFilter)
    return LAVVIDEO;
  if (Audio.pBF == pBF && Audio.internalFilter)
    return LAVAUDIO;
  if ((Source.pBF == pBF && Source.internalFilter) || (Splitter.pBF == pBF && Splitter.internalFilter))
    return LAVSPLITTER;
  if ((Subs.pBF == pBF && Subs.internalFilter))
    return XYSUBFILTER;

  return NOINTERNAL;
}

void CGraphFilters::SetupLavSettings(LAVFILTERS_TYPE type, IBaseFilter* pBF)
{
  if (type != LAVVIDEO && type != LAVAUDIO && type != LAVSPLITTER)
    return;

  // Set LavFilters in RunTimeConfig to have personal settings only for DSPlayer
  // this will reset LavFilters to default settings
  SetLavInternal(type, pBF);

  // Use LavFilters settings stored in DSPlayer DB if they are present
  if (LoadLavSettings(type))
  {
    SetLavSettings(type, pBF);
  }
  else
  {    
    // If DSPlayer DB it's empty load default LavFilters settings and then save into DB
    GetLavSettings(type, pBF);
    SaveLavSettings(type);  
  }
}

bool CGraphFilters::SetLavInternal(LAVFILTERS_TYPE type, IBaseFilter *pBF)
{
  if (type != LAVVIDEO && type != LAVAUDIO && type != LAVSPLITTER)
    return false;

  if (type == LAVVIDEO)
  {
    Com::SmartQIPtr<ILAVVideoSettings> pLAVVideoSettings = pBF;
    pLAVVideoSettings->SetRuntimeConfig(TRUE);
  }
  else if (type == LAVAUDIO)
  {
    Com::SmartQIPtr<ILAVAudioSettings> pLAVAudioSettings = pBF;
    pLAVAudioSettings->SetRuntimeConfig(TRUE);
  }
  else if (type == LAVSPLITTER)
  {
    Com::SmartQIPtr<ILAVFSettings> pLAVFSettings = pBF;
    pLAVFSettings->SetRuntimeConfig(TRUE);
  }

  return true;
}

bool CGraphFilters::GetLavSettings(LAVFILTERS_TYPE type, IBaseFilter* pBF)
{
  if (type != LAVVIDEO && type != LAVAUDIO && type != LAVSPLITTER)
    return false;

  if (type == LAVVIDEO)
  {
    Com::SmartQIPtr<ILAVVideoSettings> pLAVVideoSettings = pBF;

    CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();

    if (!pLAVVideoSettings)
      return false;

    lavSettings.video_bTrayIcon = pLAVVideoSettings->GetTrayIcon();
    lavSettings.video_dwStreamAR = pLAVVideoSettings->GetStreamAR();
    lavSettings.video_dwNumThreads = pLAVVideoSettings->GetNumThreads();
    lavSettings.video_dwDeintFieldOrder = pLAVVideoSettings->GetDeintFieldOrder();
    lavSettings.video_deintMode = pLAVVideoSettings->GetDeinterlacingMode();
    lavSettings.video_dwRGBRange = pLAVVideoSettings->GetRGBOutputRange();
    lavSettings.video_dwSWDeintMode = pLAVVideoSettings->GetSWDeintMode();
    lavSettings.video_dwSWDeintOutput = pLAVVideoSettings->GetSWDeintOutput();
    lavSettings.video_dwDitherMode = pLAVVideoSettings->GetDitherMode();
    for (int i = 0; i < LAVOutPixFmt_NB; ++i) {
      lavSettings.video_bPixFmts[i] = pLAVVideoSettings->GetPixelFormat((LAVOutPixFmts)i);
    }
    lavSettings.video_dwHWAccel = pLAVVideoSettings->GetHWAccel();
    for (int i = 0; i < HWCodec_NB; ++i) {
      lavSettings.video_bHWFormats[i] = pLAVVideoSettings->GetHWAccelCodec((LAVVideoHWCodec)i);
    }
    lavSettings.video_dwHWAccelResFlags = pLAVVideoSettings->GetHWAccelResolutionFlags();
    lavSettings.video_dwHWDeintMode = pLAVVideoSettings->GetHWAccelDeintMode();
    lavSettings.video_dwHWDeintOutput = pLAVVideoSettings->GetHWAccelDeintOutput();
    lavSettings.video_bHWDeintHQ = pLAVVideoSettings->GetHWAccelDeintHQ();
  } 
  if (type == LAVAUDIO)
  {
    Com::SmartQIPtr<ILAVAudioSettings> pLAVAudioSettings = pBF;

    CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();

    if (!pLAVAudioSettings)
      return false;

    lavSettings.audio_bTrayIcon = pLAVAudioSettings->GetTrayIcon();
    pLAVAudioSettings->GetDRC(&lavSettings.audio_bDRCEnabled, &lavSettings.audio_iDRCLevel);
    lavSettings.audio_bDTSHDFraming = pLAVAudioSettings->GetDTSHDFraming();
    lavSettings.audio_bAutoAVSync = pLAVAudioSettings->GetAutoAVSync();
    lavSettings.audio_bExpandMono = pLAVAudioSettings->GetExpandMono();
    lavSettings.audio_bExpand61 = pLAVAudioSettings->GetExpand61();
    lavSettings.audio_bOutputStandardLayout = pLAVAudioSettings->GetOutputStandardLayout();
    lavSettings.audio_b51Legacy = pLAVAudioSettings->GetOutput51LegacyLayout();
    lavSettings.audio_bMixingEnabled = pLAVAudioSettings->GetMixingEnabled();
    lavSettings.audio_dwMixingLayout = pLAVAudioSettings->GetMixingLayout();
    lavSettings.audio_dwMixingFlags = pLAVAudioSettings->GetMixingFlags();
    lavSettings.audio_dwMixingMode = pLAVAudioSettings->GetMixingMode();
    pLAVAudioSettings->GetMixingLevels(&lavSettings.audio_dwMixingCenterLevel, &lavSettings.audio_dwMixingSurroundLevel, &lavSettings.audio_dwMixingLFELevel);
    //pLAVAudioSettings->GetAudioDelay(&lavSettings.audio_bAudioDelayEnabled, &lavSettings.audio_iAudioDelay);

    for (int i = 0; i < Bitstream_NB; ++i) {
      lavSettings.audio_bBitstream[i] = pLAVAudioSettings->GetBitstreamConfig((LAVBitstreamCodec)i);
    }
    for (int i = 0; i < SampleFormat_Bitstream; ++i) {
      lavSettings.audio_bSampleFormats[i] = pLAVAudioSettings->GetSampleFormat((LAVAudioSampleFormat)i);
    }
    lavSettings.audio_bSampleConvertDither = pLAVAudioSettings->GetSampleConvertDithering();
  }
  if (type == LAVSPLITTER)
  {
    Com::SmartQIPtr<ILAVFSettings> pLAVFSettings = pBF;

    CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();

    if (!pLAVFSettings)
      return false;

    lavSettings.splitter_bTrayIcon = pLAVFSettings->GetTrayIcon();

    HRESULT hr;
    LPWSTR lpwstr = nullptr;
    hr = pLAVFSettings->GetPreferredLanguages(&lpwstr);
    if (SUCCEEDED(hr) && lpwstr) {
      lavSettings.splitter_prefAudioLangs = lpwstr;
      CoTaskMemFree(lpwstr);
    }
    lpwstr = nullptr;
    hr = pLAVFSettings->GetPreferredSubtitleLanguages(&lpwstr);
    if (SUCCEEDED(hr) && lpwstr) {
      lavSettings.splitter_prefSubLangs = lpwstr;
      CoTaskMemFree(lpwstr);
    }
    lpwstr = nullptr;
    hr = pLAVFSettings->GetAdvancedSubtitleConfig(&lpwstr);
    if (SUCCEEDED(hr) && lpwstr) {
      lavSettings.splitter_subtitleAdvanced = lpwstr;
      CoTaskMemFree(lpwstr);
    }

    lavSettings.splitter_subtitleMode = pLAVFSettings->GetSubtitleMode();
    lavSettings.splitter_bPGSForcedStream = pLAVFSettings->GetPGSForcedStream();
    lavSettings.splitter_bPGSOnlyForced = pLAVFSettings->GetPGSOnlyForced();
    lavSettings.splitter_iVC1Mode = pLAVFSettings->GetVC1TimestampMode();
    lavSettings.splitter_bSubstreams = pLAVFSettings->GetSubstreamsEnabled();
    lavSettings.splitter_bMatroskaExternalSegments = pLAVFSettings->GetLoadMatroskaExternalSegments();
    lavSettings.splitter_bStreamSwitchRemoveAudio = pLAVFSettings->GetStreamSwitchRemoveAudio();
    lavSettings.splitter_bImpairedAudio = pLAVFSettings->GetUseAudioForHearingVisuallyImpaired();
    lavSettings.splitter_bPreferHighQualityAudio = pLAVFSettings->GetPreferHighQualityAudioStreams();
    lavSettings.splitter_dwQueueMaxSize = pLAVFSettings->GetMaxQueueMemSize();
    lavSettings.splitter_dwQueueMaxPacketsSize = pLAVFSettings->GetMaxQueueSize();
    lavSettings.splitter_dwNetworkAnalysisDuration = pLAVFSettings->GetNetworkStreamAnalysisDuration();
  }

  return true;
}

bool CGraphFilters::SetLavSettings(LAVFILTERS_TYPE type, IBaseFilter* pBF)
{
  if (type != LAVVIDEO && type != LAVAUDIO && type != LAVSPLITTER)
    return false;

  if (type == LAVVIDEO)
  {
    Com::SmartQIPtr<ILAVVideoSettings> pLAVVideoSettings = pBF;

    CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();

    if (!pLAVVideoSettings)
      return false;

    pLAVVideoSettings->SetTrayIcon(lavSettings.video_bTrayIcon);
    pLAVVideoSettings->SetStreamAR(lavSettings.video_dwStreamAR);
    pLAVVideoSettings->SetNumThreads(lavSettings.video_dwNumThreads);
    pLAVVideoSettings->SetDeintFieldOrder((LAVDeintFieldOrder)lavSettings.video_dwDeintFieldOrder);
    pLAVVideoSettings->SetDeinterlacingMode(lavSettings.video_deintMode);
    pLAVVideoSettings->SetRGBOutputRange(lavSettings.video_dwRGBRange);
    pLAVVideoSettings->SetSWDeintMode((LAVSWDeintModes)lavSettings.video_dwSWDeintMode);
    pLAVVideoSettings->SetSWDeintOutput((LAVDeintOutput)lavSettings.video_dwSWDeintOutput);
    pLAVVideoSettings->SetDitherMode((LAVDitherMode)lavSettings.video_dwDitherMode);
    for (int i = 0; i < LAVOutPixFmt_NB; ++i) {
      pLAVVideoSettings->SetPixelFormat((LAVOutPixFmts)i, lavSettings.video_bPixFmts[i]);
    }
    pLAVVideoSettings->SetHWAccel((LAVHWAccel)lavSettings.video_dwHWAccel);
    for (int i = 0; i < HWCodec_NB; ++i) {
      pLAVVideoSettings->SetHWAccelCodec((LAVVideoHWCodec)i, lavSettings.video_bHWFormats[i]);
    }
    pLAVVideoSettings->SetHWAccelResolutionFlags(lavSettings.video_dwHWAccelResFlags);
    pLAVVideoSettings->SetHWAccelDeintMode((LAVHWDeintModes)lavSettings.video_dwHWDeintMode);
    pLAVVideoSettings->SetHWAccelDeintOutput((LAVDeintOutput)lavSettings.video_dwHWDeintOutput);
    pLAVVideoSettings->SetHWAccelDeintHQ(lavSettings.video_bHWDeintHQ);

    // Custom interface
    if (Com::SmartQIPtr<ILAVVideoSettingsDSPlayerCustom> pLAVFSettingsDSPlayerCustom = pLAVVideoSettings)
      pLAVFSettingsDSPlayerCustom->SetPropertyPageCallback(PropertyPageCallback);
  }
  if (type == LAVAUDIO)
  {
    Com::SmartQIPtr<ILAVAudioSettings> pLAVAudioSettings = pBF;

    CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();

    if (!pLAVAudioSettings)
      return false;

    pLAVAudioSettings->SetTrayIcon(lavSettings.audio_bTrayIcon);
    pLAVAudioSettings->SetDRC(lavSettings.audio_bDRCEnabled, lavSettings.audio_iDRCLevel);
    pLAVAudioSettings->SetDTSHDFraming(lavSettings.audio_bDTSHDFraming);
    pLAVAudioSettings->SetAutoAVSync(lavSettings.audio_bAutoAVSync);
    pLAVAudioSettings->SetExpandMono(lavSettings.audio_bExpandMono);
    pLAVAudioSettings->SetExpand61(lavSettings.audio_bExpand61);
    pLAVAudioSettings->SetOutputStandardLayout(lavSettings.audio_bOutputStandardLayout);
    pLAVAudioSettings->SetOutput51LegacyLayout(lavSettings.audio_b51Legacy);
    pLAVAudioSettings->SetMixingEnabled(lavSettings.audio_bMixingEnabled);
    pLAVAudioSettings->SetMixingLayout(lavSettings.audio_dwMixingLayout);
    pLAVAudioSettings->SetMixingFlags(lavSettings.audio_dwMixingFlags);
    pLAVAudioSettings->SetMixingMode((LAVAudioMixingMode)lavSettings.audio_dwMixingMode);
    pLAVAudioSettings->SetMixingLevels(lavSettings.audio_dwMixingCenterLevel, lavSettings.audio_dwMixingSurroundLevel, lavSettings.audio_dwMixingLFELevel);
    //pLAVAudioSettings->SetAudioDelay(lavSettings.audio_bAudioDelayEnabled, lavSettings.audio_iAudioDelay);
    for (int i = 0; i < Bitstream_NB; ++i) {
      pLAVAudioSettings->SetBitstreamConfig((LAVBitstreamCodec)i, lavSettings.audio_bBitstream[i]);
    }
    for (int i = 0; i < SampleFormat_Bitstream; ++i) {
      pLAVAudioSettings->SetSampleFormat((LAVAudioSampleFormat)i, lavSettings.audio_bSampleFormats[i]);
    }
    pLAVAudioSettings->SetSampleConvertDithering(lavSettings.audio_bSampleConvertDither);

    // The internal LAV Audio Decoder will not be registered to handle WMA formats
    // since the system decoder is preferred. However we can still enable those
    // formats internally so that they are used in low-merit mode.
    pLAVAudioSettings->SetFormatConfiguration(Codec_WMA2, TRUE);
    pLAVAudioSettings->SetFormatConfiguration(Codec_WMAPRO, TRUE);
    pLAVAudioSettings->SetFormatConfiguration(Codec_WMALL, TRUE);

    // Custom interface
    if (Com::SmartQIPtr<ILAVAudioSettingsDSPlayerCustom> pLAVFSettingsDSPlayerCustom = pLAVAudioSettings)
      pLAVFSettingsDSPlayerCustom->SetPropertyPageCallback(PropertyPageCallback);
  }
  if (type == LAVSPLITTER)
  {
    Com::SmartQIPtr<ILAVFSettings> pLAVFSettings = pBF;

    CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();

    if (!pLAVFSettings)
      return false;

    pLAVFSettings->SetTrayIcon(lavSettings.splitter_bTrayIcon);
    pLAVFSettings->SetPreferredLanguages(lavSettings.splitter_prefAudioLangs.c_str());
    pLAVFSettings->SetPreferredSubtitleLanguages(lavSettings.splitter_prefSubLangs.c_str());
    pLAVFSettings->SetAdvancedSubtitleConfig(lavSettings.splitter_subtitleAdvanced.c_str());
    pLAVFSettings->SetSubtitleMode(lavSettings.splitter_subtitleMode);
    pLAVFSettings->SetPGSForcedStream(lavSettings.splitter_bPGSForcedStream);
    pLAVFSettings->SetPGSOnlyForced(lavSettings.splitter_bPGSOnlyForced);
    pLAVFSettings->SetVC1TimestampMode(lavSettings.splitter_iVC1Mode);
    pLAVFSettings->SetSubstreamsEnabled(lavSettings.splitter_bSubstreams);
    pLAVFSettings->SetLoadMatroskaExternalSegments(lavSettings.splitter_bMatroskaExternalSegments);
    pLAVFSettings->SetStreamSwitchRemoveAudio(lavSettings.splitter_bStreamSwitchRemoveAudio);
    pLAVFSettings->SetUseAudioForHearingVisuallyImpaired(lavSettings.splitter_bImpairedAudio);
    pLAVFSettings->SetPreferHighQualityAudioStreams(lavSettings.splitter_bPreferHighQualityAudio);
    pLAVFSettings->SetMaxQueueMemSize(lavSettings.splitter_dwQueueMaxSize);
    pLAVFSettings->SetMaxQueueSize(lavSettings.splitter_dwQueueMaxPacketsSize);
    pLAVFSettings->SetNetworkStreamAnalysisDuration(lavSettings.splitter_dwNetworkAnalysisDuration);

    // Custom interface
    if (Com::SmartQIPtr<ILAVFSettingsDSPlayerCustom> pLAVFSettingsDSPlayerCustom = pLAVFSettings)
      pLAVFSettingsDSPlayerCustom->SetPropertyPageCallback(PropertyPageCallback);
  }
  return true;
}

bool CGraphFilters::SaveLavSettings(LAVFILTERS_TYPE type)
{
  if (type != LAVVIDEO && type != LAVAUDIO && type != LAVSPLITTER)
    return false;

  CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();

  CDSPlayerDatabase dsdbs;
  if (dsdbs.Open())
  {
    if (type == LAVVIDEO)
      dsdbs.SetLAVVideoSettings(lavSettings);
    if (type == LAVAUDIO)
      dsdbs.SetLAVAudioSettings(lavSettings);
    if (type == LAVSPLITTER)
      dsdbs.SetLAVSplitterSettings(lavSettings);
    dsdbs.Close();
  }

  return true;
}

bool CGraphFilters::LoadLavSettings(LAVFILTERS_TYPE type)
{
  if (type != LAVVIDEO && type != LAVAUDIO && type != LAVSPLITTER)
    return false;

  CLavSettings &lavSettings = CMediaSettings::GetInstance().GetCurrentLavSettings();
  bool result = false;
  CDSPlayerDatabase dsdbs;
  if (dsdbs.Open())
  {
    if (type == LAVVIDEO)
      result = dsdbs.GetLAVVideoSettings(lavSettings);
    if (type == LAVAUDIO)
      result = dsdbs.GetLAVAudioSettings(lavSettings);
    if (type == LAVSPLITTER)
      result = dsdbs.GetLAVSplitterSettings(lavSettings);

    dsdbs.Close();
  }
  return result;
}

void CGraphFilters::EraseLavSetting(LAVFILTERS_TYPE type)
{
  if (type != LAVVIDEO && type != LAVAUDIO && type != LAVSPLITTER)
    return;

  CDSPlayerDatabase dsdbs;
  if (dsdbs.Open())
  {
    if (type == LAVVIDEO)
      dsdbs.EraseLAVVideo();
    if (type == LAVAUDIO)
      dsdbs.EraseLAVAudio();
    if (type == LAVSPLITTER)
      dsdbs.EraseLAVSplitter();

    dsdbs.Close();
  }
}

HRESULT CGraphFilters::PropertyPageCallback(IUnknown* pBF)
{
  CDSPropertyPage *pDSPropertyPage = DNew CDSPropertyPage((IBaseFilter *)pBF);
  pDSPropertyPage->Initialize();

  return S_OK;
}

bool CGraphFilters::IsRegisteredFilter(const std::string filter)
{
  CDSFilterEnumerator p_dsfilter;
  std::vector<DSFiltersInfo> dsfilterList;
  p_dsfilter.GetDSFilters(dsfilterList);
  std::vector<DSFiltersInfo>::const_iterator iter = dsfilterList.begin();

  for (int i = 1; iter != dsfilterList.end(); i++)
  {
    DSFiltersInfo dev = *iter;
    if (dev.lpstrName == filter)
    {
      return true;
      break;
    }
    ++iter;
  }
  return false;
}

#endif
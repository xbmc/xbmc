/*
 *      Copyright (C) 2010-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ActiveAEDSPProcess.h"

#include <utility>

extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
}

#include "ActiveAEDSPMode.h"
#include "Application.h"
#include "cores/AudioEngine/AEResampleFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/IPlayer.h"
#include "settings/MediaSettings.h"
#include "utils/TimeUtils.h"

using namespace ADDON;
using namespace ActiveAE;

#define MIN_DSP_ARRAY_SIZE 4096

CActiveAEDSPProcess::CActiveAEDSPProcess(AE_DSP_STREAM_ID streamId)
 : m_streamId(streamId)
{
  m_channelLayoutIn         = 0;      /* Undefined input channel layout */
  m_channelLayoutOut        = 0;      /* Undefined output channel layout */
  m_streamTypeUsed          = AE_DSP_ASTREAM_INVALID;
  m_NewStreamType           = AE_DSP_ASTREAM_INVALID;
  m_NewMasterMode           = AE_DSP_MASTER_MODE_ID_INVALID;
  m_forceInit               = false;
  m_resamplerDSPProcessor   = NULL;
  m_convertInput            = NULL;
  m_convertOutput           = NULL;
  m_iLastProcessTime        = 0;

  /*!
   * Create predefined process arrays on every supported channel for audio dsp's.
   * All are set if used or not for safety reason and unsued ones can be used from
   * dsp addons as buffer arrays.
   * If a bigger size is neeeded it becomes reallocated during DSP processing.
   */
  m_processArraySize = MIN_DSP_ARRAY_SIZE;
  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
  {
    m_processArray[0][i] = (float*)calloc(m_processArraySize, sizeof(float));
    m_processArray[1][i] = (float*)calloc(m_processArraySize, sizeof(float));
  }
}

CActiveAEDSPProcess::~CActiveAEDSPProcess()
{
  ResetStreamFunctionsSelection();

  if (m_resamplerDSPProcessor)
  {
    delete m_resamplerDSPProcessor;
    m_resamplerDSPProcessor = NULL;
  }

  /* Clear the buffer arrays */
  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
  {
    if(m_processArray[0][i])
      free(m_processArray[0][i]);
    if(m_processArray[1][i])
      free(m_processArray[1][i]);
  }

  swr_free(&m_convertInput);
  swr_free(&m_convertOutput);
}

void CActiveAEDSPProcess::ResetStreamFunctionsSelection()
{
  m_NewMasterMode = AE_DSP_MASTER_MODE_ID_INVALID;
  m_NewStreamType = AE_DSP_ASTREAM_INVALID;
  m_addon_InputResample.Clear();
  m_addon_OutputResample.Clear();

  m_addons_InputProc.clear();
  m_addons_PreProc.clear();
  m_addons_MasterProc.clear();
  m_addons_PostProc.clear();
  m_usedMap.clear();
}

bool CActiveAEDSPProcess::Create(const AEAudioFormat &inputFormat, const AEAudioFormat &outputFormat, bool upmix, AEQuality quality, AE_DSP_STREAMTYPE iStreamType,
                                 enum AVMatrixEncoding matrix_encoding, enum AVAudioServiceType audio_service_type, int profile)
{
  m_inputFormat       = inputFormat;                        /*!< Input format of processed stream */
  m_outputFormat      = outputFormat;                       /*!< Output format of required stream (set from ADSP system on startup, to have ffmpeg compatible format */
  m_outputSamplerate  = m_inputFormat.m_sampleRate;         /*!< If no resampler addon is present output samplerate is the same as input */
  m_outputFrames      = m_inputFormat.m_frames;
  m_streamQuality     = quality;                            /*!< from KODI on settings selected resample quality, also passed to addons to support different quality */
  m_dataFormat        = AE_FMT_FLOAT;                       /*!< the base stream format, hard set to float */
  m_activeMode        = AE_DSP_MASTER_MODE_ID_PASSOVER;     /*!< Reset the pointer for m_MasterModes about active master process, set here during mode selection */
  m_ffMpegMatrixEncoding  = matrix_encoding;
  m_ffMpegAudioServiceType= audio_service_type;
  m_ffMpegProfile         = profile;

  CSingleLock lock(m_restartSection);

  CLog::Log(LOGDEBUG, "ActiveAE DSP - %s - Audio DSP processing id %d created:", __FUNCTION__, m_streamId);

  m_convertInput = swr_alloc_set_opts(m_convertInput,
                                      CAEUtil::GetAVChannelLayout(m_inputFormat.m_channelLayout),
                                      AV_SAMPLE_FMT_FLTP,
                                      m_inputFormat.m_sampleRate,
                                      CAEUtil::GetAVChannelLayout(m_inputFormat.m_channelLayout),
                                      CAEUtil::GetAVSampleFormat(m_inputFormat.m_dataFormat),
                                      m_inputFormat.m_sampleRate,
                                      0, NULL);
  if (m_convertInput == NULL)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - DSP input convert with data format '%s' not supported!", __FUNCTION__, CAEUtil::DataFormatToStr(inputFormat.m_dataFormat));
    return false;
  }

  if (swr_init(m_convertInput) < 0)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - DSP input convert failed", __FUNCTION__);
    return false;
  }

  m_convertOutput = swr_alloc_set_opts(m_convertOutput,
                                       CAEUtil::GetAVChannelLayout(m_outputFormat.m_channelLayout),
                                       CAEUtil::GetAVSampleFormat(m_outputFormat.m_dataFormat),
                                       m_outputFormat.m_sampleRate,
                                       CAEUtil::GetAVChannelLayout(m_outputFormat.m_channelLayout),
                                       AV_SAMPLE_FMT_FLTP,
                                       m_outputFormat.m_sampleRate,
                                       0, NULL);
  if (m_convertOutput == NULL)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - DSP output convert with data format '%s' not supported!", __FUNCTION__, CAEUtil::DataFormatToStr(outputFormat.m_dataFormat));
    return false;
  }

  if (swr_init(m_convertOutput) < 0)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - DSP output convert failed", __FUNCTION__);
    return false;
  }

  ResetStreamFunctionsSelection();

  CFileItem currentFile(g_application.CurrentFileItem());

  m_streamTypeDetected = DetectStreamType(&currentFile);

  if (iStreamType == AE_DSP_ASTREAM_AUTO)
    m_streamTypeUsed = m_streamTypeDetected;
  else if (iStreamType >= AE_DSP_ASTREAM_BASIC || iStreamType < AE_DSP_ASTREAM_AUTO)
    m_streamTypeUsed = iStreamType;
  else
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - Unknown audio stream type, falling back to basic", __FUNCTION__);
    m_streamTypeUsed = AE_DSP_ASTREAM_BASIC;
  }

  /*!
   * Set general stream information about the processed stream
   */

  if (g_application.m_pPlayer->GetAudioStreamCount() > 0)
  {
    int identifier = CMediaSettings::GetInstance().GetCurrentVideoSettings().m_AudioStream;
    if(identifier < 0)
      identifier = g_application.m_pPlayer->GetAudioStream();
    if (identifier < 0)
      identifier = 0;

    SPlayerAudioStreamInfo info;
    g_application.m_pPlayer->GetAudioStreamInfo(identifier, info);

    m_addonStreamProperties.strName       = info.name.c_str();
    m_addonStreamProperties.strLanguage   = info.language.c_str();
    m_addonStreamProperties.strCodecId    = info.audioCodecName.c_str();
    m_addonStreamProperties.iIdentifier   = identifier;
    m_addonStreamProperties.iSampleRate   = info.samplerate;
    m_addonStreamProperties.iChannels     = info.channels;
  }
  else
  {
    m_addonStreamProperties.strName       = "Unknown";
    m_addonStreamProperties.strLanguage   = "";
    m_addonStreamProperties.strCodecId    = "";
    m_addonStreamProperties.iIdentifier   = m_streamId;
    m_addonStreamProperties.iSampleRate   = m_inputFormat.m_sampleRate;
    m_addonStreamProperties.iChannels     = m_inputFormat.m_channelLayout.Count();
  }

  m_addonStreamProperties.iStreamID       = m_streamId;
  m_addonStreamProperties.iStreamType     = m_streamTypeUsed;
  m_addonStreamProperties.iBaseType       = GetBaseType(&m_addonStreamProperties);

  /*!
   * Create the profile about additional stream related data, e.g. the different Dolby Digital stream flags
   */
  CreateStreamProfile();

  /*!
   * Set exact input and output format settings
   */
  m_addonSettings.iStreamID               = m_streamId;
  m_addonSettings.iStreamType             = m_streamTypeUsed;
  m_addonSettings.lInChannelPresentFlags  = 0;                                    /*!< Reset input channel present flags, becomes set on next steps */
  m_addonSettings.iInChannels             = m_inputFormat.m_channelLayout.Count();/*!< The from stream given channel amount */
  m_addonSettings.iInFrames               = m_inputFormat.m_frames;               /*!< Input frames given */
  m_addonSettings.iInSamplerate           = m_inputFormat.m_sampleRate;           /*!< The basic input samplerate from stream source */
  m_addonSettings.iProcessFrames          = m_inputFormat.m_frames;               /*!< Default the same as input frames, if input resampler is present it becomes corrected */
  m_addonSettings.iProcessSamplerate      = m_inputFormat.m_sampleRate;           /*!< Default the same as input samplerate, if input resampler is present it becomes corrected */
  m_addonSettings.lOutChannelPresentFlags = 0;                                    /*!< Reset output channel present flags, becomes set on next steps */
  m_addonSettings.iOutChannels            = m_outputFormat.m_channelLayout.Count(); /*!< The for output required amount of channels */
  m_addonSettings.iOutFrames              = m_outputFormat.m_frames;              /*! Output frames requested */
  m_addonSettings.iOutSamplerate          = m_outputFormat.m_sampleRate;          /*!< The required sample rate for pass over resampling on ActiveAEResample */
  m_addonSettings.bStereoUpmix            = upmix;                                /*! Stereo upmix value given from KODI settings */
  m_addonSettings.bInputResamplingActive  = false;                                /*! Becomes true if input resampling is in use */
  m_addonSettings.iQualityLevel           = m_streamQuality;                      /*! Requested stream processing quality, is optional and can be from addon ignored */

  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_FL))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FL;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_FR))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FR;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_FC))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_LFE))  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_LFE;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_BL))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BL;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_BR))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BR;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_FLOC)) m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FLOC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_FROC)) m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FROC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_BC))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_SL))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_SL;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_SR))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_SR;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_TFL))  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TFL;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_TFR))  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TFR;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_TFC))  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TFC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_TC))   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_TBL))  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TBL;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_TBR))  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TBR;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_TBC))  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TBC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_BLOC)) m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BLOC;
  if (m_inputFormat.m_channelLayout.HasChannel(AE_CH_BROC)) m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BROC;

  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_FL))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FL;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_FR))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FR;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_FC))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_LFE))  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_LFE;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_BL))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BL;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_BR))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BR;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_FLOC)) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FLOC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_FROC)) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FROC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_BC))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_SL))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_SL;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_SR))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_SR;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_TFL))  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TFL;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_TFR))  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TFR;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_TFC))  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TFC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_TC))   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_TBL))  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TBL;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_TBR))  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TBR;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_TBC))  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TBC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_BLOC)) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BLOC;
  if (m_outputFormat.m_channelLayout.HasChannel(AE_CH_BROC)) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BROC;

  /*!
   * Setup off mode, used if dsp master processing is set off, required to have data
   * for stream information functions.
   */
  sDSPProcessHandle internalMode;
  internalMode.Clear();
  internalMode.iAddonModeNumber = AE_DSP_MASTER_MODE_ID_PASSOVER;
  internalMode.pMode            = CActiveAEDSPModePtr(new CActiveAEDSPMode(internalMode.iAddonModeNumber, (AE_DSP_BASETYPE)m_addonStreamProperties.iBaseType));
  internalMode.iLastTime        = 0;
  m_addons_MasterProc.push_back(internalMode);
  m_activeMode = AE_DSP_MASTER_MODE_ID_PASSOVER;

  if (upmix && m_addonSettings.iInChannels <= 2)
  {
    internalMode.Clear();
    internalMode.iAddonModeNumber = AE_DSP_MASTER_MODE_ID_INTERNAL_STEREO_UPMIX;
    internalMode.pMode            = CActiveAEDSPModePtr(new CActiveAEDSPMode(internalMode.iAddonModeNumber, (AE_DSP_BASETYPE)m_addonStreamProperties.iBaseType));
    internalMode.iLastTime        = 0;
    m_addons_MasterProc.push_back(internalMode);
  }

  /*!
   * Load all selected processing types, stored in a database and available from addons
   */
  AE_DSP_ADDONMAP addonMap;
  if (CServiceBroker::GetADSP().GetEnabledAudioDSPAddons(addonMap) > 0)
  {
    int foundInputResamplerId = -1; /*!< Used to prevent double call of StreamCreate if input stream resampling is together with outer processing types */

    /*!
     * First find input resample addon to become information about processing sample rate and
     * load one allowed before master processing & final resample addon
     */
    CLog::Log(LOGDEBUG, "  ---- DSP input resample addon ---");
    const AE_DSP_MODELIST listInputResample = CServiceBroker::GetADSP().GetAvailableModes(AE_DSP_MODE_TYPE_INPUT_RESAMPLE);
    if (listInputResample.empty())
      CLog::Log(LOGDEBUG, "  | - no input resample addon present or enabled");
    for (unsigned int i = 0; i < listInputResample.size(); ++i)
    {
      /// For resample only one call is allowed. Use first one and ignore everything else.
      CActiveAEDSPModePtr pMode = listInputResample[i].first;
      AE_DSP_ADDON        addon = listInputResample[i].second;
      if (!CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()) && addon->SupportsInputResample() && pMode->IsEnabled())
      {
        ADDON_HANDLE_STRUCT handle;
        AE_DSP_ERROR err = addon->StreamCreate(&m_addonSettings, &m_addonStreamProperties, &handle);
        if (err == AE_DSP_ERROR_NO_ERROR)
        {
          if (addon->StreamIsModeSupported(&handle, pMode->ModeType(), pMode->AddonModeNumber(), pMode->ModeID()))
          {
            int processSamplerate = addon->InputResampleSampleRate(&handle);
            if (processSamplerate == (int)m_inputFormat.m_sampleRate)
            {
              CLog::Log(LOGDEBUG, "  | - input resample addon %s ignored, input sample rate %i the same as process rate", addon->GetFriendlyName().c_str(), m_inputFormat.m_sampleRate);
            }
            else if (processSamplerate > 0)
            {
              CLog::Log(LOGDEBUG, "  | - %s with resampling from %i to %i", addon->GetAudioDSPName().c_str(), m_inputFormat.m_sampleRate, processSamplerate);

              m_outputSamplerate                      = processSamplerate;                  /*!< overwrite output sample rate with the new rate */
              m_outputFrames                          = (int) ceil((1.0 * processSamplerate) / m_addonSettings.iInSamplerate * m_addonSettings.iInFrames);
              m_addonSettings.iProcessSamplerate      = m_outputSamplerate;                 /*!< the processing sample rate required for all behind called processes */
              m_addonSettings.iProcessFrames          = m_outputFrames;
              m_addonSettings.bInputResamplingActive  = true;

              m_addon_InputResample.iAddonModeNumber  = pMode->AddonModeNumber();
              m_addon_InputResample.pMode             = pMode;
              m_addon_InputResample.pAddon            = addon;
              m_addon_InputResample.iLastTime         = 0;
            }
            else
            {
              CLog::Log(LOGERROR, "ActiveAE DSP - %s - input resample addon %s return invalid samplerate and becomes disabled", __FUNCTION__, addon->GetFriendlyName().c_str());
            }

            unsigned int id = addon->GetID();
            m_addon_Handles[id]   = handle;
            foundInputResamplerId = id;
            m_usedMap.insert(std::make_pair(id, addon));
          }
        }
        else if (err != AE_DSP_ERROR_IGNORE_ME)
          CLog::Log(LOGERROR, "ActiveAE DSP - %s - input resample addon creation failed on %s with %s", __FUNCTION__, addon->GetAudioDSPName().c_str(), CActiveAEDSPAddon::ToString(err));
        break;
      }
    }

    /*!
     * Now init all other dsp relavant addons
     */
    for (AE_DSP_ADDONMAP_ITR itr = addonMap.begin(); itr != addonMap.end(); ++itr)
    {
      AE_DSP_ADDON addon = itr->second;
      int id = addon->GetID();
      if (!CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()) && id != foundInputResamplerId)
      {
        ADDON_HANDLE_STRUCT handle;
        AE_DSP_ERROR err = addon->StreamCreate(&m_addonSettings, &m_addonStreamProperties, &handle);
        if (err == AE_DSP_ERROR_NO_ERROR)
        {
          m_addon_Handles[id] = handle;
          m_usedMap.insert(std::make_pair(id, addon));
        }
        else if (err == AE_DSP_ERROR_IGNORE_ME)
          continue;
        else
          CLog::Log(LOGERROR, "ActiveAE DSP - %s - addon creation failed on %s with %s", __FUNCTION__, addon->GetAudioDSPName().c_str(), CActiveAEDSPAddon::ToString(err));
      }
    }

    for (AE_DSP_ADDONMAP_ITR itr = m_usedMap.begin(); itr != m_usedMap.end(); ++itr)
    {
      AE_DSP_ADDON addon = itr->second;
      if (addon->SupportsInputInfoProcess())
      {
        sDSPProcessHandle modeHandle;
        modeHandle.pAddon = addon;
        modeHandle.handle = m_addon_Handles[addon->GetID()];
        m_addons_InputProc.push_back(modeHandle);
      }
    }

    /*!
     * Load all required pre process dsp addon functions
     */
    CLog::Log(LOGDEBUG, "  ---- DSP active pre process modes ---");
    const AE_DSP_MODELIST listPreProcess = CServiceBroker::GetADSP().GetAvailableModes(AE_DSP_MODE_TYPE_PRE_PROCESS);
    for (unsigned int i = 0; i < listPreProcess.size(); ++i)
    {
      CActiveAEDSPModePtr pMode = listPreProcess[i].first;
      AE_DSP_ADDON        addon = listPreProcess[i].second;
      int                    id = addon->GetID();

      if (m_usedMap.find(id) == m_usedMap.end())
        continue;
      if (!CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()) && addon->SupportsPreProcess() && pMode->IsEnabled() &&
          addon->StreamIsModeSupported(&m_addon_Handles[id], pMode->ModeType(), pMode->AddonModeNumber(), pMode->ModeID()))
      {
        CLog::Log(LOGDEBUG, "  | - %i - %s (%s)", i, pMode->AddonModeName().c_str(), addon->GetAudioDSPName().c_str());

        sDSPProcessHandle modeHandle;
        modeHandle.iAddonModeNumber = pMode->AddonModeNumber();
        modeHandle.pMode            = pMode;
        modeHandle.pAddon           = addon;
        modeHandle.iLastTime        = 0;
        modeHandle.handle           = m_addon_Handles[id];
        m_addons_PreProc.push_back(modeHandle);
      }
    }
    if (m_addons_PreProc.empty())
      CLog::Log(LOGDEBUG, "  | - no pre processing addon's present or enabled");

    /*!
     * Load all available master modes from addons and put together with database
     */
    CLog::Log(LOGDEBUG, "  ---- DSP active master process modes ---");
    const AE_DSP_MODELIST listMasterProcess = CServiceBroker::GetADSP().GetAvailableModes(AE_DSP_MODE_TYPE_MASTER_PROCESS);
    for (unsigned int i = 0; i < listMasterProcess.size(); ++i)
    {
      CActiveAEDSPModePtr pMode = listMasterProcess[i].first;
      AE_DSP_ADDON        addon = listMasterProcess[i].second;
      int                    id = addon->GetID();

      if (m_usedMap.find(id) == m_usedMap.end())
        continue;
      if (!CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()) && addon->SupportsMasterProcess() && pMode->IsEnabled() &&
          addon->StreamIsModeSupported(&m_addon_Handles[id], pMode->ModeType(), pMode->AddonModeNumber(), pMode->ModeID()))
      {
        CLog::Log(LOGDEBUG, "  | - %i - %s (%s)", i, pMode->AddonModeName().c_str(), addon->GetAudioDSPName().c_str());

        sDSPProcessHandle modeHandle;
        modeHandle.iAddonModeNumber = pMode->AddonModeNumber();
        modeHandle.pMode            = pMode;
        modeHandle.pAddon           = addon;
        modeHandle.iLastTime        = 0;
        modeHandle.handle           = m_addon_Handles[id];
        modeHandle.pMode->SetBaseType((AE_DSP_BASETYPE)m_addonStreamProperties.iBaseType);
        m_addons_MasterProc.push_back(modeHandle);
      }
    }
    if (m_addons_MasterProc.empty())
      CLog::Log(LOGDEBUG, "  | - no master processing addon's present or enabled");

    /*!
     * Get selected source for current input
     */
    int modeID = CMediaSettings::GetInstance().GetCurrentAudioSettings().m_MasterModes[m_addonStreamProperties.iStreamType][m_addonStreamProperties.iBaseType];
    if (modeID == AE_DSP_MASTER_MODE_ID_INVALID)
      modeID = AE_DSP_MASTER_MODE_ID_PASSOVER;

    for (unsigned int ptr = 0; ptr < m_addons_MasterProc.size(); ++ptr)
    {
      CActiveAEDSPModePtr mode = m_addons_MasterProc.at(ptr).pMode;
      if (mode->ModeID() == modeID)
      {
        m_activeMode = (int)ptr;
        CLog::Log(LOGDEBUG, "  | -- %s (selected)", mode->AddonModeName().c_str());
        break;
      }
    }

    /*!
     * Setup the one allowed master processing addon and inform about selected mode
     */
    m_activeModeOutChannels = -1;
    if (m_addons_MasterProc[m_activeMode].pAddon)
    {
      AE_DSP_ERROR err = m_addons_MasterProc[m_activeMode].pAddon->MasterProcessSetMode(&m_addons_MasterProc[m_activeMode].handle, m_addonStreamProperties.iStreamType, m_addons_MasterProc[m_activeMode].pMode->AddonModeNumber(), m_addons_MasterProc[m_activeMode].pMode->ModeID());
      if (err != AE_DSP_ERROR_NO_ERROR)
      {
        CLog::Log(LOGERROR, "ActiveAE DSP - %s - addon master mode selection failed on %s with Mode '%s' with %s",
                                __FUNCTION__,
                                m_addons_MasterProc[m_activeMode].pAddon->GetAudioDSPName().c_str(),
                                m_addons_MasterProc[m_activeMode].pMode->AddonModeName().c_str(),
                                CActiveAEDSPAddon::ToString(err));
        m_addons_MasterProc.erase(m_addons_MasterProc.begin()+m_activeMode);
        m_activeMode = AE_DSP_MASTER_MODE_ID_PASSOVER;
      }
      else
        m_activeModeOutChannels = m_addons_MasterProc[m_activeMode].pAddon->MasterProcessGetOutChannels(&m_addons_MasterProc[m_activeMode].handle, m_activeModeOutChannelsPresent);
    }
    else
    {
      CLog::Log(LOGDEBUG, "  | -- No master process selected!");
    }

    /*!
     * Load all required post process dsp addon functions
     */
    CLog::Log(LOGDEBUG, "  ---- DSP active post process modes ---");
    const AE_DSP_MODELIST listPostProcess = CServiceBroker::GetADSP().GetAvailableModes(AE_DSP_MODE_TYPE_POST_PROCESS);
    for (unsigned int i = 0; i < listPostProcess.size(); ++i)
    {
      CActiveAEDSPModePtr pMode = listPostProcess[i].first;
      AE_DSP_ADDON        addon = listPostProcess[i].second;
      int                    id = addon->GetID();

      if (m_usedMap.find(id) == m_usedMap.end())
        continue;

      if (!CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()) && addon->SupportsPostProcess() && pMode->IsEnabled() &&
          addon->StreamIsModeSupported(&m_addon_Handles[id], pMode->ModeType(), pMode->AddonModeNumber(), pMode->ModeID()))
      {
        CLog::Log(LOGDEBUG, "  | - %i - %s (%s)", i, pMode->AddonModeName().c_str(), addon->GetAudioDSPName().c_str());

        sDSPProcessHandle modeHandle;
        modeHandle.iAddonModeNumber = pMode->AddonModeNumber();
        modeHandle.pMode            = pMode;
        modeHandle.pAddon           = addon;
        modeHandle.iLastTime        = 0;
        modeHandle.handle           = m_addon_Handles[id];
        m_addons_PostProc.push_back(modeHandle);
      }
    }
    if (m_addons_PostProc.empty())
      CLog::Log(LOGDEBUG, "  | - no post processing addon's present or enabled");

    /*!
     * Load one allowed addon for resampling after final post processing
     */
    CLog::Log(LOGDEBUG, "  ---- DSP post resample addon ---");
    if (m_addonSettings.iProcessSamplerate != m_outputFormat.m_sampleRate)
    {
      const AE_DSP_MODELIST listOutputResample = CServiceBroker::GetADSP().GetAvailableModes(AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE);
      if (listOutputResample.empty())
        CLog::Log(LOGDEBUG, "  | - no final post resample addon present or enabled, becomes performed by KODI");
      for (unsigned int i = 0; i < listOutputResample.size(); ++i)
      {
        /// For resample only one call is allowed. Use first one and ignore everything else.
        CActiveAEDSPModePtr pMode = listOutputResample[i].first;
        AE_DSP_ADDON        addon = listOutputResample[i].second;
        int                    id = addon->GetID();

        if (m_usedMap.find(id) != m_usedMap.end() &&
            !CAddonMgr::GetInstance().IsAddonDisabled(addon->ID()) && addon->SupportsOutputResample() && pMode->IsEnabled() &&
            addon->StreamIsModeSupported(&m_addon_Handles[id], pMode->ModeType(), pMode->AddonModeNumber(), pMode->ModeID()))
        {
          int outSamplerate = addon->OutputResampleSampleRate(&m_addon_Handles[id]);
          if (outSamplerate > 0)
          {
            CLog::Log(LOGDEBUG, "  | - %s with resampling to %i", addon->GetAudioDSPName().c_str(), outSamplerate);

            m_outputSamplerate                      = outSamplerate;
            m_outputFrames                          = (int) ceil((1.0 * outSamplerate) / m_addonSettings.iProcessSamplerate * m_addonSettings.iProcessFrames);

            m_addon_OutputResample.iAddonModeNumber = pMode->AddonModeNumber();
            m_addon_OutputResample.pMode            = pMode;
            m_addon_OutputResample.pAddon           = addon;
            m_addon_OutputResample.iLastTime        = 0;
            m_addon_OutputResample.handle           = m_addon_Handles[id];
          }
          else
          {
            CLog::Log(LOGERROR, "ActiveAE DSP - %s - post resample addon %s return invalid samplerate and becomes disabled", __FUNCTION__, addon->GetFriendlyName().c_str());
          }
          break;
        }
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "  | - no final resampling needed, process and final samplerate the same");
    }
  }

  /*!
   * Initialize fallback matrix mixer
   */
  InitFFMpegDSPProcessor();

  if (CLog::GetLogLevel() == LOGDEBUG) // Speed improve
  {
    CLog::Log(LOGDEBUG, "  ----  Input stream  ----");
    CLog::Log(LOGDEBUG, "  | Identifier           : %d", m_addonStreamProperties.iIdentifier);
    CLog::Log(LOGDEBUG, "  | Stream Type          : %s", m_addonStreamProperties.iStreamType == AE_DSP_ASTREAM_BASIC   ? "Basic"   :
                                                         m_addonStreamProperties.iStreamType == AE_DSP_ASTREAM_MUSIC   ? "Music"   :
                                                         m_addonStreamProperties.iStreamType == AE_DSP_ASTREAM_MOVIE   ? "Movie"   :
                                                         m_addonStreamProperties.iStreamType == AE_DSP_ASTREAM_GAME    ? "Game"    :
                                                         m_addonStreamProperties.iStreamType == AE_DSP_ASTREAM_APP     ? "App"     :
                                                         m_addonStreamProperties.iStreamType == AE_DSP_ASTREAM_PHONE   ? "Phone"   :
                                                         m_addonStreamProperties.iStreamType == AE_DSP_ASTREAM_MESSAGE ? "Message" :
                                                         "Unknown");
    CLog::Log(LOGDEBUG, "  | Name                 : %s", m_addonStreamProperties.strName);
    CLog::Log(LOGDEBUG, "  | Language             : %s", m_addonStreamProperties.strLanguage);
    CLog::Log(LOGDEBUG, "  | Codec                : %s", m_addonStreamProperties.strCodecId);
    CLog::Log(LOGDEBUG, "  | Sample Rate          : %d", m_addonStreamProperties.iSampleRate);
    CLog::Log(LOGDEBUG, "  | Channels             : %d", m_addonStreamProperties.iChannels);
    CLog::Log(LOGDEBUG, "  ----  Input format  ----");
    CLog::Log(LOGDEBUG, "  | Sample Rate          : %d", m_addonSettings.iInSamplerate);
    CLog::Log(LOGDEBUG, "  | Sample Format        : %s", CAEUtil::DataFormatToStr(m_inputFormat.m_dataFormat));
    CLog::Log(LOGDEBUG, "  | Channel Count        : %d", m_inputFormat.m_channelLayout.Count());
    CLog::Log(LOGDEBUG, "  | Channel Layout       : %s", ((std::string)m_inputFormat.m_channelLayout).c_str());
    CLog::Log(LOGDEBUG, "  | Frames               : %d", m_addonSettings.iInFrames);
    CLog::Log(LOGDEBUG, "  ----  Process format ----");
    CLog::Log(LOGDEBUG, "  | Sample Rate          : %d", m_addonSettings.iProcessSamplerate);
    CLog::Log(LOGDEBUG, "  | Sample Format        : %s", "AE_FMT_FLOATP");
    CLog::Log(LOGDEBUG, "  | Frames               : %d", m_addonSettings.iProcessFrames);
    CLog::Log(LOGDEBUG, "  | Internal processing  : %s", m_resamplerDSPProcessor ? "yes" : "no");
    CLog::Log(LOGDEBUG, "  ----  Output format ----");
    CLog::Log(LOGDEBUG, "  | Sample Rate          : %d", m_outputSamplerate);
    CLog::Log(LOGDEBUG, "  | Sample Format        : %s", CAEUtil::DataFormatToStr(m_outputFormat.m_dataFormat));
    CLog::Log(LOGDEBUG, "  | Channel Count        : %d", m_outputFormat.m_channelLayout.Count());
    CLog::Log(LOGDEBUG, "  | Channel Layout       : %s", ((std::string)m_outputFormat.m_channelLayout).c_str());
    CLog::Log(LOGDEBUG, "  | Frames               : %d", m_outputFrames);
  }

  m_forceInit = true;
  return true;
}

void CActiveAEDSPProcess::InitFFMpegDSPProcessor()
{
  /*!
   * If ffmpeg resampler is already present delete it first to create it from new
   */
  if (m_resamplerDSPProcessor)
  {
    delete m_resamplerDSPProcessor;
    m_resamplerDSPProcessor = NULL;
  }

  /*!
   * if the amount of input channels is higher as output and the active master mode gives more channels out or if it is not set of it
   * a forced channel downmix becomes enabled.
   */
  bool upmix = m_addonSettings.bStereoUpmix && m_addons_MasterProc[m_activeMode].pMode->ModeID() == AE_DSP_MASTER_MODE_ID_INTERNAL_STEREO_UPMIX ? true : false;
  if (upmix || (m_addonSettings.iInChannels > m_addonSettings.iOutChannels && (m_activeModeOutChannels <= 0 || m_activeModeOutChannels > m_addonSettings.iOutChannels)))
  {
    m_resamplerDSPProcessor = CAEResampleFactory::Create();
    if (!m_resamplerDSPProcessor->Init(CAEUtil::GetAVChannelLayout(m_outputFormat.m_channelLayout),
                                m_outputFormat.m_channelLayout.Count(),
                                m_addonSettings.iProcessSamplerate,
                                AV_SAMPLE_FMT_FLTP, sizeof(float) << 3, 0,
                                CAEUtil::GetAVChannelLayout(m_inputFormat.m_channelLayout),
                                m_inputFormat.m_channelLayout.Count(),
                                m_addonSettings.iProcessSamplerate,
                                AV_SAMPLE_FMT_FLTP, sizeof(float) << 3, 0,
                                upmix,
                                true,
                                NULL,
                                m_streamQuality,
                                true))
    {
      delete m_resamplerDSPProcessor;
      m_resamplerDSPProcessor = NULL;

      CLog::Log(LOGERROR, "ActiveAE DSP - %s - Initialize of channel mixer failed", __FUNCTION__);
    }
  }
}

bool CActiveAEDSPProcess::CreateStreamProfile()
{
  bool ret = true;

  switch (m_addonStreamProperties.iBaseType)
  {
    case AE_DSP_ABASE_AC3:
    case AE_DSP_ABASE_EAC3:
    {
      unsigned int iProfile;
      switch (m_ffMpegMatrixEncoding)
      {
        case AV_MATRIX_ENCODING_DOLBY:
          iProfile = AE_DSP_PROFILE_DOLBY_SURROUND;
          break;
        case AV_MATRIX_ENCODING_DPLII:
          iProfile = AE_DSP_PROFILE_DOLBY_PLII;
          break;
        case AV_MATRIX_ENCODING_DPLIIX:
          iProfile = AE_DSP_PROFILE_DOLBY_PLIIX;
          break;
        case AV_MATRIX_ENCODING_DPLIIZ:
          iProfile = AE_DSP_PROFILE_DOLBY_PLIIZ;
          break;
        case AV_MATRIX_ENCODING_DOLBYEX:
          iProfile = AE_DSP_PROFILE_DOLBY_EX;
          break;
        case AV_MATRIX_ENCODING_DOLBYHEADPHONE:
          iProfile = AE_DSP_PROFILE_DOLBY_HEADPHONE;
          break;
        case AV_MATRIX_ENCODING_NONE:
        default:
          iProfile = AE_DSP_PROFILE_DOLBY_NONE;
          break;
      }

      unsigned int iServiceType;
      switch (m_ffMpegAudioServiceType)
      {
        case AV_AUDIO_SERVICE_TYPE_EFFECTS:
          iServiceType = AE_DSP_SERVICE_TYPE_EFFECTS;
          break;
        case AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED:
          iServiceType = AE_DSP_SERVICE_TYPE_VISUALLY_IMPAIRED;
          break;
        case AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED:
          iServiceType = AE_DSP_SERVICE_TYPE_HEARING_IMPAIRED;
          break;
        case AV_AUDIO_SERVICE_TYPE_DIALOGUE:
          iServiceType = AE_DSP_SERVICE_TYPE_DIALOGUE;
          break;
        case AV_AUDIO_SERVICE_TYPE_COMMENTARY:
          iServiceType = AE_DSP_SERVICE_TYPE_COMMENTARY;
          break;
        case AV_AUDIO_SERVICE_TYPE_EMERGENCY:
          iServiceType = AE_DSP_SERVICE_TYPE_EMERGENCY;
          break;
        case AV_AUDIO_SERVICE_TYPE_VOICE_OVER:
          iServiceType = AE_DSP_SERVICE_TYPE_VOICE_OVER;
          break;
        case AV_AUDIO_SERVICE_TYPE_KARAOKE:
          iServiceType = AE_DSP_SERVICE_TYPE_KARAOKE;
          break;
        case AV_AUDIO_SERVICE_TYPE_MAIN:
        default:
          iServiceType = AE_DSP_SERVICE_TYPE_MAIN;
          break;
      }
      m_addonStreamProperties.Profile.ac3_eac3.iProfile = iProfile;
      m_addonStreamProperties.Profile.ac3_eac3.iServiceType = iServiceType;
      break;
    }
    case AE_DSP_ABASE_DTS:
    case AE_DSP_ABASE_DTSHD_HRA:
    case AE_DSP_ABASE_DTSHD_MA:
    {

      unsigned int iProfile;
      switch (m_ffMpegProfile)
      {
        case FF_PROFILE_DTS_ES:
          iProfile = AE_DSP_PROFILE_DTS_ES;
          break;
        case FF_PROFILE_DTS_96_24:
          iProfile = AE_DSP_PROFILE_DTS_96_24;
          break;
        case FF_PROFILE_DTS_HD_HRA:
          iProfile = AE_DSP_PROFILE_DTS_HD_HRA;
          break;
        case FF_PROFILE_DTS_HD_MA:
          iProfile = AE_DSP_PROFILE_DTS_HD_MA;
          break;
        case FF_PROFILE_DTS:
        default:
          iProfile = AE_DSP_PROFILE_DTS;
          break;
      }

      m_addonStreamProperties.Profile.dts_dtshd.iProfile = iProfile;
      m_addonStreamProperties.Profile.dts_dtshd.bSurroundMatrix = m_ffMpegMatrixEncoding == AV_MATRIX_ENCODING_DOLBY ? true : false;
      break;
    }
    case AE_DSP_ABASE_TRUEHD:
    case AE_DSP_ABASE_MLP:
    {
      unsigned int iProfile;
      switch (m_ffMpegMatrixEncoding)
      {
        case AV_MATRIX_ENCODING_DOLBY:
          iProfile = AE_DSP_PROFILE_DOLBY_SURROUND;
          break;
        case AV_MATRIX_ENCODING_DPLII:
          iProfile = AE_DSP_PROFILE_DOLBY_PLII;
          break;
        case AV_MATRIX_ENCODING_DPLIIX:
          iProfile = AE_DSP_PROFILE_DOLBY_PLIIX;
          break;
        case AV_MATRIX_ENCODING_DPLIIZ:
          iProfile = AE_DSP_PROFILE_DOLBY_PLIIZ;
          break;
        case AV_MATRIX_ENCODING_DOLBYEX:
          iProfile = AE_DSP_PROFILE_DOLBY_EX;
          break;
        case AV_MATRIX_ENCODING_DOLBYHEADPHONE:
          iProfile = AE_DSP_PROFILE_DOLBY_HEADPHONE;
          break;
        case AV_MATRIX_ENCODING_NONE:
        default:
          iProfile = AE_DSP_PROFILE_DOLBY_NONE;
          break;
      }

      m_addonStreamProperties.Profile.mlp_truehd.iProfile = iProfile;
      break;
    }
    case AE_DSP_ABASE_FLAC:
      break;
    default:
      ret = false;
      break;
  }

  return ret;
}

void CActiveAEDSPProcess::Destroy()
{
  CSingleLock lock(m_restartSection);

  if (!CServiceBroker::GetADSP().IsActivated())
    return;

  for (AE_DSP_ADDONMAP_ITR itr = m_usedMap.begin(); itr != m_usedMap.end(); ++itr)
  {
    itr->second->StreamDestroy(&m_addon_Handles[itr->first]);
  }

  ResetStreamFunctionsSelection();
}

void CActiveAEDSPProcess::ForceReinit()
{
  CSingleLock lock(m_restartSection);
  m_forceInit = true;
}

AE_DSP_STREAMTYPE CActiveAEDSPProcess::DetectStreamType(const CFileItem *item)
{
  AE_DSP_STREAMTYPE detected = AE_DSP_ASTREAM_BASIC;
  if (item->HasMusicInfoTag())
    detected = AE_DSP_ASTREAM_MUSIC;
  else if (item->HasVideoInfoTag() || g_application.m_pPlayer->HasVideo())
    detected = AE_DSP_ASTREAM_MOVIE;
//    else if (item->HasVideoInfoTag())
//      detected = AE_DSP_ASTREAM_GAME;
//    else if (item->HasVideoInfoTag())
//      detected = AE_DSP_ASTREAM_APP;
//    else if (item->HasVideoInfoTag())
//      detected = AE_DSP_ASTREAM_MESSAGE;
//    else if (item->HasVideoInfoTag())
//      detected = AE_DSP_ASTREAM_PHONE;
  else
    detected = AE_DSP_ASTREAM_BASIC;

  return detected;
}

AE_DSP_STREAM_ID CActiveAEDSPProcess::GetStreamId() const
{
  return m_streamId;
}

unsigned int CActiveAEDSPProcess::GetInputChannels()
{
  return m_inputFormat.m_channelLayout.Count();
}

std::string CActiveAEDSPProcess::GetInputChannelNames()
{
  return m_inputFormat.m_channelLayout;
}

unsigned int CActiveAEDSPProcess::GetInputSamplerate()
{
  return m_inputFormat.m_sampleRate;
}

unsigned int CActiveAEDSPProcess::GetProcessSamplerate()
{
  return m_addonSettings.iProcessSamplerate;
}

unsigned int CActiveAEDSPProcess::GetOutputChannels()
{
  return m_outputFormat.m_channelLayout.Count();
}

std::string CActiveAEDSPProcess::GetOutputChannelNames()
{
  return m_outputFormat.m_channelLayout;
}

unsigned int CActiveAEDSPProcess::GetOutputSamplerate()
{
  return m_outputSamplerate;
}

unsigned int CActiveAEDSPProcess::GetOutputFrames()
{
  return m_outputFrames;
}

float CActiveAEDSPProcess::GetCPUUsage(void) const
{
  return m_fLastProcessUsage;
}

CAEChannelInfo CActiveAEDSPProcess::GetChannelLayout()
{
  return m_outputFormat.m_channelLayout;
}

AEDataFormat CActiveAEDSPProcess::GetDataFormat()
{
  return m_dataFormat;
}

AEAudioFormat CActiveAEDSPProcess::GetInputFormat()
{
  return m_inputFormat;
}

AE_DSP_STREAMTYPE CActiveAEDSPProcess::GetDetectedStreamType()
{
  return m_streamTypeDetected;
}

AE_DSP_STREAMTYPE CActiveAEDSPProcess::GetUsedStreamType()
{
  return m_streamTypeUsed;
}

AE_DSP_BASETYPE CActiveAEDSPProcess::GetBaseType(AE_DSP_STREAM_PROPERTIES *props)
{
  if (!strcmp(props->strCodecId, "ac3"))
    return AE_DSP_ABASE_AC3;
  else if (!strcmp(props->strCodecId, "eac3"))
    return AE_DSP_ABASE_EAC3;
  else if (!strcmp(props->strCodecId, "dca") || !strcmp(props->strCodecId, "dts"))
    return AE_DSP_ABASE_DTS;
  else if (!strcmp(props->strCodecId, "dtshd_hra"))
    return AE_DSP_ABASE_DTSHD_HRA;
  else if (!strcmp(props->strCodecId, "dtshd_ma"))
    return AE_DSP_ABASE_DTSHD_MA;
  else if (!strcmp(props->strCodecId, "truehd"))
    return AE_DSP_ABASE_TRUEHD;
  else if (!strcmp(props->strCodecId, "mlp"))
    return AE_DSP_ABASE_MLP;
  else if (!strcmp(props->strCodecId, "flac"))
    return AE_DSP_ABASE_FLAC;
  else if (props->iChannels > 2)
    return AE_DSP_ABASE_MULTICHANNEL;
  else if (props->iChannels == 2)
    return AE_DSP_ABASE_STEREO;
  else
    return AE_DSP_ABASE_MONO;
}

AE_DSP_BASETYPE CActiveAEDSPProcess::GetUsedBaseType()
{
  return GetBaseType(&m_addonStreamProperties);
}

bool CActiveAEDSPProcess::GetMasterModeStreamInfoString(std::string &strInfo)
{
  if (m_activeMode <= AE_DSP_MASTER_MODE_ID_PASSOVER)
  {
    strInfo = "";
    return true;
  }

  if (m_activeMode < 0 || !m_addons_MasterProc[m_activeMode].pAddon)
    return false;

  strInfo = m_addons_MasterProc[m_activeMode].pAddon->MasterProcessGetStreamInfoString(&m_addons_MasterProc[m_activeMode].handle);

  return true;
}

bool CActiveAEDSPProcess::GetMasterModeTypeInformation(AE_DSP_STREAMTYPE &streamTypeUsed, AE_DSP_BASETYPE &baseType, int &iModeID)
{
  streamTypeUsed  = (AE_DSP_STREAMTYPE)m_addonStreamProperties.iStreamType;

  if (m_activeMode < 0)
    return false;

  baseType = m_addons_MasterProc[m_activeMode].pMode->BaseType();
  iModeID  = m_addons_MasterProc[m_activeMode].pMode->ModeID();
  return true;
}

const char *CActiveAEDSPProcess::GetStreamTypeName(AE_DSP_STREAMTYPE iStreamType)
{
  return iStreamType == AE_DSP_ASTREAM_BASIC   ? "Basic"     :
         iStreamType == AE_DSP_ASTREAM_MUSIC   ? "Music"     :
         iStreamType == AE_DSP_ASTREAM_MOVIE   ? "Movie"     :
         iStreamType == AE_DSP_ASTREAM_GAME    ? "Game"      :
         iStreamType == AE_DSP_ASTREAM_APP     ? "App"       :
         iStreamType == AE_DSP_ASTREAM_PHONE   ? "Phone"     :
         iStreamType == AE_DSP_ASTREAM_MESSAGE ? "Message"   :
         iStreamType == AE_DSP_ASTREAM_AUTO    ? "Automatic" :
         "Unknown";
}

bool CActiveAEDSPProcess::MasterModeChange(int iModeID, AE_DSP_STREAMTYPE iStreamType)
{
  bool bReturn           = false;
  bool bSwitchStreamType = iStreamType != AE_DSP_ASTREAM_INVALID;

  /* The Mode is already used and need not to set up again */
  if (m_addons_MasterProc[m_activeMode].pMode->ModeID() == iModeID && !bSwitchStreamType)
    return true;

  CSingleLock lock(m_restartSection);

  CLog::Log(LOGDEBUG, "ActiveAE DSP - %s - Audio DSP processing id %d mode change:", __FUNCTION__, m_streamId);
  if (bSwitchStreamType && m_streamTypeUsed != iStreamType)
  {
    AE_DSP_STREAMTYPE old = m_streamTypeUsed;
    CLog::Log(LOGDEBUG, "  ----  Input stream  ----");
    if (iStreamType == AE_DSP_ASTREAM_AUTO)
      m_streamTypeUsed = m_streamTypeDetected;
    else if (iStreamType >= AE_DSP_ASTREAM_BASIC || iStreamType < AE_DSP_ASTREAM_AUTO)
      m_streamTypeUsed = iStreamType;
    else
    {
      CLog::Log(LOGWARNING, "ActiveAE DSP - %s - Unknown audio stream type, falling back to basic", __FUNCTION__);
      m_streamTypeUsed = AE_DSP_ASTREAM_BASIC;
    }

    CLog::Log(LOGDEBUG, "  | Stream Type change   : From '%s' to '%s'", GetStreamTypeName(old), GetStreamTypeName(m_streamTypeUsed));
  }

  /*!
   * Set the new stream type to the addon settings and properties structures.
   * If the addon want to use another stream type, it can be becomes written inside
   * the m_addonStreamProperties.iStreamType.
   */
  m_addonStreamProperties.iStreamType = m_streamTypeUsed;
  m_addonSettings.iStreamType         = m_streamTypeUsed;
  m_activeModeOutChannels             = -1;

  if (iModeID <= AE_DSP_MASTER_MODE_ID_PASSOVER)
  {
    CLog::Log(LOGINFO, "ActiveAE DSP - Switching master mode off");
    m_activeMode = 0;
    bReturn      = true;
  }
  else
  {
    CActiveAEDSPModePtr mode;
    for (unsigned int ptr = 0; ptr < m_addons_MasterProc.size(); ++ptr)
    {
      mode = m_addons_MasterProc.at(ptr).pMode;
      if (mode->ModeID() == iModeID && mode->IsEnabled())
      {
        if (m_addons_MasterProc[ptr].pAddon)
        {
          AE_DSP_ERROR err = m_addons_MasterProc[ptr].pAddon->MasterProcessSetMode(&m_addons_MasterProc[ptr].handle, m_addonStreamProperties.iStreamType, mode->AddonModeNumber(), mode->ModeID());
          if (err != AE_DSP_ERROR_NO_ERROR)
          {
            CLog::Log(LOGERROR, "ActiveAE DSP - %s - addon master mode selection failed on %s with Mode '%s' with %s",
                                    __FUNCTION__,
                                    m_addons_MasterProc[ptr].pAddon->GetAudioDSPName().c_str(),
                                    mode->AddonModeName().c_str(),
                                    CActiveAEDSPAddon::ToString(err));
          }
          else
          {
            CLog::Log(LOGINFO, "ActiveAE DSP - Switching master mode to '%s' as '%s' on '%s'",
                                    mode->AddonModeName().c_str(),
                                    GetStreamTypeName((AE_DSP_STREAMTYPE)m_addonStreamProperties.iStreamType),
                                    m_addons_MasterProc[ptr].pAddon->GetAudioDSPName().c_str());

            m_activeMode            = (int)ptr;
            m_activeModeOutChannels = m_addons_MasterProc[m_activeMode].pAddon->MasterProcessGetOutChannels(&m_addons_MasterProc[ptr].handle, m_activeModeOutChannelsPresent);
            bReturn                 = true;
          }
        }
        else if (mode->ModeID() >= AE_DSP_MASTER_MODE_ID_INTERNAL_TYPES)
        {
          CLog::Log(LOGINFO, "ActiveAE DSP - Switching master mode to internal '%s' as '%s'",
                                  mode->AddonModeName().c_str(),
                                  GetStreamTypeName((AE_DSP_STREAMTYPE)m_addonStreamProperties.iStreamType));

          m_activeMode            = (int)ptr;
          m_activeModeOutChannels = -1;
          bReturn                 = true;
        }
        break;
      }
    }
  }

  /*!
   * Initialize fallback matrix mixer
   */
  InitFFMpegDSPProcessor();

  return bReturn;
}

void CActiveAEDSPProcess::ClearArray(float **array, unsigned int samples)
{
  unsigned int presentFlag = 1;
  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
  {
    if (m_addonSettings.lOutChannelPresentFlags & presentFlag)
      memset(array[i], 0, samples*sizeof(float));
    presentFlag <<= 1;
  }
}

bool CActiveAEDSPProcess::Process(CSampleBuffer *in, CSampleBuffer *out)
{
  CSingleLock lock(m_restartSection);

  bool needDSPAddonsReinit  = m_forceInit;
  uint64_t iTime            = static_cast<uint64_t>(XbmcThreads::SystemClockMillis()) * 10000;
  int64_t hostFrequency     = CurrentHostFrequency();
  unsigned int frames       = in->pkt->nb_samples;

  /* Detect interleaved input stream channel positions if unknown or changed */
  if (m_channelLayoutIn != in->pkt->config.channel_layout)
  {
    m_channelLayoutIn = in->pkt->config.channel_layout;

    m_idx_in[AE_CH_FL]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_FRONT_LEFT);
    m_idx_in[AE_CH_FR]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_FRONT_RIGHT);
    m_idx_in[AE_CH_FC]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_FRONT_CENTER);
    m_idx_in[AE_CH_LFE]   = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_LOW_FREQUENCY);
    m_idx_in[AE_CH_BL]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_BACK_LEFT);
    m_idx_in[AE_CH_BR]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_BACK_RIGHT);
    m_idx_in[AE_CH_FLOC]  = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_FRONT_LEFT_OF_CENTER);
    m_idx_in[AE_CH_FROC]  = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_FRONT_RIGHT_OF_CENTER);
    m_idx_in[AE_CH_BC]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_BACK_CENTER);
    m_idx_in[AE_CH_SL]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_SIDE_LEFT);
    m_idx_in[AE_CH_SR]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_SIDE_RIGHT);
    m_idx_in[AE_CH_TC]    = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_TOP_CENTER);
    m_idx_in[AE_CH_TFL]   = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_TOP_FRONT_LEFT);
    m_idx_in[AE_CH_TFC]   = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_TOP_FRONT_CENTER);
    m_idx_in[AE_CH_TFR]   = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_TOP_FRONT_RIGHT);
    m_idx_in[AE_CH_TBL]   = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_TOP_BACK_LEFT);
    m_idx_in[AE_CH_TBC]   = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_TOP_BACK_CENTER);
    m_idx_in[AE_CH_TBR]   = av_get_channel_layout_channel_index(m_channelLayoutIn, AV_CH_TOP_BACK_RIGHT);
    m_idx_in[AE_CH_BLOC]  = -1; // manually disable these channels because ffmpeg does not support them
    m_idx_in[AE_CH_BROC]  = -1;

    needDSPAddonsReinit = true;
  }

  /* Detect also interleaved output stream channel positions if unknown or changed */
  if (m_channelLayoutOut != out->pkt->config.channel_layout)
  {
    m_channelLayoutOut = out->pkt->config.channel_layout;

    m_idx_out[AE_CH_FL]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_FRONT_LEFT);
    m_idx_out[AE_CH_FR]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_FRONT_RIGHT);
    m_idx_out[AE_CH_FC]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_FRONT_CENTER);
    m_idx_out[AE_CH_LFE]  = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_LOW_FREQUENCY);
    m_idx_out[AE_CH_BL]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_BACK_LEFT);
    m_idx_out[AE_CH_BR]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_BACK_RIGHT);
    m_idx_out[AE_CH_FLOC] = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_FRONT_LEFT_OF_CENTER);
    m_idx_out[AE_CH_FROC] = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_FRONT_RIGHT_OF_CENTER);
    m_idx_out[AE_CH_BC]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_BACK_CENTER);
    m_idx_out[AE_CH_SL]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_SIDE_LEFT);
    m_idx_out[AE_CH_SR]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_SIDE_RIGHT);
    m_idx_out[AE_CH_TC]   = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_TOP_CENTER);
    m_idx_out[AE_CH_TFL]  = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_TOP_FRONT_LEFT);
    m_idx_out[AE_CH_TFC]  = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_TOP_FRONT_CENTER);
    m_idx_out[AE_CH_TFR]  = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_TOP_FRONT_RIGHT);
    m_idx_out[AE_CH_TBL]  = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_TOP_BACK_LEFT);
    m_idx_out[AE_CH_TBC]  = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_TOP_BACK_CENTER);
    m_idx_out[AE_CH_TBR]  = av_get_channel_layout_channel_index(m_channelLayoutOut, AV_CH_TOP_BACK_RIGHT);
    m_idx_out[AE_CH_BLOC]  = -1; // manually disable these channels because ffmpeg does not support them
    m_idx_out[AE_CH_BROC]  = -1;

    needDSPAddonsReinit = true;
  }

  if (needDSPAddonsReinit)
  {
    m_addonSettings.lInChannelPresentFlags = 0;
    if (m_idx_in[AE_CH_FL] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FL;
    if (m_idx_in[AE_CH_FR] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FR;
    if (m_idx_in[AE_CH_FC] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FC;
    if (m_idx_in[AE_CH_LFE] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_LFE;
    if (m_idx_in[AE_CH_BL] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BL;
    if (m_idx_in[AE_CH_BR] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BR;
    if (m_idx_in[AE_CH_FLOC] >= 0)  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FLOC;
    if (m_idx_in[AE_CH_FROC] >= 0)  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_FROC;
    if (m_idx_in[AE_CH_BC] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BC;
    if (m_idx_in[AE_CH_SL] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_SL;
    if (m_idx_in[AE_CH_SR] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_SR;
    if (m_idx_in[AE_CH_TFL] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TFL;
    if (m_idx_in[AE_CH_TFR] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TFR;
    if (m_idx_in[AE_CH_TFC] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TFC;
    if (m_idx_in[AE_CH_TC] >= 0)    m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TC;
    if (m_idx_in[AE_CH_TBL] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TBL;
    if (m_idx_in[AE_CH_TBR] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TBR;
    if (m_idx_in[AE_CH_TBC] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TBC;
    if (m_idx_in[AE_CH_TBR] >= 0)   m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_TBR;
    if (m_idx_in[AE_CH_BLOC] >= 0)  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BLOC;
    if (m_idx_in[AE_CH_BROC] >= 0)  m_addonSettings.lInChannelPresentFlags |= AE_DSP_PRSNT_CH_BROC;

    m_addonSettings.lOutChannelPresentFlags = 0;
    if (m_idx_out[AE_CH_FL] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FL;
    if (m_idx_out[AE_CH_FR] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FR;
    if (m_idx_out[AE_CH_FC] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FC;
    if (m_idx_out[AE_CH_LFE] >= 0)  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_LFE;
    if (m_idx_out[AE_CH_BL] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BL;
    if (m_idx_out[AE_CH_BR] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BR;
    if (m_idx_out[AE_CH_FLOC] >= 0) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FLOC;
    if (m_idx_out[AE_CH_FROC] >= 0) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_FROC;
    if (m_idx_out[AE_CH_BC] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BC;
    if (m_idx_out[AE_CH_SL] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_SL;
    if (m_idx_out[AE_CH_SR] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_SR;
    if (m_idx_out[AE_CH_TFL] >= 0)  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TFL;
    if (m_idx_out[AE_CH_TFR] >= 0)  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TFR;
    if (m_idx_out[AE_CH_TFC] >= 0)  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TFC;
    if (m_idx_out[AE_CH_TC] >= 0)   m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TC;
    if (m_idx_out[AE_CH_TBL] >= 0)  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TBL;
    if (m_idx_out[AE_CH_TBR] >= 0)  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TBR;
    if (m_idx_out[AE_CH_TBC] >= 0)  m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_TBC;
    if (m_idx_out[AE_CH_BLOC] >= 0) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BLOC;
    if (m_idx_out[AE_CH_BROC] >= 0) m_addonSettings.lOutChannelPresentFlags |= AE_DSP_PRSNT_CH_BROC;

    m_addonSettings.iStreamID           = m_streamId;
    m_addonSettings.iInChannels         = in->pkt->config.channels;
    m_addonSettings.iOutChannels        = out->pkt->config.channels;
    m_addonSettings.iInSamplerate       = in->pkt->config.sample_rate;
    m_addonSettings.iProcessSamplerate  = m_addon_InputResample.pAddon  ? m_addon_InputResample.pAddon->InputResampleSampleRate(&m_addon_InputResample.handle)   : m_addonSettings.iInSamplerate;
    m_addonSettings.iOutSamplerate      = m_addon_OutputResample.pAddon ? m_addon_OutputResample.pAddon->OutputResampleSampleRate(&m_addon_OutputResample.handle) : m_addonSettings.iProcessSamplerate;

    if (m_NewMasterMode >= 0)
    {
      MasterModeChange(m_NewMasterMode, m_NewStreamType);
      m_NewMasterMode = AE_DSP_MASTER_MODE_ID_INVALID;
      m_NewStreamType = AE_DSP_ASTREAM_INVALID;
    }

    for (AE_DSP_ADDONMAP_ITR itr = m_usedMap.begin(); itr != m_usedMap.end(); ++itr)
    {
      AE_DSP_ERROR err = itr->second->StreamInitialize(&m_addon_Handles[itr->first], &m_addonSettings);
      if (err != AE_DSP_ERROR_NO_ERROR)
      {
        CLog::Log(LOGERROR, "ActiveAE DSP - %s - addon initialize failed on %s with %s", __FUNCTION__, itr->second->GetAudioDSPName().c_str(), CActiveAEDSPAddon::ToString(err));
      }
    }

    RecheckProcessArray(frames);
    ClearArray(m_processArray[0], m_processArraySize);
    ClearArray(m_processArray[1], m_processArraySize);

    m_forceInit         = false;
    m_iLastProcessTime  = static_cast<uint64_t>(XbmcThreads::SystemClockMillis()) * 10000;
    m_iLastProcessUsage = 0;
    m_fLastProcessUsage = 0.0f;

    /**
     * Setup ffmpeg convert array for input stream
     */
    SetFFMpegDSPProcessorArray(m_ffMpegConvertArray, m_processArray[0], NULL);
  }

  int64_t startTime;
  float **lastOutArray    = m_processArray[0];
  unsigned int togglePtr  = 1;

  /**
   * Convert to required planar float format inside dsp system
   */
  if (swr_convert(m_convertInput, (uint8_t **)m_ffMpegConvertArray[0], m_processArraySize, (const uint8_t **)in->pkt->data , frames) < 0)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - input audio convert failed", __FUNCTION__);
    return false;
  }

    /**********************************************/
   /** DSP Processing Algorithms following here **/
  /**********************************************/

  /**
   * DSP input processing
   * Can be used to have unchanged input stream..
   * All DSP addons allowed todo this.
   */
  for (unsigned int i = 0; i < m_addons_InputProc.size(); ++i)
  {
    if (!m_addons_InputProc[i].pAddon->InputProcess(&m_addons_InputProc[i].handle, (const float **)lastOutArray, frames))
    {
      CLog::Log(LOGERROR, "ActiveAE DSP - %s - input process failed on addon No. %i", __FUNCTION__, i);
      return false;
    }
  }

  /**
   * DSP resample processing before master
   * Here a high quality resample can be performed.
   * Only one DSP addon is allowed todo this!
   */
  if (m_addon_InputResample.pAddon)
  {
    startTime = CurrentHostCounter();

    frames = m_addon_InputResample.pAddon->InputResampleProcess(&m_addon_InputResample.handle, lastOutArray, m_processArray[togglePtr], frames);
    if (frames == 0)
      return false;

    m_addon_InputResample.iLastTime += 1000 * 10000 * (CurrentHostCounter() - startTime) / hostFrequency;

    lastOutArray = m_processArray[togglePtr];
    togglePtr ^= 1;
  }

  /**
   * DSP pre processing
   * All DSP addons allowed todo this and order of it set on settings.
   */
  for (unsigned int i = 0; i < m_addons_PreProc.size(); ++i)
  {
    startTime = CurrentHostCounter();

    frames = m_addons_PreProc[i].pAddon->PreProcess(&m_addons_PreProc[i].handle, m_addons_PreProc[i].iAddonModeNumber, lastOutArray, m_processArray[togglePtr], frames);
    if (frames == 0)
      return false;

    m_addons_PreProc[i].iLastTime += 1000 * 10000 * (CurrentHostCounter() - startTime) / hostFrequency;

    lastOutArray = m_processArray[togglePtr];
    togglePtr ^= 1;
  }

  /**
   * DSP master processing
   * Here a channel upmix/downmix for stereo surround sound can be performed
   * Only one DSP addon is allowed todo this!
   */
  if (m_addons_MasterProc[m_activeMode].pAddon)
  {
    startTime = CurrentHostCounter();

    frames = m_addons_MasterProc[m_activeMode].pAddon->MasterProcess(&m_addons_MasterProc[m_activeMode].handle, lastOutArray, m_processArray[togglePtr], frames);
    if (frames == 0)
      return false;

    m_addons_MasterProc[m_activeMode].iLastTime += 1000 * 10000 * (CurrentHostCounter() - startTime) / hostFrequency;

    lastOutArray = m_processArray[togglePtr];
    togglePtr ^= 1;
  }

  /**
   * Perform fallback channel mixing if input channel alignment is different
   * from output and not becomes processed by active master processing mode or
   * perform ffmpeg related internal master processes.
   */
  if (m_resamplerDSPProcessor)
  {
    startTime = CurrentHostCounter();

    if (needDSPAddonsReinit)
      SetFFMpegDSPProcessorArray(m_ffMpegProcessArray, lastOutArray, m_processArray[togglePtr]);

    frames = m_resamplerDSPProcessor->Resample((uint8_t**)m_ffMpegProcessArray[FFMPEG_PROC_ARRAY_OUT], frames, (uint8_t**)m_ffMpegProcessArray[FFMPEG_PROC_ARRAY_IN], frames, 1.0);
    if (frames <= 0)
    {
      CLog::Log(LOGERROR, "CActiveAEResample::Resample - resample failed");
      return false;
    }

    m_addons_MasterProc[m_activeMode].iLastTime += 1000 * 10000 * (CurrentHostCounter() - startTime) / hostFrequency;

    lastOutArray = m_processArray[togglePtr];
    togglePtr ^= 1;
  }

  /**
   * DSP post processing
   * On the post processing can be things performed with additional channel upmix like 6.1 to 7.1
   * or frequency/volume corrections, speaker distance handling, equalizer... .
   * All DSP addons allowed todo this and order of it set on settings.
   */
  for (unsigned int i = 0; i < m_addons_PostProc.size(); ++i)
  {
    startTime = CurrentHostCounter();

    frames = m_addons_PostProc[i].pAddon->PostProcess(&m_addons_PostProc[i].handle, m_addons_PostProc[i].iAddonModeNumber, lastOutArray, m_processArray[togglePtr], frames);
    if (frames == 0)
      return false;

    m_addons_PostProc[i].iLastTime += 1000 * 10000 * (CurrentHostCounter() - startTime) / hostFrequency;

    lastOutArray = m_processArray[togglePtr];
    togglePtr ^= 1;
  }

  /**
   * DSP resample processing behind master
   * Here a high quality resample can be performed.
   * Only one DSP addon is allowed todo this!
   */
  if (m_addon_OutputResample.pAddon)
  {
    startTime = CurrentHostCounter();

    frames = m_addon_OutputResample.pAddon->OutputResampleProcess(&m_addon_OutputResample.handle, lastOutArray, m_processArray[togglePtr], frames);
    if (frames == 0)
      return false;

    m_addon_OutputResample.iLastTime += 1000 * 10000 * (CurrentHostCounter() - startTime) / hostFrequency;

    lastOutArray = m_processArray[togglePtr];
    togglePtr ^= 1;
  }

  /**
   * Setup ffmpeg convert array for output stream, performed here to now last array
   */
  if (needDSPAddonsReinit)
    SetFFMpegDSPProcessorArray(m_ffMpegConvertArray, NULL, lastOutArray);

  /**
   * Convert back to required output format
   */
  if (swr_convert(m_convertOutput, (uint8_t **)out->pkt->data, m_processArraySize, (const uint8_t **)m_ffMpegConvertArray[1], frames) < 0)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - output audio convert failed", __FUNCTION__);
    return false;
  }
  out->pkt->nb_samples = frames;

  /**
   * Update cpu process percent usage values for modes and total (every second)
   */
  if (iTime >= m_iLastProcessTime + 1000*10000)
    CalculateCPUUsage(iTime);

  return true;
}

bool CActiveAEDSPProcess::RecheckProcessArray(unsigned int inputFrames)
{
  /* Check for big enough array */
  unsigned int framesNeeded;
  unsigned int framesOut = m_processArraySize;

  if (inputFrames > framesOut)
    framesOut = inputFrames;

  if (m_addon_InputResample.pAddon)
  {
    framesNeeded = m_addon_InputResample.pAddon->InputResampleProcessNeededSamplesize(&m_addon_InputResample.handle);
    if (framesNeeded > framesOut)
      framesOut = framesNeeded;
  }

  for (unsigned int i = 0; i < m_addons_PreProc.size(); ++i)
  {
    framesNeeded = m_addons_PreProc[i].pAddon->PreProcessNeededSamplesize(&m_addons_PreProc[i].handle, m_addons_PreProc[i].iAddonModeNumber);
    if (framesNeeded > framesOut)
      framesOut = framesNeeded;
  }

  if (m_addons_MasterProc[m_activeMode].pAddon)
  {
    framesNeeded = m_addons_MasterProc[m_activeMode].pAddon->MasterProcessNeededSamplesize(&m_addons_MasterProc[m_activeMode].handle);
    if (framesNeeded > framesOut)
      framesOut = framesNeeded;
  }

  for (unsigned int i = 0; i < m_addons_PostProc.size(); ++i)
  {
    framesNeeded = m_addons_PostProc[i].pAddon->PostProcessNeededSamplesize(&m_addons_PostProc[i].handle, m_addons_PostProc[i].iAddonModeNumber);
    if (framesNeeded > framesOut)
      framesOut = framesNeeded;
  }

  if (m_addon_OutputResample.pAddon)
  {
    framesNeeded = m_addon_OutputResample.pAddon->OutputResampleProcessNeededSamplesize(&m_addon_OutputResample.handle);
    if (framesNeeded > framesOut)
      framesOut = framesNeeded;
  }

  if (framesOut > m_processArraySize)
  {
    if (!ReallocProcessArray(framesOut))
      return false;

    m_processArraySize = framesOut;
  }
  return true;
}

bool CActiveAEDSPProcess::ReallocProcessArray(unsigned int requestSize)
{
  m_processArraySize = requestSize + MIN_DSP_ARRAY_SIZE / 10;
  for (int i = 0; i < AE_DSP_CH_MAX; ++i)
  {
    m_processArray[0][i] = (float*)realloc(m_processArray[0][i], m_processArraySize*sizeof(float));
    m_processArray[1][i] = (float*)realloc(m_processArray[1][i], m_processArraySize*sizeof(float));
    if (m_processArray[0][i] == NULL || m_processArray[1][i] == NULL)
    {
      CLog::Log(LOGERROR, "ActiveAE DSP - %s - realloc of process data array failed", __FUNCTION__);
      return false;
    }
  }
  return true;
}

// in this function the usage for each adsp-mode in percent is calculated
void CActiveAEDSPProcess::CalculateCPUUsage(uint64_t iTime)
{
  int64_t iUsage = CThread::GetCurrentThread()->GetAbsoluteUsage();

  if (iTime != m_iLastProcessTime)
  {
    // calculate usage only if we don't divide by zero
    if (m_iLastProcessUsage > 0 && m_iLastProcessTime > 0)
    {
      m_fLastProcessUsage = (float)(iUsage - m_iLastProcessUsage) / (float)(iTime - m_iLastProcessTime) * 100.0f;
    }

    float dTFactor = 100.0f / (float)(iTime - m_iLastProcessTime);

    if(m_addon_InputResample.pMode)
    {
      m_addon_InputResample.pMode->SetCPUUsage((float)(m_addon_InputResample.iLastTime)*dTFactor);
      m_addon_InputResample.iLastTime = 0;
    }

    for (unsigned int i = 0; i < m_addons_PreProc.size(); ++i)
    {
      m_addons_PreProc[i].pMode->SetCPUUsage((float)(m_addons_PreProc[i].iLastTime)*dTFactor);
      m_addons_PreProc[i].iLastTime = 0;
    }

    if (m_addons_MasterProc[m_activeMode].pMode)
    {
      m_addons_MasterProc[m_activeMode].pMode->SetCPUUsage((float)(m_addons_MasterProc[m_activeMode].iLastTime)*dTFactor);
      m_addons_MasterProc[m_activeMode].iLastTime = 0;
    }

    for (unsigned int i = 0; i < m_addons_PostProc.size(); ++i)
    {
      m_addons_PostProc[i].pMode->SetCPUUsage((float)(m_addons_PostProc[i].iLastTime)*dTFactor);
      m_addons_PostProc[i].iLastTime = 0;
    }

    if (m_addon_OutputResample.pMode)
    {
      m_addon_OutputResample.pMode->SetCPUUsage((float)(m_addon_OutputResample.iLastTime)*dTFactor);
      m_addon_OutputResample.iLastTime = 0;
    }
  }

  m_iLastProcessUsage = iUsage;
  m_iLastProcessTime  = iTime;
}

void CActiveAEDSPProcess::SetFFMpegDSPProcessorArray(float *array_ffmpeg[2][AE_DSP_CH_MAX], float **array_in, float **array_out)
{
  /*!
   * Setup ffmpeg resampler channel setup, this way is not my favorite but it works to become
   * the correct channel alignment on the given input and output signal.
   * The problem is, the process array of ffmpeg is not fixed and for every selected channel setup
   * the positions are different. For this case a translation from the fixed dsp stream format to
   * ffmpeg format must be performed. It use a separate process array table which becomes set by
   * already present channel memory storage.
   */

  //! Initialize input channel alignmment for ffmpeg process array
  if (array_in)
  {
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_FL)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_FL]]    = array_in[AE_DSP_CH_FL];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_FR)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_FR]]    = array_in[AE_DSP_CH_FR];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_FC)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_FC]]    = array_in[AE_DSP_CH_FC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_LFE)  array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_LFE]]   = array_in[AE_DSP_CH_LFE];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_BL)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_BL]]    = array_in[AE_DSP_CH_BL];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_BR)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_BR]]    = array_in[AE_DSP_CH_BR];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_FLOC) array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_FLOC]]  = array_in[AE_DSP_CH_FLOC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_FROC) array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_FROC]]  = array_in[AE_DSP_CH_FROC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_BC)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_BC]]    = array_in[AE_DSP_CH_BC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_SL)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_SL]]    = array_in[AE_DSP_CH_SL];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_SR)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_SR]]    = array_in[AE_DSP_CH_SR];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_TFL)  array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_TFL]]   = array_in[AE_DSP_CH_TFL];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_TFR)  array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_TFR]]   = array_in[AE_DSP_CH_TFR];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_TFC)  array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_TFC]]   = array_in[AE_DSP_CH_TFC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_TC)   array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_TC]]    = array_in[AE_DSP_CH_TC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_TBL)  array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_TBL]]   = array_in[AE_DSP_CH_TBL];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_TBR)  array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_TBR]]   = array_in[AE_DSP_CH_TBR];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_TBC)  array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_TBC]]   = array_in[AE_DSP_CH_TBC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_BLOC) array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_BLOC]]  = array_in[AE_DSP_CH_BLOC];
    if (m_addonSettings.lInChannelPresentFlags & AE_DSP_PRSNT_CH_BROC) array_ffmpeg[FFMPEG_PROC_ARRAY_IN][m_idx_in[AE_CH_BROC]]  = array_in[AE_DSP_CH_BROC];
  }

  //! Initialize output channel alignmment for ffmpeg process array
  if (array_out)
  {
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FL)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_FL]]    = array_out[AE_DSP_CH_FL];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FR)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_FR]]    = array_out[AE_DSP_CH_FR];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FC)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_FC]]    = array_out[AE_DSP_CH_FC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_LFE)  array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_LFE]]   = array_out[AE_DSP_CH_LFE];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BL)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_BL]]    = array_out[AE_DSP_CH_BL];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BR)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_BR]]    = array_out[AE_DSP_CH_BR];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FLOC) array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_FLOC]]  = array_out[AE_DSP_CH_FLOC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_FROC) array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_FROC]]  = array_out[AE_DSP_CH_FROC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BC)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_BC]]    = array_out[AE_DSP_CH_BC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_SL)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_SL]]    = array_out[AE_DSP_CH_SL];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_SR)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_SR]]    = array_out[AE_DSP_CH_SR];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFL)  array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_TFL]]   = array_out[AE_DSP_CH_TFL];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFR)  array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_TFR]]   = array_out[AE_DSP_CH_TFR];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TFC)  array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_TFC]]   = array_out[AE_DSP_CH_TFC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TC)   array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_TC]]    = array_out[AE_DSP_CH_TC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBL)  array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_TBL]]   = array_out[AE_DSP_CH_TBL];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBR)  array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_TBR]]   = array_out[AE_DSP_CH_TBR];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_TBC)  array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_TBC]]   = array_out[AE_DSP_CH_TBC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BLOC) array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_BLOC]]  = array_out[AE_DSP_CH_BLOC];
    if (m_addonSettings.lOutChannelPresentFlags & AE_DSP_PRSNT_CH_BROC) array_ffmpeg[FFMPEG_PROC_ARRAY_OUT][m_idx_out[AE_CH_BROC]]  = array_out[AE_DSP_CH_BROC];
  }
}

float CActiveAEDSPProcess::GetDelay()
{
  float delay = 0.0f;

  CSingleLock lock(m_critSection);

  if (m_addon_InputResample.pAddon)
    delay += m_addon_InputResample.pAddon->InputResampleGetDelay(&m_addon_InputResample.handle);

  for (unsigned int i = 0; i < m_addons_PreProc.size(); ++i)
    delay += m_addons_PreProc[i].pAddon->PreProcessGetDelay(&m_addons_PreProc[i].handle, m_addons_PreProc[i].iAddonModeNumber);

  if (m_addons_MasterProc[m_activeMode].pAddon)
    delay += m_addons_MasterProc[m_activeMode].pAddon->MasterProcessGetDelay(&m_addons_MasterProc[m_activeMode].handle);

  for (unsigned int i = 0; i < m_addons_PostProc.size(); ++i)
    delay += m_addons_PostProc[i].pAddon->PostProcessGetDelay(&m_addons_PostProc[i].handle, m_addons_PostProc[i].iAddonModeNumber);

  if (m_addon_OutputResample.pAddon)
    delay += m_addon_OutputResample.pAddon->OutputResampleGetDelay(&m_addon_OutputResample.handle);

  return delay;
}

bool CActiveAEDSPProcess::HasActiveModes(AE_DSP_MODE_TYPE type)
{
  bool bReturn(false);

  CSingleLock lock(m_critSection);

  switch (type)
  {
  case AE_DSP_MODE_TYPE_INPUT_RESAMPLE:
    if (m_addon_InputResample.pAddon != NULL)
      bReturn = true;
    break;
  case AE_DSP_MODE_TYPE_PRE_PROCESS:
    if (!m_addons_PreProc.empty())
      bReturn = true;
    break;
  case AE_DSP_MODE_TYPE_MASTER_PROCESS:
    if (!m_addons_MasterProc.empty())
      bReturn = true;
    break;
  case AE_DSP_MODE_TYPE_POST_PROCESS:
    if (!m_addons_PostProc.empty())
      bReturn = true;
    break;
  case AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE:
    if (m_addon_OutputResample.pAddon != NULL)
      bReturn = true;
    break;
  default:
    break;
  };

  return bReturn;
}

void CActiveAEDSPProcess::GetActiveModes(AE_DSP_MODE_TYPE type, std::vector<CActiveAEDSPModePtr> &modes)
{
  CSingleLock lock(m_critSection);

  if (m_addon_InputResample.pAddon != NULL && (type == AE_DSP_MODE_TYPE_UNDEFINED || type == AE_DSP_MODE_TYPE_INPUT_RESAMPLE))
    modes.push_back(m_addon_InputResample.pMode);

  if (type == AE_DSP_MODE_TYPE_UNDEFINED || type == AE_DSP_MODE_TYPE_PRE_PROCESS)
    for (unsigned int i = 0; i < m_addons_PreProc.size(); ++i)
      modes.push_back(m_addons_PreProc[i].pMode);

  if (m_addons_MasterProc[m_activeMode].pAddon != NULL && (type == AE_DSP_MODE_TYPE_UNDEFINED || type == AE_DSP_MODE_TYPE_MASTER_PROCESS))
    modes.push_back(m_addons_MasterProc[m_activeMode].pMode);

  if (type == AE_DSP_MODE_TYPE_UNDEFINED || type == AE_DSP_MODE_TYPE_POST_PROCESS)
    for (unsigned int i = 0; i < m_addons_PostProc.size(); ++i)
      modes.push_back(m_addons_PostProc[i].pMode);

  if (m_addon_OutputResample.pAddon != NULL && (type == AE_DSP_MODE_TYPE_UNDEFINED || type == AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE))
    modes.push_back(m_addon_OutputResample.pMode);
}

void CActiveAEDSPProcess::GetAvailableMasterModes(AE_DSP_STREAMTYPE streamType, std::vector<CActiveAEDSPModePtr> &modes)
{
  CSingleLock lock(m_critSection);

  for (unsigned int i = 0; i < m_addons_MasterProc.size(); ++i)
  {
    if (m_addons_MasterProc[i].pMode->SupportStreamType(streamType))
      modes.push_back(m_addons_MasterProc[i].pMode);
  }
}

int CActiveAEDSPProcess::GetActiveMasterModeID()
{
  CSingleLock lock(m_critSection);

  return m_activeMode < 0 ? AE_DSP_MASTER_MODE_ID_INVALID : m_addons_MasterProc[m_activeMode].pMode->ModeID();
}

CActiveAEDSPModePtr CActiveAEDSPProcess::GetActiveMasterMode() const
{
  CSingleLock lock(m_critSection);

  CActiveAEDSPModePtr mode;

  if (m_activeMode < 0)
    return mode;

  mode = m_addons_MasterProc[m_activeMode].pMode;
  return mode;
}

bool CActiveAEDSPProcess::SetMasterMode(AE_DSP_STREAMTYPE streamType, int iModeID, bool bSwitchStreamType)
{
  /*!
   * if the unique master mode id is already used a reinit is not needed
   */
  if (m_addons_MasterProc[m_activeMode].pMode->ModeID() == iModeID && !bSwitchStreamType)
    return true;

  CSingleLock lock(m_restartSection);

  m_NewMasterMode = iModeID;
  m_NewStreamType = bSwitchStreamType ? streamType : AE_DSP_ASTREAM_INVALID;
  m_forceInit     = true;
  return true;
}

bool CActiveAEDSPProcess::IsMenuHookModeActive(AE_DSP_MENUHOOK_CAT category, int iAddonId, unsigned int iModeNumber)
{
  std::vector <sDSPProcessHandle> *addons = NULL;

  switch (category)
  {
    case AE_DSP_MENUHOOK_MASTER_PROCESS:
      addons = &m_addons_MasterProc;
      break;
    case AE_DSP_MENUHOOK_PRE_PROCESS:
      addons = &m_addons_PreProc;
      break;
    case AE_DSP_MENUHOOK_POST_PROCESS:
      addons = &m_addons_PostProc;
      break;
    case AE_DSP_MENUHOOK_RESAMPLE:
      {
        if (m_addon_InputResample.iAddonModeNumber > 0 &&
            m_addon_InputResample.pMode &&
            m_addon_InputResample.pMode->AddonID() == iAddonId &&
            m_addon_InputResample.pMode->AddonModeNumber() == iModeNumber)
          return true;

        if (m_addon_OutputResample.iAddonModeNumber > 0 &&
            m_addon_OutputResample.pMode &&
            m_addon_OutputResample.pMode->AddonID() == iAddonId &&
            m_addon_OutputResample.pMode->AddonModeNumber() == iModeNumber)
          return true;
      }
    default:
      break;
  }

  if (addons)
  {
    for (unsigned int i = 0; i < addons->size(); ++i)
    {
      if (addons->at(i).iAddonModeNumber > 0 &&
          addons->at(i).pMode->AddonID() == iAddonId &&
          addons->at(i).pMode->AddonModeNumber() == iModeNumber)
        return true;
    }
  }
  return false;
}

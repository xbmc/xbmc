/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
 * ActiveAEDSP.cpp — Windows ADSP integration for ActiveAE
 *
 * Bridges the ActiveAE audio pipeline to a legacy ADSP add-on
 * (type "xbmc.audiodsp", e.g. audiodsp.vsthost).
 *
 * All real logic is inside #ifdef TARGET_WINDOWS to keep non-Windows builds
 * clean and compilation-error-free.
 */

#include "ActiveAEDSP.h"

#ifdef TARGET_WINDOWS

#include "ServiceBroker.h"
#include "addons/AddonInfo.h"
#include "addons/AddonManager.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/binary-addons/DllAddon.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"

// Legacy ADSP add-on API (from kodi-audiodsp-vsthost/include/kodi-legacy-adsp/)
#include "kodi_adsp_types.h"

#include <cstring>

using namespace ADDON;

namespace ActiveAE
{

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

CActiveAEDSP::CActiveAEDSP()
  : m_funcs(new AudioDSP{})
  , m_handle(new ADDON_HANDLE_STRUCT{})
{
}

CActiveAEDSP::~CActiveAEDSP()
{
  Deinit();
  delete m_funcs;
  delete m_handle;
}

// ---------------------------------------------------------------------------
// Init — load and start the first enabled ADDON_AUDIODSP add-on
// ---------------------------------------------------------------------------

bool CActiveAEDSP::Init()
{
  if (m_initialized)
    return true;

  VECADDONS addons;
  if (!CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON_AUDIODSP) || addons.empty())
  {
    CLog::Log(LOGINFO, "CActiveAEDSP::Init — no enabled ADDON_AUDIODSP add-on found");
    return false;
  }

  auto& addon = addons.front();

  // Downcast to CAddonDll to obtain the platform DLL path.
  auto addonDll = std::dynamic_pointer_cast<CAddonDll>(addon);
  if (!addonDll)
  {
    CLog::Log(LOGERROR, "CActiveAEDSP::Init — could not cast add-on to CAddonDll");
    return false;
  }

  const std::string libPath = addonDll->LibPath();
  if (libPath.empty())
  {
    CLog::Log(LOGERROR, "CActiveAEDSP::Init — add-on '{}' has no DLL for this platform",
              addon->ID());
    return false;
  }

  // Load the DLL directly, bypassing CAddonDll::Create() / CheckAPIVersion(),
  // because legacy ADSP DLLs do not export ADDON_GetTypeVersion.
  // NOTE: audiodsp.vsthost makes no callbacks into Kodi (it uses no libKODI_adsp
  // host-callback stubs), so nullptr is safe as the host-callbacks argument.
  // If a future ADSP add-on needs host callbacks this approach must be revisited.
  m_dll = new DllAddon;
  m_dll->SetFile(libPath);
  m_dll->EnableDelayedUnload(false);
  if (!m_dll->Load())
  {
    CLog::Log(LOGERROR, "CActiveAEDSP::Init — failed to load DLL '{}'", libPath);
    delete m_dll;
    m_dll = nullptr;
    return false;
  }

  // Fill the function-pointer table via get_addon().
  std::memset(m_funcs, 0, sizeof(AudioDSP));
  m_dll->GetAddon(m_funcs);

  // Prepare user-data and add-on-path strings (must outlive ADDON_Create call).
  const std::string specialPath = "special://profile/addon_data/" + addon->ID() + "/";
  XFILE::CDirectory::Create(specialPath);
  m_userPath  = CSpecialProtocol::TranslatePath(specialPath);
  m_addonPath = addon->Path();

  AE_DSP_PROPERTIES props{};
  props.strUserPath  = m_userPath.c_str();
  props.strAddonPath = m_addonPath.c_str();

  // ADDON_Create(hdl, props) — hdl is the host-callback pointer (nullptr is
  // safe here because the add-on makes no callbacks into Kodi).
  const ADDON_STATUS status = m_dll->Create(nullptr, &props);
  if (status != ADDON_STATUS_OK && status != ADDON_STATUS_NEED_SETTINGS)
  {
    CLog::Log(LOGERROR, "CActiveAEDSP::Init — ADDON_Create returned error {}", (int)status);
    m_dll->Destroy();
    delete m_dll;
    m_dll = nullptr;
    return false;
  }

  m_initialized = true;
  m_dspFailed   = false;
  CLog::Log(LOGINFO, "CActiveAEDSP::Init — loaded '{}'", addon->Name());
  return true;
}

// ---------------------------------------------------------------------------
// Deinit
// ---------------------------------------------------------------------------

void CActiveAEDSP::Deinit()
{
  if (!m_initialized)
    return;

  StreamDestroy();

  if (m_dll)
  {
    m_dll->Destroy();
    delete m_dll;
    m_dll = nullptr;
  }

  std::memset(m_funcs, 0, sizeof(AudioDSP));

  m_initialized = false;
  m_dspFailed   = false;
  CLog::Log(LOGINFO, "CActiveAEDSP::Deinit — ADSP add-on unloaded");
}

// ---------------------------------------------------------------------------
// OnConfigure — re-initialize the DSP stream when the AE format changes
// ---------------------------------------------------------------------------

void CActiveAEDSP::OnConfigure(const AEAudioFormat& fmt)
{
  if (!m_initialized || m_dspFailed.load())
    return;

  // Skip if the format has not changed.
  if (m_streamActive &&
      m_streamFmt.m_sampleRate == fmt.m_sampleRate &&
      m_streamFmt.m_channelLayout.Count() == fmt.m_channelLayout.Count() &&
      m_streamFmt.m_frames == fmt.m_frames)
    return;

  if (m_streamActive)
    StreamDestroy();

  StreamCreate(fmt);
}

// ---------------------------------------------------------------------------
// StreamCreate — private helper
// ---------------------------------------------------------------------------

void CActiveAEDSP::StreamCreate(const AEAudioFormat& fmt)
{
  if (!m_initialized || !m_funcs->StreamCreate || m_dspFailed.load())
    return;

  const int channels  = static_cast<int>(fmt.m_channelLayout.Count());
  const int blockSize = (fmt.m_frames > 0) ? fmt.m_frames : 1024;

  AE_DSP_SETTINGS settings{};
  settings.iStreamID                = 0;
  settings.iStreamType              = AE_DSP_ASTREAM_BASIC;
  settings.iInChannels              = channels;
  // NOTE: lInChannelPresentFlags / lOutChannelPresentFlags are set to 0 (unknown)
  // because ActiveAE's AEChannelInfo does not map directly to AE_DSP_PRSNT_CH_* flags.
  // audiodsp.vsthost ignores these fields; a future improvement could translate them
  // using CAEUtil::GetAVChannelLayout if a more strict ADSP add-on requires them.
  settings.lInChannelPresentFlags   = 0;
  settings.iInFrames                = blockSize;
  settings.iInSamplerate            = fmt.m_sampleRate;
  settings.iProcessFrames           = blockSize;
  settings.iProcessSamplerate       = fmt.m_sampleRate;
  settings.iOutChannels             = channels;
  settings.lOutChannelPresentFlags  = 0;
  settings.iOutFrames               = blockSize;
  settings.iOutSamplerate           = fmt.m_sampleRate;
  settings.bInputResamplingActive   = false;
  settings.bStereoUpmix             = false;
  settings.iQualityLevel            = 0;

  std::memset(m_handle, 0, sizeof(ADDON_HANDLE_STRUCT));
  m_handle->callerAddress = this;

  const AE_DSP_ERROR err = m_funcs->StreamCreate(&settings, nullptr, m_handle);
  if (err != AE_DSP_ERROR_NO_ERROR)
  {
    CLog::Log(LOGWARNING, "CActiveAEDSP::StreamCreate — returned error {}", (int)err);
    return;
  }

  // Inform the add-on which master-process mode is active (mode id 1).
  if (m_funcs->StreamIsModeSupported)
    m_funcs->StreamIsModeSupported(m_handle, AE_DSP_MODE_TYPE_MASTER_PROCESS, 1, 0);
  if (m_funcs->MasterProcessSetMode)
    m_funcs->MasterProcessSetMode(m_handle, AE_DSP_ASTREAM_BASIC, 1, 0);
  if (m_funcs->StreamInitialize)
    m_funcs->StreamInitialize(m_handle, &settings);

  // Allocate scratch buffers used for deinterleaving interleaved PCM input.
  m_scratch.assign(channels, std::vector<float>(static_cast<size_t>(blockSize), 0.0f));
  m_scratchPtrs.resize(channels);
  for (int ch = 0; ch < channels; ++ch)
    m_scratchPtrs[ch] = m_scratch[ch].data();

  m_streamFmt    = fmt;
  m_streamActive = true;

  CLog::Log(LOGINFO,
            "CActiveAEDSP::StreamCreate — {}ch @ {}Hz, {}fr",
            channels, fmt.m_sampleRate, blockSize);
}

// ---------------------------------------------------------------------------
// StreamDestroy — private helper
// ---------------------------------------------------------------------------

void CActiveAEDSP::StreamDestroy()
{
  if (!m_streamActive)
    return;

  if (m_funcs->StreamDestroy)
    m_funcs->StreamDestroy(m_handle);

  m_streamActive = false;
  std::memset(m_handle, 0, sizeof(ADDON_HANDLE_STRUCT));
  m_scratch.clear();
  m_scratchPtrs.clear();
}

// ---------------------------------------------------------------------------
// MasterProcess — in-place audio processing on the AE render thread
// ---------------------------------------------------------------------------

void CActiveAEDSP::MasterProcess(CSampleBuffer* buf)
{
  if (!m_streamActive || m_dspFailed.load() || !m_funcs->MasterProcess)
    return;
  if (!buf || !buf->pkt || buf->pkt->nb_samples <= 0)
    return;

  const int     channels = buf->pkt->config.channels;
  const unsigned int samples = static_cast<unsigned int>(buf->pkt->nb_samples);

  if (buf->pkt->planes > 1)
  {
    // Planar float (AE_FMT_FLOATP) — process directly in-place.
    auto** planes = reinterpret_cast<float**>(buf->pkt->data);

    __try
    {
      m_funcs->MasterProcess(m_handle, planes, planes, samples);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      CLog::Log(LOGERROR,
                "CActiveAEDSP::MasterProcess — add-on crashed (planar path); disabling DSP");
      m_dspFailed = true;
    }
  }
  else
  {
    // Interleaved float (AE_FMT_FLOAT) — deinterleave, process, reinterleave.
    if (channels <= 0 || static_cast<int>(m_scratchPtrs.size()) != channels)
      return;

    const size_t bytesPerCh = static_cast<size_t>(samples) * sizeof(float);
    if (m_scratch.empty() || static_cast<unsigned int>(m_scratch[0].size()) < samples)
    {
      // Resize scratch buffers if the block size grew (rare).
      for (auto& v : m_scratch)
        v.assign(samples, 0.0f);
    }

    // Deinterleave: interleaved[sample * ch + ch_idx] → scratch[ch][sample]
    const float* src = reinterpret_cast<const float*>(buf->pkt->data[0]);
    for (unsigned int s = 0; s < samples; ++s)
      for (int ch = 0; ch < channels; ++ch)
        m_scratchPtrs[ch][s] = src[s * channels + ch];

    __try
    {
      m_funcs->MasterProcess(m_handle,
                             m_scratchPtrs.data(),
                             m_scratchPtrs.data(),
                             samples);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
      CLog::Log(LOGERROR,
                "CActiveAEDSP::MasterProcess — add-on crashed (interleaved path); disabling DSP");
      m_dspFailed = true;
      return;
    }

    // Reinterleave: scratch[ch][sample] → interleaved[sample * ch + ch_idx]
    float* dst = reinterpret_cast<float*>(buf->pkt->data[0]);
    for (unsigned int s = 0; s < samples; ++s)
      for (int ch = 0; ch < channels; ++ch)
        dst[s * channels + ch] = m_scratchPtrs[ch][s];
  }
}

// ---------------------------------------------------------------------------
// IsActive
// ---------------------------------------------------------------------------

bool CActiveAEDSP::IsActive() const
{
  return m_initialized && !m_dspFailed.load();
}

} // namespace ActiveAE

#else // !TARGET_WINDOWS — stub implementations (no-ops)

namespace ActiveAE
{

CActiveAEDSP::CActiveAEDSP()  = default;
CActiveAEDSP::~CActiveAEDSP() = default;

bool CActiveAEDSP::Init()                          { return false; }
void CActiveAEDSP::Deinit()                        {}
void CActiveAEDSP::OnConfigure(const AEAudioFormat&) {}
void CActiveAEDSP::MasterProcess(CSampleBuffer*)   {}
bool CActiveAEDSP::IsActive() const                { return false; }

} // namespace ActiveAE

#endif // TARGET_WINDOWS

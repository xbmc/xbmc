/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidTVPerformance.h"
#include "platform/android/activity/XBMCApp.h"
#include "ServiceBroker.h"
#include "windowing/WinSystem.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "platform/android/CPUInfoAndroid.h"
#include "platform/android/GPUInfoAndroid.h"
#include "windowing/android/WinSystemAndroid.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "application/Application.h"
#include <androidjni/Display.h>
#include <androidjni/PowerManager.h>
#include <androidjni/Context.h>
#include <androidjni/Window.h>
#include <androidjni/Build.h>
#include <androidjni/ActivityManager.h>
#include <androidjni/View.h>
#include <androidjni/Surface.h>

// Initialize static members
std::atomic<int> CAndroidTVPerformance::m_performanceMode(TV_PERFORMANCE_MODE_STANDARD);
std::atomic<bool> CAndroidTVPerformance::m_isInitialized(false);
std::atomic<bool> CAndroidTVPerformance::m_isVideoPlaying(false);
std::atomic<bool> CAndroidTVPerformance::m_framerateSwitchingEnabled(true);
CStopWatch CAndroidTVPerformance::m_performanceMonitor;
CCriticalSection CAndroidTVPerformance::m_settingsLock;
CTimer CAndroidTVPerformance::m_monitorTimer;

// System capabilities
int CAndroidTVPerformance::m_cpuCores = 0;
int CAndroidTVPerformance::m_gpuLevel = 0;
int64_t CAndroidTVPerformance::m_totalMemory = 0;
int CAndroidTVPerformance::m_androidVersion = 0;
bool CAndroidTVPerformance::m_has4K = false;
bool CAndroidTVPerformance::m_hasHDR = false;
bool CAndroidTVPerformance::m_hasHLG = false;
bool CAndroidTVPerformance::m_hasDolbyVision = false;
bool CAndroidTVPerformance::m_hasOptimalRefreshRate = false;

void CAndroidTVPerformance::Initialize()
{
  if (m_isInitialized)
    return;

  CLog::Log(LOGINFO, "AndroidTVPerformance: Initializing performance optimizations for Android TV");
  
  // Detect system resources and capabilities
  DetectSystemResources();
  
  // Apply optimal settings based on device capabilities
  ApplyOptimalSettings();
  
  // Start performance monitoring
  m_performanceMonitor.Start();
  m_monitorTimer.SetTimeout(30000); // 30 seconds
  m_monitorTimer.Start([](){ MonitorPerformance(); });
  
  m_isInitialized = true;
  
  // Set performance mode based on device capabilities
  if (m_has4K && m_gpuLevel >= 5 && m_cpuCores >= 4)
  {
    // High-end device - use optimized mode by default
    SetPerformanceMode(TV_PERFORMANCE_MODE_OPTIMIZED);
  }
  else
  {
    // Lower-end device - use standard mode
    SetPerformanceMode(TV_PERFORMANCE_MODE_STANDARD);
  }
  
  CLog::Log(LOGINFO, "AndroidTVPerformance: Initialized with performance mode %d", GetPerformanceMode());
}

void CAndroidTVPerformance::DetectSystemResources()
{
  // Get Android version
  m_androidVersion = CJNIBuild::SDK_INT;
  
  // Get CPU information
  CCPUInfoAndroid cpuInfo;
  m_cpuCores = cpuInfo.GetCPUCount();
  
  // Get memory information
  CJNIActivityManager activityManager(CJNIContext::getSystemService(CJNIContext::ACTIVITY_SERVICE));
  CJNIActivityManager::MemoryInfo memoryInfo;
  activityManager.getMemoryInfo(memoryInfo);
  m_totalMemory = memoryInfo.totalMem;
  
  // Check display capabilities
  CWinSystemAndroid* winSystem = dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem());
  if (winSystem)
  {
    // Check if 4K resolution is supported
    m_has4K = winSystem->SupportsResolution(3840, 2160, 60.0f);
    
    // Check HDR capabilities
    CJNIWindowManager windowManager(CJNIContext::getSystemService(CJNIContext::WINDOW_SERVICE));
    CJNIDisplay display = windowManager.getDefaultDisplay();
    m_hasHDR = (CJNIBuild::SDK_INT >= 24) && display.isHdrSupported();
    
    // Get refresh rate capabilities
    std::vector<float> refreshRates = GetAvailableRefreshRates();
    m_hasOptimalRefreshRate = refreshRates.size() > 1; // If we have more than just 60Hz
  }
  
  // Evaluate GPU capabilities
  CGPUInfoAndroid gpuInfo;
  std::string gpuVendor = gpuInfo.GetVendor();
  std::string gpuRenderer = gpuInfo.GetRenderer();
  
  // Estimate GPU level based on renderer string
  // Higher number = better GPU
  m_gpuLevel = 1; // baseline
  
  if (gpuRenderer.find("Mali-G") != std::string::npos || 
      gpuRenderer.find("Adreno") != std::string::npos)
  {
    m_gpuLevel = 3; // Mid-range GPU
    
    // Check for higher-end GPUs
    if (gpuRenderer.find("Mali-G7") != std::string::npos || 
        gpuRenderer.find("Adreno 6") != std::string::npos)
    {
      m_gpuLevel = 5; // High-end GPU
    }
  }
  
  CLog::Log(LOGINFO, "AndroidTVPerformance: Detected system resources: Android %d, CPU cores: %d, "
              "Memory: %" PRId64 "MB, GPU Level: %d, 4K: %s, HDR: %s", 
              m_androidVersion, m_cpuCores, m_totalMemory / (1024*1024), m_gpuLevel, 
              m_has4K ? "yes" : "no", m_hasHDR ? "yes" : "no");
}

void CAndroidTVPerformance::SetPerformanceMode(int mode)
{
  if (mode < TV_PERFORMANCE_MODE_STANDARD || mode > TV_PERFORMANCE_MODE_MAX_PERFORMANCE)
    mode = TV_PERFORMANCE_MODE_STANDARD;
    
  if (mode == m_performanceMode.load())
    return;
    
  CLog::Log(LOGINFO, "AndroidTVPerformance: Setting performance mode to %d", mode);
  m_performanceMode = mode;
  
  // Apply settings based on the selected performance mode
  switch (mode)
  {
    case TV_PERFORMANCE_MODE_MAX_PERFORMANCE:
      ConfigureCPUGovernor(true);
      ConfigureGPUPerformance(true);
      OptimizeGUIRendering(true);
      break;
      
    case TV_PERFORMANCE_MODE_OPTIMIZED:
      ConfigureCPUGovernor(m_isVideoPlaying);
      ConfigureGPUPerformance(m_isVideoPlaying);
      OptimizeGUIRendering(true);
      break;
      
    case TV_PERFORMANCE_MODE_STANDARD:
    default:
      ConfigureCPUGovernor(false);
      ConfigureGPUPerformance(false);
      OptimizeGUIRendering(false);
      break;
  }
  
  // Apply memory management based on the mode
  ManageMemory(mode == TV_PERFORMANCE_MODE_MAX_PERFORMANCE);
}

int CAndroidTVPerformance::GetPerformanceMode()
{
  return m_performanceMode.load();
}

bool CAndroidTVPerformance::OptimizeVideoPlayback()
{
  bool success = false;

  CLog::Log(LOGINFO, "AndroidTVPerformance: Optimizing video playback");

  if (!g_application.GetAppPlayer().IsPlaying() || !g_application.GetAppPlayer().IsPlayingVideo())
  {
    CLog::Log(LOGINFO, "AndroidTVPerformance: No active video playback, nothing to optimize");
    return false;
  }

  m_isVideoPlaying = true; // Mark video as playing

  // Get actual stream properties
  const auto& player = g_application.GetAppPlayer();
  VideoStreamInfo streamInfo;
  if (player.GetVideoStreamInfo(CURRENT_STREAM, streamInfo))
  {
    int videoWidth = streamInfo.width;
    int videoHeight = streamInfo.height;
    double fps = streamInfo.fps;
    const std::string codecName = streamInfo.codecName;

    CLog::Log(LOGINFO, "AndroidTVPerformance: Stream resolution: %dx%d, FPS: %.2f, Codec: %s",
              videoWidth, videoHeight, fps, codecName.c_str());

    // Check if stream has HDR, using the actual stream info
    bool isHDRStream = IsHDRStream(streamInfo);

    // Check if stream requires DRM
    bool needsDRM = IsDRMPlayback(); // Checks player.IsVideoDRM()

    // Determine and set performance mode based on actual stream properties
    ConfigurePerformanceMode(videoWidth, videoHeight, fps, codecName, isHDRStream, streamInfo.bitrate);

    // Configure DRM *only if* the current stream requires it
    if (needsDRM)
    {
      ConfigureDRMPlayback();
    }

    // Adjust refresh rate if enabled
    if (m_framerateSwitchingEnabled && m_hasOptimalRefreshRate)
    {
        SetRefreshRate(static_cast<float>(fps));
    }

    success = true;
  }
  else
  {
    CLog::Log(LOGERROR, "AndroidTVPerformance: Failed to get video stream info");
    m_isVideoPlaying = false; // Reset flag if we failed
    return false;
  }

  return success;
}

void CAndroidTVPerformance::RestoreAfterVideoPlayback()
{
  CLog::Log(LOGINFO, "AndroidTVPerformance: Restoring settings after video playback");
  
  m_isVideoPlaying = false;
  
  // Restore performance mode to default
  SetPerformanceMode(m_has4K ? TV_PERFORMANCE_MODE_OPTIMIZED : TV_PERFORMANCE_MODE_STANDARD);
  
  // Reset refresh rate if needed
  if (m_framerateSwitchingEnabled && m_hasOptimalRefreshRate)
  {
    // Return to default refresh rate (60Hz usually)
    SetRefreshRate(60.0f);
  }
  
  // Reset audio configuration
  ConfigureAudio(false);
  
  // Reset background process priorities
  PrioritizeBackgroundProcesses(false);
}

void CAndroidTVPerformance::EnableFramerateSwitching(bool enabled)
{
  m_framerateSwitchingEnabled = enabled;
  CLog::Log(LOGINFO, "AndroidTVPerformance: Framerate switching %s", enabled ? "enabled" : "disabled");
}

void CAndroidTVPerformance::ConfigureCPUGovernor(bool highPerformance)
{
  try
  {
    JNIEnv* env = xbmc_jnienv();
    if (!env)
      return;

    CJNIPowerManager powerManager(CJNIContext::getSystemService(CJNIContext::POWER_SERVICE));
    if (powerManager)
    {
      if (highPerformance)
      {
        if (CJNIBuild::SDK_INT >= 24)
        {
          jobject activity = CXBMCApp::Get().getActivity();
          jclass activityClass = env->GetObjectClass(activity);
          jmethodID setSustainedPerformanceMode = env->GetMethodID(activityClass, "setSustainedPerformanceMode", "(Z)V");
          
          if (setSustainedPerformanceMode != NULL)
          {
            env->CallVoidMethod(activity, setSustainedPerformanceMode, JNI_TRUE);
            CLog::Log(LOGINFO, "AndroidTVPerformance: Requested sustained performance mode");
          }
          
          env->DeleteLocalRef(activityClass);
        }
      }
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "AndroidTVPerformance: Exception in ConfigureCPUGovernor: %s", e.what());
  }
}

void CAndroidTVPerformance::ConfigureGPUPerformance(bool maxPerformance)
{
  if (CServiceBroker::GetSettingsComponent())
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    
    if (maxPerformance)
    {
      settings->SetBool("videoplayer.usemediacodec", true);
      settings->SetBool("videoplayer.usemediacodecsurface", true);
      
      if (m_gpuLevel >= 3)
      {
        settings->SetInt("videoplayer.limitguiupdate", 0);
        settings->SetInt("videoplayer.maxgop", 0);
      }
      else
      {
        settings->SetInt("videoplayer.limitguiupdate", 2);
      }
    }
    else
    {
      settings->SetInt("videoplayer.limitguiupdate", 1);
    }
  }
}

void CAndroidTVPerformance::ManageMemory(bool forceGc)
{
  try
  {
    CJNIActivityManager activityManager(CJNIContext::getSystemService(CJNIContext::ACTIVITY_SERVICE));
    CJNIActivityManager::MemoryInfo memoryInfo;
    activityManager.getMemoryInfo(memoryInfo);
    
    int64_t availMemory = memoryInfo.availMem;
    int percentAvail = (int)((availMemory * 100) / m_totalMemory);
    
    CLog::Log(LOGDEBUG, "AndroidTVPerformance: Memory available: %d%% (%" PRId64 "MB of %" PRId64 "MB)",
        percentAvail, availMemory / (1024*1024), m_totalMemory / (1024*1024));
    
    if (percentAvail < TV_MEMORY_THRESHOLD_LOW || forceGc)
    {
      CLog::Log(LOGINFO, "AndroidTVPerformance: Low memory condition (%d%%), performing cleanup", percentAvail);
      
      JNIEnv* env = xbmc_jnienv();
      if (env)
        env->EnsureLocalCapacity(16);
        
      JNIEnv* jenv = xbmc_jnienv();
      jclass jcls = jenv->FindClass("java/lang/System");
      jmethodID runFinalization = jenv->GetStaticMethodID(jcls, "runFinalization", "()V");
      jmethodID gc = jenv->GetStaticMethodID(jcls, "gc", "()V");
      
      if (gc && runFinalization)
      {
        jenv->CallStaticVoidMethod(jcls, gc);
        jenv->CallStaticVoidMethod(jcls, runFinalization);
        jenv->CallStaticVoidMethod(jcls, gc);
      }
      
      jenv->DeleteLocalRef(jcls);
    }
    
    if (percentAvail < 10)
    {
      CLog::Log(LOGWARNING, "AndroidTVPerformance: Critical memory condition (%d%%)", percentAvail);
      HandleMemoryPressure(true);
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "AndroidTVPerformance: Exception in ManageMemory: %s", e.what());
  }
}

void CAndroidTVPerformance::EnableHDMICEC(bool enabled)
{
  if (CServiceBroker::GetSettingsComponent())
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    settings->SetBool("peripherals.cec.enabled", enabled);
  }
}

void CAndroidTVPerformance::ConfigureAudio(bool digitalOutput)
{
  if (CServiceBroker::GetSettingsComponent())
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    
    settings->SetBool("audiooutput.passthrough", digitalOutput);
    
    if (m_isVideoPlaying)
    {
      settings->SetInt("audiooutput.buffersize", 4);
    }
    else
    {
      settings->SetInt("audiooutput.buffersize", 2);
    }
  }
}

bool CAndroidTVPerformance::SetRefreshRate(float fps)
{
  CWinSystemAndroid* winSystem = dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;
    
  RESOLUTION_INFO res_info = winSystem->GetGfxContext().GetResInfo();
  
  std::vector<float> availableRates = GetAvailableRefreshRates();
  float closestMatch = 60.0f;
  float smallestDiff = 100.0f;
  
  std::vector<float> commonMultiples;
  if (fps <= 30.0)
  {
    commonMultiples.push_back(fps);
    commonMultiples.push_back(fps * 2.0f);
    commonMultiples.push_back(fps * 3.0f);
  }
  else
  {
    commonMultiples.push_back(fps);
  }
  
  for (float rate : availableRates)
  {
    for (float targetRate : commonMultiples)
    {
      float diff = std::abs(rate - targetRate);
      
      if (diff < 0.2f || diff < smallestDiff)
      {
        smallestDiff = diff;
        closestMatch = rate;
        
        if (diff < 0.2f)
          break;
      }
    }
  }
  
  if (std::abs(res_info.fRefreshRate - closestMatch) > 0.2f)
  {
    CLog::Log(LOGINFO, "AndroidTVPerformance: Setting refresh rate to %.2f Hz for content at %.2f fps", 
        closestMatch, fps);
        
    CXBMCApp::Get().SetDisplayMode(res_info.iScreenWidth, closestMatch);
    return true;
  }
  
  return false;
}

std::vector<float> CAndroidTVPerformance::GetAvailableRefreshRates()
{
  std::vector<float> result;
  
  try
  {
    CJNIWindowManager windowManager(CJNIContext::getSystemService(CJNIContext::WINDOW_SERVICE));
    CJNIDisplay display = windowManager.getDefaultDisplay();
    CJNISurfaceHolder holder = CXBMCApp::Get().getWindow().getDecorView().getHolder();
    
    if (CJNIBuild::SDK_INT >= 24)
    {
      jobject jsurface = holder.getSurface();
      CJNISurface surface(jsurface);
      
      JNIEnv* env = xbmc_jnienv();
      if (env && surface)
      {
        jclass surfaceClass = env->GetObjectClass(surface.get_raw());
        jmethodID getSupportedModes = env->GetMethodID(surfaceClass, "getSupportedModes", "()[Landroid/view/SurfaceHolder$SurfaceType;");
        
        if (getSupportedModes)
        {
          jobjectArray supportedModes = (jobjectArray)env->CallObjectMethod(surface.get_raw(), getSupportedModes);
          if (supportedModes)
          {
            int length = env->GetArrayLength(supportedModes);
            for (int i = 0; i < length; i++)
            {
              jobject mode = env->GetObjectArrayElement(supportedModes, i);
              jclass modeClass = env->GetObjectClass(mode);
              jmethodID getRefreshRate = env->GetMethodID(modeClass, "getRefreshRate", "()F");
              
              if (getRefreshRate)
              {
                float rate = env->CallFloatMethod(mode, getRefreshRate);
                result.push_back(rate);
              }
              
              env->DeleteLocalRef(modeClass);
              env->DeleteLocalRef(mode);
            }
            env->DeleteLocalRef(supportedModes);
          }
        }
        env->DeleteLocalRef(surfaceClass);
      }
    }
    else
    {
      result.push_back(display.getRefreshRate());
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "AndroidTVPerformance: Exception in GetAvailableRefreshRates: %s", e.what());
  }
  
  if (result.empty())
    result.push_back(60.0f);
    
  std::string ratesLog = "Available refresh rates: ";
  for (float rate : result)
    ratesLog += std::to_string(rate) + "Hz, ";
  
  CLog::Log(LOGINFO, "AndroidTVPerformance: %s", ratesLog.c_str());
  
  return result;
}

void CAndroidTVPerformance::PrioritizeBackgroundProcesses(bool enableBackgroundPriority)
{
  if (enableBackgroundPriority)
  {
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW, 1);
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW_PAUSABLE, 0);
  }
  else
  {
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW, 2);
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW_PAUSABLE, 1);
  }
}

void CAndroidTVPerformance::HandleMemoryPressure(bool lowMemory)
{
  if (lowMemory)
  {
    CLog::Log(LOGWARNING, "AndroidTVPerformance: Handling low memory pressure");
    
    ManageMemory(true);
    
    OptimizeGUIRendering(true);
    
    PrioritizeBackgroundProcesses(true);
    
    g_application.OnMemoryLow();
  }
}

void CAndroidTVPerformance::OptimizeGUIRendering(bool optimized)
{
  if (CServiceBroker::GetSettingsComponent() && CServiceBroker::GetRenderSystem())
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    
    if (optimized)
    {
      settings->SetBool("videoscreen.blankdisplays", false);
      
      if (m_totalMemory >= 2LL * 1024 * 1024 * 1024)
      {
        settings->SetInt("videoplayer.textureupprescaleratio", 2);
        CServiceBroker::GetRenderSystem()->SetTexturesCacheMaxSize(true);
      }
      else
      {
        settings->SetInt("videoplayer.textureupprescaleratio", 1);
        CServiceBroker::GetRenderSystem()->SetTexturesCacheMaxSize(false);
      }
    }
    else
    {
      settings->SetInt("videoplayer.textureupprescaleratio", 1);
      CServiceBroker::GetRenderSystem()->SetTexturesCacheMaxSize(false);
    }
  }
}

void CAndroidTVPerformance::SaveCurrentSettings()
{
}

void CAndroidTVPerformance::ApplyOptimalSettings()
{
  if (!CServiceBroker::GetSettingsComponent())
    return;
    
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  
  // PR #26685 Feedback by CastagnaIT: Forcing VP settings may break playback on some devices.
  // Avoid overriding user-configured video player acceleration settings.
  // settings->SetBool("videoplayer.usemediacodec", true);

  // For capable devices, surface rendering can improve performance, but forcing it may cause issues.
  // if (m_androidVersion >= 23 && m_gpuLevel >= 3)
  // {
  //   settings->SetBool("videoplayer.usemediacodecsurface", true);
  // }

  settings->SetBool("audiooutput.processquality", m_cpuCores >= 4);
  
  if (m_has4K && m_gpuLevel >= 5)
  {
    settings->SetInt("videoscreen.guiresolution", 0);
  }
  
  if (m_totalMemory < 1LL * 1024 * 1024 * 1024)
  {
    CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring for low-memory device");
    settings->SetInt("network.cachemembuffersize", 5242880);
  }
  else if (m_totalMemory < 2LL * 1024 * 1024 * 1024)
  {
    CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring for medium-memory device");
    settings->SetInt("network.cachemembuffersize", 20971520);
  }
  else
  {
    CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring for high-memory device");
    settings->SetInt("network.cachemembuffersize", 41943040);
  }
}

void CAndroidTVPerformance::MonitorPerformance()
{
  if (g_application.GetAppPlayer().IsPlaying() && !m_isVideoPlaying && 
      g_application.GetAppPlayer().IsPlayingVideo())
  {
    int width = 0;
    int height = 0;
    float fps = 0.0f;
    bool isHDR = false;
    
    g_application.GetAppPlayer().GetVideoResolution(width, height);
    fps = g_application.GetAppPlayer().GetVideoFps();
    
    CVideoSettings videoSettings = g_application.GetAppPlayer().GetVideoSettings();
    isHDR = videoSettings.m_HDR;
    
    OptimizeVideoPlayback();
  }
  else if (!g_application.GetAppPlayer().IsPlayingVideo() && m_isVideoPlaying)
  {
    RestoreAfterVideoPlayback();
  }
  
  if (m_performanceMonitor.IsRunning() && m_performanceMonitor.GetElapsedSeconds() > 300)
  {
    ManageMemory(false);
    m_performanceMonitor.Reset();
  }
  
  m_monitorTimer.Start();
}

bool CAndroidTVPerformance::HasUHDCapability()
{
  return m_has4K;
}

bool CAndroidTVPerformance::HasHDRCapability()
{
  return m_hasHDR;
}

// Change signature to include bitrate parameter
void CAndroidTVPerformance::ConfigurePerformanceMode(int width, int height, double fps, const std::string& codecName, bool isHDRStream, int bitrate)
{
  CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring performance for %dx%d@%.2f, Codec: %s, HDR: %s, Bitrate: %d kbps",
            width, height, fps, codecName.c_str(), isHDRStream ? "yes" : "no", bitrate / 1000);

  // PR #26685 Feedback by CastagnaIT: Consider bitrate and codec complexity when selecting performance mode.
  int targetMode = TV_PERFORMANCE_MODE_STANDARD;

  const int HIGH_BITRATE_THRESHOLD = 5000000; // 5 Mbps
  // Simplified check: high-bitrate or demanding streams
  if (bitrate >= HIGH_BITRATE_THRESHOLD || width >= 3840 || height >= 2160 || fps > 30.1 || isHDRStream)
  {
    if (m_has4K && m_gpuLevel >= 3)
      targetMode = TV_PERFORMANCE_MODE_OPTIMIZED;
    // Could escalate to MAX_PERFORMANCE for very high bitrate/codecs
  }

  SetPerformanceMode(targetMode);
}

void CAndroidTVPerformance::ConfigureDRMPlayback()
{
  CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring for DRM playback (stream requires DRM)");

  if (GetPerformanceMode() != TV_PERFORMANCE_MODE_MAX_PERFORMANCE)
  {
      CLog::Log(LOGINFO, "AndroidTVPerformance: Setting MAX_PERFORMANCE mode for DRM playback");
      SetPerformanceMode(TV_PERFORMANCE_MODE_MAX_PERFORMANCE);
  }

  if (HasDedicatedDRMHardware())
  {
    CLog::Log(LOGINFO, "AndroidTVPerformance: Device likely has dedicated DRM hardware.");
  }
  else
  {
    CLog::Log(LOGINFO, "AndroidTVPerformance: Assuming software DRM, ensuring CPU performance.");
  }
}

bool CAndroidTVPerformance::HasDedicatedDRMHardware()
{
  if (m_gpuLevel >= 4 && m_androidVersion >= 9)
    return true;
    
  return false;
}

bool CAndroidTVPerformance::IsDRMPlayback()
{
  bool isDRM = false;
  
  if (g_application.GetAppPlayer().IsPlaying())
  {
    auto& player = g_application.GetAppPlayer();
    isDRM = player.IsVideoDRM();
    
    CLog::Log(LOGDEBUG, "AndroidTVPerformance: DRM detection result: %s", isDRM ? "yes" : "no");
  }
  
  return isDRM;
}

bool CAndroidTVPerformance::IsHDRStream(const VideoStreamInfo& streamInfo)
{
  bool isHDRStream = false;
  
  if (streamInfo.hdrType == HDR_TYPE_HDR10 ||
      streamInfo.hdrType == HDR_TYPE_HDR10PLUS ||
      streamInfo.hdrType == HDR_TYPE_DOLBYVISION ||
      streamInfo.hdrType == HDR_TYPE_HLG)
  {
    isHDRStream = true;
    CLog::Log(LOGINFO, "AndroidTVPerformance: HDR stream detected (type: %d)", streamInfo.hdrType);
  }
  
  return isHDRStream;
}
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

void CAndroidTVPerformance::OptimizeVideoPlayback(int videoWidth, int videoHeight, float frameRate, bool isHDR)
{
  m_isVideoPlaying = true;
  
  CLog::Log(LOGINFO, "AndroidTVPerformance: Optimizing for video playback %dx%d @ %.2f fps, HDR: %s", 
      videoWidth, videoHeight, frameRate, isHDR ? "yes" : "no");
      
  // Save current settings for later restoration
  SaveCurrentSettings();
  
  // Configure refresh rate to match content if enabled
  if (m_framerateSwitchingEnabled && m_hasOptimalRefreshRate)
    SetRefreshRate(frameRate);
    
  // Configure CPU for video playback
  ConfigureCPUGovernor(true);
  
  // Configure GPU for video playback
  ConfigureGPUPerformance(true);
  
  // Configure audio for optimal video playback
  ConfigureAudio(true);
  
  // Check if this is 4K or HDR content and needs extra optimization
  if (videoWidth >= 3840 || videoHeight >= 2160 || isHDR)
  {
    // For 4K or HDR content, we need full performance
    SetPerformanceMode(TV_PERFORMANCE_MODE_MAX_PERFORMANCE);
    
    // Special configuration for HDR content
    if (isHDR && m_hasHDR)
    {
      // Configure DRM secure playback for HDR content if needed
      ConfigureDRMPlayback();
    }
    
    // Reduce background activity for high-demand content
    PrioritizeBackgroundProcesses(true);
  }
  else
  {
    // For regular HD content, optimized mode is sufficient
    SetPerformanceMode(TV_PERFORMANCE_MODE_OPTIMIZED);
  }
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
  // CPU governor configuration requires root on Android
  // We can only request high performance mode through standard Android APIs
  
  try
  {
    // Android's CPU boost functionality is available through PowerManager
    JNIEnv* env = xbmc_jnienv();
    if (!env)
      return;

    CJNIPowerManager powerManager(CJNIContext::getSystemService(CJNIContext::POWER_SERVICE));
    if (powerManager)
    {
      if (highPerformance)
      {
        // Request sustained performance mode if available (API 24+)
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
  // We can't directly control the GPU, but we can adjust rendering settings 
  // in Kodi to optimize GPU usage
  
  if (CServiceBroker::GetSettingsComponent())
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    
    if (maxPerformance)
    {
      // Enable all hardware acceleration features
      settings->SetBool("videoplayer.usemediacodec", true);
      settings->SetBool("videoplayer.usemediacodecsurface", true);
      
      if (m_gpuLevel >= 3)
      {
        // Higher-end GPUs can handle these features
        settings->SetInt("videoplayer.limitguiupdate", 0);  // No limit
        settings->SetInt("videoplayer.maxgop", 0);          // No limit
      }
      else
      {
        // For lower-end GPUs, limit some features
        settings->SetInt("videoplayer.limitguiupdate", 2);  // Moderate limit
      }
    }
    else
    {
      // Standard mode - balanced settings
      settings->SetInt("videoplayer.limitguiupdate", 1);  // Normal limit
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
    
    // Calculate percentage of available memory
    int64_t availMemory = memoryInfo.availMem;
    int percentAvail = (int)((availMemory * 100) / m_totalMemory);
    
    CLog::Log(LOGDEBUG, "AndroidTVPerformance: Memory available: %d%% (%" PRId64 "MB of %" PRId64 "MB)",
        percentAvail, availMemory / (1024*1024), m_totalMemory / (1024*1024));
    
    // If memory is low or GC is forced, attempt to release memory
    if (percentAvail < TV_MEMORY_THRESHOLD_LOW || forceGc)
    {
      CLog::Log(LOGINFO, "AndroidTVPerformance: Low memory condition (%d%%), performing cleanup", percentAvail);
      
      // Release JNI local references
      JNIEnv* env = xbmc_jnienv();
      if (env)
        env->EnsureLocalCapacity(16);
        
      // Call Java garbage collection
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
    
    // If memory is critically low, take emergency action
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
  // Configure audio settings for optimal performance
  if (CServiceBroker::GetSettingsComponent())
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    
    // Configure passthrough based on whether we're using digital output
    settings->SetBool("audiooutput.passthrough", digitalOutput);
    
    // Configure other audio settings
    if (m_isVideoPlaying)
    {
      // Optimize for video playback
      settings->SetInt("audiooutput.buffersize", 4); // larger buffer
    }
    else
    {
      // Normal operation
      settings->SetInt("audiooutput.buffersize", 2); // default buffer
    }
  }
}

bool CAndroidTVPerformance::SetRefreshRate(float fps)
{
  CWinSystemAndroid* winSystem = dynamic_cast<CWinSystemAndroid*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;
    
  // Get current resolution details
  RESOLUTION_INFO res_info = winSystem->GetGfxContext().GetResInfo();
  
  // Find the closest matching refresh rate
  std::vector<float> availableRates = GetAvailableRefreshRates();
  float closestMatch = 60.0f; // default
  float smallestDiff = 100.0f;
  
  // Consider common frame rates and their multiples
  std::vector<float> commonMultiples;
  if (fps <= 30.0)
  {
    // For 23.976, 24, 25, 29.97, 30 fps content
    // Consider: 1x, 2x, 3x multipliers
    commonMultiples.push_back(fps);
    commonMultiples.push_back(fps * 2.0f);
    commonMultiples.push_back(fps * 3.0f);
  }
  else
  {
    // For higher fps content, just use direct match
    commonMultiples.push_back(fps);
  }
  
  // Find the best match from available refresh rates
  for (float rate : availableRates)
  {
    for (float targetRate : commonMultiples)
    {
      float diff = std::abs(rate - targetRate);
      
      // If we're very close (within 0.2 Hz) or this is a better match than before
      if (diff < 0.2f || diff < smallestDiff)
      {
        smallestDiff = diff;
        closestMatch = rate;
        
        // If we're very close, stop searching
        if (diff < 0.2f)
          break;
      }
    }
  }
  
  // Apply the new refresh rate if it's different enough
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
    
    if (CJNIBuild::SDK_INT >= 24) // Android 7.0+
    {
      jobject jsurface = holder.getSurface();
      CJNISurface surface(jsurface);
      
      // Call to get supported modes
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
      // Fallback for older Android versions - just get current rate
      result.push_back(display.getRefreshRate());
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "AndroidTVPerformance: Exception in GetAvailableRefreshRates: %s", e.what());
  }
  
  // If we didn't get any rates, add the default 60Hz
  if (result.empty())
    result.push_back(60.0f);
    
  // Log available rates
  std::string ratesLog = "Available refresh rates: ";
  for (float rate : result)
    ratesLog += std::to_string(rate) + "Hz, ";
  
  CLog::Log(LOGINFO, "AndroidTVPerformance: %s", ratesLog.c_str());
  
  return result;
}

void CAndroidTVPerformance::PrioritizeBackgroundProcesses(bool enableBackgroundPriority)
{
  // Adjust the priority of background threads
  if (enableBackgroundPriority)
  {
    // Lower priority of non-critical background tasks
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW, 1);
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW_PAUSABLE, 0);
  }
  else
  {
    // Restore normal priority
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW, 2);
    CServiceBroker::GetJobManager()->SetMaximumWorkers(CJob::PRIORITY_LOW_PAUSABLE, 1);
  }
}

void CAndroidTVPerformance::HandleMemoryPressure(bool lowMemory)
{
  if (lowMemory)
  {
    CLog::Log(LOGWARNING, "AndroidTVPerformance: Handling low memory pressure");
    
    // Force garbage collection
    ManageMemory(true);
    
    // Reduce GUI complexity
    OptimizeGUIRendering(true);
    
    // Reduce background activity
    PrioritizeBackgroundProcesses(true);
    
    // Notify application of low memory
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
      // Configure for optimal performance
      settings->SetBool("videoscreen.blankdisplays", false);
      
      // Configure texture caching based on device capabilities
      if (m_totalMemory >= 2LL * 1024 * 1024 * 1024) // 2GB or more RAM
      {
        // High memory device - can use more texture memory
        settings->SetInt("videoplayer.textureupprescaleratio", 2);
        CServiceBroker::GetRenderSystem()->SetTexturesCacheMaxSize(true);
      }
      else
      {
        // Lower memory device - use more conservative texture settings
        settings->SetInt("videoplayer.textureupprescaleratio", 1);
        CServiceBroker::GetRenderSystem()->SetTexturesCacheMaxSize(false);
      }
    }
    else
    {
      // Standard settings
      settings->SetInt("videoplayer.textureupprescaleratio", 1);
      CServiceBroker::GetRenderSystem()->SetTexturesCacheMaxSize(false);
    }
  }
}

void CAndroidTVPerformance::SaveCurrentSettings()
{
  // This method would save current settings before optimization
  // but we don't actually need to implement it since we restore
  // to known defaults rather than previous values
}

void CAndroidTVPerformance::ApplyOptimalSettings()
{
  if (!CServiceBroker::GetSettingsComponent())
    return;
    
  auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  
  // Apply Android TV-specific optimal settings
  
  // Video settings
  settings->SetBool("videoplayer.usemediacodec", true);  // Use hardware acceleration
  
  // For capable devices, use surface rendering for better performance
  if (m_androidVersion >= 23 && m_gpuLevel >= 3)  // Android 6.0+ with decent GPU
  {
    settings->SetBool("videoplayer.usemediacodecsurface", true);
  }
  
  // GUI settings
  settings->SetBool("audiooutput.processquality", m_cpuCores >= 4);  // High quality processing only for better CPUs
  
  // For 4K capable devices, enable 4K GUI
  if (m_has4K && m_gpuLevel >= 5)
  {
    settings->SetInt("videoscreen.guiresolution", 0);  // Auto resolution
  }
  
  // Memory settings based on device capabilities
  if (m_totalMemory < 1LL * 1024 * 1024 * 1024)  // Less than 1GB RAM
  {
    // Low memory device
    CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring for low-memory device");
    settings->SetInt("network.cachemembuffersize", 5242880);  // 5MB network cache
  }
  else if (m_totalMemory < 2LL * 1024 * 1024 * 1024)  // 1-2GB RAM
  {
    // Medium memory device
    CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring for medium-memory device");
    settings->SetInt("network.cachemembuffersize", 20971520);  // 20MB network cache
  }
  else  // 2GB+ RAM
  {
    // High memory device
    CLog::Log(LOGINFO, "AndroidTVPerformance: Configuring for high-memory device");
    settings->SetInt("network.cachemembuffersize", 41943040);  // 40MB network cache
  }
}

void CAndroidTVPerformance::MonitorPerformance()
{
  // Check if video is playing and optimize if needed
  if (g_application.GetAppPlayer().IsPlaying() && !m_isVideoPlaying && 
      g_application.GetAppPlayer().IsPlayingVideo())
  {
    // Video just started
    int width = 0;
    int height = 0;
    float fps = 0.0f;
    bool isHDR = false;
    
    // Get video details
    g_application.GetAppPlayer().GetVideoResolution(width, height);
    fps = g_application.GetAppPlayer().GetVideoFps();
    
    // Check if HDR
    CVideoSettings videoSettings = g_application.GetAppPlayer().GetVideoSettings();
    isHDR = videoSettings.m_HDR;
    
    // Optimize for this video
    OptimizeVideoPlayback(width, height, fps, isHDR);
  }
  else if (!g_application.GetAppPlayer().IsPlayingVideo() && m_isVideoPlaying)
  {
    // Video playback ended
    RestoreAfterVideoPlayback();
  }
  
  // Periodically manage memory
  if (m_performanceMonitor.IsRunning() && m_performanceMonitor.GetElapsedSeconds() > 300)  // Every 5 minutes
  {
    ManageMemory(false);
    m_performanceMonitor.Reset();
  }
  
  // Restart timer for next check
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

void CAndroidTVPerformance::ConfigureDRMPlayback()
{
  if (CServiceBroker::GetSettingsComponent())
  {
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    
    // Configure DRM settings for secure playback
    settings->SetBool("videoplayer.useprimedecoder", true);
    
    // Check if we need Widevine DRM
    bool hasWidevine = false;
    JNIEnv* env = xbmc_jnienv();
    if (env)
    {
      jobject activity = CXBMCApp::Get().getActivity();
      jclass activityClass = env->GetObjectClass(activity);
      jmethodID hasWidevineDrm = env->GetMethodID(activityClass, "hasWidevineDrm", "()Z");
      
      if (hasWidevineDrm != NULL)
      {
        hasWidevine = env->CallBooleanMethod(activity, hasWidevineDrm) == JNI_TRUE;
      }
      
      env->DeleteLocalRef(activityClass);
    }
    
    if (hasWidevine)
    {
      CLog::Log(LOGINFO, "AndroidTVPerformance: Widevine DRM detected, configuring secure playback");
      settings->SetBool("videoplayer.usedrmsecuredecoder", true);
    }
  }
}
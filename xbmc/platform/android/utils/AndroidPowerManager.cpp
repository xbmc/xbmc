/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AndroidPowerManager.h"
#include "platform/android/activity/XBMCApp.h"
#include "ServiceBroker.h"
#include "windowing/WinSystem.h"
#include "utils/log.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "platform/android/activity/JNIMainActivity.h"

#include <androidjni/PowerManager.h>
#include <androidjni/Context.h>
#include <androidjni/Build.h>
#include <androidjni/PackageManager.h>
#include <androidjni/Configuration.h>
#include <androidjni/Resources.h>
#include <androidjni/UiModeManager.h>

std::atomic<bool> CAndroidPowerManager::m_powerSavingEnabled{false};
int CAndroidPowerManager::m_lastBatteryLevel{100};
bool CAndroidPowerManager::m_isCharging{true};
AndroidDeviceFormFactor CAndroidPowerManager::m_deviceFormFactor{DEVICE_FORM_FACTOR_UNKNOWN};

void CAndroidPowerManager::Initialize()
{
  // Detect device form factor (phone, tablet, TV)
  m_deviceFormFactor = DetectDeviceFormFactor();
  
  // Get initial battery level from XBMCApp
  m_lastBatteryLevel = CXBMCApp::Get().GetBatteryLevel();
  
  // Assume charging initially (safer assumption)
  m_isCharging = true;
  
  // Initialize power saving mode based on battery level for mobile devices only
  if (IsMobileDevice())
  {
    UpdatePowerState(m_lastBatteryLevel, m_isCharging);
    CLog::Log(LOGINFO, "CAndroidPowerManager: Initialized with battery level: %d on mobile device", m_lastBatteryLevel);
  }
  else
  {
    // For Android TV, always use performance mode
    m_powerSavingEnabled = false;
    CLog::Log(LOGINFO, "CAndroidPowerManager: Initialized on Android TV device - power saving disabled");
  }
}

void CAndroidPowerManager::UpdatePowerState(int batteryLevel, bool isCharging)
{
  m_lastBatteryLevel = batteryLevel;
  m_isCharging = isCharging;
  
  // Only apply power management for mobile devices
  if (!IsMobileDevice())
    return;
    
  // Skip power saving mode if charging
  if (isCharging)
  {
    if (m_powerSavingEnabled)
    {
      CLog::Log(LOGINFO, "CAndroidPowerManager: Device now charging, disabling power saving mode");
      SetPowerSavingMode(false);
    }
    return;
  }
  
  // Enter power saving mode if battery is low
  if (batteryLevel <= BATTERY_LEVEL_LOW && !m_powerSavingEnabled)
  {
    CLog::Log(LOGINFO, "CAndroidPowerManager: Battery level low (%d%%), enabling power saving mode", batteryLevel);
    SetPowerSavingMode(true);
  }
  // Exit power saving mode if battery is above threshold + hysteresis
  else if (batteryLevel > (BATTERY_LEVEL_LOW + 5) && m_powerSavingEnabled)
  {
    CLog::Log(LOGINFO, "CAndroidPowerManager: Battery level recovered (%d%%), disabling power saving mode", batteryLevel);
    SetPowerSavingMode(false);
  }
  
  // Apply critical mode if battery extremely low
  if (batteryLevel <= BATTERY_LEVEL_CRITICAL)
  {
    CLog::Log(LOGWARNING, "CAndroidPowerManager: Battery level critical (%d%%), applying emergency power settings", batteryLevel);
    
    // Force lowest refresh rate
    if (CServiceBroker::GetWinSystem() && CServiceBroker::GetSettingsComponent())
    {
      CServiceBroker::GetSettingsComponent()->GetSettings()->SetInt(
         "videoscreen.limitedrefreshrate", 30); 
    }
  }
}

void CAndroidPowerManager::SetPowerSavingMode(bool enable)
{
  // Never enable power saving for Android TV devices
  if (enable && !IsMobileDevice())
    return;
    
  if (m_powerSavingEnabled == enable)
    return;
    
  m_powerSavingEnabled = enable;
  
  // Apply Android power saving settings
  ApplyPowerSavingSettings(enable);
  
  // Configure performance profile
  ConfigurePerformanceProfile(enable);
  
  // Update refresh rate for power saving
  UpdateRefreshRate();
  
  CLog::Log(LOGINFO, "CAndroidPowerManager: Power saving mode %s", enable ? "enabled" : "disabled");
}

bool CAndroidPowerManager::IsPowerSavingModeActive()
{
  return m_powerSavingEnabled;
}

void CAndroidPowerManager::UpdateRefreshRate()
{
  if (!CServiceBroker::GetWinSystem() || !CServiceBroker::GetSettingsComponent())
    return;
    
  if (m_powerSavingEnabled && IsMobileDevice())
  {
    // Limit refresh rate to save power
    // Store current refresh rate setting first
    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    int currentRate = settings->GetInt("videoscreen.limitedrefreshrate");
    
    // In power saving mode, limit to 30fps or lower if possible
    if (currentRate > 30)
    {
      settings->SetInt("videoscreen.limitedrefreshrate", 30);
      CLog::Log(LOGINFO, "CAndroidPowerManager: Limiting refresh rate to 30Hz for power saving");
    }
  }
}

void CAndroidPowerManager::ConfigurePerformanceProfile(bool powerSaving)
{
  // Skip power saving configuration for Android TV
  if (powerSaving && !IsMobileDevice())
    return;
    
  // Get Android API level
  int apiLevel = CJNIBuild::SDK_INT;
  
  JNIEnv* env = xbmc_jnienv();
  if (!env)
    return;

  try
  {
    // Use PowerManager to optimize performance
    CJNIPowerManager powerManager(CJNIContext::getSystemService(CJNIContext::POWER_SERVICE));
    if (!powerManager)
      return;

    if (apiLevel >= 21)  // Lollipop and higher
    {
      if (powerSaving)
      {
        // Request low power mode if API supports it (API 21+)
        if (apiLevel >= 28)  // Android P and higher
        {
          jobject activity = CXBMCApp::Get().getActivity();
          jclass activityClass = env->GetObjectClass(activity);
          jmethodID setPowerSaveMode = env->GetMethodID(activityClass, "setPowerSaveMode", "(Z)Z");
          
          if (setPowerSaveMode != NULL)
          {
            jboolean result = env->CallBooleanMethod(activity, setPowerSaveMode, JNI_TRUE);
            CLog::Log(LOGDEBUG, "CAndroidPowerManager: setPowerSaveMode returned %s", (result == JNI_TRUE) ? "true" : "false");
          }
          
          env->DeleteLocalRef(activityClass);
        }
      }
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidPowerManager: Exception configuring performance profile: %s", e.what());
  }
  
  // Configure rendering system for power optimizations
  CRenderSystemBase* renderSystem = CServiceBroker::GetRenderSystem();
  if (renderSystem)
  {
    if (powerSaving)
    {
      // Lower precision settings
      renderSystem->SetShadersEnabled(false);
    }
    else
    {
      // Restore regular shader settings
      renderSystem->SetShadersEnabled(true);
    }
  }
}

void CAndroidPowerManager::HandleThermalThrottling(float temperature)
{
  // Android TV doesn't need thermal throttling from us
  if (!IsMobileDevice())
    return;
    
  // Android will handle most thermal throttling automatically,
  // but we can help by reducing our workload when temperature is high
  
  // Threshold temperatures (example values in °C)
  const float TEMP_WARNING = 40.0f;
  const float TEMP_CRITICAL = 45.0f;
  
  if (temperature > TEMP_CRITICAL)
  {
    // Critical temperature - take immediate action
    CLog::Log(LOGWARNING, "CAndroidPowerManager: Critical temperature detected (%.1f°C), applying emergency throttling", temperature);
    
    // Force power saving mode
    if (!m_powerSavingEnabled)
    {
      SetPowerSavingMode(true);
    }
    
    // Reduce resolution to minimum if needed
    if (CServiceBroker::GetRenderSystem())
    {
      // Reduce resolution scaling factor
      CServiceBroker::GetRenderSystem()->SetScalingResolution(720, 480, true);
    }
  }
  else if (temperature > TEMP_WARNING)
  {
    CLog::Log(LOGINFO, "CAndroidPowerManager: High temperature detected (%.1f°C), enabling power saving mode", temperature);
    
    // Enable power saving if not already enabled
    if (!m_powerSavingEnabled)
    {
      SetPowerSavingMode(true);
    }
  }
}

void CAndroidPowerManager::ApplyPowerSavingSettings(bool enable)
{
  // Skip for Android TV
  if (!IsMobileDevice())
    return;
    
  if (enable)
  {
    // Power saving settings
    if (CServiceBroker::GetSettingsComponent())
    {
      auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
      
      // Disable or reduce any power-intensive features
      settings->SetBool("videoplayer.usedisplayasclock", false);  // Use less precise but more efficient timing method
      settings->SetBool("videoplayer.limitguiupdate", true);      // Limit GUI updates during playback
      
      // Reduce background activity
      settings->SetInt("general.addonupdates", 0);                // Disable auto updates of add-ons
    }
  }
  else
  {
    // Restore normal settings
    if (CServiceBroker::GetSettingsComponent())
    {
      auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
      
      // Restore default settings for these options
      settings->SetBool("videoplayer.usedisplayasclock", true);
      settings->SetBool("videoplayer.limitguiupdate", false);
      
      // Reset add-on update setting to default
      settings->SetInt("general.addonupdates", 1);
    }
  }
}

float CAndroidPowerManager::GetDeviceTemperature()
{
  float temperature = 0.0f;
  
  JNIEnv* env = xbmc_jnienv();
  if (!env)
    return temperature;
    
  try
  {
    // Get the activity instance
    jobject activity = CXBMCApp::Get().getActivity();
    jclass activityClass = env->GetObjectClass(activity);
    
    // Try to call a helper method to get temperature
    jmethodID getTemperatureMethod = env->GetMethodID(activityClass, "getBatteryTemperature", "()F");
    
    if (getTemperatureMethod != NULL)
    {
      temperature = env->CallFloatMethod(activity, getTemperatureMethod);
      
      // Convert from tenths of a degree to degrees if needed
      if (temperature > 1000)
        temperature /= 10.0f;
    }
    
    env->DeleteLocalRef(activityClass);
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidPowerManager: Exception getting device temperature: %s", e.what());
  }
  
  return temperature;
}

AndroidDeviceFormFactor CAndroidPowerManager::GetDeviceFormFactor()
{
  if (m_deviceFormFactor == DEVICE_FORM_FACTOR_UNKNOWN)
    m_deviceFormFactor = DetectDeviceFormFactor();
    
  return m_deviceFormFactor;
}

bool CAndroidPowerManager::IsMobileDevice()
{
  AndroidDeviceFormFactor formFactor = GetDeviceFormFactor();
  return (formFactor == DEVICE_FORM_FACTOR_PHONE || formFactor == DEVICE_FORM_FACTOR_TABLET);
}

AndroidDeviceFormFactor CAndroidPowerManager::DetectDeviceFormFactor()
{
  try
  {
    // First check if we're on Android TV
    CJNIUiModeManager uiModeManager(CJNIContext::getSystemService(CJNIContext::UI_MODE_SERVICE));
    if (uiModeManager.getCurrentModeType() == CJNIUiModeManager::CONFIGURATION_UI_MODE_TYPE_TELEVISION)
    {
      CLog::Log(LOGINFO, "CAndroidPowerManager: Detected Android TV device");
      return DEVICE_FORM_FACTOR_TV;
    }
    
    // Next try to determine if this is a phone or tablet
    CJNIConfiguration config = CJNIContext::getResources().getConfiguration();
    int screenLayout = config.getScreenLayout() & CJNIConfiguration::SCREENLAYOUT_SIZE_MASK;
    
    if (screenLayout == CJNIConfiguration::SCREENLAYOUT_SIZE_XLARGE)
    {
      CLog::Log(LOGINFO, "CAndroidPowerManager: Detected tablet device (xlarge screen)");
      return DEVICE_FORM_FACTOR_TABLET;
    }
    else if (screenLayout == CJNIConfiguration::SCREENLAYOUT_SIZE_LARGE)
    {
      // Large screens are typically tablets but could be large phones
      // Check if telephony features are available as an additional hint
      CJNIPackageManager pm = CJNIContext::getPackageManager();
      if (pm.hasSystemFeature(CJNIPackageManager::FEATURE_TELEPHONY))
      {
        CLog::Log(LOGINFO, "CAndroidPowerManager: Detected phone device (large screen with telephony)");
        return DEVICE_FORM_FACTOR_PHONE;
      }
      else
      {
        CLog::Log(LOGINFO, "CAndroidPowerManager: Detected tablet device (large screen without telephony)");
        return DEVICE_FORM_FACTOR_TABLET;
      }
    }
    else
    {
      // Normal or small screens are phones
      CLog::Log(LOGINFO, "CAndroidPowerManager: Detected phone device (normal/small screen)");
      return DEVICE_FORM_FACTOR_PHONE;
    }
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "CAndroidPowerManager: Exception detecting device type: %s", e.what());
    return DEVICE_FORM_FACTOR_UNKNOWN;
  }
}
/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <jni.h>
#include <memory>
#include <string>
#include <atomic>
#include "powermanagement/IPowerManager.h"
#include "threads/Timer.h"

// Battery levels for different power management profiles
#define BATTERY_LEVEL_LOW 15
#define BATTERY_LEVEL_CRITICAL 5

// Device form factor types
typedef enum {
  DEVICE_FORM_FACTOR_UNKNOWN = 0,
  DEVICE_FORM_FACTOR_PHONE = 1,
  DEVICE_FORM_FACTOR_TABLET = 2,
  DEVICE_FORM_FACTOR_TV = 3
} AndroidDeviceFormFactor;

class CAndroidPowerManager
{
public:
  /**
   * @brief Initialize the Android power manager
   */
  static void Initialize();

  /**
   * @brief Update power saving mode based on battery level and charging state
   * @param batteryLevel Current battery level (0-100)
   * @param isCharging Whether the device is charging
   */
  static void UpdatePowerState(int batteryLevel, bool isCharging);
  
  /**
   * @brief Enable or disable power saving mode
   * @param enable True to enable, false to disable
   */
  static void SetPowerSavingMode(bool enable);
  
  /**
   * @brief Check if power saving mode is currently active
   * @return True if power saving mode is active
   */
  static bool IsPowerSavingModeActive();
  
  /**
   * @brief Update refresh rate based on power profile (lower refresh rate when battery is low)
   */
  static void UpdateRefreshRate();
  
  /**
   * @brief Configure CPU and GPU performance profiles based on power state
   * @param powerSaving True for power saving profile, false for performance profile
   */
  static void ConfigurePerformanceProfile(bool powerSaving);
  
  /**
   * @brief Handle thermal throttling to prevent overheating
   * @param temperature Current device temperature
   */
  static void HandleThermalThrottling(float temperature);

  /**
   * @brief Get the device form factor (phone, tablet, TV)
   * @return Form factor type
   */
  static AndroidDeviceFormFactor GetDeviceFormFactor();
  
  /**
   * @brief Check if this is a mobile device (phone or tablet)
   * @return True if mobile device, false if TV
   */
  static bool IsMobileDevice();

private:
  /**
   * @brief Apply Android-specific power saving settings via JNI
   * @param enable Whether to enable power saving mode
   */
  static void ApplyPowerSavingSettings(bool enable);
  
  /**
   * @brief Get current temperature via JNI thermal service
   * @return Current temperature in Celsius
   */
  static float GetDeviceTemperature();

  /**
   * @brief Detect the Android device form factor
   * @return The form factor enum value
   */
  static AndroidDeviceFormFactor DetectDeviceFormFactor();

  static std::atomic<bool> m_powerSavingEnabled;
  static int m_lastBatteryLevel;
  static bool m_isCharging;
  static AndroidDeviceFormFactor m_deviceFormFactor;
};
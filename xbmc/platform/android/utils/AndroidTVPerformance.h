/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include "utils/Stopwatch.h"
#include "threads/Timer.h"
#include "threads/CriticalSection.h"

// Performance modes
#define TV_PERFORMANCE_MODE_STANDARD 0
#define TV_PERFORMANCE_MODE_OPTIMIZED 1
#define TV_PERFORMANCE_MODE_MAX_PERFORMANCE 2

// Memory management thresholds for Android TV
#define TV_MEMORY_THRESHOLD_HIGH 80  // Percent of max memory
#define TV_MEMORY_THRESHOLD_LOW 50   // Percent of max memory

class CAndroidTVPerformance
{
public:
  /**
   * @brief Initialize the Android TV performance manager
   */
  static void Initialize();

  /**
   * @brief Set the performance mode of the device
   * @param mode The performance mode to set
   */
  static void SetPerformanceMode(int mode);

  /**
   * @brief Get the current performance mode
   * @return The current performance mode
   */
  static int GetPerformanceMode();

  /**
   * @brief Optimize video playback performance
   * @param videoWidth Video width in pixels
   * @param videoHeight Video height in pixels
   * @param frameRate Video frame rate
   * @param isHDR Whether the video is HDR
   */
  static void OptimizeVideoPlayback(int videoWidth, int videoHeight, float frameRate, bool isHDR);

  /**
   * @brief Restore settings after video playback
   */
  static void RestoreAfterVideoPlayback();

  /**
   * @brief Enable or disable framerate switching for video playback
   * @param enabled Whether framerate switching is enabled
   */
  static void EnableFramerateSwitching(bool enabled);

  /**
   * @brief Configure the CPU governor for TV performance
   * @param highPerformance Whether to use high performance mode
   */
  static void ConfigureCPUGovernor(bool highPerformance);

  /**
   * @brief Configure GPU settings for optimal TV performance
   * @param maxPerformance Whether to use maximum GPU performance
   */
  static void ConfigureGPUPerformance(bool maxPerformance);

  /**
   * @brief Memory management for Android TV
   * @param forceGc Whether to force garbage collection
   */
  static void ManageMemory(bool forceGc = false);

  /**
   * @brief Enable or disable HDMI-CEC features for better performance
   * @param enabled Whether CEC features should be enabled
   */
  static void EnableHDMICEC(bool enabled);

  /**
   * @brief Configure audio processing for optimal TV performance
   * @param digitalOutput Whether digital audio output is used
   */
  static void ConfigureAudio(bool digitalOutput);

  /**
   * @brief Update refresh rate for matching the content
   * @param fps The target frames per second
   * @return True if refresh rate was changed
   */
  static bool SetRefreshRate(float fps);

  /**
   * @brief Get available refresh rates on the device
   * @return Vector of available refresh rates
   */
  static std::vector<float> GetAvailableRefreshRates();

  /**
   * @brief Prioritize background processes for optimal TV performance
   * @param enableBackgroundPriority Whether to prioritize background processes
   */
  static void PrioritizeBackgroundProcesses(bool enableBackgroundPriority);

  /**
   * @brief Handle memory pressure situation
   * @param lowMemory Whether memory is critically low
   */
  static void HandleMemoryPressure(bool lowMemory);

  /**
   * @brief Optimize the GUI rendering for TV performance
   * @param optimized Whether GUI optimization is enabled
   */
  static void OptimizeGUIRendering(bool optimized);

private:
  /**
   * @brief Detect the available system resources
   */
  static void DetectSystemResources();

  /**
   * @brief Save the current settings for later restoration
   */
  static void SaveCurrentSettings();

  /**
   * @brief Apply settings after detecting system capabilities
   */
  static void ApplyOptimalSettings();

  /**
   * @brief Monitor system performance
   */
  static void MonitorPerformance();

  /**
   * @brief Check if the device has 4K capability
   * @return True if the device supports 4K output
   */
  static bool HasUHDCapability();

  /**
   * @brief Check if the device has HDR capability
   * @return True if the device supports HDR
   */
  static bool HasHDRCapability();

  /**
   * @brief Configure DRM secure playback
   */
  static void ConfigureDRMPlayback();

  static std::atomic<int> m_performanceMode;
  static std::atomic<bool> m_isInitialized;
  static std::atomic<bool> m_isVideoPlaying;
  static std::atomic<bool> m_framerateSwitchingEnabled;
  static CStopWatch m_performanceMonitor;
  static CCriticalSection m_settingsLock;
  static CTimer m_monitorTimer;

  // System capabilities
  static int m_cpuCores;
  static int m_gpuLevel;
  static int64_t m_totalMemory;
  static int m_androidVersion;
  static bool m_has4K;
  static bool m_hasHDR;
  static bool m_hasHLG;
  static bool m_hasDolbyVision;
  static bool m_hasOptimalRefreshRate;
};
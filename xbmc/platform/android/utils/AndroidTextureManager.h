/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureDatabase.h"
#include "ServiceBroker.h"
#include "guilib/TextureManager.h"

#include <memory>
#include <map>
#include <mutex>
#include <string>
#include <vector>

// Memory thresholds for different optimization strategies
#define MEM_THRESHOLD_LOW_MB 1024     // 1GB
#define MEM_THRESHOLD_MEDIUM_MB 2048  // 2GB
#define MEM_THRESHOLD_HIGH_MB 4096    // 4GB

// Maximum texture dimensions based on memory class
#define MAX_TEXTURE_WIDTH_FOR_LOW_MEM 1024
#define MAX_TEXTURE_HEIGHT_FOR_LOW_MEM 1024
#define MAX_TEXTURE_WIDTH_FOR_MEDIUM_MEM 2048
#define MAX_TEXTURE_HEIGHT_FOR_MEDIUM_MEM 2048

// Background texture load management
#define TEXTURE_LOAD_BATCH_SIZE 5
#define TEXTURE_LOAD_PAUSE_MS 16  // ~60fps

class CAndroidTextureManager
{
public:
  /**
   * @brief Initialize the Android texture manager
   */
  static void Initialize();

  /**
   * @brief Get device memory status in MB
   * @return Total memory in MB
   */
  static int GetDeviceMemoryClass();

  /**
   * @brief Add a texture to the cache with optimizations for Android devices
   * @param url Texture URL
   * @param details Texture details
   * @return True if successfully added
   */
  static bool AddCachedTextureOptimized(const std::string &url, const CTextureDetails &details);

  /**
   * @brief Load textures asynchronously for improved UI responsiveness
   * @param urls Vector of texture URLs to load
   * @param callback Callback to execute when textures are loaded
   */
  static void GetCachedTexturesAsync(const std::vector<std::string> &urls,
                                    std::function<void(std::map<std::string, CTextureDetails>)> callback);

  /**
   * @brief Pause background loading during UI animations or scrolling
   * @param pause True to pause, false to resume
   */
  static void PauseBackgroundLoading(bool pause);

  /**
   * @brief Cleanup texture cache based on Android device memory constraints
   */
  static void CleanupTextureCache();

private:
  /**
   * @brief Background worker function for texture loading
   * @param urls Texture URLs to load
   * @param callback Callback to execute when loading is complete
   */
  static void BackgroundTextureLoader(std::vector<std::string> urls,
                                     std::function<void(std::map<std::string, CTextureDetails>)> callback);

  static int m_deviceMemoryClass;
  static bool m_backgroundLoadingPaused;
  static std::mutex m_textureLock;
};
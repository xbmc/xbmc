/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaSection.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ADDON
{
enum class AddonType;
class CAddonMgr;
}

class CAdvancedSettings;

class CFileExtensionProvider
{
public:
  CFileExtensionProvider() = default;
  ~CFileExtensionProvider();

  void Initialize(ADDON::CAddonMgr& addonManager);
  void Deinitialize();

  /*!
   * @brief Returns a list of picture extensions
   */
  std::string GetPictureExtensions() const;

  /*!
   * @brief Returns a list of music extensions
   */
  std::string GetMusicExtensions() const;

  /*!
   * @brief Returns a list of video extensions
   */
  std::string GetVideoExtensions() const;

  /*!
   * @brief The extensions of the media \p section holds: video, music or pictures. Empty,
   * meaning every file, for the other sections.
   */
  std::string GetMediaExtensions(KODI::MEDIA::MediaSection section) const;

  /*!
   * @brief Returns a list of subtitle extensions
   */
  std::string GetSubtitleExtensions() const;

  /*!
   * @brief Returns a list of disc stub extensions
   */
  std::string GetDiscStubExtensions() const;

  /*!
   * @brief Returns a list of archive extensions with a single dot (eg. .zip)
   */
  std::string GetArchiveExtensions() const;

  /*!
   * @brief Returns a list of archive extensions with a multiple dots (eg. .tar.gz)
   */
  std::string GetCompoundArchiveExtensions() const;

  /*!
   * @brief Returns a file folder extensions
   */
  std::string GetFileFolderExtensions() const;

  /*!
   * @brief Returns whether a url protocol from add-ons use encoded hostnames
   */
  bool EncodedHostName(const std::string& protocol) const;

  /*!
   * @brief Returns true if related provider can operate related file
   *
   * @note Thought for cases e.g. by ISO, where can be a video or also a SACD.
   */
  static bool CanOperateExtension(const std::string& path);

  /*!
   * @brief Register game extensions
   */
  void RegisterGameExtensions(const std::set<std::string>& extensions);

  /*!
   * @brief Unregister game extensions
   */
  void UnregisterGameExtensions(const std::set<std::string>& extensions);

  /*!
   * @brief Returns a pipe-separated list of game extensions
   */
  std::string GetGameExtensions() const;

private:
  std::string GetAddonExtensions(ADDON::AddonType type) const;
  std::string GetAddonFileFolderExtensions(ADDON::AddonType type) const;
  void SetAddonExtensions();
  void SetAddonExtensions(ADDON::AddonType type);

  void OnAdvancedSettingsLoaded();

  /*!
   * \brief Release the cached lists built from the advanced settings.
   */
  void ReleaseSettingsDerivedLists();

  // Construction properties

  /*!
   * \brief Initialization status of the class, guards against access while uninitialized.
   */
  std::atomic<bool> m_initialized{false};
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  ADDON::CAddonMgr* m_addonManager{nullptr};
  std::optional<int> m_callbackId;

  mutable CCriticalSection m_critSection;

  // File extension properties
  std::map<ADDON::AddonType, std::string> m_addonExtensions;
  std::map<ADDON::AddonType, std::string> m_addonFileFolderExtensions;

  // Game extension registry
  std::set<std::string> m_gameExtensions;

  // Protocols from add-ons with encoded host names
  std::vector<std::string> m_encoded;

  /*!
   * @brief Atomically replaceable immutable string, used for the cached extension lists
   *
   * std::atomic_load/store of std::shared_ptr<T> is deprecated in C++20 and removed in C++26, and
   * gcc 16 warns on every use. The replacement, the std::atomic<std::shared_ptr<T>> partial
   * specialization (P0718), is not available everywhere yet: libstdc++ only defines
   * __cpp_lib_atomic_shared_ptr from gcc 12, and libc++ does not provide it at all as of LLVM 18
   * (verified against libc++ 18 and current Xcode), where std::atomic<T> instead static_asserts on
   * the non-trivially-copyable T. This class hides the difference behind load()/store() and falls
   * back to the deprecated free functions only where the specialization is unavailable, so the
   * warning disappears on modern toolchains without breaking the older ones.
   */
public:
  class CAtomicStringPtr
  {
  public:
    std::shared_ptr<const std::string> load(
        std::memory_order order = std::memory_order_seq_cst) const
    {
#if defined(__cpp_lib_atomic_shared_ptr)
      return m_value.load(order);
#else
      return std::atomic_load_explicit(&m_value, order);
#endif
    }

    void store(std::shared_ptr<const std::string> value,
               std::memory_order order = std::memory_order_seq_cst)
    {
#if defined(__cpp_lib_atomic_shared_ptr)
      m_value.store(std::move(value), order);
#else
      std::atomic_store_explicit(&m_value, std::move(value), order);
#endif
    }

  private:
#if defined(__cpp_lib_atomic_shared_ptr)
    mutable std::atomic<std::shared_ptr<const std::string>> m_value;
#else
    mutable std::shared_ptr<const std::string> m_value;
#endif
  };

private:
  // Cached extension lists, copied and published atomically. See CAtomicStringPtr.
  mutable CAtomicStringPtr m_discStubExtensions;
  mutable CAtomicStringPtr m_musicExtensions;
  mutable CAtomicStringPtr m_pictureExtensions;
  mutable CAtomicStringPtr m_subtitlesExtensions;
  mutable CAtomicStringPtr m_videoExtensions;
  mutable CAtomicStringPtr m_archiveExtensions;
  mutable CAtomicStringPtr m_compoundArchiveExtensions;
  mutable CAtomicStringPtr m_fileFolderExtensions;
};

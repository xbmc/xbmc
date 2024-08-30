/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_providers.h" // PVR_PROVIDER_INVALID_UID
#include "pvr/PVRConstants.h" // PVR_CLIENT_INVALID_UID

#include <string>

namespace PVR
{
class CPVRProvidersPath
{
public:
  static const std::string PATH_TV_PROVIDERS;
  static const std::string PATH_RADIO_PROVIDERS;
  static const std::string CHANNELS;
  static const std::string RECORDINGS;

  enum class Kind
  {
    UNKNOWN,
    RADIO,
    TV
  };

  explicit CPVRProvidersPath(const std::string& path);
  CPVRProvidersPath(Kind kind, int clientId, int providerUid, const std::string& lastSegment = "");

  bool IsValid() const { return m_isValid; }

  operator std::string() const { return m_path; }

  const std::string& GetPath() const { return m_path; }
  bool IsProvidersRoot() const { return m_isRoot; }
  bool IsProvider() const { return m_isProvider; }
  bool IsChannels() const { return m_isChannels; }
  bool IsRecordings() const { return m_isRecordings; }
  bool IsRadio() const { return m_kind == Kind::RADIO; }
  Kind GetKind() const { return m_kind; }
  int GetProviderUid() const { return m_providerUid; }
  int GetClientId() const { return m_clientId; }

private:
  bool Init(const std::string& path);

  std::string m_path;
  bool m_isValid{false};
  bool m_isRoot{false};
  bool m_isProvider{false};
  bool m_isChannels{false};
  bool m_isRecordings{false};
  Kind m_kind{Kind::UNKNOWN};
  int m_providerUid{PVR_PROVIDER_INVALID_UID};
  int m_clientId{PVR_CLIENT_INVALID_UID};
};
} // namespace PVR

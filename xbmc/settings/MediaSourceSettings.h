/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaSource.h"
#include "media/MediaSection.h"
#include "settings/lib/ISettingsHandler.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

class CProfileManager;

namespace tinyxml2
{
class XMLNode;
}

class CMediaSourceSettings : public ISettingsHandler
{
public:
  static CMediaSourceSettings& GetInstance();

  static std::string GetSourcesFile();

  void OnSettingsLoaded() override;
  void OnSettingsUnloaded() override;

  bool Load();
  bool Load(const std::string &file);
  bool Save() const;
  bool Save(const std::string &file) const;
  void Clear();

  std::vector<CMediaSource>& GetSources(KODI::MEDIA::MediaSection section);
  const std::string& GetDefaultSource(KODI::MEDIA::MediaSection section) const;
  void SetDefaultSource(KODI::MEDIA::MediaSection section, std::string_view source);
  static bool HasDefaultSource(KODI::MEDIA::MediaSection section);

  bool UpdateSource(KODI::MEDIA::MediaSection section,
                    std::string_view strOldName,
                    std::string_view strUpdateChild,
                    const std::string& strUpdateValue);
  bool DeleteSource(KODI::MEDIA::MediaSection section,
                    std::string_view strName,
                    std::string_view strPath,
                    bool virtualSource = false);
  bool AddShare(KODI::MEDIA::MediaSection section, const CMediaSource& share);
  bool UpdateShare(KODI::MEDIA::MediaSection section,
                   std::string_view oldName,
                   const CMediaSource& share);

protected:
  CMediaSourceSettings();
  CMediaSourceSettings(const CMediaSourceSettings&) = delete;
  CMediaSourceSettings& operator=(CMediaSourceSettings const&) = delete;
  ~CMediaSourceSettings() override;

private:
  struct SectionSources
  {
    std::vector<CMediaSource> sources;
    std::string defaultSource;
  };

  bool GetSource(KODI::MEDIA::MediaSection section,
                 const tinyxml2::XMLNode* source,
                 CMediaSource& share) const;
  void GetSources(const tinyxml2::XMLNode* rootElement,
                  KODI::MEDIA::MediaSection section,
                  SectionSources& sources) const;
  bool SetSources(tinyxml2::XMLNode* rootNode,
                  KODI::MEDIA::MediaSection section,
                  const SectionSources& sources) const;

  SectionSources& At(KODI::MEDIA::MediaSection section);
  const SectionSources& At(KODI::MEDIA::MediaSection section) const;

  std::array<SectionSources, KODI::MEDIA::MEDIA_SECTIONS.size()> m_sections;
};

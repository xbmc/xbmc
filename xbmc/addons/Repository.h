/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Addon.h"
#include "utils/Digest.h"
#include "utils/ProgressJob.h"

#include <memory>
#include <string>
#include <vector>

namespace ADDON
{
  class CRepository : public CAddon
  {
  public:
    struct DirInfo
    {
      AddonVersion version{""};
      std::string info;
      std::string checksum;
      KODI::UTILITY::CDigest::Type checksumType{KODI::UTILITY::CDigest::Type::INVALID};
      std::string datadir;
      std::string artdir;
      KODI::UTILITY::CDigest::Type hashType{KODI::UTILITY::CDigest::Type::INVALID};
    };

    typedef std::vector<DirInfo> DirList;

    explicit CRepository(const AddonInfoPtr& addonInfo);

    enum FetchStatus
    {
      STATUS_OK,
      STATUS_NOT_MODIFIED,
      STATUS_ERROR
    };

    FetchStatus FetchIfChanged(const std::string& oldChecksum,
                               std::string& checksum,
                               VECADDONS& addons,
                               int& recheckAfter) const;

    struct ResolveResult
    {
      std::string location;
      KODI::UTILITY::TypedDigest digest;
    };
    ResolveResult ResolvePathAndHash(AddonPtr const& addon) const;

  private:
    static bool FetchChecksum(const std::string& url,
                              std::string& checksum,
                              int& recheckAfter) noexcept;
    static bool FetchIndex(const DirInfo& repo, std::string const& digest, VECADDONS& addons) noexcept;

    static DirInfo ParseDirConfiguration(const CAddonExtensions& configuration);

    DirList m_dirs;
  };

  typedef std::shared_ptr<CRepository> RepositoryPtr;


  class CRepositoryUpdateJob : public CProgressJob
  {
  public:
    explicit CRepositoryUpdateJob(const RepositoryPtr& repo);
    ~CRepositoryUpdateJob() override = default;
    bool DoWork() override;
    const RepositoryPtr& GetAddon() const { return m_repo; };

  private:
    const RepositoryPtr m_repo;
  };
}


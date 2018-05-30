/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Addon.h"
#include "utils/Digest.h"
#include "utils/Job.h"
#include "utils/ProgressJob.h"

struct cp_cfg_element_t;

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

    static std::unique_ptr<CRepository> FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext);

    explicit CRepository(CAddonInfo addonInfo) : CAddon(std::move(addonInfo)) {};
    CRepository(CAddonInfo addonInfo, DirList dirs);

    /*! \brief Get the md5 hash for an addon.
     \param the addon in question.
     */
    bool GetAddonHash(const AddonPtr& addon, std::string& checksum) const;

    enum FetchStatus
    {
      STATUS_OK,
      STATUS_NOT_MODIFIED,
      STATUS_ERROR
    };

    FetchStatus FetchIfChanged(const std::string& oldChecksum, std::string& checksum, VECADDONS& addons) const;

    struct ResolveResult
    {
      std::string location;
      KODI::UTILITY::TypedDigest digest;
    };
    ResolveResult ResolvePathAndHash(AddonPtr const& addon) const;

  private:
    static bool FetchChecksum(const std::string& url, std::string& checksum) noexcept;
    static bool FetchIndex(const DirInfo& repo, std::string const& digest, VECADDONS& addons) noexcept;

    static DirInfo ParseDirConfiguration(cp_cfg_element_t* configuration);

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


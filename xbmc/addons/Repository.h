#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <memory>
#include <string>
#include <vector>

#include "Addon.h"
#include "utils/Job.h"
#include "utils/ProgressJob.h"

namespace ADDON
{
  class CRepository : public CAddon
  {
  public:
    struct DirInfo
    {
      DirInfo() : version("0.0.0"), hashes(false) {}
      AddonVersion version;
      std::string info;
      std::string checksum;
      std::string datadir;
      bool hashes;
    };

    typedef std::vector<DirInfo> DirList;

    static std::unique_ptr<CRepository> FromExtension(AddonProps props, const cp_extension_t* ext);

    explicit CRepository(AddonProps props) : CAddon(std::move(props)) {};
    CRepository(AddonProps props, DirList dirs);

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

  private:
    static bool FetchChecksum(const std::string& url, std::string& checksum) noexcept;
    static bool FetchIndex(const DirInfo& repo, VECADDONS& addons) noexcept;

    DirList m_dirs;
  };

  typedef std::shared_ptr<CRepository> RepositoryPtr;


  class CRepositoryUpdateJob : public CProgressJob
  {
  public:
    CRepositoryUpdateJob(const RepositoryPtr& repo);
    virtual ~CRepositoryUpdateJob() {}
    virtual bool DoWork();
    const RepositoryPtr& GetAddon() const { return m_repo; };

  private:
    const RepositoryPtr m_repo;
  };
}


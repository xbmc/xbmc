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

#include "Addon.h"
#include "utils/Job.h"
#include "utils/ProgressJob.h"

namespace ADDON
{
  class CRepository : public CAddon
  {
  public:
    virtual AddonPtr Clone() const;
    CRepository(const AddonProps& props);
    CRepository(const cp_extension_t *props);
    virtual ~CRepository();

    /*! \brief Get the md5 hash for an addon.
     \param the addon in question.
     \return the md5 hash for the given addon, empty if non exists.
     */
    std::string GetAddonHash(const AddonPtr& addon) const;

    struct DirInfo
    {
      DirInfo() : version("0.0.0"), compressed(false), zipped(false), hashes(false) {}
      AddonVersion version;
      std::string info;
      std::string checksum;
      std::string datadir;
      bool compressed;
      bool zipped;
      bool hashes;
    };

    typedef std::vector<DirInfo> DirList;
    DirList m_dirs;

    static bool Parse(const DirInfo& dir, VECADDONS& addons);
    static std::string FetchChecksum(const std::string& url);

  private:
    CRepository(const CRepository &rhs);

    static bool FetchIndex(const std::string& url, VECADDONS& addons);
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
    enum FetchStatus
    {
      STATUS_OK,
      STATUS_NOT_MODIFIED,
      STATUS_ERROR
    };

    FetchStatus FetchIfChanged(const std::string& oldChecksum,
        std::string& checksum, VECADDONS& addons);

    const RepositoryPtr m_repo;
  };
}


#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Addon.h"
#include "AddonManager.h"
#include "DateTime.h"
#include "URL.h"
#include "utils/Job.h"
#include "utils/CriticalSection.h"
#include "utils/SingleLock.h"

namespace ADDON
{
  class CRepository;
  typedef boost::shared_ptr<CRepository> RepositoryPtr;
  class CRepository : public CAddon
  {
  public:
    AddonPtr Clone(const AddonPtr &self) const;
    CRepository(const AddonProps& props);
    virtual ~CRepository();

    bool LoadFromXML(const CStdString& xml);
    CStdString Checksum();
    VECADDONS Parse();
    CDateTime LastUpdate();
    void SetUpdated(const CDateTime& time);
  private:
    CRepository(const CRepository&, const AddonPtr&);
    CStdString m_name;
    CStdString m_info;
    CStdString m_checksum;
    CStdString m_datadir;
    bool m_compressed; // gzipped info xml
    bool m_zipped;     // zipped addons
    CCriticalSection m_critSection;
  };

  class CRepositoryUpdateJob : public CJob
  {
  public:
    CRepositoryUpdateJob(RepositoryPtr& repo);
    virtual ~CRepositoryUpdateJob() {}

    virtual bool DoWork();
    static VECADDONS GrabAddons(RepositoryPtr& repo, bool check);

    RepositoryPtr m_repo;
  };
}


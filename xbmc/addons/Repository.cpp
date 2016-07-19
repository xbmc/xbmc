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

#include "Repository.h"

#include <iterator>
#include <utility>

#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "events/AddonManagementEvent.h"
#include "events/EventLog.h"
#include "FileItem.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/ZipFile.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

std::unique_ptr<CRepository> CRepository::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  DirList dirs;
  AddonVersion version("0.0.0");
  AddonPtr addonver;
  if (CAddonMgr::GetInstance().GetAddon("xbmc.addon", addonver))
    version = addonver->Version();
  for (size_t i = 0; i < ext->configuration->num_children; ++i)
  {
    if(ext->configuration->children[i].name &&
       strcmp(ext->configuration->children[i].name, "dir") == 0)
    {
      AddonVersion min_version(CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "@minversion"));
      if (min_version <= version)
      {
        DirInfo dir;
        dir.version = min_version;
        dir.checksum = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "checksum");
        dir.info = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "info");
        dir.datadir = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "datadir");
        dir.hashes = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "hashes") == "true";
        dirs.push_back(std::move(dir));
      }
    }
  }
  if (!CAddonMgr::GetInstance().GetExtValue(ext->configuration, "info").empty())
  {
    DirInfo info;
    info.checksum = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "checksum");
    info.info = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "info");
    info.datadir = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "datadir");
    info.hashes = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "hashes") == "true";
    dirs.push_back(std::move(info));
  }
  return std::unique_ptr<CRepository>(new CRepository(std::move(props), std::move(dirs)));
}

CRepository::CRepository(AddonProps props, DirList dirs)
    : CAddon(std::move(props)), m_dirs(std::move(dirs))
{
}


bool CRepository::GetAddonHash(const AddonPtr& addon, std::string& checksum) const
{
  DirList::const_iterator it;
  for (it = m_dirs.begin();it != m_dirs.end(); ++it)
    if (URIUtils::PathHasParent(addon->Path(), it->datadir, true))
      break;

  if (it != m_dirs.end())
  {
    if (!it->hashes)
    {
      checksum = "";
      return true;
    }
    if (FetchChecksum(addon->Path() + ".md5", checksum))
    {
      size_t pos = checksum.find_first_of(" \n");
      if (pos != std::string::npos)
      {
        checksum = checksum.substr(0, pos);
        return true;
      }
    }
  }
  return false;
}

bool CRepository::FetchChecksum(const std::string& url, std::string& checksum) noexcept
{
  CFile file;
  if (!file.Open(url))
    return false;

  // we intentionally avoid using file.GetLength() for
  // Transfer-Encoding: chunked servers.
  std::stringstream ss;
  char temp[1024];
  int read;
  while ((read = file.Read(temp, sizeof(temp))) > 0)
    ss.write(temp, read);
  if (read <= -1)
    return false;
  checksum = ss.str();
  return true;
}

bool CRepository::FetchIndex(const DirInfo& repo, VECADDONS& addons) noexcept
{
  XFILE::CCurlFile http;
  http.SetAcceptEncoding("gzip");

  std::string response;
  if (!http.Get(repo.info, response))
  {
    CLog::Log(LOGERROR, "CRepository: failed to read %s", repo.info.c_str());
    return false;
  }

  if (URIUtils::HasExtension(repo.info, ".gz")
      || CMime::GetFileTypeFromMime(http.GetMimeType()) == CMime::EFileType::FileTypeGZip)
  {
    CLog::Log(LOGDEBUG, "CRepository '%s' is gzip. decompressing", repo.info.c_str());
    std::string buffer;
    if (!CZipFile::DecompressGzip(response, buffer))
    {
      CLog::Log(LOGERROR, "CRepository: failed to decompress gzip from '%s'", repo.info.c_str());
      return false;
    }
    response = std::move(buffer);
  }

  return CAddonMgr::GetInstance().AddonsFromRepoXML(repo, response, addons);
}

CRepository::FetchStatus CRepository::FetchIfChanged(const std::string& oldChecksum,
    std::string& checksum, VECADDONS& addons) const
{
  checksum = "";
  for (const auto& dir : m_dirs)
  {
    if (!dir.checksum.empty())
    {
      std::string part;
      if (!FetchChecksum(dir.checksum, part))
      {
        CLog::Log(LOGERROR, "CRepository: failed read '%s'", dir.checksum.c_str());
        return STATUS_ERROR;
      }
      checksum += part;
    }
  }

  if (oldChecksum == checksum && !oldChecksum.empty())
    return STATUS_NOT_MODIFIED;

  for (const auto& dir : m_dirs)
  {
    VECADDONS tmp;
    if (!FetchIndex(dir, tmp))
      return STATUS_ERROR;
    addons.insert(addons.end(), tmp.begin(), tmp.end());
  }
  return STATUS_OK;
}

CRepositoryUpdateJob::CRepositoryUpdateJob(const RepositoryPtr& repo) : m_repo(repo) {}

bool CRepositoryUpdateJob::DoWork()
{
  CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] checking for updates.", m_repo->ID().c_str());
  CAddonDatabase database;
  database.Open();

  std::string oldChecksum;
  if (database.GetRepoChecksum(m_repo->ID(), oldChecksum) == -1)
    oldChecksum = "";

  std::string newChecksum;
  VECADDONS addons;
  auto status = m_repo->FetchIfChanged(oldChecksum, newChecksum, addons);

  database.SetLastChecked(m_repo->ID(), m_repo->Version(),
      CDateTime::GetCurrentDateTime().GetAsDBDateTime());

  MarkFinished();

  if (status == CRepository::STATUS_ERROR)
    return false;

  if (status == CRepository::STATUS_NOT_MODIFIED)
  {
    CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] checksum not changed.", m_repo->ID().c_str());
    return true;
  }

  //Invalidate art.
  {
    CTextureDatabase textureDB;
    textureDB.Open();
    textureDB.BeginMultipleExecute();

    for (const auto& addon : addons)
    {
      AddonPtr oldAddon;
      if (database.GetAddon(addon->ID(), oldAddon) && addon->Version() > oldAddon->Version())
      {
        if (!oldAddon->Icon().empty() || !oldAddon->FanArt().empty() || !oldAddon->Screenshots().empty())
          CLog::Log(LOGDEBUG, "CRepository: invalidating cached art for '%s'", addon->ID().c_str());
        if (!oldAddon->Icon().empty())
          textureDB.InvalidateCachedTexture(oldAddon->Icon());
        if (!oldAddon->FanArt().empty())
          textureDB.InvalidateCachedTexture(oldAddon->Icon());
        for (const auto& path : oldAddon->Screenshots())
          textureDB.InvalidateCachedTexture(path);
      }
    }
    textureDB.CommitMultipleExecute();
  }

  database.UpdateRepositoryContent(m_repo->ID(), m_repo->Version(), newChecksum, addons);

  //Update broken status
  database.BeginMultipleExecute();
  for (const auto& addon : addons)
  {
    AddonPtr localAddon;
    CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon);

    if (localAddon && localAddon->Version() > addon->Version())
      //We have a newer verison locally
      continue;

    if (database.GetAddonVersion(addon->ID()).first > addon->Version())
      //Newer version in db (ie. in a different repo)
      continue;

    std::string broken = addon->Broken();
    bool depsMet = CAddonInstaller::GetInstance().CheckDependencies(addon);
    if (!depsMet && broken.empty())
      broken = "DEPSNOTMET";

    if (localAddon)
    {
      bool brokenInDb = database.IsAddonBroken(addon->ID());
      if (!broken.empty() && !brokenInDb)
      {
        //newly broken
        int line = 24096;
        if (broken == "DEPSNOTMET")
          line = 24104;
        if (HELPERS::ShowYesNoDialogLines(CVariant{addon->Name()}, CVariant{line}, CVariant{24097}, CVariant{""}) 
          == DialogResponse::YES)
        {
          CAddonMgr::GetInstance().DisableAddon(addon->ID());
        }

        CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] addon '%s' marked broken. reason: \"%s\"",
             m_repo->ID().c_str(), addon->ID().c_str(), broken.c_str());

        CEventLog::GetInstance().Add(EventPtr(new CAddonManagementEvent(addon, 24096)));
      }
      else if (broken.empty() && brokenInDb)
      {
        //Unbroken
        CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] addon '%s' unbroken",
            m_repo->ID().c_str(), addon->ID().c_str());
      }
    }

    //Update broken status
    database.BreakAddon(addon->ID(), broken);
  }
  database.CommitMultipleExecute();
  return true;
}

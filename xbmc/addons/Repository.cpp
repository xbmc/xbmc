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

CRepository::CRepository(AddonInfoPtr addonInfo)
  : CAddon(addonInfo)
{
  for (auto element : Type(ADDON_REPOSITORY)->GetElements("dir"))
  {
    AddonVersion min_version(element.second.GetValue("@minversion").asString());
    if (min_version <= CAddonMgr::GetInstance().GetInstalledAddonInfo("xbmc.addon")->Version())
    {
      DirInfo dir;
      dir.version = min_version;
      dir.checksum = element.second.GetValue("checksum").asString();
      dir.info = element.second.GetValue("info").asString();
      dir.datadir = element.second.GetValue("datadir").asString();
      dir.hashes = element.second.GetValue("hashes").asBoolean();
      m_dirs.push_back(std::move(dir));
    }
  }
  if (!Type(ADDON_REPOSITORY)->GetValue("info").empty())
  {
    DirInfo info;
    info.checksum = Type(ADDON_REPOSITORY)->GetValue("checksum").asString();
    info.info = Type(ADDON_REPOSITORY)->GetValue("info").asString();
    info.datadir = Type(ADDON_REPOSITORY)->GetValue("datadir").asString();
    info.hashes = Type(ADDON_REPOSITORY)->GetValue("hashes").asBoolean();
    m_dirs.push_back(std::move(info));
  }
}

bool CRepository::GetAddonHash(const std::string& addonPath, std::string& checksum) const
{
  DirList::const_iterator it;
  for (it = m_dirs.begin();it != m_dirs.end(); ++it)
    if (URIUtils::PathHasParent(addonPath, it->datadir, true))
      break;

  if (it != m_dirs.end())
  {
    if (!it->hashes)
    {
      checksum = "";
      return true;
    }
    if (FetchChecksum(addonPath + ".md5", checksum))
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

bool CRepository::FetchIndex(const DirInfo& repo, AddonInfos& addons) noexcept
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

  return AddonsFromRepoXML(repo, response, addons);
}

bool CRepository::AddonsFromRepoXML(const CRepository::DirInfo& repo, const std::string& xml, AddonInfos& addonInfos)
{
  CXBMCTinyXML doc;
  if (!doc.Parse(xml))
  {
    CLog::Log(LOGERROR, "CRepository: Failed to parse addons.xml.");
    return false;
  }

  if (doc.RootElement() == nullptr || doc.RootElement()->ValueStr() != "addons")
  {
    CLog::Log(LOGERROR, "CRepository: Failed to parse addons.xml. Malformed.");
    return false;
  }

  auto element = doc.RootElement()->FirstChildElement("addon");
  while (element)
  {
    AddonInfoPtr props = std::make_shared<CAddonInfo>(element, repo.datadir);
    if (props->IsUsable())
      addonInfos.push_back(std::move(props));

    element = element->NextSiblingElement("addon");
  }

  return true;
}

CRepository::FetchStatus CRepository::FetchIfChanged(const std::string& oldChecksum,
    std::string& checksum, AddonInfos& addons) const
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
    AddonInfos tmp;
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
  AddonInfos addons;
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
      AddonInfoPtr oldAddon;
      if (database.GetAddonInfo(addon->ID(), oldAddon) && addon->Version() > oldAddon->Version())
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
  return true;
}

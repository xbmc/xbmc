/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Repository.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "filesystem/ZipFile.h"
#include "messaging/helpers/DialogHelper.h"
#include "utils/Base64.h"
#include "utils/Digest.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <tuple>
#include <utility>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;
using KODI::UTILITY::CDigest;
using KODI::UTILITY::TypedDigest;


CRepository::ResolveResult CRepository::ResolvePathAndHash(const AddonPtr& addon) const
{
  std::string const& path = addon->Path();

  auto dirIt = std::find_if(m_dirs.begin(), m_dirs.end(), [&path](DirInfo const& dir)
  {
    return URIUtils::PathHasParent(path, dir.datadir, true);
  });
  if (dirIt == m_dirs.end())
  {
    CLog::Log(LOGERROR, "Requested path {} not found in known repository directories", path);
    return {};
  }

  if (dirIt->hashType == CDigest::Type::INVALID)
  {
    // We have a path, but need no hash
    return {path, {}};
  }

  // Do not follow mirror redirect, we want the headers of the redirect response
  CURL url{path};
  url.SetProtocolOption("redirect-limit", "0");
  CCurlFile file;
  if (!file.Open(url))
  {
    CLog::Log(LOGERROR, "Could not fetch addon location and hash from {}", path);
    return {};
  }

  std::string hashTypeStr = CDigest::TypeToString(dirIt->hashType);

  // Return the location from the header so we don't have to look it up again
  // (saves one request per addon install)
  std::string location = file.GetRedirectURL();
  // content-* headers are base64, convert to base16
  TypedDigest hash{dirIt->hashType, StringUtils::ToHexadecimal(Base64::Decode(file.GetHttpHeader().GetValue(std::string("content-") + hashTypeStr)))};

  if (hash.Empty())
  {
    int tmp;
    // Expected hash, but none found -> fall back to old method
    if (!FetchChecksum(path + "." + hashTypeStr, hash.value, tmp) || hash.Empty())
    {
      CLog::Log(LOGERROR, "Failed to find hash for {} from HTTP header and in separate file", path);
      return {};
    }
  }
  if (location.empty())
  {
    // Fall back to original URL if we do not get a redirect
    location = path;
  }

  CLog::Log(LOGDEBUG, "Resolved addon path {} to {} hash {}", path, location, hash.value);

  return {location, hash};
}

CRepository::CRepository(const AddonInfoPtr& addonInfo)
  : CAddon(addonInfo, ADDON_REPOSITORY)
{
  DirList dirs;
  AddonVersion version;
  AddonInfoPtr addonver = CServiceBroker::GetAddonMgr().GetAddonInfo("xbmc.addon");
  if (addonver)
    version = addonver->Version();

  for (auto element : Type(ADDON_REPOSITORY)->GetElements("dir"))
  {
    DirInfo dir = ParseDirConfiguration(element.second);
    if (dir.version <= version)
      m_dirs.push_back(std::move(dir));
  }
  if (!Type(ADDON_REPOSITORY)->GetValue("info").empty())
  {
    m_dirs.push_back(ParseDirConfiguration(*Type(ADDON_REPOSITORY)));
  }

  for (auto const& dir : m_dirs)
  {
    CURL datadir(dir.datadir);
    if (datadir.IsProtocol("http"))
    {
      CLog::Log(LOGWARNING, "Repository add-on {} uses plain HTTP for add-on downloads in path {} - this is insecure and will make your Kodi installation vulnerable to attacks if enabled!", ID(), datadir.GetRedacted());
    }
    else if (datadir.IsProtocol("https") && datadir.HasProtocolOption("verifypeer") && datadir.GetProtocolOption("verifypeer") == "false")
    {
      CLog::Log(LOGWARNING, "Repository add-on {} disabled peer verification for add-on downloads in path {} - this is insecure and will make your Kodi installation vulnerable to attacks if enabled!", ID(), datadir.GetRedacted());
    }
  }
}

bool CRepository::FetchChecksum(const std::string& url,
                                std::string& checksum,
                                int& recheckAfter) noexcept
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
  std::size_t pos = checksum.find_first_of(" \n");
  if (pos != std::string::npos)
  {
    checksum = checksum.substr(0, pos);
  }

  // Determine update interval from (potential) HTTP response
  // Default: 24 h
  recheckAfter = 24 * 60 * 60;
  // This special header is set by the Kodi mirror redirector to control client update frequency
  // depending on the load on the mirrors
  const std::string recheckAfterHeader{
      file.GetProperty(FILE_PROPERTY_RESPONSE_HEADER, "X-Kodi-Recheck-After")};
  if (!recheckAfterHeader.empty())
  {
    try
    {
      // Limit value range to sensible values (1 hour to 1 week)
      recheckAfter =
          std::max(std::min(std::stoi(recheckAfterHeader), 24 * 7 * 60 * 60), 1 * 60 * 60);
    }
    catch (...)
    {
      CLog::Log(LOGWARNING, "Could not parse X-Kodi-Recheck-After header value '{}' from {}",
                recheckAfterHeader, url);
    }
  }

  return true;
}

bool CRepository::FetchIndex(const DirInfo& repo, std::string const& digest, VECADDONS& addons) noexcept
{
  XFILE::CCurlFile http;

  std::string response;
  if (!http.Get(repo.info, response))
  {
    CLog::Log(LOGERROR, "CRepository: failed to read %s", repo.info.c_str());
    return false;
  }

  if (repo.checksumType != CDigest::Type::INVALID)
  {
    std::string actualDigest = CDigest::Calculate(repo.checksumType, response);
    if (!StringUtils::EqualsNoCase(digest, actualDigest))
    {
      CLog::Log(LOGERROR, "CRepository: {} index has wrong digest {}, expected: {}", repo.info, actualDigest, digest);
      return false;
    }
  }

  if (URIUtils::HasExtension(repo.info, ".gz")
      || CMime::GetFileTypeFromMime(http.GetProperty(XFILE::FILE_PROPERTY_MIME_TYPE)) == CMime::EFileType::FileTypeGZip)
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

  return CServiceBroker::GetAddonMgr().AddonsFromRepoXML(repo, response, addons);
}

CRepository::FetchStatus CRepository::FetchIfChanged(const std::string& oldChecksum,
                                                     std::string& checksum,
                                                     VECADDONS& addons,
                                                     int& recheckAfter) const
{
  checksum = "";
  std::vector<std::tuple<DirInfo const&, std::string>> dirChecksums;
  std::vector<int> recheckAfterTimes;

  for (const auto& dir : m_dirs)
  {
    if (!dir.checksum.empty())
    {
      std::string part;
      int recheckAfterThisDir;
      if (!FetchChecksum(dir.checksum, part, recheckAfterThisDir))
      {
        CLog::Log(LOGERROR, "CRepository: failed read '%s'", dir.checksum.c_str());
        return STATUS_ERROR;
      }
      dirChecksums.emplace_back(dir, part);
      recheckAfterTimes.push_back(recheckAfterThisDir);
      checksum += part;
    }
  }

  // Default interval: 24 h
  recheckAfter = 24 * 60 * 60;
  if (dirChecksums.size() == m_dirs.size() && !dirChecksums.empty())
  {
    // Use smallest update interval out of all received (individual intervals per directory are
    // not possible)
    recheckAfter = *std::min_element(recheckAfterTimes.begin(), recheckAfterTimes.end());
    // If all directories have checksums and they match the last one, nothing has changed
    if (dirChecksums.size() == m_dirs.size() && oldChecksum == checksum)
      return STATUS_NOT_MODIFIED;
  }

  for (const auto& dirTuple : dirChecksums)
  {
    VECADDONS tmp;
    if (!FetchIndex(std::get<0>(dirTuple), std::get<1>(dirTuple), tmp))
      return STATUS_ERROR;
    addons.insert(addons.end(), tmp.begin(), tmp.end());
  }
  return STATUS_OK;
}

CRepository::DirInfo CRepository::ParseDirConfiguration(const CAddonExtensions& configuration)
{
  DirInfo dir;
  dir.checksum = configuration.GetValue("checksum").asString();
  std::string checksumStr = configuration.GetValue("checksum@verify").asString();
  if (!checksumStr.empty())
  {
    dir.checksumType = CDigest::TypeFromString(checksumStr);
  }
  dir.info = configuration.GetValue("info").asString();
  dir.datadir = configuration.GetValue("datadir").asString();
  dir.artdir = configuration.GetValue("artdir").asString();
  if (dir.artdir.empty())
  {
    dir.artdir = dir.datadir;
  }

  std::string hashStr = configuration.GetValue("hashes").asString();
  StringUtils::ToLower(hashStr);
  if (hashStr == "true")
  {
    // Deprecated alias
    hashStr = "md5";
  }
  if (!hashStr.empty() && hashStr != "false")
  {
    dir.hashType = CDigest::TypeFromString(hashStr);
    if (dir.hashType == CDigest::Type::MD5)
    {
      CLog::Log(LOGWARNING, "CRepository::{}: Repository has MD5 hashes enabled - this hash function is broken and will only guard against unintentional data corruption", __FUNCTION__);
    }
  }

  dir.version = AddonVersion{configuration.GetValue("@minversion").asString()};
  return dir;
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

  const CAddonDatabase::RepoUpdateData updateData{database.GetRepoUpdateData(m_repo->ID())};
  if (updateData.lastCheckedVersion != m_repo->Version())
    oldChecksum = "";

  std::string newChecksum;
  VECADDONS addons;
  int recheckAfter;
  auto status = m_repo->FetchIfChanged(oldChecksum, newChecksum, addons, recheckAfter);

  database.SetRepoUpdateData(
      m_repo->ID(), CAddonDatabase::RepoUpdateData(
                        CDateTime::GetCurrentDateTime(), m_repo->Version(),
                        CDateTime::GetCurrentDateTime() + CDateTimeSpan(0, 0, 0, recheckAfter)));

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
        if (!oldAddon->Icon().empty() || !oldAddon->Art().empty() || !oldAddon->Screenshots().empty())
          CLog::Log(LOGDEBUG, "CRepository: invalidating cached art for '%s'", addon->ID().c_str());

        if (!oldAddon->Icon().empty())
          textureDB.InvalidateCachedTexture(oldAddon->Icon());

        for (const auto& path : oldAddon->Screenshots())
          textureDB.InvalidateCachedTexture(path);

        for (const auto& art : oldAddon->Art())
          textureDB.InvalidateCachedTexture(art.second);
      }
    }
    textureDB.CommitMultipleExecute();
  }

  database.UpdateRepositoryContent(m_repo->ID(), m_repo->Version(), newChecksum, addons);
  return true;
}

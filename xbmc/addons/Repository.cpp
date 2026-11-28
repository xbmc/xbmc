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
#include "URL.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/File.h"
#include "filesystem/ZipFile.h"
#include "games/GameServices.h"
#include "messaging/helpers/DialogHelper.h"
#include "utils/Base64.h"
#include "utils/Digest.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <set>
#include <tuple>
#include <utility>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;
using KODI::UTILITY::CDigest;
using KODI::UTILITY::TypedDigest;

namespace
{
//! A repository is built each time add-ons are listed, so its configuration is reported only the
//! first time each version of it is built
bool IsFirstBuild(const std::string& id, const CAddonVersion& version)
{
  static std::mutex mutex;
  static std::set<std::pair<std::string, std::string>> built;
  std::scoped_lock lock(mutex);
  return built.emplace(id, version.asString()).second;
}
} // unnamed namespace

CRepository::ResolveResult CRepository::ResolvePathAndHash(const AddonPtr& addon) const
{
  std::string const& path = addon->Path();

  const auto dirIt =
      std::ranges::find_if(m_dirs, [&path](RepositoryDirInfo const& dir)
                           { return URIUtils::PathHasParent(path, dir.datadir, true); });
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
  XFILE::CFile file;
  if (!file.Open(url))
  {
    CLog::Log(LOGERROR, "Could not fetch addon location and hash from {}", path);
    return {};
  }

  std::string hashTypeStr = CDigest::TypeToString(dirIt->hashType);

  // Return the location from the header so we don't have to look it up again
  // (saves one request per addon install)
  std::string location = file.GetProperty(XFILE::FileProperty::REDIRECT_URL);
  // content-* headers are base64, convert to base16
  TypedDigest hash{dirIt->hashType,
                   StringUtils::ToHexadecimal(Base64::Decode(file.GetProperty(
                       XFILE::FileProperty::RESPONSE_HEADER, "content-" + hashTypeStr)))};

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

CRepository::CRepository(const AddonInfoPtr& addonInfo) : CAddon(addonInfo, AddonType::REPOSITORY)
{
  RepositoryDirList dirs;
  CAddonVersion version;
  AddonInfoPtr addonver =
      CServiceBroker::GetAddonMgr().GetAddonInfo("xbmc.addon", AddonType::UNKNOWN);
  if (addonver)
    version = addonver->Version();

  const bool firstBuild{IsFirstBuild(ID(), Version())};

  for (const auto& [_, addonExtensions] : Type(AddonType::REPOSITORY)->GetElements("dir"))
  {
    RepositoryDirInfo dir = ParseDirConfiguration(addonExtensions);
    if (firstBuild && dir.hashType == CDigest::Type::MD5)
    {
      CLog::LogF(LOGWARNING, "Repository has MD5 hashes enabled - this hash function is broken and "
                             "will only guard against unintentional data corruption");
    }
    if ((dir.minversion.empty() || version >= dir.minversion) &&
        (dir.maxversion.empty() || version <= dir.maxversion))
      m_dirs.emplace_back(std::move(dir));
  }

  if (!firstBuild)
  {
    return;
  }

  // old (dharma compatible) way of defining the addon repository structure, is no longer supported
  // we error out so the user knows how to migrate. The <dir> way is supported since gotham.
  //! @todo remove if block completely in v21
  if (!Type(AddonType::REPOSITORY)->GetValue("info").empty())
  {
    CLog::Log(LOGERROR,
              "Repository add-on {} uses old schema definition for the repository extension point! "
              "This is no longer supported, please update your addon to use <dir> definitions.",
              ID());
  }

  if (m_dirs.empty())
  {
    CLog::Log(LOGERROR,
              "Repository add-on {} does not have any directory matching {} and won't be able to "
              "update/serve addons! Please fix the addon.xml definition",
              ID(), version.asString());
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

void CRepository::OnPostInstall(bool update, bool modal)
{
  // The repo may contain game add-ons, which can introduce new file
  // extensions to the list of known game extensions
  CServiceBroker::GetGameServices().OnAddonRepoInstalled();
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
  ssize_t read;
  while ((read = file.Read(temp, sizeof(temp))) > 0)
    ss.write(temp, read);
  if (read <= -1)
    return false;
  checksum = std::move(ss).str();
  std::size_t pos = checksum.find_first_of(" \n");
  if (pos != std::string::npos)
  {
    checksum.resize(pos);
  }

  // Determine update interval from (potential) HTTP response
  // Default: 24 h
  recheckAfter = 24 * 60 * 60;
  // This special header is set by the Kodi mirror redirector to control client update frequency
  // depending on the load on the mirrors
  const std::string recheckAfterHeader{
      file.GetProperty(FileProperty::RESPONSE_HEADER, "X-Kodi-Recheck-After")};
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

bool CRepository::FetchIndex(const RepositoryDirInfo& repo,
                             std::string const& digest,
                             std::vector<AddonInfoPtr>& addons) noexcept
{
  XFILE::CFile http;

  std::vector<uint8_t> data;
  if (http.LoadFile(CURL(repo.info), data) <= 0)
  {
    CLog::Log(LOGERROR, "CRepository: failed to read {}", repo.info);
    return false;
  }

  std::string response(data.begin(), data.end());

  if (repo.checksumType != CDigest::Type::INVALID)
  {
    std::string actualDigest = CDigest::Calculate(repo.checksumType, response);
    if (!StringUtils::EqualsNoCase(digest, actualDigest))
    {
      CLog::Log(LOGERROR, "CRepository: {} index has wrong digest {}, expected: {}", repo.info, actualDigest, digest);
      return false;
    }
  }

  if (URIUtils::HasExtension(repo.info, ".gz") ||
      CMime::GetFileTypeFromMime(http.GetProperty(XFILE::FileProperty::MIME_TYPE)) ==
          CMime::EFileType::FileTypeGZip)
  {
    CLog::Log(LOGDEBUG, "CRepository '{}' is gzip. decompressing", repo.info);
    std::string buffer;
    if (!CZipFile::DecompressGzip(response, buffer))
    {
      CLog::Log(LOGERROR, "CRepository: failed to decompress gzip from '{}'", repo.info);
      return false;
    }
    response = std::move(buffer);
  }

  return CServiceBroker::GetAddonMgr().AddonsFromRepoXML(repo, response, addons);
}

CRepository::FetchStatus CRepository::FetchIfChanged(std::string_view oldChecksum,
                                                     std::string& checksum,
                                                     std::vector<AddonInfoPtr>& addons,
                                                     int& recheckAfter) const
{
  checksum = "";

  // Digests are stored per directory, separated by newlines (digests cannot
  // contain whitespace). A directory whose checksum cannot be fetched keeps its
  // previous digest for the change comparison only; the stored checksum records
  // an empty digest so its content is fetched again once it responds.
  const std::vector<std::string> oldDigests = StringUtils::Split(oldChecksum, '\n');

  struct DirState
  {
    RepositoryDirInfo const* dir;
    std::string digest;
    std::string compareDigest;
    bool fetched = false;
  };
  std::vector<DirState> states;
  std::vector<int> recheckAfterTimes;

  size_t dirIndex = 0;
  for (const auto& dir : m_dirs)
  {
    if (dir.checksum.empty())
      continue;

    DirState state{&dir, {}, {}, false};
    std::string part;
    int recheckAfterThisDir;
    if (FetchChecksum(dir.checksum, part, recheckAfterThisDir) && !part.empty())
    {
      state.digest = part;
      state.compareDigest = part;
      state.fetched = true;
      recheckAfterTimes.push_back(recheckAfterThisDir);
    }
    else
    {
      // Each directory lists its own add-ons with no fallback between them, so an
      // unreachable directory's add-ons drop out until it responds again
      CLog::Log(LOGWARNING, "CRepository: failed read '{}'", dir.checksum);
      if (dirIndex < oldDigests.size())
        state.compareDigest = oldDigests[dirIndex];
    }
    states.push_back(std::move(state));
    dirIndex++;
  }

  if (recheckAfterTimes.empty())
  {
    recheckAfter = 1 * 60 * 60; // retry after 1 hour
    return FetchStatus::FETCH_ERROR;
  }

  std::string compareChecksum;
  for (size_t i = 0; i < states.size(); i++)
  {
    if (i)
    {
      checksum += '\n';
      compareChecksum += '\n';
    }
    checksum += states[i].digest;
    compareChecksum += states[i].compareDigest;
  }

  // Default interval: 24 h, use the smallest interval out of all received
  // (individual intervals per directory are not possible)
  recheckAfter = *std::ranges::min_element(recheckAfterTimes);
  if (recheckAfterTimes.size() < states.size())
  {
    // A directory is unreachable, retry sooner so its content is not stale for long
    recheckAfter = std::min(recheckAfter, 1 * 60 * 60);
  }
  // If the checksums match the last ones, nothing has changed
  if (oldChecksum == compareChecksum)
    return FetchStatus::NOT_MODIFIED;

  for (const auto& state : states)
  {
    if (!state.fetched)
      continue;
    std::vector<AddonInfoPtr> tmp;
    if (!FetchIndex(*state.dir, state.digest, tmp))
      return FetchStatus::FETCH_ERROR;
    addons.insert(addons.end(), tmp.begin(), tmp.end());
  }
  return FetchStatus::OK;
}

RepositoryDirInfo CRepository::ParseDirConfiguration(const CAddonExtensions& configuration)
{
  RepositoryDirInfo dir;
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
  }

  dir.minversion = CAddonVersion{configuration.GetValue("@minversion").asString()};
  dir.maxversion = CAddonVersion{configuration.GetValue("@maxversion").asString()};

  return dir;
}

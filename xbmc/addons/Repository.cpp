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

AddonPtr CRepository::Clone() const
{
  return AddonPtr(new CRepository(*this));
}

CRepository::CRepository(const AddonProps& props) :
  CAddon(props)
{
}

CRepository::CRepository(const cp_extension_t *ext)
  : CAddon(ext)
{
  // read in the other props that we need
  if (ext)
  {
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
          dir.version    = min_version;
          dir.checksum   = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "checksum");
          dir.compressed = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "info@compressed") == "true";
          dir.info       = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "info");
          dir.datadir    = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "datadir");
          dir.zipped     = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "datadir@zip") == "true";
          dir.hashes     = CAddonMgr::GetInstance().GetExtValue(&ext->configuration->children[i], "hashes") == "true";
          m_dirs.push_back(dir);
        }
      }
    }
    // backward compatibility
    if (!CAddonMgr::GetInstance().GetExtValue(ext->configuration, "info").empty())
    {
      DirInfo info;
      info.checksum   = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "checksum");
      info.compressed = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "info@compressed") == "true";
      info.info       = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "info");
      info.datadir    = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "datadir");
      info.zipped     = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "datadir@zip") == "true";
      info.hashes     = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "hashes") == "true";
      m_dirs.push_back(info);
    }
  }
}

CRepository::CRepository(const CRepository &rhs)
  : CAddon(rhs), m_dirs(rhs.m_dirs)
{
}

CRepository::~CRepository()
{
}

std::string CRepository::FetchChecksum(const std::string& url)
{
  CFile file;
  try
  {
    if (file.Open(url))
    {    
      // we intentionally avoid using file.GetLength() for 
      // Transfer-Encoding: chunked servers.
      std::stringstream str;
      char temp[1024];
      int read;
      while ((read=file.Read(temp, sizeof(temp))) > 0)
        str.write(temp, read);
      return str.str();
    }
    return "";
  }
  catch (...)
  {
    return "";
  }
}

std::string CRepository::GetAddonHash(const AddonPtr& addon) const
{
  std::string checksum;
  DirList::const_iterator it;
  for (it = m_dirs.begin();it != m_dirs.end(); ++it)
    if (URIUtils::IsInPath(addon->Path(), it->datadir))
      break;
  if (it != m_dirs.end() && it->hashes)
  {
    checksum = FetchChecksum(addon->Path()+".md5");
    size_t pos = checksum.find_first_of(" \n");
    if (pos != std::string::npos)
      return checksum.substr(0, pos);
  }
  return checksum;
}

#define SET_IF_NOT_EMPTY(x,y) \
  { \
    if (!x.empty()) \
       x = y; \
  }


bool CRepository::FetchIndex(const std::string& url, VECADDONS& addons)
{
  XFILE::CCurlFile http;
  http.SetContentEncoding("gzip");

  std::string content;
  if (!http.Get(url, content))
    return false;

  if (URIUtils::HasExtension(url, ".gz")
      || CMime::GetFileTypeFromMime(http.GetMimeType()) == CMime::EFileType::FileTypeGZip)
  {
    CLog::Log(LOGDEBUG, "CRepository '%s' is gzip. decompressing", url.c_str());
    std::string buffer;
    if (!CZipFile::DecompressGzip(content, buffer))
      return false;
    content = std::move(buffer);
  }

  CXBMCTinyXML doc;
  if (!doc.Parse(content) || !doc.RootElement()
      || !CAddonMgr::GetInstance().AddonsFromRepoXML(doc.RootElement(), addons))
  {
    CLog::Log(LOGERROR, "CRepository: Failed to parse addons.xml. Malformated.");
    return false;
  }
  return true;
}

bool CRepository::Parse(const DirInfo& dir, VECADDONS& addons)
{
  if (!FetchIndex(dir.info, addons))
    return false;

    for (IVECADDONS i = addons.begin(); i != addons.end(); ++i)
    {
      AddonPtr addon = *i;
      if (dir.zipped)
      {
        std::string file = StringUtils::Format("%s/%s-%s.zip", addon->ID().c_str(), addon->ID().c_str(), addon->Version().asString().c_str());
        addon->Props().path = URIUtils::AddFileToFolder(dir.datadir,file);
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/icon.png"))
        file = StringUtils::Format("%s/changelog-%s.txt", addon->ID().c_str(), addon->Version().asString().c_str());
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(dir.datadir,file))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/fanart.jpg"))
      }
      else
      {
        addon->Props().path = URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/");
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/icon.png"))
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/changelog.txt"))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/fanart.jpg"))
      }
    }
  return true;
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
  auto status = FetchIfChanged(oldChecksum, newChecksum, addons);

  database.SetLastChecked(m_repo->ID(), m_repo->Version(),
      CDateTime::GetCurrentDateTime().GetAsDBDateTime());

  MarkFinished();

  if (status == STATUS_ERROR)
    return false;

  if (status == STATUS_NOT_MODIFIED)
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
        if (!addon->Props().icon.empty() || !addon->Props().fanart.empty())
          CLog::Log(LOGDEBUG, "CRepository: invalidating cached art for '%s'", addon->ID().c_str());
        if (!addon->Props().icon.empty())
          textureDB.InvalidateCachedTexture(addon->Props().icon);
        if (!addon->Props().fanart.empty())
          textureDB.InvalidateCachedTexture(addon->Props().fanart);
      }
    }
    textureDB.CommitMultipleExecute();
  }

  database.AddRepository(m_repo->ID(), addons, newChecksum, m_repo->Version());

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

    bool depsMet = CAddonInstaller::GetInstance().CheckDependencies(addon);
    if (!depsMet && addon->Props().broken.empty())
      addon->Props().broken = "DEPSNOTMET";

    if (localAddon)
    {
      bool brokenInDb = !database.IsAddonBroken(addon->ID()).empty();
      if (!addon->Props().broken.empty() && !brokenInDb)
      {
        //newly broken
        int line = 24096;
        if (addon->Props().broken == "DEPSNOTMET")
          line = 24104;
        if (HELPERS::ShowYesNoDialogLines(CVariant{addon->Name()}, CVariant{line}, CVariant{24097}, CVariant{""}) 
          == DialogResponse::YES)
        {
          CAddonMgr::GetInstance().DisableAddon(addon->ID());
        }

        CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] addon '%s' marked broken. reason: \"%s\"",
             m_repo->ID().c_str(), addon->ID().c_str(), addon->Props().broken.c_str());

        CEventLog::GetInstance().Add(EventPtr(new CAddonManagementEvent(addon, 24096)));
      }
      else if (addon->Props().broken.empty() && brokenInDb)
      {
        //Unbroken
        CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] addon '%s' unbroken",
            m_repo->ID().c_str(), addon->ID().c_str());
      }
    }

    //Update broken status
    database.BreakAddon(addon->ID(), addon->Props().broken);
  }
  database.CommitMultipleExecute();
  return true;
}

CRepositoryUpdateJob::FetchStatus CRepositoryUpdateJob::FetchIfChanged(const std::string& oldChecksum,
    std::string& checksum, VECADDONS& addons)
{
  SetText(StringUtils::Format(g_localizeStrings.Get(24093).c_str(), m_repo->Name().c_str()));
  const unsigned int total = m_repo->m_dirs.size() * 2;

  checksum = "";
  for (auto it = m_repo->m_dirs.cbegin(); it != m_repo->m_dirs.cend(); ++it)
  {
    if (!it->checksum.empty())
    {
      if (ShouldCancel(std::distance(m_repo->m_dirs.cbegin(), it), total))
        return STATUS_ERROR;

      auto dirsum = CRepository::FetchChecksum(it->checksum);
      if (dirsum.empty())
      {
        CLog::Log(LOGERROR, "CRepositoryUpdateJob[%s] failed read checksum for "
            "directory '%s'", m_repo->ID().c_str(), it->info.c_str());
        return STATUS_ERROR;
      }
      checksum += dirsum;
    }
  }

  if (oldChecksum == checksum && !oldChecksum.empty())
    return STATUS_NOT_MODIFIED;

  for (auto it = m_repo->m_dirs.cbegin(); it != m_repo->m_dirs.cend(); ++it)
  {
    if (ShouldCancel(m_repo->m_dirs.size() + std::distance(m_repo->m_dirs.cbegin(), it), total))
      return STATUS_ERROR;

    VECADDONS tmp;
    if (!CRepository::Parse(*it, tmp))
    {
      CLog::Log(LOGERROR, "CRepositoryUpdateJob[%s] failed to read or parse "
          "directory '%s'", m_repo->ID().c_str(), it->info.c_str());
      return STATUS_ERROR;
    }
    addons.insert(addons.end(), tmp.begin(), tmp.end());
  }

  SetProgress(total, total);
  return STATUS_OK;
}


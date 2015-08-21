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

#include <iterator>
#include "Repository.h"
#include "events/EventLog.h"
#include "events/AddonManagementEvent.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "FileItem.h"
#include "TextureDatabase.h"
#include "URL.h"

using namespace XFILE;
using namespace ADDON;

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

bool CRepository::Parse(const DirInfo& dir, VECADDONS &result)
{
  std::string file = dir.info;
  if (dir.compressed)
  {
    CURL url(dir.info);
    std::string opts = url.GetProtocolOptions();
    if (!opts.empty())
      opts += "&";
    url.SetProtocolOptions(opts+"Encoding=gzip");
    file = url.Get();
  }

  CXBMCTinyXML doc;
  if (doc.LoadFile(file) && doc.RootElement() &&
      CAddonMgr::GetInstance().AddonsFromRepoXML(doc.RootElement(), result))
  {
    for (IVECADDONS i = result.begin(); i != result.end(); ++i)
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
  return false;
}

void CRepository::OnPostInstall(bool update, bool modal)
{
  // Notify updater there is a new repo
  CRepositoryUpdater::GetInstance().ScheduleUpdate();
}

void CRepository::OnPostUnInstall()
{
  CAddonDatabase database;
  database.Open();
  database.DeleteRepository(ID());
}

CRepositoryUpdateJob::CRepositoryUpdateJob(const RepositoryPtr& repo) : m_repo(repo) {}

void MergeAddons(std::map<std::string, AddonPtr> &addons, const VECADDONS &new_addons)
{
  for (VECADDONS::const_iterator it = new_addons.begin(); it != new_addons.end(); ++it)
  {
    std::map<std::string, AddonPtr>::iterator existing = addons.find((*it)->ID());
    if (existing != addons.end())
    { // already got it - replace if we have a newer version
      if (existing->second->Version() < (*it)->Version())
        existing->second = *it;
    }
    else
      addons.insert(make_pair((*it)->ID(), *it));
  }
}

bool CRepositoryUpdateJob::DoWork()
{
  CLog::Log(LOGDEBUG, "CRepositoryUpdateJob[%s] checking for updates.", m_repo->ID().c_str());
  CAddonDatabase database;
  database.Open();

  std::string oldChecksum;
  if (!database.GetRepoChecksum(m_repo->ID(), oldChecksum))
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

  database.AddRepository(m_repo->ID(), addons, newChecksum, m_repo->Version());

  //Invalidate art. FIXME: this will cause a lot of unnecessary re-caching and
  //unnecessary HEAD requests being sent to server. icons and fanart rarely
  //change, and cannot change if there is no version bump.
  {
    CTextureDatabase textureDB;
    textureDB.Open();
    textureDB.BeginMultipleExecute();

    for (const auto& addon : addons)
    {
      if (!addon->Props().fanart.empty())
        textureDB.InvalidateCachedTexture(addon->Props().fanart);
      if (!addon->Props().icon.empty())
        textureDB.InvalidateCachedTexture(addon->Props().icon);
    }
    textureDB.CommitMultipleExecute();
  }

  //Update broken status
  database.BeginMultipleExecute();
  for (const auto& addon : addons)
  {
    AddonPtr localAddon;
    CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon);

    if (localAddon && localAddon->Version() > addon->Version())
      //We have a newer verison locally
      continue;

    if (database.GetAddonVersion(addon->ID()) > addon->Version())
      //Newer verison in db (ie. in a different repo)
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
        std::string line = g_localizeStrings.Get(24096);
        if (addon->Props().broken == "DEPSNOTMET")
          line = g_localizeStrings.Get(24104);
        if (CGUIDialogYesNo::ShowAndGetInput(CVariant{addon->Name()}, CVariant{line}, CVariant{24097}, CVariant{""}))
          CAddonMgr::GetInstance().DisableAddon(addon->ID());

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

  std::map<std::string, AddonPtr> uniqueAddons;
  for (auto it = m_repo->m_dirs.cbegin(); it != m_repo->m_dirs.cend(); ++it)
  {
    if (ShouldCancel(m_repo->m_dirs.size() + std::distance(m_repo->m_dirs.cbegin(), it), total))
      return STATUS_ERROR;

    VECADDONS addons;
    if (!CRepository::Parse(*it, addons))
    {
      CLog::Log(LOGERROR, "CRepositoryUpdateJob[%s] failed to read or parse "
          "directory '%s'", m_repo->ID().c_str(), it->info.c_str());
      return STATUS_ERROR;
    }
    MergeAddons(uniqueAddons, addons);
  }

  for (const auto& kv : uniqueAddons)
    addons.push_back(kv.second);

  SetProgress(total, total);
  return STATUS_OK;
}


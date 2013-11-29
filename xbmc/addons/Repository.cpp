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
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "filesystem/PluginDirectory.h"
#include "pvr/PVRManager.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "FileItem.h"
#include "TextureDatabase.h"
#include "URL.h"

using namespace std;
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
    if (CAddonMgr::Get().GetAddon("xbmc.addon", addonver))
      version = addonver->Version();
    for (size_t i = 0; i < ext->configuration->num_children; ++i)
    {
      if(ext->configuration->children[i].name &&
         strcmp(ext->configuration->children[i].name, "dir") == 0)
      {
        AddonVersion min_version(CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "@minversion"));
        if (min_version <= version)
        {
          DirInfo dir;
          dir.version    = min_version;
          dir.checksum   = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "checksum");
          dir.compressed = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "info@compressed").Equals("true");
          dir.info       = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "info");
          dir.datadir    = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "datadir");
          dir.zipped     = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "datadir@zip").Equals("true");
          dir.hashes     = CAddonMgr::Get().GetExtValue(&ext->configuration->children[i], "hashes").Equals("true");
          m_dirs.push_back(dir);
        }
      }
    }
    // backward compatibility
    if (!CAddonMgr::Get().GetExtValue(ext->configuration, "info").empty())
    {
      DirInfo info;
      info.checksum   = CAddonMgr::Get().GetExtValue(ext->configuration, "checksum");
      info.compressed = CAddonMgr::Get().GetExtValue(ext->configuration, "info@compressed").Equals("true");
      info.info       = CAddonMgr::Get().GetExtValue(ext->configuration, "info");
      info.datadir    = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir");
      info.zipped     = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir@zip").Equals("true");
      info.hashes     = CAddonMgr::Get().GetExtValue(ext->configuration, "hashes").Equals("true");
      m_dirs.push_back(info);
    }
  }
}

CRepository::CRepository(const CRepository &rhs)
  : CAddon(rhs)
{
  m_dirs = rhs.m_dirs;
}

CRepository::~CRepository()
{
}

string CRepository::Checksum() const
{
  string result;
  for (DirList::const_iterator it  = m_dirs.begin(); it != m_dirs.end(); ++it)
  {
    if (!it->checksum.empty())
      result += FetchChecksum(it->checksum);
  }
  return result;
}

string CRepository::FetchChecksum(const string& url)
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

string CRepository::GetAddonHash(const AddonPtr& addon) const
{
  string checksum;
  DirList::const_iterator it;
  for (it = m_dirs.begin();it != m_dirs.end(); ++it)
    if (it->datadir == addon->Path())
      break;
  if (it != m_dirs.end() && it->hashes)
  {
    checksum = FetchChecksum(addon->Path()+".md5");
    size_t pos = checksum.find_first_of(" \n");
    if (pos != string::npos)
      return checksum.substr(0, pos);
  }
  return checksum;
}

#define SET_IF_NOT_EMPTY(x,y) \
  { \
    if (!x.empty()) \
       x = y; \
  }

VECADDONS CRepository::Parse(const DirInfo& dir)
{
  VECADDONS result;
  CXBMCTinyXML doc;

  string file = dir.info;
  if (dir.compressed)
  {
    CURL url(dir.info);
    string opts = url.GetProtocolOptions();
    if (!opts.empty())
      opts += "&";
    url.SetProtocolOptions(opts+"Encoding=gzip");
    file = url.Get();
  }

  if (doc.LoadFile(file) && doc.RootElement())
  {
    CAddonMgr::Get().AddonsFromRepoXML(doc.RootElement(), result);
    for (IVECADDONS i = result.begin(); i != result.end(); ++i)
    {
      AddonPtr addon = *i;
      if (dir.zipped)
      {
        string file = StringUtils::Format("%s/%s-%s.zip", addon->ID().c_str(), addon->ID().c_str(), addon->Version().c_str());
        addon->Props().path = URIUtils::AddFileToFolder(dir.datadir,file);
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(dir.datadir,addon->ID()+"/icon.png"))
        file = StringUtils::Format("%s/changelog-%s.txt", addon->ID().c_str(), addon->Version().c_str());
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
  }

  return result;
}

CRepositoryUpdateJob::CRepositoryUpdateJob(const VECADDONS &repos)
  : m_repos(repos)
{
}

bool CRepositoryUpdateJob::DoWork()
{
  VECADDONS addons;
  for (VECADDONS::const_iterator i = m_repos.begin(); i != m_repos.end(); ++i)
  {
    RepositoryPtr repo = boost::dynamic_pointer_cast<CRepository>(*i);
    VECADDONS newAddons = GrabAddons(repo);
    addons.insert(addons.end(), newAddons.begin(), newAddons.end());
  }
  if (addons.empty())
    return false;

  // check for updates
  CAddonDatabase database;
  database.Open();
  
  CTextureDatabase textureDB;
  textureDB.Open();
  for (unsigned int i=0;i<addons.size();++i)
  {
    // manager told us to feck off
    if (ShouldCancel(0,0))
      break;

    bool deps_met = CAddonInstaller::Get().CheckDependencies(addons[i]);
    if (!deps_met && addons[i]->Props().broken.empty())
      addons[i]->Props().broken = "DEPSNOTMET";

    // invalidate the art associated with this item
    if (!addons[i]->Props().fanart.empty())
      textureDB.InvalidateCachedTexture(addons[i]->Props().fanart);
    if (!addons[i]->Props().icon.empty())
      textureDB.InvalidateCachedTexture(addons[i]->Props().icon);

    AddonPtr addon;
    CAddonMgr::Get().GetAddon(addons[i]->ID(),addon);
    if (addon && addons[i]->Version() > addon->Version() &&
        !database.IsAddonBlacklisted(addons[i]->ID(),addons[i]->Version().c_str()) &&
        deps_met)
    {
      if (CSettings::Get().GetBool("general.addonautoupdate") || addon->Type() >= ADDON_VIZ_LIBRARY)
      {
        string referer;
        if (URIUtils::IsInternetStream(addons[i]->Path()))
          referer = StringUtils::Format("Referer=%s-%s.zip",addon->ID().c_str(),addon->Version().c_str());

        if (addons[i]->Type() == ADDON_PVRDLL &&
            !PVR::CPVRManager::Get().InstallAddonAllowed(addons[i]->ID()))
          PVR::CPVRManager::Get().MarkAsOutdated(addon->ID(), referer);
        else
          CAddonInstaller::Get().Install(addon->ID(), true, referer);
      }
      else if (CSettings::Get().GetBool("general.addonnotifications"))
      {
        CGUIDialogKaiToast::QueueNotification(addon->Icon(),
                                              g_localizeStrings.Get(24061),
                                              addon->Name(),TOAST_DISPLAY_TIME,false,TOAST_DISPLAY_TIME);
      }
    }
    if (!addons[i]->Props().broken.empty())
    {
      if (database.IsAddonBroken(addons[i]->ID()).empty())
      {
        std::string line = g_localizeStrings.Get(24096);
        if (addons[i]->Props().broken == "DEPSNOTMET")
          line = g_localizeStrings.Get(24104);
        if (addon && CGUIDialogYesNo::ShowAndGetInput(addons[i]->Name(),
                                             line,
                                             g_localizeStrings.Get(24097),
                                             ""))
          CAddonMgr::Get().DisableAddon(addons[i]->ID());
      }
    }
    database.BreakAddon(addons[i]->ID(), addons[i]->Props().broken);
  }

  return true;
}

VECADDONS CRepositoryUpdateJob::GrabAddons(RepositoryPtr& repo)
{
  CAddonDatabase database;
  database.Open();
  string checksum;
  database.GetRepoChecksum(repo->ID(),checksum);
  string reposum = repo->Checksum();
  VECADDONS addons;
  if (checksum != reposum || checksum.empty())
  {
    map<string, AddonPtr> uniqueAddons;
    for (CRepository::DirList::const_iterator it = repo->m_dirs.begin(); it != repo->m_dirs.end(); ++it)
    {
      VECADDONS addons2 = CRepository::Parse(*it);
      for (VECADDONS::const_iterator it2 = addons2.begin(); it2 != addons2.end(); ++it2)
      {
        map<string, AddonPtr>::iterator existing = uniqueAddons.find((*it2)->ID());
        if (existing != uniqueAddons.end())
        { // already got it - replace if we have a newer version
          if (existing->second->Version() < (*it2)->Version())
            existing->second = *it2;
        }
        else
          uniqueAddons.insert(make_pair((*it2)->ID(), *it2));
      }
    }

    if (uniqueAddons.empty())
    {
      CLog::Log(LOGERROR,"Repository %s returned no add-ons, listing may have failed",repo->Name().c_str());
      reposum = checksum; // don't update the checksum
    }
    else
    {
      bool add=true;
      if (!repo->Props().libname.empty())
      {
        CFileItemList dummy;
        string s = StringUtils::Format("plugin://%s/?action=update", repo->ID().c_str());
        add = CDirectory::GetDirectory(s, dummy);
      }
      if (add)
      {
        for (map<string, AddonPtr>::const_iterator i = uniqueAddons.begin(); i != uniqueAddons.end(); ++i)
          addons.push_back(i->second);
        database.AddRepository(repo->ID(),addons,reposum);
      }
    }
  }
  else
    database.GetRepository(repo->ID(),addons);
  database.SetRepoTimestamp(repo->ID(),CDateTime::GetCurrentDateTime().GetAsDBDateTime());

  return addons;
}


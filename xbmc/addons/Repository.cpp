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
#include "utils/XBMCTinyXML.h"
#include "filesystem/File.h"
#include "AddonDatabase.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "utils/JobManager.h"
#include "addons/AddonInstaller.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "TextureDatabase.h"
#include "URL.h"
#include "pvr/PVRManager.h"
#include "filesystem/PluginDirectory.h"
#include "games/GameManager.h"

using namespace XFILE;
using namespace ADDON;
using namespace GAMES;

AddonPtr CRepository::Clone() const
{
  return AddonPtr(new CRepository(*this));
}

CRepository::CRepository(const AddonProps& props) :
  CAddon(props)
{
  m_compressed = false;
  m_zipped = false;
}

CRepository::CRepository(const cp_extension_t *ext)
  : CAddon(ext)
{
  m_compressed = false;
  m_zipped = false;
  // read in the other props that we need
  if (ext)
  {
    m_checksum = CAddonMgr::Get().GetExtValue(ext->configuration, "checksum");
    m_compressed = CAddonMgr::Get().GetExtValue(ext->configuration, "info@compressed").Equals("true");
    m_info = CAddonMgr::Get().GetExtValue(ext->configuration, "info");
    m_datadir = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir");
    m_zipped = CAddonMgr::Get().GetExtValue(ext->configuration, "datadir@zip").Equals("true");
    m_hashes = CAddonMgr::Get().GetExtValue(ext->configuration, "hashes").Equals("true");
  }
}

CRepository::CRepository(const CRepository &rhs)
  : CAddon(rhs)
{
  m_info       = rhs.m_info;
  m_checksum   = rhs.m_checksum;
  m_datadir    = rhs.m_datadir;
  m_compressed = rhs.m_compressed;
  m_zipped     = rhs.m_zipped;
  m_hashes     = rhs.m_hashes;
}

CRepository::~CRepository()
{
}

CStdString CRepository::Checksum()
{
  if (!m_checksum.IsEmpty())
    return FetchChecksum(m_checksum);
  return "";
}

CStdString CRepository::FetchChecksum(const CStdString& url)
{
  CSingleLock lock(m_critSection);
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

CStdString CRepository::GetAddonHash(const AddonPtr& addon)
{
  CStdString checksum;
  if (m_hashes)
  {
    checksum = FetchChecksum(addon->Path()+".md5");
    size_t pos = checksum.find_first_of(" \n");
    if (pos != CStdString::npos)
      return checksum.Left(pos);
  }
  return checksum;
}

#define SET_IF_NOT_EMPTY(x,y) \
  { \
    if (!x.IsEmpty()) \
       x = y; \
  }

VECADDONS CRepository::Parse()
{
  CSingleLock lock(m_critSection);

  VECADDONS result;
  CXBMCTinyXML doc;

  CStdString file = m_info;
  if (m_compressed)
  {
    CURL url(m_info);
    CStdString opts = url.GetProtocolOptions();
    if (!opts.IsEmpty())
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
      if (m_zipped)
      {
        CStdString file;
        file.Format("%s/%s-%s.zip", addon->ID().c_str(), addon->ID().c_str(), addon->Version().c_str());
        addon->Props().path = URIUtils::AddFileToFolder(m_datadir,file);
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/icon.png"))
        file.Format("%s/changelog-%s.txt", addon->ID().c_str(), addon->Version().c_str());
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(m_datadir,file))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/fanart.jpg"))
      }
      else
      {
        addon->Props().path = URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/");
        SET_IF_NOT_EMPTY(addon->Props().icon,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/icon.png"))
        SET_IF_NOT_EMPTY(addon->Props().changelog,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/changelog.txt"))
        SET_IF_NOT_EMPTY(addon->Props().fanart,URIUtils::AddFileToFolder(m_datadir,addon->ID()+"/fanart.jpg"))
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

  // Allow game manager to update its cache of valid game extensions
  // TODO: Create a callback interface for the Repository Updated action
  // We must call this so that the game manager's knowledge of file extensions
  // (which it uses to determine whether files are games) contains the extensions
  // supported by the game clients of new repositories.
  CGameManager::Get().UpdateRemoteAddons(addons);

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

    if (!CAddonInstaller::Get().CheckDependencies(addons[i]))
      addons[i]->Props().broken = g_localizeStrings.Get(24044);

    // invalidate the art associated with this item
    if (!addons[i]->Props().fanart.empty())
      textureDB.InvalidateCachedTexture(addons[i]->Props().fanart);
    if (!addons[i]->Props().icon.empty())
      textureDB.InvalidateCachedTexture(addons[i]->Props().icon);

    AddonPtr addon;
    CAddonMgr::Get().GetAddon(addons[i]->ID(),addon);
    if (addon && addons[i]->Version() > addon->Version() &&
        !database.IsAddonBlacklisted(addons[i]->ID(),addons[i]->Version().c_str()))
    {
      if (CSettings::Get().GetBool("general.addonautoupdate") || addon->Type() >= ADDON_VIZ_LIBRARY)
      {
        CStdString referer;
        if (URIUtils::IsInternetStream(addons[i]->Path()))
          referer.Format("Referer=%s-%s.zip",addon->ID().c_str(),addon->Version().c_str());

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
    if (!addons[i]->Props().broken.IsEmpty())
    {
      if (database.IsAddonBroken(addons[i]->ID()).IsEmpty())
      {
        if (addon && CGUIDialogYesNo::ShowAndGetInput(addons[i]->Name(),
                                             g_localizeStrings.Get(24096),
                                             g_localizeStrings.Get(24097),
                                             ""))
          database.DisableAddon(addons[i]->ID());
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
  CStdString checksum;
  database.GetRepoChecksum(repo->ID(),checksum);
  CStdString reposum = repo->Checksum();
  VECADDONS addons;
  if (!checksum.Equals(reposum) || checksum.empty())
  {
    addons = repo->Parse();
    if (addons.empty())
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
        CStdString s;
        s.Format("plugin://%s/?action=update", repo->ID());
        add = CDirectory::GetDirectory(s, dummy);
      }
      if (add)
        database.AddRepository(repo->ID(),addons,reposum);
    }
  }
  else
    database.GetRepository(repo->ID(),addons);
  database.SetRepoTimestamp(repo->ID(),CDateTime::GetCurrentDateTime().GetAsDBDateTime());

  return addons;
}


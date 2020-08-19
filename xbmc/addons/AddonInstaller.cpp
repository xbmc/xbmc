/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInstaller.h"

#include "FilesystemInstaller.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h" // for callback
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/AddonRepos.h"
#include "addons/Repository.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "events/AddonManagementEvent.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "favourites/FavouritesService.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h" // for callback
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <functional>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;
using KODI::UTILITY::TypedDigest;

CAddonInstaller::CAddonInstaller() : m_idle(true)
{ }

CAddonInstaller::~CAddonInstaller() = default;

CAddonInstaller &CAddonInstaller::GetInstance()
{
  static CAddonInstaller addonInstaller;
  return addonInstaller;
}

void CAddonInstaller::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CSingleLock lock(m_critSection);
  JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), [jobID](const std::pair<std::string, CDownloadJob>& p) {
    return p.second.jobID == jobID;
  });
  if (i != m_downloadJobs.end())
    m_downloadJobs.erase(i);
  if (m_downloadJobs.empty())
    m_idle.Set();
  lock.Leave();
  PrunePackageCache();

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CAddonInstaller::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
  CSingleLock lock(m_critSection);
  JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), [jobID](const std::pair<std::string, CDownloadJob>& p) {
    return p.second.jobID == jobID;
  });
  if (i != m_downloadJobs.end())
  {
    // update job progress
    i->second.progress = 100 / total * progress;
    i->second.downloadFinshed = std::string(job->GetType()) == CAddonInstallJob::TYPE_INSTALL;
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM);
    msg.SetStringParam(i->first);
    lock.Leave();
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}

bool CAddonInstaller::IsDownloading() const
{
  CSingleLock lock(m_critSection);
  return !m_downloadJobs.empty();
}

void CAddonInstaller::GetInstallList(VECADDONS &addons) const
{
  CSingleLock lock(m_critSection);
  std::vector<std::string> addonIDs;
  for (JobMap::const_iterator i = m_downloadJobs.begin(); i != m_downloadJobs.end(); ++i)
  {
    if (i->second.jobID)
      addonIDs.push_back(i->first);
  }
  lock.Leave();

  CAddonDatabase database;
  database.Open();
  for (std::vector<std::string>::iterator it = addonIDs.begin(); it != addonIDs.end(); ++it)
  {
    AddonPtr addon;
    if (database.GetAddon(*it, addon))
      addons.push_back(addon);
  }
}

bool CAddonInstaller::GetProgress(const std::string& addonID, unsigned int& percent, bool& downloadFinshed) const
{
  CSingleLock lock(m_critSection);
  JobMap::const_iterator i = m_downloadJobs.find(addonID);
  if (i != m_downloadJobs.end())
  {
    percent = i->second.progress;
    downloadFinshed = i->second.downloadFinshed;
    return true;
  }
  return false;
}

bool CAddonInstaller::Cancel(const std::string &addonID)
{
  CSingleLock lock(m_critSection);
  JobMap::iterator i = m_downloadJobs.find(addonID);
  if (i != m_downloadJobs.end())
  {
    CJobManager::GetInstance().CancelJob(i->second.jobID);
    m_downloadJobs.erase(i);
    if (m_downloadJobs.empty())
      m_idle.Set();
    return true;
  }

  return false;
}

bool CAddonInstaller::InstallModal(const std::string &addonID, ADDON::AddonPtr &addon, bool promptForInstall /* = true */)
{
  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return false;

  // we assume that addons that are enabled don't get to this routine (i.e. that GetAddon() has been called)
  if (CServiceBroker::GetAddonMgr().GetAddon(addonID, addon, ADDON_UNKNOWN, false))
    return false; // addon is installed but disabled, and the user has specifically activated something that needs
                  // the addon - should we enable it?

  // check we have it available
  CAddonDatabase database;
  database.Open();
  if (!database.GetAddon(addonID, addon))
    return false;

  // if specified ask the user if he wants it installed
  if (promptForInstall)
  {
    if (HELPERS::ShowYesNoDialogLines(CVariant{24076}, CVariant{24100}, CVariant{addon->Name()}, CVariant{24101}) !=
      DialogResponse::YES)
    {
      return false;
    }
  }

  if (!InstallOrUpdate(addonID, false, true))
    return false;

  return CServiceBroker::GetAddonMgr().GetAddon(addonID, addon);
}


bool CAddonInstaller::InstallOrUpdate(const std::string &addonID, bool background /* = true */, bool modal /* = false */)
{
  AddonPtr addon;
  RepositoryPtr repo;
  if (!CAddonInstallJob::GetAddon(addonID, repo, addon))
    return false;

  return DoInstall(addon, repo, background, modal);
}

bool CAddonInstaller::InstallOrUpdate(const ADDON::AddonPtr& addon,
                                      const ADDON::RepositoryPtr& repo)
{
  return DoInstall(addon, repo, false, false);
}

void CAddonInstaller::Install(const std::string& addonId, const AddonVersion& version, const std::string& repoId)
{
  CLog::Log(LOGDEBUG, "CAddonInstaller: installing '%s' version '%s' from repository '%s'",
      addonId.c_str(), version.asString().c_str(), repoId.c_str());

  AddonPtr addon;
  CAddonDatabase database;

  if (!database.Open() || !database.GetAddon(addonId, version, repoId, addon))
    return;

  AddonPtr repo;
  if (!CServiceBroker::GetAddonMgr().GetAddon(repoId, repo, ADDON_REPOSITORY))
    return;

  DoInstall(addon, std::static_pointer_cast<CRepository>(repo), true, false);
}

bool CAddonInstaller::DoInstall(const AddonPtr &addon, const RepositoryPtr& repo, bool background /* = true */, bool modal /* = false */, bool autoUpdate /* = false*/)
{
  // check whether we already have the addon installing
  CSingleLock lock(m_critSection);
  if (m_downloadJobs.find(addon->ID()) != m_downloadJobs.end())
    return false;

  CAddonInstallJob* installJob = new CAddonInstallJob(addon, repo, autoUpdate);
  if (background)
  {
    // Workaround: because CAddonInstallJob is blocking waiting for other jobs, it needs to be run
    // with priority dedicated.
    unsigned int jobID = CJobManager::GetInstance().AddJob(installJob, this, CJob::PRIORITY_DEDICATED);
    m_downloadJobs.insert(make_pair(addon->ID(), CDownloadJob(jobID)));
    m_idle.Reset();
    return true;
  }

  m_downloadJobs.insert(make_pair(addon->ID(), CDownloadJob(0)));
  m_idle.Reset();
  lock.Leave();

  bool result = false;
  if (modal)
    result = installJob->DoModal();
  else
    result = installJob->DoWork();
  delete installJob;

  lock.Enter();
  JobMap::iterator i = m_downloadJobs.find(addon->ID());
  m_downloadJobs.erase(i);
  if (m_downloadJobs.empty())
    m_idle.Set();

  return result;
}

bool CAddonInstaller::InstallFromZip(const std::string &path)
{
  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return false;

  CLog::Log(LOGDEBUG, "CAddonInstaller: installing from zip '%s'", CURL::GetRedacted(path).c_str());

  // grab the descriptive XML document from the zip, and read it in
  CFileItemList items;
  //! @bug some zip files return a single item (root folder) that we think is stored, so we don't use the zip:// protocol
  CURL pathToUrl(path);
  CURL zipDir = URIUtils::CreateArchivePath("zip", pathToUrl, "");
  if (!CDirectory::GetDirectory(zipDir, items, "", DIR_FLAG_DEFAULTS) ||
      items.Size() != 1 || !items[0]->m_bIsFolder)
  {
    CServiceBroker::GetEventLog().AddWithNotification(EventPtr(new CNotificationEvent(24045,
        StringUtils::Format(g_localizeStrings.Get(24143).c_str(), path.c_str()),
        "special://xbmc/media/icon256x256.png", EventLevel::Error)));
    return false;
  }

  AddonPtr addon;
  if (CServiceBroker::GetAddonMgr().LoadAddonDescription(items[0]->GetPath(), addon))
    return DoInstall(addon, RepositoryPtr());

  CServiceBroker::GetEventLog().AddWithNotification(EventPtr(new CNotificationEvent(24045,
      StringUtils::Format(g_localizeStrings.Get(24143).c_str(), path.c_str()),
      "special://xbmc/media/icon256x256.png", EventLevel::Error)));
  return false;
}

bool CAddonInstaller::CheckDependencies(const AddonPtr &addon, CAddonDatabase *database /* = NULL */)
{
  std::pair<std::string, std::string> failedDep;
  return CheckDependencies(addon, failedDep, database);
}

bool CAddonInstaller::CheckDependencies(const AddonPtr &addon, std::pair<std::string, std::string> &failedDep, CAddonDatabase *database /* = NULL */)
{
  std::vector<std::string> preDeps;
  preDeps.push_back(addon->ID());
  CAddonDatabase localDB;
  if (!database)
    database = &localDB;

  return CheckDependencies(addon, preDeps, *database, failedDep);
}

bool CAddonInstaller::CheckDependencies(const AddonPtr &addon,
                                        std::vector<std::string>& preDeps, CAddonDatabase &database,
                                        std::pair<std::string, std::string> &failedDep)
{
  if (addon == NULL)
    return true; // a NULL addon has no dependencies

  if (!database.Open())
    return false;

  for (const auto& it : addon->GetDependencies())
  {
    const std::string &addonID = it.id;
    const AddonVersion& versionMin = it.versionMin;
    const AddonVersion& version = it.version;
    bool optional = it.optional;
    AddonPtr dep;
    bool haveInstalledAddon =
        CServiceBroker::GetAddonMgr().GetAddon(addonID, dep, ADDON_UNKNOWN, false);
    if ((haveInstalledAddon && !dep->MeetsVersion(versionMin, version)) ||
        (!haveInstalledAddon && !optional))
    {
      // we have it but our version isn't good enough, or we don't have it and we need it
      if (!database.GetAddon(addonID, dep) || !dep->MeetsVersion(versionMin, version))
      {
        // we don't have it in a repo, or we have it but the version isn't good enough, so dep isn't satisfied.
        CLog::Log(LOGDEBUG, "CAddonInstallJob[%s]: requires %s version %s which is not available", addon->ID().c_str(), addonID.c_str(), version.asString().c_str());
        database.Close();

        // fill in the details of the failed dependency
        failedDep.first = addonID;
        failedDep.second = version.asString();

        return false;
      }
    }

    // need to enable the dependency
    if (dep && CServiceBroker::GetAddonMgr().IsAddonDisabled(addonID))
      if (!CServiceBroker::GetAddonMgr().EnableAddon(addonID))
      {
        database.Close();
        return false;
      }

    // at this point we have our dep, or the dep is optional (and we don't have it) so check that it's OK as well
    //! @todo should we assume that installed deps are OK?
    if (dep && std::find(preDeps.begin(), preDeps.end(), dep->ID()) == preDeps.end())
    {
      preDeps.push_back(dep->ID());
      if (!CheckDependencies(dep, preDeps, database, failedDep))
      {
        database.Close();
        return false;
      }
    }
  }
  database.Close();

  return true;
}

bool CAddonInstaller::HasJob(const std::string& ID) const
{
  CSingleLock lock(m_critSection);
  return m_downloadJobs.find(ID) != m_downloadJobs.end();
}

void CAddonInstaller::PrunePackageCache()
{
  std::map<std::string,CFileItemList*> packs;
  int64_t size = EnumeratePackageFolder(packs);
  int64_t limit = static_cast<int64_t>(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_addonPackageFolderSize) * 1024 * 1024;
  if (size < limit)
    return;

  // Prune packages
  // 1. Remove the largest packages, leaving at least 2 for each add-on
  CFileItemList items;
  CAddonDatabase db;
  db.Open();
  for (std::map<std::string,CFileItemList*>::const_iterator it = packs.begin(); it != packs.end(); ++it)
  {
    it->second->Sort(SortByLabel, SortOrderDescending);
    for (int j = 2; j < it->second->Size(); j++)
      items.Add(CFileItemPtr(new CFileItem(*it->second->Get(j))));
  }

  items.Sort(SortBySize, SortOrderDescending);
  int i = 0;
  while (size > limit && i < items.Size())
  {
    size -= items[i]->m_dwSize;
    db.RemovePackage(items[i]->GetPath());
    CFileUtils::DeleteItem(items[i++]);
  }

  if (size > limit)
  {
    // 2. Remove the oldest packages (leaving least 1 for each add-on)
    items.Clear();
    for (std::map<std::string,CFileItemList*>::iterator it = packs.begin(); it != packs.end(); ++it)
    {
      if (it->second->Size() > 1)
        items.Add(CFileItemPtr(new CFileItem(*it->second->Get(1))));
    }

    items.Sort(SortByDate, SortOrderAscending);
    i = 0;
    while (size > limit && i < items.Size())
    {
      size -= items[i]->m_dwSize;
      db.RemovePackage(items[i]->GetPath());
      CFileUtils::DeleteItem(items[i++]);
    }
  }

  // clean up our mess
  for (std::map<std::string,CFileItemList*>::iterator it = packs.begin(); it != packs.end(); ++it)
    delete it->second;
}

void CAddonInstaller::InstallAddons(const VECADDONS& addons, bool wait)
{
  for (const auto& addon : addons)
  {
    AddonPtr toInstall;
    RepositoryPtr repo;
    if (CAddonInstallJob::GetAddon(addon->ID(), repo, toInstall))
      DoInstall(toInstall, repo, false, false, true);
  }
  if (wait)
  {
    CSingleLock lock(m_critSection);
    if (!m_downloadJobs.empty())
    {
      m_idle.Reset();
      lock.Leave();
      m_idle.Wait();
    }
  }
}

int64_t CAddonInstaller::EnumeratePackageFolder(std::map<std::string,CFileItemList*>& result)
{
  CFileItemList items;
  CDirectory::GetDirectory("special://home/addons/packages/",items,".zip",DIR_FLAG_NO_FILE_DIRS);
  int64_t size = 0;
  for (int i = 0; i < items.Size(); i++)
  {
    if (items[i]->m_bIsFolder)
      continue;

    size += items[i]->m_dwSize;
    std::string pack,dummy;
    AddonVersion::SplitFileName(pack, dummy, items[i]->GetLabel());
    if (result.find(pack) == result.end())
      result[pack] = new CFileItemList;
    result[pack]->Add(CFileItemPtr(new CFileItem(*items[i])));
  }

  return size;
}

CAddonInstallJob::CAddonInstallJob(const AddonPtr &addon, const RepositoryPtr &repo, bool isAutoUpdate)
  : m_addon(addon),
    m_repo(repo),
    m_isAutoUpdate(isAutoUpdate)
{
  AddonPtr dummy;
  m_isUpdate = CServiceBroker::GetAddonMgr().GetAddon(addon->ID(), dummy, ADDON_UNKNOWN, false);
}

bool CAddonInstallJob::GetAddon(const std::string& addonID, RepositoryPtr& repo,
    ADDON::AddonPtr& addon)
{
  if (!CServiceBroker::GetAddonMgr().FindInstallableById(addonID, addon))
    return false;

  AddonPtr tmp;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addon->Origin(), tmp, ADDON_REPOSITORY))
    return false;

  repo = std::static_pointer_cast<CRepository>(tmp);

  return true;
}

bool CAddonInstallJob::DoWork()
{
  m_currentType = CAddonInstallJob::TYPE_DOWNLOAD;

  SetTitle(StringUtils::Format(g_localizeStrings.Get(24057).c_str(), m_addon->Name().c_str()));
  SetProgress(0);

  // check whether all the dependencies are available or not
  SetText(g_localizeStrings.Get(24058));
  std::pair<std::string, std::string> failedDep;
  if (!CAddonInstaller::GetInstance().CheckDependencies(m_addon, failedDep))
  {
    std::string details = StringUtils::Format(g_localizeStrings.Get(24142).c_str(), failedDep.first.c_str(), failedDep.second.c_str());
    CLog::Log(LOGERROR, "CAddonInstallJob[%s]: %s", m_addon->ID().c_str(), details.c_str());
    ReportInstallError(m_addon->ID(), m_addon->ID(), details);
    return false;
  }

  std::string installFrom;
  {
    // Addons are installed by downloading the .zip package on the server to the local
    // packages folder, then extracting from the local .zip package into the addons folder
    // Both these functions are achieved by "copying" using the vfs.

    if (!m_repo && URIUtils::HasSlashAtEnd(m_addon->Path()))
    { // passed in a folder - all we need do is copy it across
      installFrom = m_addon->Path();
    }
    else
    {
      std::string path{m_addon->Path()};
      TypedDigest hash;
      if (m_repo)
      {
        CRepository::ResolveResult resolvedAddon = m_repo->ResolvePathAndHash(m_addon);
        path = resolvedAddon.location;
        hash = resolvedAddon.digest;
        if (path.empty())
        {
          CLog::Log(LOGERROR, "CAddonInstallJob[%s]: failed to resolve addon install source path", m_addon->ID().c_str());
          ReportInstallError(m_addon->ID(), m_addon->ID());
          return false;
        }
      }

      CAddonDatabase db;
      if (!db.Open())
      {
        CLog::Log(LOGERROR, "CAddonInstallJob[%s]: failed to open database", m_addon->ID().c_str());
        ReportInstallError(m_addon->ID(), m_addon->ID());
        return false;
      }

      std::string packageOriginalPath, packageFileName;
      URIUtils::Split(path, packageOriginalPath, packageFileName);
      // Use ChangeBasePath so the URL is decoded if necessary
      const std::string packagePath = "special://home/addons/packages/";
      //!@todo fix design flaw in file copying: We use CFileOperationJob to download the package from the internet
      // to the local cache. It tries to be "smart" and decode the URL. But it never tells us what the result is,
      // so if we try for example to download "http://localhost/a+b.zip" the result ends up in "a b.zip".
      // First bug is that it actually decodes "+", which is not necessary except in query parts. Second bug
      // is that we cannot know that it does this and what the result is so the package will not be found without
      // using ChangeBasePath here (which is the same function the copying code uses and performs the translation).
      std::string package = URIUtils::ChangeBasePath(packageOriginalPath, packageFileName, packagePath);

      // check that we don't already have a valid copy
      if (!hash.Empty())
      {
        std::string hashExisting;
        if (db.GetPackageHash(m_addon->ID(), package, hashExisting) && hash.value != hashExisting)
        {
          db.RemovePackage(package);
        }
        if (CFile::Exists(package))
        {
          CFile::Delete(package);
        }
      }

      // zip passed in - download + extract
      if (!CFile::Exists(package))
      {
        if (!DownloadPackage(path, packagePath))
        {
          CFile::Delete(package);

          CLog::Log(LOGERROR, "CAddonInstallJob[%s]: failed to download %s", m_addon->ID().c_str(), package.c_str());
          ReportInstallError(m_addon->ID(), URIUtils::GetFileName(package));
          return false;
        }
      }

      // at this point we have the package - check that it is valid
      SetText(g_localizeStrings.Get(24077));
      if (!hash.Empty())
      {
        TypedDigest actualHash{hash.type, CUtil::GetFileDigest(package, hash.type)};
        if (hash != actualHash)
        {
          CFile::Delete(package);

          CLog::Log(LOGERROR, "CAddonInstallJob[{}]: Hash mismatch after download. Expected {}, was {}",
              m_addon->ID(), hash.value, actualHash.value);
          ReportInstallError(m_addon->ID(), URIUtils::GetFileName(package));
          return false;
        }

        db.AddPackage(m_addon->ID(), package, hash.value);
      }

      // check if the archive is valid
      CURL archive = URIUtils::CreateArchivePath("zip", CURL(package), "");

      CFileItemList archivedFiles;
      AddonPtr temp;
      if (!CDirectory::GetDirectory(archive, archivedFiles, "", DIR_FLAG_DEFAULTS) ||
          archivedFiles.Size() != 1 || !archivedFiles[0]->m_bIsFolder ||
          !CServiceBroker::GetAddonMgr().LoadAddonDescription(archivedFiles[0]->GetPath(), temp))
      {
        CLog::Log(LOGERROR, "CAddonInstallJob[%s]: invalid package %s", m_addon->ID().c_str(), package.c_str());
        db.RemovePackage(package);
        CFile::Delete(package);
        ReportInstallError(m_addon->ID(), URIUtils::GetFileName(package));
        return false;
      }

      installFrom = package;
    }
  }

  m_currentType = CAddonInstallJob::TYPE_INSTALL;

  // run any pre-install functions
  ADDON::OnPreInstall(m_addon);

  if (!CServiceBroker::GetAddonMgr().UnloadAddon(m_addon->ID()))
  {
    CLog::Log(LOGERROR, "CAddonInstallJob[%s]: failed to unload addon.", m_addon->ID().c_str());
    return false;
  }

  // perform install
  if (!Install(installFrom, m_repo))
    return false;

  // Load new installed and if successed replace defined m_addon here with new one
  if (!CServiceBroker::GetAddonMgr().LoadAddon(m_addon->ID()) ||
      !CServiceBroker::GetAddonMgr().GetAddon(m_addon->ID(), m_addon))
  {
    CLog::Log(LOGERROR, "CAddonInstallJob[%s]: failed to reload addon", m_addon->ID().c_str());
    return false;
  }

  g_localizeStrings.LoadAddonStrings(URIUtils::AddFileToFolder(m_addon->Path(), "resources/language/"),
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE), m_addon->ID());

  ADDON::OnPostInstall(m_addon, m_isUpdate, IsModal());

  // Write origin to database via addon manager, where this information is up-to-date.
  // Needed to set origin correctly for new installed addons.

  std::string origin;
  if (m_repo) // if we have an origin use it
  {
    origin = m_repo->ID();
  }
  else if (m_addon->HasType(ADDON_REPOSITORY))
  {
    origin = m_addon->ID(); // set origin to addon-id which is the repo itself
  }

  CServiceBroker::GetAddonMgr().SetAddonOrigin(m_addon->ID(), origin, m_isUpdate);

  bool notify = (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_ADDONS_NOTIFICATIONS)
        || !m_isAutoUpdate) && !IsModal();
  CServiceBroker::GetEventLog().Add(
      EventPtr(new CAddonManagementEvent(m_addon, m_isUpdate ? 24065 : 24084)), notify, false);

  if (m_isAutoUpdate && m_addon->IsBroken())
  {
    CLog::Log(LOGDEBUG, "CAddonInstallJob[%s]: auto-disabling due to being marked as broken", m_addon->ID().c_str());
    CServiceBroker::GetAddonMgr().DisableAddon(m_addon->ID(), AddonDisabledReason::USER);
    CServiceBroker::GetEventLog().Add(EventPtr(new CAddonManagementEvent(m_addon, 24094)), true, false);
  }

  // and we're done!
  MarkFinished();
  return true;
}

bool CAddonInstallJob::DownloadPackage(const std::string &path, const std::string &dest)
{
  if (ShouldCancel(0, 1))
    return false;

  SetText(g_localizeStrings.Get(24078));

  // need to download/copy the package first
  CFileItemList list;
  list.Add(CFileItemPtr(new CFileItem(path, false)));
  list[0]->Select(true);

  return DoFileOperation(CFileOperationJob::ActionReplace, list, dest, true);
}

bool CAddonInstallJob::DoFileOperation(FileAction action, CFileItemList &items, const std::string &file, bool useSameJob /* = true */)
{
  bool result = false;
  if (useSameJob)
  {
    SetFileOperation(action, items, file);

    // temporarily disable auto-closing so not to close the current progress indicator
    bool autoClose = GetAutoClose();
    if (autoClose)
      SetAutoClose(false);
    // temporarily disable updating title or text
    bool updateInformation = GetUpdateInformation();
    if (updateInformation)
      SetUpdateInformation(false);

    result = CFileOperationJob::DoWork();

    SetUpdateInformation(updateInformation);
    SetAutoClose(autoClose);
  }
  else
  {
   CFileOperationJob job(action, items, file);

   // pass our progress indicators to the temporary job and only allow it to
   // show progress updates (no title or text changes)
   job.SetProgressIndicators(GetProgressBar(), GetProgressDialog(), GetUpdateProgress(), false);

   result = job.DoWork();
  }

  return result;
}

bool CAddonInstallJob::Install(const std::string &installFrom, const RepositoryPtr& repo)
{
  SetText(g_localizeStrings.Get(24079));
  auto deps = m_addon->GetDependencies();

  unsigned int totalSteps = static_cast<unsigned int>(deps.size()) + 1;
  if (ShouldCancel(0, totalSteps))
    return false;

  const auto& addonMgr = CServiceBroker::GetAddonMgr();
  CAddonRepos addonRepos(addonMgr);
  CAddonDatabase database;

  if (database.Open())
  {
    addonRepos.LoadAddonsFromDatabase(database);
    database.Close();
  }

  // The first thing we do is install dependencies
  for (auto it = deps.begin(); it != deps.end(); ++it)
  {
    if (it->id != "xbmc.metadata")
    {
      const std::string &addonID = it->id;
      const AddonVersion& versionMin = it->versionMin;
      const AddonVersion& version = it->version;
      bool optional = it->optional;
      AddonPtr dependency;
      bool haveInstalledAddon = addonMgr.GetAddon(addonID, dependency, ADDON_UNKNOWN, false);
      if ((haveInstalledAddon && !dependency->MeetsVersion(versionMin, version)) ||
          (!haveInstalledAddon && !optional))
      {
        // we have it but our version isn't good enough, or we don't have it and we need it

        // dependency is already queued up for install - ::Install will fail
        // instead we wait until the Job has finished. note that we
        // recall install on purpose in case prior installation failed
        if (CAddonInstaller::GetInstance().HasJob(addonID))
        {
          while (CAddonInstaller::GetInstance().HasJob(addonID))
            KODI::TIME::Sleep(50);

          if (!CServiceBroker::GetAddonMgr().IsAddonInstalled(addonID))
          {
            CLog::Log(LOGERROR, "CAddonInstallJob[%s]: failed to install dependency %s", m_addon->ID().c_str(), addonID.c_str());
            ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24085));
            return false;
          }
        }
        // don't have the addon or the addon isn't new enough - grab it (no new job for these)
        else
        {
          RepositoryPtr repoForDep;
          AddonPtr dependencyToInstall;

          if (!addonRepos.FindDependency(addonID, m_addon, dependencyToInstall, repoForDep))
          {
            CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to find dependency {}", m_addon->ID(),
                      addonID);
            ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24085));
            return false;
          }

          if (IsModal())
          {
            CAddonInstallJob dependencyJob(dependencyToInstall, repoForDep, false);

            // pass our progress indicators to the temporary job and don't allow it to
            // show progress or information updates (no progress, title or text changes)
            dependencyJob.SetProgressIndicators(GetProgressBar(), GetProgressDialog(), false,
                                                false);

            if (!dependencyJob.DoModal())
            {
              CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to install dependency {}",
                        m_addon->ID(), addonID);
              ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24085));
              return false;
            }
          }
          else if (!CAddonInstaller::GetInstance().InstallOrUpdate(dependencyToInstall, repoForDep))
          {
            CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to install dependency {}",
                      m_addon->ID(), dependencyToInstall->ID());
            ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24085));
            return false;
          }
        }
      }
    }

    if (ShouldCancel(std::distance(deps.begin(), it), totalSteps))
        return false;
  }

  SetText(g_localizeStrings.Get(24086));
  SetProgress(static_cast<unsigned int>(100.0 * (totalSteps - 1.0) / totalSteps));

  CFilesystemInstaller fsInstaller;
  if (!fsInstaller.InstallToFilesystem(installFrom, m_addon->ID()))
  {
    ReportInstallError(m_addon->ID(), m_addon->ID());
    return false;
  }

  SetProgress(100);

  return true;
}

void CAddonInstallJob::ReportInstallError(const std::string& addonID, const std::string& fileName, const std::string& message /* = "" */)
{
  AddonPtr addon;
  CAddonDatabase database;
  if (database.Open())
  {
    database.GetAddon(addonID, addon);
    database.Close();
  }

  MarkFinished();

  std::string msg = message;
  EventPtr activity;
  if (addon != NULL)
  {
    AddonPtr addon2;
    CServiceBroker::GetAddonMgr().GetAddon(addonID, addon2);
    if (msg.empty())
      msg = g_localizeStrings.Get(addon2 != NULL ? 113 : 114);

    activity = EventPtr(new CAddonManagementEvent(addon, EventLevel::Error, msg));
    if (IsModal())
      HELPERS::ShowOKDialogText(CVariant{m_addon->Name()}, CVariant{msg});
  }
  else
  {
    activity = EventPtr(new CNotificationEvent(24045,
        !msg.empty() ? msg : StringUtils::Format(g_localizeStrings.Get(24143).c_str(), fileName.c_str()),
        EventLevel::Error));

    if (IsModal())
      HELPERS::ShowOKDialogText(CVariant{fileName}, CVariant{msg});
  }

  CServiceBroker::GetEventLog().Add(activity, !IsModal(), false);
}

CAddonUnInstallJob::CAddonUnInstallJob(const AddonPtr &addon, bool removeData)
  : m_addon(addon), m_removeData(removeData)
{ }

bool CAddonUnInstallJob::DoWork()
{
  ADDON::OnPreUnInstall(m_addon);

  //Unregister addon with the manager to ensure nothing tries
  //to interact with it while we are uninstalling.
  if (!CServiceBroker::GetAddonMgr().UnloadAddon(m_addon->ID()))
  {
    CLog::Log(LOGERROR, "CAddonUnInstallJob[%s]: failed to unload addon.", m_addon->ID().c_str());
    return false;
  }

  CFilesystemInstaller fsInstaller;
  if (!fsInstaller.UnInstallFromFilesystem(m_addon->Path()))
  {
    CLog::Log(LOGERROR, "CAddonUnInstallJob[%s]: could not delete addon data.", m_addon->ID().c_str());
    return false;
  }

  ClearFavourites();
  if (m_removeData)
  {
    CFileUtils::DeleteItem("special://profile/addon_data/"+m_addon->ID()+"/");
  }

  AddonPtr addon;
  CAddonDatabase database;
  // try to get the addon object from the repository as the local one does not exist anymore
  // if that doesn't work fall back to the local one
  if (!database.Open() || !database.GetAddon(m_addon->ID(), addon) || addon == NULL)
    addon = m_addon;
  CServiceBroker::GetEventLog().Add(EventPtr(new CAddonManagementEvent(addon, 24144)));

  CServiceBroker::GetAddonMgr().OnPostUnInstall(m_addon->ID());
  database.OnPostUnInstall(m_addon->ID());

  ADDON::OnPostUnInstall(m_addon);
  return true;
}

void CAddonUnInstallJob::ClearFavourites()
{
  bool bSave = false;
  CFileItemList items;
  CServiceBroker::GetFavouritesService().GetAll(items);
  for (int i = 0; i < items.Size(); i++)
  {
    if (items[i]->GetPath().find(m_addon->ID()) != std::string::npos)
    {
      items.Remove(items[i].get());
      bSave = true;
    }
  }

  if (bSave)
    CServiceBroker::GetFavouritesService().Save(items);
}

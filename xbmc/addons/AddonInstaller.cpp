/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInstaller.h"

#include "FileItem.h"
#include "FilesystemInstaller.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h" // for callback
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/AddonRepos.h"
#include "addons/Repository.h"
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "events/AddonManagementEvent.h"
#include "events/EventLog.h"
#include "events/NotificationEvent.h"
#include "favourites/FavouritesService.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h" // for callback
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileOperationJob.h"
#include "utils/FileUtils.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <functional>
#include <memory>
#include <mutex>

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

using namespace std::chrono_literals;

using KODI::MESSAGING::HELPERS::DialogResponse;
using KODI::UTILITY::TypedDigest;

namespace
{
class CAddonInstallJob : public CFileOperationJob
{
public:
  CAddonInstallJob(const ADDON::AddonPtr& addon,
                   const ADDON::RepositoryPtr& repo,
                   AutoUpdateJob isAutoUpdate);

  bool DoWork() override;

  static constexpr const char* TYPE_DOWNLOAD = "DOWNLOAD";
  static constexpr const char* TYPE_INSTALL = "INSTALL";
  /*!
   * \brief Returns the current processing type in the installation job
   *
   * \return The current processing type as string, can be \ref TYPE_DOWNLOAD or
   *         \ref TYPE_INSTALL
   */
  const char* GetType() const override { return m_currentType; }

  /*! \brief Find the add-on and its repository for the given add-on ID
   *  \param addonID ID of the add-on to find
   *  \param[out] repo the repository to use
   *  \param[out] addon Add-on with the given add-on ID
   *  \return True if the add-on and its repository were found, false otherwise.
   */
  static bool GetAddon(const std::string& addonID,
                       ADDON::RepositoryPtr& repo,
                       ADDON::AddonPtr& addon);

  void SetDependsInstall(DependencyJob dependsInstall) { m_dependsInstall = dependsInstall; }
  void SetAllowCheckForUpdates(AllowCheckForUpdates allowCheckForUpdates)
  {
    m_allowCheckForUpdates = allowCheckForUpdates;
  };

private:
  void OnPreInstall();
  void OnPostInstall();
  bool Install(const std::string& installFrom,
               const ADDON::RepositoryPtr& repo = ADDON::RepositoryPtr());
  bool DownloadPackage(const std::string& path, const std::string& dest);

  bool DoFileOperation(FileAction action,
                       CFileItemList& items,
                       const std::string& file,
                       bool useSameJob = true);

  /*! \brief Queue a notification for addon installation/update failure
   \param addonID - addon id
   \param fileName - filename which is shown in case the addon id is unknown
   \param message - error message to be displayed
   */
  void ReportInstallError(const std::string& addonID,
                          const std::string& fileName,
                          const std::string& message = "");

  ADDON::AddonPtr m_addon;
  ADDON::RepositoryPtr m_repo;
  bool m_isUpdate;
  AutoUpdateJob m_isAutoUpdate;
  DependencyJob m_dependsInstall = DependencyJob::CHOICE_NO;
  AllowCheckForUpdates m_allowCheckForUpdates = AllowCheckForUpdates::CHOICE_YES;
  const char* m_currentType = TYPE_DOWNLOAD;
};

class CAddonUnInstallJob : public CFileOperationJob
{
public:
  CAddonUnInstallJob(const ADDON::AddonPtr& addon, bool removeData);

  bool DoWork() override;
  void SetRecurseOrphaned(RecurseOrphaned recurseOrphaned) { m_recurseOrphaned = recurseOrphaned; };

private:
  void ClearFavourites();

  ADDON::AddonPtr m_addon;
  bool m_removeData;
  RecurseOrphaned m_recurseOrphaned = RecurseOrphaned::CHOICE_YES;
};

} // unnamed namespace

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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  JobMap::iterator i = find_if(m_downloadJobs.begin(), m_downloadJobs.end(), [jobID](const std::pair<std::string, CDownloadJob>& p) {
    return p.second.jobID == jobID;
  });
  if (i != m_downloadJobs.end())
    m_downloadJobs.erase(i);
  if (m_downloadJobs.empty())
    m_idle.Set();
  lock.unlock();
  PrunePackageCache();

  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CAddonInstaller::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
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
    lock.unlock();
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
}

bool CAddonInstaller::IsDownloading() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return !m_downloadJobs.empty();
}

void CAddonInstaller::GetInstallList(VECADDONS &addons) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::vector<std::string> addonIDs;
  for (JobMap::const_iterator i = m_downloadJobs.begin(); i != m_downloadJobs.end(); ++i)
  {
    if (i->second.jobID)
      addonIDs.push_back(i->first);
  }
  lock.unlock();

  auto& addonMgr = CServiceBroker::GetAddonMgr();
  for (const auto& addonId : addonIDs)
  {
    AddonPtr addon;
    if (addonMgr.FindInstallableById(addonId, addon))
      addons.emplace_back(std::move(addon));
  }
}

bool CAddonInstaller::GetProgress(const std::string& addonID, unsigned int& percent, bool& downloadFinshed) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  JobMap::iterator i = m_downloadJobs.find(addonID);
  if (i != m_downloadJobs.end())
  {
    CServiceBroker::GetJobManager()->CancelJob(i->second.jobID);
    m_downloadJobs.erase(i);
    if (m_downloadJobs.empty())
      m_idle.Set();
    return true;
  }

  return false;
}

bool CAddonInstaller::InstallModal(const std::string& addonID,
                                   ADDON::AddonPtr& addon,
                                   InstallModalPrompt promptForInstall)
{
  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return false;

  // we assume that addons that are enabled don't get to this routine (i.e. that GetAddon() has been called)
  if (CServiceBroker::GetAddonMgr().GetAddon(addonID, addon, OnlyEnabled::CHOICE_NO))
    return false; // addon is installed but disabled, and the user has specifically activated something that needs
                  // the addon - should we enable it?

  // check we have it available
  if (!CServiceBroker::GetAddonMgr().FindInstallableById(addonID, addon))
    return false;

  // if specified ask the user if he wants it installed
  if (promptForInstall == InstallModalPrompt::CHOICE_YES)
  {
    if (HELPERS::ShowYesNoDialogLines(CVariant{24076}, CVariant{24100}, CVariant{addon->Name()},
                                      CVariant{24101}) != DialogResponse::CHOICE_YES)
    {
      return false;
    }
  }

  if (!InstallOrUpdate(addonID, BackgroundJob::CHOICE_NO, ModalJob::CHOICE_YES))
    return false;

  return CServiceBroker::GetAddonMgr().GetAddon(addonID, addon, OnlyEnabled::CHOICE_YES);
}


bool CAddonInstaller::InstallOrUpdate(const std::string& addonID,
                                      BackgroundJob background,
                                      ModalJob modal)
{
  AddonPtr addon;
  RepositoryPtr repo;
  if (!CAddonInstallJob::GetAddon(addonID, repo, addon))
    return false;

  return DoInstall(addon, repo, background, modal, AutoUpdateJob::CHOICE_NO,
                   DependencyJob::CHOICE_NO, AllowCheckForUpdates::CHOICE_YES);
}

bool CAddonInstaller::InstallOrUpdateDependency(const ADDON::AddonPtr& dependsId,
                                                const ADDON::RepositoryPtr& repo)
{
  return DoInstall(dependsId, repo, BackgroundJob::CHOICE_NO, ModalJob::CHOICE_NO,
                   AutoUpdateJob::CHOICE_NO, DependencyJob::CHOICE_YES,
                   AllowCheckForUpdates::CHOICE_YES);
}

bool CAddonInstaller::RemoveDependency(const std::shared_ptr<IAddon>& dependsId) const
{
  const bool removeData = CDirectory::Exists(dependsId->Profile());
  CAddonUnInstallJob removeDependencyJob(dependsId, removeData);
  removeDependencyJob.SetRecurseOrphaned(RecurseOrphaned::CHOICE_NO);

  return removeDependencyJob.DoWork();
}

std::vector<std::string> CAddonInstaller::RemoveOrphanedDepsRecursively() const
{
  std::vector<std::string> removedItems;

  auto toRemove = CServiceBroker::GetAddonMgr().GetOrphanedDependencies();
  while (toRemove.size() > 0)
  {
    for (const auto& dep : toRemove)
    {
      if (RemoveDependency(dep))
      {
        removedItems.emplace_back(dep->Name()); // successfully removed
      }
      else
      {
        CLog::Log(LOGERROR, "CAddonMgr::{}: failed to remove orphaned add-on/dependency: {}",
                  __func__, dep->Name());
      }
    }

    toRemove = CServiceBroker::GetAddonMgr().GetOrphanedDependencies();
  }

  return removedItems;
}

bool CAddonInstaller::Install(const std::string& addonId,
                              const CAddonVersion& version,
                              const std::string& repoId)
{
  CLog::Log(LOGDEBUG, "CAddonInstaller: installing '{}' version '{}' from repository '{}'", addonId,
            version.asString(), repoId);

  AddonPtr addon;
  CAddonDatabase database;

  if (!database.Open() || !database.GetAddon(addonId, version, repoId, addon))
    return false;

  AddonPtr repo;
  if (!CServiceBroker::GetAddonMgr().GetAddon(repoId, repo, AddonType::REPOSITORY,
                                              OnlyEnabled::CHOICE_YES))
    return false;

  return DoInstall(addon, std::static_pointer_cast<CRepository>(repo), BackgroundJob::CHOICE_YES,
                   ModalJob::CHOICE_NO, AutoUpdateJob::CHOICE_NO, DependencyJob::CHOICE_NO,
                   AllowCheckForUpdates::CHOICE_YES);
}

bool CAddonInstaller::DoInstall(const AddonPtr& addon,
                                const RepositoryPtr& repo,
                                BackgroundJob background,
                                ModalJob modal,
                                AutoUpdateJob autoUpdate,
                                DependencyJob dependsInstall,
                                AllowCheckForUpdates allowCheckForUpdates)
{
  // check whether we already have the addon installing
  std::unique_lock<CCriticalSection> lock(m_critSection);
  if (m_downloadJobs.find(addon->ID()) != m_downloadJobs.end())
    return false;

  CAddonInstallJob* installJob = new CAddonInstallJob(addon, repo, autoUpdate);
  if (background == BackgroundJob::CHOICE_YES)
  {
    // Workaround: because CAddonInstallJob is blocking waiting for other jobs, it needs to be run
    // with priority dedicated.
    unsigned int jobID =
        CServiceBroker::GetJobManager()->AddJob(installJob, this, CJob::PRIORITY_DEDICATED);
    m_downloadJobs.insert(make_pair(addon->ID(), CDownloadJob(jobID)));
    m_idle.Reset();

    return true;
  }

  m_downloadJobs.insert(make_pair(addon->ID(), CDownloadJob(0)));
  m_idle.Reset();
  lock.unlock();

  installJob->SetDependsInstall(dependsInstall);
  installJob->SetAllowCheckForUpdates(allowCheckForUpdates);

  bool result = false;
  if (modal == ModalJob::CHOICE_YES)
    result = installJob->DoModal();
  else
    result = installJob->DoWork();
  delete installJob;

  lock.lock();
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

  CLog::Log(LOGDEBUG, "CAddonInstaller: installing from zip '{}'", CURL::GetRedacted(path));

  // grab the descriptive XML document from the zip, and read it in
  CFileItemList items;
  //! @bug some zip files return a single item (root folder) that we think is stored, so we don't use the zip:// protocol
  CURL pathToUrl(path);
  CURL zipDir = URIUtils::CreateArchivePath("zip", pathToUrl, "");
  auto eventLog = CServiceBroker::GetEventLog();
  if (!CDirectory::GetDirectory(zipDir, items, "", DIR_FLAG_DEFAULTS) ||
      items.Size() != 1 || !items[0]->m_bIsFolder)
  {
    if (eventLog)
      eventLog->AddWithNotification(EventPtr(
          new CNotificationEvent(24045, StringUtils::Format(g_localizeStrings.Get(24143), path),
                                 "special://xbmc/media/icon256x256.png", EventLevel::Error)));

    CLog::Log(
        LOGERROR,
        "CAddonInstaller: installing addon failed '{}' - itemsize: {}, first item is folder: {}",
        CURL::GetRedacted(path), items.Size(), items[0]->m_bIsFolder);
    return false;
  }

  AddonPtr addon;
  if (CServiceBroker::GetAddonMgr().LoadAddonDescription(items[0]->GetPath(), addon))
    return DoInstall(addon, RepositoryPtr(), BackgroundJob::CHOICE_YES, ModalJob::CHOICE_NO,
                     AutoUpdateJob::CHOICE_NO, DependencyJob::CHOICE_NO,
                     AllowCheckForUpdates::CHOICE_YES);

  if (eventLog)
    eventLog->AddWithNotification(EventPtr(
        new CNotificationEvent(24045, StringUtils::Format(g_localizeStrings.Get(24143), path),
                               "special://xbmc/media/icon256x256.png", EventLevel::Error)));
  return false;
}

bool CAddonInstaller::UnInstall(const AddonPtr& addon, bool removeData)
{
  CServiceBroker::GetJobManager()->AddJob(new CAddonUnInstallJob(addon, removeData), this);
  return true;
}

bool CAddonInstaller::CheckDependencies(const AddonPtr& addon,
                                        CAddonDatabase* database /* = nullptr */)
{
  std::pair<std::string, std::string> failedDep;
  return CheckDependencies(addon, failedDep, database);
}

bool CAddonInstaller::CheckDependencies(const AddonPtr& addon,
                                        std::pair<std::string, std::string>& failedDep,
                                        CAddonDatabase* database /* = nullptr */)
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
  if (addon == nullptr)
    return true; // a nullptr addon has no dependencies

  for (const auto& it : addon->GetDependencies())
  {
    const std::string &addonID = it.id;
    const CAddonVersion& versionMin = it.versionMin;
    const CAddonVersion& version = it.version;
    bool optional = it.optional;
    AddonPtr dep;
    const bool haveInstalledAddon =
        CServiceBroker::GetAddonMgr().GetAddon(addonID, dep, OnlyEnabled::CHOICE_NO);
    if ((haveInstalledAddon && !dep->MeetsVersion(versionMin, version)) ||
        (!haveInstalledAddon && !optional))
    {
      // we have it but our version isn't good enough, or we don't have it and we need it
      if (!CServiceBroker::GetAddonMgr().FindInstallableById(addonID, dep) ||
          (dep && !dep->MeetsVersion(versionMin, version)))
      {
        // we don't have it in a repo, or we have it but the version isn't good enough, so dep isn't satisfied.
        CLog::Log(LOGDEBUG, "CAddonInstallJob[{}]: requires {} version {} which is not available",
                  addon->ID(), addonID, version.asString());

        // fill in the details of the failed dependency
        failedDep.first = addonID;
        failedDep.second = version.asString();

        return false;
      }
    }

    // need to enable the dependency
    if (dep && CServiceBroker::GetAddonMgr().IsAddonDisabled(addonID) &&
        !CServiceBroker::GetAddonMgr().EnableAddon(addonID))
    {
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_downloadJobs.find(ID) != m_downloadJobs.end();
}

void CAddonInstaller::PrunePackageCache()
{
  std::map<std::string, std::unique_ptr<CFileItemList>> packs;
  int64_t size = EnumeratePackageFolder(packs);
  int64_t limit = static_cast<int64_t>(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_addonPackageFolderSize) * 1024 * 1024;
  if (size < limit)
    return;

  // Prune packages
  // 1. Remove the largest packages, leaving at least 2 for each add-on
  CFileItemList items;
  CAddonDatabase db;
  db.Open();
  for (auto it = packs.begin(); it != packs.end(); ++it)
  {
    it->second->Sort(SortByLabel, SortOrderDescending);
    for (int j = 2; j < it->second->Size(); j++)
      items.Add(std::make_shared<CFileItem>(*it->second->Get(j)));
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
    for (auto it = packs.begin(); it != packs.end(); ++it)
    {
      if (it->second->Size() > 1)
        items.Add(std::make_shared<CFileItem>(*it->second->Get(1)));
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
}

void CAddonInstaller::InstallAddons(const VECADDONS& addons,
                                    bool wait,
                                    AllowCheckForUpdates allowCheckForUpdates)
{
  for (const auto& addon : addons)
  {
    AddonPtr toInstall;
    RepositoryPtr repo;
    if (CAddonInstallJob::GetAddon(addon->ID(), repo, toInstall))
      DoInstall(toInstall, repo, BackgroundJob::CHOICE_NO, ModalJob::CHOICE_NO,
                AutoUpdateJob::CHOICE_YES, DependencyJob::CHOICE_NO, allowCheckForUpdates);
  }
  if (wait)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (!m_downloadJobs.empty())
    {
      m_idle.Reset();
      lock.unlock();
      m_idle.Wait();
    }
  }
}

int64_t CAddonInstaller::EnumeratePackageFolder(
    std::map<std::string, std::unique_ptr<CFileItemList>>& result)
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
    CAddonVersion::SplitFileName(pack, dummy, items[i]->GetLabel());
    if (result.find(pack) == result.end())
      result[pack] = std::make_unique<CFileItemList>();
    result[pack]->Add(std::make_shared<CFileItem>(*items[i]));
  }

  return size;
}

CAddonInstallJob::CAddonInstallJob(const AddonPtr& addon,
                                   const RepositoryPtr& repo,
                                   AutoUpdateJob isAutoUpdate)
  : m_addon(addon), m_repo(repo), m_isAutoUpdate(isAutoUpdate)
{
  AddonPtr dummy;
  m_isUpdate = CServiceBroker::GetAddonMgr().GetAddon(addon->ID(), dummy, OnlyEnabled::CHOICE_NO);
}

bool CAddonInstallJob::GetAddon(const std::string& addonID, RepositoryPtr& repo,
    ADDON::AddonPtr& addon)
{
  if (!CServiceBroker::GetAddonMgr().FindInstallableById(addonID, addon))
    return false;

  AddonPtr tmp;
  if (!CServiceBroker::GetAddonMgr().GetAddon(addon->Origin(), tmp, AddonType::REPOSITORY,
                                              OnlyEnabled::CHOICE_YES))
    return false;

  repo = std::static_pointer_cast<CRepository>(tmp);

  return true;
}

bool CAddonInstallJob::DoWork()
{
  m_currentType = CAddonInstallJob::TYPE_DOWNLOAD;

  SetTitle(StringUtils::Format(g_localizeStrings.Get(24057), m_addon->Name()));
  SetProgress(0);

  // check whether all the dependencies are available or not
  SetText(g_localizeStrings.Get(24058));
  std::pair<std::string, std::string> failedDep;
  if (!CAddonInstaller::GetInstance().CheckDependencies(m_addon, failedDep))
  {
    std::string details =
        StringUtils::Format(g_localizeStrings.Get(24142), failedDep.first, failedDep.second);
    CLog::Log(LOGERROR, "CAddonInstallJob[{}]: {}", m_addon->ID(), details);
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
          CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to resolve addon install source path",
                    m_addon->ID());
          ReportInstallError(m_addon->ID(), m_addon->ID());
          return false;
        }
      }

      CAddonDatabase db;
      if (!db.Open())
      {
        CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to open database", m_addon->ID());
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

          CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to download {}", m_addon->ID(),
                    package);
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
        CLog::Log(LOGERROR, "CAddonInstallJob[{}]: invalid package {}", m_addon->ID(), package);
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
    CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to unload addon.", m_addon->ID());
    return false;
  }

  // perform install
  if (!Install(installFrom, m_repo))
    return false;

  // Load new installed and if successed replace defined m_addon here with new one
  if (!CServiceBroker::GetAddonMgr().LoadAddon(m_addon->ID(), m_addon->Origin(),
                                               m_addon->Version()) ||
      !CServiceBroker::GetAddonMgr().GetAddon(m_addon->ID(), m_addon, OnlyEnabled::CHOICE_YES))
  {
    CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to reload addon", m_addon->ID());
    return false;
  }

  g_localizeStrings.LoadAddonStrings(URIUtils::AddFileToFolder(m_addon->Path(), "resources/language/"),
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOCALE_LANGUAGE), m_addon->ID());

  ADDON::OnPostInstall(m_addon, m_isUpdate, IsModal());

  // Write origin to database via addon manager, where this information is up-to-date.
  // Needed to set origin correctly for new installed addons.

  std::string origin;
  if (m_addon->Origin() == ORIGIN_SYSTEM)
  {
    origin = ORIGIN_SYSTEM; // keep system add-on origin as ORIGIN_SYSTEM
  }
  else if (m_addon->HasMainType(AddonType::REPOSITORY))
  {
    origin = m_addon->ID(); // use own id as origin if repository

    // if a repository is updated during the add-on migration process, we need to skip
    // calling CheckForUpdates() on the repo to prevent deadlock issues during migration

    if (m_allowCheckForUpdates == AllowCheckForUpdates::CHOICE_YES)
    {
      if (m_isUpdate)
      {
        CLog::Log(LOGDEBUG, "ADDONS: repository [{}] updated. now checking for content updates.",
                  m_addon->ID());
        CServiceBroker::GetRepositoryUpdater().CheckForUpdates(
            std::static_pointer_cast<CRepository>(m_addon), false);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "ADDONS: skipping CheckForUpdates() on repository [{}].", m_addon->ID());
    }
  }
  else if (m_repo)
  {
    origin = m_repo->ID(); // use repo id as origin
  }

  CServiceBroker::GetAddonMgr().SetAddonOrigin(m_addon->ID(), origin, m_isUpdate);

  if (m_dependsInstall == DependencyJob::CHOICE_YES)
  {
    CLog::Log(LOGDEBUG, "ADDONS: dependency [{}] will not be version checked and unpinned",
              m_addon->ID());
  }
  else
  {
    // we only do pinning/unpinning for non-system add-ons
    if (m_addon->Origin() != ORIGIN_SYSTEM)
    {
      // get all compatible versions of an addon-id regardless of their origin
      // from all installed repositories
      std::vector<std::shared_ptr<IAddon>> compatibleVersions =
          CServiceBroker::GetAddonMgr().GetCompatibleVersions(m_addon->ID());

      if (!m_addon->Origin().empty())
      {
        // handle add-ons that originate from a repository

        // find the latest version for the origin we installed from
        CAddonVersion latestVersion;
        for (const auto& compatibleVersion : compatibleVersions)
        {
          if (compatibleVersion->Origin() == m_addon->Origin() &&
              compatibleVersion->Version() > latestVersion)
          {
            latestVersion = compatibleVersion->Version();
          }
        }

        if (m_addon->Version() == latestVersion)
        {
          // unpin the installed addon if it's the latest of its origin
          CServiceBroker::GetAddonMgr().RemoveUpdateRuleFromList(m_addon->ID(),
                                                                 AddonUpdateRule::PIN_OLD_VERSION);
          CLog::Log(LOGDEBUG, "ADDONS: unpinned Addon: [{}] Origin: [{}] Version: [{}]",
                    m_addon->ID(), m_addon->Origin(), m_addon->Version().asString());
        }
        else
        {
          // pin if it is not the latest
          CServiceBroker::GetAddonMgr().AddUpdateRuleToList(m_addon->ID(),
                                                            AddonUpdateRule::PIN_OLD_VERSION);
          CLog::Log(LOGDEBUG, "ADDONS: pinned Addon: [{}] Origin: [{}] Version: [{}]",
                    m_addon->ID(), m_addon->Origin(), m_addon->Version().asString());
        }
      }
      else
      {
        // handle manually installed add-ons

        // find the latest version of any origin/repository
        CAddonVersion latestVersion;
        for (const auto& compatibleVersion : compatibleVersions)
        {
          if (compatibleVersion->Version() > latestVersion)
          {
            latestVersion = compatibleVersion->Version();
          }
        }

        if (m_addon->Version() < latestVersion)
        {
          // pin zip version if it's lesser than latest from repo(s)
          CServiceBroker::GetAddonMgr().AddUpdateRuleToList(m_addon->ID(),
                                                            AddonUpdateRule::PIN_ZIP_INSTALL);
          CLog::Log(LOGDEBUG, "ADDONS: pinned zip installed Addon: [{}] Version: [{}]",
                    m_addon->ID(), m_addon->Version().asString());
        }
        else
        {
          // unpin zip version if it's >= the latest from repos
          CServiceBroker::GetAddonMgr().RemoveUpdateRuleFromList(m_addon->ID(),
                                                                 AddonUpdateRule::PIN_ZIP_INSTALL);
          CLog::Log(LOGDEBUG, "ADDONS: unpinned zip installed Addon: [{}] Version: [{}]",
                    m_addon->ID(), m_addon->Version().asString());
        }
      }
    }
  }

  bool notify = (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                     CSettings::SETTING_ADDONS_NOTIFICATIONS) ||
                 m_isAutoUpdate == AutoUpdateJob::CHOICE_NO) &&
                !IsModal() && m_dependsInstall == DependencyJob::CHOICE_NO;
  auto eventLog = CServiceBroker::GetEventLog();
  if (eventLog)
    eventLog->Add(EventPtr(new CAddonManagementEvent(m_addon, m_isUpdate ? 24065 : 24084)), notify,
                  false);

  if (m_isAutoUpdate == AutoUpdateJob::CHOICE_YES &&
      m_addon->LifecycleState() == AddonLifecycleState::BROKEN)
  {
    CLog::Log(LOGDEBUG, "CAddonInstallJob[{}]: auto-disabling due to being marked as broken",
              m_addon->ID());
    CServiceBroker::GetAddonMgr().DisableAddon(m_addon->ID(), AddonDisabledReason::USER);
    if (eventLog)
      eventLog->Add(EventPtr(new CAddonManagementEvent(m_addon, 24094)), true, false);
  }
  else if (m_addon->LifecycleState() == AddonLifecycleState::DEPRECATED)
  {
    CLog::Log(LOGDEBUG, "CAddonInstallJob[{}]: installed addon marked as deprecated",
              m_addon->ID());
    std::string text =
        StringUtils::Format(g_localizeStrings.Get(24168), m_addon->LifecycleStateDescription());
    if (eventLog)
      eventLog->Add(EventPtr(new CAddonManagementEvent(m_addon, text)), true, false);
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
  list.Add(std::make_shared<CFileItem>(path, false));
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
  const auto& deps = m_addon->GetDependencies();

  if (!deps.empty() && m_addon->HasType(AddonType::REPOSITORY))
  {
    bool notSystemAddon = std::none_of(deps.begin(), deps.end(), [](const DependencyInfo& dep) {
      return CServiceBroker::GetAddonMgr().IsSystemAddon(dep.id);
    });

    if (notSystemAddon)
    {
      CLog::Log(
          LOGERROR,
          "CAddonInstallJob::{}: failed to install repository [{}]. It has dependencies defined",
          __func__, m_addon->ID());
      ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24088));
      return false;
    }
  }

  SetText(g_localizeStrings.Get(24079));
  unsigned int totalSteps = static_cast<unsigned int>(deps.size()) + 1;
  if (ShouldCancel(0, totalSteps))
    return false;

  CAddonRepos addonRepos;
  if (!addonRepos.IsValid())
    return false;

  // The first thing we do is install dependencies
  for (auto it = deps.begin(); it != deps.end(); ++it)
  {
    if (it->id != "xbmc.metadata")
    {
      const std::string &addonID = it->id;
      const CAddonVersion& versionMin = it->versionMin;
      const CAddonVersion& version = it->version;
      bool optional = it->optional;
      AddonPtr dependency;
      const bool haveInstalledAddon =
          CServiceBroker::GetAddonMgr().GetAddon(addonID, dependency, OnlyEnabled::CHOICE_NO);
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
            KODI::TIME::Sleep(50ms);

          if (!CServiceBroker::GetAddonMgr().IsAddonInstalled(addonID))
          {
            CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to install dependency {}",
                      m_addon->ID(), addonID);
            ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24085));
            return false;
          }
        }
        // don't have the addon or the addon isn't new enough - grab it (no new job for these)
        else
        {
          RepositoryPtr repoForDep;
          AddonPtr dependencyToInstall;

          // origin of m_addon is empty at least if an addon is installed for the first time
          // we need to override "parentRepoId" if the passed in repo is valid.

          const std::string& parentRepoId =
              m_addon->Origin().empty() && repo ? repo->ID() : m_addon->Origin();

          if (!addonRepos.FindDependency(addonID, parentRepoId, dependencyToInstall, repoForDep))
          {
            CLog::Log(LOGERROR, "CAddonInstallJob[{}]: failed to find dependency {}", m_addon->ID(),
                      addonID);
            ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24085));
            return false;
          }
          else
          {
            if (!dependencyToInstall->MeetsVersion(versionMin, version))
            {
              CLog::Log(LOGERROR,
                        "CAddonInstallJob[{}]: found dependency [{}/{}] doesn't meet minimum "
                        "version [{}]",
                        m_addon->ID(), addonID, dependencyToInstall->Version().asString(),
                        versionMin.asString());
              ReportInstallError(m_addon->ID(), m_addon->ID(), g_localizeStrings.Get(24085));
              return false;
            }
          }

          if (IsModal())
          {
            CAddonInstallJob dependencyJob(dependencyToInstall, repoForDep,
                                           AutoUpdateJob::CHOICE_NO);
            dependencyJob.SetDependsInstall(DependencyJob::CHOICE_YES);

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
          else if (!CAddonInstaller::GetInstance().InstallOrUpdateDependency(dependencyToInstall,
                                                                             repoForDep))
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
  CServiceBroker::GetAddonMgr().FindInstallableById(addonID, addon);

  MarkFinished();

  std::string msg = message;
  EventPtr activity;
  if (addon != nullptr)
  {
    AddonPtr addon2;
    bool success = CServiceBroker::GetAddonMgr().GetAddon(addonID, addon2, OnlyEnabled::CHOICE_YES);
    if (msg.empty())
    {
      msg = g_localizeStrings.Get(addon2 != nullptr && success ? 113 : 114);
    }

    activity = EventPtr(new CAddonManagementEvent(addon, EventLevel::Error, msg));
    if (IsModal())
      HELPERS::ShowOKDialogText(CVariant{m_addon->Name()}, CVariant{msg});
  }
  else
  {
    activity = EventPtr(new CNotificationEvent(
        24045, !msg.empty() ? msg : StringUtils::Format(g_localizeStrings.Get(24143), fileName),
        EventLevel::Error));

    if (IsModal())
      HELPERS::ShowOKDialogText(CVariant{fileName}, CVariant{msg});
  }

  auto eventLog = CServiceBroker::GetEventLog();
  if (eventLog)
    eventLog->Add(activity, !IsModal(), false);
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
    CLog::Log(LOGERROR, "CAddonUnInstallJob[{}]: failed to unload addon.", m_addon->ID());
    return false;
  }

  CFilesystemInstaller fsInstaller;
  if (!fsInstaller.UnInstallFromFilesystem(m_addon->Path()))
  {
    CLog::Log(LOGERROR, "CAddonUnInstallJob[{}]: could not delete addon data.", m_addon->ID());
    return false;
  }

  ClearFavourites();
  if (m_removeData)
  {
    CFileUtils::DeleteItem(m_addon->Profile());
  }

  AddonPtr addon;

  // try to get the addon object from the repository as the local one does not exist anymore
  // if that doesn't work fall back to the local one
  if (!CServiceBroker::GetAddonMgr().FindInstallableById(m_addon->ID(), addon) || addon == nullptr)
  {
    addon = m_addon;
  }

  auto eventLog = CServiceBroker::GetEventLog();
  if (eventLog)
    eventLog->Add(EventPtr(new CAddonManagementEvent(addon, 24144))); // Add-on uninstalled

  CServiceBroker::GetAddonMgr().OnPostUnInstall(m_addon->ID());

  CAddonDatabase database;
  if (database.Open())
    database.OnPostUnInstall(m_addon->ID());

  ADDON::OnPostUnInstall(m_addon);

  if (m_recurseOrphaned == RecurseOrphaned::CHOICE_YES)
  {
    const auto removedItems = CAddonInstaller::GetInstance().RemoveOrphanedDepsRecursively();

    if (removedItems.size() > 0)
    {
      CLog::Log(LOGINFO, "CAddonUnInstallJob[{}]: removed orphaned dependencies ({})",
                m_addon->ID(), StringUtils::Join(removedItems, ", "));
    }
  }

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

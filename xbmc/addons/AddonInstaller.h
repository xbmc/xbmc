#pragma once
/*
 *      Copyright (C) 2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/FileOperationJob.h"
#include "addons/Addon.h"
#include "utils/Stopwatch.h"
#include "threads/Event.h"

class CAddonInstaller : public IJobCallback
{
public:
  static CAddonInstaller &Get();

  bool IsDownloading() const;
  void GetInstallList(ADDON::VECADDONS &addons) const;
  bool GetProgress(const CStdString &addonID, unsigned int &percent) const;
  bool Cancel(const CStdString &addonID);

  /*! \brief Install an addon if it is available in a repository
   \param addonID the addon ID of the item to install
   \param force whether to force the install even if the addon is already installed (eg for updating). Defaults to false.
   \param referer string to use for referer for http fetch. Set to previous version when updating, parent when fetching a dependency
   \param background whether to install in the background or not. Defaults to true.
   \return true on successful install, false on failure.
   \sa DoInstall
   */
  bool Install(const CStdString &addonID, bool force = false, const CStdString &referer="", bool background = true);

  /*! \brief Install an addon from the given zip path
   \param path the zip file to install from
   \return true if successful, false otherwise
   \sa DoInstall
   */
  bool InstallFromZip(const CStdString &path);

  /*! \brief Install a set of addons from the official repository (if needed)
   \param addonIDs a set of addon IDs to install
   */
  void InstallFromXBMCRepo(const std::set<CStdString> &addonIDs);

  /*! \brief Check whether dependencies of an addon exist or are installable.
   Iterates through the addon's dependencies, checking they're installed or installable.
   Each dependency must also satisfies CheckDependencies in turn.
   \param addon the addon to check
   \return true if dependencies are available, false otherwise.
   */
  bool CheckDependencies(const ADDON::AddonPtr &addon);

  /*! \brief Update all repositories (if needed)
   Runs through all available repositories and queues an update of them if they
   need it (according to the set timeouts) or if forced.  Optionally busy wait
   until the repository updates are complete.
   \param force whether we should run an update regardless of the normal update cycle. Defaults to false.
   \param wait whether we should busy wait for the updates to be performed. Defaults to false.
   */

  /*! \brief Check if an installation job for a given add-on is already queued up
   *  \param ID The ID of the add-on
   *  \return true if a job exists, false otherwise
   */
  bool HasJob(const CStdString& ID) const;

  void UpdateRepos(bool force = false, bool wait = false);

  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
  void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job);

  class CDownloadJob
  {
  public:
    CDownloadJob(unsigned int id)
    {
      jobID = id;
      progress = 0;
    }
    unsigned int jobID;
    unsigned int progress;
  };

  typedef std::map<CStdString,CDownloadJob> JobMap;

private:
  // private construction, and no assignements; use the provided singleton methods
  CAddonInstaller();
  CAddonInstaller(const CAddonInstaller&);
  CAddonInstaller const& operator=(CAddonInstaller const&);
  virtual ~CAddonInstaller();

  /*! \brief Install an addon from a repository or zip
   \param addon the AddonPtr describing the addon
   \param hash the hash to verify the install. Defaults to "".
   \param update whether this is an update of an existing addon, or a new install. Defaults to false.
   \param referer string to use for referer for http fetch. Defaults to "".
   \param background whether to install in the background or not. Defaults to true.
   \return true on successful install, false on failure.
   */
  bool DoInstall(const ADDON::AddonPtr &addon, const CStdString &hash = "", bool update = false, const CStdString &referer = "", bool background = true);

  CCriticalSection m_critSection;
  JobMap m_downloadJobs;
  CStopWatch m_repoUpdateWatch;   ///< repository updates are done based on this counter
  unsigned int m_repoUpdateJob;   ///< the job ID of the repository updates
  CEvent m_repoUpdateDone;        ///< event set when the repository updates are complete
};

class CAddonInstallJob : public CFileOperationJob
{
public:
  CAddonInstallJob(const ADDON::AddonPtr &addon, const CStdString &hash = "", bool update = false, const CStdString &referer = "");

  virtual bool DoWork();

  /*! \brief return the id of the addon being installed
   \return id of the installing addon
   */
  CStdString AddonID() const;

  /*! \brief Delete an addon following install failure
   \param addonFolder - the folder to delete
   */
  static bool DeleteAddon(const CStdString &addonFolder);
private:
  bool OnPreInstall();
  void OnPostInstall(bool reloadAddon);
  bool Install(const CStdString &installFrom);
  bool DownloadPackage(const CStdString &path, const CStdString &dest);

  /*! \brief Queue a notification for addon installation/update failure
   \param addonID - addon id
   \param fileName - filename which is shown in case the addon id is unknown
   */
  void ReportInstallError(const CStdString& addonID, const CStdString& fileName);

  /*! \brief Check the hash of a downloaded addon with the hash in the repository
   \param addonZip - filename of the zipped addon to check
   \return true if the hash matches (or no hash is available on the repo), false otherwise
   */
  bool CheckHash(const CStdString& addonZip);

  ADDON::AddonPtr m_addon;
  CStdString m_hash;
  bool m_update;
  CStdString m_referer;
};

class CAddonUnInstallJob : public CFileOperationJob
{
public:
  CAddonUnInstallJob(const ADDON::AddonPtr &addon);

  virtual bool DoWork();
private:
  void OnPostUnInstall();

  ADDON::AddonPtr m_addon;
};

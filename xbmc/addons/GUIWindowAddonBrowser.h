#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "addons/Addon.h"
#include "windows/GUIMediaWindow.h"
#include "threads/CriticalSection.h"
#include "utils/Job.h"
#include "pictures/PictureThumbLoader.h"

class CFileItem;
class CFileItemList;
class CFileOperationJob;

class CDownloadJob
{
public:
  CDownloadJob(unsigned int id, const CStdString& thehash)
  {
    jobID = id;
    progress = 0;
    hash = thehash;
  }
  unsigned int jobID;
  unsigned int progress;
  CStdString hash;
};

class CGUIWindowAddonBrowser :
      public CGUIMediaWindow,
      public IJobCallback
{
public:
  CGUIWindowAddonBrowser(void);
  virtual ~CGUIWindowAddonBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);

  // job callback
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
  void OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job);

  unsigned int AddJob(const CStdString& path);

  /*! \brief Popup a selection dialog with a list of addons of the given type
   \param type the type of addon wanted
   \param addonID [out] the addon ID of the selected item
   \param showNone whether there should be a "None" item in the list (defaults to false)
   \return 1 if an addon was selected, 2 if "Get More" was chosen, or 0 if an error occurred or if the selection process was cancelled
   */
  static int SelectAddonID(ADDON::TYPE type, CStdString &addonID, bool showNone = false);

  /*! \brief Install an addon if it is available in a repository
   \param addonID the addon ID of the item to install
   \param force whether to force the install even if the addon is already installed (eg for updating). Defaults to false.
   \param referer string to use for referer for http fetch. Set to previous version when updating, parent when fetching a dependency
   */
  static void InstallAddon(const CStdString &addonID, bool force = false, const CStdString &referer="");

  /*! \brief Install a set of addons from the official repository (if needed)
   \param addonIDs a set of addon IDs to install
   */
  static void InstallAddonsFromXBMCRepo(const std::set<CStdString> &addonIDs);

  typedef std::map<CStdString,CDownloadJob> JobMap;
  JobMap m_downloadJobs;

protected:
  /* \brief set label2 of an item based on the Addon.Status property
   \param item the item to update
   */
  void SetItemLabel2(CFileItemPtr item);

  /*! \brief Queue a notification for addon installation/update failure
   \param addonID - addon id
   \param fileName - filename which is shown in case the addon id is unknown
   */
  void ReportInstallError(const CStdString& addonID, const CStdString& fileName);

  /*! \brief Queue a notification for addon installation/update failure when only zip filename is known
   \param zipName - zip filename (will try to guess addon ID from it)
   */
  void ReportInstallErrorZip(const CStdString& zipName);

  /*! \brief Check the hash of a downloaded addon with the hash in the repository
   \param addonZip - filename of the zipped addon to check
   \param hash - correct hash of the file 
   \return true if the hash matches (or no hash is available on the repo), false otherwise
   */
  bool CheckHash(const CStdString& addonZip, const CStdString& hash);
  
  void PromptForActivation(const ADDON::AddonPtr &addon, bool dontPrompt);

  void RegisterJob(const CStdString& id, unsigned int jobid, const CStdString& hash="");
  void UnRegisterJob(unsigned int jobID);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool Update(const CStdString &strDirectory);
  virtual CStdString GetStartFolder(const CStdString &dir);
private:
  CCriticalSection m_critSection;
  CPictureThumbLoader m_thumbLoader;

  ADDON::AddonPtr m_prompt;
  bool            m_promptReload;
};


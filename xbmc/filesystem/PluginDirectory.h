/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "SortFileItem.h"
#include "interfaces/generic/RunningScriptsHandler.h"
#include "threads/Event.h"

#include <atomic>
#include <memory>
#include <string>

class CURL;
class CFileItem;
class CFileItemList;

namespace XFILE
{

class CPluginDirectory : public IDirectory, public CRunningScriptsHandler<CPluginDirectory>
{
public:
  CPluginDirectory();
  ~CPluginDirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Resolve(CFileItem& item) const override;
  bool AllowAll() const override { return true; }
  bool Exists(const CURL& url) override { return true; }
  float GetProgress() const override;
  void CancelDirectory() override;
  static bool RunScriptWithParams(const std::string& strPath, bool resume);

  /*! \brief Get a reproducible CFileItem by trying to recursively resolve the plugin paths
  up to a maximum allowed limit. If no plugin paths exist it will be ignored.
  \param resultItem the CFileItem with plugin paths to be resolved.
  \return false if the plugin path cannot be resolved, true otherwise.
  */
  static bool GetResolvedPluginResult(CFileItem& resultItem);
  static bool GetPluginResult(const std::string& strPath, CFileItem &resultItem, bool resume);

  /*! \brief Check whether a plugin supports media library scanning.
  \param content content type - movies, tvshows, musicvideos.
  \param strPath full plugin url.
  \return true if scanning at specified url is allowed, false otherwise.
  */
  static bool IsMediaLibraryScanningAllowed(const std::string& content, const std::string& strPath);

  /*! \brief Check whether a plugin url exists by calling the plugin and checking result.
  Applies only to plugins that support media library scanning.
  \param content content type - movies, tvshows, musicvideos.
  \param strPath full plugin url.
  \return true if the plugin supports scanning and specified url exists, false otherwise.
  */
  static bool CheckExists(const std::string& content, const std::string& strPath);

  // callbacks from python
  static bool AddItem(int handle, const CFileItem *item, int totalItems);
  static bool AddItems(int handle, const CFileItemList *items, int totalItems);
  static void EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc);
  static void AddSortMethod(int handle, SORT_METHOD sortMethod, const std::string &labelMask, const std::string &label2Mask);
  static std::string GetSetting(int handle, const std::string &key);
  static void SetSetting(int handle, const std::string &key, const std::string &value);
  static void SetContent(int handle, const std::string &strContent);
  static void SetProperty(int handle, const std::string &strProperty, const std::string &strValue);
  static void SetResolvedUrl(int handle, bool success, const CFileItem* resultItem);
  static void SetLabel2(int handle, const std::string& ident);

protected:
  // implementations of CRunningScriptsHandler / CScriptRunner
  bool IsSuccessful() const override { return m_success; }
  bool IsCancelled() const override { return m_cancelled; }

private:
  bool StartScript(const std::string& strPath, bool resume);

  std::unique_ptr<CFileItemList> m_listItems;
  std::unique_ptr<CFileItem> m_fileResult;

  std::atomic<bool> m_cancelled;
  bool m_success = false; // set by script in EndOfDirectory
  int m_totalItems = 0; // set by script in AddDirectoryItem
};
}

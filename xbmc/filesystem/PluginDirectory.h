#pragma once
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

#include "IDirectory.h"
#include "SortFileItem.h"

#include <atomic>
#include <string>
#include <map>
#include "threads/CriticalSection.h"
#include "addons/IAddon.h"
#include "PlatformDefs.h"

#include "threads/Event.h"
#include "threads/Thread.h"

class CURL;
class CFileItem;
class CFileItemList;

namespace XFILE
{

class CPluginDirectory : public IDirectory
{
public:
  CPluginDirectory();
  ~CPluginDirectory(void);
  virtual bool GetDirectory(const CURL& url, CFileItemList& items);
  virtual bool AllowAll() const { return true; }
  virtual bool Exists(const CURL& url) { return true; }
  virtual float GetProgress() const;
  virtual void CancelDirectory();
  static bool RunScriptWithParams(const std::string& strPath);
  static bool GetPluginResult(const std::string& strPath, CFileItem &resultItem);

  // callbacks from python
  static bool AddItem(int handle, const CFileItem *item, int totalItems);
  static bool AddItems(int handle, const CFileItemList *items, int totalItems);
  static void EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc);
  static void AddSortMethod(int handle, SORT_METHOD sortMethod, const std::string &label2Mask);
  static std::string GetSetting(int handle, const std::string &key);
  static void SetSetting(int handle, const std::string &key, const std::string &value);
  static void SetContent(int handle, const std::string &strContent);
  static void SetProperty(int handle, const std::string &strProperty, const std::string &strValue);
  static void SetResolvedUrl(int handle, bool success, const CFileItem* resultItem);
  static void SetLabel2(int handle, const std::string& ident);

private:
  ADDON::AddonPtr m_addon;
  bool StartScript(const std::string& strPath, bool retrievingDir);
  bool WaitOnScriptResult(const std::string &scriptPath, int scriptId, const std::string &scriptName, bool retrievingDir);

  static std::map<int,CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void removeHandle(int handle);
  static CPluginDirectory *dirFromHandle(int handle);
  static CCriticalSection m_handleLock;
  static int handleCounter;

  CFileItemList* m_listItems;
  CFileItem*     m_fileResult;
  CEvent         m_fetchComplete;

  std::atomic<bool> m_cancelled;
  bool          m_success;      // set by script in EndOfDirectory
  int    m_totalItems;   // set by script in AddDirectoryItem

  class CScriptObserver : public CThread
  {
  public:
    CScriptObserver(int scriptId, CEvent &event);
    void Abort();
  protected:
    void Process() override;
    int m_scriptId;
    CEvent &m_event;
  };
};
}

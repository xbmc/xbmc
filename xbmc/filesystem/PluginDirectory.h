#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IDirectory.h"
#include "Directory.h"
#include "utils/StdString.h"
#include "SortFileItem.h"

#include <string>
#include <vector>
#include "threads/CriticalSection.h"
#include "addons/IAddon.h"
#include "PlatformDefs.h"

#include "threads/Event.h"

class CURL;
class CFileItemList;

namespace XFILE
{

class CPluginDirectory : public IDirectory
{
public:
  CPluginDirectory();
  ~CPluginDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
  virtual bool IsAllowed(const CStdString &strFile) const { return true; };
  virtual bool Exists(const char* strPath) { return true; }
  virtual float GetProgress() const;
  virtual void CancelDirectory();
  static bool RunScriptWithParams(const CStdString& strPath);
  static bool GetPluginResult(const CStdString& strPath, CFileItem &resultItem);

  // callbacks from python
  static bool AddItem(int handle, const CFileItem *item, int totalItems);
  static bool AddItems(int handle, const CFileItemList *items, int totalItems);
  static void EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc);
  static void AddSortMethod(int handle, SORT_METHOD sortMethod, const CStdString &label2Mask);
  static CStdString GetSetting(int handle, const CStdString &key);
  static void SetSetting(int handle, const CStdString &key, const CStdString &value);
  static void SetContent(int handle, const CStdString &strContent);
  static void SetProperty(int handle, const CStdString &strProperty, const CStdString &strValue);
  static void SetResolvedUrl(int handle, bool success, const CFileItem* resultItem);
  static void SetLabel2(int handle, const CStdString& ident);  

private:
  ADDON::AddonPtr m_addon;
  bool StartScript(const CStdString& strPath, bool retrievingDir);
  bool WaitOnScriptResult(const CStdString &scriptPath, const CStdString &scriptName, bool retrievingDir);

  static std::vector<CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void removeHandle(int handle);
  static CCriticalSection m_handleLock;

  CFileItemList* m_listItems;
  CFileItem*     m_fileResult;
  CEvent         m_fetchComplete;

  bool          m_cancelled;    // set to true when we are cancelled
  bool          m_success;      // set by script in EndOfDirectory
  int    m_totalItems;   // set by script in AddDirectoryItem
};
}

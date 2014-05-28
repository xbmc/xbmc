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

#ifndef FILESYSTEM_IDIRECTORY_H_INCLUDED
#define FILESYSTEM_IDIRECTORY_H_INCLUDED
#include "IDirectory.h"
#endif

#ifndef FILESYSTEM_DIRECTORY_H_INCLUDED
#define FILESYSTEM_DIRECTORY_H_INCLUDED
#include "Directory.h"
#endif

#ifndef FILESYSTEM_UTILS_STDSTRING_H_INCLUDED
#define FILESYSTEM_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif

#ifndef FILESYSTEM_SORTFILEITEM_H_INCLUDED
#define FILESYSTEM_SORTFILEITEM_H_INCLUDED
#include "SortFileItem.h"
#endif


#include <string>
#include <map>
#ifndef FILESYSTEM_THREADS_CRITICALSECTION_H_INCLUDED
#define FILESYSTEM_THREADS_CRITICALSECTION_H_INCLUDED
#include "threads/CriticalSection.h"
#endif

#ifndef FILESYSTEM_ADDONS_IADDON_H_INCLUDED
#define FILESYSTEM_ADDONS_IADDON_H_INCLUDED
#include "addons/IAddon.h"
#endif

#ifndef FILESYSTEM_PLATFORMDEFS_H_INCLUDED
#define FILESYSTEM_PLATFORMDEFS_H_INCLUDED
#include "PlatformDefs.h"
#endif


#ifndef FILESYSTEM_THREADS_EVENT_H_INCLUDED
#define FILESYSTEM_THREADS_EVENT_H_INCLUDED
#include "threads/Event.h"
#endif


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
  bool WaitOnScriptResult(const CStdString &scriptPath, int scriptId, const CStdString &scriptName, bool retrievingDir);

  static std::map<int,CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void removeHandle(int handle);
  static CPluginDirectory *dirFromHandle(int handle);
  static CCriticalSection m_handleLock;
  static int handleCounter;

  CFileItemList* m_listItems;
  CFileItem*     m_fileResult;
  CEvent         m_fetchComplete;

  bool          m_cancelled;    // set to true when we are cancelled
  bool          m_success;      // set by script in EndOfDirectory
  int    m_totalItems;   // set by script in AddDirectoryItem
};
}

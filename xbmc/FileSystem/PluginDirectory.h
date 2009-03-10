#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "FileSystem/IDirectory.h"
#include "FileSystem/Directory.h"
#include "StdString.h"
#include "SortFileItem.h"

#include <string>
#include <vector>
#include "../utils/CriticalSection.h"

class CURL;
class CFileItemList;

namespace DIRECTORY
{

class CPluginDirectory : public IDirectory
{
public:
  CPluginDirectory(void);
  ~CPluginDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList& items);
  virtual bool IsAllowed(const CStdString &strFile) const { return true; };
  static bool RunScriptWithParams(const CStdString& strPath);
  static bool HasPlugins(const CStdString &type);
  bool GetPluginsDirectory(const CStdString &type, CFileItemList &items);
  static void LoadPluginStrings(const CURL &url);
  static void ClearPluginStrings();
  bool StartScript(const CStdString& strPath);
  static bool GetPluginResult(const CStdString& strPath, CFileItem &resultItem);

  // callbacks from python
  static bool AddItem(int handle, const CFileItem *item, int totalItems);
  static bool AddItems(int handle, const CFileItemList *items, int totalItems);
  static void EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc);
  static void AddSortMethod(int handle, SORT_METHOD sortMethod);
  static void SetContent(int handle, const CStdString &strContent);
  static void SetProperty(int handle, const CStdString &strProperty, const CStdString &strValue);
  static void SetResolvedUrl(int handle, bool success, const CFileItem* resultItem);

private:
  bool WaitOnScriptResult(const CStdString &scriptPath, const CStdString &scriptName);

  static std::vector<CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void removeHandle(int handle);
  static CCriticalSection m_handleLock;

  CFileItemList* m_listItems;
  CFileItem*     m_fileResult;
  HANDLE         m_fetchComplete;

  bool          m_cancelled;    // set to true when we are cancelled
  bool          m_success;      // set by script in EndOfDirectory
  int    m_totalItems;   // set by script in AddDirectoryItem
};
}

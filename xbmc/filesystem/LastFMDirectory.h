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
#include "CurlFile.h"
#include "utils/XBMCTinyXML.h"
#include "threads/Thread.h"

class CGUIDialogProgress;

namespace XFILE
{
class CLastFMDirectory :
      public IDirectory, public IRunnable
{
public:
  CLastFMDirectory(void);
  virtual ~CLastFMDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual void Run();

  virtual bool IsAllowed(const CStdString &strFile) const { return true; };
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;
protected:
  void AddEntry(int iString, CStdString strPath, CStdString strIconPath, bool bFolder, CFileItemList &items);
  void AddListEntry(const char *name, const char *artist, const char *count, const char *date, const char *icon, CStdString strPath, CFileItemList &items);
  CStdString BuildURLFromInfo();
  bool RetrieveList(CStdString url);
  bool ParseArtistList(CStdString url, CFileItemList &items);
  bool ParseAlbumList(CStdString url, CFileItemList &items);
  bool ParseUserList(CStdString url, CFileItemList &items);
  bool ParseTagList(CStdString url, CFileItemList &items);
  bool ParseTrackList(CStdString url, CFileItemList &items);

  bool GetArtistInfo(CFileItemList &items);
  bool GetUserInfo(CFileItemList &items);
  bool GetTagInfo(CFileItemList &items);

  bool SearchSimilarTags(CFileItemList &items);
  bool SearchSimilarArtists(CFileItemList &items);

  bool m_Error;
  bool m_Downloaded;
  CXBMCTinyXML m_xmlDoc;

  XFILE::CCurlFile m_http;

  CStdString m_objtype;
  CStdString m_objname;
  CStdString m_encodedobjname;
  CStdString m_objrequest;

  CStdString m_strSource;
  CStdString m_strDestination;

  CGUIDialogProgress* m_dlgProgress;
};
}

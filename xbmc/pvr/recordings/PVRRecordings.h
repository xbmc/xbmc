#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include "PVRRecording.h"
#include "XBDateTime.h"
#include "threads/Thread.h"
#include "utils/Observer.h"
#include "video/VideoThumbLoader.h"

#define PVR_ALL_RECORDINGS_PATH_EXTENSION "-1"

namespace PVR
{
  class CPVRRecordings : public Observable
  {
  private:
    CCriticalSection             m_critSection;
    bool                         m_bIsUpdating;
    CVideoThumbLoader            m_thumbLoader;
    std::vector<CPVRRecording *> m_recordings;

    virtual void UpdateFromClients(void);
    virtual CStdString TrimSlashes(const CStdString &strOrig) const;
    virtual const CStdString GetDirectoryFromPath(const CStdString &strPath, const CStdString &strBase) const;
    virtual bool IsDirectoryMember(const CStdString &strDirectory, const CStdString &strEntryDirectory, bool bDirectMember = true) const;
    virtual void GetContents(const CStdString &strDirectory, CFileItemList *results);
    virtual void GetSubDirectories(const CStdString &strBase, CFileItemList *results, bool bAutoSkip = true);

    bool HasAllRecordingsPathExtension(const CStdString &strDirectory);
    CStdString AddAllRecordingsPathExtension(const CStdString &strDirectory);
    CStdString RemoveAllRecordingsPathExtension(const CStdString &strDirectory);

  public:
    CPVRRecordings(void);
    virtual ~CPVRRecordings(void) { Clear(); };

    int Load();
    void Unload();
    void Clear();
    void UpdateEntry(const CPVRRecording &tag);
    void UpdateFromClient(const CPVRRecording &tag) { UpdateEntry(tag); }

    /**
     * @brief refresh the recordings list from the clients.
     */
    void Update(void);

    int GetNumRecordings();
    int GetRecordings(CFileItemList* results);
    bool DeleteRecording(const CFileItem &item);
    bool RenameRecording(CFileItem &item, CStdString &strNewName);
    bool SetRecordingsPlayCount(const CFileItemPtr &item, int count);

    bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    CFileItemPtr GetByPath(const CStdString &path);
    void SetPlayCount(const CFileItem &item, int iPlayCount);
    void GetAll(CFileItemList &items);
  };
}

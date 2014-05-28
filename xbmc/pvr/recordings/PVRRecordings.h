#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#ifndef RECORDINGS_PVRRECORDING_H_INCLUDED
#define RECORDINGS_PVRRECORDING_H_INCLUDED
#include "PVRRecording.h"
#endif

#ifndef RECORDINGS_XBDATETIME_H_INCLUDED
#define RECORDINGS_XBDATETIME_H_INCLUDED
#include "XBDateTime.h"
#endif

#ifndef RECORDINGS_THREADS_THREAD_H_INCLUDED
#define RECORDINGS_THREADS_THREAD_H_INCLUDED
#include "threads/Thread.h"
#endif

#ifndef RECORDINGS_UTILS_OBSERVER_H_INCLUDED
#define RECORDINGS_UTILS_OBSERVER_H_INCLUDED
#include "utils/Observer.h"
#endif

#ifndef RECORDINGS_VIDEO_VIDEOTHUMBLOADER_H_INCLUDED
#define RECORDINGS_VIDEO_VIDEOTHUMBLOADER_H_INCLUDED
#include "video/VideoThumbLoader.h"
#endif


#define PVR_ALL_RECORDINGS_PATH_EXTENSION "-1"

namespace PVR
{
  class CPVRRecordings : public Observable
  {
  private:
    CCriticalSection             m_critSection;
    bool                         m_bIsUpdating;
    std::vector<CPVRRecording *> m_recordings;
    unsigned int                 m_iLastId;

    virtual void UpdateFromClients(void);
    virtual CStdString TrimSlashes(const CStdString &strOrig) const;
    virtual const CStdString GetDirectoryFromPath(const CStdString &strPath, const CStdString &strBase) const;
    virtual bool IsDirectoryMember(const CStdString &strDirectory, const CStdString &strEntryDirectory, bool bDirectMember = true) const;
    virtual void GetContents(const CStdString &strDirectory, CFileItemList *results);
    virtual void GetSubDirectories(const CStdString &strBase, CFileItemList *results);

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
    CFileItemPtr GetById(unsigned int iId) const;

    bool HasAllRecordingsPathExtension(const CStdString &strDirectory);
  };
}

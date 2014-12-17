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
    typedef std::map<CPVRRecordingUid, CPVRRecordingPtr> PVR_RECORDINGMAP;
    typedef PVR_RECORDINGMAP::const_iterator             PVR_RECORDINGMAP_CITR;

    CCriticalSection             m_critSection;
    bool                         m_bIsUpdating;
    PVR_RECORDINGMAP             m_recordings;
    unsigned int                 m_iLastId;
    bool                         m_bGroupItems;

    virtual void UpdateFromClients(void);
    virtual std::string TrimSlashes(const std::string &strOrig) const;
    virtual const std::string GetDirectoryFromPath(const std::string &strPath, const std::string &strBase) const;
    virtual bool IsDirectoryMember(const std::string &strDirectory, const std::string &strEntryDirectory) const;
    virtual void GetSubDirectories(const std::string &strBase, CFileItemList *results);
    CPVRRecordingPtr GetByFileItem(const CFileItem &item) const;

    /**
     * @brief recursively deletes all recordings in the specified directory
     * @param item the directory
     * @return true if all recordings were deleted
     */
    bool DeleteDirectory(const CFileItem &item);
    bool DeleteRecording(const CFileItem &item);

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
    
    /**
     * Deletes the item in question, be it a directory or a file
     * @param item the item to delete
     * @return whether the item was deleted successfully
     */
    bool Delete(const CFileItem &item);
    bool RenameRecording(CFileItem &item, std::string &strNewName);
    bool SetRecordingsPlayCount(const CFileItemPtr &item, int count);

    bool GetDirectory(const std::string& strPath, CFileItemList &items);
    CFileItemPtr GetByPath(const std::string &path);
    CPVRRecordingPtr GetById(int iClientId, const std::string &strRecordingId) const;
    void GetAll(CFileItemList &items);
    CFileItemPtr GetById(unsigned int iId) const;

    void SetGroupItems(bool value) { m_bGroupItems = value; };
    bool IsGroupItems() const { return m_bGroupItems; };
  };
}

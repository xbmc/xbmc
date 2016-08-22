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

#include <memory>
#include <map>

#include "FileItem.h"
#include "video/VideoDatabase.h"

#include "PVRRecording.h"

namespace EPG
{
  class CEpgInfoTag;
  typedef std::shared_ptr<EPG::CEpgInfoTag> CEpgInfoTagPtr;
}

namespace PVR
{
  class CPVRRecordingsPath;

  class CPVRRecordings
  {
  private:
    typedef std::map<CPVRRecordingUid, CPVRRecordingPtr> PVR_RECORDINGMAP;
    typedef PVR_RECORDINGMAP::iterator             PVR_RECORDINGMAP_ITR;
    typedef PVR_RECORDINGMAP::const_iterator             PVR_RECORDINGMAP_CITR;

    CCriticalSection             m_critSection;
    bool                         m_bIsUpdating;
    PVR_RECORDINGMAP             m_recordings;
    unsigned int                 m_iLastId;
    CVideoDatabase               m_database;
    bool                         m_bDeletedTVRecordings;
    bool                         m_bDeletedRadioRecordings;
    unsigned int                 m_iTVRecordings;
    unsigned int                 m_iRadioRecordings;

    virtual void UpdateFromClients(void);
    virtual std::string TrimSlashes(const std::string &strOrig) const;
    virtual bool IsDirectoryMember(const std::string &strDirectory, const std::string &strEntryDirectory) const;
    virtual void GetSubDirectories(const CPVRRecordingsPath &recParentPath, CFileItemList *results);

    /**
     * @brief recursively deletes all recordings in the specified directory
     * @param item the directory
     * @return true if all recordings were deleted
     */
    bool DeleteDirectory(const CFileItem &item);
    bool DeleteRecording(const CFileItem &item);

    /**
     * @brief special value for parameter count of method ChangeRecordingsPlayCount
     */
    static const int INCREMENT_PLAY_COUNT = -1;

    /**
     * @brief change the playcount of the given recording or recursively of all children of the given recordings folder
     * @param item the recording or directory containing recordings
     * @param count the new playcount or INCREMENT_PLAY_COUNT to denote that the current playcount(s) are to be incremented by one
     * @return true if all playcounts were changed
     */
    bool ChangeRecordingsPlayCount(const CFileItemPtr &item, int count);

  public:
    CPVRRecordings(void);
    virtual ~CPVRRecordings(void);

    int Load();
    void Clear();
    void UpdateFromClient(const CPVRRecordingPtr &tag);

    /**
     * @brief refresh the recordings list from the clients.
     */
    void Update(void);

    int GetNumTVRecordings() const;
    bool HasDeletedTVRecordings() const;
    int GetNumRadioRecordings() const;
    bool HasDeletedRadioRecordings() const;

    /**
     * Deletes the item in question, be it a directory or a file
     * @param item the item to delete
     * @return whether the item was deleted successfully
     */
    bool Delete(const CFileItem &item);
    bool Undelete(const CFileItem &item);
    bool DeleteAllRecordingsFromTrash();
    bool RenameRecording(CFileItem &item, std::string &strNewName);
    bool SetRecordingsPlayCount(const CFileItemPtr &item, int count);
    bool IncrementRecordingsPlayCount(const CFileItemPtr &item);

    bool GetDirectory(const std::string& strPath, CFileItemList &items);
    CFileItemPtr GetByPath(const std::string &path);
    CPVRRecordingPtr GetById(int iClientId, const std::string &strRecordingId) const;
    void GetAll(CFileItemList &items, bool bDeleted = false);
    CFileItemPtr GetById(unsigned int iId) const;

    /*!
     * @brief Get the recording for the given epg tag, if any.
     * @param epgTag The epg tag.
     * @return The requested recording, or an empty recordingptr if none was found.
     */
    CPVRRecordingPtr GetRecordingForEpgTag(const EPG::CEpgInfoTagPtr &epgTag) const;
  };
}

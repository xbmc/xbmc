/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class CVideoDatabase;

namespace PVR
{
  class CPVRClient;
  class CPVREpgInfoTag;
  class CPVRRecording;
  class CPVRRecordingUid;
  class CPVRRecordingsPath;

  class CPVRRecordings
  {
  public:
    CPVRRecordings();
    virtual ~CPVRRecordings();

    /*!
     * @brief (re)load the recordings from the clients.
     * @return the number of recordings loaded.
     */
    int Load();

    /*!
     * @brief unload all recordings.
     */
    void Unload();

    /*!
     * @brief client has delivered a new/updated recording.
     * @param tag The recording
     * @param client The client the recording belongs to.
     */
    void UpdateFromClient(const std::shared_ptr<CPVRRecording>& tag, const CPVRClient& client);

    /*!
     * @brief refresh the recordings list from the clients.
     */
    void Update();

    /*!
     * @brief refresh the size of any in progress recordings from the clients.
     */
    void UpdateInProgressSize();

    int GetNumTVRecordings() const;
    bool HasDeletedTVRecordings() const;
    int GetNumRadioRecordings() const;
    bool HasDeletedRadioRecordings() const;

    /*!
     * @brief Set a recording's watched state
     * @param recording The recording
     * @param bWatched True to set watched, false to set unwatched state
     * @return True on success, false otherwise
     */
    bool MarkWatched(const std::shared_ptr<CPVRRecording>& recording, bool bWatched);

    /*!
     * @brief Reset a recording's resume point, if any
     * @param recording The recording
     * @return True on success, false otherwise
     */
    bool ResetResumePoint(const std::shared_ptr<CPVRRecording>& recording);

    /*!
     * @brief Get a list of all recordings
     * @return the list of all recordings
     */
    std::vector<std::shared_ptr<CPVRRecording>> GetAll() const;

    std::shared_ptr<CPVRRecording> GetByPath(const std::string& path) const;
    std::shared_ptr<CPVRRecording> GetById(int iClientId, const std::string& strRecordingId) const;
    std::shared_ptr<CPVRRecording> GetById(unsigned int iId) const;

    /*!
     * @brief Get the recording for the given epg tag, if any.
     * @param epgTag The epg tag.
     * @return The requested recording, or an empty recordingptr if none was found.
     */
    std::shared_ptr<CPVRRecording> GetRecordingForEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const;

    /*!
     * @brief Erase stale texture db entries and image files.
     * @return number of cleaned up images.
     */
    int CleanupCachedImages();

  private:
    mutable CCriticalSection m_critSection;
    bool m_bIsUpdating = false;
    std::map<CPVRRecordingUid, std::shared_ptr<CPVRRecording>> m_recordings;
    unsigned int m_iLastId = 0;
    std::unique_ptr<CVideoDatabase> m_database;
    bool m_bDeletedTVRecordings = false;
    bool m_bDeletedRadioRecordings = false;
    unsigned int m_iTVRecordings = 0;
    unsigned int m_iRadioRecordings = 0;

    void UpdateFromClients();

    /*!
     * @brief Get/Open the video database.
     * @return A reference to the video database.
     */
    CVideoDatabase& GetVideoDatabase();

    /*!
     * @brief Set a recording's play count
     * @param recording The recording
     * @param count The new play count
     * @return True on success, false otherwise
     */
    bool SetRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording, int count);

    /*!
     * @brief Increment a recording's play count
     * @param recording The recording
     * @return True on success, false otherwise
     */
    bool IncrementRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording);

    /*!
     * @brief special value for parameter count of method ChangeRecordingsPlayCount
     */
    static const int INCREMENT_PLAY_COUNT = -1;

    /*!
     * @brief change the play count of the given recording
     * @param recording The recording
     * @param count The new play count or INCREMENT_PLAY_COUNT to denote that the current play count is to be incremented by one
     * @return true if the play count was changed successfully
     */
    bool ChangeRecordingsPlayCount(const std::shared_ptr<CPVRRecording>& recording, int count);
  };
}

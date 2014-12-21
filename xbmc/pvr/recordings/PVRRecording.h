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

/*
 * DESCRIPTION:
 *
 * CPVRRecordingInfoTag is part of the XBMC PVR system to support recording entrys,
 * stored on a other Backend like VDR or MythTV.
 *
 * The recording information tag holds data about name, length, recording time
 * and so on of recorded stream stored on the backend.
 *
 * The filename string is used to by the PVRManager and passed to DVDPlayer
 * to stream data from the backend to XBMC.
 *
 * It is a also CVideoInfoTag and some of his variables must be set!
 *
 */

#include "addons/include/xbmc_pvr_types.h"
#include "video/VideoInfoTag.h"
#include "XBDateTime.h"

namespace PVR
{
  class CPVRRecording;

  typedef std::shared_ptr<PVR::CPVRRecording> CPVRRecordingPtr;

  /*!
   * @brief Representation of a CPVRRecording unique ID.
   */
  class CPVRRecordingUid
  {
  public:
    int           m_iClientId;        /*!< ID of the backend */
    std::string   m_strRecordingId;   /*!< unique ID of the recording on the client */

    CPVRRecordingUid();
    CPVRRecordingUid(const CPVRRecordingUid& recordingId);
    CPVRRecordingUid(int iClientId, const std::string &strRecordingId);

    bool operator >(const CPVRRecordingUid& right) const;
    bool operator <(const CPVRRecordingUid& right) const;
    bool operator ==(const CPVRRecordingUid& right) const;
    bool operator !=(const CPVRRecordingUid& right) const;
  };

  class CPVRRecording : public CVideoInfoTag
  {
  public:
    int           m_iClientId;        /*!< ID of the backend */
    std::string   m_strRecordingId;   /*!< unique ID of the recording on the client */
    std::string   m_strChannelName;   /*!< name of the channel this was recorded from */
    CDateTimeSpan m_duration;         /*!< duration of this recording */
    int           m_iPriority;        /*!< priority of this recording */
    int           m_iLifetime;        /*!< lifetime of this recording */
    std::string   m_strStreamURL;     /*!< stream URL. if empty use pvr client */
    std::string   m_strDirectory;     /*!< directory of this recording on the client */
    std::string   m_strIconPath;      /*!< icon path */
    std::string   m_strThumbnailPath; /*!< thumbnail path */
    std::string   m_strFanartPath;    /*!< fanart path */
    unsigned      m_iRecordingId;     /*!< id that won't change while xbmc is running */

    CPVRRecording(void);
    CPVRRecording(const PVR_RECORDING &recording, unsigned int iClientId);
    virtual ~CPVRRecording() {};

    bool operator ==(const CPVRRecording& right) const;
    bool operator !=(const CPVRRecording& right) const;

    virtual void Serialize(CVariant& value) const;

    /*!
     * @brief Reset this tag to it's initial state.
     */
    void Reset(void);

    /*!
     * @brief The duration of this recording in seconds.
     * @return The duration.
     */
    int GetDuration() const;

    /*!
     * @brief Delete this recording on the client (if supported).
     * @return True if it was deleted successfully, false otherwise.
     */
    bool Delete(void);

    /*!
     * @brief Rename this recording on the client (if supported).
     * @param strNewName The new name.
     * @return True if it was renamed successfully, false otherwise.
     */
    bool Rename(const std::string &strNewName);

    /*!
     * @brief Set this recording's play count on the client (if supported).
     * @param count play count.
     * @return True if play count was set successfully, false otherwise.
     */
    bool SetPlayCount(int count);

    /*!
     * @brief Increment this recording's play count on the client (if supported).
     * @return True if play count was set successfully, false otherwise.
     */
    bool IncrementPlayCount();

    /*!
     * @brief Set the last watched position of a recording on the backend.
     * @param position The last watched position in seconds
     * @return True if the last played position was updated successfully, false otherwise
     */
    bool SetLastPlayedPosition(int lastplayedposition);

    /*!
     * @brief Retrieve the last watched position of a recording on the backend.
     * @return The last watched position in seconds
     */
    int GetLastPlayedPosition() const;

    /*!
     * @brief Retrieve the edit decision list (EDL) of a recording on the backend.
     * @return The edit decision list (empty on error)
     */
    std::vector<PVR_EDL_ENTRY> GetEdl() const;

    /*!
     * @brief Get the resume point and play count from the database if the 
     * client doesn't handle it itself.
     */
    void UpdateMetadata(void);

    /*!
     * @brief Update this tag with the contents of the given tag.
     * @param tag The new tag info.
     */
    void Update(const CPVRRecording &tag);

    const CDateTime &RecordingTimeAsUTC(void) const { return m_recordingTime; }
    const CDateTime &RecordingTimeAsLocalTime(void) const;
    void SetRecordingTimeFromUTC(CDateTime &recordingTime) { m_recordingTime = recordingTime; }
    void SetRecordingTimeFromLocalTime(CDateTime &recordingTime) { m_recordingTime = recordingTime.GetAsUTCDateTime(); }

    /*!
     * @brief Retrieve the recording title from the URL path
     * @param url the URL for the recording
     * @return Title of the recording
     */
    static std::string GetTitleFromURL(const std::string &url);

    /*!
     * @brief Copy some information from the client to the given video info tag
     * @param target video info tag to which the information will be copied
     */
    void CopyClientInfo(CVideoInfoTag *target) const;

  private:
    CDateTime m_recordingTime; /*!< start time of the recording */
    bool      m_bGotMetaData;

    void UpdatePath(void);
    void DisplayError(PVR_ERROR err) const;
  };
}

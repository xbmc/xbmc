#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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
#include "DateTime.h"

class CPVRRecording : public CVideoInfoTag
{
public:
  int           m_iClientId;      /*!< ID of the backend */
  int           m_iClientIndex;   /*!< index number of the recording on the client, -1 for unknown */
  CStdString    m_strChannelName; /*!< name of the channel this was recorded from */
  CDateTime     m_recordingTime;  /*!< start time of the recording */
  CDateTimeSpan m_duration;       /*!< duration of this recording */
  int           m_iPriority;      /*!< priority of this recording */
  int           m_iLifetime;      /*!< lifetime of this recording */
  CStdString    m_strStreamURL;   /*!< stream URL. if empty use pvr client */
  CStdString    m_strDirectory;   /*!< directory of this recording on the client */

  CPVRRecording(void);
  CPVRRecording(const PVR_RECORDING &recording, unsigned int iClientId);
  virtual ~CPVRRecording() {};

  bool operator ==(const CPVRRecording& right) const;
  bool operator !=(const CPVRRecording& right) const;

  /*!
   * @brief Reset this tag to it's initial state.
   */
  void Reset(void);

  /*!
   * @brief The duration of this recording in minutes.
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
  bool Rename(const CStdString &strNewName);

  /*!
   * @brief Update this tag with the contents of the given tag.
   * @param tag The new tag info.
   */
  void Update(const CPVRRecording &tag);

private:
  void UpdatePath(void);
  void DisplayError(PVR_ERROR err) const;
};

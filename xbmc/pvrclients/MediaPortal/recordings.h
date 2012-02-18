#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include "libXBMC_addon.h"
#include "libXBMC_pvr.h"
#include "Cards.h"

using namespace std;

#define DEFAULTFRAMESPERSECOND 25.0
#define MAXPRIORITY 99
#define MAXLIFETIME 99 //Based on VDR addon and VDR documentation. 99=Keep forever, 0=can be deleted at any time, 1..98=days to keep

class cRecording
{
private:
  int m_Index;
  string m_channelName;
  string m_filePath;          ///< The full recording path as returned by the backend
  string m_basePath;          ///< The base path shared by all recordings (to be determined from the Card settings)
  string m_directory;         ///< An optional subdirectory below the basePath
  string m_fileName;          ///< The recording filename without path
  string m_stream;
  string m_originalurl;
  time_t m_StartTime;
  int m_Duration;
  string m_title;             // Title of this event
  string m_description;       // Description of this event
  string m_episodeName;       // Short description of this event (typically the episode name in case of a series)
  string m_seriesNumber;
  string m_episodeNumber;
  string m_episodePart;
  int m_scheduleID;
  int m_keepUntil;
  time_t m_keepUntilDate;     ///< MediaPortal keepUntilDate
  CCards* m_cardSettings;     ///< Pointer to the MediaPortal card settings. Will be used to determine the base path of the recordings

public:
  cRecording();
  virtual ~cRecording();

  bool ParseLine(const std::string& data);
  const char *ChannelName(void) const { return m_channelName.c_str(); }
  int Index(void) const { return m_Index; }
  time_t StartTime(void) const { return m_StartTime; }
  time_t Duration(void) const { return m_Duration; }
  const char *Title(void) const { return m_title.c_str(); }
  const char *Description(void) const { return m_description.c_str(); }
  const char *EpisodeName(void) const { return m_episodeName.c_str(); }
  const char *SeriesNumber(void) const { return m_seriesNumber.c_str(); }
  const char *EpisodeNumber(void) const { return m_episodeNumber.c_str(); }
  const char *EpisodePart(void) const { return m_episodePart.c_str(); }
  int ScheduleID(void) const { return m_scheduleID; }
  int Lifetime(void) const;

  /**
   * \brief Filename of this recording with full path (at server side)
   */
  const char *FilePath(void) const { return m_filePath.c_str(); }

  /**
   * \brief Filename of this recording without full path
   * \return Filename
   */
  const char *FileName(void) const { return m_fileName.c_str(); }

  /**
   * \brief Directory where this recording is stored (at server side)
   * \return Filename
   */
  const char *Directory(void) const { return m_directory.c_str(); }

  /**
   * \brief Override the directory where this recording is stored
   */
  void SetDirectory( string& directory );

  /**
   * \brief The RTSP stream URL for this recording (hostname resolved to IP-address)
   * \return Stream URL
   */
  const char *Stream(void) const { return m_stream.c_str(); }

  /**
   * \brief The RTSP stream URL for this recording (unresolved hostname)
   * \return Stream URL
   */
  const char *OrignalURL(void) const { return m_originalurl.c_str(); }

  /**
   * \brief Pass a pointer to the MediaPortal card settings to this class
   * \param the cardSettings
   */
  void SetCardSettings(CCards* cardSettings);

  /**
   * \brief Parse Recording file path and divide it in 3 parts: base path, subdirectory and filename;
   */
  void SplitFilePath(void);
};

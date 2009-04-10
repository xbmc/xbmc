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

/*
 * for DESCRIPTION see 'TVRecordInfoTag.cpp'
 */

#include "VideoInfoTag.h"
#include "TVEPGInfoTag.h"
#include "settings/VideoSettings.h"
#include "DateTime.h"

struct CutMark_t
{
  CStdString  m_comment;              /// Comment string
  int         m_position;             /// Offset time from beginning in millisecond
}; typedef std::vector< CutMark_t > CutMarks;

class CTVRecordingInfoTag : public CVideoInfoTag
{
public:
  CTVRecordingInfoTag();
  //CTVRecordingInfoTag(long uniqueRecordingID);

  bool operator ==(const CTVRecordingInfoTag& right) const;
  bool operator !=(const CTVRecordingInfoTag& right) const;

  const long GetDbID(void) const { return m_uniqueRecordingID; };

  void Reset(void);

  bool HaveMarks(void) { return m_cutMarks.size() > 0 ? true : false; }

  void AddMark(int position, const CStdString &comment);
  void DeleteAllMarks(void);
  bool DeleteMark(int position);
  int GetMarkPrev(int position);
  int GetMarkNext(int position);

  int           m_Index;              /// Index number of the tag, given by the backend, -1 for unknown
  int           m_channelNum;         /// Channel number where recording from
  CStdString    m_strChannel;         /// Channel name where recording from
  CStdString    m_strFileNameAndPath; /// Filename for PVRManager to open and read stream

  CStdString    m_Summary;            /// Summary string with the time to show inside a GUI list
  /// see PVRManager.cpp for format.

  CStdString    m_seriesID;           /// Series ID (used?)
  CStdString    m_episodeID;          /// Episiode ID (used?)

  CDateTime     m_startTime;          /// Recording start time
  CDateTime     m_endTime;            /// Recording end time
  CDateTimeSpan m_duration;           /// Duration

  VideoProps    m_videoProps;         /// Types of video inside stream
  AudioProps    m_audioProps;         /// Types of audio inside stream
  SubtitleTypes m_subTypes;           /// Types of subtitles inside stream

  CutMarks      m_cutMarks;           /// Data array of cutting marks

  bool          m_commFree;           /// Any commercials inside stream (how detect it?)
  CStdString    m_strClient;
  uint64_t      m_resumePoint;

  /**
   * Following values are for individual video settings
   **/
  CVideoSettings m_defaultVideoSettings;

private:
  void SortMarks(void);

  long          m_uniqueRecordingID;  /// db's unique identifier for this tag
};

typedef std::vector<CTVRecordingInfoTag> VECRECORDINGS;

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
 * DESCRIPTION:
 *
 * CTVRecordingInfoTag is part of the PVRManager to support recording entrys.
 *
 * The recording information tag holds data about name, length, recording time
 * and so on of recorded stream stored on the backend.
 *
 * The filename string is used to by the PVRManager and passed to DVDPlayer
 * to stream data from the backend to XBMC.
 *
 * It is a also CVideoInfoTag and some of his variables must be set!
 *
 * TODO:
 * Nothing in the moment. Any ideas?
 *
 */

#include "stdafx.h"
#include "TVRecordInfoTag.h"


/**
 * Create a blank unmodified recording tag
 */
CTVRecordingInfoTag::CTVRecordingInfoTag()
{
  Reset();
}

bool CTVRecordingInfoTag::operator ==(const CTVRecordingInfoTag& right) const
{

  if (this == &right) return true;

  return (m_Index         == right.m_Index &&
          m_channelNum    == right.m_channelNum &&
          m_strChannel    == right.m_strChannel &&
          m_startTime     == right.m_startTime &&
          m_endTime       == right.m_endTime &&
          m_duration      == right.m_duration &&
          m_seriesID      == right.m_seriesID &&
          m_episodeID     == right.m_episodeID &&
          m_commFree      == right.m_commFree &&
          m_strPlotOutline== right.m_strPlotOutline &&
          m_strPlot       == right.m_strPlot &&
          m_strFileNameAndPath == right.m_strFileNameAndPath &&
          m_strClient     == right.m_strClient &&
          m_resumePoint   == right.m_resumePoint &&
          m_strTitle      == right.m_strTitle);
}

bool CTVRecordingInfoTag::operator !=(const CTVRecordingInfoTag& right) const
{

  if (this == &right) return false;

  if (m_Index         != right.m_Index) return true;
  if (m_channelNum    != right.m_channelNum) return true;
  if (m_strChannel    != right.m_strChannel) return true;
  if (m_startTime     != right.m_startTime) return true;
  if (m_endTime       != right.m_endTime) return true;
  if (m_duration      != right.m_duration) return true;
  if (m_seriesID      != right.m_seriesID) return true;
  if (m_episodeID     != right.m_episodeID) return true;
  if (m_commFree      != right.m_commFree) return true;
  if (m_strPlotOutline!= right.m_strPlotOutline) return true;
  if (m_strPlot       != right.m_strPlot) return true;
  if (m_strFileNameAndPath != right.m_strFileNameAndPath) return true;
  if (m_strTitle      != right.m_strTitle) return true;
  if (m_strClient     != right.m_strClient) return true;
  if (m_resumePoint   != right.m_resumePoint) return true;

  return false;
}

/**
 * Add a cut mark to info tag
 * \param int position              = Cut mark position
 * \param const CStdString &comment = Comment string
 */
void CTVRecordingInfoTag::AddMark(int position, const CStdString &comment)
{
  CutMark_t mark;
  mark.m_comment = comment;
  mark.m_position = position;
  m_cutMarks.push_back(mark);
  SortMarks();
}

/**
 * Delete cut mark on given position
 * \param int position              = Cut mark to delete
 * \return bool                     = true if deletet, false if not found
 */
bool CTVRecordingInfoTag::DeleteMark(int position)
{
  std::vector<CutMark_t>::iterator it;

  for (it = m_cutMarks.begin(); it != m_cutMarks.end(); ++it)
  {
    if ((*it).m_position == position)
    {
      m_cutMarks.erase(it);
      return true;
    }
  }

  return false;
}

/**
 * Delete all cut marks
 */
void CTVRecordingInfoTag::DeleteAllMarks(void)
{
  m_cutMarks.clear();
  return;
}

/**
 * Get cut mark in front of given position
 * \param int position              = current mark
 * \return int                      = previous mark or -1 if current is first
 */
int CTVRecordingInfoTag::GetMarkPrev(int position)
{
  std::vector<CutMark_t>::iterator it;

  for (it = m_cutMarks.end(); it != m_cutMarks.begin(); --it)
  {
    if ((*it).m_position < position)
    {
      return (*it).m_position;
    }
  }

  return -1;
}

/**
 * Get cut mark behind given position
 * \param int position              = current mark
 * \return int                      = next mark or -1 if current is last
 */
int CTVRecordingInfoTag::GetMarkNext(int position)
{
  std::vector<CutMark_t>::iterator it;

  for (it = m_cutMarks.begin(); it != m_cutMarks.end(); ++it)
  {
    if ((*it).m_position > position)
    {
      return (*it).m_position;
    }
  }

  return -1;
}

/**
 * Sort recording cut marks array (lowest position first)
 */
void CTVRecordingInfoTag::SortMarks(void)
{
  std::vector<CutMark_t>::iterator it1;
  std::vector<CutMark_t>::iterator it2;

  for (it1 = m_cutMarks.begin(); it1 != m_cutMarks.end(); ++it1)
  {
    for (it2 = m_cutMarks.end(); it2 != m_cutMarks.begin(); --it2)
    {
      if ((*it2).m_position < (*it1).m_position)
      {
        /* Swap position */
        int tmp_i           = (*it1).m_position;
        (*it1).m_position   = (*it2).m_position ;
        (*it2).m_position   = tmp_i;
        /* Swap comment */
        CStdString tmp_s    = (*it1).m_comment;
        (*it1).m_comment    = (*it2).m_comment;
        (*it2).m_comment    = tmp_s;
      }
    }
  }

  return;
}

/**
 * Initialize blank CTVRecordingInfoTag
 */
void CTVRecordingInfoTag::Reset(void)
{
  m_Index                 = -1;
  m_channelNum            = -1;
  m_strChannel            = "";
  m_seriesID              = "";
  m_episodeID             = "";

  m_Summary               = "";

  m_strFileNameAndPath    = "";
  m_strClient             = "";
  m_resumePoint           = 0;

  m_videoProps.clear();
  m_audioProps.clear();
  m_subTypes.clear();

  m_cutMarks.clear();

  m_commFree              = false;

  CVideoInfoTag::Reset();
}

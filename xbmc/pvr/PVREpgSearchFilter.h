#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "DateTime.h"

class CPVREpgInfoTag;

/* Filter data to check with a EPGEntry */
struct PVREpgSearchFilter
{
  void Reset();
  bool FilterEntry(const CPVREpgInfoTag &tag) const;

  CStdString    m_strSearchTerm;
  bool          m_bIsCaseSensitive;
  bool          m_bSearchInDescription;
  int           m_iGenreType;
  int           m_iGenreSubType;
  int           m_iMinimumDuration;
  int           m_iMaximumDuration;
  SYSTEMTIME    m_startTime;
  SYSTEMTIME    m_endTime;
  SYSTEMTIME    m_startDate;
  SYSTEMTIME    m_endDate;
  int           m_iChannelNumber;
  bool          m_bFTAOnly;
  bool          m_bIncludeUnknownGenres;
  int           m_iChannelGroup;
  bool          m_bIgnorePresentTimers;
  bool          m_bIgnorePresentRecordings;
  bool          m_bPreventRepeats;
};

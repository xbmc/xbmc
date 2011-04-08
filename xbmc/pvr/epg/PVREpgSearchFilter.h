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

#include "XBDateTime.h"
#include "epg/EpgSearchFilter.h"

class CPVREpgInfoTag;

/** Filter to apply with on a CPVREpgInfoTag */

struct PVREpgSearchFilter : public EpgSearchFilter
{
  /*!
   * @brief Clear this filter.
   */
  void Reset();

  /*!
   * @brief Check if a tag will be filtered or not.
   * @param tag The tag to check.
   * @return True if this tag matches the filter, false if not.
   */
  bool FilterEntry(const CPVREpgInfoTag &tag) const;

  int           m_iChannelNumber;           /*!< The channel number in XBMC */
  bool          m_bFTAOnly;                 /*!< Free to air only or not */
  int           m_iChannelGroup;            /*!< The group this channel belongs to */
  bool          m_bIgnorePresentTimers;     /*!< True to ignore currently present timers (future recordings), false if not */
  bool          m_bIgnorePresentRecordings; /*!< True to ignore currently active recordings, false if not */
};

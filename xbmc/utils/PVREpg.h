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

#include "PVRChannel.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVREpgInfoTag;
class CPVREpgs;

class CPVREpg : public std::vector<CPVREpgInfoTag*>
{
  friend class CPVREpgs;

private:
  const CPVRChannel *m_Channel;
  bool m_bUpdateRunning;
  bool m_bValid;
  bool m_bIsSorted;

  bool UpdateFromScraper(time_t start, time_t end);
  bool UpdateFromClient(time_t start, time_t end);

public:
  CPVREpg(const CPVRChannel &channel);
  ~CPVREpg();
  bool IsValid(void) const;
  const CPVRChannel *ChannelTag(void) const { return m_Channel; }
  void DelInfoTag(CPVREpgInfoTag *tag);
  void Cleanup(const CDateTime Time);
  void Cleanup(void);
  void Clear();
  void Sort(void);
  const CPVREpgInfoTag *GetInfoTagNow(void) const;
  const CPVREpgInfoTag *GetInfoTagNext(void) const;
  const CPVREpgInfoTag *GetInfoTag(long uniqueID, CDateTime StartTime) const;
  const CPVREpgInfoTag *GetInfoTagAround(CDateTime Time) const;
  CDateTime GetLastEPGDate();
  bool IsUpdateRunning() const { return m_bUpdateRunning; }
  void SetUpdate(bool OnOff) { m_bUpdateRunning = OnOff; }

  bool Add(const PVR_PROGINFO *data, bool bUpdateDatabase = false);

  /**
   * Remove overlapping events from the tables
   */
  bool RemoveOverlappingEvents();

  bool Update(time_t start, time_t end);
};

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

#include "utils/PVRChannel.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVREpgInfoTag;

class CPVREpg
{
  friend class CPVREpgs;

private:
  long m_channelID;
  const CPVRChannel *m_Channel;
  std::vector<CPVREpgInfoTag*> m_tags;
  bool m_bUpdateRunning;
  bool m_bValid;

public:
  CPVREpg(long ChannelID);
  CPVREpg(const CPVRChannel &channel);
  long ChannelID(void) const { return m_channelID; }
  bool IsValid(void) const;
  const CPVRChannel *ChannelTag(void) const { return m_Channel; }
  CPVREpgInfoTag *AddInfoTag(CPVREpgInfoTag *Tag);
  void DelInfoTag(CPVREpgInfoTag *tag);
  void Cleanup(const CDateTime Time);
  void Cleanup(void);
  void Sort(void);
  const std::vector<CPVREpgInfoTag*> *InfoTags(void) const { return &m_tags; }
  const CPVREpgInfoTag *GetInfoTagNow(void) const;
  const CPVREpgInfoTag *GetInfoTagNext(void) const;
  const CPVREpgInfoTag *GetInfoTag(long uniqueID, CDateTime StartTime) const;
  const CPVREpgInfoTag *GetInfoTagAround(CDateTime Time) const;
  CDateTime GetLastEPGDate();
  bool IsUpdateRunning() const { return m_bUpdateRunning; }
  void SetUpdate(bool OnOff) { m_bUpdateRunning = OnOff; }

  static bool Add(const PVR_PROGINFO *data, CPVREpg *Epg);
  static bool AddDB(const PVR_PROGINFO *data, CPVREpg *Epg);
};

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
#include "utils/Thread.h"
#include "utils/PVREpg.h"
#include "utils/PVREpgInfoTag.h"
#include "utils/PVRChannels.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRChannel;
class CTVDatabase;
struct PVREpgSearchFilter;

class CPVREpgs : public std::vector<CPVREpg*>
{
  friend class CPVREpg;

private:
  CCriticalSection m_critSection;
  bool  m_bInihibitUpdate;
  bool  m_bIgnoreDbForClient;
  int   m_iLingerTime;
  int   m_iDaysToDisplay;

  bool GetEPGForChannel(CTVDatabase *database, CPVRChannel *channel, time_t *start, time_t *end);
  const CPVREpg *AddEPG(long iChannelID);
  bool LoadSettings();

public:
  CPVREpgs(void);

  /**
   * Get an EPG table for a channel
   */
  const CPVREpg *GetEPG(long iChannelID, bool bAddIfMissing = false);

  /**
   * Get an EPG table for a channel
   */
  const CPVREpg *GetEPG(CPVRChannel *Channel, bool AddIfMissing = false);

  /**
   * Removes old data
   */
  bool Cleanup(void);

  /**
   * Clears all entries
   */
  bool ClearAll(void);

  /**
   * Clears all entries for a channel
   */
  bool ClearChannel(long ChannelID);

  /**
   * Loads and updates the EPG data
   */
  bool Update();

  void Unload();

  void InihibitUpdate(bool yesNo) { m_bInihibitUpdate = yesNo; }

  int GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter);
  int GetEPGAll(CFileItemList* results, bool radio = false);
  int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);
  int GetEPGNow(CFileItemList* results, bool radio = false);
  int GetEPGNext(CFileItemList* results, bool radio = false);
  CDateTime GetFirstEPGDate(bool radio = false);
  CDateTime GetLastEPGDate(bool radio = false);
  void SetVariableData(CFileItemList* results);
  void AssignChangedChannelTags(bool radio = false);
  bool RemoveOverlappingEvents(CTVDatabase *database, CPVREpg *epg);
  bool GrabEPGForChannel(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end);
  bool UpdateEPGForChannel(CTVDatabase *database, CPVRChannel *channel, time_t *start, time_t *end);
};

extern CPVREpgs PVREpgs;

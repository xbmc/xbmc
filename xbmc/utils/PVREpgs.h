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
  bool  m_bInihibitUpdate;        /* prevent the EPG from being updated */
  bool  m_bIgnoreDbForClient;     /* don't save the EPG data in the database */
  int   m_iLingerTime;            /* hours to keep old EPG data */
  int   m_iDaysToDisplay;         /* amount of EPG data to maintain */

  /**
   * Get the EPG for a channel
   */
  bool GetEPGForChannel(CTVDatabase *database, CPVRChannel *channel, time_t *start, time_t *end);

  /**
   * Create a new EPG table for a channel
   */
  const CPVREpg *CreateEPG(CPVRChannel *channel);

  /**
   * Load the EPG for a channel using the pvr client
   */
  bool GrabEPGForChannelFromClient(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end);

  /**
   * Load the EPG for a channel using a scraper
   */
  bool GrabEPGForChannelFromScraper(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end);

  /**
   * Load the EPG for a channel using the pvr client or scraper
   */
  bool GrabEPGForChannel(CPVRChannel *channel, CPVREpg *epg, time_t start, time_t end);

  /**
   * Load the EPG settings
   */
  bool LoadSettings();

public:
  CPVREpgs();
  ~CPVREpgs();

  /**
   * Get an EPG table for a channel
   */
  const CPVREpg *GetEPG(CPVRChannel *Channel, bool AddIfMissing = false);

  /**
   * Remove old EPG entries
   */
  bool RemoveOldEntries();

  /**
   * Clear all EPG entries
   */
  bool RemoveAllEntries(void);

  /**
   * Clear all EPG entries for a channel
   */
  bool ClearEPGForChannel(CPVRChannel *channel);

  /**
   * Loads and updates the EPG data
   */
  bool Update();

  /**
   * Unloads all EPG data
   */
  void Unload();

  /**
   * Prevent the EPG from being updated
   */
  void InihibitUpdate(bool yesNo) { m_bInihibitUpdate = yesNo; }

  /**
   * Update the EPG data for a single channel
   */
  bool UpdateEPGForChannel(CPVRChannel *channel, time_t *start, time_t *end);

  int GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter);
  int GetEPGAll(CFileItemList* results, bool radio = false);
  int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);
  int GetEPGNow(CFileItemList* results, bool radio = false);
  int GetEPGNext(CFileItemList* results, bool radio = false);
  CDateTime GetFirstEPGDate(bool radio = false);
  CDateTime GetLastEPGDate(bool radio = false);
  void SetVariableData(CFileItemList* results);
  void AssignChangedChannelTags(bool radio = false);
};

extern CPVREpgs PVREpgs;

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
#include "Thread.h"
#include "CriticalSection.h"
#include "../addons/include/xbmc_pvr_types.h"
#include "utils/Thread.h"

class CPVREpg;
class CPVREpgInfoTag;
class CPVRChannel;
class CTVDatabase;
class CFileItemList;
struct PVREpgSearchFilter;

class CPVREpgs : public std::vector<CPVREpg*>,
                 private CThread
{
  friend class CPVREpg;

private:
  CCriticalSection m_critSection;
  bool             m_bInihibitUpdate;    /* prevent the EPG from being updated */
  bool             m_bIgnoreDbForClient; /* don't save the EPG data in the database */
  int              m_iLingerTime;        /* hours to keep old EPG data */
  int              m_iDaysToDisplay;     /* amount of EPG data to maintain */
  int              m_iUpdateTime;        /* update the full EPG after this period */
  bool             m_bDatabaseLoaded;    /* true if we already loaded the EPG from the database */
  CDateTime        m_RadioFirst;         /* the earliest EPG date in our radio channel tables */
  CDateTime        m_RadioLast;          /* the latest EPG date in our radio channel tables */
  CDateTime        m_TVFirst;            /* the earliest EPG date in our tv channel tables */
  CDateTime        m_TVLast;             /* the latest EPG date in our tv channel tables */

  /**
   * Load the EPG for a channel using the pvr client
   */
  bool GrabEPGForChannelFromClient(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end);

  /**
   * Load the EPG for a channel using a scraper
   */
  bool GrabEPGForChannelFromScraper(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end);

  /**
   * Load the EPG for a channel using the pvr client or scraper
   */
  bool GrabEPGForChannel(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end);

  /**
   * Load the EPG settings
   */
  bool LoadSettings();

  /**
   * Remove old EPG entries
   */
  bool RemoveOldEntries();

  void UpdateFirstAndLastEPGDates(const CPVREpgInfoTag &tag);

protected:
  virtual void Process();

public:
  CPVREpgs();
  ~CPVREpgs();

  void Start();
  void Stop();

  void Clear();

  /**
   * Update the EPG pointers for all channels
   */
  void UpdateAllChannelEPGPointers();

  /**
   * Clear all EPG entries
   */
  bool RemoveAllEntries(bool bShowProgress = false);

  /**
   * Loads and updates the EPG data
   */
  bool UpdateEPG(bool bShowProgress = false);

  /**
   * Prevent the EPG from being updated
   */
  void InihibitUpdate(bool bSetTo) { m_bInihibitUpdate = bSetTo; }

  int GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter);
  int GetEPGAll(CFileItemList* results, bool bRadio = false);
  int GetEPGForChannel(CPVRChannel *channel, CFileItemList *results);
  int GetEPGNow(CFileItemList* results, bool bRadio = false);
  int GetEPGNext(CFileItemList* results, bool bRadio = false);
  CDateTime GetFirstEPGDate(bool bRadio = false);
  CDateTime GetLastEPGDate(bool bRadio = false);
  void UpdateTimers();
//  void SetVariableData(CFileItemList* results);
};

extern CPVREpgs PVREpgs;

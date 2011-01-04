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
#include "Observer.h"
#include "../addons/include/xbmc_pvr_types.h"
#include "utils/Thread.h"

class CPVREpg;
class CPVREpgInfoTag;
class CPVRChannel;
class CPVRDatabase;
class CFileItemList;
struct PVREpgSearchFilter;

class CPVREpgs : public std::vector<CPVREpg*>,
                 public Observer,
                 private CThread
{
  friend class CPVREpg;

private:
  /* config settings */
  bool             m_bIgnoreDbForClient;      /* don't save the EPG data in the database */
  int              m_iLingerTime;             /* hours to keep old EPG data */
  int              m_iDisplayTime;            /* hours of EPG data to fetch */
  int              m_iUpdateTime;             /* update the full EPG after this period */

  /* cached data */
  CDateTime        m_RadioFirst;              /* the earliest EPG date in our radio channel tables */
  CDateTime        m_RadioLast;               /* the latest EPG date in our radio channel tables */
  CDateTime        m_TVFirst;                 /* the earliest EPG date in our tv channel tables */
  CDateTime        m_TVLast;                  /* the latest EPG date in our tv channel tables */

  /* class state properties */
  bool             m_bDatabaseLoaded;         /* true if we already loaded the EPG from the database */
  time_t           m_iLastEpgUpdate;          /* the time the EPG was updated */
  time_t           m_iLastEpgCleanup;         /* the time the EPG was cleaned up */
  time_t           m_iLastTimerUpdate;        /* the time the timers were updated */
  time_t           m_iLastPointerUpdate;      /* the time the now playing pointers were updated */

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

  /**
   * Update the last and first EPG date cache
   */
  void UpdateFirstAndLastEPGDates(const CPVREpgInfoTag &tag);

  /**
   * Update the EPG pointers for all channels
   */
  bool UpdateAllChannelEPGPointers();

  /**
   * Loads and updates the EPG data
   */
  bool UpdateEPG(bool bShowProgress = false);

  /**
   * Update the timers
   */
  bool UpdateTimers(void);

  /**
   * Load all EPG entries from the database
   */
  bool LoadFromDb(bool bShowProgress = false);

  /**
   * Clear all EPG entries
   */
  void Clear(bool bClearDb = false);

  /**
   * Create an EPG table for each channel
   */
  bool CreateChannelEpgs(void);

protected:
  virtual void Process();

public:
  CPVREpgs();
  virtual ~CPVREpgs();

  void Start();
  bool Stop();
  bool Reset(bool bClearDb = false);
  void Notify(const Observable &obs, const CStdString& msg);

  int GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter);
  int GetEPGAll(CFileItemList* results, bool bRadio = false);
  int GetEPGNow(CFileItemList* results, bool bRadio = false);
  int GetEPGNext(CFileItemList* results, bool bRadio = false);
  CDateTime GetFirstEPGDate(bool bRadio = false);
  CDateTime GetLastEPGDate(bool bRadio = false);
};

extern CPVREpgs PVREpgs;

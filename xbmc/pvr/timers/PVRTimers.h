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
#include "addons/include/xbmc_pvr_types.h"

class CFileItem;
class CPVREpgInfoTag;
class CGUIDialogPVRTimerSettings;
class CPVRTimerInfoTag;

class CPVRTimers : public std::vector<CPVRTimerInfoTag *>
{
private:
  CCriticalSection m_critSection;

public:
  CPVRTimers(void);

  /**
   * Load the timers from the clients.
   * Returns the amount of timers that were added.
   */
  int Load();

  /**
   * Clear this timer list.
   */
  void Unload();

  /**
   * Refresh the channel list from the clients.
   * Returns the amount of timers that were added.
   */
  int Update();

  /**
   * Update a timer entry in this container.
   */
  bool Update(const CPVRTimerInfoTag &timer);

  /********** getters **********/

  /**
   * Get all known timers.
   */
  int GetTimers(CFileItemList* results);

  /**
   * The timer that will be active next.
   * Returns null if there is none.
   */
  CPVRTimerInfoTag *GetNextActiveTimer(void);

  /**
   * The amount of timers in this container.
   */
  int GetNumTimers();

  /**
   * Get the directory for a path.
   */
  bool GetDirectory(const CStdString& strPath, CFileItemList &items);

  /********** channel methods **********/

  /**
   * Check if there are any active timers on a channel.
   */
  bool ChannelHasTimers(const CPVRChannel &channel);

  /*!
   * @brief Delete all timers on a channel.
   * @param channel The channel to delete the timers for.
   * @param bDeleteRepeating True to delete repeating events too, false otherwise.
   * @param bCurrentlyActiveOnly True to delete timers that are currently running only.
   * @return True if timers any were deleted, false otherwise.
   */
  bool DeleteTimersOnChannel(const CPVRChannel &channel, bool bDeleteRepeating = true, bool bCurrentlyActiveOnly = false);

  /*!
   * @brief Create a new instant timer on a channel.
   * @param channel The channel to create the timer on.
   * @param bStartTimer True to start the timer instantly, false otherwise.
   * @return The new timer or NULL if it couldn't be created.
   */
  CPVRTimerInfoTag *InstantTimer(CPVRChannel *channel, bool bStartTimer = true);

  /********** static methods **********/

  /**
   * Add a timer to the client.
   * True if it was sent correctly, false if not.
   */
  static bool AddTimer(const CFileItem &item);

  /**
   * Add a timer to the client.
   * True if it was sent correctly, false if not.
   */
  static bool AddTimer(const CPVRTimerInfoTag &item);

  /**
   * Delete a timer on the client.
   * True if it was sent correctly, false if not.
   */
  static bool DeleteTimer(const CFileItem &item, bool bForce = false);

  /**
   * Delete a timer on the client.
   * True if it was sent correctly, false if not.
   */
  static bool DeleteTimer(const CPVRTimerInfoTag &item, bool bForce = false);

  /**
   * Rename a timer on the client.
   * True if it was sent correctly, false if not.
   */
  static bool RenameTimer(CFileItem &item, const CStdString &strNewName);

  /**
   * Rename a timer on the client.
   * True if it was sent correctly, false if not.
   */
  static bool RenameTimer(CPVRTimerInfoTag &item, const CStdString &strNewName);

  /**
   * Get updated timer information from the client.
   * True if it was requested correctly, false if not.
   */
  static bool UpdateTimer(const CFileItem &item);

  /**
   * Get updated timer information from the client.
   * True if it was requested correctly, false if not.
   */
  static bool UpdateTimer(const CPVRTimerInfoTag &item);

  CPVRTimerInfoTag *GetMatch(const CEpgInfoTag *Epg);
  CPVRTimerInfoTag *GetMatch(const CFileItem *item);
};

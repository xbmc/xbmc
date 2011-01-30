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

#include "PVRChannelGroups.h"

class CPVRManager;

class CPVRChannelGroupsContainer
{
  friend class CPVRManager;

private:
  CPVRChannelGroups *m_groupsRadio; /*!< all radio channel groups */
  CPVRChannelGroups *m_groupsTV;    /*!< all TV channel groups */

protected:
  /*!
   * @brief Update the contents of all the groups in this container.
   * @return True if the update was successful, false otherwise.
   */
  bool Update(void);

public:
  /*!
   * @brief Create a new container for all channel groups
   */
  CPVRChannelGroupsContainer(void);

  /*!
   * @brief Destroy this container.
   */
  ~CPVRChannelGroupsContainer(void);

  /*!
   * @brief Load all channel groups and all channels in those channel groups.
   * @return True if all groups were loaded, false otherwise.
   */
  bool Load(void);

  /*!
   * @brief Unload and destruct all channel groups and all channels in them.
   */
  void Unload(void);

  /*!
   * @brief Get the TV channel groups.
   * @return The TV channel groups.
   */
  const CPVRChannelGroups *GetTV(void) const { return Get(false); }

  /*!
   * @brief Get the radio channel groups.
   * @return The radio channel groups.
   */
  const CPVRChannelGroups *GetRadio(void) const { return Get(true); }

  /*!
   * @brief Get the radio or TV channel groups.
   * @param bRadio If true, get the radio channel groups. Get the TV channel groups otherwise.
   * @return The requested groups.
   */
  const CPVRChannelGroups *Get(bool bRadio) const;

  /*!
   * @brief Get the group containing all TV channels.
   * @return The group containing all TV channels.
   */
  const CPVRChannelGroup *GetGroupAllTV(void)  const{ return GetGroupAll(false); }

  /*!
   * @brief Get the group containing all radio channels.
   * @return The group containing all radio channels.
   */
  const CPVRChannelGroup *GetGroupAllRadio(void)  const{ return GetGroupAll(true); }

  /*!
   * @brief Get the group containing all TV or radio channels.
   * @param bRadio If true, get the group containing all radio channels. Get the group containing all TV channels otherwise.
   * @return The requested group.
   */
  const CPVRChannelGroup *GetGroupAll(bool bRadio) const;
};

extern CPVRChannelGroupsContainer g_PVRChannelGroups; /*!< The channel groups container, containing all TV and radio channel groups. */

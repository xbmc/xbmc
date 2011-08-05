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
#include "epg/EpgInfoTag.h"
#include "addons/include/xbmc_pvr_types.h"

namespace PVR
{
  class CPVREpg;
  class CPVRTimerInfoTag;

  /** an EPG info tag */

  class CPVREpgInfoTag : public EPG::CEpgInfoTag
  {
    friend class CPVREpg;

  public:
    /*!
     * @brief Create a new empty EPG infotag.
     */
    CPVREpgInfoTag(void);

    /*!
     * @brief Create a new EPG infotag with 'data' as content.
     * @param data The tag's content.
     */
    CPVREpgInfoTag(const EPG_TAG &data);

    /*!
     * @brief Get the channel that plays this event.
     * @return a pointer to the channel.
     */
    const CPVRChannel *ChannelTag(void) const;

    /*!
     * @brief Update the value of m_strFileNameAndPath after a value changed.
     */
    void UpdatePath(void);

    /*!
     * @brief Update the information in this tag with the info in the given tag.
     * @param tag The new info.
     */
    void Update(const EPG_TAG &tag);

    const CStdString &Icon(void) const;
  };
}

#pragma once
/*
 *      Copyright (C) 2012-2016 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/interfaces/AddonInterfaces.h"
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxUtils.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_inputstream.h"

namespace KodiAPI
{
namespace InputStream
{

class CAddonCallbacksInputStream : public ADDON::IAddonInterface
{
public:
  CAddonCallbacksInputStream(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksInputStream();

  /*!
   * @return The callback table.
   */
  CB_INPUTSTREAMLib *GetCallbacks() { return m_callbacks; }

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param addonData A pointer to the add-on.
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet.
   */
  static DemuxPacket* InputStreamAllocateDemuxPacket(void* addonData, int iDataSize = 0);

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param addonData A pointer to the add-on.
   * @param pPacket The packet to free.
   */
  static void InputStreamFreeDemuxPacket(void* addonData, DemuxPacket* pPacket);

private:
  CB_INPUTSTREAMLib* m_callbacks; /*!< callback addresses */
};

} /* namespace InputStream */
} /* namespace KodiAPI */

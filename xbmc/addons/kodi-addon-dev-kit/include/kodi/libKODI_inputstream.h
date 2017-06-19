/*
 *      Copyright (C) 2005-2016 Team XBMC
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

#pragma once

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "kodi_inputstream_types.h"
#include "versions.h"

#ifdef BUILD_KODI_ADDON
#include "DVDDemuxPacket.h"
#else
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#endif

class CHelper_libKODI_inputstream
{
public:
  CHelper_libKODI_inputstream(void)
  {
    m_Handle = nullptr;
    m_Callbacks = nullptr;
  }

  ~CHelper_libKODI_inputstream(void)
  {
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = static_cast<AddonCB*>(handle);
    if (m_Handle)
      m_Callbacks = (AddonInstance_InputStream*)m_Handle->INPUTSTREAMLib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libKODI_inputstream-ERROR: InputStream_RegisterMe can't get callback table from Kodi !!!\n");

    return m_Callbacks != nullptr;
  }

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet
   */
  DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return m_Callbacks->toKodi.AllocateDemuxPacket(m_Callbacks->toKodi.kodiInstance, iDataSize);
  }

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param pPacket The packet to free
   */
  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return m_Callbacks->toKodi.FreeDemuxPacket(m_Callbacks->toKodi.kodiInstance, pPacket);
  }

private:
  AddonCB* m_Handle;
  AddonInstance_InputStream* m_Callbacks;
};

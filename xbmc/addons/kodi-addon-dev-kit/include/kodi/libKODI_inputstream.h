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
#include "libXBMC_addon.h"

#ifdef BUILD_KODI_ADDON
#include "DVDDemuxPacket.h"
#else
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#endif

/* current input stream API version */
#define KODI_INPUTSTREAM_API_VERSION "1.0.0"

namespace KodiAPI
{
namespace InputStream
{

typedef struct CB_INPUTSTREAMLib
{
  void (*FreeDemuxPacket)(void *addonData, DemuxPacket* pPacket);
  DemuxPacket* (*AllocateDemuxPacket)(void *addonData, int iDataSize);
} CB_INPUTSTREAMLib;

} /* namespace InputStream */
} /* namespace KodiAPI */

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
    if (m_Handle && m_Callbacks)
    {
      m_Handle->INPUTSTREAMLib_UnRegisterMe(m_Handle->addonData, m_Callbacks);
    }
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
      m_Callbacks = (KodiAPI::InputStream::CB_INPUTSTREAMLib*)m_Handle->INPUTSTREAMLib_RegisterMe(m_Handle->addonData);
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
    return m_Callbacks->AllocateDemuxPacket(m_Handle->addonData, iDataSize);
  }

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param pPacket The packet to free
   */
  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return m_Callbacks->FreeDemuxPacket(m_Handle->addonData, pPacket);
  }

private:
  AddonCB* m_Handle;
  KodiAPI::InputStream::CB_INPUTSTREAMLib* m_Callbacks;
};

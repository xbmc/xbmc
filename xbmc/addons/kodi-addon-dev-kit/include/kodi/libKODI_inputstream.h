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

#define INPUTSTREAM_HELPER_DLL KODI_DLL("inputstream")
#define INPUTSTREAM_HELPER_DLL_NAME KODI_DLL_NAME("inputstream")

/* current input stream API version */
#define KODI_INPUTSTREAM_API_VERSION "1.0.0"

class CHelper_libKODI_inputstream
{
public:
  CHelper_libKODI_inputstream(void)
  {
    m_libKODI_inputstream = nullptr;
    m_Handle = nullptr;
  }

  ~CHelper_libKODI_inputstream(void)
  {
    if (m_libKODI_inputstream)
    {
      INPUTSTREAM_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libKODI_inputstream);
    }
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += INPUTSTREAM_HELPER_DLL;

    m_libKODI_inputstream = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libKODI_inputstream == nullptr)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    INPUTSTREAM_register_me = (void* (*)(void *HANDLE))
      dlsym(m_libKODI_inputstream, "INPUTSTREAM_register_me");
    if (INPUTSTREAM_register_me == nullptr)
    {
      fprintf(stderr, "Unable to assign function %s\n", dlerror());
      return false;
    }

    INPUTSTREAM_unregister_me = (void (*)(void* HANDLE, void* CB))
      dlsym(m_libKODI_inputstream, "INPUTSTREAM_unregister_me");
    if (INPUTSTREAM_unregister_me == nullptr)
    {
      fprintf(stderr, "Unable to assign function %s\n", dlerror());
      return false;
    }

    INPUTSTREAM_free_demux_packet = (void (*)(void* HANDLE, void* CB, DemuxPacket* pPacket))
      dlsym(m_libKODI_inputstream, "INPUTSTREAM_free_demux_packet");
    if (INPUTSTREAM_free_demux_packet == NULL)
    {
      fprintf(stderr, "Unable to assign function %s\n", dlerror());
      return false;
    }

    INPUTSTREAM_allocate_demux_packet = (DemuxPacket* (*)(void* HANDLE, void* CB, int iDataSize))
      dlsym(m_libKODI_inputstream, "INPUTSTREAM_allocate_demux_packet");
    if (INPUTSTREAM_allocate_demux_packet == NULL)
    {
      fprintf(stderr, "Unable to assign function %s\n", dlerror());
      return false;
    }

    m_Callbacks = INPUTSTREAM_register_me(m_Handle);
    return m_Callbacks != nullptr;
  }

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet
   */
  DemuxPacket* AllocateDemuxPacket(int iDataSize)
  {
    return INPUTSTREAM_allocate_demux_packet(m_Handle, m_Callbacks, iDataSize);
  }

  /*!
   * @brief Free a packet that was allocated with AllocateDemuxPacket
   * @param pPacket The packet to free
   */
  void FreeDemuxPacket(DemuxPacket* pPacket)
  {
    return INPUTSTREAM_free_demux_packet(m_Handle, m_Callbacks, pPacket);
  }

protected:
  void* (*INPUTSTREAM_register_me)(void*);
  void (*INPUTSTREAM_unregister_me)(void*, void*);
  void (*INPUTSTREAM_free_demux_packet)(void*, void*, DemuxPacket*);
  DemuxPacket* (*INPUTSTREAM_allocate_demux_packet)(void*, void*, int);

private:
  void* m_libKODI_inputstream;
  void* m_Handle;
  void* m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};

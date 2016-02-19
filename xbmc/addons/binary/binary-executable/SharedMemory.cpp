/*
 *      Copyright (C) 2010-2015 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "BinaryAddon.h"
#include "SharedMemory.h"
#include "Application.h"
#include "tools.h"

#include <netinet/in.h>
#include <cstdlib>

namespace ADDON
{

CBinaryAddonSharedMemory::CBinaryAddonSharedMemory(int randNumber, CBinaryAddon* addon, size_t size /* = DEFAULT_SHARED_MEM_SIZE*/)
 : CThread("Addon to Kodi shared mem handler"), // Communication Thread Addon to Kodi
   m_randomConnectionNumber(randNumber),        // Connection number used here and on add-on to identify shared memory
   m_sharedMemSize(size),                       // The memory size used for shared place of Kodi and add-on
   m_sharedMem_AddonToKodi(nullptr),            // Shared memory pointer used for Communication from Add-on to Kodi
   m_sharedMem_KodiToAddon(nullptr),            // Shared memory pointer used for Communication from Kodi to Add-on
   m_addon(addon),                              // The binary addon handler from connection itself
   m_LoggedIn(false)                            // LockedIn value, used to prevent not wanted locks on operaion thread
{
}

CBinaryAddonSharedMemory::~CBinaryAddonSharedMemory()
{
}

/*
 * Intercommunication thread to handler function calls from add-on
 * to Kodi over shared memory.
 */
void CBinaryAddonSharedMemory::Process(void)
{
  while (!m_bStop && !g_application.m_bStop && m_LoggedIn)
  {
    if (Lock_AddonToKodi_Kodi())
    {
      if (m_bStop || g_application.m_bStop || !m_LoggedIn)
        break;

      CRequestPacket req(ntohl(m_sharedMem_AddonToKodi->message.in.m_serialNumber),
                         ntohl(m_sharedMem_AddonToKodi->message.in.m_opcode),
                         (uint8_t*)&m_sharedMem_AddonToKodi->message.in.data,
                         ntohl(m_sharedMem_AddonToKodi->message.in.m_dataLength),
                         true);
      CResponsePacket resp((uint8_t*)&m_sharedMem_AddonToKodi->message.out, m_sharedMemSize);
      m_addon->processRequest(req, resp);
    }
    Unlock_AddonToKodi_Addon();
  }
}

}; /* namespace ADDON */

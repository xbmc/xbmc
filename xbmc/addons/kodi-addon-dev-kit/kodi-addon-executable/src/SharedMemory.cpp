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

#include "SharedMemory.h"
#include <string>

CBinaryAddonSharedMemory::CBinaryAddonSharedMemory(size_t size /* = DEFAULT_SHARED_MEM_SIZE*/)
 : m_randomConnectionNumber(std::rand()),
   m_sharedMem_Size(size),
   m_sharedMem_AddonToKodi(NULL),
   m_sharedMem_KodiToAddon(NULL)
{

}

CBinaryAddonSharedMemory::~CBinaryAddonSharedMemory()
{
}

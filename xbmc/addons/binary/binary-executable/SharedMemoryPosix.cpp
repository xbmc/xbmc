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

#if (defined TARGET_POSIX)

#include "SharedMemoryPosix.h"
#include "utils/log.h"
#include "tools.h"

#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>

namespace ADDON
{

CBinaryAddonSharedMemoryPosix::CBinaryAddonSharedMemoryPosix(int randNumber, CBinaryAddon* addon, size_t size /* = DEFAULT_SHARED_MEM_SIZE*/)
 : CBinaryAddonSharedMemory(randNumber, addon, size),
   m_shmId_FromAddonToKodi(-1),
   m_shmId_FromKodiToAddon(-1)
{

}

CBinaryAddonSharedMemoryPosix::~CBinaryAddonSharedMemoryPosix()
{
  DestroySharedMemory();
}

bool CBinaryAddonSharedMemoryPosix::CreateSharedMemory()
{
  /*
   * Interface from Addon to Kodi
   */
  if ((m_shmId_FromAddonToKodi = shmget(m_randomConnectionNumber, m_sharedMemSize, IPC_CREAT | 0666)) < 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Request of interface 'Add-on to Kodi' shared memory failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  if ((m_sharedMem_AddonToKodi = (KodiAPI_ShareData*)shmat(m_shmId_FromAddonToKodi, NULL, 0)) == (KodiAPI_ShareData *) -1)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Connect of interface 'Add-on to Kodi' shared memory failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  if (sem_init(&m_sharedMem_AddonToKodi->shmSegmentToKodi, 1, 0) != 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of interface 'Add-on to Kodi' Lock to Kodi failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  if (sem_init(&m_sharedMem_AddonToKodi->shmSegmentToAddon, 1, 0) != 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of interface 'Add-on to Kodi' Lock to Add-on failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  /*
   * Interface from Kodi to Addon
   */
  if ((m_shmId_FromKodiToAddon = shmget(m_randomConnectionNumber+1, m_sharedMemSize, IPC_CREAT | 0666)) < 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Request of interface 'Kodi to Add-on' shared memory failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  if ((m_sharedMem_KodiToAddon = (KodiAPI_ShareData*)shmat(m_shmId_FromKodiToAddon, NULL, 0)) == (KodiAPI_ShareData*) -1)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Connect of interface 'Kodi to Add-on' shared memory failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  if (sem_init(&m_sharedMem_KodiToAddon->shmSegmentToKodi, 1, 0) != 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of interface 'Kodi to Add-on' Lock to Kodi failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  if (sem_init(&m_sharedMem_KodiToAddon->shmSegmentToAddon, 1, 0) != 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Creation of interface 'Kodi to Add-on' Lock to Add-on failed with error '%i': %s",
                errno, SystemErrorString(errno).c_str());
    return false;
  }

  m_LoggedIn = true;
  Create();

  return true;
}

void CBinaryAddonSharedMemoryPosix::DestroySharedMemory()
{
  m_LoggedIn = false;
  if (m_sharedMem_AddonToKodi)
  {
    sem_post(&m_sharedMem_AddonToKodi->shmSegmentToKodi);
    sem_post(&m_sharedMem_AddonToKodi->shmSegmentToAddon);
  }
  if (m_sharedMem_KodiToAddon)
  {
    sem_post(&m_sharedMem_KodiToAddon->shmSegmentToKodi);
    sem_post(&m_sharedMem_KodiToAddon->shmSegmentToAddon);
  }

  StopThread();

  /* freeing the reference to the semaphore */
  if (m_sharedMem_AddonToKodi)
  {
    usleep(1);
    sem_close(&m_sharedMem_AddonToKodi->shmSegmentToKodi);
    sem_close(&m_sharedMem_AddonToKodi->shmSegmentToAddon);

    if (shmdt(m_sharedMem_AddonToKodi) != 0)
      CLog::Log(LOGERROR, "Binary AddOn: Detach of shared memory from Add-on failed with error '%i': %s",errno, SystemErrorString(errno).c_str());

    m_sharedMem_AddonToKodi = nullptr;
  }

  if (m_sharedMem_KodiToAddon)
  {
    usleep(1);
    sem_close(&m_sharedMem_KodiToAddon->shmSegmentToKodi);
    sem_close(&m_sharedMem_KodiToAddon->shmSegmentToAddon);

    if (shmdt(m_sharedMem_KodiToAddon) != 0)
      CLog::Log(LOGERROR, "Binary AddOn: Detach of shared memory from Kodi failed with error '%i': %s",errno, SystemErrorString(errno).c_str());

    m_sharedMem_KodiToAddon = nullptr;
  }
}

bool CBinaryAddonSharedMemoryPosix::Lock_AddonToKodi_Kodi()
{
  int32_t err = sem_wait(&m_sharedMem_AddonToKodi->shmSegmentToKodi);
  if (err != 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: LockAddon of interface 'Add-on to Kodi' failed with error '%i': %s", errno, SystemErrorString(errno).c_str());
    return false;
  }

  return true;
}

void CBinaryAddonSharedMemoryPosix::Unlock_AddonToKodi_Kodi()
{
  sem_post(&m_sharedMem_AddonToKodi->shmSegmentToAddon);
}

void CBinaryAddonSharedMemoryPosix::Unlock_AddonToKodi_Addon()
{
  sem_post(&m_sharedMem_AddonToKodi->shmSegmentToAddon);
}

bool CBinaryAddonSharedMemoryPosix::Lock_KodiToAddon_Kodi()
{
  int32_t err = sem_wait(&m_sharedMem_KodiToAddon->shmSegmentToKodi);
  if (err != 0)
  {
    CLog::Log(LOGERROR, "Binary AddOn: Lock Kodi of interface 'Kodi to Add-on' failed with error '%i': %s", errno, SystemErrorString(errno).c_str());
    return false;
  }

  return true;
}

void CBinaryAddonSharedMemoryPosix::Unlock_KodiToAddon_Kodi()
{
  sem_post(&m_sharedMem_KodiToAddon->shmSegmentToKodi);
}

void CBinaryAddonSharedMemoryPosix::Unlock_KodiToAddon_Addon()
{
  sem_post(&m_sharedMem_KodiToAddon->shmSegmentToAddon);
}

}; /* namespace ADDON */

#endif /* (defined TARGET_POSIX) */

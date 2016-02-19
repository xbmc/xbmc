/*
 *      Copyright (C) 2015 Team KODI
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

#include "BinaryAddonStatus.h"
#include "BinaryAddon.h"

#include "Application.h"
#include "utils/log.h"

#if (defined TARGET_WINDOWS)
#include <windows.h>
#elif (defined TARGET_POSIX)
#include <signal.h>
#endif

namespace ADDON
{
  
const KODI_API_ErrorTranslator CBinaryAddonStatus::errorTranslator[API_ERR_LASTCODE] =
{
  { API_SUCCESS,          "Successful return code" },
  { API_ERR_BUFFER,       "Invalid buffer pointer" },
  { API_ERR_COUNT,        "Invalid count argument" },
  { API_ERR_TYPE,         "Invalid datatype argument" },
  { API_ERR_TAG,          "Invalid tag argument" },
  { API_ERR_COMM,         "Invalid communicator" },
  { API_ERR_RANK,         "Invalid rank" },
  { API_ERR_ROOT,         "Invalid root" },
  { API_ERR_GROUP,        "Null group passed to function" },
  { API_ERR_OP,           "Invalid operation" },
  { API_ERR_TOPOLOGY,     "Invalid topology" },
  { API_ERR_DIMS,         "Illegal dimension argument" },
  { API_ERR_ARG,          "Invalid argument" },
  { API_ERR_UNKNOWN,      "Unknown error" },
  { API_ERR_TRUNCATE,     "message truncated on receive" },
  { API_ERR_OTHER,        "Other error; use Error_string" },
  { API_ERR_INTERN,       "internal error code" },
  { API_ERR_IN_STATUS,    "Look in status for error value" },
  { API_ERR_PENDING,      "Pending request" },
  { API_ERR_REQUEST,      "illegal API_request handle" },
};
  
CBinaryAddonStatus::CBinaryAddonStatus()
 : CThread("Binary add-on status")
{
}

CBinaryAddonStatus::~CBinaryAddonStatus()
{
  Shutdown();
}

void CBinaryAddonStatus::Shutdown()
{
  StopThread();
  CSingleLock lock(m_mutex);
  for (AddonList::iterator itr = m_addons.begin(); itr != m_addons.end(); ++itr)
  {
    delete (*itr);
  }
  m_addons.clear();
}

bool CBinaryAddonStatus::KillOfProcess(uint64_t processId)
{
#if (defined TARGET_WINDOWS)
  HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
  if (hProcess == NULL)
      return FALSE;

  return TerminateProcess(hProcess, 1);
#elif (defined TARGET_POSIX)
  return (kill(processId, 9) == 0) ? true : false;
#endif
}

bool CBinaryAddonStatus::CheckPresenceOfProcess(uint64_t processId)
{
#if (defined TARGET_WINDOWS)
  HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, processId);
  DWORD ret = WaitForSingleObject(hProcess, 0);
  CloseHandle(hProcess);
  return ret == WAIT_TIMEOUT;
#elif (defined TARGET_POSIX)
  return (kill(processId, 0) == 0) ? true : false;
#endif
}

void CBinaryAddonStatus::AddAddon(CBinaryAddon* addon)
{
  CSingleLock lock(m_mutex);
  m_addons.push_back(addon);
}

void CBinaryAddonStatus::Process(void)
{
  int killCountdown = -1;

  while (!m_bStop && !g_application.m_bStop)
  {
    {
      m_mutex.lock();

      // remove disconnected clients
      for (AddonList::iterator itr = m_addons.begin(); itr != m_addons.end();)
      {
        bool forcedDelete = false;

        if (killCountdown < 0)
        {
          if (!(*itr)->IsActive() &&
              !(*itr)->IsIndependent() &&
              CheckPresenceOfProcess((*itr)->GetAddonPID()))
          {
            killCountdown = PROCESS_KILL_COUNTDOWN;
            m_mutex.unlock();
            continue;
          }
          else if ((*itr)->IsActive() && !CheckPresenceOfProcess((*itr)->GetAddonPID()))
          {
            forcedDelete = true;
            if (!(*itr)->IsSubThread())
            {
              CLog::Log(LOGINFO, "Binary AddOn: Enabled addon '%s' with ID %u marked as present but not running anymore, forcing delete",
                        (*itr)->GetAddonName().c_str(), 
                        (*itr)->GetID());
            }
          }
        }
        else if (killCountdown == 0)
        {
          KillOfProcess((*itr)->GetAddonPID());
          Sleep(500);
          CLog::Log(LOGINFO, "Binary AddOn: Disabled addon '%s' (Pid: %li) with ID %u seems to be still running, forced close %s", 
                    (*itr)->GetAddonName().c_str(),
                    (*itr)->GetAddonPID(),
                    (*itr)->GetID(), 
                    CheckPresenceOfProcess((*itr)->GetAddonPID()) ? "failed" : "successful done");
          killCountdown = -1;
        }
        else
        {
          --killCountdown;
          m_mutex.unlock();
          Sleep(1000);
          continue;
        }

        if (forcedDelete || !(*itr)->IsActive())
        {
          if (!(*itr)->IsSubThread())
          {
            CLog::Log(LOGDEBUG, "Binary AddOn: Addon '%s' with ID %u seems to be disconnected, removing from list",
                      (*itr)->GetAddonName().c_str(),
                      (*itr)->GetID());
          }
          delete (*itr);
          itr = m_addons.erase(itr);
          killCountdown = -1;
        }
        else 
          ++itr;
      }
      m_mutex.unlock();
    }

    Sleep(250);
  }
}

}; /* namespace ADDON */

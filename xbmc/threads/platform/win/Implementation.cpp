/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "threads/Condition.h"

namespace XbmcThreads
{

  namespace intern
  {
    ConditionVariableVista::TakesCV ConditionVariableVista::InitializeConditionVariableProc;
    ConditionVariableVista::SleepCVCS ConditionVariableVista::SleepConditionVariableCSProc;
    ConditionVariableVista::TakesCV ConditionVariableVista::WakeConditionVariableProc;
    ConditionVariableVista::TakesCV ConditionVariableVista::WakeAllConditionVariableProc;

    bool ConditionVariableVista::setConditionVarFuncs()
    {
      HMODULE mod = GetModuleHandle("Kernel32");

      if (mod == NULL)
        return false;

      InitializeConditionVariableProc = (TakesCV)GetProcAddress(mod,"InitializeConditionVariable");
      if (InitializeConditionVariableProc == NULL)
        return false;
    
      SleepConditionVariableCSProc = (SleepCVCS)GetProcAddress(mod,"SleepConditionVariableCS");
      WakeAllConditionVariableProc = (TakesCV)GetProcAddress(mod,"WakeAllConditionVariable");
      WakeConditionVariableProc = (TakesCV)GetProcAddress(mod,"WakeConditionVariable");

      return SleepConditionVariableCSProc != NULL && 
        WakeAllConditionVariableProc != NULL &&
        WakeConditionVariableProc != NULL;
    }
  }

  bool ConditionVariable::getIsVista()
  {
    if (!isIsVistaSet)
    {
      isVista = intern::ConditionVariableVista::setConditionVarFuncs();
      isIsVistaSet = true;
    }

    return isVista;
  }

  bool ConditionVariable::isVista = getIsVista();
  // bss segment nulled out by loader.
  bool ConditionVariable::isIsVistaSet;
}

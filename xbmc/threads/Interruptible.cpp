/*
 *      Copyright (C) 2005-2011 Team XBMC
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


#include <vector>

#include "threads/Interruptible.h"
#include "threads/ThreadLocal.h"
#include "threads/SingleLock.h"

namespace XbmcThreads
{
  static CCriticalSection staticMutexForLockingTheListOfIInterruptiblesForCallingInterrupt;
  static ThreadLocal<std::vector<IInterruptible*> > threadSpecificInterruptibles;
  static std::vector<IInterruptible*> allInterruptibles;

  static void callInterrupt(std::vector<IInterruptible*> * interruptibles)
  {
    if (interruptibles != NULL)
    {
      // copy the list in case the Interrupt call modifies it.
      std::vector<IInterruptible*> list(interruptibles->size());
      std::vector<IInterruptible*>::iterator iter;

      {
        CSingleLock lock(staticMutexForLockingTheListOfIInterruptiblesForCallingInterrupt);
        list = *interruptibles;
      }

      for (iter=list.begin(); iter != list.end(); iter++)
        (*iter)->Interrupt();
    }
  }

  void IInterruptible::InterruptAll()
  {
    callInterrupt(&allInterruptibles);
  }

  void IInterruptible::InterruptThreadSpecific()
  {
    callInterrupt(threadSpecificInterruptibles.get());
  }

  void IInterruptible::enteringWaitState()
  {
    std::vector<IInterruptible*> * cur = threadSpecificInterruptibles.get();
    if (cur == NULL)
    {
      cur = new std::vector<IInterruptible*>();
      threadSpecificInterruptibles.set(cur);
    }
    cur->push_back(this);

    CSingleLock lock(staticMutexForLockingTheListOfIInterruptiblesForCallingInterrupt);
    allInterruptibles.push_back(this);
  }

  void static removeIt(std::vector<IInterruptible*> * list, IInterruptible* val)
  {
    std::vector<IInterruptible*>::iterator iter;
    for (iter = list->begin(); iter != list->end(); iter++)
    {
      if ((*iter) == val)
      {
        list->erase(iter);
        break;
      }
    }
  }

  void IInterruptible::leavingWaitState()
  {
    std::vector<IInterruptible*> * cur = threadSpecificInterruptibles.get();
    if (cur != NULL)
      removeIt(cur,this);
    CSingleLock lock(staticMutexForLockingTheListOfIInterruptiblesForCallingInterrupt);
    removeIt(&allInterruptibles,this);
  }

}

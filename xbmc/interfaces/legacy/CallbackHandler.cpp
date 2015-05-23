/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "CallbackHandler.h"
#include "AddonUtils.h"
#include "threads/SingleLock.h"
#include <vector>
#include "commons/Exception.h"
#include "utils/log.h"

namespace XBMCAddon
{
  class AsynchCallbackMessage : public AddonClass
  {
  public:
    AddonClass::Ref<Callback> cb;
    AddonClass::Ref<RetardedAsynchCallbackHandler> handler;
    AsynchCallbackMessage(Callback* _cb, RetardedAsynchCallbackHandler* _handler) :
      cb(_cb), handler(_handler) { XBMC_TRACE; }
  };

  //********************************************************************
  // This holds the callback messages which will be executed. It doesn't
  //  seem to work correctly with the Ref object so we'll go with Ref*'s
  typedef std::vector<AddonClass::Ref<AsynchCallbackMessage> > CallbackQueue;
  //********************************************************************

  static CCriticalSection critSection;
  static CallbackQueue g_callQueue;

  void RetardedAsynchCallbackHandler::invokeCallback(Callback* cb)
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);
    g_callQueue.push_back(new AsynchCallbackMessage(cb,this));
  }

  RetardedAsynchCallbackHandler::~RetardedAsynchCallbackHandler()
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);

    // find any messages that might be there because of me ... and remove them
    CallbackQueue::iterator iter = g_callQueue.begin();
    while (iter != g_callQueue.end())
    {
      AddonClass::Ref<AsynchCallbackMessage> cur(*iter);
      {
        if (cur->handler.get() == this) // then this message is because of me
        {
          g_callQueue.erase(iter);
          iter = g_callQueue.begin();
        }
        else
          ++iter;
      }
    }
  }

  void RetardedAsynchCallbackHandler::makePendingCalls()
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);
    CallbackQueue::iterator iter = g_callQueue.begin();
    while (iter != g_callQueue.end())
    {
      AddonClass::Ref<AsynchCallbackMessage> p(*iter);

      // only call when we are in the right thread state
      if(p->handler->isStateOk(p->cb->getObject()))
      {
        // remove it from the queue. No matter what we're done with
        //  this. Even if it doesn't execute for some reason.
        g_callQueue.erase(iter);

        // we need to release the critSection lock prior to grabbing the
        //  lock on the object. Not doing so results in deadlocks. We no
        //  longer are accessing the g_callQueue so it's fine to do this now
        {
          XBMCAddonUtils::InvertSingleLockGuard unlock(lock);

          // make sure the object is not deallocating

          // we need to grab the object lock to see if the object of the call 
          //  is deallocating. holding this lock should prevent it from 
          //  deallocating durring the execution of this call.
#ifdef ENABLE_XBMC_TRACE_API
          CLog::Log(LOGDEBUG,"%sNEWADDON executing callback 0x%lx",_tg.getSpaces(),(long)(p->cb.get()));
#endif
          AddonClass* obj = (p->cb->getObject());
          AddonClass::Ref<AddonClass> ref(obj);
          CSingleLock lock2(*obj);
          if (!p->cb->getObject()->isDeallocating())
          {
            try
            {
              // need to make the call
              p->cb->executeCallback();
            }
            catch (XbmcCommons::Exception& e) { e.LogThrowMessage(); }
            catch (...)
            {
              CLog::Log(LOGERROR,"Unknown exception while executeing callback 0x%lx", (long)(p->cb.get()));
            }
          }
        }

        // since the state of the iterator may have been corrupted by
        //  the changing state of the list from another thread durring
        //  the releasing fo the lock in the immediately preceeding 
        //  codeblock, we need to reset it before continuing the loop
        iter = g_callQueue.begin();
      }
      else // if we're not in the right thread for this callback...
        ++iter;
    }  
  }

  void RetardedAsynchCallbackHandler::clearPendingCalls(void* userData)
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);
    CallbackQueue::iterator iter = g_callQueue.begin();
    while (iter != g_callQueue.end())
    {
      AddonClass::Ref<AsynchCallbackMessage> p(*iter);

      if(p->handler->shouldRemoveCallback(p->cb->getObject(),userData))
      {
#ifdef ENABLE_XBMC_TRACE_API
        CLog::Log(LOGDEBUG,"%sNEWADDON removing callback 0x%lx for PyThreadState 0x%lx from queue", _tg.getSpaces(),(long)(p->cb.get()) ,(long)userData);
#endif
        iter = g_callQueue.erase(iter);
      }
      else
        ++iter;
    }
  }
}


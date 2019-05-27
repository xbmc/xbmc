/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CallbackHandler.h"
#include "AddonUtils.h"
#include "threads/SingleLock.h"
#include <vector>
#include "commons/Exception.h"
#include "utils/log.h"

namespace XBMCAddon
{
  class AsyncCallbackMessage : public AddonClass
  {
  public:
    AddonClass::Ref<Callback> cb;
    AddonClass::Ref<RetardedAsyncCallbackHandler> handler;
    AsyncCallbackMessage(Callback* _cb, RetardedAsyncCallbackHandler* _handler) :
      cb(_cb), handler(_handler) { XBMC_TRACE; }
  };

  //********************************************************************
  // This holds the callback messages which will be executed. It doesn't
  //  seem to work correctly with the Ref object so we'll go with Ref*'s
  typedef std::vector<AddonClass::Ref<AsyncCallbackMessage> > CallbackQueue;
  //********************************************************************

  static CCriticalSection critSection;
  static CallbackQueue g_callQueue;

  void RetardedAsyncCallbackHandler::invokeCallback(Callback* cb)
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);
    g_callQueue.push_back(new AsyncCallbackMessage(cb,this));
  }

  RetardedAsyncCallbackHandler::~RetardedAsyncCallbackHandler()
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);

    // find any messages that might be there because of me ... and remove them
    CallbackQueue::iterator iter = g_callQueue.begin();
    while (iter != g_callQueue.end())
    {
      if ((*iter)->handler.get() == this) // then this message is because of me
      {
        g_callQueue.erase(iter);
        iter = g_callQueue.begin();
      }
      else
        ++iter;
    }
  }

  void RetardedAsyncCallbackHandler::makePendingCalls()
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);
    CallbackQueue::iterator iter = g_callQueue.begin();
    while (iter != g_callQueue.end())
    {
      AddonClass::Ref<AsyncCallbackMessage> p(*iter);

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
          //  deallocating during the execution of this call.
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
              CLog::Log(LOGERROR,"Unknown exception while executing callback 0x%lx", (long)(p->cb.get()));
            }
          }
        }

        // since the state of the iterator may have been corrupted by
        //  the changing state of the list from another thread during
        //  the releasing fo the lock in the immediately preceeding
        //  codeblock, we need to reset it before continuing the loop
        iter = g_callQueue.begin();
      }
      else // if we're not in the right thread for this callback...
        ++iter;
    }
  }

  void RetardedAsyncCallbackHandler::clearPendingCalls(void* userData)
  {
    XBMC_TRACE;
    CSingleLock lock(critSection);
    CallbackQueue::iterator iter = g_callQueue.begin();
    while (iter != g_callQueue.end())
    {
      AddonClass::Ref<AsyncCallbackMessage> p(*iter);

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


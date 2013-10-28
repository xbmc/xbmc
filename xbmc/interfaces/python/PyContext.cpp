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

#include <Python.h>

#include "PyContext.h"
#include "threads/ThreadLocal.h"
#include "commons/Exception.h"
#include "utils/log.h"

namespace XBMCAddon
{
  namespace Python
  {
    struct PyContextState
    {
      inline PyContextState(PyContextState* pparent = NULL) : parent(pparent), release(true),
          value(0), state(NULL), currentState(NULL), gilDepth(0), createdByGilReleaseOrLock(false)
      {}

      PyContextState* parent;
      bool release; // true for PyGILRelease context. False for a PyGILLock context

      int value;
      PyThreadState* state;
      PyThreadState* currentState;
      int gilDepth;
      bool createdByGilReleaseOrLock;
    };

    static XbmcThreads::ThreadLocal<PyContextState> tlsPyContextState;

    static inline PyContextState* pushNewContextState(bool createdByGilReleaseOrLock)
    {
      PyContextState* cur = new PyContextState(tlsPyContextState.get());
      tlsPyContextState.set(cur);
      cur->value++; // put us inside of a PyContext
      cur->createdByGilReleaseOrLock = createdByGilReleaseOrLock;
      return cur;
    }

    static PyThreadState* getCurrentThreadState(PyContextState* ctx)
    {
      if (ctx == NULL)
        ctx = tlsPyContextState.get();
      if (ctx)
        return ctx->currentState == NULL ? (ctx->parent == NULL ? NULL : getCurrentThreadState(ctx->parent)) : ctx->currentState;
      return NULL;
    }

    /**
     * This conditionally pops the context. It decrements the value as
     *  if we're leaving the scope of a PyContext but only pops the current
     *  PyContextState if the current context count (value) = 0.
     *
     * It returns the popped PyContextState if one was actually popped.
     *  Otherwise it returns NULL.
     */
    static PyContextState* popContext()
    {
      PyContextState* cur = tlsPyContextState.get();
      cur->value--;
      int curlevel = cur->value;

      // this is a hack but ...
      if (curlevel < 0)
      {
        CLog::Log(LOGERROR, "FATAL: PyContext closed more than opened");
        curlevel = cur->value = 0;
      }

      if (curlevel == 0)
      {
        // clear the tlsPyContextState
        tlsPyContextState.set(cur->parent); // pop the stack of contexts
        return cur;
      }

      return NULL;
    }

    void PyContext::enterContext()
    {
      PyContextState* cur = tlsPyContextState.get();
      if (cur == NULL)
      {
        cur = new PyContextState();
        // The assumption here is that contexts are only created when we're holding the GIL
        cur->currentState = PyThreadState_Get();
        tlsPyContextState.set(cur);
      }
      else
      {
        // make sure we're still in the same thread?
        if (cur->currentState != PyThreadState_Get())
          throw XbmcCommons::UncheckedException("Cannot create a PyContext without holding the GIL.");
      }

      // increment the count
      cur->value++;
    }

    void PyContext::leaveContext()
    {
      PyContextState* cur = popContext();
      if (cur) delete cur;
    }

    // There is an assumption here that a PyGILRelease will only be used 
    //  at the outermost point one is used when the GIL is already held.
    void PyGILRelease::releaseGil()
    {
      PyContextState* cur = tlsPyContextState.get();

      // This means we're not within the python context, but
      // because we may be in a thread spawned by python itself,
      // we need to handle this.
      if (!cur || !cur->release)
        cur = pushNewContextState(true);

      if (cur->gilDepth == 0) // true if we are at the outermost
      {
        PyThreadState* _save;
        // this macro sets _save
        {
          Py_UNBLOCK_THREADS
        }
        cur->state = _save;
      }
      cur->gilDepth++; // the first time this goes to 1
      cur->release = true;
    }

    void PyGILRelease::acquireGil()
    {
      PyContextState* cur = tlsPyContextState.get(); 

      // This is bad. It means we're within the context of a PyGILLock rather
      // than the release we're trying to close.
      if (!cur->release)
      {
        CLog::Log(LOGERROR, "FATAL: PyGILRelease seemed to have an imbalanced pairing. It's closing scope is within the context of a PyGILLock.");
        return;
      }

      // it's not possible for cur to be NULL (and if it is, we want to fail anyway).

      // decrement the depth and make sure we're in the right place.
      cur->gilDepth--;
      if (cur->gilDepth == 0) // are we back to zero?
      {
        PyThreadState* _save = cur->state;
        // This macros uses _save
        {
          Py_BLOCK_THREADS
        }
        cur->state = NULL; // clear the state to indicate we've reacquired the gil

        // we clear it only if we created it on this level.
        if (cur->createdByGilReleaseOrLock)
        {
          cur = popContext();
          if (cur == NULL)
            CLog::Log(LOGERROR, "FATAL: PyGILRelease seemed to have an imbalanced pairing");
          else
            delete cur;
        }
      }
    }

    // The assumption here is that if we're not within a context then we
    // already have the lock. The only way we actually GET the lock is if
    // there's a release above us.
    void PyGILLock::acquireGil()
    {
      PyContextState* cur = tlsPyContextState.get();

      // This means we're not within the python context, but
      // because we may be in a thread spawned by python itself,
      // we need to handle this.
      if (!cur || cur->release)
        cur = pushNewContextState(true);

      if (cur->gilDepth == 0) // true if we are at the outermost PyGILLock
      {
        // walk back through the chain looking for a ThreadState. If we have none
        // Then we're already locked.
        PyContextState* tmp = cur->parent;
        PyThreadState* ct = NULL;
        while (tmp != NULL && ct == NULL)
        {
          ct = tmp->state;
          tmp = tmp->parent;
        }

        // if ct is still NULL then we don't have a PyGILRelease above us
        // in the call stack, so let's just grab the PyThreadState
        if (ct == NULL)
          ct = getCurrentThreadState(cur);

        // if ct == NULL then we're screwed.
        if (ct == NULL)
          CLog::Log(LOGERROR, "ERROR: using thread PyGILLock from an uninitialized thread. Assuming I already have it");
        else // if (ct != NULL)
        {
          // see if we actually have the lock
          if (ct != PyThreadState_Get()) // if the current state is us, the we already have it.
          {
            PyThreadState* _save = ct;
            // This macros uses _save
            {
              Py_BLOCK_THREADS
            }

            // store the thread state in the PyContextState as a flag 
            // to let the release know we actually called the block macro
            cur->state = _save;
          }
        }
      }
      cur->gilDepth++; // the first time this goes to 1
      cur->release = false;
    }

    // This is used to prevent a dumb warning
    inline static void pointless(bool) {}

    void PyGILLock::releaseGil()
    {
      PyContextState* cur = tlsPyContextState.get(); 

      // This is bad. It means we're within the context of a PyGILLock rather
      // than the release we're trying to close.
      if (cur->release)
      {
        CLog::Log(LOGERROR, "FATAL: PyGILLock seemed to have an imbalanced pairing. It's closing scope is within the context of a PyGILRelease.");
        return;
      }

      // it's not possible for cur to be NULL (and if it is, we want to fail anyway).

      // decrement the depth and make sure we're in the right place.
      cur->gilDepth--;
      if (cur->gilDepth == 0) // are we back to zero?
      {
        // Only unblock if we actually blocked.
        if (cur->state != NULL)
        {
          PyThreadState* _save;
          // this macro sets _save
          {
            Py_UNBLOCK_THREADS
          }

          // This stupid line just avoids a set-but-not-used warning.
          pointless(NULL == _save);

          cur->state = NULL;
        }

        // we clear it only if we created it on this level.
        if (cur->createdByGilReleaseOrLock)
        {
          cur = popContext();
          if (cur == NULL)
            CLog::Log(LOGERROR, "FATAL: PyGILLock seemed to have an imbalanced pairing");
          else
            delete cur;
        }
      }
    }
  }
}

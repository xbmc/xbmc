/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <Python.h>

#include "PyContext.h"
#include "utils/log.h"

namespace XBMCAddon
{
  namespace Python
  {
    struct PyContextState
    {
      inline explicit PyContextState(bool pcreatedByGilRelease = false) :
        state(NULL), createdByGilRelease(pcreatedByGilRelease) {}

      int value = 0;
      PyThreadState* state;
      int gilReleasedDepth = 0;
      bool createdByGilRelease;
    };

    static thread_local PyContextState* tlsPyContextState;

    void* PyContext::enterContext()
    {
      PyContextState* cur = tlsPyContextState;
      if (cur == NULL)
      {
        cur = new PyContextState();
        tlsPyContextState = cur;
      }

      // increment the count
      cur->value++;

      return cur;
    }

    void PyContext::leaveContext()
    {
      // here we ASSUME that the constructor was called.
      PyContextState* cur = tlsPyContextState;
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
        tlsPyContextState = NULL;
        delete cur;
      }
    }

    void PyGILLock::releaseGil()
    {
      PyContextState* cur = tlsPyContextState;

      // This means we're not within the python context, but
      // because we may be in a thread spawned by python itself,
      // we need to handle this.
      if (!cur)
      {
        cur = static_cast<PyContextState*>(PyContext::enterContext());
        cur->createdByGilRelease = true;
      }

      if (cur->gilReleasedDepth == 0) // true if we are at the outermost
      {
        PyThreadState* _save;
        // this macro sets _save
        {
          Py_UNBLOCK_THREADS
        }
        cur->state = _save;
      }
      cur->gilReleasedDepth++; // the first time this goes to 1
    }

    void PyGILLock::acquireGil()
    {
      PyContextState* cur = tlsPyContextState;

      // it's not possible for cur to be NULL (and if it is, we want to fail anyway).

      // decrement the depth and make sure we're in the right place.
      cur->gilReleasedDepth--;
      if (cur->gilReleasedDepth == 0) // are we back to zero?
      {
        PyThreadState* _save = cur->state;
        // This macros uses _save
        {
          Py_BLOCK_THREADS
        }
        cur->state = NULL; // clear the state to indicate we've reacquired the gil

        // we clear it only if we created it on this level.
        if (cur->createdByGilRelease)
          PyContext::leaveContext();
      }
    }
  }
}

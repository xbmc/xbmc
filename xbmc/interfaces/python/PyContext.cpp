/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

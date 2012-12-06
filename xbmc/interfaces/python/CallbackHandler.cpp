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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "CallbackHandler.h"

namespace XBMCAddon
{
  namespace Python
  {

    /**
     * We are ASS-U-MEing that this construction is happening
     *  within the context of a Python call. This way we can
     *  store off the PyThreadState to later verify that we're
     *  handling callbacks in the appropriate thread.
     */
    PythonCallbackHandler::PythonCallbackHandler() : RetardedAsynchCallbackHandler("PythonCallbackHandler")
    {
      objectThreadState = PyThreadState_Get();
      CLog::Log(LOGDEBUG,"NEWADDON PythonCallbackHandler construction with PyThreadState 0x%lx",(long)objectThreadState);
    }

    /**
     * Now we are answering the question as to whether or not we are in the
     *  PyThreadState that we were in when we started.
     */
    bool PythonCallbackHandler::isThreadStateOk()
    {
      TRACE;
      return objectThreadState == PyThreadState_Get();
    }

    /**
     * For this method we expect the PyThreadState to be passed as the user
     *  data for the check. 
     *
     * TODO: This is a stupid way to get this information back to the handler.
     *  there should be a more language neutral means.
     */
    bool PythonCallbackHandler::shouldRemoveCallback(void* threadState)
    {
      TRACE;
      return threadState == objectThreadState;
    }
  }
}

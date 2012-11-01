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

#include <Python.h>

#include "interfaces/legacy/CallbackHandler.h"

namespace XBMCAddon
{
  namespace Python
  {
    /**
     * This class represents a specialization of the callback handler
     *  that specifically checks to see if we're in an OK thread state
     *  based on Python specifics.
     */
    class PythonCallbackHandler : public RetardedAsynchCallbackHandler
    {
      PyThreadState* objectThreadState;
    public:

      /**
       * We are ASS-U-MEing that this construction is happening
       *  within the context of a Python call. This way we can
       *  store off the PyThreadState to later verify that we're
       *  handling callbacks in the appropriate thread.
       */
      PythonCallbackHandler();
      virtual bool isThreadStateOk();
      virtual bool shouldRemoveCallback(void* threadState);
    };
  }
}

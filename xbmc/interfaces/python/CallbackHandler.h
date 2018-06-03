/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
    class PythonCallbackHandler : public RetardedAsyncCallbackHandler
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
      bool isStateOk(AddonClass* obj) override;
      bool shouldRemoveCallback(AddonClass* obj, void* threadState) override;
    };
  }
}

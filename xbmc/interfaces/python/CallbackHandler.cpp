/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CallbackHandler.h"
#include "LanguageHook.h"

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
    PythonCallbackHandler::PythonCallbackHandler()
    {
      XBMC_TRACE;
      objectThreadState = PyThreadState_Get();
    }

    /**
     * Now we are answering the question as to whether or not we are in the
     *  PyThreadState that we were in when we started.
     */
    bool PythonCallbackHandler::isStateOk(AddonClass* obj)
    {
      XBMC_TRACE;
      PyThreadState* state = PyThreadState_Get();
      if (objectThreadState == state)
      {
        // make sure the interpreter is still active.
        AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook> lh(XBMCAddon::Python::PythonLanguageHook::GetIfExists(state->interp));
        if (lh.isNotNull() && lh->HasRegisteredAddonClassInstance(obj) && lh.get() == obj->GetLanguageHook())
          return true;
      }
      return false;
    }

    /**
     * For this method we expect the PyThreadState to be passed as the user
     *  data for the check.
     *
     * @todo This is a stupid way to get this information back to the handler.
     *  there should be a more language neutral means.
     */
    bool PythonCallbackHandler::shouldRemoveCallback(AddonClass* obj, void* threadState)
    {
      XBMC_TRACE;
      if (threadState == objectThreadState)
        return true;

      // we also want to remove the callback if the language hook no longer exists.
      //   this is a belt-and-suspenders cleanup mechanism
      return ! XBMCAddon::Python::PythonLanguageHook::IsAddonClassInstanceRegistered(obj);
    }
  }
}

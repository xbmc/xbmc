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

#pragma once

#ifndef LEGACY_ADDONCLASS_H_INCLUDED
#define LEGACY_ADDONCLASS_H_INCLUDED
#include "AddonClass.h"
#endif

#ifndef LEGACY_CALLBACKFUNCTION_H_INCLUDED
#define LEGACY_CALLBACKFUNCTION_H_INCLUDED
#include "CallbackFunction.h"
#endif

#ifndef LEGACY_CALLBACKHANDLER_H_INCLUDED
#define LEGACY_CALLBACKHANDLER_H_INCLUDED
#include "CallbackHandler.h"
#endif

#ifndef LEGACY_LANGUAGEHOOK_H_INCLUDED
#define LEGACY_LANGUAGEHOOK_H_INCLUDED
#include "LanguageHook.h"
#endif


namespace XBMCAddon
{

  /**
   * This class is the superclass for all API classes that are expected
   * to be able to handle cross-language polymorphism. 
   */
  class AddonCallback : public AddonClass
  {
  protected:
    AddonClass::Ref<CallbackHandler> handler;

    bool hasHandler() { return handler.isNotNull(); }

    inline AddonCallback() : handler(NULL)
    {
      // if there is a LanguageHook, it should be set already.
      if (languageHook != NULL)
        setHandler(languageHook->GetCallbackHandler());
    }
  public:

    virtual ~AddonCallback();

    inline void setHandler(CallbackHandler* _handler) { handler = _handler; }
    void invokeCallback(Callback* callback);
  };
}

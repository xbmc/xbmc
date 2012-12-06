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

#pragma once

#include "AddonClass.h"
#include "CallbackFunction.h"
#include "CallbackHandler.h"
#include "LanguageHook.h"

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

  public:
    inline AddonCallback(const char* classname) : AddonClass(classname), handler(NULL)
    {
      // if there is a LanguageHook, it should be set already.
      if (languageHook != NULL)
        setHandler(languageHook->getCallbackHandler());
    }
    virtual ~AddonCallback();

    void setHandler(CallbackHandler* _handler) { handler = _handler; }
    void invokeCallback(Callback* callback);
  };
}

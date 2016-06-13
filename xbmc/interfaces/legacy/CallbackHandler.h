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

#include "AddonClass.h"
#include "CallbackFunction.h"

namespace XBMCAddon
{
  /**
   * This is the abstraction representing different ways to handle
   *  the execution of callbacks. Different language bindings may
   *  have different requirements.
   */
  class CallbackHandler : public AddonClass
  {
  protected:
    inline CallbackHandler() {}

  public:
    virtual void invokeCallback(Callback* cb) = 0;
  };

  /**
   * This class is primarily for Python support (hence the "Retarded"
   *  prefix). Python (et. al. Retarded languages) require that 
   *  the context within which a callback executes is under the control
   *  of the language. Therefore, this handler is used to queue
   *  messages over to a language controlled thread for eventual
   *  execution.
   *
   * @todo Allow a cross thread synchronous execution.
   * Fix the stupid means of calling the clearPendingCalls by passing
   *  userData which is specific to the handler/language type.
   */
  class RetardedAsynchCallbackHandler : public CallbackHandler
  {
  protected:
    inline RetardedAsynchCallbackHandler() {}
  public:

    virtual ~RetardedAsynchCallbackHandler();

    virtual void invokeCallback(Callback* cb);
    static void makePendingCalls();
    static void clearPendingCalls(void* userData);

    virtual bool isStateOk(AddonClass* obj) =  0;
    virtual bool shouldRemoveCallback(AddonClass* obj, void* userData) = 0;
  };

}

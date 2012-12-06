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
    inline CallbackHandler(const char* classname):AddonClass(classname) {}

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
   * TODO: Allow a cross thread synchronous execution.
   * TODO: Fix the stupid means of calling the clearPendingCalls by passing
   *  userData which is specific to the handler/language type.
   */
  class RetardedAsynchCallbackHandler : public CallbackHandler
  {
  protected:
    RetardedAsynchCallbackHandler(const char* classname) : CallbackHandler(classname) {}
  public:

    virtual ~RetardedAsynchCallbackHandler();

    virtual void invokeCallback(Callback* cb);
    static void makePendingCalls();
    static void clearPendingCalls(void* userData);

    virtual bool isThreadStateOk() =  0;
    virtual bool shouldRemoveCallback(void* userData) = 0;
  };

}

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    inline CallbackHandler() = default;

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
  class RetardedAsyncCallbackHandler : public CallbackHandler
  {
  protected:
    inline RetardedAsyncCallbackHandler() = default;
  public:

    ~RetardedAsyncCallbackHandler() override;

    void invokeCallback(Callback* cb) override;
    static void makePendingCalls();
    static void clearPendingCalls(void* userData);

    virtual bool isStateOk(AddonClass* obj) =  0;
    virtual bool shouldRemoveCallback(AddonClass* obj, void* userData) = 0;
  };

}

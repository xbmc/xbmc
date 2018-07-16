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

    inline AddonCallback() : handler(NULL)
    {
      // if there is a LanguageHook, it should be set already.
      if (languageHook != NULL)
        setHandler(languageHook->GetCallbackHandler());
    }
  public:

    ~AddonCallback() override;

    inline void setHandler(CallbackHandler* _handler) { handler = _handler; }
    void invokeCallback(Callback* callback);
  };
}

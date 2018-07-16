/*
 *  Copyright (C) 2005-2013 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AddonCallback.h"

namespace XBMCAddon
{
  // need a place to put the vtab
  AddonCallback::~AddonCallback() = default;

  void AddonCallback::invokeCallback(Callback* callback)
  {
    if (callback)
    {
      if (hasHandler())
        handler->invokeCallback(callback);
      else
        callback->executeCallback();
    }
  }
}


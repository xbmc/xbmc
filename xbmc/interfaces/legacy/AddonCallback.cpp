/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "AddonCallback.h"

namespace XBMCAddon
{
  // need a place to put the vtab
  AddonCallback::~AddonCallback()
  {
  }

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


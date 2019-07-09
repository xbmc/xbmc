/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Window.h"

// These messages are a side effect of the way dialogs work through the
// main ApplicationMessenger. At some point it would be nice to remove
// the messenger and have direct (or even drive) communications.
#define HACK_CUSTOM_ACTION_CLOSING -3
#define HACK_CUSTOM_ACTION_OPENING -4

namespace XBMCAddon
{
  namespace xbmcgui
  {
    class WindowDialogMixin
    {
    private:
      Window* w;

    protected:
      inline explicit WindowDialogMixin(Window* window) : w(window) {}

    public:
      virtual ~WindowDialogMixin() = default;

      SWIGHIDDENVIRTUAL void show();
      SWIGHIDDENVIRTUAL void close();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool IsDialogRunning() const;
      SWIGHIDDENVIRTUAL bool OnAction(const CAction &action);
#endif
    };
  }
}

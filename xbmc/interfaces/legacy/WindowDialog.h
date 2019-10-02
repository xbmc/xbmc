/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Window.h"
#include "WindowDialogMixin.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    //
    /// \defgroup python_xbmcgui_window_dialog Subclass - WindowDialog
    /// \ingroup python_xbmcgui_window
    /// @{
    /// @brief __GUI window dialog class for Add-Ons.__
    ///
    /// \python_class{ xbmcgui.WindowDialog(int windowId): }
    ///
    /// Creates a new window from Add-On usable dialog class. This is to create
    /// window for related controls by system calls.
    ///
    /// @param windowId                  [opt] Specify an id to use an existing
    ///                                  window.
    /// @throws ValueError               if supplied window Id does not exist.
    /// @throws Exception                if more then 200 windows are created.
    ///
    /// Deleting this window will activate the old window that was active
    /// and resets (not delete) all controls that are associated with this
    /// window.
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// dialog = xbmcgui.WindowDialog()
    /// width = dialog.getWidth()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    ///
    //
    class WindowDialog : public Window, private WindowDialogMixin
    {
    public:
      WindowDialog();
      ~WindowDialog() override;

#ifndef SWIG
      bool OnMessage(CGUIMessage& message) override;
      bool OnAction(const CAction& action) override;
      void OnDeinitWindow(int nextWindowID) override;

      bool IsDialogRunning() const override { return WindowDialogMixin::IsDialogRunning(); }
      bool IsModalDialog() const override
      {
        XBMC_TRACE;
        return true;
      };
      bool IsDialog() const override
      {
        XBMC_TRACE;
        return true;
      };

      inline void show() override
      {
        XBMC_TRACE;
        WindowDialogMixin::show();
      }
      inline void close() override
      {
        XBMC_TRACE;
        WindowDialogMixin::close();
      }
#endif
    };
    ///@}
  }
}

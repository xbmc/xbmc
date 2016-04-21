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
      virtual ~WindowDialog();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool OnMessage(CGUIMessage& message);
      SWIGHIDDENVIRTUAL bool OnAction(const CAction &action);
      SWIGHIDDENVIRTUAL void OnDeinitWindow(int nextWindowID);

      SWIGHIDDENVIRTUAL bool IsDialogRunning() const { return WindowDialogMixin::IsDialogRunning(); }
      SWIGHIDDENVIRTUAL bool IsModalDialog() const { XBMC_TRACE; return true; };
      SWIGHIDDENVIRTUAL bool IsDialog() const { XBMC_TRACE; return true; };

      SWIGHIDDENVIRTUAL inline void show() { XBMC_TRACE; WindowDialogMixin::show(); }
      SWIGHIDDENVIRTUAL inline void close() { XBMC_TRACE; WindowDialogMixin::close(); }
#endif
    };
    ///@}
  }
}

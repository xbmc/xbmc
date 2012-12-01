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

#include "swighelper.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    /**
     * lock() -- Lock the gui until xbmcgui.unlock() is called.
     * 
     * *Note, This will improve performance when doing a lot of gui manipulation at once.
     *        The main program (xbmc itself) will freeze until xbmcgui.unlock() is called.
     * 
     * example:
     *   - xbmcgui.lock()
     */
    void lock();

    /**
     * unlock() -- Unlock the gui from a lock() call.
     * 
     * example:
     *   - xbmcgui.unlock()
     */
    void unlock();

    /**
     * getCurrentWindowId() -- Returns the id for the current 'active' window as an integer.
     * 
     * example:
     *   - wid = xbmcgui.getCurrentWindowId()
     */
    long getCurrentWindowId();

    /**
     * getCurrentWindowDialogId() -- Returns the id for the current 'active' dialog as an integer.
     * 
     * example:
     *   - wid = xbmcgui.getCurrentWindowDialogId()
     */
    long getCurrentWindowDialogId();

    SWIG_CONSTANT2(int,ICON_OVERLAY_NONE, CGUIListItem::ICON_OVERLAY_NONE);
    SWIG_CONSTANT2(int,ICON_OVERLAY_RAR, CGUIListItem::ICON_OVERLAY_RAR);
    SWIG_CONSTANT2(int,ICON_OVERLAY_ZIP, CGUIListItem::ICON_OVERLAY_ZIP);
    SWIG_CONSTANT2(int,ICON_OVERLAY_LOCKED, CGUIListItem::ICON_OVERLAY_LOCKED);
    SWIG_CONSTANT2(int,ICON_OVERLAY_HAS_TRAINER, CGUIListItem::ICON_OVERLAY_HAS_TRAINER);
    SWIG_CONSTANT2(int,ICON_OVERLAY_TRAINED, CGUIListItem::ICON_OVERLAY_TRAINED);
    SWIG_CONSTANT2(int,ICON_OVERLAY_UNWATCHED, CGUIListItem::ICON_OVERLAY_UNWATCHED);
    SWIG_CONSTANT2(int,ICON_OVERLAY_WATCHED, CGUIListItem::ICON_OVERLAY_WATCHED);
    SWIG_CONSTANT2(int,ICON_OVERLAY_HD, CGUIListItem::ICON_OVERLAY_HD);

  }
}

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

#include "swighelper.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    /**
     * lock() -- Lock the gui until xbmcgui.unlock() is called.\n
     * \n
     * *Note, This will improve performance when doing a lot of gui manipulation at once.\n
     *        The main program (xbmc itself) will freeze until xbmcgui.unlock() is called.\n
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
    SWIG_CONSTANT2(int,ICON_OVERLAY_UNWATCHED, CGUIListItem::ICON_OVERLAY_UNWATCHED);
    SWIG_CONSTANT2(int,ICON_OVERLAY_WATCHED, CGUIListItem::ICON_OVERLAY_WATCHED);
    SWIG_CONSTANT2(int,ICON_OVERLAY_HD, CGUIListItem::ICON_OVERLAY_HD);

    SWIG_CONSTANT_FROM_GETTER(const char*,NOTIFICATION_INFO);
    SWIG_CONSTANT_FROM_GETTER(const char*,NOTIFICATION_WARNING);
    SWIG_CONSTANT_FROM_GETTER(const char*,NOTIFICATION_ERROR);

    SWIG_CONSTANT(int,INPUT_ALPHANUM);
    SWIG_CONSTANT(int,INPUT_NUMERIC);
    SWIG_CONSTANT(int,INPUT_DATE);
    SWIG_CONSTANT(int,INPUT_TIME);
    SWIG_CONSTANT(int,INPUT_IPADDRESS);
    SWIG_CONSTANT(int,INPUT_PASSWORD);

    SWIG_CONSTANT(int,PASSWORD_VERIFY);
    SWIG_CONSTANT(int,ALPHANUM_HIDE_INPUT);

  }
}

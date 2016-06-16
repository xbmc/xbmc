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

#include <limits.h>
#include <vector>

#include "Window.h"
#include "windows/GUIMediaWindow.h"
#include "swighelper.h"
#include "WindowDialogMixin.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    class ListItem;
    class WindowXMLInterceptor;

    //
    /// \defgroup python_xbmcgui_window_xml Subclass - WindowXML
    /// \ingroup python_xbmcgui_window
    /// @{
    /// @brief __GUI xml window class.__
    ///
    /// \python_class{ xbmcgui.WindowXML(xmlFilename, scriptPath[, defaultSkin, defaultRes]) }
    ///
    /// Creates a new xml file based window class.
    ///
    /// \note This class include also all calls from <b><c>\ref python_xbmcgui_window</c></b>.
    ///
    /// @param xmlFilename          string - the name of the xml file to
    ///                             look for.
    /// @param scriptPath           string - path to script. used to
    ///                             fallback to if the xml doesn't exist in
    ///                             the current skin. (eg xbmcaddon.Addon().getAddonInfo('path').decode('utf-8'))
    /// @param defaultSkin          [opt] string - name of the folder in the
    ///                             skins path to look in for the xml.
    ///                             (default='Default')
    /// @param defaultRes           [opt] string - default skins resolution.
    ///                             (default='720p')
    /// @throws Exception           if more then 200 windows are created.
    ///
    /// \remark Skin folder structure is e.g. **resources/skins/Default/720p**
    ///
    /// Deleting this window will activate the old window that was active
    /// and resets (not delete) all controls that are associated with this
    /// window.
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// win = xbmcgui.WindowXML('script-Lyrics-main.xml', xbmcaddon.Addon().getAddonInfo('path').decode('utf-8'), 'default', '1080p')
    /// win.doModal()
    /// del win
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// On functions defined input variable <b><tt>controlId</tt> (GUI control identifier)</b>
    /// is the on window.xml defined value behind type added with <tt><b>id="..."</b></tt> and
    /// used to identify for changes there and on callbacks.
    ///
    /// ~~~~~~~~~~~~~{.xml}
    ///   <control type="label" id="31">
    ///     <description>Title Label</description>
    ///     ...
    ///   </control>
    ///   <control type="progress" id="32">
    ///     <description>progress control</description>
    ///     ...
    ///   </control>
    /// ~~~~~~~~~~~~~
    ///
    //
    class WindowXML : public Window
    {
      std::string sFallBackPath;

    protected:
#ifndef SWIG
      /**
       * This helper retrieves the next available id. It is doesn't
       *  assume that the global lock is already being held.
       */
      static int lockingGetNextAvailableWindowId();

      WindowXMLInterceptor* interceptor;
#endif

     public:
      WindowXML(const String& xmlFilename, const String& scriptPath,
                const String& defaultSkin = "Default",
                const String& defaultRes = "720p");
      virtual ~WindowXML();

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ addItem(item[, position]) }
      ///-----------------------------------------------------------------------
      /// Add a new item to this Window List.
      ///
      /// @param item            string, unicode or ListItem - item to add.
      /// @param position        [opt] integer - position of item to add. (NO Int = Adds to bottom,0 adds to top, 1 adds to one below from top,-1 adds to one above from bottom etc etc )
      ///  - If integer positions are greater than list size, negative positions will add to top of list, positive positions will add to bottom of list
      ///
      ///
      ///
      /// ----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.addItem('Reboot Kodi', 0)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      addItem(...);
#else
      SWIGHIDDENVIRTUAL void addItem(const Alternative<String, const ListItem*>& item, int position = INT_MAX);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ addItems(items) }
      ///-----------------------------------------------------------------------
      /// Add a list of items to to the window list.
      ///
      ///
      /// @param items                      List - list of strings, unicode objects or ListItems to add.
      ///
      ///
      /// ----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.addItems(['Reboot Kodi', 'Restart Kodi'])
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      addItems(...);
#else
      SWIGHIDDENVIRTUAL void addItems(const std::vector<Alternative<String, const XBMCAddon::xbmcgui::ListItem* > > & items);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ removeItem(position) }
      ///-----------------------------------------------------------------------
      /// Removes a specified item based on position, from the Window List.
      ///
      /// @param position        integer - position of item to remove.
      ///
      ///
      ///
      /// ----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.removeItem(5)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      removeItem(...);
#else
      SWIGHIDDENVIRTUAL void removeItem(int position);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ getCurrentListPosition() }
      ///-----------------------------------------------------------------------
      /// Gets the current position in the Window List.
      ///
      ///
      ///
      /// ----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pos = self.getCurrentListPosition()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getCurrentListPosition();
#else
      SWIGHIDDENVIRTUAL int getCurrentListPosition();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ setCurrentListPosition(position) }
      ///-----------------------------------------------------------------------
      /// Set the current position in the Window List.
      ///
      /// @param position        integer - position of item to set.
      ///
      ///
      ///
      /// ----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.setCurrentListPosition(5)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setCurrentListPosition(...);
#else
      SWIGHIDDENVIRTUAL void setCurrentListPosition(int position);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ getListItem(position) }
      ///-----------------------------------------------------------------------
      /// Returns a given ListItem in this Window List.
      ///
      /// @param position        integer - position of item to return.
      ///
      ///
      ///
      /// ----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// listitem = self.getListItem(6)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getListItem(...);
#else
      SWIGHIDDENVIRTUAL ListItem* getListItem(int position);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ getListSize() }
      ///-----------------------------------------------------------------------
      /// Returns the number of items in this Window List.
      ///
      ///
      ///
      /// ------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// listSize = self.getListSize()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getListSize();
#else
      SWIGHIDDENVIRTUAL int getListSize();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ clearList() }
      ///-----------------------------------------------------------------------
      /// Clear the Window List.
      ///
      ///
      ///
      /// ------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.clearList()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      clearList();
#else
      /**
       * clearList() -- Clear the Window List.
       *
       * example:\n
       *   - self.clearList()
       */
      SWIGHIDDENVIRTUAL void clearList();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ setContainerProperty(key, value) }
      ///-----------------------------------------------------------------------
      /// Sets a container property, similar to an infolabel.
      ///
      /// @param key            string - property name.
      /// @param value          string or unicode - value of property.
      ///
      /// @note Key is NOT case sensitive.\n
      /// You can use the above as keywords for arguments and skip certain
      /// optional arguments.\n
      /// Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      /// ------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.setContainerProperty('Category', 'Newest')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setContainerProperty(...);
#else
      SWIGHIDDENVIRTUAL void setContainerProperty(const String &strProperty, const String &strValue);
#endif

      /**
       * getCurrentContainerId() -- Get the id of the currently visible container
       * 
       * 
       * example:\n
       *   - container_id = self.getCurrentContainerId()
       */

      SWIGHIDDENVIRTUAL int getCurrentContainerId();


#ifndef SWIG
      // CGUIWindow
      SWIGHIDDENVIRTUAL bool      OnMessage(CGUIMessage& message);
      SWIGHIDDENVIRTUAL bool      OnAction(const CAction &action);
      SWIGHIDDENVIRTUAL void      AllocResources(bool forceLoad = false);
      SWIGHIDDENVIRTUAL void      FreeResources(bool forceUnLoad = false);
      SWIGHIDDENVIRTUAL bool      OnClick(int iItem);
      SWIGHIDDENVIRTUAL bool      OnDoubleClick(int iItem);
      SWIGHIDDENVIRTUAL void      Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);

      SWIGHIDDENVIRTUAL bool IsMediaWindow() const { XBMC_TRACE; return true; };

      // This method is identical to the Window::OnDeinitWindow method
      //  except it passes the message on to their respective parents.
      // Since the respective parent differences are handled by the
      // interceptor there's no reason to define this one here.
//      SWIGHIDDENVIRTUAL void    OnDeinitWindow(int nextWindowID);


    protected:
      // CGUIWindow
      SWIGHIDDENVIRTUAL bool     LoadXML(const String &strPath, const String &strPathLower);

      // CGUIMediaWindow
      SWIGHIDDENVIRTUAL void     GetContextButtons(int itemNumber, CContextButtons &buttons);
      SWIGHIDDENVIRTUAL bool     Update(const String &strPath);

      void             SetupShares();
      String       m_scriptPath;
      String       m_mediaDir;

      friend class WindowXMLInterceptor;
#endif
    };
    ///@}

    // Ideally what we want here is a Dialog/Media Window. The problem is that these
    //  are two orthogonal discriminations of CGUIWindow and there wasn't a previous
    //  accounting for this possibility through the use of making CGUIWindow a
    //  virtual base class of the pertinent subclasses. So now we're left with
    //  no good solution.
    //
    // <strike>So here we're going to have the 'main' hierarchy (the one visible to SWIG)
    //  go the way intended - through WindowXML, but we're going to borrow dialog
    //  functionality from CGUIDialog by using it as a Mixin.</strike>
    //
    // jmarshall says that this class has no reason to inherit from CGUIMediaWindow.
    //  At some point this entire hierarchy needs to be reworked. The XML handling
    //  routines should be put in a mixin.
    //
    /// \defgroup python_xbmcgui_window_dialog_xml Subclass - WindowDialogXML
    /// \ingroup python_xbmcgui_window_xml
    /// @{
    /// @brief __GUI xml window dialog__
    ///
    /// \python_class{ xbmcgui.WindowXMLDialog(xmlFilename, scriptPath[, defaultSkin, defaultRes]) }
    ///
    /// Creates a new xml file based window dialog class.
    ///
    /// @param xmlFilename              string - the name of the xml file to
    ///                                 look for.
    /// @param scriptPath               string - path to script. used to
    ///                                 fallback to if the xml doesn't exist in
    ///                                 the current skin. (eg \ref python_xbmcaddon_Addon "xbmcaddon.Addon().getAddonInfo('path').decode('utf-8'))"
    /// @param defaultSkin              [opt] string - name of the folder in the
    ///                                 skins path to look in for the xml.
    ///                                 (default='Default')
    /// @param defaultRes               [opt] string - default skins resolution.
    ///                                 (default='720p')
    /// @throws Exception               if more then 200 windows are created.
    ///
    /// @note Skin folder structure is e.g. **resources/skins/Default/720p**
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// dialog = xbmcgui.WindowXMLDialog('script-Lyrics-main.xml', xbmcaddon.Addon().getAddonInfo('path').decode('utf-8'), 'default', '1080p')
    /// dialog.doModal()
    /// del dialog
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// On functions defined input variable <b><tt>controlId</tt> (GUI control identifier)</b>
    /// is the on window.xml defined value behind type added with <tt><b>id="..."</b></tt> and
    /// used to identify for changes there and on callbacks.
    ///
    /// ~~~~~~~~~~~~~{.xml}
    ///   <control type="label" id="31">
    ///     <description>Title Label</description>
    ///     ...
    ///   </control>
    ///   <control type="progress" id="32">
    ///     <description>progress control</description>
    ///     ...
    ///   </control>
    /// ~~~~~~~~~~~~~
    ///
    //
    class WindowXMLDialog : public WindowXML, private WindowDialogMixin
    {
    public:
      WindowXMLDialog(const String& xmlFilename, const String& scriptPath,
                      const String& defaultSkin = "Default",
                      const String& defaultRes = "720p");

      virtual ~WindowXMLDialog();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool    OnMessage(CGUIMessage &message);
      SWIGHIDDENVIRTUAL bool    IsDialogRunning() const { XBMC_TRACE; return WindowDialogMixin::IsDialogRunning(); }
      SWIGHIDDENVIRTUAL bool    IsDialog() const { XBMC_TRACE; return true;};
      SWIGHIDDENVIRTUAL bool    IsModalDialog() const { XBMC_TRACE; return true; };
      SWIGHIDDENVIRTUAL bool    IsMediaWindow() const { XBMC_TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    OnAction(const CAction &action);
      SWIGHIDDENVIRTUAL void    OnDeinitWindow(int nextWindowID);

      SWIGHIDDENVIRTUAL bool    LoadXML(const String &strPath, const String &strPathLower);

      SWIGHIDDENVIRTUAL inline void show() { XBMC_TRACE; WindowDialogMixin::show(); }
      SWIGHIDDENVIRTUAL inline void close() { XBMC_TRACE; WindowDialogMixin::close(); }

      friend class DialogJumper;
#endif
    };
    ///@}
  }
}

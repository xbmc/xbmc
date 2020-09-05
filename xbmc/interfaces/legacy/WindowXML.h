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
#include "swighelper.h"
#include "windows/GUIMediaWindow.h"

#include <limits.h>
#include <vector>

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
    ///                             the current skin. (eg xbmcaddon.Addon().getAddonInfo('path'))
    /// @param defaultSkin          [opt] string - name of the folder in the
    ///                             skins path to look in for the xml.
    ///                             (default='Default')
    /// @param defaultRes           [opt] string - default skins resolution.
    ///                             (1080i, 720p, ntsc16x9, ntsc, pal16x9 or pal. default='720p')
    /// @param isMedia              [opt] bool - if False, create a regular window.
    ///                             if True, create a mediawindow.
    ///                             (default=False)
    /// @throws Exception           if more then 200 windows are created.
    ///
    /// \remark Skin folder structure is e.g. **resources/skins/Default/720p**
    ///
    /// Deleting this window will activate the old window that was active
    /// and resets (not delete) all controls that are associated with this
    /// window.
    ///
    ///--------------------------------------------------------------------------
    /// @python_v18 New param added **isMedia**.
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// win = xbmcgui.WindowXML('script-Lyrics-main.xml', xbmcaddon.Addon().getAddonInfo('path'), 'default', '1080i', False)
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
                const String& defaultRes = "720p",
                bool isMedia = false);
      ~WindowXML() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ addItem(item[, position]) }
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
      /// Clear the Window List.
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
      SWIGHIDDENVIRTUAL void clearList();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ setContainerProperty(key, value) }
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
      /// @python_v17 Changed function from **setProperty** to **setContainerProperty**.
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

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ setContent(value) }
      /// Sets the content type of the container.
      ///
      /// @param value          string or unicode - content value.
      ///
      /// __Available content types__
      /// | Name        | Media                                    |
      /// |:-----------:|:-----------------------------------------|
      /// | actors      | Videos
      /// | addons      | Addons, Music, Pictures, Programs, Videos
      /// | albums      | Music, Videos
      /// | artists     | Music, Videos
      /// | countries   | Music, Videos
      /// | directors   | Videos
      /// | files       | Music, Videos
      /// | games       | Games
      /// | genres      | Music, Videos
      /// | images      | Pictures
      /// | mixed       | Music, Videos
      /// | movies      | Videos
      /// | Musicvideos | Music, Videos
      /// | playlists   | Music, Videos
      /// | seasons     | Videos
      /// | sets        | Videos
      /// | songs       | Music
      /// | studios     | Music, Videos
      /// | tags        | Music, Videos
      /// | tvshows     | Videos
      /// | videos      | Videos
      /// | years       | Music, Videos
      ///
      /// ------------------------------------------------------------------------
      /// @python_v18 Added new function.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// self.setContent('movies')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      setContent(...);
#else
      SWIGHIDDENVIRTUAL void setContent(const String &strValue);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_window_xml
      /// @brief \python_func{ getCurrentContainerId() }
      /// Get the id of the currently visible container.
      ///
      /// ------------------------------------------------------------------------
      /// @python_v17 Added new function.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// container_id = self.getCurrentContainerId()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      getCurrentContainerId(...);
#else
      SWIGHIDDENVIRTUAL int getCurrentContainerId();
#endif

#ifndef SWIG
      // CGUIWindow
      bool OnMessage(CGUIMessage& message) override;
      bool OnAction(const CAction& action) override;
      SWIGHIDDENVIRTUAL void AllocResources(bool forceLoad = false);
      SWIGHIDDENVIRTUAL void FreeResources(bool forceUnLoad = false);
      SWIGHIDDENVIRTUAL bool OnClick(int iItem);
      SWIGHIDDENVIRTUAL bool OnDoubleClick(int iItem);
      SWIGHIDDENVIRTUAL void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);

      bool IsMediaWindow() const override
      {
        XBMC_TRACE;
        return m_isMedia;
      };

      // This method is identical to the Window::OnDeinitWindow method
      //  except it passes the message on to their respective parents.
      // Since the respective parent differences are handled by the
      // interceptor there's no reason to define this one here.
//      SWIGHIDDENVIRTUAL void    OnDeinitWindow(int nextWindowID);


    protected:
      // CGUIWindow
      SWIGHIDDENVIRTUAL bool LoadXML(const String &strPath, const String &strPathLower);

      // CGUIMediaWindow
      SWIGHIDDENVIRTUAL void GetContextButtons(int itemNumber, CContextButtons &buttons);
      SWIGHIDDENVIRTUAL bool Update(const String &strPath);

      void SetupShares();
      String m_scriptPath;
      String m_mediaDir;
      bool m_isMedia;

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
    ///                                 the current skin. (eg \ref python_xbmcaddon_Addon "xbmcaddon.Addon().getAddonInfo('path'))"
    /// @param defaultSkin              [opt] string - name of the folder in the
    ///                                 skins path to look in for the xml.
    ///                                 (default='Default')
    /// @param defaultRes               [opt] string - default skins resolution.
    ///                                 (1080i, 720p, ntsc16x9, ntsc, pal16x9 or pal. default='720p')
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
    /// dialog = xbmcgui.WindowXMLDialog('script-Lyrics-main.xml', xbmcaddon.Addon().getAddonInfo('path'), 'default', '1080i')
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

      ~WindowXMLDialog() override;

#ifndef SWIG
      bool OnMessage(CGUIMessage& message) override;
      bool IsDialogRunning() const override
      {
        XBMC_TRACE;
        return WindowDialogMixin::IsDialogRunning();
      }
      bool IsDialog() const override
      {
        XBMC_TRACE;
        return true;
      };
      bool IsModalDialog() const override
      {
        XBMC_TRACE;
        return true;
      };
      bool IsMediaWindow() const override
      {
        XBMC_TRACE;
        return false;
      };
      bool OnAction(const CAction& action) override;
      void OnDeinitWindow(int nextWindowID) override;

      bool LoadXML(const String& strPath, const String& strPathLower) override;

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

      friend class DialogJumper;
#endif
    };
    ///@}
  }
}

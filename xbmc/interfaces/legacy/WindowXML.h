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

    /**
     * WindowXML class.
     * 
     * WindowXML(self, xmlFilename, scriptPath[, defaultSkin, defaultRes]) -- Create a new WindowXML script.
     * 
     * xmlFilename     : string - the name of the xml file to look for.\n
     * scriptPath      : string - path to script. used to fallback to if the xml doesn't exist in the current skin. (eg os.getcwd())\n
     * defaultSkin     : [opt] string - name of the folder in the skins path to look in for the xml. (default='Default')\n
     * defaultRes      : [opt] string - default skins resolution. (default='720p')
     * 
     * *Note, skin folder structure is eg(resources/skins/Default/720p)
     * 
     * example:\n
     *  - ui = GUI('script-Lyrics-main.xml', os.getcwd(), 'LCARS', 'PAL')\n
     *    ui.doModal()\n
     *    del ui
     */
    class WindowXML : public Window
    {
      std::string sFallBackPath;

    protected:
#ifndef SWIG
      /**
       * This helper retrieves the next available id. It is doesn't
       *  assume that the global lock is already being held.
       */
      static int lockingGetNextAvailalbeWindowId();

      WindowXMLInterceptor* interceptor;
#endif

     public:
      WindowXML(const String& xmlFilename, const String& scriptPath,
                const String& defaultSkin = "Default",
                const String& defaultRes = "720p");
      virtual ~WindowXML();

      /**
       * addItem(item[, position]) -- Add a new item to this Window List.
       * 
       * - item            : string, unicode or ListItem - item to add.
       * - position        : [opt] integer - position of item to add. (NO Int = Adds to bottom,0 adds to top, 1 adds to one below from top,-1 adds to one above from bottom etc etc )
       *             - If integer positions are greater than list size, negative positions will add to top of list, positive positions will add to bottom of list
       *
       * example:
       *   - self.addItem('Reboot XBMC', 0)
       */
      SWIGHIDDENVIRTUAL void addItem(const Alternative<String, const ListItem*>& item, int position = INT_MAX);

      // these calls represent the python interface
      /**
       * removeItem(position) -- Removes a specified item based on position, from the Window List.
       * 
       * position        : integer - position of item to remove.
       * 
       * example:\n
       *   - self.removeItem(5)
       */
      SWIGHIDDENVIRTUAL void removeItem(int position);

      /**
       * getCurrentListPosition() -- Gets the current position in the Window List.
       * 
       * example:\n
       *   - pos = self.getCurrentListPosition()
       */
      SWIGHIDDENVIRTUAL int getCurrentListPosition();

      /**
       * setCurrentListPosition(position) -- Set the current position in the Window List.
       * 
       * position        : integer - position of item to set.
       * 
       * example:\n
       *   - self.setCurrentListPosition(5)
       */
      SWIGHIDDENVIRTUAL void setCurrentListPosition(int position);

      /**
       * getListItem(position) -- Returns a given ListItem in this Window List.
       * 
       * position        : integer - position of item to return.
       * 
       * example:\n
       *   - listitem = self.getListItem(6)
       */
      SWIGHIDDENVIRTUAL ListItem* getListItem(int position);

      /**
       * getListSize() -- Returns the number of items in this Window List.
       * 
       * example:\n
       *   - listSize = self.getListSize()
       */
      SWIGHIDDENVIRTUAL int getListSize();

      /**
       * clearList() -- Clear the Window List.
       * 
       * example:\n
       *   - self.clearList()
       */
      SWIGHIDDENVIRTUAL void clearList();

      /**
       * setProperty(key, value) -- Sets a container property, similar to an infolabel.
       * 
       * key            : string - property name.\n
       * value          : string or unicode - value of property.
       * 
       * *Note, Key is NOT case sensitive.
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:\n
       *   - self.setProperty('Category', 'Newest')
       */
      SWIGHIDDENVIRTUAL void setProperty(const String &strProperty, const String &strValue);

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

    /**
     * WindowXMLDialog class.
     * 
     * WindowXMLDialog(self, xmlFilename, scriptPath[, defaultSkin, defaultRes]) -- Create a new WindowXMLDialog script.
     * 
     * xmlFilename     : string - the name of the xml file to look for.\n
     * scriptPath      : string - path to script. used to fallback to if the xml doesn't exist in the current skin. (eg os.getcwd())\n
     * defaultSkin     : [opt] string - name of the folder in the skins path to look in for the xml. (default='Default')\n
     * defaultRes      : [opt] string - default skins resolution. (default='720p')
     * 
     * *Note, skin folder structure is eg(resources/skins/Default/720p)
     * 
     * example:
     *  - ui = GUI('script-Lyrics-main.xml', os.getcwd(), 'LCARS', 'PAL')
     *  - ui.doModal()
     *  - del ui
     */
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

      SWIGHIDDENVIRTUAL inline void show() { XBMC_TRACE; WindowDialogMixin::show(); }
      SWIGHIDDENVIRTUAL inline void close() { XBMC_TRACE; WindowDialogMixin::close(); }

      friend class DialogJumper;
#endif
    };
  }
}

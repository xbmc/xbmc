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

#pragma once

#include <limits.h>

#include "Window.h"
#include "windows/GUIMediaWindow.h"

#include "threads/Thread.h"

#include "swighelper.h"
#include "FileItem.h"

#include "WindowDialogMixin.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    class ListItem;
    class WindowXMLInterceptor;

    class WindowXML : public Window
    {
      std::string sFallBackPath;

      void initialize(const String& xmlFilename,
                      const String& scriptPath,
                      const String& defaultSkin,
                      const String& defaultRes);
    protected:
#ifndef SWIG
      /**
       * This helper retrieves the next available id. It is doesn't
       *  assume that the global lock is already being held.
       */
      static int lockingGetNextAvailalbeWindowId() throw (WindowException);

      WindowXMLInterceptor* interceptor;

      WindowXML(const char* classname, const String& xmlFilename, const String& scriptPath,
                const String& defaultSkin,
                const String& defaultRes) throw(WindowException);
#endif
     public:
      WindowXML(const String& xmlFilename, const String& scriptPath,
                const String& defaultSkin = "Default",
                const String& defaultRes = "720p") throw(WindowException);
      virtual ~WindowXML();

      // these calls represent the python interface
      SWIGHIDDENVIRTUAL void addItem(const String& item, int position = INT_MAX);
      SWIGHIDDENVIRTUAL void addListItem(ListItem* item, int position = INT_MAX);
      SWIGHIDDENVIRTUAL void removeItem(int position);
      SWIGHIDDENVIRTUAL int getCurrentListPosition();
      SWIGHIDDENVIRTUAL void setCurrentListPosition(int position);
      SWIGHIDDENVIRTUAL ListItem* getListItem(int position) throw (WindowException);
      SWIGHIDDENVIRTUAL int getListSize();
      SWIGHIDDENVIRTUAL void clearList();
      SWIGHIDDENVIRTUAL void setProperty(const String &strProperty, const String &strValue);

#ifndef SWIG
      // CGUIWindow
      SWIGHIDDENVIRTUAL bool      OnMessage(CGUIMessage& message);
      SWIGHIDDENVIRTUAL bool      OnAction(const CAction &action);
      SWIGHIDDENVIRTUAL void      AllocResources(bool forceLoad = false);
      SWIGHIDDENVIRTUAL void      FreeResources(bool forceUnLoad = false);
                        void      Process(unsigned int currentTime, CDirtyRegionList &regions);
      SWIGHIDDENVIRTUAL bool      OnClick(int iItem);

      SWIGHIDDENVIRTUAL bool IsMediaWindow() const { TRACE; return true; };
      SWIGHIDDENVIRTUAL bool    IsDialogRunning() const;

    protected:
      // CGUIWindow
      SWIGHIDDENVIRTUAL bool     LoadXML(const String &strPath, const String &strPathLower);

      // CGUIMediaWindow
      SWIGHIDDENVIRTUAL void     GetContextButtons(int itemNumber, CContextButtons &buttons);
      SWIGHIDDENVIRTUAL bool     Update(const String &strPath);

      unsigned int     LoadScriptStrings();
      void             ClearScriptStrings();
// see bug #10575.
//      void             SetupShares();
      String       m_scriptPath;
      String       m_mediaDir;

      friend class WindowXMLInterceptor;
#endif
    };

    /**
     * Ideally what we want here is a Dialog/Media Window. The problem is that these
     *  are two orthogonal discriminations of CGUIWindow and there wasn't a previous
     *  accounting for this possibility through the use of making CGUIWindow a 
     *  virtual base class of the pertinent subclasses. So now we're left with
     *  no good solution.
     *
     * <strike>So here we're going to have the 'main' hierarchy (the one visible to SWIG)
     *  go the way intended - through WindowXML, but we're going to borrow dialog 
     *  functionality from CGUIDialog by using it as a Mixin.</strike>
     *
     * jmarshall says that this class has no reason to inherit from CGUIMediaWindow.
     *  At some point this entire hierarchy needs to be reworked. The XML handling
     *  routines should be put in a mixin.
     */
    class WindowXMLDialog : public WindowXML, private WindowDialogMixin
    {
    public:
      WindowXMLDialog(const String& xmlFilename, const String& scriptPath,
                      const String& defaultSkin = "Default",
                      const String& defaultRes = "720p") throw(WindowException);

      virtual ~WindowXMLDialog();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool    OnMessage(CGUIMessage &message);
      SWIGHIDDENVIRTUAL bool    IsDialog() const { TRACE; return true;};
      SWIGHIDDENVIRTUAL bool    IsModalDialog() const { TRACE; return true; };
      SWIGHIDDENVIRTUAL bool    IsMediaWindow() const { TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    OnAction(const CAction &action);
#endif

      SWIGHIDDENVIRTUAL inline void show() { WindowDialogMixin::show(); }
      SWIGHIDDENVIRTUAL inline void close() { WindowDialogMixin::close(); }

      friend class DialogJumper;
    };
  }
}

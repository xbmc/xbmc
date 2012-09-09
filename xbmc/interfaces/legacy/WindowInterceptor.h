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

#include "guilib/GUIWindow.h"
#include "Window.h"

#include "threads/ThreadLocal.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {

#ifndef SWIG
    class Window;
    class ref;

    /**
     * These two classes are closely associated with the Interceptor template below.
     *  For more detailed explanation, see that class.
     */
    class InterceptorBase
    {
    protected:
      Window* window;
      XbmcThreads::ThreadLocal<ref> upcallTls;

      InterceptorBase() : window(NULL) { upcallTls.set(NULL); }

      /**
       * Calling up ONCE resets the upcall to to false. The reason is that when
       *   a call is recursive we cannot assume the ref has cleared the flag.
       *   so ... 
       *
       * ref(window)->UpCall()
       *
       * durring the context of 'UpCall' it's possible that another call will
       *  be made back on the window from the xbmc core side (this happens in
       *  sometimes in OnMessage). In that case, if upcall is still 'true', than
       *  the call will wrongly proceed back to the xbmc core side rather than
       *  to the Addon API side.
       */  
      bool up() { bool ret = (upcallTls.get() != NULL); upcallTls.set(NULL); return ret; }
    public:

      // TODO: Add thread safety
      void clear() { window = NULL; }
      virtual ~InterceptorBase() {}

      virtual CGUIWindow* get() = 0;

      virtual void setActive(bool active) { } ;
      virtual bool isActive() { return false; }
      
      friend class ref;
    };

    /**
     * Guard class. But instead of managing memory or thread resources,
     * any call made using the operator-> results in an 'upcall.' That is, 
     * it expects the call about to be made to have come from the XBMCAddon
     * xbmcgui Window API and not from either the scripting language or the 
     * XBMC core Windowing system.
     *
     * This class is meant to hold references to instances of Interceptor<P>.
     *   see that template definition below.
     */
    class ref 
    {
      InterceptorBase* w;
    public:
      ref(InterceptorBase* b) : w(b) { w->upcallTls.set(this); }
      ~ref() { w->upcallTls.set(NULL); }
      CGUIWindow* operator->() { return w->get(); }
      CGUIWindow* get() { return w->get(); }
    };

    /**
     * The intention of this class is to intercept calls from
     *  multiple points in the CGUIWindow class hierarchy and pass
     *  those calls over to the XBMCAddon API Window hierarchy. It 
     *  is a class template that uses the type template parameter
     *  to determine the parent class.
     *
     * Since this class also maintains two way communication between
     *  the XBMCAddon API Window hierarchy and the core XBMC CGUIWindow
     *  hierarchy, it is used as a general bridge. For calls going
     *  TO the window hierarchy (as in callbacks) it operates in a
     *  straightforward manner. In the reverse direction (from the
     *  XBMCAddon hierarchy) it uses some hackery in a "smart pointer"
     *  overloaded -> operator.
     */

#define checkedb(methcall) ( window ? window-> methcall : false )
#define checkedv(methcall) { if (window) window-> methcall ; }

    template <class P /* extends CGUIWindow*/> class Interceptor : 
      public P, public InterceptorBase
    {
      std::string classname;
    protected:
      virtual CGUIWindow* get() { return this; }

      // this is only called from XBMC core and we only want it to return true every time
      virtual bool     Update(const String &strPath) { return true; }

    public:
      Interceptor(const char* specializedName,
                  Window* _window, int windowid) : P(windowid, "")
      { 
        ((classname = "Interceptor<") += specializedName) += ">";
        CLog::Log(LOGDEBUG, "NEWADDON LIFECYCLE constructing %s 0x%lx", classname.c_str(), (long)(((void*)this)));
        window = _window; 
        P::SetLoadType(CGUIWindow::LOAD_ON_GUI_INIT);
      }
                    
      Interceptor(const char* specializedName,
                  Window* _window, int windowid,
                  const char* xmlfile) : P(windowid, xmlfile)
      { 
        ((classname = "Interceptor<") += specializedName) += ">";
        CLog::Log(LOGDEBUG, "NEWADDON LIFECYCLE constructing %s 0x%lx", classname.c_str(), (long)(((void*)this)));
        window = _window;
        P::SetLoadType(CGUIWindow::LOAD_ON_GUI_INIT);
      }

      virtual ~Interceptor()
      { 
        CLog::Log(LOGDEBUG, "NEWADDON LIFECYCLE destroying %s 0x%lx", classname.c_str(), (long)(((void*)this)));
      }

      virtual bool    OnMessage(CGUIMessage& message) 
      { TRACE; return up() ? P::OnMessage(message) : checkedb(OnMessage(message)); }
      virtual bool    OnAction(const CAction &action) 
      { TRACE; return up() ? P::OnAction(action) : checkedb(OnAction(action)); }
      virtual void OnDeinitWindow(int nextWindowID)
      { TRACE; if(up()) P::OnDeinitWindow(nextWindowID); else checkedv(OnDeinitWindow(nextWindowID)); }

      virtual bool    IsModalDialog() const { TRACE; return checkedb(IsModalDialog()); };

      virtual bool    IsDialog() const { TRACE; return checkedb(IsDialog()); };
      virtual bool    IsMediaWindow() const { return checkedb(IsMediaWindow());; };

      virtual void    setActive(bool active) { P::m_active = active; }
      virtual bool    isActive() { return P::m_active; }
    };

    template <class P /* extends CGUIWindow*/> class InterceptorDialog : 
      public Interceptor<P> //, public DialogMixin
    {
    public:
      InterceptorDialog(const char* specializedName,
                        Window* _window, int windowid) : 
        Interceptor<P>(specializedName, _window, windowid)
      { }
                    
      InterceptorDialog(const char* specializedName,
                  Window* _window, int windowid,
                  const char* xmlfile) : 
        Interceptor<P>(specializedName, _window, windowid,xmlfile)
      { }

      // Another skip level hack
      bool skipLevelOnMessage(CGUIMessage& message) 
      { TRACE; return CGUIWindow::OnMessage(message); }
    };

#undef checkedb
#undef checkedv

#endif
  }
}

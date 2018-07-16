 /*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"
#include "Window.h"

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
      AddonClass::Ref<Window> window;
      // This instance is in Window.cpp
      static thread_local ref* upcallTls;

      InterceptorBase() : window(NULL) { upcallTls = NULL; }

      /**
       * Calling up ONCE resets the upcall to to false. The reason is that when
       *   a call is recursive we cannot assume the ref has cleared the flag.
       *   so ...
       *
       * ref(window)->UpCall()
       *
       * during the context of 'UpCall' it's possible that another call will
       *  be made back on the window from the xbmc core side (this happens in
       *  sometimes in OnMessage). In that case, if upcall is still 'true', than
       *  the call will wrongly proceed back to the xbmc core side rather than
       *  to the Addon API side.
       */
      static bool up() { bool ret = ((upcallTls) != NULL); upcallTls = NULL; return ret; }
    public:

      virtual ~InterceptorBase() { if (window.isNotNull()) { window->interceptorClear(); } }

      virtual CGUIWindow* get() = 0;

      virtual void SetRenderOrder(int renderOrder) { }

      virtual void setActive(bool active) { }
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
      inline explicit ref(InterceptorBase* b) : w(b) { w->upcallTls = this; }
      inline ~ref() { w->upcallTls = NULL; }
      inline CGUIWindow* operator->() { return w->get(); }
      inline CGUIWindow* get() { return w->get(); }
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

#define checkedb(methcall) ( window.isNotNull() ? window-> methcall : false )
#define checkedv(methcall) { if (window.isNotNull()) window-> methcall ; }

    template <class P /* extends CGUIWindow*/> class Interceptor :
      public P, public InterceptorBase
    {
      std::string classname;
    protected:
      CGUIWindow* get() override { return this; }

    public:
      Interceptor(const char* specializedName,
                  Window* _window, int windowid) : P(windowid, ""),
        classname("Interceptor<" + std::string(specializedName) + ">")
      {
#ifdef ENABLE_XBMC_TRACE_API
        XBMCAddonUtils::TraceGuard tg;
        CLog::Log(LOGDEBUG, "%sNEWADDON constructing %s 0x%lx", tg.getSpaces(),classname.c_str(), (long)(((void*)this)));
#endif
        window.reset(_window);
        P::SetLoadType(CGUIWindow::LOAD_ON_GUI_INIT);
      }

      Interceptor(const char* specializedName,
                  Window* _window, int windowid,
                  const char* xmlfile) : P(windowid, xmlfile),
        classname("Interceptor<" + std::string(specializedName) + ">")
      {
#ifdef ENABLE_XBMC_TRACE_API
        XBMCAddonUtils::TraceGuard tg;
        CLog::Log(LOGDEBUG, "%sNEWADDON constructing %s 0x%lx", tg.getSpaces(),classname.c_str(), (long)(((void*)this)));
#endif
        window.reset(_window);
        P::SetLoadType(CGUIWindow::LOAD_ON_GUI_INIT);
      }

      ~Interceptor() override
      {
#ifdef ENABLE_XBMC_TRACE_API
        XBMCAddonUtils::TraceGuard tg;
        CLog::Log(LOGDEBUG, "%sNEWADDON LIFECYCLE destroying %s 0x%lx", tg.getSpaces(),classname.c_str(), (long)(((void*)this)));
#endif
      }

      bool OnMessage(CGUIMessage& message) override
      { XBMC_TRACE; return up() ? P::OnMessage(message) : checkedb(OnMessage(message)); }
      bool OnAction(const CAction &action) override
      { XBMC_TRACE; return up() ? P::OnAction(action) : checkedb(OnAction(action)); }

      // NOTE!!: This ALWAYS skips up to the CGUIWindow instance.
      bool OnBack(int actionId) override
      { XBMC_TRACE; return up() ? CGUIWindow::OnBack(actionId) : checkedb(OnBack(actionId)); }

      void OnDeinitWindow(int nextWindowID) override
      { XBMC_TRACE; if(up()) P::OnDeinitWindow(nextWindowID); else checkedv(OnDeinitWindow(nextWindowID)); }

      bool IsModalDialog() const override { XBMC_TRACE; return checkedb(IsModalDialog()); };

      bool IsDialogRunning() const override { XBMC_TRACE; return checkedb(IsDialogRunning()); };
      bool IsDialog() const override { XBMC_TRACE; return checkedb(IsDialog()); };
      bool IsMediaWindow() const override { XBMC_TRACE; return checkedb(IsMediaWindow()); };

      void SetRenderOrder(int renderOrder) override { XBMC_TRACE; P::m_renderOrder = renderOrder; }

      void setActive(bool active) override { XBMC_TRACE; P::m_active = active; }
      bool isActive() override { XBMC_TRACE; return P::m_active; }
    };

    template <class P /* extends CGUIWindow*/> class InterceptorDialog :
      public Interceptor<P>
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
    };

#undef checkedb
#undef checkedv

#endif
  }
}

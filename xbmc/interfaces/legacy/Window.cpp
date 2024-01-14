/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Window.h"

#include "ServiceBroker.h"
#include "WindowException.h"
#include "WindowInterceptor.h"
#include "application/Application.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#define ACTIVE_WINDOW CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow()

namespace XBMCAddon
{
  namespace xbmcgui
  {
    thread_local ref* InterceptorBase::upcallTls;

    /**
     * Used in add/remove control. It only locks if it's given a
     * non-NULL CCriticalSection. It's given a NULL CCriticalSection
     * when a function higher in the call stack already has a
     */
    class MaybeLock
    {
      CCriticalSection* lock;
    public:
      inline explicit MaybeLock(CCriticalSection* p_lock) : lock(p_lock) { if (lock) lock->lock(); }
      inline ~MaybeLock() { if (lock) lock->unlock(); }
    };

    class SingleLockWithDelayGuard
    {
      DelayedCallGuard dcg;
      CCriticalSection& lock;
    public:
      inline SingleLockWithDelayGuard(CCriticalSection& ccrit, LanguageHook* lh) : dcg(lh), lock(ccrit) { lock.lock(); }
      inline ~SingleLockWithDelayGuard() { lock.unlock(); }
    };

    /**
     * Explicit template instantiation
     */
    template class Interceptor<CGUIWindow>;

    /**
     * This interceptor is a simple, non-callbackable (is that a word?)
     *  Interceptor to satisfy the Window requirements for upcalling
     *  for the purposes of instantiating a Window instance from
     *  an already existing window.
     */
    class ProxyExistingWindowInterceptor : public InterceptorBase
    {
      CGUIWindow* cguiwindow;

    public:
      inline ProxyExistingWindowInterceptor(CGUIWindow* window) :
        cguiwindow(window) { XBMC_TRACE; }

      CGUIWindow* get() override;
    };

    CGUIWindow* ProxyExistingWindowInterceptor::get() { XBMC_TRACE; return cguiwindow; }

    Window::Window(bool discrim):
      window(NULL),
      m_actionEvent(true),
      canPulse(true), existingWindow(false)
    {
      XBMC_TRACE;
    }

    /**
     * This just creates a default window.
     */
    Window::Window(int existingWindowId) :
      window(NULL),
      m_actionEvent(true)
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);

      if (existingWindowId == -1)
      {
        // in this case just do the other constructor.
        canPulse = true;
        existingWindow = false;

        setWindow(new Interceptor<CGUIWindow>("CGUIWindow",this,getNextAvailableWindowId()));
      }
      else
      {
        // user specified window id, use this one if it exists
        // It is not possible to capture key presses or button presses
        CGUIWindow* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(existingWindowId);
        if (!pWindow)
          throw WindowException("Window id does not exist");

        setWindow(new ProxyExistingWindowInterceptor(pWindow));
      }
    }

    Window::~Window()
    {
      XBMC_TRACE;

      deallocating();
    }

    void Window::deallocating()
    {
      AddonCallback::deallocating();

      dispose();
    }

    void Window::dispose()
    {
      XBMC_TRACE;

      //! @todo rework locking
      // Python GIL and CServiceBroker::GetWinSystem()->GetGfxContext() are deadlock happy
      // dispose is called from GUIWindowManager and in this case DelayGuard must not be used.
      if (!CServiceBroker::GetAppMessenger()->IsProcessThread())
      {
        SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(), languageHook);
      }

      if (!isDisposed)
      {
        isDisposed = true;

        // no callbacks are possible any longer
        //   - this will be handled by the parent constructor

        // first change to an existing window
        if (!existingWindow)
        {
          if (ACTIVE_WINDOW == iWindowId && !g_application.m_bStop)
          {
            if(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(iOldWindowId))
            {
              CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(iOldWindowId);
            }
            // old window does not exist anymore, switch to home
            else CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_HOME);
          }

        }
        else
        {
          //! @bug
          //! This is an existing window, so no resources are free'd.  Note that
          //! THIS WILL FAIL for any controls newly created by python - they will
          //! remain after the script ends.  Ideally this would be remedied by
          //! a flag in Control that specifies that it was python created - any python
          //! created controls could then be removed + free'd from the window.
          //! how this works with controlgroups though could be a bit tricky.
        }

        // and free our list of controls
        std::vector<AddonClass::Ref<Control> >::iterator it = vecControls.begin();
        while (it != vecControls.end())
        {
          AddonClass::Ref<Control> pControl = *it;
          // initialize control to zero
          pControl->pGUIControl = NULL;
          pControl->iControlId = 0;
          pControl->iParentId = 0;
          ++it;
        }

        if (!existingWindow)
        {
          if (window)
          {
            if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowVisible(ref(window)->GetID()))
            {
              destroyAfterDeInit = true;
              close();
            }
            else
              CServiceBroker::GetGUI()->GetWindowManager().Delete(ref(window)->GetID());
          }
        }

        vecControls.clear();
      }
    }

    void Window::setWindow(InterceptorBase* _window)
    {
      XBMC_TRACE;
      window = _window;
      iWindowId = _window->get()->GetID();

      if (!existingWindow)
        CServiceBroker::GetGUI()->GetWindowManager().Add(window->get());
    }

    int Window::getNextAvailableWindowId()
    {
      XBMC_TRACE;
      // window id's 13000 - 13100 are reserved for python
      // get first window id that is not in use
      int id = WINDOW_PYTHON_START;
      // if window 13099 is in use it means python can't create more windows
      if (CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_PYTHON_END))
        throw WindowException("maximum number of windows reached");

      while(id < WINDOW_PYTHON_END && CServiceBroker::GetGUI()->GetWindowManager().GetWindow(id) != NULL) id++;
      return id;
    }

    void Window::popActiveWindowId()
    {
      XBMC_TRACE;
      if (iOldWindowId != iWindowId &&
          iWindowId != ACTIVE_WINDOW)
        iOldWindowId = ACTIVE_WINDOW;
    }

    // Internal helper method
    /* Searches for a control in Window->vecControls
     * If we can't find any but the window has the controlId (in case of a not python window)
     * we create a new control with basic functionality
     */
    Control* Window::GetControlById(int iControlId, CCriticalSection* gc)
    {
      XBMC_TRACE;

      // find in window vector first!!!
      // this saves us from creating a complete new control
      std::vector<AddonClass::Ref<Control> >::iterator it = vecControls.begin();
      while (it != vecControls.end())
      {
        AddonClass::Ref<Control> control = (*it);
        if (control->iControlId == iControlId)
        {
          return control.get();
        } else ++it;
      }

      // lock xbmc GUI before accessing data from it
      MaybeLock lock(gc);

      // check if control exists
      CGUIControl* pGUIControl = ref(window)->GetControl(iControlId);
      if (!pGUIControl)
      {
        // control does not exist.
        throw WindowException("Non-Existent Control %d",iControlId);
      }

      // allocate a new control with a new reference
      CLabelInfo li;

      Control* pControl = NULL;

      //! @todo Yuck! Should probably be done with a Factory pattern
      switch(pGUIControl->GetControlType())
      {
      case CGUIControl::GUICONTROL_BUTTON:
        pControl = new ControlButton();

        li = ((CGUIButtonControl *)pGUIControl)->GetLabelInfo();

        // note: conversion from infocolors -> plain colors here
        ((ControlButton*)pControl)->disabledColor = li.disabledColor;
        ((ControlButton*)pControl)->focusedColor  = li.focusedColor;
        ((ControlButton*)pControl)->textColor  = li.textColor;
        ((ControlButton*)pControl)->shadowColor   = li.shadowColor;
        if (li.font) ((ControlButton*)pControl)->strFont = li.font->GetFontName();
        ((ControlButton*)pControl)->align = li.align;
        break;
      case CGUIControl::GUICONTROL_LABEL:
        pControl = new ControlLabel();
        break;
      case CGUIControl::GUICONTROL_SPIN:
        pControl = new ControlSpin();
        break;
      case CGUIControl::GUICONTROL_FADELABEL:
        pControl = new ControlFadeLabel();
        break;
      case CGUIControl::GUICONTROL_TEXTBOX:
        pControl = new ControlTextBox();
        break;
      case CGUIControl::GUICONTROL_IMAGE:
      case CGUIControl::GUICONTROL_BORDEREDIMAGE:
        pControl = new ControlImage();
        break;
      case CGUIControl::GUICONTROL_PROGRESS:
        pControl = new ControlProgress();
        break;
      case CGUIControl::GUICONTROL_SLIDER:
        pControl = new ControlSlider();
        break;
      case CGUIControl::GUICONTAINER_LIST:
      case CGUIControl::GUICONTAINER_WRAPLIST:
      case CGUIControl::GUICONTAINER_FIXEDLIST:
      case CGUIControl::GUICONTAINER_PANEL:
        pControl = new ControlList();
        // create a python spin control
        ((ControlList*)pControl)->pControlSpin = new ControlSpin();
        break;
      case CGUIControl::GUICONTROL_GROUP:
        pControl = new ControlGroup();
        break;
      case CGUIControl::GUICONTROL_RADIO:
        pControl = new ControlRadioButton();

        li = ((CGUIRadioButtonControl *)pGUIControl)->GetLabelInfo();

        // note: conversion from infocolors -> plain colors here
        ((ControlRadioButton*)pControl)->disabledColor = li.disabledColor;
        ((ControlRadioButton*)pControl)->focusedColor  = li.focusedColor;
        ((ControlRadioButton*)pControl)->textColor  = li.textColor;
        ((ControlRadioButton*)pControl)->shadowColor   = li.shadowColor;
        if (li.font) ((ControlRadioButton*)pControl)->strFont = li.font->GetFontName();
        ((ControlRadioButton*)pControl)->align = li.align;
        break;
      case CGUIControl::GUICONTROL_EDIT:
        pControl = new ControlEdit();

        li = ((CGUIEditControl *)pGUIControl)->GetLabelInfo();

        // note: conversion from infocolors -> plain colors here
        ((ControlEdit*)pControl)->disabledColor = li.disabledColor;
        ((ControlEdit*)pControl)->textColor  = li.textColor;
        if (li.font) ((ControlEdit*)pControl)->strFont = li.font->GetFontName();
        ((ControlButton*)pControl)->align = li.align;
        break;
      default:
        break;
      }

      if (!pControl)
        // throw an exception
        throw WindowException("Unknown control type for python");

      // we have a valid control here, fill in all the 'Control' data
      pControl->pGUIControl = pGUIControl;
      pControl->iControlId = pGUIControl->GetID();
      pControl->iParentId = iWindowId;
      pControl->dwHeight = (int)pGUIControl->GetHeight();
      pControl->dwWidth = (int)pGUIControl->GetWidth();
      pControl->dwPosX = (int)pGUIControl->GetXPosition();
      pControl->dwPosY = (int)pGUIControl->GetYPosition();
      pControl->iControlUp = pGUIControl->GetAction(ACTION_MOVE_UP).GetNavigation();
      pControl->iControlDown = pGUIControl->GetAction(ACTION_MOVE_DOWN).GetNavigation();
      pControl->iControlLeft = pGUIControl->GetAction(ACTION_MOVE_LEFT).GetNavigation();
      pControl->iControlRight = pGUIControl->GetAction(ACTION_MOVE_RIGHT).GetNavigation();

      // It got this far so means the control isn't actually in the vector of controls
      // so lets add it to save doing all that next time
      vecControls.emplace_back(pControl);

      // return the control with increased reference (+1)
      return pControl;
    }

    void Window::PulseActionEvent()
    {
      XBMC_TRACE;
      if (canPulse)
        m_actionEvent.Set();
    }

    bool Window::WaitForActionEvent(unsigned int milliseconds)
    {
      XBMC_TRACE;
      // DO NOT MAKE THIS A DELAYED CALL!!!!
      bool ret = languageHook == NULL ? m_actionEvent.Wait(std::chrono::milliseconds(milliseconds))
                                      : languageHook->WaitForEvent(m_actionEvent, milliseconds);
      if (ret)
        m_actionEvent.Reset();
      return ret;
    }

    bool Window::OnAction(const CAction &action)
    {
      XBMC_TRACE;
      // do the base class window first, and the call to python after this
      bool ret = ref(window)->OnAction(action);

      // workaround - for scripts which try to access the active control (focused) when there is none.
      // for example - the case when the mouse enters the screen.
      CGUIControl *pControl = ref(window)->GetFocusedControl();
      if (action.IsMouse() && !pControl)
        return ret;

      AddonClass::Ref<Action> inf(new Action(action));
      invokeCallback(new CallbackFunction<Window,AddonClass::Ref<Action> >(this,&Window::onAction,inf.get()));
      PulseActionEvent();

      return ret;
    }

    bool Window::OnBack(int actionID)
    {
      // we are always a Python window ... keep that in mind when reviewing the old code
      return true;
    }

    void Window::OnDeinitWindow(int nextWindowID /*= 0*/)
    {
      // NOTE!: This handle child classes correctly. XML windows will call
      // the OnDeinitWindow from CGUIMediaWindow while non-XML classes will
      // call the OnDeinitWindow on CGUIWindow
      ref(window)->OnDeinitWindow(nextWindowID);
      if (destroyAfterDeInit)
        CServiceBroker::GetGUI()->GetWindowManager().Delete(window->get()->GetID());
    }

    void Window::onAction(Action* action)
    {
      XBMC_TRACE;
      // default onAction behavior
      if(action->id == ACTION_PREVIOUS_MENU || action->id == ACTION_NAV_BACK)
        close();
    }

    bool Window::OnMessage(CGUIMessage& message)
    {
      XBMC_TRACE;
      switch (message.GetMessage())
      {
      case GUI_MSG_CLICKED:
        {
          int iControl=message.GetSenderId();
          AddonClass::Ref<Control> inf;
          // find python control object with same iControl
          std::vector<AddonClass::Ref<Control> >::iterator it = vecControls.begin();
          while (it != vecControls.end())
          {
            AddonClass::Ref<Control> pControl = (*it);
            if (pControl->iControlId == iControl)
            {
              inf = pControl.get();
              break;
            }
            ++it;
          }

          // did we find our control?
          if (inf.isNotNull())
          {
            // currently we only accept messages from a button or controllist with a select action
            if (inf->canAcceptMessages(message.GetParam1()))
            {
              invokeCallback(new CallbackFunction<Window,AddonClass::Ref<Control> >(this,&Window::onControl,inf.get()));
              PulseActionEvent();

              // return true here as we are handling the event
              return true;
            }
          }
          // if we get here, we didn't add the action
        }
        break;
      }

      return ref(window)->OnMessage(message);
    }

    void Window::onControl(Control* action) { XBMC_TRACE; /* do nothing by default */ }
    void Window::onClick(int controlId) { XBMC_TRACE; /* do nothing by default */ }
    void Window::onDoubleClick(int controlId) { XBMC_TRACE; /* do nothing by default */ }
    void Window::onFocus(int controlId) { XBMC_TRACE; /* do nothing by default */ }
    void Window::onInit() { XBMC_TRACE; /* do nothing by default */ }

    void Window::show()
    {
      XBMC_TRACE;
      DelayedCallGuard dcguard(languageHook);
      popActiveWindowId();

      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_ACTIVATE_WINDOW, iWindowId, 0);
    }

    void Window::setFocus(Control* pControl)
    {
      XBMC_TRACE;
      if(pControl == NULL)
        throw WindowException("Object should be of type Control");

      CGUIMessage msg = CGUIMessage(GUI_MSG_SETFOCUS,pControl->iParentId, pControl->iControlId);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, pControl->iParentId);
    }

    void Window::setFocusId(int iControlId)
    {
      XBMC_TRACE;
      CGUIMessage msg = CGUIMessage(GUI_MSG_SETFOCUS,iWindowId,iControlId);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, iWindowId);
    }

    Control* Window::getFocus()
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);

      int iControlId = ref(window)->GetFocusedControlID();
      if(iControlId == -1)
        throw WindowException("No control in this window has focus");
      // Sine I'm already holding the lock theres no reason to give it to GetFocusedControlID
      return GetControlById(iControlId,NULL);
    }

    long Window::getFocusId()
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);
      int iControlId = ref(window)->GetFocusedControlID();
      if(iControlId == -1)
        throw WindowException("No control in this window has focus");
      return (long)iControlId;
    }

    void Window::removeControl(Control* pControl)
    {
      XBMC_TRACE;
      DelayedCallGuard dg(languageHook);
      doRemoveControl(pControl,&CServiceBroker::GetWinSystem()->GetGfxContext(),true);
    }

    void Window::doRemoveControl(Control* pControl, CCriticalSection* gcontext, bool wait)
    {
      XBMC_TRACE;
      // type checking, object should be of type Control
      if(pControl == NULL)
        throw WindowException("Object should be of type Control");

      {
        MaybeLock mlock(gcontext);
        if(!ref(window)->GetControl(pControl->iControlId))
          throw WindowException("Control does not exist in window");
      }

      CGUIMessage msg(GUI_MSG_REMOVE_CONTROL, 0, 0);
      msg.SetPointer(pControl->pGUIControl);
      CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, iWindowId, wait);

      // delete control from vecControls in window object
      std::vector<AddonClass::Ref<Control>>::iterator it = vecControls.begin();
      while (it != vecControls.end())
      {
        AddonClass::Ref<Control> control = (*it);
        if (control->iControlId == pControl->iControlId)
        {
          it = vecControls.erase(it);
        }
        else
        {
          ++it;
        }
      }

      // initialize control to zero
      pControl->pGUIControl = NULL;
      pControl->iControlId = 0;
      pControl->iParentId = 0;
    }

    void Window::removeControls(std::vector<Control*> pControls)
    {
      XBMC_TRACE;
      DelayedCallGuard dg(languageHook);
      int count = 1; int size = pControls.size();
      for (std::vector<Control*>::iterator iter = pControls.begin(); iter != pControls.end(); count++, ++iter)
        doRemoveControl(*iter,NULL, count == size);
    }

    long Window::getHeight()
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(), languageHook);
      RESOLUTION_INFO resInfo = ref(window)->GetCoordsRes();
      return resInfo.iHeight;
    }

    long Window::getWidth()
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(), languageHook);
      RESOLUTION_INFO resInfo = ref(window)->GetCoordsRes();
      return resInfo.iWidth;
    }

    void Window::setProperty(const char* key, const String& value)
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);
      std::string lowerKey = key;
      StringUtils::ToLower(lowerKey);

      ref(window)->SetProperty(lowerKey, value);
    }

    String Window::getProperty(const char* key)
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);
      std::string lowerKey = key;
      StringUtils::ToLower(lowerKey);
      std::string value = ref(window)->GetProperty(lowerKey).asString();
      return value;
    }

    void Window::clearProperty(const char* key)
    {
      XBMC_TRACE;
      if (!key) return;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);

      std::string lowerKey = key;
      StringUtils::ToLower(lowerKey);
      ref(window)->SetProperty(lowerKey, "");
    }

    void Window::clearProperties()
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);
      ref(window)->ClearProperties();
    }

    void Window::close()
    {
      XBMC_TRACE;
      bModal = false;

      if (!existingWindow)
        PulseActionEvent();

      {
        DelayedCallGuard dcguard(languageHook);
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_PREVIOUS_WINDOW, iOldWindowId, 0);
      }

      iOldWindowId = 0;
    }

    void Window::doModal()
    {
      XBMC_TRACE;
      if (!existingWindow)
      {
        bModal = true;

        if(iWindowId != ACTIVE_WINDOW)
          show();

        while (bModal && !g_application.m_bStop)
        {
//! @todo garbear added this code to the python window.cpp class and
//!  commented in XBPyThread.cpp. I'm not sure how to handle this
//! in this native implementation.
//          // Check if XBPyThread::stop() raised a SystemExit exception
//          if (PyThreadState_Get()->async_exc == PyExc_SystemExit)
//          {
//            CLog::Log(LOGDEBUG, "PYTHON: doModal() encountered a SystemExit exception, closing window and returning");
//            Window_Close(self, NULL);
//            break;
//          }
          languageHook->MakePendingCalls(); // MakePendingCalls

          bool stillWaiting;
          do
          {
            {
              DelayedCallGuard dcguard(languageHook);
              stillWaiting = WaitForActionEvent(100) ? false : true;
            }
            languageHook->MakePendingCalls();
          } while (stillWaiting);
        }
      }
    }

    void Window::addControl(Control* pControl)
    {
      XBMC_TRACE;
      DelayedCallGuard dg(languageHook);
      doAddControl(pControl,&CServiceBroker::GetWinSystem()->GetGfxContext(),true);
    }

    void Window::doAddControl(Control* pControl, CCriticalSection* gcontext, bool wait)
    {
      XBMC_TRACE;
      if(pControl == NULL)
        throw WindowException("NULL Control passed to WindowBase::addControl");

      if(pControl->iControlId != 0)
        throw WindowException("Control is already used");

      // lock kodi GUI before accessing data from it
      pControl->iParentId = iWindowId;

      {
        MaybeLock mlock(gcontext);
        // assign control id, if id is already in use, try next id
        do pControl->iControlId = ++iCurrentControlId;
        while (ref(window)->GetControl(pControl->iControlId));
      }

      pControl->Create();

      // set default navigation for control
      pControl->iControlUp = pControl->iControlId;
      pControl->iControlDown = pControl->iControlId;
      pControl->iControlLeft = pControl->iControlId;
      pControl->iControlRight = pControl->iControlId;

      pControl->pGUIControl->SetAction(ACTION_MOVE_UP,    CGUIAction(pControl->iControlUp));
      pControl->pGUIControl->SetAction(ACTION_MOVE_DOWN,  CGUIAction(pControl->iControlDown));
      pControl->pGUIControl->SetAction(ACTION_MOVE_LEFT,  CGUIAction(pControl->iControlLeft));
      pControl->pGUIControl->SetAction(ACTION_MOVE_RIGHT, CGUIAction(pControl->iControlRight));

      // add control to list and allocate resources for the control
      vecControls.emplace_back(pControl);
      pControl->pGUIControl->AllocResources();

      // This calls the CGUIWindow parent class to do the final add
      CGUIMessage msg(GUI_MSG_ADD_CONTROL, 0, 0);
      msg.SetPointer(pControl->pGUIControl);
      CServiceBroker::GetAppMessenger()->SendGUIMessage(msg, iWindowId, wait);
    }

    void Window::addControls(std::vector<Control*> pControls)
    {
      XBMC_TRACE;
      SingleLockWithDelayGuard gslock(CServiceBroker::GetWinSystem()->GetGfxContext(),languageHook);
      int count = 1; int size = pControls.size();
      for (std::vector<Control*>::iterator iter = pControls.begin(); iter != pControls.end(); count++, ++iter)
        doAddControl(*iter,NULL, count == size);
    }

    Control* Window::getControl(int iControlId)
    {
      XBMC_TRACE;
      DelayedCallGuard dg(languageHook);
      return GetControlById(iControlId,&CServiceBroker::GetWinSystem()->GetGfxContext());
    }

    void Action::setFromCAction(const CAction& action)
    {
      XBMC_TRACE;
      id = action.GetID();
      buttonCode = action.GetButtonCode();
      fAmount1 = action.GetAmount(0);
      fAmount2 = action.GetAmount(1);
      fRepeat = action.GetRepeat();
      strAction = action.GetName();
    }

  }
}

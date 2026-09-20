/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* AddonClass lifetime.

   Without this, SWIG emits a plain `delete` in the destructor wrapper, which
   bypasses AddonClass's reference count entirely: dropping a python object
   while Kodi still holds a reference is a use-after-free, and an object handed
   back from a getter is never freed at all.

   Every class wrapped by these modules derives from XBMCAddon::AddonClass
   (verified against the headers), so the features are declared globally rather
   than repeated per class. %feature("ref") is emitted wherever SWIG takes
   ownership of a pointer, %feature("unref") in the destructor wrapper.

   Registration with the language hook is not optional bookkeeping: it is what
   gates queued callback delivery through PythonCallbackHandler::isStateOk. */

%{
#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/Window.h"
#include "interfaces/legacy/Exception.h"
#include "interfaces/legacy/LanguageHook.h"
#include "interfaces/python/LanguageHook.h"
#include "interfaces/python/PythonToCppException.h"
#include "commons/Exception.h"
#include "utils/log.h"

namespace
{
[[maybe_unused]] inline void KodiSwig_acquire(XBMCAddon::AddonClass* c)
{
  if (!c)
    return;
  c->Acquire();
  XBMCAddon::AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook> lh =
      XBMCAddon::Python::PythonLanguageHook::GetIfExists(PyThreadState_Get()->interp);
  if (lh.isNotNull())
    lh->RegisterAddonClassInstance(c);
}

/* Windows must tear down before release: the shipped cleanForDealloc(Window*)
   calls dispose() then Release(). dispose() cannot run from the destructor. */
[[maybe_unused]] inline void KodiSwig_releaseWindow(XBMCAddon::xbmcgui::Window* c)
{
  if (!c)
    return;
  XBMCAddon::AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook> lh =
      XBMCAddon::AddonClass::Ref<XBMCAddon::AddonClass>(c->GetLanguageHook());
  if (lh.isNull())
    lh = XBMCAddon::Python::PythonLanguageHook::GetIfExists(PyThreadState_Get()->interp);
  if (lh.isNotNull())
    lh->UnregisterAddonClassInstance(c);
  c->dispose();
  c->Release();
}

[[maybe_unused]] inline void KodiSwig_release(XBMCAddon::AddonClass* c)
{
  if (!c)
    return;
  XBMCAddon::AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook> lh =
      XBMCAddon::AddonClass::Ref<XBMCAddon::AddonClass>(c->GetLanguageHook());
  if (lh.isNull())
    lh = XBMCAddon::Python::PythonLanguageHook::GetIfExists(PyThreadState_Get()->interp);
  if (lh.isNotNull())
    lh->UnregisterAddonClassInstance(c);
  c->Release();
}
} // namespace
%}

%feature("ref")   "KodiSwig_acquire($this);"
%feature("unref") "KodiSwig_release($this);"

/* Every wrapped call runs under SetLanguageHookGuard. AddonClass constructors
   read the hook from that TLS slot, so without it xbmcaddon.Addon() cannot
   resolve its own id and throws "No valid addon id could be obtained"; the
   no-argument DelayedCallGuard in the legacy free functions reads the same slot.

   The catch clauses are equally load-bearing: a Kodi API call that throws
   XbmcCommons::Exception would otherwise unwind out of a CPython wrapper as an
   uncaught C++ exception rather than becoming a python exception. */
%exception {
  try {
    XBMCAddon::SetLanguageHookGuard slhg(
        XBMCAddon::Python::PythonLanguageHook::GetIfExists(PyThreadState_Get()->interp).get());
    $action
  } catch (const XBMCAddon::WrongTypeException& e) {
    CLog::Log(LOGERROR, "EXCEPTION: {}", e.GetExMessage());
    SWIG_exception_fail(SWIG_TypeError, e.GetExMessage());
  } catch (const XbmcCommons::Exception& e) {
    CLog::Log(LOGERROR, "EXCEPTION: {}", e.GetExMessage());
    SWIG_exception_fail(SWIG_RuntimeError, e.GetExMessage());
  } catch (...) {
    CLog::Log(LOGERROR, "EXCEPTION: Unknown exception thrown from the call \"$symname\"");
    SWIG_exception_fail(SWIG_RuntimeError, "Unknown exception thrown from the call \"$symname\"");
  }
}

/* ---- ownership of returned objects ---------------------------------- */
/* --- callee allocates: caller owns --- */
%newobject XBMCAddon::xbmcgui::ListItem::getVideoInfoTag;
%newobject XBMCAddon::xbmcgui::ListItem::getMusicInfoTag;
%newobject XBMCAddon::xbmcgui::ListItem::getPictureInfoTag;
%newobject XBMCAddon::xbmcgui::ListItem::getGameInfoTag;
%newobject XBMCAddon::xbmcgui::WindowXML::getListItem;

%newobject XBMCAddon::xbmc::Player::getVideoInfoTag;
%newobject XBMCAddon::xbmc::Player::getMusicInfoTag;
%newobject XBMCAddon::xbmc::Player::getRadioRDSInfoTag;
%newobject XBMCAddon::xbmc::Player::getGameInfoTag;
%newobject XBMCAddon::xbmc::Player::getPlayingItem;

%newobject XBMCAddon::xbmcaddon::Addon::getSettings;

%newobject XBMCAddon::xbmcwsgi::WsgiInputStream::begin;
%newobject XBMCAddon::xbmcwsgi::WsgiInputStream::end;

/* --- borrowed: %newobject here would be a double free ---
   Window::getControl        - the control is held by Window::vecControls, a
                               vector<Ref<Control>>, on both the cache-hit and
                               the freshly-allocated path (Window.cpp)
   ControlList::getSelectedItem, ControlList::getListItem
                             - return pListItem.get() from a Ref member
   ControlList::getSpinControl - returns a member pointer                    */

/* ---- director callbacks --------------------------------------------- */
/* A python exception raised inside a director callback must not escape as
   Swig::DirectorMethodException: nothing in Kodi catches that, so it would
   reach std::terminate and abort the application. */
%{
#include "interfaces/python/PythonToCppException.h"
%}

%feature("director:except") %{
  if ($error != NULL) {
    throw PythonBindings::PythonToCppException();
  }
%}

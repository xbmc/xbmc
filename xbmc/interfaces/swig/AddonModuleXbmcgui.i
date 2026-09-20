/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%module(directors="1") xbmcgui

%{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "interfaces/legacy/Dialog.h"
#include "interfaces/legacy/ModuleXbmcgui.h"
#include "interfaces/legacy/Control.h"
#include "interfaces/legacy/Window.h"
#include "interfaces/legacy/WindowDialog.h"
#include "interfaces/legacy/WindowXML.h"
#include "input/actions/ActionIDs.h"
#include "input/keymaps/keyboard/KeyIDs.h"

using namespace XBMCAddon;
using namespace xbmcgui;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

%include "kodi_common.i"

// Window.getControl() and getFocus() are declared to return the base Control*,
// but the object is a concrete subclass. Without RTTI dispatch python gets a
// base Control and every derived method raises AttributeError.
KODI_DYNAMIC_RETURN(XBMCAddon::xbmcgui::Control)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlButton)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlEdit)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlFadeLabel)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlGroup)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlImage)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlLabel)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlList)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlProgress)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlRadioButton)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlSlider)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlSpin)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlTextBox)
KODI_REGISTER_TYPE(XBMCAddon::xbmcgui::ControlVideoWindow)

// Window subclasses dispose() before Release; see kodi_addonclass.i
%feature("unref") XBMCAddon::xbmcgui::Window          "KodiSwig_releaseWindow($this);"
%feature("unref") XBMCAddon::xbmcgui::WindowDialog    "KodiSwig_releaseWindow($this);"
%feature("unref") XBMCAddon::xbmcgui::WindowXML       "KodiSwig_releaseWindow($this);"
%feature("unref") XBMCAddon::xbmcgui::WindowXMLDialog "KodiSwig_releaseWindow($this);"

// construction in tp_new; see kodi_construct.i
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ListItem)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, Action)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlButton)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlEdit)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlFadeLabel)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlGroup)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlImage)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlLabel)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlList)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlProgress)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlRadioButton)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlSlider)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlTextBox)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, ControlVideoWindow)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, Dialog)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, DialogProgress)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, DialogProgressBG)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, Window)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, WindowDialog)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, WindowXML)
KODI_CONSTRUCT(XBMCAddon::xbmcgui, WindowXMLDialog)

namespace XBMCAddon { namespace xbmcgui { class ListItem; class Control; } }

// ListItem.h sits inside namespace XBMCAddon and spells these partially
// qualified as xbmc::InfoTagVideo. Without the fully qualified declaration
// SWIG mints "xbmc::InfoTagVideo", which does not match the name the xbmc
// module registers, and cross-module lookup silently yields an opaque pointer.
namespace XBMCAddon { namespace xbmc {
  class InfoTagVideo; class InfoTagMusic; class InfoTagPicture; class InfoTagGame;
} }
%traits_swigtype(XBMCAddon::xbmcgui::ListItem);
%fragment(SWIG_Traits_frag(XBMCAddon::xbmcgui::ListItem));
%traits_swigtype(XBMCAddon::xbmcgui::Control);
%fragment(SWIG_Traits_frag(XBMCAddon::xbmcgui::Control));

/* one line per shape crossing the boundary in this module */
%template() std::vector<int>;
%template() std::vector<std::string>;
%template() std::vector<XBMCAddon::xbmcgui::Control*>;
%template() std::vector<XBMCAddon::xbmcgui::ListItem const*>;
%template() XBMCAddon::Tuple<std::string, std::string>;
%template() std::vector<XBMCAddon::Tuple<std::string, std::string> >;
%template() XBMCAddon::Alternative<std::string, std::vector<std::string> >;
%template() XBMCAddon::Alternative<std::string, XBMCAddon::xbmcgui::ListItem const*>;
%template() std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::xbmcgui::ListItem const*> >;
%template() XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> >;
%template() std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> > >;
%template() XBMCAddon::Alternative<std::string, std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> > > >;
%template() XBMCAddon::Dictionary<std::string>;
%template() std::vector<XBMCAddon::Dictionary<std::string> >;
%template() XBMCAddon::Dictionary<XBMCAddon::Alternative<std::string, std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> > > > >;

/* -builtin routes the comparison operators through tp_richcompare, whose
   dispatcher calls a two-argument wrapper. */
%feature("kwargs", "0") XBMCAddon::xbmcgui::Control::operator==;
%feature("kwargs", "0") XBMCAddon::xbmcgui::Control::operator>;
%feature("kwargs", "0") XBMCAddon::xbmcgui::Control::operator<;

/* multiselect hands back ownership of a list it built; python gets the list. */
%typemap(out) std::unique_ptr<std::vector<int> >
{
  $result = $1 ? swig::from(*$1) : SWIG_Py_Void();
}

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

%include "interfaces/legacy/ModuleXbmcgui.h"

%include "interfaces/legacy/Exception.h"

%include "interfaces/legacy/Dictionary.h"

%include "interfaces/legacy/ListItem.h"

%template() std::vector<XBMCAddon::xbmcgui::ListItem*>;
%include "interfaces/legacy/Control.h"

%kodi_defaulted_container(%arg(std::vector<XBMCAddon::xbmcgui::ListItem const *>))
%kodi_defaulted_container(%arg(std::vector<int>))
%include "interfaces/legacy/Dialog.h"

%feature("director") Window;
%feature("director") WindowDialog;
%feature("director") WindowXML;
%feature("director") WindowXMLDialog;

// scripts compare Actions to ints: a non-Action operand compares against the id.
%feature("python:slot", "tp_richcompare", functype="richcmpfunc") XBMCAddon::xbmcgui::Action::__eq__;
%feature("kwargs", "0") XBMCAddon::xbmcgui::Action::__eq__;
%extend XBMCAddon::xbmcgui::Action {
  PyObject* __eq__(PyObject* obj2) {
    void* argp = 0;
    if (SWIG_IsOK(SWIG_ConvertPtr(obj2, &argp, SWIGTYPE_p_XBMCAddon__xbmcgui__Action, 0)) && argp)
    {
      XBMCAddon::xbmcgui::Action* a2 = reinterpret_cast<XBMCAddon::xbmcgui::Action*>(argp);
      if ($self->id == a2->id && $self->buttonCode == a2->buttonCode &&
          $self->fAmount1 == a2->fAmount1 && $self->fAmount2 == a2->fAmount2 &&
          $self->fRepeat == a2->fRepeat && $self->strAction == a2->strAction)
        Py_RETURN_TRUE;
      Py_RETURN_FALSE;
    }
    PyObject* o1 = PyLong_FromLong($self->id);
    PyObject* r = PyObject_RichCompare(o1, obj2, Py_EQ);
    Py_DECREF(o1);
    return r;
  }
}

%include "interfaces/legacy/Window.h"
%include "interfaces/legacy/WindowDialog.h"

%include "interfaces/legacy/WindowXML.h"

%include "input/actions/ActionIDs.h"
%include "input/keymaps/keyboard/KeyIDs.h"

// for ContextItemAddonInvoker; %wrapper has C linkage; owned!=0: dealloc Releases the caller's Acquire
%wrapper %{
extern "C" PyObject* KodiSwig_wrapListItem(XBMCAddon::xbmcgui::ListItem* obj, int owned)
{
  return SWIG_Python_NewPointerObj(NULL, obj, SWIGTYPE_p_XBMCAddon__xbmcgui__ListItem,
                                   owned ? SWIG_POINTER_OWN : 0);
}
%}

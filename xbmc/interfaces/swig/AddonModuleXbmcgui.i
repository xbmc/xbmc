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

%module(directors="1") xbmcgui

%{
#include "interfaces/legacy/Dialog.h"
#include "interfaces/legacy/ModuleXbmcgui.h"
#include "interfaces/legacy/Control.h"
#include "interfaces/legacy/Window.h"
#include "interfaces/legacy/WindowDialog.h"
#include "interfaces/legacy/Dialog.h"
#include "interfaces/legacy/WindowXML.h"
#include "input/Key.h"

using namespace XBMCAddon;
using namespace xbmcgui;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

// This is all about warning suppression. It's OK that these base classes are 
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmcgui "AddonClass,AddonCallback"

%feature("knownapitypes") XBMCAddon::xbmcgui "XBMCAddon::xbmc::InfoTagVideo,xbmc::InfoTagMusic"

%include "interfaces/legacy/swighelper.h"
%include "interfaces/legacy/AddonString.h"

%include "interfaces/legacy/ModuleXbmcgui.h"

%include "interfaces/legacy/Exception.h"

%include "interfaces/legacy/Dictionary.h"

%include "interfaces/legacy/ListItem.h"

%feature("python:coerceToUnicode") XBMCAddon::xbmcgui::ControlButton::getLabel "true"
%feature("python:coerceToUnicode") XBMCAddon::xbmcgui::ControlButton::getLabel2 "true"
%include "interfaces/legacy/Control.h"

%include "interfaces/legacy/Dialog.h"

%feature("python:nokwds") XBMCAddon::xbmcgui::Dialog::Dialog "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::Window::Window "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::WindowXML::WindowXML "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::WindowXMLDialog::WindowXMLDialog "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::WindowDialog::WindowDialog "true"

%feature("director") Window;
%feature("director") WindowDialog;
%feature("director") WindowXML;
%feature("director") WindowXMLDialog;

// This is such a damn hack it makes me nauseous
%feature("python:rcmp") XBMCAddon::xbmcgui::Action
  { XBMC_TRACE;
    if (method == Py_EQ)
    {
      XBMCAddon::xbmcgui::Action* a1 = (Action*)retrieveApiInstance(obj1,&TyXBMCAddon_xbmcgui_Action_Type,"rcmp","XBMCAddon::xbmcgui::Action");
      if (PyObject_TypeCheck(obj2, &(TyXBMCAddon_xbmcgui_Action_Type.pythonType)))
      {
        // both are Action objects
        XBMCAddon::xbmcgui::Action* a2 = (Action*)retrieveApiInstance(obj2,&TyXBMCAddon_xbmcgui_Action_Type,"rcmp","XBMCAddon::xbmcgui::Action");

        if (a1->id == a2->id &&
            a1->buttonCode == a2->buttonCode &&
            a1->fAmount1 == a2->fAmount1 &&
            a1->fAmount2 == a2->fAmount2 &&
            a1->fRepeat == a2->fRepeat &&
            a1->strAction == a2->strAction)
        {
          Py_RETURN_TRUE;
        }
        else
        {
          Py_RETURN_FALSE;
        }
      }
      else
      {
        // for backwards compatability in python scripts
        PyObject* o1 = PyLong_FromLong(a1->id);
        return PyObject_RichCompare(o1, obj2, method);
      }
    }
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }


%include "interfaces/legacy/Window.h"
%include "interfaces/legacy/WindowDialog.h"
%include "interfaces/legacy/Dialog.h"

%include "interfaces/legacy/WindowXML.h"

%include "input/Key.h"

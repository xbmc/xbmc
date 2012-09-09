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

%module(directors="1") xbmcgui

%{
#include "interfaces/legacy/Dialog.h"
#include "interfaces/legacy/ModuleXbmcgui.h"
#include "interfaces/legacy/Control.h"
#include "interfaces/legacy/Window.h"
#include "interfaces/legacy/WindowDialog.h"
#include "interfaces/legacy/Dialog.h"
#include "interfaces/legacy/WindowXML.h"

using namespace XBMCAddon;
using namespace xbmcgui;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

// This is all about warning suppression. It's OK that these base classes are 
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmcgui "AddonClass,AddonCallback"

%include "interfaces/legacy/swighelper.h"

%include "interfaces/legacy/ModuleXbmcgui.h"

%feature("ref") ListItem "${ths}->Acquire();"
%feature("unref") ListItem "${ths}->Release();"

%include "interfaces/legacy/Exception.h"

%include "interfaces/legacy/ListItem.h"


%feature("python:method:addItem") ControlList
{
  TRACE;

  static const char *keywords[] = {
    "item",
    NULL};

  String  item;
  PyObject* pyitem = NULL;
  if (!PyArg_ParseTupleAndKeywords(
                                   args,
                                   kwds,
                                   (char*)"O",
                                   (char**)keywords,
                                   &pyitem
                                   ))
    return NULL;

  const char* callName = "addItem";
  try
  {
    XBMCAddon::xbmcgui::ControlList* apiobj = ((XBMCAddon::xbmcgui::ControlList*)retrieveApiInstance((PyObject*)self,
                                           &PyXBMCAddon_xbmcgui_ControlList_Type,"addItem","XBMCAddon::xbmcgui::Control"));

    if (PyUnicode_Check(pyitem) || PyString_Check(pyitem))
    {
      callName = "addItemStream";
      PyXBMCGetUnicodeString(item,pyitem,false,"item","addItem"); 
      apiobj->addItemStream(item);
    }
    else
    {
      callName = "addListItem";
      // assume it's a ListItem. retrieveApiInstance will throw an exception if it's not
      XBMCAddon::xbmcgui::ListItem* listItem = ((XBMCAddon::xbmcgui::ListItem*)retrieveApiInstance((PyObject*)pyitem,
                                              &PyXBMCAddon_xbmcgui_ListItem_Type,"addItem","XBMCAddon::xbmcgui::ListItem"));
      apiobj->addListItem(listItem);
    }
  }
  catch (const XbmcCommons::Exception& e)
  { 
    CLog::Log(LOGERROR,"Leaving Python method 'XBMCAddon_xbmcgui_Control_addItemStream'. Exception from call to '%s' '%s' ... returning NULL", callName,e.GetMessage());
    PyErr_SetString(PyExc_RuntimeError, e.GetMessage()); 
    return NULL; 
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"Unknown exception thrown from the call '%s'",callName);
    PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call 'addItem'"); 
    return NULL; 
  }

  PyObject* result;

  // transform the result
  Py_INCREF(Py_None);
  result = Py_None;

  return result; 
 } 

%feature("python:coerceToUnicode") XBMCAddon::xbmcgui::ControlButton::getLabel "true"
%feature("python:coerceToUnicode") XBMCAddon::xbmcgui::ControlButton::getLabel2 "true"
%include "interfaces/legacy/Control.h"

%include "interfaces/legacy/Dialog.h"

%feature("director") Window;
%feature("ref") Window "${ths}->Acquire();"
%feature("unref") Window "${ths}->Release();"
%feature("director") WindowDialog;
%feature("director") WindowXML;
%feature("director") WindowXMLDialog;

%include "interfaces/legacy/Window.h"
%include "interfaces/legacy/WindowDialog.h"
%include "interfaces/legacy/Dialog.h"
%include "interfaces/legacy/WindowXML.h"


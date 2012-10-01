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

/**
 * This file contains the two python methods, addItem and addItems for controls
 */
#pragma once

%feature("python:method:addItem") Control
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
    XBMCAddon::xbmcgui::Control* tmp = ((XBMCAddon::xbmcgui::Control*)retrieveApiInstance((PyObject*)self,
                                        &PyXBMCAddon_xbmcgui_Control_Type,"addItem","XBMCAddon::xbmcgui::Control"));

    XBMCAddon::xbmcgui::ControlList* apiobj = dynamic_cast<XBMCAddon::xbmcgui::ControlList*>(tmp);

    if (apiobj == NULL)
      throw WrongTypeException("Incorrect type passed to '%s', was expecting a '%s'.",callName,"XBMCAddon::xbmcgui::Control");


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
    CLog::Log(LOGERROR,"EXCEPTION: from call to '%s' '%s' ... returning NULL", callName,e.GetMessage());
    PyErr_SetString(PyExc_RuntimeError, e.GetMessage()); 
    return NULL; 
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call '%s'",callName);
    PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call 'addItem'"); 
    return NULL; 
  }

  PyObject* result;

  // transform the result
  Py_INCREF(Py_None);
  result = Py_None;

  return result; 
} 

%feature("python:method:addItems") Control
{
  TRACE;

  static const char *keywords[] = {
    "items",
    NULL};

  String  items;
  PyObject* pyitems = NULL;
  if (!PyArg_ParseTupleAndKeywords(
                                   args,
                                   kwds,
                                   (char*)"O",
                                   (char**)keywords,
                                   &pyitems
                                   ))
    return NULL;

  const char* callName = "addItems";
  try
  {
    XBMCAddon::xbmcgui::Control* tmp = ((XBMCAddon::xbmcgui::Control*)retrieveApiInstance((PyObject*)self,
                                        &PyXBMCAddon_xbmcgui_Control_Type,"addItem","XBMCAddon::xbmcgui::Control"));

    XBMCAddon::xbmcgui::ControlList* apiobj = dynamic_cast<XBMCAddon::xbmcgui::ControlList*>(tmp);

    if (apiobj == NULL)
      throw WrongTypeException("Incorrect type passed to '%s', was expecting a '%s'.",callName,"XBMCAddon::xbmcgui::Control");

    CGUIListItemPtr items(new CFileItemList());
    int listSize = PyList_Size(pyitems);
    for (int index = 0; index < listSize; index++)
    {
      PyObject *pyitem = PyList_GetItem(pyitems, index);

      if (PyUnicode_Check(pyitem) || PyString_Check(pyitem))
      {
        callName = "addItemStream";
        String  item;
        PyXBMCGetUnicodeString(item,pyitem,false,"item","addItem"); 
        apiobj->addItemStream(item,false);
      }
      else
      {
        callName = "addListItem";
        // assume it's a ListItem. retrieveApiInstance will throw an exception if it's not
        XBMCAddon::xbmcgui::ListItem* listItem = ((XBMCAddon::xbmcgui::ListItem*)retrieveApiInstance((PyObject*)pyitem,
                                                &PyXBMCAddon_xbmcgui_ListItem_Type,"addItem","XBMCAddon::xbmcgui::ListItem"));
        apiobj->addListItem(listItem,false);
      }
    }

    apiobj->sendLabelBind(listSize);
  }
  catch (const XbmcCommons::Exception& e)
  { 
    CLog::Log(LOGERROR,"EXCEPTION: from call to '%s' '%s' ... returning NULL", callName,e.GetMessage());
    PyErr_SetString(PyExc_RuntimeError, e.GetMessage()); 
    return NULL; 
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"EXCEPTION: Unknown exception thrown from the call '%s'",callName);
    PyErr_SetString(PyExc_RuntimeError, "Unknown exception thrown from the call 'addItem'"); 
    return NULL; 
  }

  PyObject* result;

  // transform the result
  Py_INCREF(Py_None);
  result = Py_None;

  return result; 
} 


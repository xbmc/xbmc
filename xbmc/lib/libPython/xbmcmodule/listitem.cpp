/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "lib/libPython/Python/Include/Python.h"
#include "../XBPythonDll.h"
#include "listitem.h"
#include "pyutil.h"
#include "VideoInfoTag.h"
#include "PictureInfoTag.h"
#include "MusicInfoTag.h"
#include "FileItem.h"

using namespace std;

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* ListItem_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    ListItem *self;
    static const char *keywords[] = { "label", "label2",
      "iconImage", "thumbnailImage", "path", NULL };

    PyObject* label = NULL;
    PyObject* label2 = NULL;
    char* cIconImage = NULL;
    char* cThumbnailImage = NULL;
    PyObject* path = NULL;

    // allocate new object
    self = (ListItem*)type->tp_alloc(type, 0);
    if (!self)
      return NULL;
    
    self->item.reset();

    // parse user input
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"|OOssO",
      (char**)keywords,
      &label,
      &label2,
      &cIconImage,
      &cThumbnailImage,
      &path))
    {
      Py_DECREF( self );
      return NULL;
    }

    // create CFileItem
    self->item.reset(new CFileItem());
    if (!self->item)
    {
      Py_DECREF( self );
      return NULL;
    }
    CStdString utf8String;
    if (label && PyGetUnicodeString(utf8String, label, 1))
    {
      self->item->SetLabel( utf8String );
    }
    if (label2 && PyGetUnicodeString(utf8String, label2, 1))
    {
      self->item->SetLabel2( utf8String );
    }
    if (cIconImage)
    {
      self->item->SetIconImage( cIconImage );
    }
    if (cThumbnailImage)
    {
      self->item->SetThumbnailImage( cThumbnailImage );
    }
    if (path && PyGetUnicodeString(utf8String, path, 1))
    {
      self->item->m_strPath = utf8String;
    }
    return (PyObject*)self;
  }

  /*
   * allocate a new listitem. Used for c++ and not the python user
   * returns a new reference
   */
  ListItem* ListItem_FromString(string strLabel)
  {
    ListItem* self = (ListItem*)ListItem_Type.tp_alloc(&ListItem_Type, 0);
    if (!self) return NULL;

    self->item.reset(new CFileItem(strLabel));
    if (!self->item)
    {
      Py_DECREF( self );
      return NULL;
    }

    return self;
  }

  void ListItem_Dealloc(ListItem* self)
  {
    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(getLabel__doc__,
    "getLabel() -- Returns the listitem label.\n"
    "\n"
    "example:\n"
    "  - label = self.list.getSelectedItem().getLabel()\n");

  PyObject* ListItem_GetLabel(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyGUILock();
    const char *cLabel = self->item->GetLabel().c_str();
    PyGUIUnlock();

    return Py_BuildValue((char*)"s", cLabel);
  }

  PyDoc_STRVAR(getLabel2__doc__,
    "getLabel2() -- Returns the listitem's second label.\n"
    "\n"
    "example:\n"
    "  - label2 = self.list.getSelectedItem().getLabel2()\n");

  PyObject* ListItem_GetLabel2(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyGUILock();
    const char *cLabel = self->item->GetLabel2().c_str();
    PyGUIUnlock();

    return Py_BuildValue((char*)"s", cLabel);
  }

  PyDoc_STRVAR(setLabel__doc__,
    "setLabel(label) -- Sets the listitem's label.\n"
    "\n"
    "label          : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setLabel('Casino Royale')\n");

  PyObject* ListItem_SetLabel(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;
    PyObject* unicodeLine = NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;
    // set label
    PyGUILock();
    self->item->SetLabel(utf8Line);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setLabel2__doc__,
    "setLabel2(label2) -- Sets the listitem's second label.\n"
    "\n"
    "label2         : string or unicode - text string.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setLabel2('[pg-13]')\n");
  
  PyObject* ListItem_SetLabel2(ListItem *self, PyObject *args)
  {
    PyObject* unicodeLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &unicodeLine)) return NULL;

    string utf8Line;
    if (unicodeLine && !PyGetUnicodeString(utf8Line, unicodeLine, 1))
      return NULL;
    // set label
    PyGUILock();
    self->item->SetLabel2(utf8Line);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setIconImage__doc__,
    "setIconImage(icon) -- Sets the listitem's icon image.\n"
    "\n"
    "icon            : string - image filename.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setIconImage('emailread.png')\n");

  PyObject* ListItem_SetIconImage(ListItem *self, PyObject *args)
  {
    char *cLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, (char*)"s", &cLine)) return NULL;

    // set label
    PyGUILock();
    self->item->SetIconImage(cLine ? cLine : "");
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setThumbnailImage__doc__,
    "setThumbnailImage(thumb) -- Sets the listitem's thumbnail image.\n"
    "\n"
    "thumb           : string - image filename.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setThumbnailImage('emailread.png')\n");

  PyObject* ListItem_SetThumbnailImage(ListItem *self, PyObject *args)
  {
    char *cLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, (char*)"s", &cLine)) return NULL;

    // set label
    PyGUILock();
    self->item->SetThumbnailImage(cLine ? cLine : "");
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(select__doc__,
    "select(selected) -- Sets the listitem's selected status.\n"
    "\n"
    "selected        : bool - True=selected/False=not selected\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().select(True)\n");

  PyObject* ListItem_Select(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    char bOnOff = false;
    if (!PyArg_ParseTuple(args, (char*)"b", &bOnOff)) return NULL;

    PyGUILock();
    self->item->Select((bool)bOnOff);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }
  
  PyDoc_STRVAR(isSelected__doc__,
    "isSelected() -- Returns the listitem's selected status.\n"
    "\n"
    "example:\n"
    "  - is = self.list.getSelectedItem().isSelected()\n");

  PyObject* ListItem_IsSelected(ListItem *self, PyObject *args)
  {
    if (!self->item) return NULL;

    PyGUILock();
    bool bOnOff = self->item->IsSelected();
    PyGUIUnlock();

    return Py_BuildValue((char*)"b", bOnOff);
  }

  PyDoc_STRVAR(setInfo__doc__,
    "setInfo(type, infoLabels) -- Sets the listitem's infoLabels.\n"
    "\n"
    "type           : string - type of media(video/music/pictures).\n"
    "infoLabels     : dictionary - pairs of { label: value }.\n"
    "\n"
    "*Note, To set pictures exif info, prepend 'exif:' to the label. Exif values must be passed\n"
    "       as strings, separate value pairs with a comma. (eg. {'exif:resolution': '720,480'}\n"
    "       See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings.\n"
    "\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setInfo('video', { 'Genre': 'Comedy' })\n");

  PyObject* ListItem_SetInfo(ListItem *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "type", "infoLabels", NULL };
    char *cType = NULL;
    PyObject *pInfoLabels = NULL;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"sO",
      (char**)keywords,
      &cType,
      &pInfoLabels))
    {
      return NULL;
    }
    if (!PyObject_TypeCheck(pInfoLabels, &PyDict_Type))
    {
      PyErr_SetString(PyExc_TypeError, "infoLabels object should be of type Dict");
      return NULL;
    }
    if (PyDict_Size(pInfoLabels) == 0)
    {
      PyErr_SetString(PyExc_ValueError, "Empty InfoLabel dictionary");
      return NULL;
    }

    PyObject *key, *value;
    int pos = 0;

    PyGUILock();

    CStdString tmp;
    while (PyDict_Next(pInfoLabels, &pos, &key, &value)) {
      if (strcmpi(cType, "video") == 0)
      {
        // TODO: add the rest of the infolabels
        if (strcmpi(PyString_AsString(key), "year") == 0)
          self->item->GetVideoInfoTag()->m_iYear = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "episode") == 0)
          self->item->GetVideoInfoTag()->m_iEpisode = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "season") == 0)
          self->item->GetVideoInfoTag()->m_iSeason = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "count") == 0)
          self->item->m_iprogramCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "rating") == 0)
          self->item->GetVideoInfoTag()->m_fRating = (float)PyFloat_AsDouble(value);
        else if (strcmpi(PyString_AsString(key), "size") == 0)
          self->item->m_dwSize = (__int64)PyLong_AsLongLong(value);
        else if (strcmpi(PyString_AsString(key), "watched") == 0) // backward compat - do we need it?
          self->item->GetVideoInfoTag()->m_playCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "playcount") == 0)
          self->item->GetVideoInfoTag()->m_playCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "overlay") == 0)
          // TODO: Add a check for a valid overlay value
          self->item->SetOverlayImage((CGUIListItem::GUIIconOverlay)PyInt_AsLong(value));
        else if (strcmpi(PyString_AsString(key), "cast") == 0 || strcmpi(PyString_AsString(key), "castandrole") == 0)
        {
          if (!PyObject_TypeCheck(value, &PyList_Type)) continue;
          self->item->GetVideoInfoTag()->m_cast.clear();
          for (int i = 0; i < PyList_Size(value); i++)
          {
            PyObject *pTuple = NULL;
            pTuple = PyList_GetItem(value, i);
            if (pTuple == NULL) continue;
            PyObject *pActor = NULL;
            PyObject *pRole = NULL;
            if (PyObject_TypeCheck(pTuple, &PyTuple_Type))
            {
              if (!PyArg_ParseTuple(pTuple, (char*)"O|O", &pActor, &pRole)) continue;
            }
            else
              pActor = pTuple;
            SActorInfo info;
            if (!PyGetUnicodeString(info.strName, pActor, 1)) continue;
            if (pRole != NULL)
              PyGetUnicodeString(info.strRole, pRole, 1);
            self->item->GetVideoInfoTag()->m_cast.push_back(info);
          }
        }
        else
        {
          if (!PyGetUnicodeString(tmp, value, 1)) continue;
          if (strcmpi(PyString_AsString(key), "genre") == 0)
            self->item->GetVideoInfoTag()->m_strGenre = tmp;
          else if (strcmpi(PyString_AsString(key), "director") == 0)
            self->item->GetVideoInfoTag()->m_strDirector = tmp;
          else if (strcmpi(PyString_AsString(key), "mpaa") == 0)
            self->item->GetVideoInfoTag()->m_strMPAARating = tmp;
          else if (strcmpi(PyString_AsString(key), "plot") == 0)
            self->item->GetVideoInfoTag()->m_strPlot = tmp;
          else if (strcmpi(PyString_AsString(key), "plotoutline") == 0)
            self->item->GetVideoInfoTag()->m_strPlotOutline = tmp;
          else if (strcmpi(PyString_AsString(key), "title") == 0)
            self->item->GetVideoInfoTag()->m_strTitle = tmp;
          else if (strcmpi(PyString_AsString(key), "duration") == 0)
            self->item->GetVideoInfoTag()->m_strRuntime = tmp;
          else if (strcmpi(PyString_AsString(key), "studio") == 0)
            self->item->GetVideoInfoTag()->m_strStudio = tmp;
          else if (strcmpi(PyString_AsString(key), "tagline") == 0)
            self->item->GetVideoInfoTag()->m_strTagLine = tmp;
          else if (strcmpi(PyString_AsString(key), "writer") == 0)
            self->item->GetVideoInfoTag()->m_strWritingCredits = tmp;
          else if (strcmpi(PyString_AsString(key), "tvshowtitle") == 0)
            self->item->GetVideoInfoTag()->m_strShowTitle = tmp;
          else if (strcmpi(PyString_AsString(key), "premiered") == 0)
            self->item->GetVideoInfoTag()->m_strPremiered = tmp;
          else if (strcmpi(PyString_AsString(key), "votes") == 0)
            self->item->GetVideoInfoTag()->m_strVotes = tmp;
          else if (strcmpi(PyString_AsString(key), "trailer") == 0)
            self->item->GetVideoInfoTag()->m_strTrailer = tmp;
          else if (strcmpi(PyString_AsString(key), "date") == 0)
          {
            if (strlen(tmp) == 10)
              self->item->m_dateTime.SetDate(atoi(tmp.Right(4)), atoi(tmp.Mid(3,4)), atoi(tmp.Left(2)));
          }
        }
      }
      else if (strcmpi(cType, "music") == 0)
      {
        // TODO: add the rest of the infolabels
        if (strcmpi(PyString_AsString(key), "tracknumber") == 0)
          self->item->GetMusicInfoTag()->SetTrackNumber(PyInt_AsLong(value));
        else if (strcmpi(PyString_AsString(key), "count") == 0)
          self->item->m_iprogramCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "size") == 0)
          self->item->m_dwSize = (__int64)PyLong_AsLongLong(value);
        else if (strcmpi(PyString_AsString(key), "duration") == 0)
          self->item->GetMusicInfoTag()->SetDuration(PyInt_AsLong(value));
        else
        {
          if (!PyGetUnicodeString(tmp, value, 1)) continue;
          if (strcmpi(PyString_AsString(key), "genre") == 0)
            self->item->GetMusicInfoTag()->SetGenre(tmp);
          else if (strcmpi(PyString_AsString(key), "album") == 0)
            self->item->GetMusicInfoTag()->SetAlbum(tmp);
          else if (strcmpi(PyString_AsString(key), "artist") == 0)
            self->item->GetMusicInfoTag()->SetArtist(tmp);
          else if (strcmpi(PyString_AsString(key), "title") == 0)
            self->item->GetMusicInfoTag()->SetTitle(tmp);
          else if (strcmpi(PyString_AsString(key), "lyrics") == 0)
            self->item->SetProperty("lyrics", tmp);
          else if (strcmpi(PyString_AsString(key), "date") == 0)
          {
            if (strlen(tmp) == 10)
              self->item->m_dateTime.SetDate(atoi(tmp.Right(4)), atoi(tmp.Mid(3,4)), atoi(tmp.Left(2)));
          }
        }
        self->item->GetMusicInfoTag()->SetLoaded(true);
      }
      else if (strcmpi(cType, "pictures") == 0)
      {
        if (strcmpi(PyString_AsString(key), "count") == 0)
          self->item->m_iprogramCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "size") == 0)
          self->item->m_dwSize = (__int64)PyLong_AsLongLong(value);
        else
        {
          if (!PyGetUnicodeString(tmp, value, 1)) continue;
          if (strcmpi(PyString_AsString(key), "title") == 0)
            self->item->m_strTitle = tmp;
          else if (strcmpi(PyString_AsString(key), "picturepath") == 0)
            self->item->m_strPath = tmp;
          else if (strcmpi(PyString_AsString(key), "date") == 0)
          {
            if (strlen(tmp) == 10)
              self->item->m_dateTime.SetDate(atoi(tmp.Right(4)), atoi(tmp.Mid(3,4)), atoi(tmp.Left(2)));
          }
          else
          {
            CStdString exifkey = PyString_AsString(key);
            if (!exifkey.Left(5).Equals("exif:") || exifkey.length() < 6) continue;
            int info = CPictureInfoTag::TranslateString(exifkey.Mid(5));
            self->item->GetPictureInfoTag()->SetInfo(info, tmp);
          }
        }
        self->item->GetPictureInfoTag()->SetLoaded(true);
      }
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setProperty__doc__,
    "setProperty(key, value) -- Sets a listitem property, similar to an infolabel.\n"
    "\n"
    "key            : string - property name.\n"
    "value          : string or unicode - value of property.\n"
    "\n"
    "*Note, Key is NOT case sensitive.\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setProperty('AspectRatio', '1.85 : 1')\n");

  PyObject* ListItem_SetProperty(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;

    static const char *keywords[] = { "key", "value", NULL };
    char *key = NULL;
    PyObject *value = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"sO",
      (char**)keywords,
      &key,
      &value))
    {
      return NULL;
    }
    if (!key || !value) return NULL;

    string uText;
    if (!PyGetUnicodeString(uText, value, 1))
      return NULL;

    PyGUILock();
    CStdString lowerKey = key;
    self->item->SetProperty(lowerKey.ToLower(), uText.c_str());
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getProperty__doc__,
    "getProperty(key) -- Returns a listitem property as a string, similar to an infolabel.\n"
    "\n"
    "key            : string - property name.\n"
    "\n"
    "*Note, Key is NOT case sensitive.\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - AspectRatio = self.list.getSelectedItem().getProperty('AspectRatio')\n");

  PyObject* ListItem_GetProperty(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;

    static const char *keywords[] = { "key", NULL };
    char *key = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"s",
      (char**)keywords,
      &key))
    {
      return NULL;
    }
    if (!key) return NULL;

    PyGUILock();
    CStdString lowerKey = key;
    string value = self->item->GetProperty(lowerKey.ToLower());
    PyGUIUnlock();

    return Py_BuildValue((char*)"s", value.c_str());
  }

  // addContextMenuItems() method
  PyDoc_STRVAR(addContextMenuItems__doc__,
  "addContextMenuItems([(label, action,)*], replaceItems) -- Adds item(s) to the context menu for media lists.\n"
    "\n"
    "items               : list - [(label, action,)*] A list of tuples consisting of label and action pairs.\n"
    "  - label           : string or unicode - item's label.\n"
    "  - action          : string or unicode - any built-in function to perform.\n"
    "replaceItems        : [opt] bool - True=only your items will show/False=your items will be added to context menu(Default).\n"
    "\n"
    "List of functions - http://xbmc.org/wiki/?title=List_of_Built_In_Functions \n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - listitem.addContextMenuItems([('Theater Showtimes', 'XBMC.RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])\n");

  PyObject* ListItem_AddContextMenuItems(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;

    PyObject *pList = NULL;
    char bReplaceItems = false;
    static const char *keywords[] = { "items", "replaceItems", NULL };

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O|b",
      (char**)keywords,
      &pList,
      &bReplaceItems) || pList == NULL || !PyObject_TypeCheck(pList, &PyList_Type))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type List");
      return NULL;
    }

    for (int item = 0; item < PyList_Size(pList); item++)
    {
      PyObject *pTuple = NULL;
      pTuple = PyList_GetItem(pList, item);
      if (!pTuple || !PyObject_TypeCheck(pTuple, &PyTuple_Type))
      {
        PyErr_SetString(PyExc_TypeError, "List must only contain tuples");
        return NULL;
      }
      PyObject *label = NULL;
      PyObject *action = NULL;
      if (!PyArg_ParseTuple(pTuple, (char*)"OO", &label, &action))
      {
        PyErr_SetString(PyExc_TypeError, "Error unpacking tuple found in list");
        return NULL;
      }
      if (!label || !action) return NULL;

      string uText;
      if (!PyGetUnicodeString(uText, label, 1))
        return NULL;
      string uAction;
      if (!PyGetUnicodeString(uAction, action, 1))
        return NULL;
      PyGUILock();

      CStdString property;
      property.Format("contextmenulabel(%i)", item);
      self->item->SetProperty(property, uText);

      property.Format("contextmenuaction(%i)", item);
      self->item->SetProperty(property, uAction);

      PyGUIUnlock();
    }

    // set our replaceItems status
    if (bReplaceItems)
      self->item->SetProperty("pluginreplacecontextitems", (bool)bReplaceItems);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(setPath__doc__,
    "setPath(path) -- Sets the listitem's path.\n"
    "\n"
    "path           : string or unicode - path, activated when item is clicked.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setPath(path='ActivateWindow(Weather)')\n");

  PyObject* ListItem_SetPath(ListItem *self, PyObject *args, PyObject *kwds)
  {
    if (!self->item) return NULL;
    PyObject* pPath = NULL;

    if (!PyArg_ParseTuple(args, (char*)"O", &pPath)) return NULL;
    static const char *keywords[] = { "path", NULL };

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"O",
      (char**)keywords,
      &pPath
      ))
    {
      return NULL;
    }

    string path;
    if (pPath && !PyGetUnicodeString(path, pPath, 1))
      return NULL;
    // set path
    PyGUILock();
    self->item->m_strPath = path;
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ListItem_methods[] = {
    {(char*)"getLabel" , (PyCFunction)ListItem_GetLabel, METH_VARARGS, getLabel__doc__},
    {(char*)"setLabel" , (PyCFunction)ListItem_SetLabel, METH_VARARGS, setLabel__doc__},
    {(char*)"getLabel2", (PyCFunction)ListItem_GetLabel2, METH_VARARGS, getLabel2__doc__},
    {(char*)"setLabel2", (PyCFunction)ListItem_SetLabel2, METH_VARARGS, setLabel2__doc__},
    {(char*)"setIconImage", (PyCFunction)ListItem_SetIconImage, METH_VARARGS, setIconImage__doc__},
    {(char*)"setThumbnailImage", (PyCFunction)ListItem_SetThumbnailImage, METH_VARARGS, setThumbnailImage__doc__},
    {(char*)"select", (PyCFunction)ListItem_Select, METH_VARARGS, select__doc__},
    {(char*)"isSelected", (PyCFunction)ListItem_IsSelected, METH_VARARGS, isSelected__doc__},
    {(char*)"setInfo", (PyCFunction)ListItem_SetInfo, METH_VARARGS|METH_KEYWORDS, setInfo__doc__},
    {(char*)"setProperty", (PyCFunction)ListItem_SetProperty, METH_VARARGS|METH_KEYWORDS, setProperty__doc__},
    {(char*)"getProperty", (PyCFunction)ListItem_GetProperty, METH_VARARGS|METH_KEYWORDS, getProperty__doc__},
    {(char*)"addContextMenuItems", (PyCFunction)ListItem_AddContextMenuItems, METH_VARARGS|METH_KEYWORDS, addContextMenuItems__doc__},
    {(char*)"setPath" , (PyCFunction)ListItem_SetPath, METH_VARARGS|METH_KEYWORDS, setPath__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(listItem__doc__,
    "ListItem class.\n"
    "\n"
    "ListItem([label, label2, iconImage, thumbnailImage, path]) -- Creates a new ListItem.\n"
    "\n"
    "label          : [opt] string or unicode - label1 text.\n"
    "label2         : [opt] string or unicode - label2 text.\n"
    "iconImage      : [opt] string - icon filename.\n"
    "thumbnailImage : [opt] string - thumbnail filename.\n"
    "path           : [opt] string or unicode - listitem's path.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - listitem = xbmcgui.ListItem('Casino Royale', '[PG-13]', 'blank-poster.tbn', 'poster.tbn', path='f:\\\\movies\\\\casino_royale.mov')\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject ListItem_Type;

  void initListItem_Type()
  {
    PyInitializeTypeObject(&ListItem_Type);

    ListItem_Type.tp_name = (char*)"xbmcgui.ListItem";
    ListItem_Type.tp_basicsize = sizeof(ListItem);
    ListItem_Type.tp_dealloc = (destructor)ListItem_Dealloc;
    ListItem_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    ListItem_Type.tp_doc = listItem__doc__;
    ListItem_Type.tp_methods = ListItem_methods;
    ListItem_Type.tp_base = 0;
    ListItem_Type.tp_new = ListItem_New;
  }
}

#ifdef __cplusplus
}
#endif

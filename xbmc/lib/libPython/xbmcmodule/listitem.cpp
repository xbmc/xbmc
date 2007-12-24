#include "stdafx.h"
#include "../python/python.h"
#include "listitem.h"
#include "pyutil.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* ListItem_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    ListItem *self;
    static char *keywords[] = { "label", "label2",
      "iconImage", "thumbnailImage", NULL };

    PyObject* label = NULL;
    PyObject* label2 = NULL;
    char* cIconImage = NULL;
    char* cThumbnailImage = NULL;

    // allocate new object
    self = (ListItem*)type->tp_alloc(type, 0);
    if (!self) return NULL;
      self->item = NULL;

    // parse user input
    if (!PyArg_ParseTupleAndKeywords(
      args, kwds,
      "|OOss", keywords,
      &label, &label2,
      &cIconImage, &cThumbnailImage))
    {
      Py_DECREF( self );
      return NULL;
    }

    // create CFileItem
    self->item = new CFileItem();
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

    self->item = new CFileItem(strLabel);
    if (!self->item)
    {
      Py_DECREF( self );
      return NULL;
    }

    return self;
  }

  void ListItem_Dealloc(ListItem* self)
  {
    if (self->item) delete self->item;
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
    const char *cLabel =  self->item->GetLabel().c_str();
    PyGUIUnlock();

    return Py_BuildValue("s", cLabel);
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

    return Py_BuildValue("s", cLabel);
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
    PyObject* unicodeLine = NULL;
    if (!self->item) return NULL;

    if (!PyArg_ParseTuple(args, "O", &unicodeLine)) return NULL;

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

    if (!PyArg_ParseTuple(args, "O", &unicodeLine)) return NULL;

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

    if (!PyArg_ParseTuple(args, "s", &cLine)) return NULL;

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

    if (!PyArg_ParseTuple(args, "s", &cLine)) return NULL;

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

    bool bOnOff = false;
    if (!PyArg_ParseTuple(args, "b", &bOnOff)) return NULL;

    PyGUILock();
    self->item->Select(bOnOff);
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

    return Py_BuildValue("b", bOnOff);
  }

  PyDoc_STRVAR(setInfo__doc__,
    "setInfo(type, infoLabels) -- Sets the listitem's infoLabels.\n"
    "\n"
    "type           : string - type of media(video/music/pictures).\n"
    "infoLabels     : dictionary - pairs of { label: value }.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - self.list.getSelectedItem().setInfo('video', { 'Genre': 'Comedy' })\n");

  PyObject* ListItem_SetInfo(ListItem *self, PyObject *args, PyObject *kwds)
  {
    static char *keywords[] = { "type", "infoLabels", NULL };
    char *cType = NULL;
    PyObject *pInfoLabels = NULL;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      "sO",
      keywords,
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
        else if (strcmpi(PyString_AsString(key), "watched") == 0)
          self->item->GetVideoInfoTag()->m_bWatched = PyInt_AsLong(value)==1;
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
              if (!PyArg_ParseTuple(pTuple, "O|O", &pActor, &pRole)) continue;
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
        // TODO: Figure out how to set picture tags
        if (!PyGetUnicodeString(tmp, value, 1)) continue;
        if (strcmpi(PyString_AsString(key), "title") == 0)
          self->item->m_strTitle = tmp;
        else if (strcmpi(PyString_AsString(key), "count") == 0)
          self->item->m_iprogramCount = PyInt_AsLong(value);
        else if (strcmpi(PyString_AsString(key), "size") == 0)
          self->item->m_dwSize = (__int64)PyLong_AsLongLong(value);
        else if (strcmpi(PyString_AsString(key), "picturepath") == 0)
          self->item->m_strPath = tmp;
        //else if (strcmpi(PyString_AsString(key), "picturedatetime") == 0)
        //  self->item->GetPictureInfoTag()->m_exifInfo.DateTime = PyString_AsString(value);
        //else if (strcmpi(PyString_AsString(key), "pictureresolution") == 0)
        //{
          // TODO: Grab a tuple and set width/height
          //value.Format("%d x %d", m_exifInfo.Width, m_exifInfo.Height);
          //self->item->GetPictureInfoTag()->m_exifInfo.Width = PyInt_AsLong(value);
          //self->item->GetPictureInfoTag()->m_exifInfo.Height = PyInt_AsLong(value);
        //}
      }
    }
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef ListItem_methods[] = {
    {"getLabel" , (PyCFunction)ListItem_GetLabel, METH_VARARGS, getLabel__doc__},
    {"setLabel" , (PyCFunction)ListItem_SetLabel, METH_VARARGS, setLabel__doc__},
    {"getLabel2", (PyCFunction)ListItem_GetLabel2, METH_VARARGS, getLabel2__doc__},
    {"setLabel2", (PyCFunction)ListItem_SetLabel2, METH_VARARGS, setLabel2__doc__},
    {"setIconImage", (PyCFunction)ListItem_SetIconImage, METH_VARARGS, setIconImage__doc__},
    {"setThumbnailImage", (PyCFunction)ListItem_SetThumbnailImage, METH_VARARGS, setThumbnailImage__doc__},
    {"select", (PyCFunction)ListItem_Select, METH_VARARGS, select__doc__},
    {"isSelected", (PyCFunction)ListItem_IsSelected, METH_VARARGS, isSelected__doc__},
    {"setInfo", (PyCFunction)ListItem_SetInfo, METH_KEYWORDS, setInfo__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(listItem__doc__,
    "ListItem class.\n"
    "\n"
    "ListItem([label, label2, iconImage, thumbnailImage]) -- Creates a new ListItem.\n"
    "\n"
    "label          : [opt] string or unicode - label1 text.\n"
    "label2         : [opt] string or unicode - label2 text.\n"
    "iconImage      : [opt] string - icon filename.\n"
    "thumbnailImage : [opt] string - thumbnail filename.\n"
    "\n"
    "example:\n"
    "  - listitem = xbmcgui.ListItem('Casino Royale', '[PG-13]', 'blank-poster.tbn', 'poster.tbn')\n");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject ListItem_Type;

  void initListItem_Type()
  {
    PyInitializeTypeObject(&ListItem_Type);

    ListItem_Type.tp_name = "xbmcgui.ListItem";
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

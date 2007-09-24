#include "stdafx.h"
#include "dialog.h"
#ifndef _LINUX
#include "../python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include "pyutil.h"
#include "../../../Application.h"
#include "../xbmc/GUIDialogFileBrowser.h"
#include "../xbmc/GUIDialogNumeric.h"
#include "../xbmc/GUIDialogGamepad.h"

#define ACTIVE_WINDOW  m_gWindowManager.GetActiveWindow()

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* WindowDialog_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    WindowDialog *self;

    self = (WindowDialog*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->sXMLFileName) string();
    new(&self->sFallBackPath) string();
    new(&self->vecControls) std::vector<Control*>();    

    self->iWindowId = -1;

    if (!PyArg_ParseTuple(args, "|i", &self->iWindowId)) return NULL;

    // create new GUIWindow
    if (!Window_CreateNewWindow((Window*)self, true))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->vecControls.clear();
      self->vecControls.~vector();
      self->sFallBackPath.~string();          
      self->sXMLFileName.~string();         
      self->ob_type->tp_free((PyObject*)self);
      return NULL;
    }

    return (PyObject*)self;
  }

  PyDoc_STRVAR(ok__doc__,
    "ok(heading, line1[, line2, line3]) -- Show a dialog 'OK'.\n"
    "\n"
    "heading        : string or unicode - dialog heading.\n"
    "line1          : string or unicode - line #1 text.\n"
    "line2          : [opt] string or unicode - line #2 text.\n"
    "line3          : [opt] string or unicode - line #3 text.\n"
    "\n"
    "*Note, Returns True if 'Ok' was pressed, else False.\n"
    "\n"
    "example:\n"
    "  - dialog = xbmcgui.Dialog()\n"
    "  - ok = dialog.ok('Xbox Media Center', 'There was an error.')\n");

  PyObject* Dialog_OK(PyObject *self, PyObject *args)
  {
    const DWORD dWindow = WINDOW_DIALOG_OK;
    PyObject* unicodeLine[4];
    for (int i = 0; i < 4; i++) unicodeLine[i] = NULL;

    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(dWindow);
    if (PyWindowIsNull(pDialog)) return NULL;

    // get lines, last 2 lines are optional.
    string utf8Line[4];
    if (!PyArg_ParseTuple(args, "OO|OO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3]))  return NULL;

    for (int i = 0; i < 4; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
    pDialog->SetHeading(utf8Line[0]);
    pDialog->SetLine(0, utf8Line[1]);
    pDialog->SetLine(1, utf8Line[2]);
    pDialog->SetLine(2, utf8Line[3]);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};
    g_applicationMessenger.SendMessage(tMsg, true);

    return Py_BuildValue("b", pDialog->IsConfirmed());
  }

  PyDoc_STRVAR(browse__doc__,
    "browse(type, heading, shares[, mask, useThumbs, treatAsFolder, default]) -- Show a 'Browse' dialog.\n"
    "\n"
    "type           : integer - the type of browse dialog.\n"
    "heading        : string or unicode - dialog heading.\n"
    "shares         : string or unicode - from sources.xml. (i.e. 'myprograms')\n"
    "mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')\n"
    "useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist.\n"
    "treatAsFolder  : [opt] boolean - if True playlists and archives act as folders.\n"
    "default        : [opt] string - default path or file.\n"
    "\n"
    "Types:\n"
    "  0 : ShowAndGetDirectory\n"
    "  1 : ShowAndGetFile\n"
    "  2 : ShowAndGetImage\n"
    "  3 : ShowAndGetWriteableDirectory\n"
    "\n"
    "*Note, Returns filename and/or path as a string to the location of the highlighted item,\n"
    "       if user pressed 'Ok' or a masked item was selected.\n"
    "       Returns the default value if dialog was canceled.\n"
    "example:\n"
    "  - dialog = xbmcgui.Dialog()\n"
    "  - fn = dialog.browse(3, 'Xbox Media Center', 'files', '', False, False, 'T:\\script_data\\XBMC Lyrics')\n");

  PyObject* Dialog_Browse(PyObject *self, PyObject *args)
  {
    int browsetype = 0;
    bool useThumbs = false;
    bool useFileDirectories = false;
    CStdString value;
    PyObject* unicodeLine[3];
    string utf8Line[3];
    char *cDefault = NULL;
    for (int i = 0; i < 3; i++) unicodeLine[i] = NULL;
    if (!PyArg_ParseTuple(args, "iOO|Obbs", &browsetype , &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &useThumbs, &useFileDirectories, &cDefault))  return NULL;
    for (int i = 0; i < 3; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
    VECSHARES *shares = g_settings.GetSharesFromType(utf8Line[1]);
    if (!shares) return NULL;

    if (useFileDirectories && !utf8Line[2].size() == 0) 
      utf8Line[2] += "|.rar|.zip";
    
    value = cDefault;
    if (browsetype == 1)
      CGUIDialogFileBrowser::ShowAndGetFile(*shares, utf8Line[2], utf8Line[0], value, useThumbs, useFileDirectories);
    else if (browsetype == 2)
      CGUIDialogFileBrowser::ShowAndGetImage(*shares, utf8Line[0], value);
    else
      CGUIDialogFileBrowser::ShowAndGetDirectory(*shares, utf8Line[0], value, browsetype != 0);
    return Py_BuildValue("s", value.c_str());
  }

  PyDoc_STRVAR(numeric__doc__,
    "numeric(type, heading[, default]) -- Show a 'Numeric' dialog.\n"
    "\n"
    "type           : integer - the type of numeric dialog.\n"
    "heading        : string or unicode - dialog heading.\n"
    "default        : [opt] string - default value.\n"
    "\n"
    "Types:\n"
    "  0 : ShowAndGetNumber    (default format: #)\n"
    "  1 : ShowAndGetDate      (default format: DD/MM/YYYY)\n"
    "  2 : ShowAndGetTime      (default format: HH:MM)\n"
    "  3 : ShowAndGetIPAddress (default format: #.#.#.#)\n"
    "\n"
    "*Note, Returns the entered data as a string.\n"
    "       Returns the default value if dialog was canceled.\n"
    "\n"
    "example:\n"
    "  - dialog = xbmcgui.Dialog()\n"
    "  - d = dialog.numeric(1, 'Enter date of birth')\n");

  PyObject* Dialog_Numeric(PyObject *self, PyObject *args)
  {
    int inputtype = 0;
    CStdString value;
    PyObject *heading = NULL;
    char *cDefault = NULL;
    SYSTEMTIME timedate;
    GetLocalTime(&timedate);
    if (!PyArg_ParseTuple(args, "iO|s", &inputtype, &heading, &cDefault))  return NULL;

    CStdString utf8Heading;
    if (heading && PyGetUnicodeString(utf8Heading, heading, 1))
    {
      if (inputtype == 1)
      {
        if (cDefault && strlen(cDefault) == 10)
        {
          CStdString sDefault = cDefault;
          timedate.wDay = atoi(sDefault.Left(2));
          timedate.wMonth = atoi(sDefault.Mid(3,4));
          timedate.wYear = atoi(sDefault.Right(4));
        }
        if (CGUIDialogNumeric::ShowAndGetDate(timedate, utf8Heading))
          value.Format("%2d/%2d/%4d", timedate.wDay, timedate.wMonth, timedate.wYear);
        else
          value = cDefault;
      }
      else if (inputtype == 2)
      {
        if (cDefault && strlen(cDefault) == 5)
        {
          CStdString sDefault = cDefault;
          timedate.wHour = atoi(sDefault.Left(2));
          timedate.wMinute = atoi(sDefault.Right(2));
        }
        if (CGUIDialogNumeric::ShowAndGetTime(timedate, utf8Heading))
          value.Format("%2d:%02d", timedate.wHour, timedate.wMinute);
        else
          value = cDefault;
      }
      else if (inputtype == 3)
      {
        value = cDefault;
        CGUIDialogNumeric::ShowAndGetIPAddress(value, utf8Heading);
      }
      else
      {
        value = cDefault;
        CGUIDialogNumeric::ShowAndGetNumber(value, utf8Heading);
      }
    }
    return Py_BuildValue("s", value.c_str());
  }

  PyDoc_STRVAR(yesno__doc__,
    "yesno(heading, line1[, line2, line3]) -- Show a dialog 'YES/NO'.\n"
    "\n"
    "heading        : string or unicode - dialog heading.\n"
    "line1          : string or unicode - line #1 text.\n"
    "line2          : [opt] string or unicode - line #2 text.\n"
    "line3          : [opt] string or unicode - line #3 text.\n"
    "nolabel        : [opt] label to put on the no button.\n"
    "yeslabel       : [opt] label to put on the yes button.\n"
    "\n"
    "*Note, Returns True if 'Yes' was pressed, else False.\n"
    "\n"
    "example:\n"
    "  - dialog = xbmcgui.Dialog()\n"
    "  - ret = dialog.yesno('Xbox Media Center', 'Do you want to exit this script?')\n");

  PyObject* Dialog_YesNo(PyObject *self, PyObject *args)
  {
    const DWORD dWindow = WINDOW_DIALOG_YES_NO;
    PyObject* unicodeLine[6];
    for (int i = 0; i < 6; i++) unicodeLine[i] = NULL;
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(dWindow);
    if (PyWindowIsNull(pDialog)) return NULL;

    // get lines, last 4 lines are optional.
    string utf8Line[6];
    if (!PyArg_ParseTuple(args, "OO|OOOO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3],&unicodeLine[4],&unicodeLine[5]))  return NULL;

    for (int i = 0; i < 6; ++i)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
    pDialog->SetHeading(utf8Line[0]);
    pDialog->SetLine(0, utf8Line[1]);
    pDialog->SetLine(1, utf8Line[2]);
    pDialog->SetLine(2, utf8Line[3]);
    if (utf8Line[4] != "")
      pDialog->SetChoice(0,utf8Line[4]);
    if (utf8Line[5] != "")
      pDialog->SetChoice(1,utf8Line[5]);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};
    g_applicationMessenger.SendMessage(tMsg, true);

    return Py_BuildValue("b", pDialog->IsConfirmed());
  }

  PyDoc_STRVAR(select__doc__,
    "select(heading, list) -- Show a select dialog.\n"
    "\n"
    "heading        : string or unicode - dialog heading.\n"
    "list           : string list - list of items.\n"
    "\n"
    "*Note, Returns the position of the highlighted item as an integer.\n"
    "\n"
    "example:\n"
    "  - dialog = xbmcgui.Dialog()\n"
    "  - ret = dialog.select('Choose a playlist', ['Playlist #1', 'Playlist #2, 'Playlist #3'])\n");

  PyObject* Dialog_Select(PyObject *self, PyObject *args)
  {
    const DWORD dWindow = WINDOW_DIALOG_SELECT;
    PyObject *heading = NULL;
    PyObject *list = NULL;

    if (!PyArg_ParseTuple(args, "OO", &heading, &list))  return NULL;
    if (!PyList_Check(list)) return NULL;

    CGUIDialogSelect* pDialog= (CGUIDialogSelect*)m_gWindowManager.GetWindow(dWindow);
    if (PyWindowIsNull(pDialog)) return NULL;

    pDialog->Reset();
    CStdString utf8Heading;
    if (heading && PyGetUnicodeString(utf8Heading, heading, 1))
      pDialog->SetHeading(utf8Heading);

    PyObject *listLine = NULL;
    for(int i = 0; i < PyList_Size(list); i++)
    {
      listLine = PyList_GetItem(list, i);
      CStdString utf8Line;
      if (listLine && PyGetUnicodeString(utf8Line, listLine, i))
        pDialog->Add(utf8Line);
    }

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, dWindow, ACTIVE_WINDOW};
    g_applicationMessenger.SendMessage(tMsg, true);

    return Py_BuildValue("i", pDialog->GetSelectedLabel());
  }

/*****************************************************************
 * start of dialog process methods and python objects
 *****************************************************************/

  PyDoc_STRVAR(create__doc__,
    "create(heading[, line1, line2, line3]) -- Create and show a progress dialog.\n"
    "\n"
    "heading        : string or unicode - dialog heading.\n"
    "line1          : string or unicode - line #1 text.\n"
    "line2          : [opt] string or unicode - line #2 text.\n"
    "line3          : [opt] string or unicode - line #3 text.\n"
    "\n"
    "*Note, Use update() to update lines and progressbar.\n"
    "\n"
    "example:\n"
    "  - pDialog = xbmcgui.ProgressDialog()\n"
    "  - ret = pDialog.create('Xbox Media Center', 'Initializing script...')\n");

  PyObject* Dialog_ProgressCreate(PyObject *self, PyObject *args)
  {
    PyObject* unicodeLine[4];
    for (int i = 0; i < 4; i++) unicodeLine[i] = NULL;

    // get lines, last 3 lines are optional.
    if (!PyArg_ParseTuple(args, "O|OOO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3]))  return NULL;

    string utf8Line[4];
    for (int i = 0; i < 4; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }

    CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (PyWindowIsNull(pDialog)) return NULL;

    pDialog->SetHeading(utf8Line[0]);

    for (int i = 1; i < 4; i++)
      pDialog->SetLine(i - 1,utf8Line[i]);

    pDialog->StartModal();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(update__doc__,
    "update(percent[, line1, line2, line3]) -- Update's the progress dialog.\n"
    "\n"
    "percent        : integer - percent complete. (0:100)\n"
    "line1          : [opt] string or unicode - line #1 text.\n"
    "line2          : [opt] string or unicode - line #2 text.\n"
    "line3          : [opt] string or unicode - line #3 text.\n"
    "\n"
    "*Note, If percent == 0, the progressbar will be hidden.\n"
    "\n"
    "example:\n"
    "  - pDialog.update(25, 'Importing modules...')\n");

  PyObject* Dialog_ProgressUpdate(PyObject *self, PyObject *args)
  {
    int percentage = 0;
    PyObject *unicodeLine[3];
    for (int i = 0; i < 3; i++)  unicodeLine[i] = NULL;
    if (!PyArg_ParseTuple(args, "i|OOO", &percentage,&unicodeLine[0], &unicodeLine[1], &unicodeLine[2]))  return NULL;

    string utf8Line[3];
    for (int i = 0; i < 3; i++)
    {
      if (unicodeLine[i] && !PyGetUnicodeString(utf8Line[i], unicodeLine[i], i+2))
        return NULL;
    }

    CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (PyWindowIsNull(pDialog)) return NULL;

    if (percentage >= 0 && percentage <= 100)
    {
      pDialog->SetPercentage(percentage);
      pDialog->ShowProgressBar(true);
    }
    else
    {
      pDialog->ShowProgressBar(false);
    }
    for (int i = 0; i < 3; i++)
    {
      if (unicodeLine[i])
        pDialog->SetLine(i,utf8Line[i]);
    }

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(isCanceled__doc__,
    "iscanceled() -- Returns True if the user pressed cancel.\n"
    "\n"
    "example:\n"
    "  - if (pDialog.iscanceled()): return\n");

  PyObject* Dialog_ProgressIsCanceled(PyObject *self, PyObject *args)
  {
    bool canceled = false;
    CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (PyWindowIsNull(pDialog)) return NULL;

    canceled = pDialog->IsCanceled();

    return Py_BuildValue("b", canceled);
  }

  PyDoc_STRVAR(close__doc__,
    "close() -- Close the progress dialog.\n"
    "\n"
    "example:\n"
    "  - pDialog.close()\n");


  PyObject* Dialog_ProgressClose(PyObject *self, PyObject *args)
  {
    CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (PyWindowIsNull(pDialog)) return NULL;

    pDialog->Close();

    Py_INCREF(Py_None);
    return Py_None;
  }

  static void Dialog_ProgressDealloc(PyObject *self)
  {
    CGUIDialogProgress* pDialog= (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (PyWindowIsNull(pDialog)) return;
    pDialog->Close();
    self->ob_type->tp_free((PyObject*)self);
  }

  PyMethodDef WindowDialog_methods[] = {
    {NULL, NULL, 0, NULL}
  };

  /* xbmc Dialog functions for use in python */
  PyMethodDef Dialog_methods[] = {
    {"yesno", (PyCFunction)Dialog_YesNo, METH_VARARGS, yesno__doc__},
    {"select", (PyCFunction)Dialog_Select, METH_VARARGS, select__doc__},
    {"ok", (PyCFunction)Dialog_OK, METH_VARARGS, ok__doc__},
    {"browse", (PyCFunction)Dialog_Browse, METH_VARARGS, browse__doc__},
    {"numeric", (PyCFunction)Dialog_Numeric, METH_VARARGS, numeric__doc__},
    {NULL, NULL, 0, NULL}
  };

  /* xbmc progress Dialog functions for use in python */
  PyMethodDef DialogProgress_methods[] = {
    {"create", (PyCFunction)Dialog_ProgressCreate, METH_VARARGS, create__doc__},
    {"update", (PyCFunction)Dialog_ProgressUpdate, METH_VARARGS, update__doc__},
    {"close", (PyCFunction)Dialog_ProgressClose, METH_VARARGS, close__doc__},
    {"iscanceled", (PyCFunction)Dialog_ProgressIsCanceled, METH_VARARGS, isCanceled__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(windowDialog__doc__,
    "WindowDialog class.\n");

  PyDoc_STRVAR(dialog__doc__,
    "Dialog class.\n");

  PyDoc_STRVAR(dialogProgress__doc__,
    "DialogProgress class.\n");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject WindowDialog_Type;

  void initWindowDialog_Type()
  {
    PyInitializeTypeObject(&WindowDialog_Type);

    WindowDialog_Type.tp_name = "xbmcgui.WindowDialog";
    WindowDialog_Type.tp_basicsize = sizeof(WindowDialog);
    WindowDialog_Type.tp_dealloc = (destructor)Window_Dealloc;
    WindowDialog_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    WindowDialog_Type.tp_doc = windowDialog__doc__;
    WindowDialog_Type.tp_methods = WindowDialog_methods;
    WindowDialog_Type.tp_base = &Window_Type;
    WindowDialog_Type.tp_new = WindowDialog_New;
  }

  PyTypeObject DialogProgress_Type;

  void initDialogProgress_Type()
  {
    PyInitializeTypeObject(&DialogProgress_Type);

    DialogProgress_Type.tp_name = "xbmcgui.DialogProgress";
    DialogProgress_Type.tp_basicsize = sizeof(DialogProgress);
    DialogProgress_Type.tp_dealloc = (destructor)Dialog_ProgressDealloc;
    DialogProgress_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    DialogProgress_Type.tp_doc = dialogProgress__doc__;
    DialogProgress_Type.tp_methods = DialogProgress_methods;
    DialogProgress_Type.tp_base = 0;
    DialogProgress_Type.tp_new = PyType_GenericNew;
  }


  PyTypeObject Dialog_Type;

  void initDialog_Type()
  {
    PyInitializeTypeObject(&Dialog_Type);

    Dialog_Type.tp_name = "xbmcgui.Dialog";
    Dialog_Type.tp_basicsize = sizeof(Dialog);
    Dialog_Type.tp_dealloc = 0;
    Dialog_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Dialog_Type.tp_doc = dialog__doc__;
    Dialog_Type.tp_methods = Dialog_methods;
    Dialog_Type.tp_base = 0;
    Dialog_Type.tp_new = PyType_GenericNew;
  }
}

#ifdef __cplusplus
}
#endif


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

#include "dialog.h"

#include "Application.h"
#include "settings/Settings.h"
#include "pyutil.h"
#include "pythreadstate.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogGamepad.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"

using namespace std;

#define ACTIVE_WINDOW  g_windowManager.GetActiveWindow()


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

    if (!PyArg_ParseTuple(args, (char*)"|i", &self->iWindowId)) return NULL;

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
    "  - ok = dialog.ok('XBMC', 'There was an error.')\n");

  PyObject* Dialog_OK(PyObject *self, PyObject *args)
  {
    const int window = WINDOW_DIALOG_OK;
    PyObject* unicodeLine[4];
    for (int i = 0; i < 4; i++) unicodeLine[i] = NULL;

    // get lines, last 2 lines are optional.
    string utf8Line[4];
    if (!PyArg_ParseTuple(args, (char*)"OO|OO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3]))  return NULL;

    for (int i = 0; i < 4; i++)
    {
      if (unicodeLine[i] && !PyXBMCGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
    PyXBMCGUILock();
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(window);
    if (PyXBMCWindowIsNull(pDialog)) return NULL;

    pDialog->SetHeading(utf8Line[0]);
    pDialog->SetLine(0, utf8Line[1]);
    pDialog->SetLine(1, utf8Line[2]);
    pDialog->SetLine(2, utf8Line[3]);

    PyXBMCGUIUnlock();

    //send message and wait for user input
    PyXBMCWaitForThreadMessage(TMSG_DIALOG_DOMODAL, window, ACTIVE_WINDOW);
    PyXBMCGUILock();
    bool result = pDialog->IsConfirmed();
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"b", result);
  }

  PyDoc_STRVAR(browse__doc__,
    "browse(type, heading, shares[, mask, useThumbs, treatAsFolder, default, enableMultiple]) -- Show a 'Browse' dialog.\n"
    "\n"
    "type           : integer - the type of browse dialog.\n"
    "heading        : string or unicode - dialog heading.\n"
    "shares         : string or unicode - from sources.xml. (i.e. 'myprograms')\n"
    "mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')\n"
    "useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist.\n"
    "treatAsFolder  : [opt] boolean - if True playlists and archives act as folders.\n"
    "default        : [opt] string - default path or file.\n"
    "\n"
    "enableMultiple : [opt] boolean - if True multiple file selection is enabled.\n"
    "Types:\n"
    "  0 : ShowAndGetDirectory\n"
    "  1 : ShowAndGetFile\n"
    "  2 : ShowAndGetImage\n"
    "  3 : ShowAndGetWriteableDirectory\n"
    "\n"
    "*Note, If enableMultiple is False (default): returns filename and/or path as a string\n"
    "       to the location of the highlighted item, if user pressed 'Ok' or a masked item\n"
    "       was selected. Returns the default value if dialog was canceled.\n"
    "       If enableMultiple is True: returns tuple of marked filenames as a string,"
    "       if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.\n"
    "\n"
    "       If type is 0 or 3 the enableMultiple parameter is ignored."
    "\n"
    "example:\n"
    "  - dialog = xbmcgui.Dialog()\n"
    "  - fn = dialog.browse(3, 'XBMC', 'files', '', False, False, False, 'special://masterprofile/script_data/XBMC Lyrics')\n");

  PyObject* Dialog_Browse(PyObject *self, PyObject *args)
  {
    int browsetype = 0;
    char useThumbs = false;
    char useFileDirectories = false;
    char enableMultiple = false;
    CStdString value;
    CStdStringArray valuelist;
    PyObject* unicodeLine[3];
    string utf8Line[3];
    char *cDefault = NULL;
    PyObject *result;

    for (int i = 0; i < 3; i++)
      unicodeLine[i] = NULL;
    if (!PyArg_ParseTuple(args, (char*)"iOO|Obbsb", 
                          &browsetype , &unicodeLine[0],
                          &unicodeLine[1], &unicodeLine[2],
                          &useThumbs, &useFileDirectories,
                          &cDefault, &enableMultiple))
    {
      return NULL;
    }
    for (int i = 0; i < 3; i++)
    {
      if (unicodeLine[i] && !PyXBMCGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
    VECSOURCES *shares = g_settings.GetSourcesFromType(utf8Line[1]);
    if (!shares) return NULL;

    if (useFileDirectories && !utf8Line[2].size() == 0)
      utf8Line[2] += "|.rar|.zip";

    value = cDefault;

    CPyThreadState pyState;

    if (browsetype == 1)
    {
      if (enableMultiple)
        CGUIDialogFileBrowser::ShowAndGetFileList(*shares, utf8Line[2], utf8Line[0], valuelist, 0 != useThumbs, 0 != useFileDirectories);
      else
        CGUIDialogFileBrowser::ShowAndGetFile(*shares, utf8Line[2], utf8Line[0], value, 0 != useThumbs, 0 != useFileDirectories);
    }
    else if (browsetype == 2)
    {
      if (enableMultiple)
        CGUIDialogFileBrowser::ShowAndGetImageList(*shares, utf8Line[0], valuelist);
      else
        CGUIDialogFileBrowser::ShowAndGetImage(*shares, utf8Line[0], value);
    }
    else
      CGUIDialogFileBrowser::ShowAndGetDirectory(*shares, utf8Line[0], value, browsetype != 0);

    pyState.Restore();

    if (enableMultiple && (browsetype == 1 || browsetype == 2))
    {
      result = PyTuple_New(valuelist.size());
      if (!result)
        return NULL;

      for (unsigned int i = 0; i < valuelist.size(); i++)
        PyTuple_SetItem(result, i, PyString_FromString(valuelist.at(i).c_str()));

      return result;
    }
    else
      return Py_BuildValue((char*)"s", value.c_str());
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
    if (!PyArg_ParseTuple(args, (char*)"iO|s", &inputtype, &heading, &cDefault))  return NULL;

    CStdString utf8Heading;
    if (heading && PyXBMCGetUnicodeString(utf8Heading, heading, 1))
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
        bool gotDate;
        Py_BEGIN_ALLOW_THREADS
        gotDate = CGUIDialogNumeric::ShowAndGetDate(timedate, utf8Heading);
        Py_END_ALLOW_THREADS
        if (gotDate)
          value.Format("%2d/%2d/%4d", timedate.wDay, timedate.wMonth, timedate.wYear);
        else
        {
          Py_INCREF(Py_None);
          return Py_None;
        }
      }
      else if (inputtype == 2)
      {
        if (cDefault && strlen(cDefault) == 5)
        {
          CStdString sDefault = cDefault;
          timedate.wHour = atoi(sDefault.Left(2));
          timedate.wMinute = atoi(sDefault.Right(2));
        }
        bool gotTime;
        Py_BEGIN_ALLOW_THREADS
        gotTime = CGUIDialogNumeric::ShowAndGetTime(timedate, utf8Heading);
        Py_END_ALLOW_THREADS
        if (gotTime)
          value.Format("%2d:%02d", timedate.wHour, timedate.wMinute);
        else
        {
          Py_INCREF(Py_None);
          return Py_None;
        }
      }
      else if (inputtype == 3)
      {
        value = cDefault;
        bool gotIPAddress;
        Py_BEGIN_ALLOW_THREADS
        gotIPAddress = CGUIDialogNumeric::ShowAndGetIPAddress(value, utf8Heading);
        Py_END_ALLOW_THREADS
        if (!gotIPAddress)
        {
          Py_INCREF(Py_None);
          return Py_None;
        }
      }
      else
      {
        value = cDefault;
        bool gotNumber;
        Py_BEGIN_ALLOW_THREADS
        gotNumber = CGUIDialogNumeric::ShowAndGetNumber(value, utf8Heading);
        Py_END_ALLOW_THREADS
        if (!gotNumber)
        {
          Py_INCREF(Py_None);
          return Py_None;
        }
      }
    }
    return Py_BuildValue((char*)"s", value.c_str());
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
    "  - ret = dialog.yesno('XBMC', 'Do you want to exit this script?')\n");

  PyObject* Dialog_YesNo(PyObject *self, PyObject *args)
  {
    const int window = WINDOW_DIALOG_YES_NO;
    PyObject* unicodeLine[6];
    for (int i = 0; i < 6; i++) unicodeLine[i] = NULL;

    // get lines, last 4 lines are optional.
    string utf8Line[6];
    if (!PyArg_ParseTuple(args, (char*)"OO|OOOO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3],&unicodeLine[4],&unicodeLine[5]))  return NULL;

    for (int i = 0; i < 6; ++i)
    {
      if (unicodeLine[i] && !PyXBMCGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }
    PyXBMCGUILock();
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(window);
    if (PyXBMCWindowIsNull(pDialog)) return NULL;
    pDialog->SetHeading(utf8Line[0]);
    pDialog->SetLine(0, utf8Line[1]);
    pDialog->SetLine(1, utf8Line[2]);
    pDialog->SetLine(2, utf8Line[3]);
    if (utf8Line[4] != "")
      pDialog->SetChoice(0,utf8Line[4]);
    if (utf8Line[5] != "")
      pDialog->SetChoice(1,utf8Line[5]);
    PyXBMCGUIUnlock();

    //send message and wait for user input
    PyXBMCWaitForThreadMessage(TMSG_DIALOG_DOMODAL, window, ACTIVE_WINDOW);
    PyXBMCGUILock();
    bool result = pDialog->IsConfirmed();
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"b", result);
  }

  PyDoc_STRVAR(select__doc__,
    "select(heading, list) -- Show a select dialog.\n"
    "\n"
    "heading        : string or unicode - dialog heading.\n"
    "list           : string list - list of items.\n"
    "autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)\n"
    "\n"
    "*Note, Returns the position of the highlighted item as an integer.\n"
    "\n"
    "example:\n"
    "  - dialog = xbmcgui.Dialog()\n"
    "  - ret = dialog.select('Choose a playlist', ['Playlist #1', 'Playlist #2, 'Playlist #3'])\n");

  PyObject* Dialog_Select(PyObject *self, PyObject *args)
  {
    const int window = WINDOW_DIALOG_SELECT;
    PyObject *heading = NULL;
    PyObject *list = NULL;
    int autoClose = 0;

    if (!PyArg_ParseTuple(args, (char*)"OO|i", &heading, &list, &autoClose))  return NULL;
    if (!PyList_Check(list)) return NULL;

    PyXBMCGUILock();
    CGUIDialogSelect* pDialog= (CGUIDialogSelect*)g_windowManager.GetWindow(window);
    if (PyXBMCWindowIsNull(pDialog)) return NULL;

    pDialog->Reset();
    CStdString utf8Heading;
    if (heading && PyXBMCGetUnicodeString(utf8Heading, heading, 1))
      pDialog->SetHeading(utf8Heading);

    PyObject *listLine = NULL;
    for(int i = 0; i < PyList_Size(list); i++)
    {
      listLine = PyList_GetItem(list, i);
      CStdString utf8Line;
      if (listLine && PyXBMCGetUnicodeString(utf8Line, listLine, i))
        pDialog->Add(utf8Line);
    }
    if (autoClose > 0)
      pDialog->SetAutoClose(autoClose);

    PyXBMCGUIUnlock();

    //send message and wait for user input
    PyXBMCWaitForThreadMessage(TMSG_DIALOG_DOMODAL, window, ACTIVE_WINDOW);

    PyXBMCGUILock();
    int result = pDialog->GetSelectedLabel();
    PyXBMCGUIUnlock();
    return Py_BuildValue((char*)"i", result);
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
    "  - pDialog = xbmcgui.DialogProgress()\n"
    "  - ret = pDialog.create('XBMC', 'Initializing script...')\n");

  PyObject* Dialog_ProgressCreate(PyObject *self, PyObject *args)
  {
    PyObject* unicodeLine[4];
    for (int i = 0; i < 4; i++) unicodeLine[i] = NULL;

    // get lines, last 3 lines are optional.
    if (!PyArg_ParseTuple(args, (char*)"O|OOO", &unicodeLine[0], &unicodeLine[1], &unicodeLine[2], &unicodeLine[3]))  return NULL;

    string utf8Line[4];
    for (int i = 0; i < 4; i++)
    {
      if (unicodeLine[i] && !PyXBMCGetUnicodeString(utf8Line[i], unicodeLine[i], i+1))
        return NULL;
    }

    PyXBMCGUILock();
    CGUIDialogProgress* pDialog= (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (PyXBMCWindowIsNull(pDialog)) return NULL;
    ((DialogProgress*)self)->dlg = pDialog;

    pDialog->SetHeading(utf8Line[0]);

    for (int i = 1; i < 4; i++)
      pDialog->SetLine(i - 1,utf8Line[i]);

    PyXBMCGUIUnlock();
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
    if (!PyArg_ParseTuple(args, (char*)"i|OOO", &percentage,&unicodeLine[0], &unicodeLine[1], &unicodeLine[2]))  return NULL;

    string utf8Line[3];
    for (int i = 0; i < 3; i++)
    {
      if (unicodeLine[i] && !PyXBMCGetUnicodeString(utf8Line[i], unicodeLine[i], i+2))
        return NULL;
    }

    PyXBMCGUILock();
    CGUIDialogProgress* pDialog= ((DialogProgress*)self)->dlg;
    if (PyXBMCWindowIsNull(pDialog)) return NULL;

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
    PyXBMCGUIUnlock();

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
    CGUIDialogProgress* pDialog= ((DialogProgress*)self)->dlg;
    if (PyXBMCWindowIsNull(pDialog)) return NULL;

    PyXBMCGUILock();
    canceled = pDialog->IsCanceled();
    PyXBMCGUIUnlock();

    return Py_BuildValue((char*)"b", canceled);
  }

  PyDoc_STRVAR(close__doc__,
    "close() -- Close the progress dialog.\n"
    "\n"
    "example:\n"
    "  - pDialog.close()\n");


  PyObject* Dialog_ProgressClose(PyObject *self, PyObject *args)
  {
    CGUIDialogProgress* pDialog= ((DialogProgress*)self)->dlg;
    if (PyXBMCWindowIsNull(pDialog)) return NULL;

    PyXBMCGUILock();
    pDialog->Close();
    PyXBMCGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  static void Dialog_ProgressDealloc(PyObject *self)
  {
    CGUIDialogProgress* pDialog= ((DialogProgress*)self)->dlg;
    if(pDialog)
      pDialog->Close();

    self->ob_type->tp_free((PyObject*)self);
  }

  PyMethodDef WindowDialog_methods[] = {
    {NULL, NULL, 0, NULL}
  };

  /* xbmc Dialog functions for use in python */
  PyMethodDef Dialog_methods[] = {
    {(char*)"yesno", (PyCFunction)Dialog_YesNo, METH_VARARGS, yesno__doc__},
    {(char*)"select", (PyCFunction)Dialog_Select, METH_VARARGS, select__doc__},
    {(char*)"ok", (PyCFunction)Dialog_OK, METH_VARARGS, ok__doc__},
    {(char*)"browse", (PyCFunction)Dialog_Browse, METH_VARARGS, browse__doc__},
    {(char*)"numeric", (PyCFunction)Dialog_Numeric, METH_VARARGS, numeric__doc__},
    {NULL, NULL, 0, NULL}
  };

  /* xbmc progress Dialog functions for use in python */
  PyMethodDef DialogProgress_methods[] = {
    {(char*)"create", (PyCFunction)Dialog_ProgressCreate, METH_VARARGS, create__doc__},
    {(char*)"update", (PyCFunction)Dialog_ProgressUpdate, METH_VARARGS, update__doc__},
    {(char*)"close", (PyCFunction)Dialog_ProgressClose, METH_VARARGS, close__doc__},
    {(char*)"iscanceled", (PyCFunction)Dialog_ProgressIsCanceled, METH_VARARGS, isCanceled__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(windowDialog__doc__,
    "WindowDialog class.\n");

  PyDoc_STRVAR(dialog__doc__,
    "Dialog class.\n");

  PyDoc_STRVAR(dialogProgress__doc__,
    "DialogProgress class.\n");

// Restore code and data sections to normal.

  PyTypeObject WindowDialog_Type;

  void initWindowDialog_Type()
  {
    PyXBMCInitializeTypeObject(&WindowDialog_Type);

    WindowDialog_Type.tp_name = (char*)"xbmcgui.WindowDialog";
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
    PyXBMCInitializeTypeObject(&DialogProgress_Type);

    DialogProgress_Type.tp_name = (char*)"xbmcgui.DialogProgress";
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
    PyXBMCInitializeTypeObject(&Dialog_Type);

    Dialog_Type.tp_name = (char*)"xbmcgui.Dialog";
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


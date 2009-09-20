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

#include "PythonSettings.h"
#include "pyutil.h"
#include "GUIDialogPluginSettings.h"

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
  PyObject* Settings_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Settings *self;

    self = (Settings*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    static const char *keywords[] = { "path", NULL };
    char *cScriptPath = NULL;

    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"s",
      (char**)keywords,
      &cScriptPath
      ))
    {
      Py_DECREF(self);
      return NULL;
    };

    if (!CScriptSettings::SettingsExist(cScriptPath))
    {
      PyErr_SetString(PyExc_Exception, "No settings.xml file could be found!");
      return NULL;
    }

    self->pSettings = new CScriptSettings();
    self->pSettings->Clear();
    self->pSettings->Load(cScriptPath);

    return (PyObject*)self;
  }

  void Settings_Dealloc(Settings* self)
  {
    if (self->pSettings)
    {
      self->pSettings->Clear();
      delete self->pSettings;
    }
    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(getSetting__doc__,
    "getSetting(id) -- Returns the value of a setting as a string.\n"
    "\n"
    "id        : string - id of the setting that the module needs to access.\n"
    "\n"
    "*Note, You can use the above as a keyword.\n"
    "\n"
    "example:\n"
    "  - apikey = self.Settings.getSetting('apikey')\n");

  PyObject* Settings_GetSetting(Settings *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "id", NULL };
    char *id;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"s",
      (char**)keywords,
      &id
      ))
    {
      return NULL;
    };

    return Py_BuildValue((char*)"s", self->pSettings->Get(id).c_str());
  }

  PyDoc_STRVAR(setSetting__doc__,
    "setSetting(id, value) -- Sets a script setting.\n"
    "\n"
    "id        : string - id of the setting that the module needs to access.\n"
    "value     : string or unicode - value of the setting.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - self.Settings.setSetting(id='username', value='teamxbmc')\n");

  PyObject* Settings_SetSetting(Settings *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "id", "value", NULL };
    char *id;
    PyObject *pValue = NULL;

    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"sO",
      (char**)keywords,
      &id,
      &pValue
      ))
    {
      return NULL;
    };

    CStdString value;
    if (!id || !PyGetUnicodeString(value, pValue, 1))
    {
      PyErr_SetString(PyExc_ValueError, "Invalid id or value!");
      return NULL;
    }
    
    self->pSettings->Set(id, value);
    self->pSettings->Save();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(openSettings__doc__,
    "openSettings() -- Opens this scripts settings dialog.\n"
    "\n"
    "example:\n"
    "  - self.Settings.openSettings()\n");

  PyObject* Settings_OpenSettings(Settings *self, PyObject *args, PyObject *kwds)
  {
    // show settings dialog
    CStdString path = self->pSettings->getPath();
    CGUIDialogPluginSettings::ShowAndGetInput(path);

    // reload settings
    self->pSettings->Load(path);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef Settings_methods[] = {
    {(char*)"getSetting", (PyCFunction)Settings_GetSetting, METH_VARARGS|METH_KEYWORDS, getSetting__doc__},
    {(char*)"setSetting", (PyCFunction)Settings_SetSetting, METH_VARARGS|METH_KEYWORDS, setSetting__doc__},
    {(char*)"openSettings", (PyCFunction)Settings_OpenSettings, METH_VARARGS|METH_KEYWORDS, openSettings__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(settings__doc__,
    "Settings class.\n"
    "\n"
    "Settings(path) -- Creates a new Settings class.\n"
    "\n"
    "path            : string - path to script. (eg special://home/scripts/Apple Movie Trailers)\n"
    "\n"
    "*Note, settings folder structure is eg(resources/settings.xml)\n"
    "\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    " - self.Settings = xbmc.Settings(path=os.getcwd())\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject Settings_Type;

  void initSettings_Type()
  {
    PyInitializeTypeObject(&Settings_Type);

    Settings_Type.tp_name = (char*)"xbmc.Settings";
    Settings_Type.tp_basicsize = sizeof(Settings);
    Settings_Type.tp_dealloc = (destructor)Settings_Dealloc;
    Settings_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Settings_Type.tp_doc = settings__doc__;
    Settings_Type.tp_methods = Settings_methods;
    Settings_Type.tp_base = 0;
    Settings_Type.tp_new = Settings_New;
  }
}

#ifdef __cplusplus
}
#endif

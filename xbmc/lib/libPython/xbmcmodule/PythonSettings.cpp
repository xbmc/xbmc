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
#include "addons/AddonManager.h"
#include "GUIDialogAddonSettings.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

using ADDON::AddonPtr;
using ADDON::CAddonMgr;

namespace PYXBMC
{
  PyObject* Settings_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Settings *self;

    self = (Settings*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    static const char *keywords[] = { "uuid", NULL };
    char *cScriptUUID = NULL;

    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"s",
      (char**)keywords,
      &cScriptUUID
      ))
    {
      Py_DECREF(self);
      return NULL;
    };

    AddonPtr addon;
    if (CAddonMgr::Get()->GetAddon(CStdString(cScriptUUID), addon, ADDON::ADDON_SCRIPT))
    {
      PyErr_SetString(PyExc_Exception, "Could not get AddonPtr!");
      return NULL;
    }

    self->pAddon = addon.get();
    if (!self->pAddon->HasSettings())
    {
      PyErr_SetString(PyExc_Exception, "No settings.xml file could be found!");
      return NULL;
    }

    self->pAddon->LoadSettings();

    return (PyObject*)self;
  }

  void Settings_Dealloc(Settings* self)
  {
    //TODO is there anything that should be freed here, other than the AddonPtr?
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

    return Py_BuildValue((char*)"s", self->pAddon->GetSetting(id).c_str());
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
    if (!id || !PyXBMCGetUnicodeString(value, pValue, 1))
    {
      PyErr_SetString(PyExc_ValueError, "Invalid id or value!");
      return NULL;
    }

    self->pAddon->UpdateSetting(id, "", value);
    self->pAddon->SaveSettings();

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
    AddonPtr addon(self->pAddon);
    CGUIDialogAddonSettings::ShowAndGetInput(addon);

    // reload settings
    self->pAddon->LoadSettings();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef Settings_methods[] = {
    {(char*)"getSetting", (PyCFunction)Settings_GetSetting, METH_VARARGS|METH_KEYWORDS, getSetting__doc__},
    {(char*)"setSetting", (PyCFunction)Settings_SetSetting, METH_VARARGS|METH_KEYWORDS, setSetting__doc__},
    {(char*)"openSettings", (PyCFunction)Settings_OpenSettings, METH_VARARGS|METH_KEYWORDS, openSettings__doc__},
    {NULL, NULL, 0, NULL}
  };

  //FIXME!! incomplete docs
  PyDoc_STRVAR(settings__doc__,
    "Settings class.\n"
    "\n"
    "Settings(uuid) -- Creates a new Settings class.\n"
    "\n"
    "uuid           : string - UUID of this script.\n"
    "\n"
    "*Note, settings folder structure must match (resources/settings.xml)\n"
    "\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    " - self.Settings = xbmc.Settings(uuid=os.uuid())\n");

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
    PyXBMCInitializeTypeObject(&Settings_Type);

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

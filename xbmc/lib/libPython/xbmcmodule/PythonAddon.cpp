/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "PythonAddon.h"
#include "pyutil.h"
#include "addons/AddonManager.h"
#include "utils/CharsetConverter.h"
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
  PyObject* Addon_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Addon *self;

    self = (Addon*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    static const char *keywords[] = { "id", NULL };
    char *id = NULL;

    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"s",
      (char**)keywords,
      &id
      ))
    {
      Py_DECREF(self);
      return NULL;
    };

    if (!CAddonMgr::Get().GetAddon(id, self->pAddon))
    {
      PyErr_SetString(PyExc_Exception, "Could not get AddonPtr!");
      return NULL;
    }
    if (self->pAddon->HasSettings())
    {
      self->pAddon->LoadSettings();
    }

    return (PyObject*)self;
  }

  void Addon_Dealloc(Addon* self)
  {
    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(getLocalizedString__doc__,
    "getLocalizedString(id) -- Returns an addon's localized 'unicode string'.\n"
    "\n"
    "id             : integer - id# for string you want to localize.\n"
    "\n"
    "*Note, You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - locstr = self.Addon.getLocalizedString(id=6)\n");

  PyObject* Addon_GetLocalizedString(Addon *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "id", NULL };
    int id = -1;
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"i",
      (char**)keywords,
      &id
      ))
    {
      return NULL;
    };

    CStdStringW label;
    if (self->pAddon->Parent())
      g_charsetConverter.utf8ToW(self->pAddon->Parent()->GetString(id), label);
    else
      g_charsetConverter.utf8ToW(self->pAddon->GetString(id), label);

    return Py_BuildValue((char*)"u", label.c_str());
  }

    PyDoc_STRVAR(getSetting__doc__,
    "getSetting(id) -- Returns the value of a setting as a unicode string.\n"
    "\n"
    "id        : string - id of the setting that the module needs to access.\n"
    "\n"
    "*Note, You can use the above as a keyword.\n"
    "\n"
    "example:\n"
    "  - apikey = self.Addon.getSetting('apikey')\n");

  PyObject* Addon_GetSetting(Addon *self, PyObject *args, PyObject *kwds)
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

  PyObject* Addon_SetSetting(Addon *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "id", "value", NULL };
    char *id = NULL;
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

    self->pAddon->UpdateSetting(id, value);
    self->pAddon->SaveSettings();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(openSettings__doc__,
    "openSettings() -- Opens this scripts settings dialog.\n"
    "\n"
    "example:\n"
    "  - self.Settings.openSettings()\n");

  PyObject* Addon_OpenSettings(Addon *self, PyObject *args, PyObject *kwds)
  {
    // show settings dialog
    AddonPtr addon(self->pAddon);
    CGUIDialogAddonSettings::ShowAndGetInput(addon);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef Addon_methods[] = {
    {(char*)"getLocalizedString", (PyCFunction)Addon_GetLocalizedString, METH_VARARGS|METH_KEYWORDS, getLocalizedString__doc__},
    {(char*)"getSetting", (PyCFunction)Addon_GetSetting, METH_VARARGS|METH_KEYWORDS, getSetting__doc__},
    {(char*)"setSetting", (PyCFunction)Addon_SetSetting, METH_VARARGS|METH_KEYWORDS, setSetting__doc__},
    {(char*)"openSettings", (PyCFunction)Addon_OpenSettings, METH_VARARGS|METH_KEYWORDS, openSettings__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(addon__doc__,
    "Addon class.\n"
    "\n"
    "Addon(id) -- Creates a new Addon class.\n"
    "\n"
    "id          : string - id of the addon.\n"
    "\n"
    "*Note, You can use the above as a keyword.\n"
    "\n"
    "example:\n"
    " - self.Addon = xbmcaddon.Addon(id='script.recentlyadded')\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject Addon_Type;

  void initAddon_Type()
  {
    PyXBMCInitializeTypeObject(&Addon_Type);

    Addon_Type.tp_name = (char*)"xbmcaddon.Addon";
    Addon_Type.tp_basicsize = sizeof(Addon);
    Addon_Type.tp_dealloc = (destructor)Addon_Dealloc;
    Addon_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Addon_Type.tp_doc = addon__doc__;
    Addon_Type.tp_methods = Addon_methods;
    Addon_Type.tp_base = 0;
    Addon_Type.tp_new = Addon_New;
  }
}

#ifdef __cplusplus
}
#endif

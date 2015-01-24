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

    CStdString label = self->pAddon->GetString(id);

    return PyUnicode_DecodeUTF8(label.c_str(), label.size(), "replace");
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

    AddonPtr addon(self->pAddon);
    Py_BEGIN_ALLOW_THREADS
    addon->UpdateSetting(id, value);
    addon->SaveSettings();
    Py_END_ALLOW_THREADS

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
    Py_BEGIN_ALLOW_THREADS
    CGUIDialogAddonSettings::ShowAndGetInput(addon);
    Py_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getAddonInfo__doc__,
    "getAddonInfo(id) -- Returns the value of an addon property as a string.\n"
    "\n"
    "id        : string - id of the property that the module needs to access.\n"
    "\n"
    // Handle all props available
    "*Note, choices are (author, changelog, description, disclaimer, fanart. icon, id, name, path\n"
    "                    profile, stars, summary, type, version)\n"
    "\n"
    "       You can use the above as keywords for arguments.\n"
    "\n"
    "example:\n"
    "  - version = self.Addon.getAddonInfo('version')\n");

  PyObject* Addon_GetAddonInfo(Addon *self, PyObject *args, PyObject *kwds)
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

    if (strcmpi(id, "author") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Author().c_str());
    else if (strcmpi(id, "changelog") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->ChangeLog().c_str());
    else if (strcmpi(id, "description") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Description().c_str());
    else if (strcmpi(id, "disclaimer") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Disclaimer().c_str());
    else if (strcmpi(id, "fanart") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->FanArt().c_str());
    else if (strcmpi(id, "icon") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Icon().c_str());
    else if (strcmpi(id, "id") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->ID().c_str());
    else if (strcmpi(id, "name") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Name().c_str());
    else if (strcmpi(id, "path") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Path().c_str());
    else if (strcmpi(id, "profile") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Profile().c_str());
    else if (strcmpi(id, "stars") == 0)
      return Py_BuildValue((char*)"i", self->pAddon->Stars());
    else if (strcmpi(id, "summary") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Summary().c_str());
    else if (strcmpi(id, "type") == 0)
      return Py_BuildValue((char*)"s", ADDON::TranslateType(self->pAddon->Type()).c_str());
    else if (strcmpi(id, "version") == 0)
      return Py_BuildValue((char*)"s", self->pAddon->Version().str.c_str());
    else
    {
      CStdString error;
      error.Format("'%s' is an invalid Id", id);
      PyErr_SetString(PyExc_ValueError, error.c_str());
      return NULL;
    }
  }

  PyMethodDef Addon_methods[] = {
    {(char*)"getLocalizedString", (PyCFunction)Addon_GetLocalizedString, METH_VARARGS|METH_KEYWORDS, getLocalizedString__doc__},
    {(char*)"getSetting", (PyCFunction)Addon_GetSetting, METH_VARARGS|METH_KEYWORDS, getSetting__doc__},
    {(char*)"setSetting", (PyCFunction)Addon_SetSetting, METH_VARARGS|METH_KEYWORDS, setSetting__doc__},
    {(char*)"openSettings", (PyCFunction)Addon_OpenSettings, METH_VARARGS|METH_KEYWORDS, openSettings__doc__},
    {(char*)"getAddonInfo", (PyCFunction)Addon_GetAddonInfo, METH_VARARGS|METH_KEYWORDS, getAddonInfo__doc__},
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

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

#include "language.h"
#include "pyutil.h"
#include "Util.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"
#include "utils/CharsetConverter.h"

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
  PyObject* Language_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Language *self;

    self = (Language*)type->tp_alloc(type, 0);
    if (!self) return NULL;

    static const char *keywords[] = { "scriptPath", "defaultLanguage", NULL };
    char *cScriptPath = NULL;
    char *cDefaultLanguage = NULL;

    // parse arguments
    if (!PyArg_ParseTupleAndKeywords(
      args,
      kwds,
      (char*)"s|s",
      (char**)keywords,
      &cScriptPath,
      &cDefaultLanguage
      ))
    {
      Py_DECREF(self);
      return NULL;
    };
    self->pLanguage = new CLocalizeStrings();

    CStdString languagePath = cScriptPath;
    CStdString languageFallbackPath = languagePath;
    CStdString defaultLanguage;

    // set our default fallback language
    if (!cDefaultLanguage)
      defaultLanguage = "english";
    else
      defaultLanguage = cDefaultLanguage;

    // Path where the language strings reside
    CUtil::AddFileToFolder(languagePath, "resources", languagePath);
    CUtil::AddFileToFolder(languageFallbackPath, "resources", languageFallbackPath);
    CUtil::AddFileToFolder(languagePath, "language", languagePath);
    CUtil::AddFileToFolder(languageFallbackPath, "language", languageFallbackPath);
    CUtil::AddFileToFolder(languagePath, g_guiSettings.GetString("locale.language"), languagePath);
    CUtil::AddFileToFolder(languageFallbackPath, defaultLanguage, languageFallbackPath);
    CUtil::AddFileToFolder(languagePath, "strings.xml", languagePath);
    CUtil::AddFileToFolder(languageFallbackPath, "strings.xml", languageFallbackPath);

    // Load language strings
    self->pLanguage->Clear();
    self->pLanguage->Load(languagePath, languageFallbackPath);

    return (PyObject*)self;
  }

  void Language_Dealloc(Language* self)
  {
    if (self->pLanguage)
    {
      self->pLanguage->Clear();
      delete self->pLanguage;
    }
    self->ob_type->tp_free((PyObject*)self);
  }

  // getLocalizedString() method
  PyDoc_STRVAR(getLocalizedString__doc__,
    "getLocalizedString(id) -- Returns a localized 'unicode string'.\n"
    "\n"
    "id             : integer - id# for string you want to localize.\n"
    "\n"
    "*Note, getLocalizedString() will fallback to XBMC strings if no string found.\n"
    "\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    "  - locstr = self.Language.getLocalizedString(id=6)\n");

  PyObject* Language_GetLocalizedString(Language *self, PyObject *args, PyObject *kwds)
  {
    static const char *keywords[] = { "id", NULL };
    int id = -1;
    // parse arguments
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

    CStdStringW unicodeLabel;
    if (!self->pLanguage->Get(id))
      g_charsetConverter.utf8ToW(g_localizeStrings.Get(id), unicodeLabel);
    else
      g_charsetConverter.utf8ToW(self->pLanguage->Get(id), unicodeLabel);

    return Py_BuildValue((char*)"u", unicodeLabel.c_str());
  }

  PyMethodDef Language_methods[] = {
    {(char*)"getLocalizedString", (PyCFunction)Language_GetLocalizedString, METH_VARARGS|METH_KEYWORDS, getLocalizedString__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(language__doc__,
    "Language class.\n"
    "\n"
    "Language(scriptPath, defaultLanguage) -- Creates a new Language class.\n"
    "\n"
    "scriptPath      : string - path to script. (eg os.getcwd())\n"
    "defaultLanguage : [opt] string - default language to fallback to. (default=English)\n"
    "\n"
    "*Note, language folder structure is eg(language/English/strings.xml)\n"
    "\n"
    "       You can use the above as keywords for arguments and skip certain optional arguments.\n"
    "       Once you use a keyword, all following arguments require the keyword.\n"
    "\n"
    "example:\n"
    " - self.Language = xbmc.Language(os.getcwd())\n");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject Language_Type;

  void initLanguage_Type()
  {
    PyInitializeTypeObject(&Language_Type);

    Language_Type.tp_name = (char*)"xbmc.Language";
    Language_Type.tp_basicsize = sizeof(Language);
    Language_Type.tp_dealloc = (destructor)Language_Dealloc;
    Language_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Language_Type.tp_doc = language__doc__;
    Language_Type.tp_methods = Language_methods;
    Language_Type.tp_base = 0;
    Language_Type.tp_new = Language_New;
  }
}

#ifdef __cplusplus
}
#endif

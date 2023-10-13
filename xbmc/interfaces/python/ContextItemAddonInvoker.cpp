/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// python.h should always be included first before any other includes
#include "ContextItemAddonInvoker.h"

#include "interfaces/python/swig.h"
#include "utils/log.h"

#include <memory>

#include <Python.h>
#include <osdefs.h>

CContextItemAddonInvoker::CContextItemAddonInvoker(ILanguageInvocationHandler* invocationHandler,
                                                   const CFileItemPtr& item)
  : CAddonPythonInvoker(invocationHandler), m_item(std::make_shared<CFileItem>(*item.get()))
{
}

CContextItemAddonInvoker::~CContextItemAddonInvoker() = default;

void CContextItemAddonInvoker::onPythonModuleInitialization(void* moduleDict)
{
  CAddonPythonInvoker::onPythonModuleInitialization(moduleDict);
  if (m_item)
  {
    XBMCAddon::xbmcgui::ListItem* arg = new XBMCAddon::xbmcgui::ListItem(m_item);
    PyObject* pyItem = PythonBindings::makePythonInstance(arg, true);
    if (pyItem == Py_None || PySys_SetObject("listitem", pyItem) == -1)
    {
      CLog::Log(LOGERROR, "CPythonInvoker({}, {}): Failed to set sys parameter", GetId(),
                m_sourceFile);
      //FIXME: we should really abort execution
    }
  }
}

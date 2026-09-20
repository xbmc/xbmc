/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// python.h should always be included first before any other includes
#include "ContextItemAddonInvoker.h"

#include "interfaces/legacy/ListItem.h"
#include "utils/log.h"

#include <memory>

#include <Python.h>
#include <osdefs.h>

// Host-side wrap helper compiled into the xbmcgui module
extern "C" PyObject* KodiSwig_wrapListItem(XBMCAddon::xbmcgui::ListItem* obj, int owned);

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
    // the python wrapper holds the only reference and Releases on dealloc
    arg->Acquire();
    PyObject* pyItem = KodiSwig_wrapListItem(arg, 1);
    if (pyItem == Py_None || PySys_SetObject("listitem", pyItem) == -1)
    {
      CLog::Log(LOGERROR, "CPythonInvoker({}, {}): Failed to set sys parameter", GetId(),
                m_sourceFile);
      //FIXME: we should really abort execution
    }
  }
}

/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined TARGET_WINDOWS)
  #include "config.h"
#endif

// python.h should always be included first before any other includes
#include <Python.h>
#include <osdefs.h>

#include "system.h"
#include "ContextItemAddonInvoker.h"
#include "utils/log.h"
#include "interfaces/python/swig.h"


CContextItemAddonInvoker::CContextItemAddonInvoker(
    ILanguageInvocationHandler *invocationHandler,
    const CFileItemPtr& item)
  : CAddonPythonInvoker(invocationHandler), m_item(CFileItemPtr(new CFileItem(*item.get())))
{
}

CContextItemAddonInvoker::~CContextItemAddonInvoker()
{
}

void CContextItemAddonInvoker::onPythonModuleInitialization(void* moduleDict)
{
  CAddonPythonInvoker::onPythonModuleInitialization(moduleDict);
  if (m_item)
  {
    XBMCAddon::xbmcgui::ListItem* arg = new XBMCAddon::xbmcgui::ListItem(m_item);
    PyObject* pyItem = PythonBindings::makePythonInstance(arg, true);
    if (pyItem == Py_None || PySys_SetObject((char*)"listitem", pyItem) == -1)
    {
      CLog::Log(LOGERROR, "CPythonInvoker(%d, %s): Failed to set sys parameter", GetId(), m_sourceFile.c_str());
      //FIXME: we should really abort execution
    }
  }
}

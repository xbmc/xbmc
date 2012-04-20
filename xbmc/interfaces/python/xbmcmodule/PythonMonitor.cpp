/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "PythonMonitor.h"
#include "pyutil.h"
#include "pythreadstate.h"
#include "../XBPython.h"
#include "threads/Atomics.h"

using namespace PYXBMC;

struct SPyMonitor
{
  SPyMonitor(CPythonMonitor* monitor
         , const std::string &function, const std::string &arg = "")
  {
    m_monitor   = monitor;
    m_monitor->Acquire();
    m_function = function;
    m_arg = arg;
  }

  ~SPyMonitor()
  {
    m_monitor->Release();
  }

  std::string    m_function;
  std::string    m_arg;
  CPythonMonitor* m_monitor;
};

/*
 * called from python library!
 */
static int SPyMonitor_Function(void* e)
{
  SPyMonitor* object = (SPyMonitor*)e;
  PyObject* ret    = NULL;

  if(object->m_monitor->m_callback)
  { 
    if (!object->m_arg.empty())
      ret = PyObject_CallMethod(object->m_monitor->m_callback, (char*)object->m_function.c_str(), (char*)"(s)", object->m_arg.c_str());
    else
      ret = PyObject_CallMethod(object->m_monitor->m_callback, (char*)object->m_function.c_str(), NULL);
  }

  if(ret)
  {
    Py_DECREF(ret);
  }

  CPyThreadState pyState;
  delete object;

  return 0;

}

CPythonMonitor::CPythonMonitor()
{
  m_callback = NULL;
  m_refs     = 1;
  g_pythonParser.RegisterPythonMonitorCallBack(this);
}

void CPythonMonitor::Release()
{
  if(AtomicDecrement(&m_refs) == 0)
    delete this;
}

void CPythonMonitor::Acquire()
{
  AtomicIncrement(&m_refs);
}

CPythonMonitor::~CPythonMonitor(void)
{
  g_pythonParser.UnregisterPythonMonitorCallBack(this);
}

void CPythonMonitor::OnSettingsChanged()
{
  PyXBMC_AddPendingCall(m_state, SPyMonitor_Function, new SPyMonitor(this, "onSettingsChanged"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonMonitor::OnScreensaverActivated()
{
  PyXBMC_AddPendingCall(m_state, SPyMonitor_Function, new SPyMonitor(this, "onScreensaverActivated"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonMonitor::OnScreensaverDeactivated()
{
  PyXBMC_AddPendingCall(m_state, SPyMonitor_Function, new SPyMonitor(this, "onScreensaverDeactivated"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonMonitor::OnDatabaseUpdated(const std::string &database)
{
 PyXBMC_AddPendingCall(m_state, SPyMonitor_Function, new SPyMonitor(this, "onDatabaseUpdated", database));
 g_pythonParser.PulseGlobalEvent();
}

void CPythonMonitor::SetCallback(PyThreadState *state, PyObject *object)
{
  /* python lock should be held */
  m_callback = object;
  m_state    = state;
}

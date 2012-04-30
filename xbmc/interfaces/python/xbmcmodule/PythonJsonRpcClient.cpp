/*
 *      Copyright (C) 2012 Team XBMC
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

#include <string>

#include "PythonJsonRpcClient.h"
#include "pythreadstate.h"
#include "pyutil.h"
#include "../XBPython.h"
#include "settings/AdvancedSettings.h"
#include "threads/Atomics.h"

using namespace std;
using namespace ANNOUNCEMENT;
using namespace JSONRPC;

#ifdef HAS_JSONRPC

struct SPyJsonRpcClient
{
  SPyJsonRpcClient(CPythonJsonRpcClient *client, const char *function, const std::string &arg)
  {
    m_client = client;
    m_client->Acquire();
    m_function = function;
    m_arg = arg;
  }

  ~SPyJsonRpcClient()
  {
    m_client->Release();
  }

  const char* m_function;
  std::string m_arg;
  CPythonJsonRpcClient* m_client;
};

/*
 * called from python library!
 */
static int SPyJsonRpcClient_Function(void* e)
{
  SPyJsonRpcClient* object = (SPyJsonRpcClient*)e;
  PyObject* ret    = NULL;

  if (object->m_client->m_callback)
    ret = PyObject_CallMethod(object->m_client->m_callback, (char*)object->m_function, "(s)", object->m_arg.c_str());

  if (ret)
    Py_DECREF(ret);

  CPyThreadState pyState;
  delete object;

  return 0;
}

CPythonJsonRpcClient::CPythonJsonRpcClient()
  : m_callback(NULL),
    m_state(NULL),
    m_refs(1),
    m_announcementFlags(ANNOUNCE_ALL)
{
  g_pythonParser.RegisterPythonJsonRpcClientCallBack(this);
}

CPythonJsonRpcClient::~CPythonJsonRpcClient()
{
  g_pythonParser.UnregisterPythonJsonRpcClientCallBack(this);
}

void CPythonJsonRpcClient::Release()
{
  if(AtomicDecrement(&m_refs) == 0)
    delete this;
}

void CPythonJsonRpcClient::Acquire()
{
  AtomicIncrement(&m_refs);
}

void CPythonJsonRpcClient::SetCallback(PyThreadState *state, PyObject *object)
{
  /* python lock should be held */
  m_callback = object;
  m_state    = state;
}

void CPythonJsonRpcClient::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (m_callback == NULL ||
     (m_announcementFlags & flag) == 0)
    return;

  std::string strJson = AnnouncementToJSONRPC(flag, sender, message, data, g_advancedSettings.m_jsonOutputCompact);

  PyXBMC_AddPendingCall(m_state, SPyJsonRpcClient_Function, new SPyJsonRpcClient(this, "onNotification", strJson));
  g_pythonParser.PulseGlobalEvent();
}

#endif
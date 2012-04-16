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

#include "jsonrpcclient.h"
#include "pythreadstate.h"
#include "pyutil.h"

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  /* JsonRpcClient Fucntions */

  PyObject* JsonRpcClient_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    JsonRpcClient *self = (JsonRpcClient*)type->tp_alloc(type, 0);
    if (!self) 
      return NULL;
    
    self->pClient = new CPythonJsonRpcClient();
    if (self->pClient == NULL)
    {
      PyErr_SetString((PyObject*)self, "Unable to create JSON-RPC client");
      return NULL;
    }

    self->pClient->SetCallback(PyThreadState_Get(), (PyObject*)self);

    return (PyObject*)self;
  }

  void JsonRpcClient_Dealloc(JsonRpcClient* self)
  {
    if (self == NULL)
      return;

    if (self->pClient)
    {
      self->pClient->SetCallback(NULL, NULL);
      CPyThreadState pyState;
      self->pClient->Release();
      pyState.Restore();
      
      self->pClient = NULL;
    }

    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(execute__doc__,
    "execute(jsonrpccommand) -- Execute a JSON-RPC command.\n"
    "\n"
    "jsonrpccommand    : string - JSON-RPC command to execute.\n"
    "\n"
    "List of commands - \n"
    "\n"
    "example:\n"
    "  - response = xbmc.executeJSONRPC('{ \"jsonrpc\": \"2.0\", \"method\": \"JSONRPC.Introspect\", \"id\": 1 }')\n");

  PyObject* JsonRpcClient_execute(JsonRpcClient *self, PyObject *args)
  {
    if (self == NULL || self->pClient == NULL)
      return NULL;

    PyObject *requestObj = NULL;
    if (!PyArg_ParseTuple(args, (char*)"O", &requestObj))
      return NULL;

    CStdString request;
    if (requestObj == NULL || !PyXBMCGetUnicodeString(request, requestObj, 1))
      return NULL;

    CPythonJsonRpcTransport transport;
    return PyString_FromString(JSONRPC::CJSONRPC::MethodCall(request, &transport, self->pClient).c_str());
  }

  PyDoc_STRVAR(onnotification__doc__,
    "onNotification(json) -- callback function for JSON-RPC notifications.\n");

  PyObject* JsonRpcClient_onNotification(JsonRpcClient *self, PyObject *args)
  {
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef JsonRpcClient_methods[] = {
    {(char*)"execute", (PyCFunction)JsonRpcClient_execute, METH_VARARGS, execute__doc__},
    {(char*)"onNotification", (PyCFunction)JsonRpcClient_onNotification, METH_VARARGS, onnotification__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(jsonrpcclient__doc__,
    "JsonRpcClient class.\n"
    "\n"
    "Derive from this class and overwrite onNotification() to receive notifications.");

// Restore code and data sections to normal.

  PyTypeObject JsonRpcClient_Type;

  void initJsonRpcClient_Type()
  {
    PyXBMCInitializeTypeObject(&JsonRpcClient_Type);

    JsonRpcClient_Type.tp_name = (char*)"xbmc.JsonRpcClient";
    JsonRpcClient_Type.tp_basicsize = sizeof(JsonRpcClient);
    JsonRpcClient_Type.tp_dealloc = (destructor)JsonRpcClient_Dealloc;
    JsonRpcClient_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    JsonRpcClient_Type.tp_doc = jsonrpcclient__doc__;
    JsonRpcClient_Type.tp_methods = JsonRpcClient_methods;
    JsonRpcClient_Type.tp_base = 0;
    JsonRpcClient_Type.tp_new = JsonRpcClient_New;
  }
}

#ifdef __cplusplus
}
#endif
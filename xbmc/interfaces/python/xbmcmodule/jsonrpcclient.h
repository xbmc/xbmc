#pragma once
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

#include <Python.h>

#include "PythonJsonRpcClient.h"

#define JsonRpcClient_Check(op) PyObject_TypeCheck(op, &JsonRpcClient_Type)
#define JsonRpcClient_CheckExact(op) ((op)->ob_type == &JsonRpcClient_Type)

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  extern PyTypeObject JsonRpcClient_Type;

  typedef struct {
    PyObject_HEAD
    CPythonJsonRpcClient* pClient;
  } JsonRpcClient;

  void initJsonRpcClient_Type();

  PyObject* JsonRpcClient_New(PyTypeObject *type, PyObject *args, PyObject *kwds);
  PyObject* JsonRpcClient_execute(JsonRpcClient *self, PyObject *args);
}

#ifdef __cplusplus
}
#endif
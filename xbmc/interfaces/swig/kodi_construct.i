/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* C++ construction must run in tp_new, not tp_init.

   Under -builtin SWIG constructs in tp_init. A python subclass that defines
   __init__ and never calls super().__init__() therefore never constructs the
   C++ side, and the first method call dereferences a null pointer. Subclassing
   xbmc.Monitor exactly that way is the standard idiom in service addons, and
   it segfaults in SWIG_Python_ConvertPtrAndOwn.

   The shipped Groovy bindings construct in tp_new with a dummy tp_init, so
   this restores their behaviour rather than inventing one.

   The thunk has to be per class because it calls that class's own
   _wrap_new_<Name>; there is no untargeted form. */

%{
#include <Python.h>
%}

%wrapper %{
[[maybe_unused]] static int KodiSwig_tpInitNoop(PyObject*, PyObject*, PyObject*)
{
  return 0;
}

/* A subclass supplies its own __init__ arguments, which need not fit this
   constructor. Retry first without the keywords, then with nothing at all.

   Dropping the keywords is what the shipped bindings did: %feature("nokwds")
   selected PyArg_ParseTuple over PyArg_ParseTupleAndKeywords, and the former
   never inspects the keywords dict. Passing extra keywords to a WindowXML
   subclass for its own __init__ to read is the documented idiom, so without
   this those calls raise TypeError against a constructor that has silently
   ignored them for years. */
[[maybe_unused]] static PyObject* KodiSwig_tpNewCall(PyTypeObject* type, PyObject* args,
                                    PyObject* kwargs, initproc ctor)
{
  PyObject* self = type->tp_alloc(type, 0);
  if (!self)
    return NULL;
  if (ctor(self, args, kwargs) < 0)
  {
    if (type->tp_init == KodiSwig_tpInitNoop || !PyErr_ExceptionMatches(PyExc_TypeError))
    {
      Py_DECREF(self);
      return NULL;
    }
    PyErr_Clear();
    int rc = kwargs ? ctor(self, args, NULL) : -1;
    if (rc < 0)
    {
      PyErr_Clear();
      PyObject* empty = PyTuple_New(0);
      rc = ctor(self, empty, NULL);
      Py_DECREF(empty);
    }
    if (rc < 0)
    {
      Py_DECREF(self);
      return NULL;
    }
  }
  return self;
}
%}

%define KODI_CONSTRUCT(NS, NAME)
%feature("python:tp_new") NS::NAME "KodiTpNew_" #NAME;
%feature("python:tp_init") NS::NAME "KodiSwig_tpInitNoop";
// autodoc would install an __init__ descriptor that reactivates tp_init over the thunk above
%feature("noautodoc") NS::NAME::NAME;
%wrapper %{
static int _wrap_new_##NAME(PyObject*, PyObject*, PyObject*);
static PyObject* KodiTpNew_##NAME(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
  return KodiSwig_tpNewCall(type, args, kwargs, (initproc)_wrap_new_##NAME);
}
%}
%enddef

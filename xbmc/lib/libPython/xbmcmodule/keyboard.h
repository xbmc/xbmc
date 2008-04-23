#ifndef _LINUX
#include "lib/libPython/python/Python.h"
#else
#include <python2.4/Python.h>
#include "../XBPythonDll.h"
#endif
#include <string>
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD
    std::string strDefault;
    std::string strHeading;
    bool bHidden;
  } Keyboard;

  extern PyTypeObject Keyboard_Type;
  void initKeyboard_Type();
}

#ifdef __cplusplus
}
#endif

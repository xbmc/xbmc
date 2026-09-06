/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//! @todo upstream PR swig/swig#3540 makes Lib/linkruntime.c return this same
//! self-linked sentinel; once the required SWIG release has it, drop this file
//! and compile linkruntime.c against the -external-runtime header instead.

#include <Python.h>

#include "swigpyrun.h"

// self-linked empty ring head; modules attach at init, so under SWIG_LINK_RUNTIME type sharing needs no Python objects and works from any sub-interpreter
static swig_module_info sentinel = {0, 0, &sentinel, 0, 0, 0};

void* SWIG_ReturnGlobalTypeList(void*)
{
  return &sentinel;
}

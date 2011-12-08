/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "window.h"
#pragma once


class CGUIDialogProgress;

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  typedef struct {
    PyObject_HEAD
  } Dialog;

  typedef struct {
    PyObject_HEAD_XBMC_WINDOW
  } WindowDialog;

  struct DialogProgress {
    PyObject_HEAD
    CGUIDialogProgress* dlg;

    DialogProgress()
    {
      dlg = 0;
    }
  };

  extern PyTypeObject WindowDialog_Type;
  extern PyTypeObject DialogProgress_Type;
  extern PyTypeObject Dialog_Type;

  void initWindowDialog_Type();
  void initDialogProgress_Type();
  void initDialog_Type();
}

#ifdef __cplusplus
}
#endif

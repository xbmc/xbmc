#pragma once

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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
#include <Python.h>
#else
  #include "python/Include/Python.h"
#endif
#include "FileItem.h"

#define PlayList_Check(op) PyObject_TypeCheck(op, &PlayList_Type)
#define PlayList_CheckExact(op) ((op)->ob_type == &PlayList_Type)
#define PlayListItem_Check(op) PyObject_TypeCheck(op, &PlayListItem_Type)
#define PlayListItem_CheckExact(op) ((op)->ob_type == &PlayListItem_Type)

#ifdef __cplusplus
extern "C" {
#endif

namespace PLAYLIST
{
  class CPlayList;
}

namespace PYXBMC
{

  extern PyTypeObject PlayList_Type;
  extern PyTypeObject PlayListItem_Type;

  typedef struct {
    PyObject_HEAD
    PLAYLIST::CPlayList *pPlayList;
    int iPlayList;
  } PlayList;

  typedef struct {
    PyObject_HEAD
    CFileItemPtr item;
  } PlayListItem;

  void initPlayList_Type();
  void initPlayListItem_Type();

}

#ifdef __cplusplus
}
#endif

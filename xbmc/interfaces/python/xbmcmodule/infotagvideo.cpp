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

#include "infotagvideo.h"
#include "pyutil.h"

#ifndef __GNUC__
#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")
#endif

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  /*
   * allocate a new InfoTagVideo. Used for c++ and not the python user
   * returns a new reference
   */
  InfoTagVideo* InfoTagVideo_FromCVideoInfoTag(const CVideoInfoTag& infoTag)
  {
    InfoTagVideo* self = (InfoTagVideo*)InfoTagVideo_Type.tp_alloc(&InfoTagVideo_Type, 0);
    if (!self) return NULL;
    new(&self->infoTag) CVideoInfoTag();
    self->infoTag = infoTag;

    return self;
  }

  void InfoTagVideo_Dealloc(InfoTagVideo* self)
  {
    self->infoTag.~CVideoInfoTag();
    self->ob_type->tp_free((PyObject*)self);
  }

  // InfoTagVideo_GetDirector
  PyDoc_STRVAR(getDirector__doc__,
    "getDirector() -- returns a string.\n");

  PyObject* InfoTagVideo_GetDirector(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strDirector.c_str());
  }

  // InfoTagVideo_GetWritingCredits
  PyDoc_STRVAR(getWritingCredits__doc__,
    "getWritingCredits() -- returns a string.\n");

  PyObject* InfoTagVideo_GetWritingCredits(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strWritingCredits.c_str());
  }

  // InfoTagVideo_GetGenre
  PyDoc_STRVAR(getGenre__doc__,
    "getGenre() -- returns a string.\n");

  PyObject* InfoTagVideo_GetGenre(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strGenre.c_str());
  }

  // InfoTagVideo_GetTagLine
  PyDoc_STRVAR(getTagLine__doc__,
    "getTagLine() -- returns a string.\n");

  PyObject* InfoTagVideo_GetTagLine(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strTagLine.c_str());
  }

  // InfoTagVideo_GetPlotOutline
  PyDoc_STRVAR(getPlotOutline__doc__,
    "getPlotOutline() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPlotOutline(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strPlotOutline.c_str());
  }

  // InfoTagVideo_GetPlot
  PyDoc_STRVAR(getPlot__doc__,
    "getPlot() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPlot(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strPlot.c_str());
  }

  // InfoTagVideo_GetPictureURL
  PyDoc_STRVAR(getPictureURL__doc__,
    "getPictureURL() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPictureURL(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strPictureURL.GetFirstThumb().m_url.c_str());
  }

  // InfoTagVideo_GetTitle
  PyDoc_STRVAR(getTitle__doc__,
    "getTitle() -- returns a string.\n");

  PyObject* InfoTagVideo_GetTitle(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strTitle.c_str());
  }

  // InfoTagVideo_GetVotes
  PyDoc_STRVAR(getVotes__doc__,
    "getVotes() -- returns a string.\n");

  PyObject* InfoTagVideo_GetVotes(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strVotes.c_str());
  }

  // InfoTagVideo_GetCast
  PyDoc_STRVAR(getCast__doc__,
    "getCast() -- returns a string.\n");

  PyObject* InfoTagVideo_GetCast(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetCast(true).c_str());
  }

  // InfoTagVideo_GetFile
  PyDoc_STRVAR(getFile__doc__,
    "getFile() -- returns a string.\n");

  PyObject* InfoTagVideo_GetFile(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strFile.c_str());
  }

  // InfoTagVideo_GetPath
  PyDoc_STRVAR(getPath__doc__,
    "getPath() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPath(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strPath.c_str());
  }

  // InfoTagVideo_GetIMDBNumber
  PyDoc_STRVAR(getIMDBNumber__doc__,
    "getIMDBNumber() -- returns a string.\n");

  PyObject* InfoTagVideo_GetIMDBNumber(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_strIMDBNumber.c_str());
  }

  // InfoTagVideo_GetYear
  PyDoc_STRVAR(getYear__doc__,
    "getYear() -- returns a integer.\n");

  PyObject* InfoTagVideo_GetYear(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.m_iYear);
  }

  // InfoTagVideo_GetRating
  PyDoc_STRVAR(getRating__doc__,
    "getRating() -- returns a float.\n");

  PyObject* InfoTagVideo_GetRating(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"f", self->infoTag.m_fRating);
  }

  // InfoTagVideo_GetPlayCount
  PyDoc_STRVAR(getPlayCount__doc__,
    "getPlayCount() -- returns a integer.\n");

  PyObject* InfoTagVideo_GetPlayCount(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.m_playCount);
  }

  // InfoTagVideo_GetLastPlayed
  PyDoc_STRVAR(getLastPlayed__doc__,
    "getLastPlayed() -- returns a string.\n");

  PyObject* InfoTagVideo_GetLastPlayed(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.m_lastPlayed.c_str());
  }

  PyMethodDef InfoTagVideo_methods[] = {
    {(char*)"getDirector", (PyCFunction)InfoTagVideo_GetDirector, METH_VARARGS, getDirector__doc__},
    {(char*)"getWritingCredits", (PyCFunction)InfoTagVideo_GetWritingCredits, METH_VARARGS, getWritingCredits__doc__},
    {(char*)"getGenre", (PyCFunction)InfoTagVideo_GetGenre, METH_VARARGS, getGenre__doc__},
    {(char*)"getTagLine", (PyCFunction)InfoTagVideo_GetTagLine, METH_VARARGS, getTagLine__doc__},
    {(char*)"getPlotOutline", (PyCFunction)InfoTagVideo_GetPlotOutline, METH_VARARGS, getPlotOutline__doc__},
    {(char*)"getPlot", (PyCFunction)InfoTagVideo_GetPlot, METH_VARARGS, getPlot__doc__},
    {(char*)"getPictureURL", (PyCFunction)InfoTagVideo_GetPictureURL, METH_VARARGS, getPictureURL__doc__},
    {(char*)"getTitle", (PyCFunction)InfoTagVideo_GetTitle, METH_VARARGS, getTitle__doc__},
    {(char*)"getVotes", (PyCFunction)InfoTagVideo_GetVotes, METH_VARARGS, getVotes__doc__},
    {(char*)"getCast", (PyCFunction)InfoTagVideo_GetCast, METH_VARARGS, getCast__doc__},
    {(char*)"getFile", (PyCFunction)InfoTagVideo_GetFile, METH_VARARGS, getFile__doc__},
    {(char*)"getPath", (PyCFunction)InfoTagVideo_GetPath, METH_VARARGS, getPath__doc__},
    {(char*)"getIMDBNumber", (PyCFunction)InfoTagVideo_GetIMDBNumber, METH_VARARGS, getIMDBNumber__doc__},
    {(char*)"getYear", (PyCFunction)InfoTagVideo_GetYear, METH_VARARGS, getYear__doc__},
    {(char*)"getRating", (PyCFunction)InfoTagVideo_GetRating, METH_VARARGS, getRating__doc__},
    {(char*)"getPlayCount", (PyCFunction)InfoTagVideo_GetPlayCount, METH_VARARGS, getPlayCount__doc__},
    {(char*)"getLastPlayed", (PyCFunction)InfoTagVideo_GetLastPlayed, METH_VARARGS, getLastPlayed__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(videoInfoTag__doc__,
    "InfoTagVideo class.\n"
    "\n"
    "");

// Restore code and data sections to normal.
#ifndef __GNUC__
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

  PyTypeObject InfoTagVideo_Type;

  void initInfoTagVideo_Type()
  {
    PyXBMCInitializeTypeObject(&InfoTagVideo_Type);

    InfoTagVideo_Type.tp_name = (char*)"xbmc.InfoTagVideo";
    InfoTagVideo_Type.tp_basicsize = sizeof(InfoTagVideo);
    InfoTagVideo_Type.tp_dealloc = (destructor)InfoTagVideo_Dealloc;
    InfoTagVideo_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    InfoTagVideo_Type.tp_doc = videoInfoTag__doc__;
    InfoTagVideo_Type.tp_methods = InfoTagVideo_methods;
    InfoTagVideo_Type.tp_base = 0;
    InfoTagVideo_Type.tp_new = 0;
  }
}

#ifdef __cplusplus
}
#endif

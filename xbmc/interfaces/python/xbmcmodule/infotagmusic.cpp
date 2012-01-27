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

#include "infotagmusic.h"
#include "pyutil.h"


#ifdef __cplusplus
extern "C" {
#endif

using namespace MUSIC_INFO;

namespace PYXBMC
{
  /*
   * allocate a new InfoTagMusic. Used for c++ and not the python user
   * returns a new reference
   */
  InfoTagMusic* InfoTagMusic_FromCMusicInfoTag(const MUSIC_INFO::CMusicInfoTag& infoTag)
  {
    InfoTagMusic* self = (InfoTagMusic*)InfoTagMusic_Type.tp_alloc(&InfoTagMusic_Type, 0);
    if (!self) return NULL;
    new(&self->infoTag) MUSIC_INFO::CMusicInfoTag();
    self->infoTag = infoTag;

    return self;
  }

  void InfoTagMusic_Dealloc(InfoTagMusic* self)
  {
    self->infoTag.~CMusicInfoTag();
    self->ob_type->tp_free((PyObject*)self);
  }

  // InfoTagMusic_GetURL
  PyDoc_STRVAR(getURL__doc__,
    "getURL() -- returns a string.\n");

  PyObject* InfoTagMusic_GetURL(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetURL().c_str());
  }

  // InfoTagMusic_GetTitle
  PyDoc_STRVAR(getTitle__doc__,
    "getTitle() -- returns a string.\n");

  PyObject* InfoTagMusic_GetTitle(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetTitle().c_str());
  }

  // InfoTagMusic_GetArtist
  PyDoc_STRVAR(getArtist__doc__,
    "getArtist() -- returns a string.\n");

  PyObject* InfoTagMusic_GetArtist(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetArtist().c_str());
  }

  // InfoTagMusic_GetAlbumArtist
  PyDoc_STRVAR(getAlbumArtist__doc__,
    "getAlbumArtist() -- returns a string.\n");

  PyObject* InfoTagMusic_GetAlbumArtist(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetAlbumArtist().c_str());
  }

  // InfoTagMusic_GetAlbum
  PyDoc_STRVAR(getAlbum__doc__,
    "getAlbum() -- returns a string.\n");

  PyObject* InfoTagMusic_GetAlbum(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetAlbum().c_str());
  }

  // InfoTagMusic_GetGenre
  PyDoc_STRVAR(getGenre__doc__,
    "getAlbum() -- returns a string.\n");

  PyObject* InfoTagMusic_GetGenre(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetGenre().c_str());
  }

  // InfoTagMusic_GetDuration
  PyDoc_STRVAR(getDuration__doc__,
    "getDuration() -- returns an integer.\n");

  PyObject* InfoTagMusic_GetDuration(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.GetDuration());
  }

  // InfoTagMusic_GetTrack
  PyDoc_STRVAR(getTrack__doc__,
    "getTrack() -- returns an integer.\n");

  PyObject* InfoTagMusic_GetTrack(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.GetTrackNumber());
  }

  // InfoTagMusic_GetDisc
  PyDoc_STRVAR(getDisc__doc__,
    "getDisc() -- returns an integer.\n");

  PyObject* InfoTagMusic_GetDisc(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.GetDiscNumber());
  }

  // InfoTagMusic_GetTrackAndDisc
  PyDoc_STRVAR(getTrackAndDisc__doc__,
    "getTrackAndDisc() -- returns an integer.\n");

  PyObject* InfoTagMusic_GetTrackAndDisc(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.GetTrackAndDiskNumber());
  }

  // InfoTagMusic_ReleaseDate
  PyDoc_STRVAR(getReleaseDate__doc__,
    "getReleaseDate() -- returns a string.\n");

  PyObject* InfoTagMusic_GetReleaseDate(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetYearString().c_str());
  }

  // InfoTagMusic_GetListeners
  PyDoc_STRVAR(getListeners__doc__,
    "getListeners() -- returns an integer.\n");

  PyObject* InfoTagMusic_GetListeners(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.GetListeners());
  }

  // InfoTagMusic_GetPlayCount
  PyDoc_STRVAR(getPlayCount__doc__,
    "getPlayCount() -- returns an integer.\n");

  PyObject* InfoTagMusic_GetPlayCount(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", self->infoTag.GetPlayCount());
  }

  // InfoTagMusic_GetLastPlayed
  PyDoc_STRVAR(getLastPlayed__doc__,
    "getLastPlayed() -- returns a string.\n");

  PyObject* InfoTagMusic_GetLastPlayed(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetLastPlayed().c_str());
  }

  // InfoTagMusic_GetComment
  PyDoc_STRVAR(getComment__doc__,
    "getComment() -- returns a string.\n");

  PyObject* InfoTagMusic_GetComment(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetComment().c_str());
  }

  // InfoTagMusic_GetLyrics
  PyDoc_STRVAR(getLyrics__doc__,
    "getLyrics() -- returns a string.\n");

  PyObject* InfoTagMusic_GetLyrics(InfoTagMusic *self, PyObject *args)
  {
    return Py_BuildValue((char*)"s", self->infoTag.GetLyrics().c_str());
  }

  PyMethodDef InfoTagMusic_methods[] = {
    {(char*)"getURL", (PyCFunction)InfoTagMusic_GetURL, METH_VARARGS, getURL__doc__},
    {(char*)"getTitle", (PyCFunction)InfoTagMusic_GetTitle, METH_VARARGS, getTitle__doc__},
    {(char*)"getAlbum", (PyCFunction)InfoTagMusic_GetAlbum, METH_VARARGS, getAlbum__doc__},
    {(char*)"getArtist", (PyCFunction)InfoTagMusic_GetArtist, METH_VARARGS, getArtist__doc__},
    {(char*)"getAlbumArtist", (PyCFunction)InfoTagMusic_GetAlbumArtist, METH_VARARGS, getAlbumArtist__doc__},
    {(char*)"getGenre", (PyCFunction)InfoTagMusic_GetGenre, METH_VARARGS, getGenre__doc__},
    {(char*)"getDuration", (PyCFunction)InfoTagMusic_GetDuration, METH_VARARGS, getDuration__doc__},
    {(char*)"getTrack", (PyCFunction)InfoTagMusic_GetTrack, METH_VARARGS, getTrack__doc__},
    {(char*)"getDisc", (PyCFunction)InfoTagMusic_GetDisc, METH_VARARGS, getDisc__doc__},
    {(char*)"getTrackAndDisc", (PyCFunction)InfoTagMusic_GetTrackAndDisc, METH_VARARGS, getTrackAndDisc__doc__},
    {(char*)"getReleaseDate", (PyCFunction)InfoTagMusic_GetReleaseDate, METH_VARARGS, getReleaseDate__doc__},
    {(char*)"getListeners", (PyCFunction)InfoTagMusic_GetListeners, METH_VARARGS, getListeners__doc__},
    {(char*)"getPlayCount", (PyCFunction)InfoTagMusic_GetPlayCount, METH_VARARGS, getPlayCount__doc__},
    {(char*)"getLastPlayed", (PyCFunction)InfoTagMusic_GetLastPlayed, METH_VARARGS, getLastPlayed__doc__},
    {(char*)"getComment", (PyCFunction)InfoTagMusic_GetComment, METH_VARARGS, getComment__doc__},
    {(char*)"getLyrics", (PyCFunction)InfoTagMusic_GetLyrics, METH_VARARGS, getLyrics__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(musicInfoTag__doc__,
    "InfoTagMusic class.\n"
    "\n"
    "");

// Restore code and data sections to normal.

  PyTypeObject InfoTagMusic_Type;

  void initInfoTagMusic_Type()
  {
    PyXBMCInitializeTypeObject(&InfoTagMusic_Type);

    InfoTagMusic_Type.tp_name = (char*)"xbmc.InfoTagMusic";
    InfoTagMusic_Type.tp_basicsize = sizeof(InfoTagMusic);
    InfoTagMusic_Type.tp_dealloc = (destructor)InfoTagMusic_Dealloc;
    InfoTagMusic_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    InfoTagMusic_Type.tp_doc = musicInfoTag__doc__;
    InfoTagMusic_Type.tp_methods = InfoTagMusic_methods;
    InfoTagMusic_Type.tp_base = 0;
    InfoTagMusic_Type.tp_new = 0;
  }
}

#ifdef __cplusplus
}
#endif

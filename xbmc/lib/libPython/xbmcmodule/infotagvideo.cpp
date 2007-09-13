#include "stdafx.h"
#include "infotagvideo.h"
#include "pyutil.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

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
    return Py_BuildValue("s", self->infoTag.m_strDirector.c_str());
  }

  // InfoTagVideo_GetWritingCredits
  PyDoc_STRVAR(getWritingCredits__doc__,
    "getWritingCredits() -- returns a string.\n");

  PyObject* InfoTagVideo_GetWritingCredits(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strWritingCredits.c_str());
  }

  // InfoTagVideo_GetGenre
  PyDoc_STRVAR(getGenre__doc__,
    "getGenre() -- returns a string.\n");

  PyObject* InfoTagVideo_GetGenre(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strGenre.c_str());
  }

  // InfoTagVideo_GetTagLine
  PyDoc_STRVAR(getTagLine__doc__,
    "getTagLine() -- returns a string.\n");

  PyObject* InfoTagVideo_GetTagLine(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strTagLine.c_str());
  }

  // InfoTagVideo_GetPlotOutline
  PyDoc_STRVAR(getPlotOutline__doc__,
    "getPlotOutline() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPlotOutline(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strPlotOutline.c_str());
  }

  // InfoTagVideo_GetPlot
  PyDoc_STRVAR(getPlot__doc__,
    "getPlot() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPlot(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strPlot.c_str());
  }

  // InfoTagVideo_GetPictureURL
  PyDoc_STRVAR(getPictureURL__doc__,
    "getPictureURL() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPictureURL(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strPictureURL.GetFirstThumb().m_url.c_str());
  }

  // InfoTagVideo_GetTitle
  PyDoc_STRVAR(getTitle__doc__,
    "getTitle() -- returns a string.\n");

  PyObject* InfoTagVideo_GetTitle(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strTitle.c_str());
  }

  // InfoTagVideo_GetVotes
  PyDoc_STRVAR(getVotes__doc__,
    "getVotes() -- returns a string.\n");

  PyObject* InfoTagVideo_GetVotes(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strVotes.c_str());
  }

  // InfoTagVideo_GetCast
  PyDoc_STRVAR(getCast__doc__,
    "getCast() -- returns a string.\n");

  PyObject* InfoTagVideo_GetCast(InfoTagVideo *self, PyObject *args)
  {
    CStdString cast = self->infoTag.GetCast(true);
    /*for (CVideoInfoTag::iCast it = self->infoTag.m_cast.begin(); it != self->infoTag.m_cast.end(); ++it)
    {
      CStdString character;
      character.Format("%s %s %s\n", it->first.c_str(), g_localizeStrings.Get(20347).c_str(), it->second.c_str());
      cast += character;
    }*/
    return Py_BuildValue("s", cast.c_str());
  }

  // InfoTagVideo_GetFile
  PyDoc_STRVAR(getFile__doc__,
    "getFile() -- returns a string.\n");

  PyObject* InfoTagVideo_GetFile(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strFile.c_str());
  }

  // InfoTagVideo_GetPath
  PyDoc_STRVAR(getPath__doc__,
    "getPath() -- returns a string.\n");

  PyObject* InfoTagVideo_GetPath(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strPath.c_str());
  }

  /*// InfoTagVideo_GetDVDLabel
  PyDoc_STRVAR(getDVDLabel__doc__,
    "getDVDLabel() -- returns a string.\n");

  PyObject* InfoTagVideo_GetDVDLabel(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strDVDLabel.c_str());
  }
  */

  // InfoTagVideo_GetIMDBNumber
  PyDoc_STRVAR(getIMDBNumber__doc__,
    "getIMDBNumber() -- returns a string.\n");

  PyObject* InfoTagVideo_GetIMDBNumber(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("s", self->infoTag.m_strIMDBNumber.c_str());
  }

  // InfoTagVideo_GetIMDBNumber
  PyDoc_STRVAR(getYear__doc__,
    "getYear() -- returns a integer.\n");

  PyObject* InfoTagVideo_GetYear(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("i", self->infoTag.m_iYear);
  }

  // InfoTagVideo_GetIMDBNumber
  PyDoc_STRVAR(getRating__doc__,
    "getRating() -- returns a float.\n");

  PyObject* InfoTagVideo_GetRating(InfoTagVideo *self, PyObject *args)
  {
    return Py_BuildValue("f", self->infoTag.m_fRating);
  }

  PyMethodDef InfoTagVideo_methods[] = {
    {"getDirector", (PyCFunction)InfoTagVideo_GetDirector, METH_VARARGS, getDirector__doc__},
    {"getWritingCredits", (PyCFunction)InfoTagVideo_GetWritingCredits, METH_VARARGS, getWritingCredits__doc__},
    {"getGenre", (PyCFunction)InfoTagVideo_GetGenre, METH_VARARGS, getGenre__doc__},
    {"getTagLine", (PyCFunction)InfoTagVideo_GetTagLine, METH_VARARGS, getTagLine__doc__},
    {"getPlotOutline", (PyCFunction)InfoTagVideo_GetPlotOutline, METH_VARARGS, getPlotOutline__doc__},
    {"getPlot", (PyCFunction)InfoTagVideo_GetPlot, METH_VARARGS, getPlot__doc__},
    {"getPictureURL", (PyCFunction)InfoTagVideo_GetPictureURL, METH_VARARGS, getPictureURL__doc__},
    {"getTitle", (PyCFunction)InfoTagVideo_GetTitle, METH_VARARGS, getTitle__doc__},
    {"getVotes", (PyCFunction)InfoTagVideo_GetVotes, METH_VARARGS, getVotes__doc__},
    {"getCast", (PyCFunction)InfoTagVideo_GetCast, METH_VARARGS, getCast__doc__},
    {"getFile", (PyCFunction)InfoTagVideo_GetFile, METH_VARARGS, getFile__doc__},
    //{"getDVDLabel", (PyCFunction)InfoTagVideo_GetDVDLabel, METH_VARARGS, getDVDLabel__doc__},
    {"getIMDBNumber", (PyCFunction)InfoTagVideo_GetIMDBNumber, METH_VARARGS, getIMDBNumber__doc__},
    {"getYear", (PyCFunction)InfoTagVideo_GetYear, METH_VARARGS, getYear__doc__},
    {"getRating", (PyCFunction)InfoTagVideo_GetRating, METH_VARARGS, getRating__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(videoInfoTag__doc__,
    "InfoTagVideo class.\n"
    "\n"
    "");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject InfoTagVideo_Type;

  void initInfoTagVideo_Type()
  {
    PyInitializeTypeObject(&InfoTagVideo_Type);

    InfoTagVideo_Type.tp_name = "xbmc.InfoTagVideo";
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

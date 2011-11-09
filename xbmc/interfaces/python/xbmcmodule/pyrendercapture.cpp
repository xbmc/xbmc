/*
 *      Copyright (C) 2011 Team XBMC
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

#include "pyrendercapture.h"

#ifdef HAS_PYRENDERCAPTURE

#include "pyutil.h"
#include "pythreadstate.h"
#include "cores/VideoRenderers/RenderManager.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  PyObject* RenderCapture_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    RenderCapture *self = (RenderCapture*)type->tp_alloc(type, 0);
    if (!self)
      return NULL;

    self->capture = g_renderManager.AllocRenderCapture();
    if (!self->capture)
    {
        self->ob_type->tp_free((PyObject*)self);
        return PyErr_NoMemory();
    }

    return (PyObject*)self;
  }

  void RenderCapture_Dealloc(RenderCapture* self)
  {
    g_renderManager.ReleaseRenderCapture(self->capture);
    self->ob_type->tp_free((PyObject*)self);
  }

  // RenderCapture_GetWidth
  PyDoc_STRVAR(getWidth__doc__,
    "getWidth() -- returns width of captured image.\n");

  PyObject* RenderCapture_GetWidth(RenderCapture *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", (int)self->capture->GetWidth());
  }

  // RenderCapture_GetHeight
  PyDoc_STRVAR(getHeight__doc__,
    "getHeight() -- returns height of captured image.\n");

  PyObject* RenderCapture_GetHeight(RenderCapture *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", (int)self->capture->GetHeight());
  }

  // RenderCapture_GetCaptureState
  PyDoc_STRVAR(getCaptureState__doc__,
    "getCaptureState() -- returns processing state of capture request.\n"
    "\n"
    "The returned value could be compared against the following constants:\n"
    "xbmc.CAPTURE_STATE_WORKING  : Capture request in progress.\n"
    "xbmc.CAPTURE_STATE_DONE     : Capture request done. The image could be retrieved with getImage()\n"
    "xbmc.CAPTURE_STATE_FAILED   : Capture request failed.\n");

  PyObject* RenderCapture_GetCaptureState(RenderCapture *self, PyObject *args)
  {
    return Py_BuildValue((char*)"i", (int)self->capture->GetUserState());
  }

  // RenderCapture_GetAspectRatio
  PyDoc_STRVAR(getAspectRatio__doc__,
    "getAspectRatio() -- returns aspect ratio of currently displayed video.\n");

  PyObject* RenderCapture_GetAspectRatio(RenderCapture *self, PyObject *args)
  {
    return Py_BuildValue((char*)"f", g_renderManager.GetAspectRatio());
  }

  // RenderCapture_PixelFormat
  PyDoc_STRVAR(getImageFormat__doc__,
    "getImageFormat() -- returns format of captured image: 'BGRA' or 'RGBA'.\n");

  PyObject* RenderCapture_GetImageFormat(RenderCapture *self, PyObject *args)
  {
    if (self->capture->GetCaptureFormat() == CAPTUREFORMAT_BGRA)
      return Py_BuildValue((char*)"s", "BGRA");
    else if (self->capture->GetCaptureFormat() == CAPTUREFORMAT_RGBA)
      return Py_BuildValue((char*)"s", "RGBA");

    Py_INCREF(Py_None);
    return Py_None;
  }

  // RenderCapture_GetImage
  PyDoc_STRVAR(getImage__doc__,
    "getImage() -- returns captured image as a bytearray.\n"
    "\n"
    "The size of the image is getWidth() * getHeight() * 4\n");

  PyObject* RenderCapture_GetImage(RenderCapture *self, PyObject *args)
  {
    if (self->capture->GetUserState() != CAPTURESTATE_DONE)
    {
      PyErr_SetString(PyExc_SystemError, "illegal user state");
      return NULL;
    }

    Py_ssize_t size = self->capture->GetWidth() * self->capture->GetHeight() * 4;
    return PyByteArray_FromStringAndSize((const char *)self->capture->GetPixels(), size);
  }

  // RenderCapture_Capture
  PyDoc_STRVAR(capture__doc__,
    "capture(width, height [, flags]) -- issue capture request.\n"
    "\n"
    "width    : Width capture image should be rendered to\n"
    "height   : Height capture image should should be rendered to\n"
    "flags    : Optional. Flags that control the capture processing.\n"
    "\n"
    "The value for 'flags' could be or'ed from the following constants:\n"
    "xbmc.CAPTURE_FLAG_CONTINUOUS    : after a capture is done, issue a new capture request immediately\n"
    "xbmc.CAPTURE_FLAG_IMMEDIATELY   : read out immediately when capture() is called, this can cause a busy wait\n");

  PyObject* RenderCapture_Capture(RenderCapture *self, PyObject *args)
  {
    if (self->capture->GetUserState() != CAPTURESTATE_DONE && self->capture->GetUserState() != CAPTURESTATE_FAILED)
    {
      PyErr_SetString(PyExc_SystemError, "illegal user state");
      return NULL;
    }

    int width, height, flags = 0;
    if (!PyArg_ParseTuple(args, "ii|i", &width, &height, &flags))
      return NULL;

    CPyThreadState threadstate;
    g_renderManager.Capture(self->capture, (unsigned int)width, (unsigned int)height, flags);
    threadstate.Restore();

    Py_INCREF(Py_None);
    return Py_None;
  }

  // RenderCapture_WaitForCaptureStateChangeEvent
  PyDoc_STRVAR(waitForCaptureStateChangeEvent__doc__,
    "waitForCaptureStateChangeEvent([msecs]) -- wait for capture state change event.\n"
    "\n"
    "msecs     : Milliseconds to wait. Waits forever if not specified.\n"
    "\n"
    "The method will return 1 if the Event was triggered. Otherwise it will return 0.\n");

  PyObject* RenderCapture_WaitForCaptureStateChangeEvent(RenderCapture *self, PyObject *args)
  {
    bool rc;
    int msecs = 0;
    if (!PyArg_ParseTuple(args, "|i", &msecs))
      return NULL;

    CPyThreadState threadstate;
    if (msecs)
      rc = self->capture->GetEvent().WaitMSec((unsigned int)msecs);
    else
      rc = self->capture->GetEvent().Wait();
    threadstate.Restore();

    return Py_BuildValue((char*)"i", (rc ? 1: 0));
  }


  PyMethodDef RenderCapture_methods[] = {
    {(char*)"getWidth", (PyCFunction)RenderCapture_GetWidth, METH_VARARGS, getWidth__doc__},
    {(char*)"getHeight", (PyCFunction)RenderCapture_GetHeight, METH_VARARGS, getHeight__doc__},
    {(char*)"getCaptureState", (PyCFunction)RenderCapture_GetCaptureState, METH_VARARGS, getCaptureState__doc__},
    {(char*)"getAspectRatio", (PyCFunction)RenderCapture_GetAspectRatio, METH_VARARGS, getAspectRatio__doc__},
    {(char*)"getImageFormat", (PyCFunction)RenderCapture_GetImageFormat, METH_VARARGS, getImageFormat__doc__},
    {(char*)"getImage", (PyCFunction)RenderCapture_GetImage, METH_VARARGS, getImage__doc__},
    {(char*)"capture", (PyCFunction)RenderCapture_Capture, METH_VARARGS, capture__doc__},
    {(char*)"waitForCaptureStateChangeEvent", (PyCFunction)RenderCapture_WaitForCaptureStateChangeEvent, METH_VARARGS, waitForCaptureStateChangeEvent__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(RenderCapture__doc__,
    "RenderCapture class.\n"
    "\n"
    "Capture images of abitrary size of the currently displayed video stream.\n");

// Restore code and data sections to normal.

  PyTypeObject RenderCapture_Type;

  void initRenderCapture_Type()
  {
    PyXBMCInitializeTypeObject(&RenderCapture_Type);

    RenderCapture_Type.tp_name      = (char*)"xbmc.RenderCapture";
    RenderCapture_Type.tp_basicsize = sizeof(RenderCapture);
    RenderCapture_Type.tp_dealloc   = (destructor)RenderCapture_Dealloc;
    RenderCapture_Type.tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    RenderCapture_Type.tp_doc       = RenderCapture__doc__;
    RenderCapture_Type.tp_methods   = RenderCapture_methods;
    RenderCapture_Type.tp_base      = 0;
    RenderCapture_Type.tp_new       = RenderCapture_New;
  }
}

#ifdef __cplusplus
}
#endif

#endif //HAS_PYRENDERCAPTURE


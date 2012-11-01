/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#pragma once

#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/RenderCapture.h"
#include "AddonClass.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    class RenderCapture : public AddonClass
    {
      CRenderCapture* m_capture;
    public:
      inline RenderCapture() : AddonClass("RenderCapture"), m_capture(g_renderManager.AllocRenderCapture()) {}
      inline virtual ~RenderCapture() { g_renderManager.ReleaseRenderCapture(m_capture); }

      /**
       * getWidth() -- returns width of captured image.
       */
      inline int getWidth() { return m_capture->GetWidth(); }

      /**
       * getHeight() -- returns height of captured image.
       */
      inline int getHeight() { return m_capture->GetHeight(); }

      /**
       * getCaptureState() -- returns processing state of capture request.
       *
       * The returned value could be compared against the following constants:
       * xbmc.CAPTURE_STATE_WORKING  : Capture request in progress.
       * xbmc.CAPTURE_STATE_DONE     : Capture request done. The image could be retrieved with getImage()
       * xbmc.CAPTURE_STATE_FAILED   : Capture request failed.
       */
      inline int getCaptureState() { return m_capture->GetUserState(); }

      /**
       * getAspectRatio() -- returns aspect ratio of currently displayed video.
       */
      inline float getAspectRatio() { return g_renderManager.GetAspectRatio(); }

      /**
       * getImageFormat() -- returns format of captured image: 'BGRA' or 'RGBA'.
       */
      inline const char* getImageFormat()
      {
        return m_capture->GetCaptureFormat() == CAPTUREFORMAT_BGRA ? "BGRA" :
          (m_capture->GetCaptureFormat() == CAPTUREFORMAT_RGBA ? "RGBA" : NULL);
      }

      // RenderCapture_GetImage
      // TODO: This needs to be done with a class that holds the Image
      // data. A memory buffer type. Then a typemap needs to be defined
      // for that type.
      /**
       * getImage() -- returns captured image as a bytearray.
       * 
       * The size of the image is getWidth() * getHeight() * 4
       */
//      PyObject* RenderCapture_GetImage(RenderCapture *self, PyObject *args)
//      {
//        if (self->capture->GetUserState() != CAPTURESTATE_DONE)
//        {
//          PyErr_SetString(PyExc_SystemError, "illegal user state");
//          return NULL;
//        }
//
//        Py_ssize_t size = self->capture->GetWidth() * self->capture->GetHeight() * 4;
//        return PyByteArray_FromStringAndSize((const char *)self->capture->GetPixels(), size);
//      }

      /**
       * capture(width, height [, flags]) -- issue capture request.
       * 
       * width    : Width capture image should be rendered to
       * height   : Height capture image should should be rendered to
       * flags    : Optional. Flags that control the capture processing.
       * 
       * The value for 'flags' could be or'ed from the following constants:
       * xbmc.CAPTURE_FLAG_CONTINUOUS    : after a capture is done, issue a new capture request immediately
       * xbmc.CAPTURE_FLAG_IMMEDIATELY   : read out immediately when capture() is called, this can cause a busy wait
       */
      inline void capture(int width, int height, int flags = 0)
      {
        g_renderManager.Capture(m_capture, (unsigned int)width, (unsigned int)height, flags);
      }

      /**
       * waitForCaptureStateChangeEvent([msecs]) -- wait for capture state change event.
       * 
       * msecs     : Milliseconds to wait. Waits forever if not specified.
       * 
       * The method will return 1 if the Event was triggered. Otherwise it will return 0.
       */
      inline int waitForCaptureStateChangeEvent(unsigned int msecs = 0)
      {
        return msecs ? m_capture->GetEvent().WaitMSec(msecs) : m_capture->GetEvent().Wait();
      }

// hide these from swig
#ifndef SWIG
      inline uint8_t*     GetPixels() { return m_capture->GetPixels();   }
      inline ECAPTURESTATE GetUserState() { return m_capture->GetUserState();  }
#endif

    };
  }
}

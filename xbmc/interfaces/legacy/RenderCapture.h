/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/RenderCapture.h"
#include "AddonClass.h"
#include "LanguageHook.h"
#include "Exception.h"
#include "commons/Buffer.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(RenderCaptureException);

    class RenderCapture : public AddonClass
    {
      CRenderCapture* m_capture;
    public:
      inline RenderCapture() : m_capture(g_renderManager.AllocRenderCapture()) {}
      inline virtual ~RenderCapture() { g_renderManager.ReleaseRenderCapture(m_capture); }

      /**
       * getWidth() -- returns width of captured image as set during\n
       *     RenderCapture.capture(). Returns 0 prior to calling capture.\n
       */
      inline int getWidth() { return m_capture->GetWidth(); }

      /**
       * getHeight() -- returns height of captured image as set during\n
       *     RenderCapture.capture(). Returns 0 prior to calling capture.\n
       */
      inline int getHeight() { return m_capture->GetHeight(); }

      /**
       * getCaptureState() -- returns processing state of capture request.
       *
       * The returned value could be compared against the following constants:
       * - xbmc.CAPTURE_STATE_WORKING  : Capture request in progress.
       * - xbmc.CAPTURE_STATE_DONE     : Capture request done. The image could be retrieved with getImage()
       * - xbmc.CAPTURE_STATE_FAILED   : Capture request failed.
       */
      inline int getCaptureState() { return m_capture->GetUserState(); }

      /**
       * getAspectRatio() -- returns aspect ratio of currently displayed video.\n
       *     This may be called prior to calling RenderCapture.capture().\n
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
      /**
       * getImage() -- returns captured image as a bytearray.
       * 
       * The size of the image is getWidth() * getHeight() * 4
       */
      inline XbmcCommons::Buffer getImage()
      {
        if (GetUserState() != CAPTURESTATE_DONE)
          throw RenderCaptureException("illegal user state");
        size_t size = getWidth() * getHeight() * 4;
        return XbmcCommons::Buffer(this->GetPixels(), size);
      }

      /**
       * capture(width, height [, flags]) -- issue capture request.
       * 
       * width    : Width capture image should be rendered to\n
       * height   : Height capture image should should be rendered to\n
       * flags    : Optional. Flags that control the capture processing.
       * 
       * The value for 'flags' could be or'ed from the following constants:
       * - xbmc.CAPTURE_FLAG_CONTINUOUS    : after a capture is done, issue a new capture request immediately
       * - xbmc.CAPTURE_FLAG_IMMEDIATELY   : read out immediately when capture() is called, this can cause a busy wait
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
        DelayedCallGuard dg(languageHook);
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

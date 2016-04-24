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

#include "AddonClass.h"
#include "LanguageHook.h"
#include "Exception.h"
#include "commons/Buffer.h"
#include "Application.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(RenderCaptureException);

    //
    /// \defgroup python_xbmc_RenderCapture RenderCapture
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's render capture.**
    ///
    /// \python_class{ RenderCapture() }
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    //
    class RenderCapture : public AddonClass
    {
      unsigned int m_captureId;
      unsigned int m_width;
      unsigned int m_height;
      uint8_t *m_buffer;

    public:
      inline RenderCapture()
      {
        m_captureId = UINT_MAX;
        m_buffer = nullptr;
        m_width = 0;
        m_height = 0;
      }
      inline virtual ~RenderCapture()
      {
        g_application.m_pPlayer->RenderCaptureRelease(m_captureId);
        delete [] m_buffer;
      }

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ getWidth() }
      ///-----------------------------------------------------------------------
      /// Get width
      ///
      /// To get width of captured image as set during RenderCapture.capture().
      /// Returns 0 prior to calling capture.
      ///
      /// @return                        Width or 0 prior to calling capture
      ///
      getWidth();
#else
      inline int getWidth() { return m_width; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ getHeight() }
      ///-----------------------------------------------------------------------
      /// Get height
      ///
      /// To get height of captured image as set during RenderCapture.capture().
      /// Returns 0 prior to calling capture.
      ///
      /// @return                        height or 0 prior to calling capture
      getHeight();
#else
      inline int getHeight() { return m_height; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ getAspectRatio() }
      ///-----------------------------------------------------------------------
      /// Get aspect ratio of currently displayed video.
      ///
      /// @return                        Aspect ratio
      /// @warning This may be called prior to calling RenderCapture.capture().
      ///
      getAspectRatio();
#else
      inline float getAspectRatio() { return g_application.m_pPlayer->GetRenderAspectRatio(); }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ getImageFormat() }
      ///-----------------------------------------------------------------------
      /// Get image format
      ///
      /// @return                        Format of captured image: 'BGRA'
      ///
      getImageFormat()
#else
      inline const char* getImageFormat()
#endif
      {
        return "BGRA";
      }

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ getImage([msecs]) }
      ///-----------------------------------------------------------------------
      /// Returns captured image as a bytearray.
      ///
      /// @param msecs               [opt] Milliseconds to wait. Waits
      ///                            1000ms if not specified
      /// @return                    Captured image as a bytearray
      ///
      /// @note The size of the image is m_width * m_height * 4
      ///
      getImage(...)
#else
      inline XbmcCommons::Buffer getImage(unsigned int msecs = 0)
#endif
      {
        if (!GetPixels(msecs))
          return XbmcCommons::Buffer(0);

        size_t size = m_width * m_height * 4;
        return XbmcCommons::Buffer(m_buffer, size);
      }

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ capture(width, height) }
      ///-----------------------------------------------------------------------
      /// Issue capture request.
      ///
      /// @param width               Width capture image should be rendered to
      /// @param height              Height capture image should should be rendered to
      ///
      capture(...)
#else
      inline void capture(int width, int height)
#endif
      {
        if (m_buffer)
        {
          g_application.m_pPlayer->RenderCaptureRelease(m_captureId);
          delete [] m_buffer;
        }
        m_captureId = g_application.m_pPlayer->RenderCaptureAlloc();
        m_width = width;
        m_height = height;
        m_buffer = new uint8_t[m_width*m_height*4];
        g_application.m_pPlayer->RenderCapture(m_captureId, m_width, m_height, CAPTUREFLAG_CONTINUOUS);
      }

// hide these from swig
#ifndef SWIG
      inline bool GetPixels(unsigned int msec)
      {
        return g_application.m_pPlayer->RenderCaptureGetPixels(m_captureId, msec, m_buffer, m_width*m_height*4);
      }
#endif

    };
    //@}
  }
}

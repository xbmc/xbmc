/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "Exception.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "commons/Buffer.h"

#include <climits>

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
      inline ~RenderCapture() override
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        appPlayer->RenderCaptureRelease(m_captureId);
        delete [] m_buffer;
      }

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ getWidth() }
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
      /// Get aspect ratio of currently displayed video.
      ///
      /// @return                        Aspect ratio
      /// @warning This may be called prior to calling RenderCapture.capture().
      ///
      getAspectRatio();
#else
      inline float getAspectRatio()
      {
        const auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        return appPlayer->GetRenderAspectRatio();
      }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmc_RenderCapture
      /// @brief \python_func{ getImageFormat() }
      /// Get image format
      ///
      /// @return                        Format of captured image: 'BGRA'
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 Image will now always be returned in BGRA
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
      /// Returns captured image as a bytearray.
      ///
      /// @param msecs               [opt] Milliseconds to wait. Waits
      ///                            1000ms if not specified
      /// @return                    Captured image as a bytearray
      ///
      /// @note The size of the image is m_width * m_height * 4
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 Added the option to specify wait time in msec.
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
      /// Issue capture request.
      ///
      /// @param width               Width capture image should be rendered to
      /// @param height              Height capture image should should be rendered to
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 Removed the option to pass **flags**
      ///
      capture(...)
#else
      inline void capture(int width, int height)
#endif
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();

        if (m_buffer)
        {
          appPlayer->RenderCaptureRelease(m_captureId);
          delete [] m_buffer;
        }
        m_captureId = appPlayer->RenderCaptureAlloc();
        m_width = width;
        m_height = height;
        m_buffer = new uint8_t[m_width*m_height*4];
        appPlayer->RenderCapture(m_captureId, m_width, m_height, CAPTUREFLAG_CONTINUOUS);
      }

// hide these from swig
#ifndef SWIG
      inline bool GetPixels(unsigned int msec)
      {
        auto& components = CServiceBroker::GetAppComponents();
        const auto appPlayer = components.GetComponent<CApplicationPlayer>();
        return appPlayer->RenderCaptureGetPixels(m_captureId, msec, m_buffer,
                                                 m_width * m_height * 4);
      }
#endif

    };
    //@}
  }
}

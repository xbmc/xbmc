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
#include "rendering/capture/CaptureConvert.h"
#include "rendering/capture/CaptureHandle.h"
#include "rendering/capture/CaptureService.h"

#include <chrono>
#include <cstdint>
#include <memory>

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
      //! the service outlives the handle: it is unregistered before script shutdown
      std::shared_ptr<KODI::RENDERING::CAPTURE::CCaptureService> m_service;
      std::unique_ptr<KODI::RENDERING::CAPTURE::CCaptureHandle> m_handle;
      unsigned int m_width{0};
      unsigned int m_height{0};
      std::unique_ptr<uint8_t[]> m_buffer;

    public:
      RenderCapture() = default;
      ~RenderCapture() override = default;

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
        return XbmcCommons::Buffer(m_buffer.get(), size);
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
        // cancel any previous request before its buffer goes away
        m_handle.reset();

        m_width = width;
        m_height = height;
        m_buffer = std::make_unique<uint8_t[]>(static_cast<size_t>(m_width) * m_height * 4);

        m_service = CServiceBroker::GetCaptureService();
        if (!m_service)
          return;

        KODI::RENDERING::CAPTURE::CaptureSpec spec;
        spec.content = KODI::RENDERING::CAPTURE::CaptureContent::VIDEO;
        spec.cadence = KODI::RENDERING::CAPTURE::CaptureCadence::CONTINUOUS;
        spec.width = m_width;
        spec.height = m_height;
        m_handle = m_service->Submit(spec);
      }

// hide these from swig
#ifndef SWIG
      inline bool GetPixels(unsigned int msec)
      {
        if (!m_handle)
          return false;
        if (!m_handle->WaitNext(std::chrono::milliseconds(msec ? msec : 1000)))
          return false;
        const KODI::RENDERING::CAPTURE::CaptureResult result = m_handle->CopyResult();
        // legacy contract: raw output-coded 8-bit BGRA, never tonemapped; an
        // HDR session delivers PQ/HLG-coded bytes exactly as glReadPixels did
        return KODI::RENDERING::CAPTURE::CaptureCopyBGRA8(result, m_width, m_height, m_buffer.get());
      }
#endif

    };
    //@}
  }
}

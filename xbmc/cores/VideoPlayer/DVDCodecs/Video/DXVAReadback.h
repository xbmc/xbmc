/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"

#include <array>
#include <utility>
#include <vector>

#include <d3d11_4.h>
#include <wrl/client.h>

namespace KODI::VIDEO::GEOMETRY
{
struct ReducedFrame;
} // namespace KODI::VIDEO::GEOMETRY

namespace DXVA
{

/*!
 * \brief Scales a decoder surface to a small NV12 target and reads it back.
 *
 * Runs on the surface's own device. One instance serves a whole buffer pool.
 *
 * The read is a frame behind what was just submitted, and never waits.
 */
class CSurfaceReadback
{
public:
  ReductionResult Reduce(ID3D11Resource* surface,
                         unsigned int slice,
                         unsigned int sourceWidth,
                         unsigned int sourceHeight,
                         bool fullRange,
                         KODI::VIDEO::GEOMETRY::ReducedFrame& reduction,
                         unsigned int targetWidth);

private:
  bool EnsureProcessor(ID3D11Resource* surface,
                       unsigned int sourceWidth,
                       unsigned int sourceHeight,
                       unsigned int targetWidth,
                       unsigned int targetHeight,
                       bool fullRange);

  //! \brief State the colour space and the rectangles on the processor. None of it changes
  //! from frame to frame, so this runs only when the processor is built or the range moves.
  void ApplyProcessorState(bool fullRange);
  ID3D11VideoProcessorInputView* GetInputView(ID3D11Resource* surface, unsigned int slice);

  //! \brief Read the slot the previous frame filled, if the GPU has finished with it, leaving
  //! it filled when it has not.
  ReductionResult ReadFilledSlot(KODI::VIDEO::GEOMETRY::ReducedFrame& reduction);

  //! \brief Say what went wrong once, then retry quietly.
  void RuntimeFail(const char* what, HRESULT hr);

  static constexpr size_t STAGING_SLOTS = 2;

  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> m_videoDevice;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_videoContext;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> m_enumerator;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessor> m_processor;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_target;
  std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>, STAGING_SLOTS> m_staging;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorOutputView> m_outputView;
  std::vector<std::pair<unsigned int, Microsoft::WRL::ComPtr<ID3D11VideoProcessorInputView>>>
      m_inputViews;
  ID3D11Resource* m_viewedSurface{nullptr};

  //! \brief Which slot the next copy goes to, and which slots hold a frame waiting to be read.
  size_t m_writeSlot{0};
  std::array<bool, STAGING_SLOTS> m_filled{};

  unsigned int m_sourceWidth{0};
  unsigned int m_sourceHeight{0};
  unsigned int m_targetWidth{0};
  unsigned int m_targetHeight{0};
  bool m_fullRange{false}; //!< what ApplyProcessorState() last stated
  bool m_failed{false}; //!< a device that cannot do this once cannot do it next frame either
  bool m_runtimeFailLogged{false}; //!< see RuntimeFail()
};
} // namespace DXVA

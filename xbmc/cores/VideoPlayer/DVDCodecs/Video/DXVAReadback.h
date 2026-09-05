/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
 * Runs on the decoder device, never touching the app device's immediate context, which belongs
 * to the render thread. One instance serves a whole buffer pool and reuses every D3D object in
 * it, so a per-frame reduction costs one blit, one small copy and one mapping.
 */
class CSurfaceReadback
{
public:
  bool Reduce(ID3D11Resource* surface,
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
                       unsigned int targetHeight);
  ID3D11VideoProcessorInputView* GetInputView(ID3D11Resource* surface, unsigned int slice);

  //! \brief Say what went wrong once, then retry quietly - the callers run at the frame rate.
  //! The attempt keeps being made, so a transient failure heals with no codec reopen.
  void RuntimeFail(const char* what, HRESULT hr);

  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_deviceContext;
  Microsoft::WRL::ComPtr<ID3D11VideoDevice> m_videoDevice;
  Microsoft::WRL::ComPtr<ID3D11VideoContext> m_videoContext;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> m_enumerator;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessor> m_processor;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_target;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_staging;
  Microsoft::WRL::ComPtr<ID3D11VideoProcessorOutputView> m_outputView;
  std::vector<std::pair<unsigned int, Microsoft::WRL::ComPtr<ID3D11VideoProcessorInputView>>>
      m_inputViews;
  ID3D11Resource* m_viewedSurface{nullptr};

  unsigned int m_sourceWidth{0};
  unsigned int m_sourceHeight{0};
  unsigned int m_targetWidth{0};
  unsigned int m_targetHeight{0};
  bool m_failed{false}; //!< a device that cannot do this once cannot do it next frame either
  bool m_runtimeFailLogged{false}; //!< see RuntimeFail()
};
} // namespace DXVA

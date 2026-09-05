/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DXVAReadback.h"

#include "utils/log.h"
#include "video/geometry/FrameReduction.h"

#include "platform/win32/WIN32Util.h"

#include <cstdint>

using namespace DXVA;
using namespace Microsoft::WRL;

void CSurfaceReadback::RuntimeFail(const char* what, HRESULT hr)
{
  if (m_runtimeFailLogged)
    return;

  m_runtimeFailLogged = true;
  CLog::LogF(LOGWARNING, "{}, error description: {}", what, CWIN32Util::FormatHRESULT(hr));
}

void CSurfaceReadback::ApplyProcessorState(bool fullRange)
{
  // clang-format off
  D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace
  {
    0u,                    // 0 - Playback, 1 - Processing
    fullRange ? 0u : 1u,   // RGB range: 0 - Full (0-255), 1 - Limited (16-235)
    1u,                    // 0 - BT.601, 1 - BT.709
    0u,                    // 0 - Conventional YCbCr, 1 - xvYCC
    fullRange ? 2u : 1u    // YUV range: 2 - Full (0-255), 1 - Studio (16-235)
  };
  // clang-format on
  m_videoContext->VideoProcessorSetStreamColorSpace(m_processor.Get(), 0, &colorSpace);
  m_videoContext->VideoProcessorSetOutputColorSpace(m_processor.Get(), &colorSpace);

  RECT source = {0, 0, static_cast<LONG>(m_sourceWidth), static_cast<LONG>(m_sourceHeight)};
  RECT destination = {0, 0, static_cast<LONG>(m_targetWidth), static_cast<LONG>(m_targetHeight)};
  m_videoContext->VideoProcessorSetStreamSourceRect(m_processor.Get(), 0, TRUE, &source);
  m_videoContext->VideoProcessorSetStreamDestRect(m_processor.Get(), 0, TRUE, &destination);
  m_videoContext->VideoProcessorSetOutputTargetRect(m_processor.Get(), TRUE, &destination);

  m_fullRange = fullRange;
}

bool CSurfaceReadback::EnsureProcessor(ID3D11Resource* surface,
                                       unsigned int sourceWidth,
                                       unsigned int sourceHeight,
                                       unsigned int targetWidth,
                                       unsigned int targetHeight,
                                       bool fullRange)
{
  if (m_failed)
    return false;

  if (m_processor && m_sourceWidth == sourceWidth && m_sourceHeight == sourceHeight &&
      m_targetWidth == targetWidth && m_targetHeight == targetHeight)
  {
    // The processor stands; only a stream that changed range needs anything restating.
    if (m_fullRange != fullRange)
      ApplyProcessorState(fullRange);
    return true;
  }

  m_inputViews.clear();
  m_viewedSurface = nullptr;
  m_outputView = nullptr;

  m_staging = {};
  m_filled = {};
  m_writeSlot = 0;

  m_target = nullptr;
  m_processor = nullptr;
  m_enumerator = nullptr;

  const auto fail = [this](const char* what, HRESULT hr)
  {
    CLog::LogF(LOGWARNING, "surface readback unavailable: {} failed, error description: {}", what,
               CWIN32Util::FormatHRESULT(hr));
    m_failed = true;
    return false;
  };

  ComPtr<ID3D11Device> device;
  surface->GetDevice(&device);

  HRESULT hr;
  if (FAILED(hr = device.As(&m_videoDevice)))
    return fail("querying the video device", hr);

  device->GetImmediateContext(&m_deviceContext);
  if (FAILED(hr = m_deviceContext.As(&m_videoContext)))
    return fail("querying the video context", hr);

  D3D11_VIDEO_PROCESSOR_CONTENT_DESC desc = {};
  desc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
  desc.InputWidth = sourceWidth;
  desc.InputHeight = sourceHeight;
  desc.OutputWidth = targetWidth;
  desc.OutputHeight = targetHeight;
  desc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

  if (FAILED(hr = m_videoDevice->CreateVideoProcessorEnumerator(&desc, &m_enumerator)))
    return fail("creating the video processor enumerator", hr);

  if (FAILED(hr = m_videoDevice->CreateVideoProcessor(m_enumerator.Get(), 0, &m_processor)))
    return fail("creating the video processor", hr);

  D3D11_TEXTURE2D_DESC texture = {};
  texture.Width = targetWidth;
  texture.Height = targetHeight;
  texture.MipLevels = 1;
  texture.ArraySize = 1;
  texture.Format = DXGI_FORMAT_NV12;
  texture.SampleDesc.Count = 1;
  texture.Usage = D3D11_USAGE_DEFAULT;
  texture.BindFlags = D3D11_BIND_RENDER_TARGET;

  if (FAILED(hr = device->CreateTexture2D(&texture, nullptr, &m_target)))
    return fail("creating the target texture", hr);

  texture.BindFlags = 0;
  texture.Usage = D3D11_USAGE_STAGING;
  texture.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

  for (Microsoft::WRL::ComPtr<ID3D11Texture2D>& staging : m_staging)
  {
    if (FAILED(hr = device->CreateTexture2D(&texture, nullptr, &staging)))
      return fail("creating a staging texture", hr);
  }

  D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputView = {};
  outputView.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;

  if (FAILED(hr = m_videoDevice->CreateVideoProcessorOutputView(m_target.Get(), m_enumerator.Get(),
                                                                &outputView, &m_outputView)))
    return fail("creating the output view", hr);

  m_videoContext->VideoProcessorSetStreamAutoProcessingMode(m_processor.Get(), 0, FALSE);

  m_sourceWidth = sourceWidth;
  m_sourceHeight = sourceHeight;
  m_targetWidth = targetWidth;
  m_targetHeight = targetHeight;

  ApplyProcessorState(fullRange);
  return true;
}

ID3D11VideoProcessorInputView* CSurfaceReadback::GetInputView(ID3D11Resource* surface,
                                                              unsigned int slice)
{
  if (m_viewedSurface != surface)
  {
    m_inputViews.clear();
    m_viewedSurface = surface;
  }

  for (const auto& [cachedSlice, view] : m_inputViews)
  {
    if (cachedSlice == slice)
      return view.Get();
  }

  D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inputView = {};
  inputView.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
  inputView.Texture2D.MipSlice = 0;
  inputView.Texture2D.ArraySlice = slice;

  Microsoft::WRL::ComPtr<ID3D11VideoProcessorInputView> view;
  const HRESULT hr =
      m_videoDevice->CreateVideoProcessorInputView(surface, m_enumerator.Get(), &inputView, &view);
  if (FAILED(hr))
  {
    RuntimeFail("unable to create an input view", hr);
    return nullptr;
  }

  m_inputViews.emplace_back(slice, view);
  return view.Get();
}

ReductionResult CSurfaceReadback::ReadFilledSlot(KODI::VIDEO::GEOMETRY::ReducedFrame& reduction)
{
  const size_t readSlot = m_writeSlot;
  if (!m_filled[readSlot])
    return ReductionResult::Pending;

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  const HRESULT hr = m_deviceContext->Map(m_staging[readSlot].Get(), 0, D3D11_MAP_READ,
                                          D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);

  if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
    return ReductionResult::Pending;

  if (FAILED(hr))
  {
    m_filled[readSlot] = false;
    RuntimeFail("mapping the staging texture failed", hr);
    return ReductionResult::Unsupported;
  }

  KODI::VIDEO::GEOMETRY::ReductionSource nv12;
  nv12.width = m_targetWidth;
  nv12.height = m_targetHeight;
  nv12.bitDepth = 8;
  nv12.chroma = KODI::VIDEO::GEOMETRY::ChromaLayout::Interleaved;
  nv12.y = static_cast<const uint8_t*>(mapped.pData);
  nv12.yStrideBytes = static_cast<int>(mapped.RowPitch);
  nv12.u = nv12.y + static_cast<size_t>(mapped.RowPitch) * m_targetHeight;
  nv12.uStrideBytes = static_cast<int>(mapped.RowPitch);

  const bool copied = KODI::VIDEO::GEOMETRY::ReduceFrame(nv12, m_targetWidth, reduction);

  m_deviceContext->Unmap(m_staging[readSlot].Get(), 0);
  m_filled[readSlot] = false;

  return copied ? ReductionResult::Produced : ReductionResult::Unsupported;
}

ReductionResult CSurfaceReadback::Reduce(ID3D11Resource* surface,
                                         unsigned int slice,
                                         unsigned int sourceWidth,
                                         unsigned int sourceHeight,
                                         bool fullRange,
                                         KODI::VIDEO::GEOMETRY::ReducedFrame& reduction,
                                         unsigned int targetWidth)
{
  const auto [width, height] =
      KODI::VIDEO::GEOMETRY::ReductionOutputSize(sourceWidth, sourceHeight, targetWidth);
  if (width == 0 || height == 0)
    return ReductionResult::Unsupported;

  if (!EnsureProcessor(surface, sourceWidth, sourceHeight, width, height, fullRange))
    return ReductionResult::Unsupported;

  ID3D11VideoProcessorInputView* input = GetInputView(surface, slice);
  if (!input)
    return ReductionResult::Unsupported;

  D3D11_VIDEO_PROCESSOR_STREAM stream = {};
  stream.Enable = TRUE;
  stream.pInputSurface = input;

  HRESULT hr;
  if (FAILED(hr = m_videoContext->VideoProcessorBlt(m_processor.Get(), m_outputView.Get(), 0, 1,
                                                    &stream)))
  {
    RuntimeFail("blit failed", hr);
    return ReductionResult::Unsupported;
  }

  m_deviceContext->CopyResource(m_staging[m_writeSlot].Get(), m_target.Get());
  m_filled[m_writeSlot] = true;
  m_writeSlot = (m_writeSlot + 1) % STAGING_SLOTS;

  return ReadFilledSlot(reduction);
}

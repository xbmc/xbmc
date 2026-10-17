/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DolbyVisionEffect.h"

#include "rendering/dx/DeviceResources.h"
#include "utils/log.h"

#include <cmath>
#include <cstring>
#include <string>
#include <thread>

#include <d3d11_4.h>
#include <mferror.h>
#include <mfidl.h>
#include <propvarutil.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Display.h>

using namespace Microsoft::WRL;
using namespace winrt::Windows::ApplicationModel::AppService;
using namespace winrt::Windows::Graphics::Display;
using namespace std::chrono_literals;

namespace
{
constexpr GUID kVideoRendererEffectCategory{
    0x145cd8b4, 0x92f4, 0x4b23, {0x8a, 0xe7, 0xe0, 0xdf, 0x06, 0xc2, 0xda, 0x95}};
constexpr GUID kVideoRendererExtensionProfile{
    0x62c56928, 0x9a4e, 0x443b, {0xb9, 0xdc, 0xca, 0xc8, 0x30, 0xc2, 0x41, 0x00}};
constexpr GUID kForwardedDecodeUnits{
    0x424c754c, 0x97c8, 0x48d6, {0x87, 0x77, 0xfc, 0x41, 0xf7, 0xb6, 0x08, 0x79}};
constexpr GUID kForwardedDecodeUnitType{
    0x089e57c7, 0x47d3, 0x4a26, {0xbf, 0x9c, 0x4b, 0x64, 0xfa, 0xfb, 0x5d, 0x1e}};
constexpr GUID kAppServiceName{
    0xc6052a80, 0x6d9c, 0x40a3, {0x9d, 0xb8, 0xf0, 0x27, 0xa2, 0x5c, 0x9a, 0xb9}};
constexpr GUID kCustomVideoPrimaries{
    0x47537213, 0x8cfb, 0x4722, {0xaa, 0x34, 0xfb, 0xc9, 0xe2, 0x4d, 0x77, 0xb8}};

HRESULT FindRendererEffect(IMFActivate** activation)
{
  IMFActivate** activations = nullptr;
  UINT32 count = 0;
  HRESULT hr = MFTEnumEx(kVideoRendererEffectCategory, MFT_ENUM_FLAG_SORTANDFILTER, nullptr,
                         nullptr, &activations, &count);
  if (FAILED(hr))
    return hr;

  hr = MF_E_TOPO_CODEC_NOT_FOUND;
  for (UINT32 i = 0; i < count; ++i)
  {
    PROPVARIANT value;
    PropVariantInit(&value);
    bool matches = false;
    if (SUCCEEDED(activations[i]->GetItem(kVideoRendererExtensionProfile, &value)) &&
        value.vt == (VT_VECTOR | VT_LPWSTR))
    {
      for (ULONG j = 0; j < value.calpwstr.cElems && !matches; ++j)
        matches = value.calpwstr.pElems[j] && _wcsicmp(value.calpwstr.pElems[j], L"dvhe.05") == 0;
    }
    PropVariantClear(&value);

    if (matches && hr != S_OK)
    {
      activations[i]->AddRef();
      *activation = activations[i];
      hr = S_OK;
    }
    activations[i]->Release();
  }
  CoTaskMemFree(activations);
  return hr;
}

HRESULT CreateTextureSample(ID3D11Texture2D* texture, LONGLONG time, IMFSample** sample)
{
  ComPtr<IMFMediaBuffer> buffer;
  HRESULT hr = MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), texture, 0, FALSE, &buffer);
  ComPtr<IMFSample> textureSample;
  if (SUCCEEDED(hr))
    hr = MFCreateSample(&textureSample);
  if (SUCCEEDED(hr))
    hr = textureSample->AddBuffer(buffer.Get());
  if (SUCCEEDED(hr))
    hr = textureSample->SetSampleTime(time);
  if (SUCCEEDED(hr))
    *sample = textureSample.Detach();
  return hr;
}

HRESULT AttachRpu(IMFSample* frame, const std::vector<uint8_t>& rpu, LONGLONG time)
{
  ComPtr<IMFMediaBuffer> buffer;
  HRESULT hr = MFCreateMemoryBuffer(static_cast<DWORD>(rpu.size()), &buffer);
  BYTE* data = nullptr;
  if (SUCCEEDED(hr))
    hr = buffer->Lock(&data, nullptr, nullptr);
  if (SUCCEEDED(hr))
  {
    std::memcpy(data, rpu.data(), rpu.size());
    hr = buffer->Unlock();
  }
  if (SUCCEEDED(hr))
    hr = buffer->SetCurrentLength(static_cast<DWORD>(rpu.size()));

  ComPtr<IMFSample> unit;
  if (SUCCEEDED(hr))
    hr = MFCreateSample(&unit);
  if (SUCCEEDED(hr))
    hr = unit->AddBuffer(buffer.Get());
  if (SUCCEEDED(hr))
    hr = unit->SetSampleTime(time);
  if (SUCCEEDED(hr))
    hr = unit->SetUINT32(kForwardedDecodeUnitType, MF_DECODE_UNIT_NAL);

  ComPtr<IMFCollection> units;
  if (SUCCEEDED(hr))
    hr = MFCreateCollection(&units);
  if (SUCCEEDED(hr))
    hr = units->AddElement(unit.Get());
  if (SUCCEEDED(hr))
    hr = frame->SetUnknown(kForwardedDecodeUnits, units.Get());
  return hr;
}

struct InputColor
{
  UINT32 transfer;
  UINT32 matrix;
  UINT32 range;
};

UINT32 ToTransferMatrix(AVColorSpace colorSpace, bool hdr)
{
  switch (colorSpace)
  {
    case AVCOL_SPC_BT709:
      return MFVideoTransferMatrix_BT709;
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      return MFVideoTransferMatrix_BT601;
    case AVCOL_SPC_SMPTE240M:
      return MFVideoTransferMatrix_SMPTE240M;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
      return MFVideoTransferMatrix_BT2020_10;
    default:
      return hdr ? MFVideoTransferMatrix_BT2020_10 : MFVideoTransferMatrix_BT709;
  }
}

// proprietary IPTPQc2 base layer
bool HasIptBaseLayer(const CDolbyVisionEffect::StreamInfo& stream)
{
  return stream.profile == 5 || (stream.profile == 10 && stream.compatibilityId == 0);
}

bool HasHdr10BaseLayer(const CDolbyVisionEffect::StreamInfo& stream)
{
  return (stream.profile == 8 || stream.profile == 10) && stream.compatibilityId == 1;
}

InputColor GetInputColor(const CDolbyVisionEffect::StreamInfo& stream, bool hdrOutput)
{
  if (HasIptBaseLayer(stream))
    return {MFVideoTransFunc_2084, MFVideoTransferMatrix_BT2020_10, MFNominalRange_0_255};
  if (stream.profile == 7 || HasHdr10BaseLayer(stream))
    return {MFVideoTransFunc_2084, MFVideoTransferMatrix_BT2020_10, MFNominalRange_16_235};
  if (stream.profile == 8 || stream.profile == 10)
  {
    if (stream.compatibilityId == 2)
      return {MFVideoTransFunc_709, MFVideoTransferMatrix_BT709, MFNominalRange_16_235};
    if (stream.compatibilityId == 4)
      return {MFVideoTransFunc_HLG, MFVideoTransferMatrix_BT2020_10, MFNominalRange_16_235};
  }

  const bool hdr = hdrOutput || stream.colorTransfer == AVCOL_TRC_SMPTE2084 ||
                   stream.colorTransfer == AVCOL_TRC_ARIB_STD_B67;
  UINT32 transfer;
  switch (stream.colorTransfer)
  {
    case AVCOL_TRC_SMPTE2084:
      transfer = MFVideoTransFunc_2084;
      break;
    case AVCOL_TRC_ARIB_STD_B67:
      transfer = MFVideoTransFunc_HLG;
      break;
    case AVCOL_TRC_BT709:
      transfer = MFVideoTransFunc_709;
      break;
    default:
      transfer = hdrOutput ? MFVideoTransFunc_2084 : MFVideoTransFunc_709;
      break;
  }
  return {transfer, ToTransferMatrix(stream.colorSpace, hdr),
          static_cast<UINT32>(stream.fullRange ? MFNominalRange_0_255 : MFNominalRange_16_235)};
}
} // namespace

CDolbyVisionEffect::~CDolbyVisionEffect()
{
  if (m_transform && m_streaming)
  {
    m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
    m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
  }
  m_transform.Reset();
  m_deviceManager.Reset();
}

bool CDolbyVisionEffect::IsAvailable()
{
  if (FAILED(MFStartup(MF_VERSION)))
    return false;

  ComPtr<IMFActivate> activation;
  const bool available = SUCCEEDED(FindRendererEffect(&activation));
  activation.Reset();
  MFShutdown();
  return available;
}

bool CDolbyVisionEffect::Open(bool hdrOutput, float fps, const StreamInfo& stream)
{
  m_hdrOutput = hdrOutput;
  m_fps = fps;
  m_stream = stream;

  // effect keeps working on its own threads after it is released.
  static const HRESULT startupHr = MFStartup(MF_VERSION);
  HRESULT hr = startupHr;
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "MFStartup failed: {:#x}", static_cast<uint32_t>(hr));
    return false;
  }

  ComPtr<IMFActivate> activation;
  hr = FindRendererEffect(&activation);
  if (SUCCEEDED(hr))
    hr = activation->ActivateObject(IID_PPV_ARGS(&m_transform));
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "No Dolby Vision renderer effect for: {:#x}", static_cast<uint32_t>(hr));
    return false;
  }

  ID3D11Device* device = DX::DeviceResources::Get()->GetD3DDevice();
  ComPtr<ID3D11Multithread> multithread;
  if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&multithread))))
    multithread->SetMultithreadProtected(TRUE);

  UINT resetToken = 0;
  hr = MFCreateDXGIDeviceManager(&resetToken, &m_deviceManager);
  if (SUCCEEDED(hr))
    hr = m_deviceManager->ResetDevice(device, resetToken);
  if (SUCCEEDED(hr))
    hr = m_transform->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER,
                                     reinterpret_cast<ULONG_PTR>(m_deviceManager.Get()));
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Unable to share the D3D device with the effect: {:#x}",
               static_cast<uint32_t>(hr));
    return false;
  }

  ComPtr<IMFAttributes> attributes;
  if (SUCCEEDED(m_transform->GetAttributes(&attributes)))
    attributes->SetUINT32(MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT,
                          static_cast<UINT32>(m_outputPool.size()));

  SetOutputDevice();

  CLog::LogF(LOGINFO, "Dolby Vision effect loaded for profile {}.{}, output {}", m_stream.profile,
             m_stream.compatibilityId,
             m_hdrOutput ? "HDR PQ BT.2020 (R10G10B10A2)" : "SDR sRGB (B8G8R8A8)");
  return true;
}

void CDolbyVisionEffect::SetOutputDevice()
{
  ComPtr<IMFVideoProcessorControl3> control;
  if (FAILED(m_transform.As(&control)))
  {
    CLog::LogF(LOGWARNING, "Effect has no IMFVideoProcessorControl3");
    return;
  }

  try
  {
    const auto display = DisplayInformation::GetForCurrentView();
    const HRESULT hr = control->SetOutputDevice(static_cast<::IUnknown*>(winrt::get_abi(display)));
    if (FAILED(hr))
    {
      CLog::LogF(LOGWARNING, "SetOutputDevice failed: {:#x}", static_cast<uint32_t>(hr));
      return;
    }

    const auto color = display.GetAdvancedColorInfo();
    const auto red = color.RedPrimary();
    const auto green = color.GreenPrimary();
    const auto blue = color.BluePrimary();
    const auto white = color.WhitePoint();
    m_displayPrimaries = {red.X, red.Y, green.X, green.Y, blue.X, blue.Y, white.X, white.Y};
    m_displayMaxLuminance = color.MaxLuminanceInNits();
    m_displayMinLuminance = color.MinLuminanceInNits();
    m_hasDisplayColor = m_displayMaxLuminance > 0.0f;

    CLog::LogF(LOGDEBUG,
               "display: maxLum={:.0f} minLum={:.4f} prim={{{:.4f},{:.4f} {:.4f},{:.4f} "
               "{:.4f},{:.4f} {:.4f},{:.4f}}}",
               m_displayMaxLuminance, m_displayMinLuminance, red.X, red.Y, green.X, green.Y, blue.X,
               blue.Y, white.X, white.Y);
  }
  catch (const winrt::hresult_error& e)
  {
    CLog::LogF(LOGWARNING, "Display information unavailable: {:#x}",
               static_cast<uint32_t>(e.code()));
  }
}

DXGI_FORMAT CDolbyVisionEffect::GetOutputFormat() const
{
  return m_hdrOutput ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
}

HRESULT CDolbyVisionEffect::ConfigureStream(unsigned width, unsigned height)
{
  if (width == m_width && height == m_height)
    return S_OK;

  HRESULT hr = SetInputType(width, height);
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Set input type failed: {:#x}", static_cast<uint32_t>(hr));
    return hr;
  }
  hr = SetOutputType();
  if (FAILED(hr))
  {
    CLog::LogF(LOGERROR, "Set output type failed: {:#x}", static_cast<uint32_t>(hr));
    return hr;
  }

  m_width = width;
  m_height = height;
  for (auto& texture : m_inputPool)
    texture.Reset();
  for (auto& texture : m_outputPool)
    texture.Reset();

  if (!m_streaming)
  {
    m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    m_transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    m_streaming = true;
  }
  return S_OK;
}

HRESULT CDolbyVisionEffect::SetInputType(unsigned width, unsigned height)
{
  ComPtr<IMFMediaType> type;
  HRESULT hr = MFCreateMediaType(&type);

  if (FAILED(hr))
    return hr;

  const InputColor color = GetInputColor(m_stream, m_hdrOutput);
  MFVideoArea area{};
  area.Area = {static_cast<LONG>(width), static_cast<LONG>(height)};

  type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_P010);
  MFSetAttributeSize(type.Get(), MF_MT_FRAME_SIZE, width, height);
  type->SetUINT32(MF_MT_DEFAULT_STRIDE, width * 2);
  type->SetUINT32(MF_MT_SAMPLE_SIZE, width * height * 3);
  MFSetAttributeRatio(type.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
  type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
  type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
  type->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);
  type->SetUINT32(MF_MT_VIDEO_ROTATION, MFVideoRotationFormat_0);
  type->SetBlob(MF_MT_GEOMETRIC_APERTURE, reinterpret_cast<UINT8*>(&area), sizeof(area));
  type->SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, reinterpret_cast<UINT8*>(&area), sizeof(area));
  type->SetBlob(MF_MT_PAN_SCAN_APERTURE, reinterpret_cast<UINT8*>(&area), sizeof(area));

  if (m_fps > 0.0f)
    MFSetAttributeRatio(type.Get(), MF_MT_FRAME_RATE,
                        static_cast<UINT32>(std::lround(m_fps * 1000.0f)), 1000);

  if (HasHdr10BaseLayer(m_stream) && m_stream.maxMasteringLuminance)
    type->SetUINT32(MF_MT_MAX_MASTERING_LUMINANCE, m_stream.maxMasteringLuminance);

  // IPTPQc2 has no YCbCr transfer function to describe
  if (!HasIptBaseLayer(m_stream))
    type->SetUINT32(MF_MT_TRANSFER_FUNCTION, color.transfer);

  type->SetUINT32(MF_MT_YUV_MATRIX, color.matrix);
  type->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, color.range);
  type->SetUINT32(MF_MT_VIDEO_CHROMA_SITING, MFVideoChromaSubsampling_MPEG2);

  return m_transform->SetInputType(0, type.Get(), 0);
}

HRESULT CDolbyVisionEffect::SetOutputType()
{
  const GUID subtype = m_hdrOutput ? MFVideoFormat_A2R10G10B10 : MFVideoFormat_ARGB32;
  const UINT32 primaries = m_hdrOutput ? MFVideoPrimaries_BT2020 : MFVideoPrimaries_BT709;
  const UINT32 transfer = m_hdrOutput ? MFVideoTransFunc_2084 : MFVideoTransFunc_sRGB;

  for (DWORD i = 0;; ++i)
  {
    ComPtr<IMFMediaType> type;
    if (FAILED(m_transform->GetOutputAvailableType(0, i, &type)))
      return MF_E_INVALIDMEDIATYPE;

    GUID typeSubtype{};
    if (FAILED(type->GetGUID(MF_MT_SUBTYPE, &typeSubtype)))
      continue;

    const UINT32 typePrimaries = MFGetAttributeUINT32(type.Get(), MF_MT_VIDEO_PRIMARIES, 0);
    const UINT32 typeTransfer = MFGetAttributeUINT32(type.Get(), MF_MT_TRANSFER_FUNCTION, 0);
    if (typeSubtype != subtype || typePrimaries != primaries || typeTransfer != transfer)
      continue;

    if (m_hdrOutput && m_hasDisplayColor)
    {
      type->SetBlob(kCustomVideoPrimaries, reinterpret_cast<UINT8*>(m_displayPrimaries.data()),
                    sizeof(m_displayPrimaries));
      type->SetUINT32(MF_MT_MAX_MASTERING_LUMINANCE,
                      static_cast<UINT32>(std::lround(m_displayMaxLuminance)));
      type->SetUINT32(MF_MT_MIN_MASTERING_LUMINANCE,
                      static_cast<UINT32>(std::lround(m_displayMinLuminance * 10000.0f)));
    }

    return m_transform->SetOutputType(0, type.Get(), 0);
  }
}

ID3D11Texture2D* CDolbyVisionEffect::GetInputTexture(const D3D11_TEXTURE2D_DESC& decoderDesc)
{
  auto& texture = m_inputPool[m_inputIndex];
  if (!texture)
  {
    const CD3D11_TEXTURE2D_DESC desc(decoderDesc.Format, decoderDesc.Width, decoderDesc.Height, 1,
                                     1, D3D11_BIND_SHADER_RESOURCE);
    if (FAILED(
            DX::DeviceResources::Get()->GetD3DDevice()->CreateTexture2D(&desc, nullptr, &texture)))
    {
      CLog::LogF(LOGERROR, "unable to create Dolby Vision input texture");
      return nullptr;
    }
  }

  m_inputIndex = (m_inputIndex + 1) % m_inputPool.size();

  return texture.Get();
}

ID3D11Texture2D* CDolbyVisionEffect::GetOutputTexture(unsigned width, unsigned height)
{
  auto& texture = m_outputPool[m_outputIndex];
  if (!texture)
  {
    const CD3D11_TEXTURE2D_DESC desc(GetOutputFormat(), width, height, 1, 1,
                                     D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (FAILED(
            DX::DeviceResources::Get()->GetD3DDevice()->CreateTexture2D(&desc, nullptr, &texture)))
    {
      CLog::LogF(LOGERROR, "unable to create Dolby Vision output texture");
      return nullptr;
    }
  }

  m_outputIndex = (m_outputIndex + 1) % m_outputPool.size();

  return texture.Get();
}

HRESULT CDolbyVisionEffect::ReadOutput(LONGLONG sampleTime, ID3D11Texture2D** output)
{
  ID3D11Texture2D* texture = GetOutputTexture(m_width, m_height);
  if (!texture)
    return E_OUTOFMEMORY;

  ComPtr<IMFSample> sample;
  HRESULT hr = CreateTextureSample(texture, sampleTime, &sample);
  if (FAILED(hr))
    return hr;

  for (int attempt = 0;; ++attempt)
  {
    MFT_OUTPUT_DATA_BUFFER buffer{};
    buffer.pSample = sample.Get();
    DWORD status = 0;
    hr = m_transform->ProcessOutput(0, 1, &buffer, &status);
    if (buffer.pEvents)
      buffer.pEvents->Release();
    if (hr != MF_E_TRANSFORM_STREAM_CHANGE || attempt == 1)
      break;

    // the transform changed its output type, which has to be negotiated again
    hr = SetOutputType();
    if (FAILED(hr))
      break;
  }

  if (SUCCEEDED(hr))
    *output = texture;
  return hr;
}

HRESULT CDolbyVisionEffect::Process(ID3D11Resource* decoderTexture,
                                    unsigned arrayIndex,
                                    const std::vector<uint8_t>& rpu,
                                    LONGLONG sampleTime,
                                    ID3D11Texture2D** output)
{
  *output = nullptr;

  ComPtr<ID3D11Texture2D> decoder;
  if (!m_transform || !decoderTexture ||
      FAILED(decoderTexture->QueryInterface(IID_PPV_ARGS(&decoder))))
    return E_INVALIDARG;

  D3D11_TEXTURE2D_DESC desc;
  decoder->GetDesc(&desc);
  HRESULT hr = ConfigureStream(desc.Width, desc.Height);
  if (FAILED(hr))
    return hr;

  ID3D11Texture2D* input = GetInputTexture(desc);
  if (!input)
    return E_OUTOFMEMORY;

  DX::DeviceResources::Get()->GetImmediateContext()->CopySubresourceRegion(
      input, 0, 0, 0, 0, decoder.Get(), D3D11CalcSubresource(0, arrayIndex, desc.MipLevels),
      nullptr);

  ComPtr<IMFSample> inputSample;
  hr = CreateTextureSample(input, sampleTime, &inputSample);
  if (SUCCEEDED(hr) && !rpu.empty())
    hr = AttachRpu(inputSample.Get(), rpu, sampleTime);
  if (SUCCEEDED(hr))
    hr = m_transform->ProcessInput(0, inputSample.Get(), 0);
  if (hr == MF_E_NOTACCEPTING)
  {
    // the transform holds output of an earlier frame, which has to be collected first
    hr = ReadOutput(sampleTime, output);
    if (SUCCEEDED(hr) || hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
      hr = m_transform->ProcessInput(0, inputSample.Get(), 0);
  }
  if (FAILED(hr))
  {
    if (m_failures++ < 8)
      CLog::LogF(LOGWARNING, "Effect input failed: {:#x}", static_cast<uint32_t>(hr));
    return hr;
  }

  ID3D11Texture2D* latest = nullptr;
  hr = ReadOutput(sampleTime, &latest);
  if (SUCCEEDED(hr))
    *output = latest;
  else if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
  {
    if (m_failures++ < 8)
      CLog::LogF(LOGWARNING, "Effect output failed: {:#x}", static_cast<uint32_t>(hr));
    return hr;
  }

  return *output ? S_OK : S_FALSE;
}

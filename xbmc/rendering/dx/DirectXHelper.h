/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ppltasks.h>	// For create_task
#include "commons/Exception.h"
#include "dxerr.h"
#include "platform/win32/CharsetConverter.h"

#include <d3d11_1.h>

namespace DX
{
#define RATIONAL_TO_FLOAT(rational) ((rational.Denominator != 0) ? \
  static_cast<float>(rational.Numerator) / static_cast<float>(rational.Denominator) : 0.0f)

  inline void BreakIfFailed(HRESULT hr)
  {
    if (FAILED(hr))
    {
      // Set a breakpoint on this line to catch Win32 API errors.
#if _DEBUG && !defined(TARGET_WINDOWS_STORE)
      DebugBreak();
#endif
      throw new XbmcCommons::UncheckedException(__FUNCTION__, "Unhandled error");
    }
  }

	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
	}

	inline float ConvertPixelsToDips(float pixels, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(pixels / (dpi / dipsPerInch) + 0.5f); // Round to nearest integer.
	}

  inline float RationalToFloat(DXGI_RATIONAL rational)
  {
    return RATIONAL_TO_FLOAT(rational);
  }

  inline void GetRefreshRatio(uint32_t refresh, uint32_t *num, uint32_t *den)
  {
    int i = (((refresh + 1) % 24) == 0 || ((refresh + 1) % 30) == 0) ? 1 : 0;
    *num = (refresh + i) * 1000;
    *den = 1000 + i;
  }

  inline std::string GetErrorDescription(HRESULT hr)
  {
    using namespace KODI::PLATFORM::WINDOWS;

    WCHAR buff[2048];
    DXGetErrorDescriptionW(hr, buff, 2048);

    return FromW(StringUtils::Format(L"%X - %s (%s)", hr, DXGetErrorStringW(hr), buff));
  }

  template <typename T> struct SizeGen
  {
    SizeGen<T>() { Width = Height = 0; }
    SizeGen<T>(T width, T height) { Width = width; Height = height; }

    bool operator !=(const SizeGen<T> &size) const
    {
      return Width != size.Width || Height != size.Height;
    }

    const SizeGen<T> &operator -=(const SizeGen<T> &size)
    {
      Width -= size.Width;
      Height -= size.Height;
      return *this;
    };

    const SizeGen<T> &operator +=(const SizeGen<T> &size)
    {
      Width += size.Width;
      Height += size.Height;
      return *this;
    };

    const SizeGen<T> &operator -=(const T &size)
    {
      Width -= size;
      Height -= size;
      return *this;
    };

    const SizeGen<T> &operator +=(const T &size)
    {
      Width += size;
      Height += size;
      return *this;
    };

    T Width, Height;
  };

#if defined(_DEBUG)
	// Check for SDK Layer support.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			nullptr,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
			);

		return SUCCEEDED(hr);
	}
#endif
}

#ifdef TARGET_WINDOWS_DESKTOP
namespace winrt
{
  namespace Windows
  {
    namespace Foundation
    {
      typedef DX::SizeGen<float>  Size;
      typedef DX::SizeGen<int>    SizeInt;
    }
  }
}
#endif

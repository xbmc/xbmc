/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

namespace winrt
{
using namespace Windows::Foundation;
}
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;

struct __declspec(uuid("905a0fef-bc53-11df-8c49-001e4fc686da")) IBufferByteAccess : ::IUnknown
{
  virtual HRESULT __stdcall Buffer(void** value) = 0;
};

struct CustomBuffer : winrt::implements<CustomBuffer, IBuffer, IBufferByteAccess>
{
  void* m_address;
  uint32_t m_capacity;
  uint32_t m_length;

  CustomBuffer(void* address, uint32_t capacity)
      : m_address(address)
      , m_capacity(capacity)
      , m_length(0)
  {
  }
  uint32_t Capacity() const
  {
    return m_capacity;
  }
  uint32_t Length() const
  {
    return m_length;
  }
  void Length(uint32_t length)
  {
    m_length = length;
  }

  HRESULT __stdcall Buffer(void** value) final
  {
    *value = m_address;
    return S_OK;
  }
};
/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>

#include <d3d11.h>
#include <wrl/client.h>
extern "C"
{
#include <libavutil/pixfmt.h>
}

/*!
 \brief Loads the colour management 3D LUT for a source's primaries and hands it to the output
 shader.

 Loading happens on the calling thread, which must be the render thread: the view is handed to
 the output shader the render thread draws with, and creating it uses the immediate context.
 */
class CLutLoader
{
public:
  //! Reads the LUT for \p primaries into \p view and sets \p lutSize; false when there is none
  using LoadFunction = std::function<bool(AVColorPrimaries primaries,
                                          int& lutSize,
                                          Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& view)>;
  //! Hands a LUT to the output shader; a size of 0 is no LUT
  using ApplyFunction = std::function<void(int lutSize, ID3D11ShaderResourceView* view)>;

  CLutLoader(LoadFunction load, ApplyFunction apply);

  void Load(AVColorPrimaries primaries);

  int GetSize() const { return m_size; }
  ID3D11ShaderResourceView* GetView() const { return m_view.Get(); }

private:
  LoadFunction m_load;
  ApplyFunction m_apply;
  int m_size{0};
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_view;
};

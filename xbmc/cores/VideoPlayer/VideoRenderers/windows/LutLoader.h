/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <optional>

#include <d3d11.h>
#include <ppltasks.h>
#include <wrl/client.h>
extern "C"
{
#include <libavutil/pixfmt.h>
}

/*!
 \brief Loads the colour management 3D LUT for a source's primaries on a worker thread, and hands
 the result over on the thread that owns the output shader.

 A load still in flight when the loader is destroyed is abandoned: it runs to its end on the
 worker and its result is never applied.
 */
class CLutLoader
{
public:
  struct Lut
  {
    int size{0}; //!< 0 is no LUT
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
    std::optional<int> cmsToken; //!< set when the colour manager produced the LUT data
  };

  //! Runs on a worker thread and can outlive the loader, so it must own everything it uses
  using LoadFunction = std::function<Lut(AVColorPrimaries primaries)>;
  //! Runs on the thread that calls ApplyFinishedLoad()
  using ApplyFunction = std::function<void(const Lut& lut)>;

  CLutLoader(LoadFunction load, ApplyFunction apply);

  //! Starts a load on a worker thread. A load still in flight is abandoned.
  void Load(AVColorPrimaries primaries);
  bool IsLoading() const { return m_task.has_value(); }
  //! Hands the result of a load that has finished to the apply function, without waiting for one
  void ApplyFinishedLoad();

private:
  LoadFunction m_load;
  ApplyFunction m_apply;
  std::optional<Concurrency::task<Lut>> m_task;
};

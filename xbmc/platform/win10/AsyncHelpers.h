/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ppl.h>
#include <ppltasks.h>
#include <sdkddkver.h>

namespace winrt
{
  using namespace Windows::Foundation;
}

inline bool is_sta()
{
#ifdef NTDDI_WIN10_CO // Windows SDK 10.0.22000.0 or newer
  return winrt::impl::is_sta_thread();
#else
  return winrt::impl::is_sta();
#endif
}

inline void Wait(const winrt::IAsyncAction& asyncOp)
{
  if (asyncOp.Status() == winrt::AsyncStatus::Completed)
    return;

  if (!is_sta())
    return asyncOp.get();

  auto __sync = std::make_shared<Concurrency::event>();
  asyncOp.Completed([&](auto&&, auto&&) {
    __sync->set();
  });
  __sync->wait();
}

template <typename TResult, typename TProgress> inline
TResult Wait(const winrt::IAsyncOperationWithProgress<TResult, TProgress>& asyncOp)
{
  if (asyncOp.Status() == winrt::AsyncStatus::Completed)
    return asyncOp.GetResults();

  if (!is_sta())
    return asyncOp.get();

  auto __sync = std::make_shared<Concurrency::event>();
  asyncOp.Completed([&](auto&&, auto&&) {
    __sync->set();
  });
  __sync->wait();

  return asyncOp.GetResults();
}

template <typename TResult> inline
TResult Wait(const winrt::IAsyncOperation<TResult>& asyncOp)
{
  if (asyncOp.Status() == winrt::AsyncStatus::Completed)
    return asyncOp.GetResults();

  if (!is_sta())
    return asyncOp.get();

  auto __sync = std::make_shared<Concurrency::event>();
  asyncOp.Completed([&](auto&&, auto&&)
  {
    __sync->set();
  });
  __sync->wait();

  return asyncOp.GetResults();
}

template <typename TResult> inline
TResult Wait(const Concurrency::task<TResult>& asyncOp)
{
  if (asyncOp.is_done())
    return asyncOp.get();

  if (!is_sta()) // blocking suspend is allowed
    return asyncOp.get();

  auto _sync = std::make_shared<Concurrency::event>();
  asyncOp.then([&](TResult result)
  {
    _sync->set();
  });
  _sync->wait();

  return asyncOp.get();
}

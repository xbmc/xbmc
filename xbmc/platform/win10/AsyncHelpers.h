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

namespace winrt
{
  using namespace Windows::Foundation;
}

inline void Wait(const winrt::IAsyncAction& asyncOp)
{
  if (asyncOp.Status() == winrt::AsyncStatus::Completed)
    return;

  if (!winrt::impl::is_sta())
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

  if (!winrt::impl::is_sta())
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

  if (!winrt::impl::is_sta())
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

  if (!winrt::impl::is_sta()) // blocking suspend is allowed
    return asyncOp.get();

  auto _sync = std::make_shared<Concurrency::event>();
  asyncOp.then([&](TResult result)
  {
    _sync->set();
  });
  _sync->wait();

  return asyncOp.get();
}

/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#ifdef TARGET_WINDOWS_STORE

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

#endif

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

inline void Wait(Windows::Foundation::IAsyncAction^ asyncOp)
{
  auto _sync = std::make_shared<Concurrency::event>();

  auto workItem = ref new Windows::System::Threading::WorkItemHandler(
    [&](Windows::Foundation::IAsyncAction^ workItem)
  {
    Concurrency::task<void>(asyncOp).then(
      [&]()
    {
      _sync->set();
    });
  });
  Windows::System::Threading::ThreadPool::RunAsync(workItem);
  _sync->wait();
}

template <typename TResult, typename TProgress> inline
TResult Wait(Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress>^ asyncOp)
{
  auto _sync = std::make_shared<Concurrency::event>();

  auto workItem = ref new Windows::System::Threading::WorkItemHandler(
    [&](Windows::Foundation::IAsyncAction^ workItem)
  {
    Concurrency::task<TResult>(asyncOp).then(
      [&](TResult result)
    {
      _sync->set();
    });
  });
  Windows::System::Threading::ThreadPool::RunAsync(workItem);
  _sync->wait();

  return asyncOp->GetResults();
}

template <typename TResult> inline
TResult Wait(Windows::Foundation::IAsyncOperation<TResult>^ asyncOp)
{
  auto _sync = std::make_shared<Concurrency::event>();

  auto workItem = ref new Windows::System::Threading::WorkItemHandler(
    [&](Windows::Foundation::IAsyncAction^ workItem)
  {
    Concurrency::task<TResult>(asyncOp).then(
      [&](TResult result)
    {
      _sync->set();
    });
  });
  Windows::System::Threading::ThreadPool::RunAsync(workItem);
  _sync->wait();

  return asyncOp->GetResults();
}

template <typename TResult> inline
TResult Wait(Concurrency::task<TResult> &asyncOp)
{
  auto _sync = std::make_shared<Concurrency::event>();

  auto workItem = ref new Windows::System::Threading::WorkItemHandler(
    [&](Windows::Foundation::IAsyncAction^ workItem)
  {
    asyncOp.then(
      [&](TResult result)
    {
      _sync->set();
    });
  });
  Windows::System::Threading::ThreadPool::RunAsync(workItem);
  _sync->wait();

  return asyncOp.get();
}

#endif
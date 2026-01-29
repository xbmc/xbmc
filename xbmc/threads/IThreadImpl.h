/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

#include <memory>
#include <string>
#include <thread>

class IThreadImpl
{
public:
  virtual ~IThreadImpl() = default;

  static std::unique_ptr<IThreadImpl> CreateThreadImpl(std::thread::native_handle_type handle);

  /*!
   * \brief Set the thread name and other info (platform dependent)
   *
   */
  virtual void SetThreadInfo(const std::string& name) = 0;

  /*!
   * \brief Set the thread priority via the native threading library
   *
   */
  virtual bool SetPriority(const ThreadPriority& priority) = 0;

  /*!
   * \brief Assign the current thread a task for OS scheduling (platform dependent)
   * \param[in] task Type of task
   * \return true for success, false for failure
   */
  virtual bool SetTask(const ThreadTask& task) { return true; }

  /*!
   * \brief Revert the current thread to normal scheduling (platform dependent)
   * \return true for success, false for failure
   */
  virtual bool RevertTask() { return true; }

protected:
  IThreadImpl(std::thread::native_handle_type handle) : m_handle(handle) {}

  std::thread::native_handle_type m_handle;

private:
  IThreadImpl() = delete;
};

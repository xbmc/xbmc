/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace JSONRPC
{
/*!
 * \brief Hand a payload to the messenger. The message's handler frees it once it has run,
 * whether the message was sent or posted.
 */
template<typename T>
void* TransferToMessenger(std::unique_ptr<T> payload)
{
  return payload.release();
}

/*!
 * \brief Lend a payload to a synchronous message. The caller keeps it, so only SendMsg may
 * carry it.
 */
template<typename T>
void* LendToMessenger(T& payload)
{
  return &payload;
}
} // namespace JSONRPC

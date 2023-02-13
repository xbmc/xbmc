/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Lockables.h"

#include <mutex>

class CCriticalSection : public XbmcThreads::CountingLockable<std::recursive_mutex>
{
};

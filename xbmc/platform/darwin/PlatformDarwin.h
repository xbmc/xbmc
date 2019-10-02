/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/PlatformPosix.h"

class CPlatformDarwin : public CPlatformPosix
{
  public:
    /**\brief C'tor */
    CPlatformDarwin() = default;

    /**\brief D'tor */
    ~CPlatformDarwin() override = default;

    void Init() override;
};

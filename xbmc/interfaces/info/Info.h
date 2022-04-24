/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace INFO
{
/*! Default context of the INFO interface
    @note when info conditions are evaluated different contexts can be passed. Context usually refers to the window ids where
    the conditions are being evaluated. By default conditions, skin variables and labels are initialized using the DEFAULT_CONTEXT
    value unless specifically bound to a given window.
    */
constexpr int DEFAULT_CONTEXT = 0;
} // namespace INFO

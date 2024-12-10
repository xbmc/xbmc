/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 * \brief Specifies the types of texture scaling algorithms
 */
enum class TEXTURE_SCALING
{
  UNKNOWN, ///< Unknown scaling method
  LINEAR, ///< Linear scaling method
  NEAREST, ///< Nearest-neighbor scaling method
};

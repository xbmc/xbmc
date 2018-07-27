/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/TransformMatrix.h"
#include "ShaderFormats.h"

void CalculateYUVMatrix(TransformMatrix &matrix
                        , unsigned int  flags
                        , EShaderFormat format
                        , float         black
                        , float         contrast
                        , bool          limited);

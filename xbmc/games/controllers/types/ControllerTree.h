/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//
// Note: Hierarchy of headers is:
//
//  - ControllerTree.h (this file)
//    - ControllerHub.h
//      - PortNode.h
//        - ControllerNode.h
//
#include "ControllerHub.h"

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 *
 * \brief Collection of ports on a console
 */
using CControllerTree = CControllerHub;

} // namespace GAME
} // namespace KODI

/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Dialog title
#define CONTROL_PORT_DIALOG_LABEL 2

// GUI control IDs
#define CONTROL_PORT_LIST 3

// GUI button IDs
#define CONTROL_CLOSE_BUTTON 18
#define CONTROL_RESET_BUTTON 19

// Skin XML file
#define PORT_DIALOG_XML "DialogGameControllers.xml"

/*!
 * \ingroup games
 *
 * \brief The maximum port count, chosen to allow for two Saturn 6 Player Adapters
 */
constexpr unsigned int MAX_PORT_COUNT = 12;

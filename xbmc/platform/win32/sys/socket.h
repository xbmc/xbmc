/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <WS2tcpip.h>
#include <Winsock2.h>
#include <ws2bth.h>

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif

#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif


#ifndef AF_BTH
#define AF_BTH          32
#endif

#ifndef BTHPROTO_RFCOMM
#define BTHPROTO_RFCOMM 3
#endif

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH AF_BTH
#endif

#ifndef BTPROTO_RFCOMM
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
#endif

typedef int socklen_t;


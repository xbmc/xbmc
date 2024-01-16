/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if defined(TARGET_WINDOWS_DESKTOP)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "platform/win32/peripherals/PeripheralBusUSB.h"
#elif defined(TARGET_WINDOWS_STORE)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "platform/win10/peripherals/PeripheralBusUSB.h"
#elif defined(TARGET_LINUX) && defined(HAVE_LIBUDEV)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "platform/linux/peripherals/PeripheralBusUSBLibUdev.h"
#elif defined(TARGET_LINUX) && defined(HAVE_LIBUSB)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "platform/linux/peripherals/PeripheralBusUSBLibUSB.h"
#elif defined(TARGET_FREEBSD) && defined(HAVE_LIBUSB)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "platform/linux/peripherals/PeripheralBusUSBLibUSB.h"
#elif defined(TARGET_DARWIN_OSX)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "platform/darwin/osx/peripherals/PeripheralBusUSB.h"
#endif

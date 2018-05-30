/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#if   defined(TARGET_WINDOWS_DESKTOP)
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
#elif defined(TARGET_DARWIN)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "platform/darwin/osx/peripherals/PeripheralBusUSB.h"
#endif

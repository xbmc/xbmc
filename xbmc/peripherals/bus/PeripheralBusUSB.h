#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://xbmc.org
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

#if   defined(TARGET_WINDOWS)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "win32/PeripheralBusUSB.h"
#elif defined(TARGET_LINUX) && defined(HAVE_LIBUDEV)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "linux/PeripheralBusUSBLibUdev.h"
#elif defined(TARGET_LINUX) && defined(HAVE_LIBUSB)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "linux/PeripheralBusUSBLibUSB.h"
#elif defined(TARGET_FREEBSD) && defined(HAVE_LIBUSB)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "linux/PeripheralBusUSBLibUSB.h"
#elif defined(TARGET_DARWIN)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "osx/PeripheralBusUSB.h"
#elif defined(TARGET_ANDROID)
#define HAVE_PERIPHERAL_BUS_USB 1
#include "linux/PeripheralBusUSBLibUSB.h"
#endif

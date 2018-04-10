/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlatformWin32.h"
#include "platform/win32/input/IRServerSuite.h"
#include "platform/win32/powermanagement/Win32PowerSyscall.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWin32();
}

CPlatformWin32::CPlatformWin32() = default;

CPlatformWin32::~CPlatformWin32() = default;

void CPlatformWin32::Init()
{
  CRemoteControl::Register();
  CWin32PowerSyscall::Register();
}

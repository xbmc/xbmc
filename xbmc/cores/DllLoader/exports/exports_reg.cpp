/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "stdafx.h"
#include "../DllLoader.h"
#include "emu_registry.h"

Export export_advapi32[] =
{
  { "RegCloseKey",                -1, dllRegCloseKey,                NULL },
  { "RegOpenKeyExA",              -1, dllRegOpenKeyExA,              NULL },
  { "RegOpenKeyA",                -1, dllRegOpenKeyA,                NULL },
  { "RegSetValueA",               -1, dllRegSetValueA,               NULL },
  { "RegEnumKeyExA",              -1, dllRegEnumKeyExA,              NULL },
  { "RegDeleteKeyA",              -1, dllRegDeleteKeyA,              NULL },
  { "RegQueryValueExA",           -1, dllRegQueryValueExA,           NULL },
  { "RegQueryValueExW",           -1, dllRegQueryValueExW,           NULL },
  { "RegCreateKeyA",              -1, dllRegCreateKeyA,              NULL },
  { "RegSetValueExA",             -1, dllRegSetValueExA,             NULL },
  { "RegCreateKeyExA",            -1, dllRegCreateKeyExA,            NULL },
  { "RegEnumValueA",              -1, dllRegEnumValueA,              NULL },
  { "RegQueryInfoKeyA",           -1, dllRegQueryInfoKeyA,           NULL },
  { "CryptAcquireContextA",       -1, dllCryptAcquireContextA,       NULL },
  { "CryptGenRandom",             -1, dllCryptGenRandom,             NULL },
  { "CryptReleaseContext",        -1, dllCryptReleaseContext,        NULL },
  { "RegQueryValueA",             -1, dllRegQueryValueA,             NULL },
  { NULL,                         -1, NULL,                          NULL }
};

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
#include "emu_ole32.h"

Export export_ole32[] =
{
  { "CoInitialize",               -1, dllCoInitialize,               NULL },
  { "CoUninitialize",             -1, dllCoUninitialize,             NULL },
  { "CoCreateInstance",           -1, dllCoCreateInstance,           NULL },
  { "CoFreeUnusedLibraries",      -1, dllCoFreeUnusedLibraries,      NULL },
  { "StringFromGUID2",            -1, dllStringFromGUID2,            NULL },
  { "CoTaskMemFree",              -1, dllCoTaskMemFree,              NULL },
  { "CoTaskMemAlloc",             -1, dllCoTaskMemAlloc,             NULL },
  { NULL,                         -1, NULL,                          NULL }
};


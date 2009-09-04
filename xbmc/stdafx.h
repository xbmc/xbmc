// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
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

#define XBMC_MAX_PATH 1024 // normal max path is 260, but smb shares and the like can be longer

#define DEBUG_MOUSE
#define DEBUG_KEYBOARD
#include "system.h"

#include <vector>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <queue>
#include "StdString.h"

#undef GetFreeSpace

#define SAFE_DELETE(p)       { delete (p);     (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=NULL; }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

#if defined(_MSC_VER) && _MSC_VER < 1500 //vs2008 and up already has strnlen, libsmb needs this function
extern "C" {
  size_t strnlen(const char *s, size_t n);
}
#endif

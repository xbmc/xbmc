/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*

  This file is used instead of FileZilla's version.cpp

*/


#include "stdafx.h"
#include "version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define XBFILEZILLA_VERSION "1.4"
#define FILEZILLA_VERSION "0.8.8"

CStdString GetVersionString()
{
  CStdString str;
  str.Format("XBFileZilla version %s, (based on FileZilla Server %s)", XBFILEZILLA_VERSION, FILEZILLA_VERSION);
  return str;
}

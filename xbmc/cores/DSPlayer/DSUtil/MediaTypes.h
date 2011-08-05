/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#pragma pack(1)
struct VIH
{
  VIDEOINFOHEADER vih;
  UINT mask[3];
  int size;
  const GUID* subtype;
};
struct VIH2
{
  VIDEOINFOHEADER2 vih;
  UINT mask[3];
  int size;
  const GUID* subtype;
};
#pragma pack()

extern VIH vihs[];
extern VIH2 vih2s[];

extern int VIHSIZE;

extern CStdStringW VIH2String(int i), Subtype2String(const GUID& subtype);
extern void CorrectMediaType(AM_MEDIA_TYPE* pmt);

/*
 *  LAME MP3 encoder for DirectShow
 *  CLSIDs of the filter and property pages
 *
 *  Copyright (c) 2000-2005 Marie Orlova, Peter Gubanov, Vitaly Ivanov, Elecard Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

// {b8d27088-df5f-4b7c-98dc-0e91a1696286}
DEFINE_GUID(CLSID_LAMEDShowFilter,
0xb8d27088, 0xff5f, 0x4b7c, 0x98, 0xdc, 0x0e, 0x91, 0xa1, 0x69, 0x62, 0x86);


// {b8d27089-df5f-4b7c-98dc-0e91a1696286}
DEFINE_GUID(CLSID_LAMEDShow_PropertyPage,
0xb8d27089, 0xff5f, 0x4b7c, 0x98, 0xdc, 0x0e, 0x91, 0xa1, 0x69, 0x62, 0x86);

// {b8d2708a-df5f-4b7c-98dc-0e91a1696286}
DEFINE_GUID(CLSID_LAMEDShow_PropertyPageAdv,
0xb8d2708a, 0xff5f, 0x4b7c, 0x98, 0xdc, 0x0e, 0x91, 0xa1, 0x69, 0x62, 0x86);

// {FE69EDD1-04CB-11d5-994A-000021D1FE2F}
DEFINE_GUID(CLSID_LAMEDShow_About,
0xfe69edd1, 0xf4cb, 0x11d5, 0x99, 0x4a, 0x0, 0x0, 0x21, 0xd1, 0xfe, 0x2f);

DEFINE_GUID(MEDIASUBTYPE_MP3,
0x00000055, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
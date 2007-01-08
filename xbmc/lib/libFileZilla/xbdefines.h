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

#ifndef __XBDEFINES_H__
#define __XBDEFINES_H__

#define USES_CONVERSION
inline LPCSTR T2CA(LPCTSTR lp) { return lp; }
inline LPSTR T2A(LPTSTR lp) { return lp; }

// common messages
#define WM_DESTROY  0x0100
#define WM_CLOSE    0x0101
#define WM_QUIT     0x0102
#define WM_TIMER    0x0103

#define WM_USER     0x0400

/*
 * PeekMessage() Options
 */
#define PM_NOREMOVE         0x0000
#define PM_REMOVE           0x0001


#define MAKELPARAM(l, h)   ((LPARAM) MAKELONG(l, h)) 

// defines for notifications, handled by CXBServer::OnServerMessage
#define FSM_XBOX           0x8000
#define FSM_FILETRANSFER   (FSM_XBOX + 0x0001)

#define FILETRANSFER_RECV  0x0000
#define FILETRANSFER_SEND  0x8000
#define FILETRANSFER_BEGIN 0x0001
#define FILETRANSFER_END   0x0002
#define FILETRANSFER_ABORT 0x0004


#endif // __XBDEFINES_H__

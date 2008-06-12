#ifndef _EMU_OLE32_H
#define _EMU_OLE32_H

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

extern "C" HRESULT  WINAPI dllCoInitialize();
extern "C" void     WINAPI dllCoUninitialize();
extern "C" HRESULT  WINAPI dllCoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv);
extern "C" void     WINAPI dllCoFreeUnusedLibraries();
extern "C" int      WINAPI dllStringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cchMax);
extern "C" void     WINAPI dllCoTaskMemFree(void * cb);
extern "C" void *   WINAPI dllCoTaskMemAlloc(unsigned long cb);

#endif // _EMU_OLE32_H

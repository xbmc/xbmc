/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#pragma once

MIDL_INTERFACE("B92D8991-6C42-4e51-B942-E61CB8696FCB")
IEVRPresenterCallback : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE PresentSurfaceCB(IDirect3DSurface9 *pSurface) = 0;
};

MIDL_INTERFACE("9019EA9C-F1B4-44b5-ADD5-D25704313E48")
IEVRPresenterRegisterCallback : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE RegisterCallback(IEVRPresenterCallback *pCallback) = 0;
};

MIDL_INTERFACE("4527B2E7-49BE-4b61-A19D-429066D93A99")
IEVRPresenterSettings : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE SetBufferCount(int bufferCount) = 0;
};


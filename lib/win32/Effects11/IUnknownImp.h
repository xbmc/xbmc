//--------------------------------------------------------------------------------------
// File: IUnknownImp.h
//
// Direct3D 11 Effects Helper for COM interop
//
// Lifetime for most Effects objects is based on the the lifetime of the master
// effect, so the reference count is not used.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

#define IUNKNOWN_IMP(Class, Interface, BaseInterface) \
 \
HRESULT STDMETHODCALLTYPE Class##::QueryInterface(REFIID iid, _COM_Outptr_ LPVOID *ppv) override \
{ \
    if( !ppv ) \
        return E_INVALIDARG; \
 \
    *ppv = nullptr; \
    if(IsEqualIID(iid, IID_IUnknown)) \
    { \
        *ppv = (IUnknown*)((Interface*)this); \
    } \
    else if(IsEqualIID(iid, IID_##Interface)) \
    { \
        *ppv = (Interface *)this; \
    } \
    else if(IsEqualIID(iid, IID_##BaseInterface)) \
    { \
        *ppv = (BaseInterface *)this; \
    } \
    else \
    { \
        return E_NOINTERFACE; \
    } \
 \
    return S_OK; \
} \
 \
ULONG STDMETHODCALLTYPE Class##::AddRef() override \
{ \
    return 1; \
} \
 \
ULONG STDMETHODCALLTYPE Class##::Release() override \
{ \
    return 0; \
}

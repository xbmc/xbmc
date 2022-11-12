//--------------------------------------------------------------------------------------
// File: IUnknownImp.h
//
// Direct3D 11 Effects Helper for COM interop
//
// Lifetime for most Effects objects is based on the the lifetime of the master
// effect, so the reference count is not used.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

#define IUNKNOWN_IMP(Class, Interface, BaseInterface) \
 \
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, _COM_Outptr_ LPVOID *ppv) override \
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
ULONG STDMETHODCALLTYPE AddRef() override \
{ \
    return 1; \
} \
 \
ULONG STDMETHODCALLTYPE Release() override \
{ \
    return 0; \
}

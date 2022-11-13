//--------------------------------------------------------------------------------------
// File: EffectAPI.cpp
//
// Effect API entry point
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#include "pchfx.h"

#include <memory>

using namespace D3DX11Effects;

//-------------------------------------------------------------------------------------

struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

typedef std::unique_ptr<void, handle_closer> ScopedHandle;

inline HANDLE safe_handle( HANDLE h ) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

//-------------------------------------------------------------------------------------

static HRESULT LoadBinaryFromFile( _In_z_ LPCWSTR pFileName, _Inout_ std::unique_ptr<uint8_t[]>& data, _Out_ uint32_t& size )
{
    // open the file
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile( safe_handle( CreateFile2( pFileName,
                                                  GENERIC_READ,
                                                  FILE_SHARE_READ,
                                                  OPEN_EXISTING,
                                                  nullptr ) ) );
#else
    ScopedHandle hFile( safe_handle( CreateFileW( pFileName,
                                                  GENERIC_READ,
                                                  FILE_SHARE_READ,
                                                  nullptr,
                                                  OPEN_EXISTING,
                                                  FILE_ATTRIBUTE_NORMAL,
                                                  nullptr ) ) );
#endif

    if ( !hFile )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Get the file size
    FILE_STANDARD_INFO fileInfo;
    if ( !GetFileInformationByHandleEx( hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo) ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // File is too big for 32-bit allocation or contains no data, so reject read
    if ( !fileInfo.EndOfFile.LowPart || fileInfo.EndOfFile.HighPart > 0 )
    {
        return E_FAIL;
    }

    // create enough space for the file data
    data.reset( new uint8_t[ fileInfo.EndOfFile.LowPart ] );
    if (!data)
    {
        return E_OUTOFMEMORY;
    }

    // read the data in
    DWORD BytesRead = 0;
    if (!ReadFile( hFile.get(),
                   data.get(),
                   fileInfo.EndOfFile.LowPart,
                   &BytesRead,
                   nullptr
                 ))
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    if (BytesRead < fileInfo.EndOfFile.LowPart)
    {
        return E_FAIL;
    }

    size = BytesRead;

    return S_OK;
}

//--------------------------------------------------------------------------------------

_Use_decl_annotations_
HRESULT WINAPI D3DX11CreateEffectFromMemory(LPCVOID pData, SIZE_T DataLength, UINT FXFlags,
                                            ID3D11Device *pDevice, ID3DX11Effect **ppEffect, LPCSTR srcName )
{
    if ( !pData || !DataLength || !pDevice || !ppEffect )
        return E_INVALIDARG;

    if ( DataLength > UINT32_MAX )
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    // Note that pData must point to a compiled effect, not HLSL
    VN( *ppEffect = new CEffect( FXFlags & D3DX11_EFFECT_RUNTIME_VALID_FLAGS) );
    VH( ((CEffect*)(*ppEffect))->LoadEffect(pData, static_cast<uint32_t>(DataLength) ) );
    VH( ((CEffect*)(*ppEffect))->BindToDevice(pDevice, (srcName) ? srcName : "D3DX11Effect" ) );

lExit:
    if (FAILED(hr))
    {
        SAFE_RELEASE(*ppEffect);
    }
    return hr;
}

//--------------------------------------------------------------------------------------

_Use_decl_annotations_
HRESULT WINAPI D3DX11CreateEffectFromFile( LPCWSTR pFileName, UINT FXFlags, ID3D11Device *pDevice, ID3DX11Effect **ppEffect )
{
    if ( !pFileName || !pDevice || !ppEffect )
        return E_INVALIDARG;

    std::unique_ptr<uint8_t[]> fileData;
    uint32_t size;
    HRESULT hr = LoadBinaryFromFile( pFileName, fileData, size );
    if ( FAILED(hr) )
        return hr;

    hr = S_OK;

    // Note that pData must point to a compiled effect, not HLSL
    VN( *ppEffect = new CEffect( FXFlags & D3DX11_EFFECT_RUNTIME_VALID_FLAGS) );
    VH( ((CEffect*)(*ppEffect))->LoadEffect( fileData.get(), size ) );

    // Create debug object name from input filename
    CHAR strFileA[MAX_PATH];
    int result = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, pFileName, -1, strFileA, MAX_PATH, nullptr, FALSE );
    if ( !result )
    {
        DPF(0, "Failed to load effect file due to WC to MB conversion failure: %ls", pFileName);
        hr = E_FAIL;
        goto lExit;
    }

    const CHAR* pstrName = strrchr( strFileA, '\\' );
    if (!pstrName)
    {
        pstrName = strFileA;
    }
    else
    {
        pstrName++;
    }

    VH( ((CEffect*)(*ppEffect))->BindToDevice(pDevice, pstrName) );

lExit:
    if (FAILED(hr))
    {
        SAFE_RELEASE(*ppEffect);
    }
    return hr;
}


//--------------------------------------------------------------------------------------

_Use_decl_annotations_
HRESULT D3DX11CompileEffectFromMemory( LPCVOID pData, SIZE_T DataLength, LPCSTR srcName,
                                       const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, UINT HLSLFlags, UINT FXFlags,
                                       ID3D11Device *pDevice, ID3DX11Effect **ppEffect, ID3DBlob **ppErrors )
{
    if ( !pData || !DataLength || !pDevice || !ppEffect )
        return E_INVALIDARG;

    if ( FXFlags & D3DCOMPILE_EFFECT_CHILD_EFFECT )
    {
        DPF(0, "Effect pools (i.e. D3DCOMPILE_EFFECT_CHILD_EFFECT) not supported" );
        return E_NOTIMPL;
    }

    ID3DBlob *blob = nullptr;
    HRESULT hr = D3DCompile( pData, DataLength, srcName, pDefines, pInclude, "", "fx_5_0", HLSLFlags, FXFlags, &blob, ppErrors );
    if ( FAILED(hr) )
    {
        DPF(0, "D3DCompile of fx_5_0 profile failed: %08X", hr );
        return hr;
    }

    hr = S_OK;

    VN( *ppEffect = new CEffect( FXFlags & D3DX11_EFFECT_RUNTIME_VALID_FLAGS ) );
    VH( ((CEffect*)(*ppEffect))->LoadEffect(blob->GetBufferPointer(), static_cast<uint32_t>( blob->GetBufferSize() ) ) );
    SAFE_RELEASE( blob );

    VH( ((CEffect*)(*ppEffect))->BindToDevice(pDevice, (srcName) ? srcName : "D3DX11Effect" ) );

lExit:
    if (FAILED(hr))
    {
        SAFE_RELEASE(*ppEffect);
    }
    return hr;
}

//--------------------------------------------------------------------------------------

_Use_decl_annotations_
HRESULT D3DX11CompileEffectFromFile( LPCWSTR pFileName,
                                     const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude, UINT HLSLFlags, UINT FXFlags,
                                     ID3D11Device *pDevice, ID3DX11Effect **ppEffect, ID3DBlob **ppErrors )
{
    if ( !pFileName || !pDevice || !ppEffect )
        return E_INVALIDARG;

    if ( FXFlags & D3DCOMPILE_EFFECT_CHILD_EFFECT )
    {
        DPF(0, "Effect pools (i.e. D3DCOMPILE_EFFECT_CHILD_EFFECT) not supported" );
        return E_NOTIMPL;
    }

    ID3DBlob *blob = nullptr;

#if (D3D_COMPILER_VERSION >= 46) && ( !defined(WINAPI_FAMILY) || ( (WINAPI_FAMILY != WINAPI_FAMILY_APP) && (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP) ) )

    HRESULT hr = D3DCompileFromFile( pFileName, pDefines, pInclude, "", "fx_5_0", HLSLFlags, FXFlags, &blob, ppErrors );
    if ( FAILED(hr) )
    {
        DPF(0, "D3DCompileFromFile of fx_5_0 profile failed %08X: %ls", hr, pFileName );
        return hr;
    }

#else // D3D_COMPILER_VERSION < 46

    std::unique_ptr<uint8_t[]> fileData;
    uint32_t size;
    HRESULT hr = LoadBinaryFromFile( pFileName, fileData, size );
    if ( FAILED(hr) )
    {
        DPF(0, "Failed to load effect file %08X: %ls", hr, pFileName);
        return hr;
    }

    // Create debug object name from input filename
    CHAR strFileA[MAX_PATH];
    int result = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, pFileName, -1, strFileA, MAX_PATH, nullptr, FALSE );
    if ( !result )
    {
        DPF(0, "Failed to load effect file due to WC to MB conversion failure: %ls", pFileName);
        return E_FAIL;
    }

    const CHAR* pstrName = strrchr( strFileA, '\\' );
    if (!pstrName)
    {
        pstrName = strFileA;
    }
    else
    {
        pstrName++;
    }

    hr = D3DCompile( fileData.get(), size, pstrName, pDefines, pInclude, "", "fx_5_0", HLSLFlags, FXFlags, &blob, ppErrors );
    if ( FAILED(hr) )
    {
        DPF(0, "D3DCompile of fx_5_0 profile failed: %08X", hr );
        return hr;
    }

#endif // D3D_COMPILER_VERSION

    if ( blob->GetBufferSize() > UINT32_MAX)
    {
        SAFE_RELEASE( blob );
        return E_FAIL;
    }

    hr = S_OK;

    VN( *ppEffect = new CEffect( FXFlags & D3DX11_EFFECT_RUNTIME_VALID_FLAGS ) );
    VH( ((CEffect*)(*ppEffect))->LoadEffect(blob->GetBufferPointer(), static_cast<uint32_t>( blob->GetBufferSize() ) ) );
    SAFE_RELEASE( blob );

#if (D3D_COMPILER_VERSION >= 46) && ( !defined(WINAPI_FAMILY) || ( (WINAPI_FAMILY != WINAPI_FAMILY_APP) && (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP) ) )
    // Create debug object name from input filename
    CHAR strFileA[MAX_PATH];
    int result = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, pFileName, -1, strFileA, MAX_PATH, nullptr, FALSE );
    if ( !result  )
    {
        DPF(0, "Failed to load effect file due to WC to MB conversion failure: %ls", pFileName);
        hr = E_FAIL;
        goto lExit;
    }

    const CHAR* pstrName = strrchr( strFileA, '\\' );
    if (!pstrName)
    {
        pstrName = strFileA;
    }
    else
    {
        pstrName++;
    }
#endif

    VH( ((CEffect*)(*ppEffect))->BindToDevice(pDevice, pstrName) );

lExit:
    if (FAILED(hr))
    {
        SAFE_RELEASE(*ppEffect);
    }
    return hr;
}

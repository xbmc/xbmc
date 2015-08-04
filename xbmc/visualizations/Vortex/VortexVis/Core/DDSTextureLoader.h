//--------------------------------------------------------------------------------------
// File: DDSTextureLoader.h
//
// Functions for loading a DDS texture and creating a Direct3D 11 runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#include <d3d11_1.h>

#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#pragma warning(pop)

namespace DirectX
{
    enum DDS_ALPHA_MODE
    {
        DDS_ALPHA_MODE_UNKNOWN       = 0,
        DDS_ALPHA_MODE_STRAIGHT      = 1,
        DDS_ALPHA_MODE_PREMULTIPLIED = 2,
        DDS_ALPHA_MODE_OPAQUE        = 3,
        DDS_ALPHA_MODE_CUSTOM        = 4,
    };

    // Standard version
    HRESULT __cdecl CreateDDSTextureFromMemory( _In_ ID3D11Device* d3dDevice,
                                                _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                                _In_ size_t ddsDataSize,
                                                _Outptr_opt_ ID3D11Resource** texture,
                                                _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                                _In_ size_t maxsize = 0,
                                                _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                              );

    HRESULT __cdecl CreateDDSTextureFromFile( _In_ ID3D11Device* d3dDevice,
                                              _In_z_ const wchar_t* szFileName,
                                              _Outptr_opt_ ID3D11Resource** texture,
                                              _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                              _In_ size_t maxsize = 0,
                                              _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                            );

    // Standard version with optional auto-gen mipmap support
    #if defined(_XBOX_ONE) && defined(_TITLE)
    HRESULT __cdecl CreateDDSTextureFromMemory( _In_ ID3D11DeviceX* d3dDevice,
                                                _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
    HRESULT __cdecl CreateDDSTextureFromMemory( _In_ ID3D11Device* d3dDevice,
                                                _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
                                                _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                                _In_ size_t ddsDataSize,
                                                _Outptr_opt_ ID3D11Resource** texture,
                                                _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                                _In_ size_t maxsize = 0,
                                                _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                              );

    #if defined(_XBOX_ONE) && defined(_TITLE)
    HRESULT __cdecl CreateDDSTextureFromFile( _In_ ID3D11DeviceX* d3dDevice,
                                              _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
    HRESULT __cdecl CreateDDSTextureFromFile( _In_ ID3D11Device* d3dDevice,
                                              _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
                                              _In_z_ const wchar_t* szFileName,
                                              _Outptr_opt_ ID3D11Resource** texture,
                                              _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                              _In_ size_t maxsize = 0,
                                              _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                            );

    // Extended version
    HRESULT __cdecl CreateDDSTextureFromMemoryEx( _In_ ID3D11Device* d3dDevice,
                                                  _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                                  _In_ size_t ddsDataSize,
                                                  _In_ size_t maxsize,
                                                  _In_ D3D11_USAGE usage,
                                                  _In_ unsigned int bindFlags,
                                                  _In_ unsigned int cpuAccessFlags,
                                                  _In_ unsigned int miscFlags,
                                                  _In_ bool forceSRGB,
                                                  _Outptr_opt_ ID3D11Resource** texture,
                                                  _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                                  _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                                );

    HRESULT __cdecl CreateDDSTextureFromFileEx( _In_ ID3D11Device* d3dDevice,
                                                _In_z_ const wchar_t* szFileName,
                                                _In_ size_t maxsize,
                                                _In_ D3D11_USAGE usage,
                                                _In_ unsigned int bindFlags,
                                                _In_ unsigned int cpuAccessFlags,
                                                _In_ unsigned int miscFlags,
                                                _In_ bool forceSRGB,
                                                _Outptr_opt_ ID3D11Resource** texture,
                                                _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                                _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                              );

    // Extended version with optional auto-gen mipmap support
    #if defined(_XBOX_ONE) && defined(_TITLE)
    HRESULT __cdecl CreateDDSTextureFromMemoryEx( _In_ ID3D11DeviceX* d3dDevice,
                                                  _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
    HRESULT __cdecl CreateDDSTextureFromMemoryEx( _In_ ID3D11Device* d3dDevice,
                                                  _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
                                                  _In_reads_bytes_(ddsDataSize) const uint8_t* ddsData,
                                                  _In_ size_t ddsDataSize,
                                                  _In_ size_t maxsize,
                                                  _In_ D3D11_USAGE usage,
                                                  _In_ unsigned int bindFlags,
                                                  _In_ unsigned int cpuAccessFlags,
                                                  _In_ unsigned int miscFlags,
                                                  _In_ bool forceSRGB,
                                                  _Outptr_opt_ ID3D11Resource** texture,
                                                  _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                                  _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                                );

    #if defined(_XBOX_ONE) && defined(_TITLE)
    HRESULT __cdecl CreateDDSTextureFromFileEx( _In_ ID3D11DeviceX* d3dDevice,
                                                _In_opt_ ID3D11DeviceContextX* d3dContext,
    #else
    HRESULT __cdecl CreateDDSTextureFromFileEx( _In_ ID3D11Device* d3dDevice,
                                                _In_opt_ ID3D11DeviceContext* d3dContext,
    #endif
                                                _In_z_ const wchar_t* szFileName,
                                                _In_ size_t maxsize,
                                                _In_ D3D11_USAGE usage,
                                                _In_ unsigned int bindFlags,
                                                _In_ unsigned int cpuAccessFlags,
                                                _In_ unsigned int miscFlags,
                                                _In_ bool forceSRGB,
                                                _Outptr_opt_ ID3D11Resource** texture,
                                                _Outptr_opt_ ID3D11ShaderResourceView** textureView,
                                                _Out_opt_ DDS_ALPHA_MODE* alphaMode = nullptr
                                              );
}
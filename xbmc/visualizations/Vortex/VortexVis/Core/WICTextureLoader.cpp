//--------------------------------------------------------------------------------------
// File: WICTextureLoader.cpp
//
// Function for loading a WIC image and creating a Direct3D 11 runtime texture for it
// (auto-generating mipmaps if possible)
//
// Note: Assumes application has already called CoInitializeEx
//
// Warning: CreateWICTexture* functions are not thread-safe if given a d3dContext instance for
//          auto-gen mipmap support.
//
// Note these functions are useful for images created as simple 2D textures. For
// more complex resources, DDSTextureLoader is an excellent light-weight runtime loader.
// For a full-featured DDS file reader, writer, and texture processing pipeline see
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

// We could load multi-frame images (TIFF/GIF) into a texture array.
// For now, we just load the first frame (note: DirectXTex supports multi-frame images)

// VS 2010's stdint.h conflicts with intsafe.h
#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

#include "WICTextureLoader.h"
#include "DirectXHelpers.h"
#include <wrl.h>
#include <assert.h>
#include <memory>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

//-------------------------------------------------------------------------------------
// WIC Pixel Format Translation Data
//-------------------------------------------------------------------------------------
struct WICTranslate
{
    GUID                wic;
    DXGI_FORMAT         format;
};

static WICTranslate g_WICFormats[] = 
{
    { GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

    { GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
    { GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

    { GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
    { GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

    { GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
    { GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },

    { GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
    { GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

    { GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
    { GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
    { GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
    { GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

    { GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },
};

//-------------------------------------------------------------------------------------
// WIC Pixel Format nearest conversion table
//-------------------------------------------------------------------------------------

struct WICConvert
{
    GUID        source;
    GUID        target;
};

static WICConvert g_WICConvert[] = 
{
    // Note target GUID in this conversion table must be one of those directly supported formats (above).

    { GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

    { GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
    { GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
    { GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
    { GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

    { GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 
    { GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 

    { GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT 
    { GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT 

    { GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

    { GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

    { GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
    { GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
    { GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
    { GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

    { GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

    { GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
    { GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
    { GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
    { GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
    { GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
    { GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
    { GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 

    { GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
    { GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
    { GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
    { GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
    { GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 

    { GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
    { GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    { GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
    { GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
    { GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
#endif

    // We don't support n-channel formats
};

static bool g_WIC2 = false;

//--------------------------------------------------------------------------------------
namespace DirectX
{

bool _IsWIC2()
{
    return g_WIC2;
}

IWICImagingFactory* _GetWIC()
{
    static IWICImagingFactory* s_Factory = nullptr;

    if ( s_Factory )
        return s_Factory;

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory2),
        (LPVOID*)&s_Factory
        );

    if ( SUCCEEDED(hr) )
    {
        // WIC2 is available on Windows 8 and Windows 7 SP1 with KB 2670838 installed
        g_WIC2 = true;
    }
    else
    {
        hr = CoCreateInstance(
            CLSID_WICImagingFactory1,
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IWICImagingFactory),
            (LPVOID*)&s_Factory
            );

        if ( FAILED(hr) )
        {
            s_Factory = nullptr;
            return nullptr;
        }
    }
#else
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory),
        (LPVOID*)&s_Factory
        );

    if ( FAILED(hr) )
    {
        s_Factory = nullptr;
        return nullptr;
    }
#endif

    return s_Factory;
}

} // namespace DirectX


//---------------------------------------------------------------------------------
static DXGI_FORMAT _WICToDXGI( const GUID& guid )
{
    for( size_t i=0; i < _countof(g_WICFormats); ++i )
    {
        if ( memcmp( &g_WICFormats[i].wic, &guid, sizeof(GUID) ) == 0 )
            return g_WICFormats[i].format;
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    if ( g_WIC2 )
    {
        if ( memcmp( &GUID_WICPixelFormat96bppRGBFloat, &guid, sizeof(GUID) ) == 0 )
            return DXGI_FORMAT_R32G32B32_FLOAT;
    }
#endif

    return DXGI_FORMAT_UNKNOWN;
}

//---------------------------------------------------------------------------------
static size_t _WICBitsPerPixel( REFGUID targetGuid )
{
    IWICImagingFactory* pWIC = _GetWIC();
    if ( !pWIC )
        return 0;
 
    ComPtr<IWICComponentInfo> cinfo;
    if ( FAILED( pWIC->CreateComponentInfo( targetGuid, cinfo.GetAddressOf() ) ) )
        return 0;

    WICComponentType type;
    if ( FAILED( cinfo->GetComponentType( &type ) ) )
        return 0;

    if ( type != WICPixelFormat )
        return 0;

    ComPtr<IWICPixelFormatInfo> pfinfo;
    if ( FAILED( cinfo.As( &pfinfo ) ) )
        return 0;

    UINT bpp;
    if ( FAILED( pfinfo->GetBitsPerPixel( &bpp ) ) )
        return 0;

    return bpp;
}


//--------------------------------------------------------------------------------------
static DXGI_FORMAT MakeSRGB( _In_ DXGI_FORMAT format )
{
    switch( format )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    case DXGI_FORMAT_BC1_UNORM:
        return DXGI_FORMAT_BC1_UNORM_SRGB;

    case DXGI_FORMAT_BC2_UNORM:
        return DXGI_FORMAT_BC2_UNORM_SRGB;

    case DXGI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM_SRGB;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

    case DXGI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM_SRGB;

    default:
        return format;
    }
}


//---------------------------------------------------------------------------------
static HRESULT CreateTextureFromWIC( _In_ ID3D11Device* d3dDevice,
                                     _In_opt_ ID3D11DeviceContext* d3dContext,
#if defined(_XBOX_ONE) && defined(_TITLE)
                                     _In_opt_ ID3D11DeviceX* d3dDeviceX,
                                     _In_opt_ ID3D11DeviceContextX* d3dContextX,
#endif
                                     _In_ IWICBitmapFrameDecode *frame,
                                     _In_ size_t maxsize,
                                     _In_ D3D11_USAGE usage,
                                     _In_ unsigned int bindFlags,
                                     _In_ unsigned int cpuAccessFlags,
                                     _In_ unsigned int miscFlags,
                                     _In_ bool forceSRGB,
                                     _Out_opt_ ID3D11Resource** texture,
                                     _Out_opt_ ID3D11ShaderResourceView** textureView )
{
    UINT width, height;
    HRESULT hr = frame->GetSize( &width, &height );
    if ( FAILED(hr) )
        return hr;

    assert( width > 0 && height > 0 );

    if ( !maxsize )
    {
        // This is a bit conservative because the hardware could support larger textures than
        // the Feature Level defined minimums, but doing it this way is much easier and more
        // performant for WIC than the 'fail and retry' model used by DDSTextureLoader

        switch( d3dDevice->GetFeatureLevel() )
        {
            case D3D_FEATURE_LEVEL_9_1:
            case D3D_FEATURE_LEVEL_9_2:
                maxsize = 2048 /*D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
                break;

            case D3D_FEATURE_LEVEL_9_3:
                maxsize = 4096 /*D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
                break;

            case D3D_FEATURE_LEVEL_10_0:
            case D3D_FEATURE_LEVEL_10_1:
                maxsize = 8192 /*D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION*/;
                break;

            default:
                maxsize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
                break;
        }
    }

    assert( maxsize > 0 );

    UINT twidth, theight;
    if ( width > maxsize || height > maxsize )
    {
        float ar = static_cast<float>(height) / static_cast<float>(width);
        if ( width > height )
        {
            twidth = static_cast<UINT>( maxsize );
            theight = static_cast<UINT>( static_cast<float>(maxsize) * ar );
        }
        else
        {
            theight = static_cast<UINT>( maxsize );
            twidth = static_cast<UINT>( static_cast<float>(maxsize) / ar );
        }
        assert( twidth <= maxsize && theight <= maxsize );
    }
    else
    {
        twidth = width;
        theight = height;
    }

    // Determine format
    WICPixelFormatGUID pixelFormat;
    hr = frame->GetPixelFormat( &pixelFormat );
    if ( FAILED(hr) )
        return hr;

    WICPixelFormatGUID convertGUID;
    memcpy( &convertGUID, &pixelFormat, sizeof(WICPixelFormatGUID) );

    size_t bpp = 0;

    DXGI_FORMAT format = _WICToDXGI( pixelFormat );
    if ( format == DXGI_FORMAT_UNKNOWN )
    {
        if ( memcmp( &GUID_WICPixelFormat96bppRGBFixedPoint, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
        {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
            if ( g_WIC2 )
            {
                memcpy( &convertGUID, &GUID_WICPixelFormat96bppRGBFloat, sizeof(WICPixelFormatGUID) );
                format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
            else
#endif
            {
                memcpy( &convertGUID, &GUID_WICPixelFormat128bppRGBAFloat, sizeof(WICPixelFormatGUID) );
                format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }
        else
        {
            for( size_t i=0; i < _countof(g_WICConvert); ++i )
            {
                if ( memcmp( &g_WICConvert[i].source, &pixelFormat, sizeof(WICPixelFormatGUID) ) == 0 )
                {
                    memcpy( &convertGUID, &g_WICConvert[i].target, sizeof(WICPixelFormatGUID) );

                    format = _WICToDXGI( g_WICConvert[i].target );
                    assert( format != DXGI_FORMAT_UNKNOWN );
                    bpp = _WICBitsPerPixel( convertGUID );
                    break;
                }
            }
        }

        if ( format == DXGI_FORMAT_UNKNOWN )
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }
    else
    {
        bpp = _WICBitsPerPixel( pixelFormat );
    }

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    if ( (format == DXGI_FORMAT_R32G32B32_FLOAT) && d3dContext != 0 && textureView != 0 )
    {
        // Special case test for optional device support for autogen mipchains for R32G32B32_FLOAT 
        UINT fmtSupport = 0;
        hr = d3dDevice->CheckFormatSupport( DXGI_FORMAT_R32G32B32_FLOAT, &fmtSupport );
        if ( FAILED(hr) || !( fmtSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) )
        {
            // Use R32G32B32A32_FLOAT instead which is required for Feature Level 10.0 and up
            memcpy( &convertGUID, &GUID_WICPixelFormat128bppRGBAFloat, sizeof(WICPixelFormatGUID) );
            format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            bpp = 128;
        }
    }
#endif

    if ( !bpp )
        return E_FAIL;

    // Handle sRGB formats
    if ( forceSRGB )
    {
        format = MakeSRGB( format );
    }
    else
    {
        ComPtr<IWICMetadataQueryReader> metareader;
        if ( SUCCEEDED( frame->GetMetadataQueryReader( metareader.GetAddressOf() ) ) )
        {
            GUID containerFormat;
            if ( SUCCEEDED( metareader->GetContainerFormat( &containerFormat ) ) )
            {
                // Check for sRGB colorspace metadata
                bool sRGB = false;

                PROPVARIANT value;
                PropVariantInit( &value );

                if ( memcmp( &containerFormat, &GUID_ContainerFormatPng, sizeof(GUID) ) == 0 )
                {
                    // Check for sRGB chunk
                    if ( SUCCEEDED( metareader->GetMetadataByName( L"/sRGB/RenderingIntent", &value ) ) && value.vt == VT_UI1 )
                    {
                        sRGB = true;
                    }
                }
#if defined(_XBOX_ONE) && defined(_TITLE)
                else if ( memcmp( &containerFormat, &GUID_ContainerFormatJpeg, sizeof(GUID) ) == 0 )
                {
                    if ( SUCCEEDED( metareader->GetMetadataByName( L"/app1/ifd/exif/{ushort=40961}", &value ) ) && value.vt == VT_UI2 && value.uiVal == 1 )
                    {
                        sRGB = true;
                    }
                }
                else if ( memcmp( &containerFormat, &GUID_ContainerFormatTiff, sizeof(GUID) ) == 0 )
                {
                    if ( SUCCEEDED( metareader->GetMetadataByName( L"/ifd/exif/{ushort=40961}", &value ) ) && value.vt == VT_UI2 && value.uiVal == 1 )
                    {
                        sRGB = true;
                    }
                }
#else
                else if ( SUCCEEDED( metareader->GetMetadataByName( L"System.Image.ColorSpace", &value ) ) && value.vt == VT_UI2 && value.uiVal == 1 )
                {
                    sRGB = true;
                }
#endif

                PropVariantClear( &value );

                if ( sRGB )
                    format = MakeSRGB( format );
            }
        }
    }

    // Verify our target format is supported by the current device
    // (handles WDDM 1.0 or WDDM 1.1 device driver cases as well as DirectX 11.0 Runtime without 16bpp format support)
    UINT support = 0;
    hr = d3dDevice->CheckFormatSupport( format, &support );
    if ( FAILED(hr) || !(support & D3D11_FORMAT_SUPPORT_TEXTURE2D) )
    {
        // Fallback to RGBA 32-bit format which is supported by all devices
        memcpy( &convertGUID, &GUID_WICPixelFormat32bppRGBA, sizeof(WICPixelFormatGUID) );
        format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bpp = 32;
    }

    // Allocate temporary memory for image
    size_t rowPitch = ( twidth * bpp + 7 ) / 8;
    size_t imageSize = rowPitch * theight;

    std::unique_ptr<uint8_t[]> temp( new (std::nothrow) uint8_t[ imageSize ] );
    if (!temp)
        return E_OUTOFMEMORY;

    // Load image data
    if ( memcmp( &convertGUID, &pixelFormat, sizeof(GUID) ) == 0
         && twidth == width
         && theight == height )
    {
        // No format conversion or resize needed
        hr = frame->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
        if ( FAILED(hr) )
            return hr;
    }
    else if ( twidth != width || theight != height )
    {
        // Resize
        IWICImagingFactory* pWIC = _GetWIC();
        if ( !pWIC )
            return E_NOINTERFACE;

        ComPtr<IWICBitmapScaler> scaler;
        hr = pWIC->CreateBitmapScaler( scaler.GetAddressOf() );
        if ( FAILED(hr) )
            return hr;

        hr = scaler->Initialize( frame, twidth, theight, WICBitmapInterpolationModeFant );
        if ( FAILED(hr) )
            return hr;

        WICPixelFormatGUID pfScaler;
        hr = scaler->GetPixelFormat( &pfScaler );
        if ( FAILED(hr) )
            return hr;

        if ( memcmp( &convertGUID, &pfScaler, sizeof(GUID) ) == 0 )
        {
            // No format conversion needed
            hr = scaler->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
            if ( FAILED(hr) )
                return hr;
        }
        else
        {
            ComPtr<IWICFormatConverter> FC;
            hr = pWIC->CreateFormatConverter( FC.GetAddressOf() );
            if ( FAILED(hr) )
                return hr;

            BOOL canConvert = FALSE;
            hr = FC->CanConvert( pfScaler, convertGUID, &canConvert );
            if ( FAILED(hr) || !canConvert )
            {
                return E_UNEXPECTED;
            }

            hr = FC->Initialize( scaler.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
            if ( FAILED(hr) )
                return hr;

            hr = FC->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
            if ( FAILED(hr) )
                return hr;
        }
    }
    else
    {
        // Format conversion but no resize
        IWICImagingFactory* pWIC = _GetWIC();
        if ( !pWIC )
            return E_NOINTERFACE;

        ComPtr<IWICFormatConverter> FC;
        hr = pWIC->CreateFormatConverter( FC.GetAddressOf() );
        if ( FAILED(hr) )
            return hr;

        BOOL canConvert = FALSE;
        hr = FC->CanConvert( pixelFormat, convertGUID, &canConvert );
        if ( FAILED(hr) || !canConvert )
        {
            return E_UNEXPECTED;
        }

        hr = FC->Initialize( frame, convertGUID, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom );
        if ( FAILED(hr) )
            return hr;

        hr = FC->CopyPixels( 0, static_cast<UINT>( rowPitch ), static_cast<UINT>( imageSize ), temp.get() );  
        if ( FAILED(hr) )
            return hr;
    }

    // See if format is supported for auto-gen mipmaps (varies by feature level)
    bool autogen = false;
    if ( d3dContext != 0 && textureView != 0 ) // Must have context and shader-view to auto generate mipmaps
    {
        UINT fmtSupport = 0;
        hr = d3dDevice->CheckFormatSupport( format, &fmtSupport );
        if ( SUCCEEDED(hr) && ( fmtSupport & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) )
        {
            autogen = true;
#if defined(_XBOX_ONE) && defined(_TITLE)
            if ( !d3dDeviceX || !d3dContextX )
                return E_INVALIDARG;
#endif
        }
    }

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = twidth;
    desc.Height = theight;
    desc.MipLevels = (autogen) ? 0 : 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = usage;
    desc.CPUAccessFlags = cpuAccessFlags;

    if ( autogen )
    {
        desc.BindFlags = bindFlags | D3D11_BIND_RENDER_TARGET;
        desc.MiscFlags = miscFlags | D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    else
    {
        desc.BindFlags = bindFlags;
        desc.MiscFlags = miscFlags;
    }

    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = temp.get();
    initData.SysMemPitch = static_cast<UINT>( rowPitch );
    initData.SysMemSlicePitch = static_cast<UINT>( imageSize );

    ID3D11Texture2D* tex = nullptr;
    hr = d3dDevice->CreateTexture2D( &desc, (autogen) ? nullptr : &initData, &tex );
    if ( SUCCEEDED(hr) && tex != 0 )
    {
        if (textureView != 0)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            memset( &SRVDesc, 0, sizeof( SRVDesc ) );
            SRVDesc.Format = desc.Format;

            SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            SRVDesc.Texture2D.MipLevels = (autogen) ? -1 : 1;

            hr = d3dDevice->CreateShaderResourceView( tex, &SRVDesc, textureView );
            if ( FAILED(hr) )
            {
                tex->Release();
                return hr;
            }

            if ( autogen )
            {
                assert( d3dContext != 0 );

#if defined(_XBOX_ONE) && defined(_TITLE)
                ID3D11Texture2D *pStaging = nullptr;
                CD3D11_TEXTURE2D_DESC stagingDesc( format, twidth, theight, 1, 1, 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, 1, 0, 0 );
                D3D11_SUBRESOURCE_DATA initData;
                initData.pSysMem =  temp.get();
                initData.SysMemPitch = static_cast<UINT>(rowPitch);
                initData.SysMemSlicePitch = static_cast<UINT>(imageSize);

                hr = d3dDevice->CreateTexture2D( &stagingDesc, &initData, &pStaging );
                if ( SUCCEEDED(hr) )
                {
                    d3dContext->CopySubresourceRegion( tex, 0, 0, 0, 0, pStaging, 0, nullptr );

                    UINT64 copyFence = d3dContextX->InsertFence(0);
                    while( d3dDeviceX->IsFencePending( copyFence ) ) { SwitchToThread(); }
                    pStaging->Release();
                }
#else
                d3dContext->UpdateSubresource( tex, 0, nullptr, temp.get(), static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize) );
#endif
                d3dContext->GenerateMips( *textureView );
            }
        }

        if (texture != 0)
        {
            *texture = tex;
        }
        else
        {
            SetDebugObjectName(tex, "WICTextureLoader");
            tex->Release();
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::CreateWICTextureFromMemory( ID3D11Device* d3dDevice,
                                             const uint8_t* wicData,
                                             size_t wicDataSize,
                                             ID3D11Resource** texture,
                                             ID3D11ShaderResourceView** textureView,
                                             size_t maxsize )
{
    return CreateWICTextureFromMemoryEx( d3dDevice, wicData, wicDataSize, maxsize,
                                         D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
                                         texture, textureView );
}

_Use_decl_annotations_
#if defined(_XBOX_ONE) && defined(_TITLE)
HRESULT DirectX::CreateWICTextureFromMemory( ID3D11DeviceX* d3dDevice,
                                             ID3D11DeviceContextX* d3dContext,
#else
HRESULT DirectX::CreateWICTextureFromMemory( ID3D11Device* d3dDevice,
                                             ID3D11DeviceContext* d3dContext,
#endif
                                             const uint8_t* wicData,
                                             size_t wicDataSize,
                                             ID3D11Resource** texture,
                                             ID3D11ShaderResourceView** textureView,
                                             size_t maxsize )
{
    return CreateWICTextureFromMemoryEx( d3dDevice, d3dContext, wicData, wicDataSize, maxsize,
                                         D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
                                         texture, textureView );
}

_Use_decl_annotations_
HRESULT DirectX::CreateWICTextureFromMemoryEx( ID3D11Device* d3dDevice,
                                               const uint8_t* wicData,
                                               size_t wicDataSize,
                                               size_t maxsize,
                                               D3D11_USAGE usage,
                                               unsigned int bindFlags,
                                               unsigned int cpuAccessFlags,
                                               unsigned int miscFlags,
                                               bool forceSRGB,
                                               ID3D11Resource** texture,
                                               ID3D11ShaderResourceView** textureView )
{
    if ( texture )
    {
        *texture = nullptr;
    }
    if ( textureView )
    {
        *textureView = nullptr;
    }

    if (!d3dDevice || !wicData || (!texture && !textureView))
        return E_INVALIDARG;

    if ( !wicDataSize )
        return E_FAIL;

#ifdef _M_AMD64
    if ( wicDataSize > 0xFFFFFFFF )
        return HRESULT_FROM_WIN32( ERROR_FILE_TOO_LARGE );
#endif

    IWICImagingFactory* pWIC = _GetWIC();
    if ( !pWIC )
        return E_NOINTERFACE;

    // Create input stream for memory
    ComPtr<IWICStream> stream;
    HRESULT hr = pWIC->CreateStream( stream.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = stream->InitializeFromMemory( const_cast<uint8_t*>( wicData ), static_cast<DWORD>( wicDataSize ) );
    if ( FAILED(hr) )
        return hr;

    // Initialize WIC
    ComPtr<IWICBitmapDecoder> decoder;
    hr = pWIC->CreateDecoderFromStream( stream.Get(), 0, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, frame.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = CreateTextureFromWIC( d3dDevice, nullptr,
#if defined(_XBOX_ONE) && defined(_TITLE)
                               nullptr, nullptr,
#endif
                               frame.Get(), maxsize,
                               usage, bindFlags, cpuAccessFlags, miscFlags, forceSRGB,
                               texture, textureView );
    if ( FAILED(hr)) 
        return hr;

    if (texture != 0 && *texture != 0)
    {
        SetDebugObjectName(*texture, "WICTextureLoader");
    }

    if (textureView != 0 && *textureView != 0)
    {
        SetDebugObjectName(*textureView, "WICTextureLoader");
    }

    return hr;
}

_Use_decl_annotations_
#if defined(_XBOX_ONE) && defined(_TITLE)
HRESULT DirectX::CreateWICTextureFromMemoryEx( ID3D11DeviceX* d3dDevice,
                                               ID3D11DeviceContextX* d3dContext,
#else
HRESULT DirectX::CreateWICTextureFromMemoryEx( ID3D11Device* d3dDevice,
                                               ID3D11DeviceContext* d3dContext,
#endif
                                               const uint8_t* wicData,
                                               size_t wicDataSize,
                                               size_t maxsize,
                                               D3D11_USAGE usage,
                                               unsigned int bindFlags,
                                               unsigned int cpuAccessFlags,
                                               unsigned int miscFlags,
                                               bool forceSRGB,
                                               ID3D11Resource** texture,
                                               ID3D11ShaderResourceView** textureView )
{
    if ( texture )
    {
        *texture = nullptr;
    }
    if ( textureView )
    {
        *textureView = nullptr;
    }

    if (!d3dDevice || !wicData || (!texture && !textureView))
        return E_INVALIDARG;

    if ( !wicDataSize )
        return E_FAIL;

#ifdef _M_AMD64
    if ( wicDataSize > 0xFFFFFFFF )
        return HRESULT_FROM_WIN32( ERROR_FILE_TOO_LARGE );
#endif

    IWICImagingFactory* pWIC = _GetWIC();
    if ( !pWIC )
        return E_NOINTERFACE;

    // Create input stream for memory
    ComPtr<IWICStream> stream;
    HRESULT hr = pWIC->CreateStream( stream.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = stream->InitializeFromMemory( const_cast<uint8_t*>( wicData ), static_cast<DWORD>( wicDataSize ) );
    if ( FAILED(hr) )
        return hr;

    // Initialize WIC
    ComPtr<IWICBitmapDecoder> decoder;
    hr = pWIC->CreateDecoderFromStream( stream.Get(), 0, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, frame.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = CreateTextureFromWIC( d3dDevice, d3dContext,
#if defined(_XBOX_ONE) && defined(_TITLE)
                               d3dDevice, d3dContext,
#endif
                               frame.Get(), maxsize,
                               usage, bindFlags, cpuAccessFlags, miscFlags, forceSRGB,
                               texture, textureView );
    if ( FAILED(hr)) 
        return hr;

    if (texture != 0 && *texture != 0)
    {
        SetDebugObjectName(*texture, "WICTextureLoader");
    }

    if (textureView != 0 && *textureView != 0)
    {
        SetDebugObjectName(*textureView, "WICTextureLoader");
    }

    return hr;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::CreateWICTextureFromFile( ID3D11Device* d3dDevice,
                                           const wchar_t* fileName,
                                           ID3D11Resource** texture,
                                           ID3D11ShaderResourceView** textureView,
                                           size_t maxsize )
{
    return CreateWICTextureFromFileEx( d3dDevice, fileName, maxsize,
                                       D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
                                       texture, textureView );
}

_Use_decl_annotations_
#if defined(_XBOX_ONE) && defined(_TITLE)
HRESULT DirectX::CreateWICTextureFromFile( ID3D11DeviceX* d3dDevice,
                                           ID3D11DeviceContextX* d3dContext,
#else
HRESULT DirectX::CreateWICTextureFromFile( ID3D11Device* d3dDevice,
                                           ID3D11DeviceContext* d3dContext,
#endif
                                           const wchar_t* fileName,
                                           ID3D11Resource** texture,
                                           ID3D11ShaderResourceView** textureView,
                                           size_t maxsize )
{
    return CreateWICTextureFromFileEx( d3dDevice, d3dContext, fileName, maxsize,
                                       D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
                                       texture, textureView );
}

_Use_decl_annotations_
HRESULT DirectX::CreateWICTextureFromFileEx( ID3D11Device* d3dDevice,
                                             const wchar_t* fileName,
                                             size_t maxsize,
                                             D3D11_USAGE usage,
                                             unsigned int bindFlags,
                                             unsigned int cpuAccessFlags,
                                             unsigned int miscFlags,
                                             bool forceSRGB,
                                             ID3D11Resource** texture,
                                             ID3D11ShaderResourceView** textureView )
{
    if ( texture )
    {
        *texture = nullptr;
    }
    if ( textureView )
    {
        *textureView = nullptr;
    }

    if (!d3dDevice || !fileName || (!texture && !textureView))
        return E_INVALIDARG;

    IWICImagingFactory* pWIC = _GetWIC();
    if ( !pWIC )
        return E_NOINTERFACE;

    // Initialize WIC
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = pWIC->CreateDecoderFromFilename( fileName, 0, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, frame.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = CreateTextureFromWIC( d3dDevice, nullptr,
#if defined(_XBOX_ONE) && defined(_TITLE)
                               nullptr, nullptr,
#endif
                               frame.Get(), maxsize,
                               usage, bindFlags, cpuAccessFlags, miscFlags, forceSRGB,
                               texture, textureView );

#if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
    if ( SUCCEEDED(hr) )
    {
        if (texture != 0 || textureView != 0)
        {
            CHAR strFileA[MAX_PATH];
            int result = WideCharToMultiByte( CP_ACP,
                                              WC_NO_BEST_FIT_CHARS,
                                              fileName,
                                              -1,
                                              strFileA,
                                              MAX_PATH,
                                              nullptr,
                                              FALSE
                               );
            if ( result > 0 )
            {
                const CHAR* pstrName = strrchr( strFileA, '\\' );
                if (!pstrName)
                {
                    pstrName = strFileA;
                }
                else
                {
                    pstrName++;
                }

                if (texture != 0 && *texture != 0)
                {
                    (*texture)->SetPrivateData( WKPDID_D3DDebugObjectName,
                                                static_cast<UINT>( strnlen_s(pstrName, MAX_PATH) ),
                                                pstrName
                                              );
                }

                if (textureView != 0 && *textureView != 0 )
                {
                    (*textureView)->SetPrivateData( WKPDID_D3DDebugObjectName,
                                                    static_cast<UINT>( strnlen_s(pstrName, MAX_PATH) ),
                                                    pstrName
                                                  );
                }
            }
        }
    }
#endif

    return hr;
}

_Use_decl_annotations_
#if defined(_XBOX_ONE) && defined(_TITLE)
HRESULT DirectX::CreateWICTextureFromFileEx( ID3D11DeviceX* d3dDevice,
                                             ID3D11DeviceContextX* d3dContext,
#else
HRESULT DirectX::CreateWICTextureFromFileEx( ID3D11Device* d3dDevice,
                                             ID3D11DeviceContext* d3dContext,
#endif
                                             const wchar_t* fileName,
                                             size_t maxsize,
                                             D3D11_USAGE usage,
                                             unsigned int bindFlags,
                                             unsigned int cpuAccessFlags,
                                             unsigned int miscFlags,
                                             bool forceSRGB,
                                             ID3D11Resource** texture,
                                             ID3D11ShaderResourceView** textureView )
{
    if ( texture )
    {
        *texture = nullptr;
    }
    if ( textureView )
    {
        *textureView = nullptr;
    }

    if (!d3dDevice || !fileName || (!texture && !textureView))
        return E_INVALIDARG;

    IWICImagingFactory* pWIC = _GetWIC();
    if ( !pWIC )
        return E_NOINTERFACE;

    // Initialize WIC
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = pWIC->CreateDecoderFromFilename( fileName, 0, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame( 0, frame.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = CreateTextureFromWIC( d3dDevice, d3dContext,
#if defined(_XBOX_ONE) && defined(_TITLE)
                               d3dDevice, d3dContext,
#endif
                               frame.Get(), maxsize,
                               usage, bindFlags, cpuAccessFlags, miscFlags, forceSRGB,
                               texture, textureView );

#if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
    if ( SUCCEEDED(hr) )
    {
        if (texture != 0 || textureView != 0)
        {
            CHAR strFileA[MAX_PATH];
            int result = WideCharToMultiByte( CP_ACP,
                                              WC_NO_BEST_FIT_CHARS,
                                              fileName,
                                              -1,
                                              strFileA,
                                              MAX_PATH,
                                              nullptr,
                                              FALSE
                               );
            if ( result > 0 )
            {
                const CHAR* pstrName = strrchr( strFileA, '\\' );
                if (!pstrName)
                {
                    pstrName = strFileA;
                }
                else
                {
                    pstrName++;
                }

                if (texture != 0 && *texture != 0)
                {
                    (*texture)->SetPrivateData( WKPDID_D3DDebugObjectName,
                                                static_cast<UINT>( strnlen_s(pstrName, MAX_PATH) ),
                                                pstrName
                                              );
                }

                if (textureView != 0 && *textureView != 0 )
                {
                    (*textureView)->SetPrivateData( WKPDID_D3DDebugObjectName,
                                                    static_cast<UINT>( strnlen_s(pstrName, MAX_PATH) ),
                                                    pstrName
                                                  );
                }
            }
        }
    }
#endif

    return hr;
}

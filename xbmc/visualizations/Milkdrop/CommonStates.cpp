//--------------------------------------------------------------------------------------
// File: CommonStates.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include <wrl.h>
#include "CommonStates.h"
#include "DirectXHelpers.h"
#include "DemandCreate.h"
#include "SharedResourcePool.h"
#include <mutex>

using namespace DirectX;
using namespace Microsoft::WRL;


// Internal state object implementation class. Only one of these helpers is allocated
// per D3D device, even if there are multiple public facing CommonStates instances.
class CommonStates::Impl
{
public:
    Impl(_In_ ID3D11Device* device)
      : device(device)
    { }

    HRESULT CreateBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend, D3D11_BLEND_OP blendOp, _Out_ ID3D11BlendState** pResult);
    HRESULT CreateDepthStencilState(bool enable, bool writeEnable, _Out_ ID3D11DepthStencilState** pResult);
    HRESULT CreateRasterizerState(D3D11_CULL_MODE cullMode, D3D11_FILL_MODE fillMode, _Out_ ID3D11RasterizerState** pResult);
    HRESULT CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode, _Out_ ID3D11SamplerState** pResult);

    ComPtr<ID3D11Device> device;

    ComPtr<ID3D11BlendState> opaque;
    ComPtr<ID3D11BlendState> alphaBlend;
    ComPtr<ID3D11BlendState> additive;
    ComPtr<ID3D11BlendState> nonPremultiplied;

    ComPtr<ID3D11DepthStencilState> depthNone;
    ComPtr<ID3D11DepthStencilState> depthDefault;
    ComPtr<ID3D11DepthStencilState> depthRead;

    ComPtr<ID3D11RasterizerState> cullNone;
    ComPtr<ID3D11RasterizerState> cullClockwise;
    ComPtr<ID3D11RasterizerState> cullCounterClockwise;
    ComPtr<ID3D11RasterizerState> wireframe;

    ComPtr<ID3D11SamplerState> pointWrap;
    ComPtr<ID3D11SamplerState> pointClamp;
    ComPtr<ID3D11SamplerState> linearWrap;
    ComPtr<ID3D11SamplerState> linearClamp;
    ComPtr<ID3D11SamplerState> anisotropicWrap;
    ComPtr<ID3D11SamplerState> anisotropicClamp;

    std::mutex mutex;

    static SharedResourcePool<ID3D11Device*, Impl> instancePool;
};


// Global instance pool.
SharedResourcePool<ID3D11Device*, CommonStates::Impl> CommonStates::Impl::instancePool;


// Helper for creating blend state objects.
HRESULT CommonStates::Impl::CreateBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend, D3D11_BLEND_OP blendOp, _Out_ ID3D11BlendState** pResult)
{
    D3D11_BLEND_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.RenderTarget[0].BlendEnable = (srcBlend != D3D11_BLEND_ONE) ||
                                       (destBlend != D3D11_BLEND_ZERO);

    desc.RenderTarget[0].SrcBlend  = desc.RenderTarget[0].SrcBlendAlpha  = srcBlend;
    desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = destBlend;
    desc.RenderTarget[0].BlendOp   = desc.RenderTarget[0].BlendOpAlpha   = blendOp;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;

    HRESULT hr = device->CreateBlendState(&desc, pResult);

    if (SUCCEEDED(hr))
        SetDebugObjectName(*pResult, "Milkdrop:CommonStates");

    return hr;
}


// Helper for creating depth stencil state objects.
HRESULT CommonStates::Impl::CreateDepthStencilState(bool enable, bool writeEnable, _Out_ ID3D11DepthStencilState** pResult)
{
    D3D11_DEPTH_STENCIL_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.DepthEnable = enable;
    desc.DepthWriteMask = writeEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    desc.StencilEnable = false;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

    desc.BackFace = desc.FrontFace;

    HRESULT hr = device->CreateDepthStencilState(&desc, pResult);

    if (SUCCEEDED(hr))
        SetDebugObjectName(*pResult, "Milkdrop:CommonStates");

    return hr;
}


// Helper for creating rasterizer state objects.
HRESULT CommonStates::Impl::CreateRasterizerState(D3D11_CULL_MODE cullMode, D3D11_FILL_MODE fillMode, _Out_ ID3D11RasterizerState** pResult)
{
    D3D11_RASTERIZER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.CullMode = cullMode;
    desc.FillMode = fillMode;
    desc.DepthClipEnable = true;
    desc.MultisampleEnable = true;
    desc.AntialiasedLineEnable = true;

    HRESULT hr = device->CreateRasterizerState(&desc, pResult);

    if (SUCCEEDED(hr))
        SetDebugObjectName(*pResult, "Milkdrop:CommonStates");

    return hr;
}


// Helper for creating sampler state objects.
HRESULT CommonStates::Impl::CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode, _Out_ ID3D11SamplerState** pResult)
{
    D3D11_SAMPLER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.Filter = filter;

    desc.AddressU = addressMode;
    desc.AddressV = addressMode;
    desc.AddressW = addressMode;

    desc.MaxAnisotropy = (device->GetFeatureLevel() > D3D_FEATURE_LEVEL_9_1) ? 16 : 2;
    
    desc.MaxLOD = FLT_MAX;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    HRESULT hr = device->CreateSamplerState(&desc, pResult);

    if (SUCCEEDED(hr))
        SetDebugObjectName(*pResult, "Milkdrop:CommonStates");

    return hr;
}


//--------------------------------------------------------------------------------------
// CommonStates
//--------------------------------------------------------------------------------------

// Public constructor.
CommonStates::CommonStates(_In_ ID3D11Device* device)
  : pImpl(Impl::instancePool.DemandCreate(device))
{
}


// Move constructor.
CommonStates::CommonStates(CommonStates&& moveFrom)
  : pImpl(std::move(moveFrom.pImpl))
{
}


// Move assignment.
CommonStates& CommonStates::operator= (CommonStates&& moveFrom)
{
    pImpl = std::move(moveFrom.pImpl);
    return *this;
}


// Public destructor.
CommonStates::~CommonStates()
{
}


//--------------------------------------------------------------------------------------
// Blend states
//--------------------------------------------------------------------------------------

ID3D11BlendState* CommonStates::Opaque() const
{
    return DemandCreate(pImpl->opaque, pImpl->mutex, [&](ID3D11BlendState** pResult)
    {
      return pImpl->CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, pResult);
    });
}


ID3D11BlendState* CommonStates::AlphaBlend() const
{
    return DemandCreate(pImpl->alphaBlend, pImpl->mutex, [&](ID3D11BlendState** pResult)
    {
        return pImpl->CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, pResult);
    });
}


ID3D11BlendState* CommonStates::Additive() const
{
    return DemandCreate(pImpl->additive, pImpl->mutex, [&](ID3D11BlendState** pResult)
    {
        return pImpl->CreateBlendState(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, pResult);
    });
}


ID3D11BlendState* CommonStates::NonPremultiplied() const
{
    return DemandCreate(pImpl->nonPremultiplied, pImpl->mutex, [&](ID3D11BlendState** pResult)
    {
        return pImpl->CreateBlendState(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, pResult);
    });
}

ID3D11BlendState* CommonStates::Maximize() const
{
  return DemandCreate(pImpl->nonPremultiplied, pImpl->mutex, [&](ID3D11BlendState** pResult)
  {
    return pImpl->CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_MAX, pResult);
  });
}


//--------------------------------------------------------------------------------------
// Depth stencil states
//--------------------------------------------------------------------------------------

ID3D11DepthStencilState* CommonStates::DepthNone() const
{
    return DemandCreate(pImpl->depthNone, pImpl->mutex, [&](ID3D11DepthStencilState** pResult)
    {
        return pImpl->CreateDepthStencilState(false, false, pResult);
    });
}


ID3D11DepthStencilState* CommonStates::DepthDefault() const
{
    return DemandCreate(pImpl->depthDefault, pImpl->mutex, [&](ID3D11DepthStencilState** pResult)
    {
        return pImpl->CreateDepthStencilState(true, true, pResult);
    });
}


ID3D11DepthStencilState* CommonStates::DepthRead() const
{
    return DemandCreate(pImpl->depthRead, pImpl->mutex, [&](ID3D11DepthStencilState** pResult)
    {
        return pImpl->CreateDepthStencilState(true, false, pResult);
    });
}


//--------------------------------------------------------------------------------------
// Rasterizer states
//--------------------------------------------------------------------------------------

ID3D11RasterizerState* CommonStates::CullNone() const
{
    return DemandCreate(pImpl->cullNone, pImpl->mutex, [&](ID3D11RasterizerState** pResult)
    {
        return pImpl->CreateRasterizerState(D3D11_CULL_NONE, D3D11_FILL_SOLID, pResult);
    });
}


ID3D11RasterizerState* CommonStates::CullClockwise() const
{
    return DemandCreate(pImpl->cullClockwise, pImpl->mutex, [&](ID3D11RasterizerState** pResult)
    {
        return pImpl->CreateRasterizerState(D3D11_CULL_FRONT, D3D11_FILL_SOLID, pResult);
    });
}


ID3D11RasterizerState* CommonStates::CullCounterClockwise() const
{
    return DemandCreate(pImpl->cullCounterClockwise, pImpl->mutex, [&](ID3D11RasterizerState** pResult)
    {
        return pImpl->CreateRasterizerState(D3D11_CULL_BACK, D3D11_FILL_SOLID, pResult);
    });
}


ID3D11RasterizerState* CommonStates::Wireframe() const
{
    return DemandCreate(pImpl->wireframe, pImpl->mutex, [&](ID3D11RasterizerState** pResult)
    {
        return pImpl->CreateRasterizerState(D3D11_CULL_NONE, D3D11_FILL_WIREFRAME, pResult);
    });
}


//--------------------------------------------------------------------------------------
// Sampler states
//--------------------------------------------------------------------------------------

ID3D11SamplerState* CommonStates::PointWrap() const
{
    return DemandCreate(pImpl->pointWrap, pImpl->mutex, [&](ID3D11SamplerState** pResult)
    {
        return pImpl->CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, pResult);
    });
}


ID3D11SamplerState* CommonStates::PointClamp() const
{
    return DemandCreate(pImpl->pointClamp, pImpl->mutex, [&](ID3D11SamplerState** pResult)
    {
        return pImpl->CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, pResult);
    });
}


ID3D11SamplerState* CommonStates::LinearWrap() const
{
    return DemandCreate(pImpl->linearWrap, pImpl->mutex, [&](ID3D11SamplerState** pResult)
    {
        return pImpl->CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, pResult);
    });
}


ID3D11SamplerState* CommonStates::LinearClamp() const
{
    return DemandCreate(pImpl->linearClamp, pImpl->mutex, [&](ID3D11SamplerState** pResult)
    {
        return pImpl->CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, pResult);
    });
}


ID3D11SamplerState* CommonStates::AnisotropicWrap() const
{
    return DemandCreate(pImpl->anisotropicWrap, pImpl->mutex, [&](ID3D11SamplerState** pResult)
    {
        return pImpl->CreateSamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP, pResult);
    });
}


ID3D11SamplerState* CommonStates::AnisotropicClamp() const
{
    return DemandCreate(pImpl->anisotropicClamp, pImpl->mutex, [&](ID3D11SamplerState** pResult)
    {
        return pImpl->CreateSamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_CLAMP, pResult);
    });
}

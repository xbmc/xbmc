//--------------------------------------------------------------------------------------
// File: EffectNonRuntime.cpp
//
// D3DX11 Effect low-frequency utility functions
// These functions are not intended to be called regularly.  They
// are typically called when creating, cloning, or optimizing an 
// Effect, or reflecting a variable.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#include "pchfx.h"
#include "SOParser.h"

namespace D3DX11Effects
{

extern SUnorderedAccessView g_NullUnorderedAccessView;

SBaseBlock::SBaseBlock() noexcept :
    BlockType(EBT_Invalid),
    IsUserManaged(false),
    AssignmentCount(0),
    pAssignments(nullptr)
{
}

SPassBlock::SPassBlock() noexcept :
    BackingStore{},
    pName(nullptr),
    AnnotationCount(0),
    pAnnotations(nullptr),
    pEffect(nullptr),
    InitiallyValid(true),
    HasDependencies(false)
{
}

STechnique::STechnique() noexcept :
    pName(nullptr),
    PassCount(0),
    pPasses(nullptr),
    AnnotationCount(0),
    pAnnotations(nullptr),
    InitiallyValid( true ),
    HasDependencies( false )
{
}

SGroup::SGroup() noexcept :
    pName(nullptr),
    TechniqueCount(0),
    pTechniques(nullptr),
    AnnotationCount(0),
    pAnnotations(nullptr),
    InitiallyValid( true ),
    HasDependencies( false )
{
}

SDepthStencilBlock::SDepthStencilBlock() noexcept :
    pDSObject(nullptr),
    BackingStore{},
    IsValid(true)
{
    BackingStore.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    BackingStore.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    BackingStore.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    BackingStore.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    BackingStore.DepthEnable = true;
    BackingStore.DepthFunc = D3D11_COMPARISON_LESS;
    BackingStore.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    BackingStore.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    BackingStore.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    BackingStore.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    BackingStore.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    BackingStore.StencilEnable = false;
    BackingStore.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    BackingStore.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
}

SBlendBlock::SBlendBlock() noexcept :
    pBlendObject(nullptr),
    BackingStore{},
    IsValid(true)
{
    BackingStore.AlphaToCoverageEnable = false;
    BackingStore.IndependentBlendEnable = true;
    for( size_t i=0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++ )
    {
        BackingStore.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        BackingStore.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
        BackingStore.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        BackingStore.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
        BackingStore.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
        BackingStore.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        memset(&BackingStore.RenderTarget[i].RenderTargetWriteMask, 0x0F, sizeof(BackingStore.RenderTarget[i].RenderTargetWriteMask));
    }
}

SRasterizerBlock::SRasterizerBlock() noexcept :
    pRasterizerObject(nullptr),
    BackingStore{},
    IsValid(true)
{
    BackingStore.AntialiasedLineEnable = false;
    BackingStore.CullMode = D3D11_CULL_BACK;
    BackingStore.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
    BackingStore.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
    BackingStore.FillMode = D3D11_FILL_SOLID;
    BackingStore.FrontCounterClockwise = false;
    BackingStore.MultisampleEnable = false;
    BackingStore.ScissorEnable = false;
    BackingStore.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    BackingStore.DepthClipEnable = true;
}

SSamplerBlock::SSamplerBlock() noexcept :
    pD3DObject(nullptr),
    BackingStore{}
{
    BackingStore.SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    BackingStore.SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    BackingStore.SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    BackingStore.SamplerDesc.BorderColor[3] = D3D11_DEFAULT_BORDER_COLOR_COMPONENT;
    BackingStore.SamplerDesc.BorderColor[2] = D3D11_DEFAULT_BORDER_COLOR_COMPONENT;
    BackingStore.SamplerDesc.BorderColor[1] = D3D11_DEFAULT_BORDER_COLOR_COMPONENT;
    BackingStore.SamplerDesc.BorderColor[0] = D3D11_DEFAULT_BORDER_COLOR_COMPONENT;
    BackingStore.SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    BackingStore.SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    BackingStore.SamplerDesc.MaxAnisotropy = (UINT32) D3D11_DEFAULT_MAX_ANISOTROPY;
    BackingStore.SamplerDesc.MipLODBias = D3D11_DEFAULT_MIP_LOD_BIAS;
    BackingStore.SamplerDesc.MinLOD = -FLT_MAX;
    BackingStore.SamplerDesc.MaxLOD = FLT_MAX;
}

SShaderBlock::SShaderBlock(SD3DShaderVTable *pVirtualTable) noexcept :
    IsValid(true),
    pVT(pVirtualTable),
    pReflectionData(nullptr),
    pD3DObject(nullptr),
    CBDepCount(0),
    pCBDeps(nullptr),
    SampDepCount(0),
    pSampDeps(nullptr),
    InterfaceDepCount(0),
    pInterfaceDeps(nullptr),
    ResourceDepCount(0),
    pResourceDeps(nullptr),
    UAVDepCount(0),
    pUAVDeps(nullptr),
    TBufferDepCount(0),
    ppTbufDeps(nullptr),
    pInputSignatureBlob(nullptr)
{
}

HRESULT SShaderBlock::OnDeviceBind()
{
    HRESULT hr = S_OK;
    uint32_t  i, j;

    // Update all CB deps
    for (i=0; i<CBDepCount; i++)
    {
        assert(pCBDeps[i].Count);

        for (j=0; j<pCBDeps[i].Count; j++)
        {
            pCBDeps[i].ppD3DObjects[j] = pCBDeps[i].ppFXPointers[j]->pD3DObject;

            if ( !pCBDeps[i].ppD3DObjects[j] )
                VH( E_FAIL );
        }
    }

    // Update all sampler deps
    for (i=0; i<SampDepCount; i++)
    {
        assert(pSampDeps[i].Count);

        for (j=0; j<pSampDeps[i].Count; j++)
        {
            pSampDeps[i].ppD3DObjects[j] = pSampDeps[i].ppFXPointers[j]->pD3DObject;

            if ( !pSampDeps[i].ppD3DObjects[j] )
                VH( E_FAIL );
        }
    }

    // Texture deps will be set automatically on use since they are initially marked dirty.

lExit:
    return hr;
}

extern SD3DShaderVTable g_vtVS;
extern SD3DShaderVTable g_vtGS;
extern SD3DShaderVTable g_vtPS;
extern SD3DShaderVTable g_vtHS;
extern SD3DShaderVTable g_vtDS;
extern SD3DShaderVTable g_vtCS;

EObjectType SShaderBlock::GetShaderType()
{
    if (&g_vtVS == pVT)
        return EOT_VertexShader;
    else if (&g_vtGS == pVT)
        return EOT_GeometryShader;
    else if (&g_vtPS == pVT)
        return EOT_PixelShader;
    else if (&g_vtHS == pVT)
        return EOT_HullShader5;
    else if (&g_vtDS == pVT)
        return EOT_DomainShader5;
    else if (&g_vtCS == pVT)
        return EOT_ComputeShader5;
    
    return EOT_Invalid;
}

#define _SET_BIT(bytes, x) (bytes[x / 8] |= (1 << (x % 8)))

HRESULT SShaderBlock::ComputeStateBlockMask(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask)
{
    HRESULT hr = S_OK;
    uint32_t i, j;
    uint8_t *pSamplerMask = nullptr, *pShaderResourceMask = nullptr, *pConstantBufferMask = nullptr, *pUnorderedAccessViewMask = nullptr, *pInterfaceMask = nullptr;

    switch (GetShaderType())
    {
    case EOT_VertexShader:
    case EOT_VertexShader5:
        pStateBlockMask->VS = 1;
        pSamplerMask = pStateBlockMask->VSSamplers;
        pShaderResourceMask = pStateBlockMask->VSShaderResources;
        pConstantBufferMask = pStateBlockMask->VSConstantBuffers;
        pInterfaceMask = pStateBlockMask->VSInterfaces;
        pUnorderedAccessViewMask = nullptr;
        break;

    case EOT_GeometryShader:
    case EOT_GeometryShader5:
        pStateBlockMask->GS = 1;
        pSamplerMask = pStateBlockMask->GSSamplers;
        pShaderResourceMask = pStateBlockMask->GSShaderResources;
        pConstantBufferMask = pStateBlockMask->GSConstantBuffers;
        pInterfaceMask = pStateBlockMask->GSInterfaces;
        pUnorderedAccessViewMask = nullptr;
        break;

    case EOT_PixelShader:
    case EOT_PixelShader5:
        pStateBlockMask->PS = 1;
        pSamplerMask = pStateBlockMask->PSSamplers;
        pShaderResourceMask = pStateBlockMask->PSShaderResources;
        pConstantBufferMask = pStateBlockMask->PSConstantBuffers;
        pInterfaceMask = pStateBlockMask->PSInterfaces;
        pUnorderedAccessViewMask = &pStateBlockMask->PSUnorderedAccessViews;
        break;

    case EOT_HullShader5:
        pStateBlockMask->HS = 1;
        pSamplerMask = pStateBlockMask->HSSamplers;
        pShaderResourceMask = pStateBlockMask->HSShaderResources;
        pConstantBufferMask = pStateBlockMask->HSConstantBuffers;
        pInterfaceMask = pStateBlockMask->HSInterfaces;
        pUnorderedAccessViewMask = nullptr;
        break;

    case EOT_DomainShader5:
        pStateBlockMask->DS = 1;
        pSamplerMask = pStateBlockMask->DSSamplers;
        pShaderResourceMask = pStateBlockMask->DSShaderResources;
        pConstantBufferMask = pStateBlockMask->DSConstantBuffers;
        pInterfaceMask = pStateBlockMask->DSInterfaces;
        pUnorderedAccessViewMask = nullptr;
        break;

    case EOT_ComputeShader5:
        pStateBlockMask->CS = 1;
        pSamplerMask = pStateBlockMask->CSSamplers;
        pShaderResourceMask = pStateBlockMask->CSShaderResources;
        pConstantBufferMask = pStateBlockMask->CSConstantBuffers;
        pInterfaceMask = pStateBlockMask->CSInterfaces;
        pUnorderedAccessViewMask = &pStateBlockMask->CSUnorderedAccessViews;
        break;

    default:
        assert(0);
        VH(E_FAIL);
    }

    for (i = 0; i < SampDepCount; ++ i)
    {
        for (j = 0; j < pSampDeps[i].Count; ++ j)
        {
            _SET_BIT(pSamplerMask, (pSampDeps[i].StartIndex + j));
        }
    }

    for (i = 0; i < InterfaceDepCount; ++ i)
    {
        for (j = 0; j < pInterfaceDeps[i].Count; ++ j)
        {
            _SET_BIT(pInterfaceMask, (pInterfaceDeps[i].StartIndex + j));
        }
    }

    for (i = 0; i < ResourceDepCount; ++ i)
    {
        for (j = 0; j < pResourceDeps[i].Count; ++ j)
        {
            _SET_BIT(pShaderResourceMask, (pResourceDeps[i].StartIndex + j));
        }
    }

    for (i = 0; i < CBDepCount; ++ i)
    {
        for (j = 0; j < pCBDeps[i].Count; ++ j)
        {
            _SET_BIT(pConstantBufferMask, (pCBDeps[i].StartIndex + j));
        }
    }

    for (i = 0; i < UAVDepCount; ++ i)
    {
        assert( pUnorderedAccessViewMask != 0 );
        _Analysis_assume_( pUnorderedAccessViewMask != 0 );
        for (j = 0; j < pUAVDeps[i].Count; ++ j)
        {
            if( pUAVDeps[i].ppFXPointers[j] != &g_NullUnorderedAccessView )
                _SET_BIT(pUnorderedAccessViewMask, (pUAVDeps[i].StartIndex + j));
        }
    }

lExit:
    return hr;
}

#undef _SET_BIT

HRESULT SShaderBlock::GetShaderDesc(_Out_ D3DX11_EFFECT_SHADER_DESC *pDesc, _In_ bool IsInline)
{
    HRESULT hr = S_OK;
    
    ZeroMemory(pDesc, sizeof(*pDesc));

    pDesc->pInputSignature = pInputSignatureBlob ? (const uint8_t*)pInputSignatureBlob->GetBufferPointer() : nullptr;
    pDesc->IsInline = IsInline;

    if (nullptr != pReflectionData)
    {
        // initialize these only if present; otherwise leave them nullptr or 0
        pDesc->pBytecode = pReflectionData->pBytecode;
        pDesc->BytecodeLength = pReflectionData->BytecodeLength;
        for( size_t iDecl=0; iDecl < D3D11_SO_STREAM_COUNT; ++iDecl )
        {
            pDesc->SODecls[iDecl] = pReflectionData->pStreamOutDecls[iDecl];
        }
        pDesc->RasterizedStream = pReflectionData->RasterizedStream;

        // get # of input & output signature entries
        assert( pReflectionData->pReflection != 0 );
        _Analysis_assume_( pReflectionData->pReflection != 0 );

        D3D11_SHADER_DESC ShaderDesc;
        hr = pReflectionData->pReflection->GetDesc( &ShaderDesc );
        if ( SUCCEEDED(hr) )
        {
            pDesc->NumInputSignatureEntries = ShaderDesc.InputParameters;
            pDesc->NumOutputSignatureEntries = ShaderDesc.OutputParameters;
            pDesc->NumPatchConstantSignatureEntries = ShaderDesc.PatchConstantParameters;
        }
    }

    return hr;
}

HRESULT SShaderBlock::GetVertexShader(_Outptr_ ID3D11VertexShader **ppVS)
{
    if (EOT_VertexShader == GetShaderType() ||
        EOT_VertexShader5 == GetShaderType())
    {
        assert( pD3DObject != 0 );
        _Analysis_assume_( pD3DObject != 0 );
        *ppVS = static_cast<ID3D11VertexShader *>( pD3DObject );
        SAFE_ADDREF(*ppVS);
        return S_OK;
    }
    else
    {
        *ppVS = nullptr;
        DPF(0, "ID3DX11EffectShaderVariable::GetVertexShader: This shader variable is not a vertex shader");
        return D3DERR_INVALIDCALL;
    }
}

HRESULT SShaderBlock::GetGeometryShader(_Outptr_ ID3D11GeometryShader **ppGS)
{
    if (EOT_GeometryShader == GetShaderType() ||
        EOT_GeometryShaderSO == GetShaderType() ||
        EOT_GeometryShader5 == GetShaderType())
    {
        assert( pD3DObject != 0 );
        _Analysis_assume_( pD3DObject != 0 );
        *ppGS = static_cast<ID3D11GeometryShader *>( pD3DObject );
        SAFE_ADDREF(*ppGS);
        return S_OK;
    }
    else
    {
        *ppGS = nullptr;
        DPF(0, "ID3DX11EffectShaderVariable::GetGeometryShader: This shader variable is not a geometry shader");
        return D3DERR_INVALIDCALL;
    }
}

HRESULT SShaderBlock::GetPixelShader(_Outptr_ ID3D11PixelShader **ppPS)
{
    if (EOT_PixelShader == GetShaderType() ||
        EOT_PixelShader5 == GetShaderType())
    {
        assert( pD3DObject != 0 );
        _Analysis_assume_( pD3DObject != 0 );
        *ppPS = static_cast<ID3D11PixelShader *>( pD3DObject );
        SAFE_ADDREF(*ppPS);
        return S_OK;
    }
    else
    {
        *ppPS = nullptr;
        DPF(0, "ID3DX11EffectShaderVariable::GetPixelShader: This shader variable is not a pixel shader");
        return D3DERR_INVALIDCALL;
    }
}

HRESULT SShaderBlock::GetHullShader(_Outptr_ ID3D11HullShader **ppHS)
{
    if (EOT_HullShader5 == GetShaderType())
    {
        assert( pD3DObject != 0 );
        _Analysis_assume_( pD3DObject != 0 );
        *ppHS = static_cast<ID3D11HullShader *>( pD3DObject );
        SAFE_ADDREF(*ppHS);
        return S_OK;
    }
    else
    {
        *ppHS = nullptr;
        DPF(0, "ID3DX11EffectShaderVariable::GetHullShader: This shader variable is not a hull shader");
        return D3DERR_INVALIDCALL;
    }
}

HRESULT SShaderBlock::GetDomainShader(_Outptr_ ID3D11DomainShader **ppDS)
{
    if (EOT_DomainShader5 == GetShaderType())
    {
        assert( pD3DObject != 0 );
        _Analysis_assume_( pD3DObject != 0 );
        *ppDS = static_cast<ID3D11DomainShader *>( pD3DObject );
        SAFE_ADDREF(*ppDS);
        return S_OK;
    }
    else
    {
        *ppDS = nullptr;
        DPF(0, "ID3DX11EffectShaderVariable::GetDomainShader: This shader variable is not a domain shader");
        return D3DERR_INVALIDCALL;
    }
}

HRESULT SShaderBlock::GetComputeShader(_Outptr_ ID3D11ComputeShader **ppCS)
{
    if (EOT_ComputeShader5 == GetShaderType())
    {
        assert( pD3DObject != 0 );
        _Analysis_assume_( pD3DObject != 0 );
        *ppCS = static_cast<ID3D11ComputeShader *>( pD3DObject );
        SAFE_ADDREF(*ppCS);
        return S_OK;
    }
    else
    {
        *ppCS = nullptr;
        DPF(0, "ID3DX11EffectShaderVariable::GetComputeShader: This shader variable is not a compute shader");
        return D3DERR_INVALIDCALL;
    }
}

_Use_decl_annotations_
HRESULT SShaderBlock::GetSignatureElementDesc(ESigType SigType, uint32_t Element, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
    HRESULT hr = S_OK;
    LPCSTR pFuncName = nullptr;
    switch( SigType )
    {
    case ST_Input:
        pFuncName = "ID3DX11EffectShaderVariable::GetInputSignatureElementDesc";
        break;
    case ST_Output:
        pFuncName = "ID3DX11EffectShaderVariable::GetOutputSignatureElementDesc";
        break;
    case ST_PatchConstant:
        pFuncName = "ID3DX11EffectShaderVariable::GetPatchConstantSignatureElementDesc";
        break;
    default:
        assert( false );
        return E_FAIL;
    };

    if (nullptr != pReflectionData)
    {
        // get # of signature entries
        assert( pReflectionData->pReflection != 0 );
        _Analysis_assume_( pReflectionData->pReflection != 0 );

        D3D11_SHADER_DESC ShaderDesc;
        VH( pReflectionData->pReflection->GetDesc( &ShaderDesc ) );

        D3D11_SIGNATURE_PARAMETER_DESC ParamDesc ={};
        if( pReflectionData->IsNullGS )
        {
            switch( SigType )
            {
            case ST_Input:
                // The input signature for a null-GS is the output signature of the previous VS
                SigType = ST_Output;
                break;
            case ST_PatchConstant:
                // GeometryShaders cannot have patch constant signatures
                return E_INVALIDARG;
            };
        }

        switch( SigType )
        {
        case ST_Input:
            if( Element >= ShaderDesc.InputParameters )
            {
                DPF( 0, "%s: Invalid Element index (%u) specified", pFuncName, Element );
                VH( E_INVALIDARG );
            }
            VH( pReflectionData->pReflection->GetInputParameterDesc( Element, &ParamDesc ) );
            break;
        case ST_Output:
            if( Element >= ShaderDesc.OutputParameters )
            {
                DPF( 0, "%s: Invalid Element index (%u) specified", pFuncName, Element );
                VH( E_INVALIDARG );
            }
            VH( pReflectionData->pReflection->GetOutputParameterDesc( Element, &ParamDesc ) );
            break;
        case ST_PatchConstant:
            if( Element >= ShaderDesc.PatchConstantParameters )
            {
                DPF( 0, "%s: Invalid Element index (%u) specified", pFuncName, Element );
                VH( E_INVALIDARG );
            }
            VH( pReflectionData->pReflection->GetPatchConstantParameterDesc( Element, &ParamDesc ) );
            break;
        };

        pDesc->SemanticName = ParamDesc.SemanticName;
        pDesc->SystemValueType = ParamDesc.SystemValueType;

        // Pixel shaders need to be special-cased as they don't technically output SVs
        if( pDesc->SystemValueType == D3D_NAME_UNDEFINED && GetShaderType() == EOT_PixelShader && pDesc->SemanticName != 0 )
        {
            if( _stricmp(pDesc->SemanticName, "SV_TARGET") == 0 )
            {
                pDesc->SystemValueType = D3D_NAME_TARGET;
            } 
            else if( _stricmp(pDesc->SemanticName, "SV_DEPTH") == 0 )
            {
                pDesc->SystemValueType = D3D_NAME_DEPTH;
            } 
            else if( _stricmp(pDesc->SemanticName, "SV_COVERAGE") == 0 )
            {
                pDesc->SystemValueType = D3D_NAME_COVERAGE;
            }
        }

        pDesc->SemanticIndex = ParamDesc.SemanticIndex;
        pDesc->Register = ParamDesc.Register;
        pDesc->Mask = ParamDesc.Mask;
        pDesc->ComponentType = ParamDesc.ComponentType;
        pDesc->ReadWriteMask = ParamDesc.ReadWriteMask;
    }
    else
    {
        DPF(0, "%s: Cannot get signatures; shader bytecode is not present", pFuncName);
        VH( D3DERR_INVALIDCALL );
    }
    
lExit:
    return hr;
}

void * GetBlockByIndex(EVarType VarType, EObjectType ObjectType, void *pBaseBlock, uint32_t Index)
{
    switch( VarType )
    {
    case EVT_Interface:
        return (SInterface *)pBaseBlock + Index;
    case EVT_Object:
        switch (ObjectType)
        {
        case EOT_Blend:
            return (SBlendBlock *)pBaseBlock + Index;
        case EOT_DepthStencil:
            return (SDepthStencilBlock *)pBaseBlock + Index;
        case EOT_Rasterizer:
            return (SRasterizerBlock *)pBaseBlock + Index;
        case EOT_PixelShader:
        case EOT_PixelShader5:
        case EOT_GeometryShader:
        case EOT_GeometryShaderSO:
        case EOT_GeometryShader5:
        case EOT_VertexShader:
        case EOT_VertexShader5:
        case EOT_HullShader5:
        case EOT_DomainShader5:
        case EOT_ComputeShader5:
            return (SShaderBlock *)pBaseBlock + Index;
        case EOT_String:
            return (SString *)pBaseBlock + Index;
        case EOT_Sampler:
            return (SSamplerBlock *)pBaseBlock + Index;
        case EOT_Buffer:
        case EOT_Texture:
        case EOT_Texture1D:
        case EOT_Texture1DArray:
        case EOT_Texture2D:
        case EOT_Texture2DArray:
        case EOT_Texture2DMS:
        case EOT_Texture2DMSArray:
        case EOT_Texture3D:
        case EOT_TextureCube:
        case EOT_TextureCubeArray:
        case EOT_ByteAddressBuffer:
        case EOT_StructuredBuffer:
            return (SShaderResource *)pBaseBlock + Index;
        case EOT_DepthStencilView:
            return (SDepthStencilView *)pBaseBlock + Index;
        case EOT_RenderTargetView:
            return (SRenderTargetView *)pBaseBlock + Index;
        case EOT_RWTexture1D:
        case EOT_RWTexture1DArray:
        case EOT_RWTexture2D:
        case EOT_RWTexture2DArray:
        case EOT_RWTexture3D:
        case EOT_RWBuffer:
        case EOT_RWByteAddressBuffer:
        case EOT_RWStructuredBuffer:
        case EOT_RWStructuredBufferAlloc:
        case EOT_RWStructuredBufferConsume:
        case EOT_AppendStructuredBuffer:
        case EOT_ConsumeStructuredBuffer:    
            return (SUnorderedAccessView *)pBaseBlock + Index;
        default:
            assert(0);
            return nullptr;
        }
    default:
        assert(0);
        return nullptr;
    }
}

//--------------------------------------------------------------------------------------
// CEffect
//--------------------------------------------------------------------------------------

CEffect::CEffect( uint32_t Flags ) noexcept :
    m_RefCount(1),
    m_Flags(Flags),
    m_pReflection(nullptr),
    m_VariableCount(0),
    m_pVariables(nullptr),
    m_AnonymousShaderCount(0),
    m_pAnonymousShaders(nullptr),
    m_TechniqueCount(0),
    m_GroupCount(0),
    m_pGroups(nullptr),
    m_pNullGroup(nullptr),
    m_ShaderBlockCount(0),
    m_pShaderBlocks(nullptr),
    m_DepthStencilBlockCount(0),
    m_pDepthStencilBlocks(nullptr),
    m_BlendBlockCount(0),
    m_pBlendBlocks(nullptr),
    m_RasterizerBlockCount(0),
    m_pRasterizerBlocks(nullptr),
    m_SamplerBlockCount(0),
    m_pSamplerBlocks(nullptr),
    m_MemberDataCount(0),
    m_pMemberDataBlocks(nullptr),
    m_InterfaceCount(0),
    m_pInterfaces(nullptr),
    m_CBCount(0),
    m_pCBs(nullptr),
    m_StringCount(0),
    m_pStrings(nullptr),
    m_ShaderResourceCount(0),
    m_pShaderResources(nullptr),
    m_UnorderedAccessViewCount(0),
    m_pUnorderedAccessViews(nullptr),
    m_RenderTargetViewCount(0),
    m_pRenderTargetViews(nullptr),
    m_DepthStencilViewCount(0),
    m_pDepthStencilViews(nullptr),
    m_LocalTimer(1),
    m_FXLIndex(0),
    m_pDevice(nullptr),
    m_pContext(nullptr),
    m_pClassLinkage(nullptr),
    m_pTypePool(nullptr),
    m_pStringPool(nullptr),
    m_pPooledHeap(nullptr),
    m_pOptimizedTypeHeap(nullptr)
{
}

void CEffect::ReleaseShaderRefection()
{
    for( size_t i = 0; i < m_ShaderBlockCount; ++ i )
    {
        SAFE_RELEASE( m_pShaderBlocks[i].pInputSignatureBlob );
        if( m_pShaderBlocks[i].pReflectionData )
        {
            SAFE_RELEASE( m_pShaderBlocks[i].pReflectionData->pReflection );
        }
    }
}

CEffect::~CEffect()
{
    ID3D11InfoQueue *pInfoQueue = nullptr;

    // Mute debug spew
    if (m_pDevice)
    {
        HRESULT hr = m_pDevice->QueryInterface(__uuidof(ID3D11InfoQueue), (void**) &pInfoQueue);
        if ( FAILED(hr) )
            pInfoQueue = nullptr;
    }

    if (pInfoQueue)
    {
        D3D11_INFO_QUEUE_FILTER filter = {};
        D3D11_MESSAGE_CATEGORY messageCategory = D3D11_MESSAGE_CATEGORY_STATE_SETTING;

        filter.DenyList.NumCategories = 1;
        filter.DenyList.pCategoryList = &messageCategory;
        pInfoQueue->PushStorageFilter(&filter);
    }

    if( nullptr != m_pDevice )
    {
        // if m_pDevice == nullptr, then we failed LoadEffect(), which means ReleaseShaderReflection was already called.

        // Release the shader reflection info, as it was not created on the private heap
        // This must be called before we delete m_pReflection
        ReleaseShaderRefection();
    }

    SAFE_DELETE( m_pReflection );
    SAFE_DELETE( m_pTypePool );
    SAFE_DELETE( m_pStringPool );
    SAFE_DELETE( m_pPooledHeap );
    SAFE_DELETE( m_pOptimizedTypeHeap );

    // this code assumes the effect has been loaded & relocated,
    // so check for that before freeing the resources

    if (nullptr != m_pDevice)
    {
        // Keep the following in line with AddRefAllForCloning

        assert(nullptr == m_pRasterizerBlocks || m_Heap.IsInHeap(m_pRasterizerBlocks));
        for (size_t i = 0; i < m_RasterizerBlockCount; ++ i)
        {
            SAFE_RELEASE(m_pRasterizerBlocks[i].pRasterizerObject);
        }

        assert(nullptr == m_pBlendBlocks || m_Heap.IsInHeap(m_pBlendBlocks));
        for (size_t i = 0; i < m_BlendBlockCount; ++ i)
        {
            SAFE_RELEASE(m_pBlendBlocks[i].pBlendObject);
        }

        assert(nullptr == m_pDepthStencilBlocks || m_Heap.IsInHeap(m_pDepthStencilBlocks));
        for (size_t i = 0; i < m_DepthStencilBlockCount; ++ i)
        {
            SAFE_RELEASE(m_pDepthStencilBlocks[i].pDSObject);
        }

        assert(nullptr == m_pSamplerBlocks || m_Heap.IsInHeap(m_pSamplerBlocks));
        for (size_t i = 0; i < m_SamplerBlockCount; ++ i)
        {
            SAFE_RELEASE(m_pSamplerBlocks[i].pD3DObject);
        }

        assert(nullptr == m_pShaderResources || m_Heap.IsInHeap(m_pShaderResources));
        for (size_t i = 0; i < m_ShaderResourceCount; ++ i)
        {
            SAFE_RELEASE(m_pShaderResources[i].pShaderResource);
        }

        assert(nullptr == m_pUnorderedAccessViews || m_Heap.IsInHeap(m_pUnorderedAccessViews));
        for (size_t i = 0; i < m_UnorderedAccessViewCount; ++ i)
        {
            SAFE_RELEASE(m_pUnorderedAccessViews[i].pUnorderedAccessView);
        }

        assert(nullptr == m_pRenderTargetViews || m_Heap.IsInHeap(m_pRenderTargetViews));
        for (size_t i = 0; i < m_RenderTargetViewCount; ++ i)
        {
            SAFE_RELEASE(m_pRenderTargetViews[i].pRenderTargetView);
        }

        assert(nullptr == m_pDepthStencilViews || m_Heap.IsInHeap(m_pDepthStencilViews));
        for (size_t i = 0; i < m_DepthStencilViewCount; ++ i)
        {
            SAFE_RELEASE(m_pDepthStencilViews[i].pDepthStencilView);
        }

        assert(nullptr == m_pMemberDataBlocks || m_Heap.IsInHeap(m_pMemberDataBlocks));
        for (size_t i = 0; i < m_MemberDataCount; ++ i)
        {
            switch( m_pMemberDataBlocks[i].Type )
            {
            case MDT_ClassInstance:
                SAFE_RELEASE(m_pMemberDataBlocks[i].Data.pD3DClassInstance);
                break;
            case MDT_BlendState:
                SAFE_RELEASE(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedBlendState);
                break;
            case MDT_DepthStencilState:
                SAFE_RELEASE(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedDepthStencilState);
                break;
            case MDT_RasterizerState:
                SAFE_RELEASE(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedRasterizerState);
                break;
            case MDT_SamplerState:
                SAFE_RELEASE(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedSamplerState);
                break;
            case MDT_Buffer:
                SAFE_RELEASE(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedConstantBuffer);
                break;
            case MDT_ShaderResourceView:
                SAFE_RELEASE(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedTextureBuffer);
                break;
            default:
                assert( false );
            }
        }

        assert(nullptr == m_pCBs || m_Heap.IsInHeap(m_pCBs));
        for (size_t i = 0; i < m_CBCount; ++ i)
        {
            SAFE_RELEASE(m_pCBs[i].TBuffer.pShaderResource);
            SAFE_RELEASE(m_pCBs[i].pD3DObject);
        }

        assert(nullptr == m_pShaderBlocks || m_Heap.IsInHeap(m_pShaderBlocks));
        _Analysis_assume_( m_ShaderBlockCount == 0 || m_pShaderBlocks != 0 );
        for (size_t i = 0; i < m_ShaderBlockCount; ++ i)
        {
            SAFE_RELEASE(m_pShaderBlocks[i].pD3DObject);
        }

        SAFE_RELEASE( m_pDevice );
    }
    SAFE_RELEASE( m_pClassLinkage );
    assert( m_pContext == nullptr );

    // Restore debug spew
    if (pInfoQueue)
    {
        pInfoQueue->PopStorageFilter();
        SAFE_RELEASE(pInfoQueue);
    }
}

// AddRef all D3D object when cloning
void CEffect::AddRefAllForCloning( _In_ CEffect* pEffectSource )
{
#ifdef NDEBUG
    UNREFERENCED_PARAMETER(pEffectSource);
#endif
    // Keep the following in line with ~CEffect

    assert( m_pDevice != nullptr );

    for( size_t i = 0; i < m_ShaderBlockCount; ++ i )
    {
        SAFE_ADDREF( m_pShaderBlocks[i].pInputSignatureBlob );
        if( m_pShaderBlocks[i].pReflectionData )
        {
            SAFE_ADDREF( m_pShaderBlocks[i].pReflectionData->pReflection );
        }
    }

    assert(nullptr == m_pRasterizerBlocks || pEffectSource->m_Heap.IsInHeap(m_pRasterizerBlocks));
    for ( size_t i = 0; i < m_RasterizerBlockCount; ++ i)
    {
        SAFE_ADDREF(m_pRasterizerBlocks[i].pRasterizerObject);
    }

    assert(nullptr == m_pBlendBlocks || pEffectSource->m_Heap.IsInHeap(m_pBlendBlocks));
    for ( size_t i = 0; i < m_BlendBlockCount; ++ i)
    {
        SAFE_ADDREF(m_pBlendBlocks[i].pBlendObject);
    }

    assert(nullptr == m_pDepthStencilBlocks || pEffectSource->m_Heap.IsInHeap(m_pDepthStencilBlocks));
    for ( size_t i = 0; i < m_DepthStencilBlockCount; ++ i)
    {
        SAFE_ADDREF(m_pDepthStencilBlocks[i].pDSObject);
    }

    assert(nullptr == m_pSamplerBlocks || pEffectSource->m_Heap.IsInHeap(m_pSamplerBlocks));
    for ( size_t i = 0; i < m_SamplerBlockCount; ++ i)
    {
        SAFE_ADDREF(m_pSamplerBlocks[i].pD3DObject);
    }

    assert(nullptr == m_pShaderResources || pEffectSource->m_Heap.IsInHeap(m_pShaderResources));
    for ( size_t i = 0; i < m_ShaderResourceCount; ++ i)
    {
        SAFE_ADDREF(m_pShaderResources[i].pShaderResource);
    }

    assert(nullptr == m_pUnorderedAccessViews || pEffectSource->m_Heap.IsInHeap(m_pUnorderedAccessViews));
    for ( size_t i = 0; i < m_UnorderedAccessViewCount; ++ i)
    {
        SAFE_ADDREF(m_pUnorderedAccessViews[i].pUnorderedAccessView);
    }

    assert(nullptr == m_pRenderTargetViews || pEffectSource->m_Heap.IsInHeap(m_pRenderTargetViews));
    for ( size_t i = 0; i < m_RenderTargetViewCount; ++ i)
    {
        SAFE_ADDREF(m_pRenderTargetViews[i].pRenderTargetView);
    }

    assert(nullptr == m_pDepthStencilViews || pEffectSource->m_Heap.IsInHeap(m_pDepthStencilViews));
    for ( size_t i = 0; i < m_DepthStencilViewCount; ++ i)
    {
        SAFE_ADDREF(m_pDepthStencilViews[i].pDepthStencilView);
    }

    assert(nullptr == m_pMemberDataBlocks || pEffectSource->m_Heap.IsInHeap(m_pMemberDataBlocks));
    for ( size_t i = 0; i < m_MemberDataCount; ++ i)
    {
        switch( m_pMemberDataBlocks[i].Type )
        {
        case MDT_ClassInstance:
            SAFE_ADDREF(m_pMemberDataBlocks[i].Data.pD3DClassInstance);
            break;
        case MDT_BlendState:
            SAFE_ADDREF(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedBlendState);
            break;
        case MDT_DepthStencilState:
            SAFE_ADDREF(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedDepthStencilState);
            break;
        case MDT_RasterizerState:
            SAFE_ADDREF(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedRasterizerState);
            break;
        case MDT_SamplerState:
            SAFE_ADDREF(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedSamplerState);
            break;
        case MDT_Buffer:
            SAFE_ADDREF(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedConstantBuffer);
            break;
        case MDT_ShaderResourceView:
            SAFE_ADDREF(m_pMemberDataBlocks[i].Data.pD3DEffectsManagedTextureBuffer);
            break;
        default:
            assert( false );
        }
    }

    // There's no need to AddRef CBs, since they are recreated
    if (m_pCBs)
    {
        assert(pEffectSource->m_Heap.IsInHeap(m_pCBs));
        for (size_t i = 0; i < m_CBCount; ++i)
        {
            SAFE_ADDREF(m_pCBs[i].TBuffer.pShaderResource);
            SAFE_ADDREF(m_pCBs[i].pD3DObject);
        }
    }

    assert(nullptr == m_pShaderBlocks || pEffectSource->m_Heap.IsInHeap(m_pShaderBlocks));
    for ( size_t i = 0; i < m_ShaderBlockCount; ++ i)
    {
        SAFE_ADDREF(m_pShaderBlocks[i].pD3DObject);
    }

    SAFE_ADDREF( m_pDevice );

    SAFE_ADDREF( m_pClassLinkage );
    assert( m_pContext == nullptr );
}

_Use_decl_annotations_
HRESULT CEffect::QueryInterface(REFIID iid, LPVOID *ppv)
{
    HRESULT hr = S_OK;

    if(nullptr == ppv)
    {
        DPF(0, "ID3DX11Effect::QueryInterface: nullptr parameter");
        hr = E_INVALIDARG;
        goto EXIT;
    }

    *ppv = nullptr;
    if(IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IUnknown *) this;
    }
    else if(IsEqualIID(iid, IID_ID3DX11Effect))
    {
        *ppv = (ID3DX11Effect *) this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();

EXIT:
    return hr;
}

ULONG CEffect::AddRef()
{
    return ++ m_RefCount;
}

ULONG CEffect::Release()
{
    if (-- m_RefCount > 0)
    {
        return m_RefCount;
    }
    else
    {
        delete this;
    }

    return 0;
}

// In all shaders, replace pOldBufferBlock with pNewBuffer, if pOldBufferBlock is a dependency
_Use_decl_annotations_
void CEffect::ReplaceCBReference(SConstantBuffer *pOldBufferBlock, ID3D11Buffer *pNewBuffer)
{
    for (size_t iShaderBlock=0; iShaderBlock<m_ShaderBlockCount; iShaderBlock++)
    {
        for (size_t iCBDep = 0; iCBDep < m_pShaderBlocks[iShaderBlock].CBDepCount; iCBDep++)
        {
            for (size_t iCB = 0; iCB < m_pShaderBlocks[iShaderBlock].pCBDeps[iCBDep].Count; iCB++)
            {
                if (m_pShaderBlocks[iShaderBlock].pCBDeps[iCBDep].ppFXPointers[iCB] == pOldBufferBlock)
                    m_pShaderBlocks[iShaderBlock].pCBDeps[iCBDep].ppD3DObjects[iCB] = pNewBuffer;
            }
        }
    }
}

// In all shaders, replace pOldSamplerBlock with pNewSampler, if pOldSamplerBlock is a dependency
_Use_decl_annotations_
void CEffect::ReplaceSamplerReference(SSamplerBlock *pOldSamplerBlock, ID3D11SamplerState *pNewSampler)
{
    for (size_t iShaderBlock=0; iShaderBlock<m_ShaderBlockCount; iShaderBlock++)
    {
        for (size_t  iSamplerDep = 0; iSamplerDep < m_pShaderBlocks[iShaderBlock].SampDepCount; iSamplerDep++)
        {
            for (size_t  iSampler = 0; iSampler < m_pShaderBlocks[iShaderBlock].pSampDeps[iSamplerDep].Count; iSampler++)
            {
                if (m_pShaderBlocks[iShaderBlock].pSampDeps[iSamplerDep].ppFXPointers[iSampler] == pOldSamplerBlock)
                    m_pShaderBlocks[iShaderBlock].pSampDeps[iSamplerDep].ppD3DObjects[iSampler] = pNewSampler;
            }
        }
    }
}

// Call BindToDevice after the effect has been fully loaded.
// BindToDevice will release all D3D11 objects and create new ones on the new device
_Use_decl_annotations_
HRESULT CEffect::BindToDevice(ID3D11Device *pDevice, LPCSTR srcName)
{
    HRESULT hr = S_OK;

    // Set new device
    if (pDevice == nullptr)
    {
        DPF(0, "ID3DX11Effect: pDevice must point to a valid D3D11 device");
        return D3DERR_INVALIDCALL;
    }

    if (m_pDevice != nullptr)
    {
        DPF(0, "ID3DX11Effect: Internal error, rebinding effects to a new device is not supported");
        return D3DERR_INVALIDCALL;
    }

    bool featureLevelGE11 = ( pDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0 );

    pDevice->AddRef();
    SAFE_RELEASE(m_pDevice);
    m_pDevice = pDevice;
    VH( m_pDevice->CreateClassLinkage( &m_pClassLinkage ) );
    SetDebugObjectName(m_pClassLinkage,srcName);

    // Create all constant buffers
    SConstantBuffer *pCB = m_pCBs;
    SConstantBuffer *pCBLast = m_pCBs + m_CBCount;
    for(; pCB != pCBLast; pCB++)
    {
        SAFE_RELEASE(pCB->pD3DObject);
        SAFE_RELEASE(pCB->TBuffer.pShaderResource);

        // This is a CBuffer
        if (pCB->Size > 0)
        {
            if (pCB->IsTBuffer)
            {
                D3D11_BUFFER_DESC bufDesc;
                // size is always register aligned
                bufDesc.ByteWidth = pCB->Size;
                bufDesc.Usage = D3D11_USAGE_DEFAULT;
                bufDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                bufDesc.CPUAccessFlags = 0;
                bufDesc.MiscFlags = 0;

                VH( pDevice->CreateBuffer( &bufDesc, nullptr, &pCB->pD3DObject) );
                SetDebugObjectName(pCB->pD3DObject, srcName );
                
                D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
                viewDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                viewDesc.Buffer.ElementOffset = 0;
                viewDesc.Buffer.ElementWidth = pCB->Size / SType::c_RegisterSize;

                VH( pDevice->CreateShaderResourceView( pCB->pD3DObject, &viewDesc, &pCB->TBuffer.pShaderResource) );
                SetDebugObjectName(pCB->TBuffer.pShaderResource, srcName );
            }
            else
            {
                D3D11_BUFFER_DESC bufDesc;
                // size is always register aligned
                bufDesc.ByteWidth = pCB->Size;
                bufDesc.Usage = D3D11_USAGE_DEFAULT;
                bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                bufDesc.CPUAccessFlags = 0;
                bufDesc.MiscFlags = 0;

                VH( pDevice->CreateBuffer( &bufDesc, nullptr, &pCB->pD3DObject) );
                SetDebugObjectName( pCB->pD3DObject, srcName );
                pCB->TBuffer.pShaderResource = nullptr;
            }

            pCB->IsDirty = true;
        }
        else
        {
            pCB->IsDirty = false;
        }
    }

    // Create all RasterizerStates
    SRasterizerBlock *pRB = m_pRasterizerBlocks;
    SRasterizerBlock *pRBLast = m_pRasterizerBlocks + m_RasterizerBlockCount;
    for(; pRB != pRBLast; pRB++)
    {
        SAFE_RELEASE(pRB->pRasterizerObject);
        if( SUCCEEDED( m_pDevice->CreateRasterizerState( &pRB->BackingStore, &pRB->pRasterizerObject) ) )
        {
            pRB->IsValid = true;
            SetDebugObjectName( pRB->pRasterizerObject, srcName );
        }
        else
            pRB->IsValid = false;
    }

    // Create all DepthStencils
    SDepthStencilBlock *pDS = m_pDepthStencilBlocks;
    SDepthStencilBlock *pDSLast = m_pDepthStencilBlocks + m_DepthStencilBlockCount;
    for(; pDS != pDSLast; pDS++)
    {
        SAFE_RELEASE(pDS->pDSObject);
        if( SUCCEEDED( m_pDevice->CreateDepthStencilState( &pDS->BackingStore, &pDS->pDSObject) ) )
        {
            pDS->IsValid = true;
            SetDebugObjectName( pDS->pDSObject, srcName );
        }
        else
            pDS->IsValid = false;
    }

    // Create all BlendStates
    SBlendBlock *pBlend = m_pBlendBlocks;
    SBlendBlock *pBlendLast = m_pBlendBlocks + m_BlendBlockCount;
    for(; pBlend != pBlendLast; pBlend++)
    {
        SAFE_RELEASE(pBlend->pBlendObject);
        if( SUCCEEDED( m_pDevice->CreateBlendState( &pBlend->BackingStore, &pBlend->pBlendObject ) ) )
        {
            pBlend->IsValid = true;
            SetDebugObjectName( pBlend->pBlendObject, srcName );
        }
        else
            pBlend->IsValid = false;
    }

    // Create all Samplers
    SSamplerBlock *pSampler = m_pSamplerBlocks;
    SSamplerBlock *pSamplerLast = m_pSamplerBlocks + m_SamplerBlockCount;
    for(; pSampler != pSamplerLast; pSampler++)
    {
        SAFE_RELEASE(pSampler->pD3DObject);

        VH( m_pDevice->CreateSamplerState( &pSampler->BackingStore.SamplerDesc, &pSampler->pD3DObject) );
        SetDebugObjectName( pSampler->pD3DObject, srcName );
    }

    // Create all shaders
    ID3D11ClassLinkage* neededClassLinkage = featureLevelGE11 ? m_pClassLinkage : nullptr;
    SShaderBlock *pShader = m_pShaderBlocks;
    SShaderBlock *pShaderLast = m_pShaderBlocks + m_ShaderBlockCount;
    for(; pShader != pShaderLast; pShader++)
    {
        SAFE_RELEASE(pShader->pD3DObject);

        if (nullptr == pShader->pReflectionData)
        {
            // nullptr shader. It's one of these:
            // PixelShader ps;
            // or
            // SetPixelShader( nullptr );
            continue;
        }
        
        if (pShader->pReflectionData->pStreamOutDecls[0] || pShader->pReflectionData->pStreamOutDecls[1] || 
            pShader->pReflectionData->pStreamOutDecls[2] || pShader->pReflectionData->pStreamOutDecls[3] )
        {
            // This is a geometry shader, process it's data
            CSOParser soParser;
            VH( soParser.Parse(pShader->pReflectionData->pStreamOutDecls) );
            uint32_t strides[4];
            soParser.GetStrides( strides );
            hr = m_pDevice->CreateGeometryShaderWithStreamOutput(pShader->pReflectionData->pBytecode,
                                                                pShader->pReflectionData->BytecodeLength,
                                                                soParser.GetDeclArray(),
                                                                soParser.GetDeclCount(),
                                                                strides,
                                                                featureLevelGE11 ? 4 : 1,
                                                                pShader->pReflectionData->RasterizedStream,
                                                                neededClassLinkage,
                                                                reinterpret_cast<ID3D11GeometryShader**>(&pShader->pD3DObject) );
            if (FAILED(hr))
            {
                DPF(1, "ID3DX11Effect::Load - failed to create GeometryShader with StreamOutput decl: \"%s\"", soParser.GetErrorString() );
                pShader->IsValid = false;
                hr = S_OK;
            }
            else
            {
                SetDebugObjectName( pShader->pD3DObject, srcName );
            }
        }
        else
        {
            // This is a regular shader
            if( pShader->pReflectionData->RasterizedStream == D3D11_SO_NO_RASTERIZED_STREAM )
                pShader->IsValid = false;
            else 
            {
                if( FAILED( (m_pDevice->*(pShader->pVT->pCreateShader))( (uint32_t *) pShader->pReflectionData->pBytecode, pShader->pReflectionData->BytecodeLength, neededClassLinkage, &pShader->pD3DObject) ) )
                {
                    DPF(1, "ID3DX11Effect::Load - failed to create shader" );
                    pShader->IsValid = false;
                }
                else
                {
                    SetDebugObjectName( pShader->pD3DObject, srcName );
                }
            }
        }

        // Update all dependency pointers
        VH( pShader->OnDeviceBind() );
    }

    // Initialize the member data pointers for all variables
    uint32_t CurMemberData = 0;
    for (uint32_t i = 0; i < m_VariableCount; ++ i)
    {
        if( m_pVariables[i].pMemberData )
        {
            if( m_pVariables[i].pType->IsClassInstance() )
            {
                for (uint32_t j = 0; j < std::max<size_t>(m_pVariables[i].pType->Elements,1); ++j)
                {
                    assert( CurMemberData < m_MemberDataCount );
                    ID3D11ClassInstance** ppCI = &(m_pVariables[i].pMemberData + j)->Data.pD3DClassInstance;
                    (m_pVariables[i].pMemberData + j)->Type = MDT_ClassInstance;
                    (m_pVariables[i].pMemberData + j)->Data.pD3DClassInstance = nullptr;
                    if( m_pVariables[i].pType->TotalSize > 0 )
                    {
                        // ignore failures in GetClassInstance;
                        m_pClassLinkage->GetClassInstance( m_pVariables[i].pName, j, ppCI );
                    }
                    else
                    {
                        // The HLSL compiler optimizes out zero-sized classes, so we have to create class instances from scratch
                        if( FAILED( m_pClassLinkage->CreateClassInstance( m_pVariables[i].pType->pTypeName, 0, 0, 0, 0, ppCI ) ) )
                        {
                            DPF(0, "ID3DX11Effect: Out of memory while trying to create new class instance interface");
                        }
                        else
                        {
                            SetDebugObjectName( *ppCI, srcName );
                        }
                    }
                    CurMemberData++;
                }
            }
            else if( m_pVariables[i].pType->IsStateBlockObject() )
            {
                for (size_t j = 0; j < std::max<size_t>(m_pVariables[i].pType->Elements,1); ++j)
                {
                    switch( m_pVariables[i].pType->ObjectType )
                    {
                    case EOT_Blend:
                        (m_pVariables[i].pMemberData + j)->Type = MDT_BlendState;
                        (m_pVariables[i].pMemberData + j)->Data.pD3DEffectsManagedBlendState = nullptr;
                        break;
                    case EOT_Rasterizer:
                        (m_pVariables[i].pMemberData + j)->Type = MDT_RasterizerState;
                        (m_pVariables[i].pMemberData + j)->Data.pD3DEffectsManagedRasterizerState = nullptr;
                        break;
                    case EOT_DepthStencil:
                        (m_pVariables[i].pMemberData + j)->Type = MDT_DepthStencilState;
                        (m_pVariables[i].pMemberData + j)->Data.pD3DEffectsManagedDepthStencilState = nullptr;
                        break;
                    case EOT_Sampler:
                        (m_pVariables[i].pMemberData + j)->Type = MDT_SamplerState;
                        (m_pVariables[i].pMemberData + j)->Data.pD3DEffectsManagedSamplerState = nullptr;
                        break;
                    default:
                        VB( false );
                    }
                    CurMemberData++;
                }
            }
            else
            {
                VB( false );
            }
        }
    }
    for(pCB = m_pCBs; pCB != pCBLast; pCB++)
    {
        (pCB->pMemberData + 0)->Type = MDT_Buffer;
        (pCB->pMemberData + 0)->Data.pD3DEffectsManagedConstantBuffer = nullptr;
        CurMemberData++;
        (pCB->pMemberData + 1)->Type = MDT_ShaderResourceView;
        (pCB->pMemberData + 1)->Data.pD3DEffectsManagedTextureBuffer = nullptr;
        CurMemberData++;
    }


    // Determine which techniques and passes are known to be invalid
    for( size_t iGroup=0; iGroup < m_GroupCount; iGroup++ )
    {
        SGroup* pGroup = &m_pGroups[iGroup];
        pGroup->InitiallyValid = true;

        for( size_t iTech=0; iTech < pGroup->TechniqueCount; iTech++ )
        {
            STechnique* pTechnique = &pGroup->pTechniques[iTech];
            pTechnique->InitiallyValid = true;
           
            for( size_t iPass = 0; iPass < pTechnique->PassCount; iPass++ )
            {
                SPassBlock* pPass = &pTechnique->pPasses[iPass];
                pPass->InitiallyValid = true;

                if( pPass->BackingStore.pBlendBlock != nullptr && !pPass->BackingStore.pBlendBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pDepthStencilBlock != nullptr && !pPass->BackingStore.pDepthStencilBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pRasterizerBlock != nullptr && !pPass->BackingStore.pRasterizerBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pVertexShaderBlock != nullptr && !pPass->BackingStore.pVertexShaderBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pPixelShaderBlock != nullptr && !pPass->BackingStore.pPixelShaderBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pGeometryShaderBlock != nullptr && !pPass->BackingStore.pGeometryShaderBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pHullShaderBlock != nullptr && !pPass->BackingStore.pHullShaderBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pDomainShaderBlock != nullptr && !pPass->BackingStore.pDomainShaderBlock->IsValid )
                    pPass->InitiallyValid = false;
                if( pPass->BackingStore.pComputeShaderBlock != nullptr && !pPass->BackingStore.pComputeShaderBlock->IsValid )
                    pPass->InitiallyValid = false;

                pTechnique->InitiallyValid &= pPass->InitiallyValid;
            }
            pGroup->InitiallyValid &= pTechnique->InitiallyValid;
        }
    }

lExit:
    return hr;
}

// FindVariableByName, plus an understanding of literal indices
// This code handles A[i].
// It does not handle anything else, like A.B, A[B[i]], A[B]
SVariable * CEffect::FindVariableByNameWithParsing(_In_z_ LPCSTR pName)
{
    SGlobalVariable *pVariable;
    const uint32_t MAX_PARSABLE_NAME_LENGTH = 256;
    char pScratchString[MAX_PARSABLE_NAME_LENGTH];

    const char* pSource = pName;
    char* pDest = pScratchString;
    char* pEnd = pScratchString + MAX_PARSABLE_NAME_LENGTH;

    pVariable = nullptr;

    while( *pSource != 0 )
    {
        if( pDest == pEnd )
        {
            pVariable = FindLocalVariableByName(pName);
            if( pVariable == nullptr )
            {
                DPF( 0, "Name %s is too long to parse", pName );
            }
            return pVariable;
        }

        if( *pSource == '[' )
        {
            // parse previous variable name
            *pDest = 0;
            assert( pVariable == nullptr );
            pVariable = FindLocalVariableByName(pScratchString);
            if( pVariable == nullptr )
            {
                return nullptr;
            }
            pDest = pScratchString;
        }
        else if( *pSource == ']' )
        {
            // parse integer
            *pDest = 0;
            uint32_t index = atoi(pScratchString);
            assert( pVariable != 0 );
            _Analysis_assume_( pVariable != 0 );
            pVariable = (SGlobalVariable*)pVariable->GetElement(index);
            if( pVariable && !pVariable->IsValid() )
            {
                pVariable = nullptr;
            }
            return pVariable;
        }
        else
        {
            // add character
            *pDest = *pSource;
            pDest++;
        }
        pSource++;
    }

    if( pDest != pScratchString )
    {
        // parse the variable name (there was no [i])
        *pDest = 0;
        assert( pVariable == nullptr );
        pVariable = FindLocalVariableByName(pScratchString);
    }

    return pVariable;
}

SGlobalVariable * CEffect::FindVariableByName(_In_z_ LPCSTR pName)
{
    SGlobalVariable *pVariable;

    pVariable = FindLocalVariableByName(pName);

    return pVariable;
}

SGlobalVariable * CEffect::FindLocalVariableByName(_In_z_ LPCSTR pName)
{
    SGlobalVariable *pVariable, *pVariableEnd;

    pVariableEnd = m_pVariables + m_VariableCount;
    for (pVariable = m_pVariables; pVariable != pVariableEnd; pVariable++)
    {
        if (strcmp( pVariable->pName, pName) == 0)
        {
            return pVariable;
        }
    }

    return nullptr;
}


//
// Checks to see if two types are equivalent (either at runtime
// or during the type-pooling load process)
//
// Major assumption: if both types are structures, then their
// member types & names should already have been added to the pool,
// in which case their member type & name pointers should be equal.
//
// This is true because complex data types (structures) have all
// sub-types translated before the containing type is translated,
// which means that simple sub-types (numeric types) have already
// been pooled.
//
bool SType::IsEqual(SType *pOtherType) const
{
    if (VarType != pOtherType->VarType || Elements != pOtherType->Elements
        || strcmp(pTypeName, pOtherType->pTypeName) != 0)
    {
        return false;
    }

    switch (VarType)
    {
    case EVT_Struct:
        {
            if (StructType.Members != pOtherType->StructType.Members)
            {
                return false;
            }
            assert(StructType.pMembers != nullptr && pOtherType->StructType.pMembers != nullptr);

            uint32_t  i;
            for (i = 0; i < StructType.Members; ++ i)
            {
                // names for types must exist (not true for semantics)
                assert(StructType.pMembers[i].pName != nullptr && pOtherType->StructType.pMembers[i].pName != nullptr);

                if (StructType.pMembers[i].pType != pOtherType->StructType.pMembers[i].pType ||
                    StructType.pMembers[i].Data.Offset != pOtherType->StructType.pMembers[i].Data.Offset ||
                    StructType.pMembers[i].pName != pOtherType->StructType.pMembers[i].pName ||
                    StructType.pMembers[i].pSemantic != pOtherType->StructType.pMembers[i].pSemantic)
                {
                    return false;
                }
            }
        }
        break;

    case EVT_Object:
        {
            if (ObjectType != pOtherType->ObjectType)
            {
                return false;
            }
        }
        break;

    case EVT_Numeric:
        {
            if (NumericType.Rows != pOtherType->NumericType.Rows ||
                NumericType.Columns != pOtherType->NumericType.Columns ||
                NumericType.ScalarType != pOtherType->NumericType.ScalarType ||
                NumericType.NumericLayout != pOtherType->NumericType.NumericLayout ||
                NumericType.IsColumnMajor != pOtherType->NumericType.IsColumnMajor ||
                NumericType.IsPackedArray != pOtherType->NumericType.IsPackedArray)
            {
                return false;
            }
        }
        break;

    case EVT_Interface:
        {
            // VarType and pTypeName handled above
        }
        break;

    default:
        {
            assert(0);
            return false;
        }
        break;
    }

    assert(TotalSize == pOtherType->TotalSize && Stride == pOtherType->Stride && PackedSize == pOtherType->PackedSize);

    return true;
}

uint32_t SType::GetTotalUnpackedSize(_In_ bool IsSingleElement) const
{
    if (VarType == EVT_Object)
    {
        return 0;
    }
    else if (VarType == EVT_Interface)
    {
        return 0;
    }
    else if (Elements > 0 && IsSingleElement)
    {
        assert( ( TotalSize == 0 && Stride == 0 ) ||
                    ( (TotalSize > (Stride * (Elements - 1))) && (TotalSize <= (Stride * Elements)) ) );
        return TotalSize - Stride * (Elements - 1);
    }
    else
    {
        return TotalSize;
    }
}

uint32_t SType::GetTotalPackedSize(_In_ bool IsSingleElement) const
{
    if (Elements > 0 && IsSingleElement)
    {
        assert(PackedSize % Elements == 0);
        return PackedSize / Elements;
    }
    else
    {
        return PackedSize;
    }
}

SConstantBuffer *CEffect::FindCB(_In_z_ LPCSTR pName)
{
    uint32_t  i;

    for (i=0; i<m_CBCount; i++)
    {
        if (!strcmp(m_pCBs[i].pName, pName))
        {
            return &m_pCBs[i];
        }
    }

    return nullptr;
}

bool CEffect::IsOptimized()
{
    if ((m_Flags & D3DX11_EFFECT_OPTIMIZED) != 0)
    {
        assert(nullptr == m_pReflection);
        return true;
    }
    else
    {
        assert(nullptr != m_pReflection);
        return false;
    }
}

// Replace *ppType with the corresponding value in pMappingTable
// pMappingTable table describes how to map old type pointers to new type pointers
static HRESULT RemapType(_Inout_ SType **ppType, _Inout_ CPointerMappingTable *pMappingTable)
{
    HRESULT hr = S_OK;

    SPointerMapping ptrMapping;
    CPointerMappingTable::CIterator iter;
    ptrMapping.pOld = *ppType;
    VH( pMappingTable->FindValueWithHash(ptrMapping, ptrMapping.Hash(), &iter) );
    *ppType = (SType *) iter.GetData().pNew;

lExit:
    return hr;
}

// Replace *ppString with the corresponding value in pMappingTable
// pMappingTable table describes how to map old string pointers to new string pointers
static HRESULT RemapString(_In_ char **ppString, _Inout_ CPointerMappingTable *pMappingTable)
{
    HRESULT hr = S_OK;

    SPointerMapping ptrMapping;
    CPointerMappingTable::CIterator iter;
    ptrMapping.pOld = *ppString;
    VH( pMappingTable->FindValueWithHash(ptrMapping, ptrMapping.Hash(), &iter) );
    *ppString = (char *) iter.GetData().pNew;

lExit:
    return hr;
}

// Used in cloning, copy m_pMemberInterfaces from pEffectSource to this
HRESULT CEffect::CopyMemberInterfaces( _In_ CEffect* pEffectSource )
{
    HRESULT hr = S_OK;

    uint32_t Members = pEffectSource->m_pMemberInterfaces.GetSize();
    m_pMemberInterfaces.AddRange(Members);
    uint32_t i=0; // after a failure, this holds the failing index
    for(; i < Members; i++ )
    {
        SMember* pOldMember = pEffectSource->m_pMemberInterfaces[i];
        if( pOldMember == nullptr )
        {
            // During Optimization, m_pMemberInterfaces[i] was set to nullptr because it was an annotation
            m_pMemberInterfaces[i] = nullptr;
            continue;
        }

        SMember *pNewMember;
        assert( pOldMember->pTopLevelEntity != nullptr );

        if (nullptr == (pNewMember = CreateNewMember((SType*)pOldMember->pType, false)))
        {
            DPF(0, "ID3DX11Effect: Out of memory while trying to create new member variable interface");
            VN( pNewMember );
        }

        pNewMember->pType = pOldMember->pType;
        pNewMember->pName = pOldMember->pName;
        pNewMember->pSemantic = pOldMember->pSemantic;
        pNewMember->Data.pGeneric = pOldMember->Data.pGeneric;
        pNewMember->IsSingleElement = pOldMember->IsSingleElement;
        pNewMember->pTopLevelEntity = pOldMember->pTopLevelEntity;
        pNewMember->pMemberData = pOldMember->pMemberData;

        m_pMemberInterfaces[i] = pNewMember;
    }

lExit:
    if( FAILED(hr) )
    {
        assert( i < Members );
        ZeroMemory( &m_pMemberInterfaces[i], sizeof(SMember) * ( Members - i ) );
    }
    return hr;
}

// Used in cloning, copy the string pool from pEffectSource to this and build mappingTable
// for use in RemapString
_Use_decl_annotations_
HRESULT CEffect::CopyStringPool( CEffect* pEffectSource, CPointerMappingTable& mappingTable )
{
    HRESULT hr = S_OK;
    assert( m_pPooledHeap != 0 );
    _Analysis_assume_( m_pPooledHeap != 0 );
    VN( m_pStringPool = new CEffect::CStringHashTable );
    m_pStringPool->SetPrivateHeap(m_pPooledHeap);
    VH( m_pStringPool->AutoGrow() );

    CStringHashTable::CIterator stringIter;

    // move strings over, build mapping table
    for (pEffectSource->m_pStringPool->GetFirstEntry(&stringIter); !pEffectSource->m_pStringPool->PastEnd(&stringIter); pEffectSource->m_pStringPool->GetNextEntry(&stringIter))
    {
        SPointerMapping ptrMapping;
        char *pString;

        const char* pOldString = stringIter.GetData();
        ptrMapping.pOld = (void*)pOldString;
        uint32_t len = (uint32_t)strlen(pOldString);
        uint32_t hash = ptrMapping.Hash();
        VN( pString = new(*m_pPooledHeap) char[len + 1] );
        ptrMapping.pNew = (void*)pString;
        memcpy(ptrMapping.pNew, ptrMapping.pOld, len + 1);
        VH( m_pStringPool->AddValueWithHash(pString, hash) );

        VH( mappingTable.AddValueWithHash(ptrMapping, hash) );
    }

    // Uncomment to print string mapping
    /*
    CPointerMappingTable::CIterator mapIter;
    for (mappingTable.GetFirstEntry(&mapIter); !mappingTable.PastEnd(&mapIter); mappingTable.GetNextEntry(&mapIter))
    {
    SPointerMapping ptrMapping = mapIter.GetData();
    DPF(0, "string: 0x%x : 0x%x  %s", (UINT_PTR)ptrMapping.pOld, (UINT_PTR)ptrMapping.pNew, (char*)ptrMapping.pNew );
    }*/

lExit:
    return hr;
}

// Used in cloning, copy the unoptimized type pool from pEffectSource to this and build mappingTableTypes
// for use in RemapType.  mappingTableStrings is the mapping table previously filled when copying strings.
_Use_decl_annotations_
HRESULT CEffect::CopyTypePool( CEffect* pEffectSource, CPointerMappingTable& mappingTableTypes, CPointerMappingTable& mappingTableStrings )
{
    HRESULT hr = S_OK;
    assert( m_pPooledHeap != 0 );
    _Analysis_assume_( m_pPooledHeap != 0 );
    VN( m_pTypePool = new CEffect::CTypeHashTable );
    m_pTypePool->SetPrivateHeap(m_pPooledHeap);
    VH( m_pTypePool->AutoGrow() );

    CTypeHashTable::CIterator typeIter;
    CPointerMappingTable::CIterator mapIter;

    // first pass: move types over, build mapping table
    for (pEffectSource->m_pTypePool->GetFirstEntry(&typeIter); !pEffectSource->m_pTypePool->PastEnd(&typeIter); pEffectSource->m_pTypePool->GetNextEntry(&typeIter))
    {
        SPointerMapping ptrMapping;
        SType *pType;

        ptrMapping.pOld = typeIter.GetData();
        uint32_t hash = ptrMapping.Hash();
        VN( (ptrMapping.pNew) = new(*m_pPooledHeap) SType );
        memcpy(ptrMapping.pNew, ptrMapping.pOld, sizeof(SType));

        pType = (SType *) ptrMapping.pNew;

        // if this is a struct, move its members to the newly allocated space
        if (EVT_Struct == pType->VarType)
        {
            SVariable* pOldMembers = pType->StructType.pMembers;
            VN( pType->StructType.pMembers = new(*m_pPooledHeap) SVariable[pType->StructType.Members] );
            memcpy(pType->StructType.pMembers, pOldMembers, pType->StructType.Members * sizeof(SVariable));
        }

        VH( m_pTypePool->AddValueWithHash(pType, hash) );
        VH( mappingTableTypes.AddValueWithHash(ptrMapping, hash) );
    }

    // second pass: fixup structure member & name pointers
    for (mappingTableTypes.GetFirstEntry(&mapIter); !mappingTableTypes.PastEnd(&mapIter); mappingTableTypes.GetNextEntry(&mapIter))
    {
        SPointerMapping ptrMapping = mapIter.GetData();

        // Uncomment to print type mapping
        //DPF(0, "type: 0x%x : 0x%x", (UINT_PTR)ptrMapping.pOld, (UINT_PTR)ptrMapping.pNew );

        SType *pType = (SType *) ptrMapping.pNew;

        if( pType->pTypeName )
        {
            VH( RemapString(&pType->pTypeName, &mappingTableStrings) );
        }

        // if this is a struct, fix up its members' pointers
        if (EVT_Struct == pType->VarType)
        {
            for (uint32_t i = 0; i < pType->StructType.Members; ++ i)
            {
                VH( RemapType((SType**)&pType->StructType.pMembers[i].pType, &mappingTableTypes) );
                if( pType->StructType.pMembers[i].pName )
                {
                    VH( RemapString(&pType->StructType.pMembers[i].pName, &mappingTableStrings) );
                }
                if( pType->StructType.pMembers[i].pSemantic )
                {
                    VH( RemapString(&pType->StructType.pMembers[i].pSemantic, &mappingTableStrings) );
                }
            }
        }
    } 

lExit:
    return hr;
}

// Used in cloning, copy the unoptimized type pool from pEffectSource to this and build mappingTableTypes
// for use in RemapType.  mappingTableStrings is the mapping table previously filled when copying strings.
_Use_decl_annotations_
HRESULT CEffect::CopyOptimizedTypePool( CEffect* pEffectSource, CPointerMappingTable& mappingTableTypes )
{
    HRESULT hr = S_OK;
    CEffectHeap* pOptimizedTypeHeap = nullptr;

    assert( pEffectSource->m_pOptimizedTypeHeap != 0 );
    _Analysis_assume_( pEffectSource->m_pOptimizedTypeHeap != 0 );
    assert( m_pTypePool == 0 );
    assert( m_pStringPool == 0 );
    assert( m_pPooledHeap == 0 );

    VN( pOptimizedTypeHeap = new CEffectHeap );
    VH( pOptimizedTypeHeap->ReserveMemory( pEffectSource->m_pOptimizedTypeHeap->GetSize() ) );
    CPointerMappingTable::CIterator mapIter;

    // first pass: move types over, build mapping table
    uint8_t* pReadTypes = pEffectSource->m_pOptimizedTypeHeap->GetDataStart();
    while( pEffectSource->m_pOptimizedTypeHeap->IsInHeap( pReadTypes ) )
    {
        SPointerMapping ptrMapping;
        SType *pType;
        uint32_t moveSize;

        ptrMapping.pOld = ptrMapping.pNew = pReadTypes;
        moveSize = sizeof(SType);
        VH( pOptimizedTypeHeap->MoveData(&ptrMapping.pNew, moveSize) );
        pReadTypes += moveSize;

        pType = (SType *) ptrMapping.pNew;

        // if this is a struct, move its members to the newly allocated space
        if (EVT_Struct == pType->VarType)
        {
            moveSize = pType->StructType.Members * sizeof(SVariable);
            VH( pOptimizedTypeHeap->MoveData((void **)&pType->StructType.pMembers, moveSize) );
            pReadTypes += moveSize;
        }

        VH( mappingTableTypes.AddValueWithHash(ptrMapping, ptrMapping.Hash()) );
    }

    // second pass: fixup structure member & name pointers
    for (mappingTableTypes.GetFirstEntry(&mapIter); !mappingTableTypes.PastEnd(&mapIter); mappingTableTypes.GetNextEntry(&mapIter))
    {
        SPointerMapping ptrMapping = mapIter.GetData();

        // Uncomment to print type mapping
        //DPF(0, "type: 0x%x : 0x%x", (UINT_PTR)ptrMapping.pOld, (UINT_PTR)ptrMapping.pNew );

        SType *pType = (SType *) ptrMapping.pNew;

        // if this is a struct, fix up its members' pointers
        if (EVT_Struct == pType->VarType)
        {
            for (uint32_t i = 0; i < pType->StructType.Members; ++ i)
            {
                VH( RemapType((SType**)&pType->StructType.pMembers[i].pType, &mappingTableTypes) );
            }
        }
    }  

lExit:
    return hr;
}

// Used in cloning, create new ID3D11ConstantBuffers for each non-single CB
HRESULT CEffect::RecreateCBs()
{
    HRESULT hr = S_OK;
    uint32_t i; // after a failure, this holds the failing index

    for (i = 0; i < m_CBCount; ++ i)
    {
        SConstantBuffer* pCB = &m_pCBs[i];

        pCB->IsNonUpdatable = pCB->IsUserManaged || pCB->ClonedSingle();

        if( pCB->Size > 0 && !pCB->ClonedSingle() )
        {
            ID3D11Buffer** ppOriginalBuffer;
            ID3D11ShaderResourceView** ppOriginalTBufferView;

            if( pCB->IsUserManaged )
            {
                ppOriginalBuffer = &pCB->pMemberData[0].Data.pD3DEffectsManagedConstantBuffer;
                ppOriginalTBufferView = &pCB->pMemberData[1].Data.pD3DEffectsManagedTextureBuffer;
            }
            else
            {
                ppOriginalBuffer = &pCB->pD3DObject;
                ppOriginalTBufferView = &pCB->TBuffer.pShaderResource;
            }

            VN( *ppOriginalBuffer );
            D3D11_BUFFER_DESC bufDesc;
            (*ppOriginalBuffer)->GetDesc( &bufDesc );
            ID3D11Buffer* pNewBuffer = nullptr;
            VH( m_pDevice->CreateBuffer( &bufDesc, nullptr, &pNewBuffer ) );
            SetDebugObjectName( pNewBuffer, "D3DX11Effect" );
            (*ppOriginalBuffer)->Release();
            (*ppOriginalBuffer) = pNewBuffer;
            pNewBuffer = nullptr;

            if( pCB->IsTBuffer )
            {
                VN( *ppOriginalTBufferView );
                D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
                (*ppOriginalTBufferView)->GetDesc( &viewDesc );
                ID3D11ShaderResourceView* pNewView = nullptr;
                VH( m_pDevice->CreateShaderResourceView( (*ppOriginalBuffer), &viewDesc, &pNewView) );
                SetDebugObjectName( pNewView, "D3DX11Effect" );
                (*ppOriginalTBufferView)->Release();
                (*ppOriginalTBufferView) = pNewView;
                pNewView = nullptr;
            }
            else
            {
                assert( *ppOriginalTBufferView == nullptr );
                ReplaceCBReference( pCB, (*ppOriginalBuffer) );
            }

            pCB->IsDirty = true;
        }
    }

lExit:
    return hr;
}

// Move Name and Semantic strings using mappingTableStrings
_Use_decl_annotations_
HRESULT CEffect::FixupMemberInterface( SMember* pMember, CEffect* pEffectSource, CPointerMappingTable& mappingTableStrings )
{
    HRESULT hr = S_OK;

    if( pMember->pName )
    {
        if( pEffectSource->m_pReflection && pEffectSource->m_pReflection->m_Heap.IsInHeap(pMember->pName) )
        {
            pMember->pName = (char*)((UINT_PTR)pMember->pName - (UINT_PTR)pEffectSource->m_pReflection->m_Heap.GetDataStart() + (UINT_PTR)m_pReflection->m_Heap.GetDataStart());
        }
        else
        {
            VH( RemapString(&pMember->pName, &mappingTableStrings) );
        }
    }
    if( pMember->pSemantic )
    {
        if( pEffectSource->m_pReflection && pEffectSource->m_pReflection->m_Heap.IsInHeap(pMember->pSemantic) )
        {
            pMember->pSemantic = (char*)((UINT_PTR)pMember->pSemantic - (UINT_PTR)pEffectSource->m_pReflection->m_Heap.GetDataStart() + (UINT_PTR)m_pReflection->m_Heap.GetDataStart());
        }
        else
        {
            VH( RemapString(&pMember->pSemantic, &mappingTableStrings) );
        }
    }

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// Public API to create a copy of this effect
HRESULT CEffect::CloneEffect(_In_ uint32_t Flags, _Outptr_ ID3DX11Effect** ppClonedEffect )
{
    HRESULT hr = S_OK;
    CPointerMappingTable mappingTableTypes;
    CPointerMappingTable mappingTableStrings;

    CEffectLoader loader;
    CEffect* pNewEffect = nullptr;    
    CDataBlockStore* pTempHeap = nullptr;


    VN( pNewEffect = new CEffect( m_Flags ) );
    if( Flags & D3DX11_EFFECT_CLONE_FORCE_NONSINGLE )
    {
        // The effect is cloned as if there was no original, so don't mark it as cloned
        pNewEffect->m_Flags &= ~(uint32_t)D3DX11_EFFECT_CLONE;
    }
    else
    {
        pNewEffect->m_Flags |= D3DX11_EFFECT_CLONE;
    }

    pNewEffect->m_VariableCount = m_VariableCount;
    pNewEffect->m_pVariables = m_pVariables;
    pNewEffect->m_AnonymousShaderCount = m_AnonymousShaderCount;
    pNewEffect->m_pAnonymousShaders = m_pAnonymousShaders;
    pNewEffect->m_TechniqueCount = m_TechniqueCount;
    pNewEffect->m_GroupCount = m_GroupCount;
    pNewEffect->m_pGroups = m_pGroups;
    pNewEffect->m_pNullGroup = m_pNullGroup;
    pNewEffect->m_ShaderBlockCount = m_ShaderBlockCount;
    pNewEffect->m_pShaderBlocks = m_pShaderBlocks;
    pNewEffect->m_DepthStencilBlockCount = m_DepthStencilBlockCount;
    pNewEffect->m_pDepthStencilBlocks = m_pDepthStencilBlocks;
    pNewEffect->m_BlendBlockCount = m_BlendBlockCount;
    pNewEffect->m_pBlendBlocks = m_pBlendBlocks;
    pNewEffect->m_RasterizerBlockCount = m_RasterizerBlockCount;
    pNewEffect->m_pRasterizerBlocks = m_pRasterizerBlocks;
    pNewEffect->m_SamplerBlockCount = m_SamplerBlockCount;
    pNewEffect->m_pSamplerBlocks = m_pSamplerBlocks;
    pNewEffect->m_MemberDataCount = m_MemberDataCount;
    pNewEffect->m_pMemberDataBlocks = m_pMemberDataBlocks;
    pNewEffect->m_InterfaceCount = m_InterfaceCount;
    pNewEffect->m_pInterfaces = m_pInterfaces;
    pNewEffect->m_CBCount = m_CBCount;
    pNewEffect->m_pCBs = m_pCBs;
    pNewEffect->m_StringCount = m_StringCount;
    pNewEffect->m_pStrings = m_pStrings;
    pNewEffect->m_ShaderResourceCount = m_ShaderResourceCount;
    pNewEffect->m_pShaderResources = m_pShaderResources;
    pNewEffect->m_UnorderedAccessViewCount = m_UnorderedAccessViewCount;
    pNewEffect->m_pUnorderedAccessViews = m_pUnorderedAccessViews;
    pNewEffect->m_RenderTargetViewCount = m_RenderTargetViewCount;
    pNewEffect->m_pRenderTargetViews = m_pRenderTargetViews;
    pNewEffect->m_DepthStencilViewCount = m_DepthStencilViewCount;
    pNewEffect->m_pDepthStencilViews = m_pDepthStencilViews; 
    pNewEffect->m_LocalTimer = m_LocalTimer;
    pNewEffect->m_FXLIndex = m_FXLIndex;
    pNewEffect->m_pDevice = m_pDevice;
    pNewEffect->m_pClassLinkage = m_pClassLinkage;

    pNewEffect->AddRefAllForCloning( this );


    // m_pMemberInterfaces is a vector of cbuffer members that were created when the user called GetMemberBy* or GetElement
    // or during Effect loading when an interface is initialized to a global class variable elment.
    VH( pNewEffect->CopyMemberInterfaces( this ) );

    loader.m_pvOldMemberInterfaces = &m_pMemberInterfaces;
    loader.m_pEffect = pNewEffect;
    loader.m_EffectMemory = loader.m_ReflectionMemory = 0;


    // Move data from current effect to new effect
    if( !IsOptimized() )
    {
        VN( pNewEffect->m_pReflection = new CEffectReflection() );
        loader.m_pReflection = pNewEffect->m_pReflection;

        // make sure strings are moved before ReallocateEffectData
        VH( loader.InitializeReflectionDataAndMoveStrings( m_pReflection->m_Heap.GetSize() ) );
    }
    VH( loader.ReallocateEffectData( true ) );
    if( !IsOptimized() )
    {
        VH( loader.ReallocateReflectionData( true ) );
    }


    // Data structures for remapping type pointers and string pointers
    VN( pTempHeap = new CDataBlockStore );
    pTempHeap->EnableAlignment();
    mappingTableTypes.SetPrivateHeap(pTempHeap);
    mappingTableStrings.SetPrivateHeap(pTempHeap);
    VH( mappingTableTypes.AutoGrow() );
    VH( mappingTableStrings.AutoGrow() );

    if( !IsOptimized() )
    {
        // Let's re-create the type pool and string pool
        VN( pNewEffect->m_pPooledHeap = new CDataBlockStore );
        pNewEffect->m_pPooledHeap->EnableAlignment();

        VH( pNewEffect->CopyStringPool( this, mappingTableStrings ) );
        VH( pNewEffect->CopyTypePool( this, mappingTableTypes, mappingTableStrings ) );
    }
    else
    {
        // There's no string pool after optimizing.  Let's re-create the type pool
        VH( pNewEffect->CopyOptimizedTypePool( this, mappingTableTypes ) );
    }

    // fixup this effect's variable's types
    VH( pNewEffect->OptimizeTypes(&mappingTableTypes, true) );
    VH( pNewEffect->RecreateCBs() );


    for (uint32_t i = 0; i < pNewEffect->m_pMemberInterfaces.GetSize(); ++ i)
    {
        SMember* pMember = pNewEffect->m_pMemberInterfaces[i];
        VH( pNewEffect->FixupMemberInterface( pMember, this, mappingTableStrings ) );
    }


lExit:
    SAFE_DELETE( pTempHeap );
    if( FAILED( hr ) )
    {
        SAFE_DELETE( pNewEffect );
    }
    *ppClonedEffect = pNewEffect;
    return hr;
}

// Move all type pointers using pMappingTable.
// This is called after creating the optimized type pool or during cloning.
HRESULT CEffect::OptimizeTypes(_Inout_ CPointerMappingTable *pMappingTable, _In_ bool Cloning)
{
    HRESULT hr = S_OK;

    // find all child types, point them to the new location
    for (size_t i = 0; i < m_VariableCount; ++ i)
    {
        VH( RemapType((SType**)&m_pVariables[i].pType, pMappingTable) );
    }

    uint32_t Members = m_pMemberInterfaces.GetSize();
    for( size_t i=0; i < Members; i++ )
    {
        if( m_pMemberInterfaces[i] != nullptr )
        {
            VH( RemapType((SType**)&m_pMemberInterfaces[i]->pType, pMappingTable) );
        }
    }

    // when cloning, there may be annotations
    if( Cloning )
    {
        for (size_t iVar = 0; iVar < m_VariableCount; ++ iVar)
        {
            for(size_t i = 0; i < m_pVariables[iVar].AnnotationCount; ++ i )
            {
                VH( RemapType((SType**)&m_pVariables[iVar].pAnnotations[i].pType, pMappingTable) );
            }
        }
        for (size_t iCB = 0; iCB < m_CBCount; ++ iCB)
        {
            for(size_t i = 0; i < m_pCBs[iCB].AnnotationCount; ++ i )
            {
                VH( RemapType((SType**)&m_pCBs[iCB].pAnnotations[i].pType, pMappingTable) );
            }
        }
        for (size_t iGroup = 0; iGroup < m_GroupCount; ++ iGroup)
        {
            for(size_t i = 0; i < m_pGroups[iGroup].AnnotationCount; ++ i )
            {
                VH( RemapType((SType**)&m_pGroups[iGroup].pAnnotations[i].pType, pMappingTable) );
            }
            for(size_t iTech = 0; iTech < m_pGroups[iGroup].TechniqueCount; ++ iTech )
            {
                for(size_t i = 0; i < m_pGroups[iGroup].pTechniques[iTech].AnnotationCount; ++ i )
                {
                    VH( RemapType((SType**)&m_pGroups[iGroup].pTechniques[iTech].pAnnotations[i].pType, pMappingTable) );
                }
                for(size_t iPass = 0; iPass < m_pGroups[iGroup].pTechniques[iTech].PassCount; ++ iPass )
                {
                    for(size_t i = 0; i < m_pGroups[iGroup].pTechniques[iTech].pPasses[iPass].AnnotationCount; ++ i )
                    {
                        VH( RemapType((SType**)&m_pGroups[iGroup].pTechniques[iTech].pPasses[iPass].pAnnotations[i].pType, pMappingTable) );
                    }
                }
            }
        }
    }
lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// Public API to shed this effect of its reflection data

HRESULT CEffect::Optimize()
{
    HRESULT hr = S_OK;
    CEffectHeap *pOptimizedTypeHeap = nullptr;
    
    if (IsOptimized())
    {
        DPF(0, "ID3DX11Effect::Optimize: Effect has already been Optimize()'ed");
        return S_OK;
    }

    // Delete annotations, names, semantics, and string data on variables
    
    for (size_t i = 0; i < m_VariableCount; ++ i)
    {
        m_pVariables[i].AnnotationCount = 0;
        m_pVariables[i].pAnnotations = nullptr;
        m_pVariables[i].pName = nullptr;
        m_pVariables[i].pSemantic = nullptr;

        // 2) Point string variables to nullptr
        if (m_pVariables[i].pType->IsObjectType(EOT_String))
        {
            assert(nullptr != m_pVariables[i].Data.pString);
            m_pVariables[i].Data.pString = nullptr;
        }
    }

    // Delete annotations and names on CBs

    for (size_t i = 0; i < m_CBCount; ++ i)
    {
        m_pCBs[i].AnnotationCount = 0;
        m_pCBs[i].pAnnotations = nullptr;
        m_pCBs[i].pName = nullptr;
        m_pCBs[i].IsEffectOptimized = true;
    }

    // Delete annotations and names on techniques and passes

    for (size_t i = 0; i < m_GroupCount; ++ i)
    {
        m_pGroups[i].AnnotationCount = 0;
        m_pGroups[i].pAnnotations = nullptr;
        m_pGroups[i].pName = nullptr;

        for (size_t j = 0; j < m_pGroups[i].TechniqueCount; ++ j)
        {
            m_pGroups[i].pTechniques[j].AnnotationCount = 0;
            m_pGroups[i].pTechniques[j].pAnnotations = nullptr;
            m_pGroups[i].pTechniques[j].pName = nullptr;

            for (size_t k = 0; k < m_pGroups[i].pTechniques[j].PassCount; ++ k)
            {
                m_pGroups[i].pTechniques[j].pPasses[k].AnnotationCount = 0;
                m_pGroups[i].pTechniques[j].pPasses[k].pAnnotations = nullptr;
                m_pGroups[i].pTechniques[j].pPasses[k].pName = nullptr;
            }
        }
    };

    // 2) Remove shader bytecode & stream out decls
    //    (all are contained within pReflectionData)

    for (size_t i = 0; i < m_ShaderBlockCount; ++ i)
    {
        if( m_pShaderBlocks[i].pReflectionData )
        {
            // pReflection was not created with PRIVATENEW
            SAFE_RELEASE( m_pShaderBlocks[i].pReflectionData->pReflection );

            m_pShaderBlocks[i].pReflectionData = nullptr;
        }
    }

    uint32_t Members = m_pMemberInterfaces.GetSize();
    for( size_t i=0; i < Members; i++ )
    {
        assert( m_pMemberInterfaces[i] != nullptr );
        if( IsReflectionData(m_pMemberInterfaces[i]->pTopLevelEntity) )
        {
            assert( IsReflectionData(m_pMemberInterfaces[i]->Data.pGeneric) );

            // This is checked when cloning (so we don't clone Optimized-out member variables)
            m_pMemberInterfaces[i] = nullptr;
        }
        else
        {
            m_pMemberInterfaces[i]->pName = nullptr;
            m_pMemberInterfaces[i]->pSemantic = nullptr;
        }
    }



    // get rid of the name/type hash tables and string data, 
    // then reallocate the type data and fix up this effect
    CPointerMappingTable mappingTable;
    CTypeHashTable::CIterator typeIter;
    CPointerMappingTable::CIterator mapIter;
    CCheckedDword chkSpaceNeeded = 0;
    uint32_t  spaceNeeded;

    // first pass: compute needed space
    for (m_pTypePool->GetFirstEntry(&typeIter); !m_pTypePool->PastEnd(&typeIter); m_pTypePool->GetNextEntry(&typeIter))
    {
        SType *pType = typeIter.GetData();
        
        chkSpaceNeeded += AlignToPowerOf2(sizeof(SType), c_DataAlignment);

        // if this is a struct, allocate room for its members
        if (EVT_Struct == pType->VarType)
        {
            chkSpaceNeeded += AlignToPowerOf2(pType->StructType.Members * sizeof(SVariable), c_DataAlignment);
        }
    }

    VH( chkSpaceNeeded.GetValue(&spaceNeeded) );

    assert(nullptr == m_pOptimizedTypeHeap);
    VN( pOptimizedTypeHeap = new CEffectHeap );
    VH( pOptimizedTypeHeap->ReserveMemory(spaceNeeded));

    // use the private heap that we're about to destroy as scratch space for the mapping table
    mappingTable.SetPrivateHeap(m_pPooledHeap);
    VH( mappingTable.AutoGrow() );

    // second pass: move types over, build mapping table
    for (m_pTypePool->GetFirstEntry(&typeIter); !m_pTypePool->PastEnd(&typeIter); m_pTypePool->GetNextEntry(&typeIter))
    {
        SPointerMapping ptrMapping;
        SType *pType;

        ptrMapping.pOld = ptrMapping.pNew = typeIter.GetData();
        VH( pOptimizedTypeHeap->MoveData(&ptrMapping.pNew, sizeof(SType)) );

        pType = (SType *) ptrMapping.pNew;

        // if this is a struct, move its members to the newly allocated space
        if (EVT_Struct == pType->VarType)
        {
            VH( pOptimizedTypeHeap->MoveData((void **)&pType->StructType.pMembers, pType->StructType.Members * sizeof(SVariable)) );
        }

        VH( mappingTable.AddValueWithHash(ptrMapping, ptrMapping.Hash()) );
    }
    
    // third pass: fixup structure member & name pointers
    for (mappingTable.GetFirstEntry(&mapIter); !mappingTable.PastEnd(&mapIter); mappingTable.GetNextEntry(&mapIter))
    {
        SPointerMapping ptrMapping = mapIter.GetData();
        SType *pType = (SType *) ptrMapping.pNew;

        pType->pTypeName = nullptr;

        // if this is a struct, fix up its members' pointers
        if (EVT_Struct == pType->VarType)
        {
            for (size_t i = 0; i < pType->StructType.Members; ++ i)
            {
                VH( RemapType((SType**)&pType->StructType.pMembers[i].pType, &mappingTable) );
                pType->StructType.pMembers[i].pName = nullptr;
                pType->StructType.pMembers[i].pSemantic = nullptr;
            }
        }
    }        

    // fixup this effect's variable's types
    VH( OptimizeTypes(&mappingTable) );

    m_pOptimizedTypeHeap = pOptimizedTypeHeap;
    pOptimizedTypeHeap = nullptr;

#ifdef D3DX11_FX_PRINT_HASH_STATS
    DPF(0, "Compiler string pool hash table statistics:");
    m_pTypePool->PrintHashTableStats();
    DPF(0, "Compiler type pool hash table statistics:");
    m_pStringPool->PrintHashTableStats();
#endif // D3DX11_FX_PRINT_HASH_STATS

    SAFE_DELETE(m_pTypePool);
    SAFE_DELETE(m_pStringPool);
    SAFE_DELETE(m_pPooledHeap);

    DPF(0, "ID3DX11Effect::Optimize: %u bytes of reflection data freed.", m_pReflection->m_Heap.GetSize());
    SAFE_DELETE(m_pReflection);
    m_Flags |= D3DX11_EFFECT_OPTIMIZED;

lExit:
    SAFE_DELETE(pOptimizedTypeHeap);
    return hr;
}

SMember * CreateNewMember(_In_ SType *pType, _In_ bool IsAnnotation)
{
    switch (pType->VarType)
    {
    case EVT_Struct:
        if (IsAnnotation)
        {
            assert(sizeof(SNumericAnnotationMember) == sizeof(SMember));
            return (SMember*) new SNumericAnnotationMember;
        }
        else if (pType->StructType.ImplementsInterface)
        {
            assert(sizeof(SClassInstanceGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SClassInstanceGlobalVariableMember;
        }
        else
        {
            assert(sizeof(SNumericGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SNumericGlobalVariableMember;
        }
        break;
    case EVT_Interface:
        assert(sizeof(SInterfaceGlobalVariableMember) == sizeof(SMember));
        return (SMember*) new SInterfaceGlobalVariableMember;
        break;
    case EVT_Object:
        switch (pType->ObjectType)
        {
        case EOT_String:
            if (IsAnnotation)
            {
                assert(sizeof(SStringAnnotationMember) == sizeof(SMember));
                return (SMember*) new SStringAnnotationMember;
            }
            else
            {
                assert(sizeof(SStringGlobalVariableMember) == sizeof(SMember));
                return (SMember*) new SStringGlobalVariableMember;
            }

            break;
        case EOT_Texture:
        case EOT_Texture1D:
        case EOT_Texture1DArray:
        case EOT_Texture2D:
        case EOT_Texture2DArray:
        case EOT_Texture2DMS:
        case EOT_Texture2DMSArray:
        case EOT_Texture3D:
        case EOT_TextureCube:
        case EOT_TextureCubeArray:
        case EOT_Buffer:
        case EOT_ByteAddressBuffer:
        case EOT_StructuredBuffer:
            assert(!IsAnnotation);
            assert(sizeof(SShaderResourceGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SShaderResourceGlobalVariableMember;
            break;
        case EOT_RWTexture1D:
        case EOT_RWTexture1DArray:
        case EOT_RWTexture2D:
        case EOT_RWTexture2DArray:
        case EOT_RWTexture3D:
        case EOT_RWBuffer:
        case EOT_RWByteAddressBuffer:
        case EOT_RWStructuredBuffer:
        case EOT_RWStructuredBufferAlloc:
        case EOT_RWStructuredBufferConsume:
        case EOT_AppendStructuredBuffer:
        case EOT_ConsumeStructuredBuffer:
            assert(!IsAnnotation);
            assert(sizeof(SUnorderedAccessViewGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SUnorderedAccessViewGlobalVariableMember;
            break;
        case EOT_VertexShader:
        case EOT_VertexShader5:
        case EOT_GeometryShader:
        case EOT_GeometryShaderSO:
        case EOT_GeometryShader5:
        case EOT_PixelShader:
        case EOT_PixelShader5:
        case EOT_HullShader5:
        case EOT_DomainShader5:
        case EOT_ComputeShader5:
            assert(!IsAnnotation);
            assert(sizeof(SShaderGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SShaderGlobalVariableMember;
            break;
        case EOT_Blend:
            assert(!IsAnnotation);
            assert(sizeof(SBlendGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SBlendGlobalVariableMember;
            break;
        case EOT_Rasterizer:
            assert(!IsAnnotation);
            assert(sizeof(SRasterizerGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SRasterizerGlobalVariableMember;
            break;
        case EOT_DepthStencil:
            assert(!IsAnnotation);
            assert(sizeof(SDepthStencilGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SDepthStencilGlobalVariableMember;
            break;
        case EOT_Sampler:
            assert(!IsAnnotation);
            assert(sizeof(SSamplerGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SSamplerGlobalVariableMember;
            break;
        case EOT_DepthStencilView:
            assert(!IsAnnotation);
            assert(sizeof(SDepthStencilViewGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SDepthStencilViewGlobalVariableMember;
            break;
        case EOT_RenderTargetView:
            assert(!IsAnnotation);
            assert(sizeof(SRenderTargetViewGlobalVariableMember) == sizeof(SMember));
            return (SMember*) new SRenderTargetViewGlobalVariableMember;
            break;
        default:
            assert(0);
            DPF( 0, "Internal error: invalid object type." );
            return nullptr;
            break;
        }
        break;
    case EVT_Numeric:
        switch (pType->NumericType.NumericLayout)
        {
        case ENL_Matrix:
            if (IsAnnotation)
            {
                assert(sizeof(SMatrixAnnotationMember) == sizeof(SMember));
                return (SMember*) new SMatrixAnnotationMember;
            }
            else
            {
                assert(sizeof(SMatrixGlobalVariableMember) == sizeof(SMember));
                assert(sizeof(SMatrix4x4ColumnMajorGlobalVariableMember) == sizeof(SMember));
                assert(sizeof(SMatrix4x4RowMajorGlobalVariableMember) == sizeof(SMember));

                if (pType->NumericType.Rows == 4 && pType->NumericType.Columns == 4)
                {
                    if (pType->NumericType.IsColumnMajor)
                    {
                        return (SMember*) new SMatrix4x4ColumnMajorGlobalVariableMember;
                    }
                    else
                    {
                        return (SMember*) new SMatrix4x4RowMajorGlobalVariableMember;
                    }
                }
                else
                {
                    return (SMember*) new SMatrixGlobalVariableMember;
                }
            }
            break;
        case ENL_Vector:
            switch (pType->NumericType.ScalarType)
            {
            case EST_Float:
                if (IsAnnotation)
                {
                    assert(sizeof(SFloatVectorAnnotationMember) == sizeof(SMember));
                    return (SMember*) new SFloatVectorAnnotationMember;
                }
                else
                {
                    assert(sizeof(SFloatVectorGlobalVariableMember) == sizeof(SMember));
                    assert(sizeof(SFloatVector4GlobalVariableMember) == sizeof(SMember));

                    if (pType->NumericType.Columns == 4)
                    {
                        return (SMember*) new SFloatVector4GlobalVariableMember;
                    }
                    else
                    {
                        return (SMember*) new SFloatVectorGlobalVariableMember;
                    }
                }
                break;
            case EST_Bool:
                if (IsAnnotation)
                {
                    assert(sizeof(SBoolVectorAnnotationMember) == sizeof(SMember));
                    return (SMember*) new SBoolVectorAnnotationMember;
                }
                else
                {
                    assert(sizeof(SBoolVectorGlobalVariableMember) == sizeof(SMember));
                    return (SMember*) new SBoolVectorGlobalVariableMember;
                }
                break;
            case EST_UInt:
            case EST_Int:
                if (IsAnnotation)
                {
                    assert(sizeof(SIntVectorAnnotationMember) == sizeof(SMember));
                    return (SMember*) new SIntVectorAnnotationMember;
                }
                else
                {
                    assert(sizeof(SIntVectorGlobalVariableMember) == sizeof(SMember));
                    return (SMember*) new SIntVectorGlobalVariableMember;
                }
                break;
            default:
                assert(0);
                DPF( 0, "Internal loading error: invalid vector type." );
                break;
            }
            break;
        case ENL_Scalar:
            switch (pType->NumericType.ScalarType)
            {
            case EST_Float:
                if (IsAnnotation)
                {
                    assert(sizeof(SFloatScalarAnnotationMember) == sizeof(SMember));
                    return (SMember*) new SFloatScalarAnnotationMember;
                }
                else
                {
                    assert(sizeof(SFloatScalarGlobalVariableMember) == sizeof(SMember));
                    return (SMember*) new SFloatScalarGlobalVariableMember;
                }
                break;
            case EST_UInt:
            case EST_Int:
                if (IsAnnotation)
                {
                    assert(sizeof(SIntScalarAnnotationMember) == sizeof(SMember));
                    return (SMember*) new SIntScalarAnnotationMember;
                }
                else
                {
                    assert(sizeof(SIntScalarGlobalVariableMember) == sizeof(SMember));
                    return (SMember*) new SIntScalarGlobalVariableMember;
                }
                break;
            case EST_Bool:
                if (IsAnnotation)
                {
                    assert(sizeof(SBoolScalarAnnotationMember) == sizeof(SMember));
                    return (SMember*) new SBoolScalarAnnotationMember;
                }
                else
                {
                    assert(sizeof(SBoolScalarGlobalVariableMember) == sizeof(SMember));
                    return (SMember*) new SBoolScalarGlobalVariableMember;
                }
                break;
            default:
                DPF( 0, "Internal loading error: invalid scalar type." );
                assert(0);
                break;
            }            
            break;
        default:
            assert(0);
            DPF( 0, "Internal loading error: invalid numeric type." );
            break;
        }
        break;
    default:
        assert(0);
        DPF( 0, "Internal loading error: invalid variable type." );
        break;
    }
    return nullptr;
}

// Global variables are created in place because storage for them was allocated during LoadEffect
HRESULT PlacementNewVariable(_In_ void *pVar, _In_ SType *pType, _In_ bool IsAnnotation)
{
    switch (pType->VarType)
    {
    case EVT_Struct:
        if (IsAnnotation)
        {
            assert(sizeof(SNumericAnnotation) == sizeof(SAnnotation));
            new(pVar) SNumericAnnotation();
        }
        else if (pType->StructType.ImplementsInterface)
        {
            assert(sizeof(SClassInstanceGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SClassInstanceGlobalVariable;
        }
        else 
        {
            assert(sizeof(SNumericGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SNumericGlobalVariable;
        }
        break;
    case EVT_Interface:
        assert(sizeof(SInterfaceGlobalVariable) == sizeof(SGlobalVariable));
        new(pVar) SInterfaceGlobalVariable;
        break;
    case EVT_Object:
        switch (pType->ObjectType)
        {
        case EOT_String:
            if (IsAnnotation)
            {
                assert(sizeof(SStringAnnotation) == sizeof(SAnnotation));
                new(pVar) SStringAnnotation;
            }
            else
            {
                assert(sizeof(SStringGlobalVariable) == sizeof(SGlobalVariable));
                new(pVar) SStringGlobalVariable;
            }
            
            break;
        case EOT_Texture:
        case EOT_Texture1D:
        case EOT_Texture1DArray:
        case EOT_Texture2D:
        case EOT_Texture2DArray:
        case EOT_Texture2DMS:
        case EOT_Texture2DMSArray:
        case EOT_Texture3D:
        case EOT_TextureCube:
        case EOT_TextureCubeArray:
        case EOT_Buffer:
        case EOT_ByteAddressBuffer:
        case EOT_StructuredBuffer:
            assert(!IsAnnotation);
            assert(sizeof(SShaderResourceGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SShaderResourceGlobalVariable;
            break;
        case EOT_RWTexture1D:
        case EOT_RWTexture1DArray:
        case EOT_RWTexture2D:
        case EOT_RWTexture2DArray:
        case EOT_RWTexture3D:
        case EOT_RWBuffer:
        case EOT_RWByteAddressBuffer:
        case EOT_RWStructuredBuffer:
        case EOT_RWStructuredBufferAlloc:
        case EOT_RWStructuredBufferConsume:
        case EOT_AppendStructuredBuffer:
        case EOT_ConsumeStructuredBuffer:
            assert(!IsAnnotation);
            assert(sizeof(SUnorderedAccessViewGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SUnorderedAccessViewGlobalVariable;
            break;
        case EOT_VertexShader:
        case EOT_VertexShader5:
        case EOT_GeometryShader:
        case EOT_GeometryShaderSO:
        case EOT_GeometryShader5:
        case EOT_PixelShader:
        case EOT_PixelShader5:
        case EOT_HullShader5:
        case EOT_DomainShader5:
        case EOT_ComputeShader5:
            assert(!IsAnnotation);
            assert(sizeof(SShaderGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SShaderGlobalVariable;
            break;
        case EOT_Blend:
            assert(!IsAnnotation);
            assert(sizeof(SBlendGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SBlendGlobalVariable;
            break;
        case EOT_Rasterizer:
            assert(!IsAnnotation);
            assert(sizeof(SRasterizerGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SRasterizerGlobalVariable;
            break;
        case EOT_DepthStencil:
            assert(!IsAnnotation);
            assert(sizeof(SDepthStencilGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SDepthStencilGlobalVariable;
            break;
        case EOT_Sampler:
            assert(!IsAnnotation);
            assert(sizeof(SSamplerGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SSamplerGlobalVariable;
            break;
        case EOT_RenderTargetView:
            assert(!IsAnnotation);
            assert(sizeof(SRenderTargetViewGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SRenderTargetViewGlobalVariable;
            break;
        case EOT_DepthStencilView:
            assert(!IsAnnotation);
            assert(sizeof(SDepthStencilViewGlobalVariable) == sizeof(SGlobalVariable));
            new(pVar) SDepthStencilViewGlobalVariable;
            break;
        default:
            assert(0);
            DPF( 0, "Internal loading error: invalid object type." );
            return E_FAIL;
            break;
        }
        break;
    case EVT_Numeric:
        switch (pType->NumericType.NumericLayout)
        {
        case ENL_Matrix:
            if (IsAnnotation)
            {
                assert(sizeof(SMatrixAnnotation) == sizeof(SAnnotation));
                new(pVar) SMatrixAnnotation;
            }
            else
            {
                assert(sizeof(SMatrixGlobalVariable) == sizeof(SGlobalVariable));
                assert(sizeof(SMatrix4x4ColumnMajorGlobalVariable) == sizeof(SGlobalVariable));
                assert(sizeof(SMatrix4x4RowMajorGlobalVariable) == sizeof(SGlobalVariable));
                
                if (pType->NumericType.Rows == 4 && pType->NumericType.Columns == 4)
                {
                    if (pType->NumericType.IsColumnMajor)
                    {
                        new(pVar) SMatrix4x4ColumnMajorGlobalVariable;
                    }
                    else
                    {
                        new(pVar) SMatrix4x4RowMajorGlobalVariable;
                    }
                }
                else
                {
                    new(pVar) SMatrixGlobalVariable;
                }
            }
            break;
        case ENL_Vector:
            switch (pType->NumericType.ScalarType)
            {
            case EST_Float:
                if (IsAnnotation)
                {
                    assert(sizeof(SFloatVectorAnnotation) == sizeof(SAnnotation));
                    new(pVar) SFloatVectorAnnotation;
                }
                else
                {
                    assert(sizeof(SFloatVectorGlobalVariable) == sizeof(SGlobalVariable));
                    assert(sizeof(SFloatVector4GlobalVariable) == sizeof(SGlobalVariable));

                    if (pType->NumericType.Columns == 4)
                    {
                        new(pVar) SFloatVector4GlobalVariable;
                    }
                    else
                    {
                        new(pVar) SFloatVectorGlobalVariable;
                    }
                }
                break;
            case EST_Bool:
                if (IsAnnotation)
                {
                    assert(sizeof(SBoolVectorAnnotation) == sizeof(SAnnotation));
                    new(pVar) SBoolVectorAnnotation;
                }
                else
                {
                    assert(sizeof(SBoolVectorGlobalVariable) == sizeof(SGlobalVariable));
                    new(pVar) SBoolVectorGlobalVariable;
                }
                break;
            case EST_UInt:
            case EST_Int:
                if (IsAnnotation)
                {
                    assert(sizeof(SIntVectorAnnotation) == sizeof(SAnnotation));
                    new(pVar) SIntVectorAnnotation;
                }
                else
                {
                    assert(sizeof(SIntVectorGlobalVariable) == sizeof(SGlobalVariable));
                    new(pVar) SIntVectorGlobalVariable;
                }
                break;
            }
            break;
        case ENL_Scalar:
            switch (pType->NumericType.ScalarType)
            {
            case EST_Float:
                if (IsAnnotation)
                {
                    assert(sizeof(SFloatScalarAnnotation) == sizeof(SAnnotation));
                    new(pVar) SFloatScalarAnnotation;
                }
                else
                {
                    assert(sizeof(SFloatScalarGlobalVariable) == sizeof(SGlobalVariable));
                    new(pVar) SFloatScalarGlobalVariable;
                }
                break;
            case EST_UInt:
            case EST_Int:
                if (IsAnnotation)
                {
                    assert(sizeof(SIntScalarAnnotation) == sizeof(SAnnotation));
                    new(pVar) SIntScalarAnnotation;
                }
                else
                {
                    assert(sizeof(SIntScalarGlobalVariable) == sizeof(SGlobalVariable));
                    new(pVar) SIntScalarGlobalVariable;
                }
                break;
            case EST_Bool:
                if (IsAnnotation)
                {
                    assert(sizeof(SBoolScalarAnnotation) == sizeof(SAnnotation));
                    new(pVar) SBoolScalarAnnotation;
                }
                else
                {
                    assert(sizeof(SBoolScalarGlobalVariable) == sizeof(SGlobalVariable));
                    new(pVar) SBoolScalarGlobalVariable;
                }
                break;
            default:
                assert(0);
                DPF( 0, "Internal loading error: invalid scalar type." );
                return E_FAIL;
                break;
            }            
            break;
        default:
            assert(0);
            DPF( 0, "Internal loading error: invalid numeric type." );
            return E_FAIL;
            break;
        }
        break;
    default:
        assert(0);
        DPF( 0, "Internal loading error: invalid variable type." );
        return E_FAIL;
        break;
    }
    return S_OK;
}

}

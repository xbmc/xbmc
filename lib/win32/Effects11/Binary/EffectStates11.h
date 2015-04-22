//--------------------------------------------------------------------------------------
// File: EffectStates11.h
//
// Direct3D 11 Effects States Header
// This file defines properties of states which can appear in
// state blocks and pass blocks.
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

#include "EffectStateBase11.h"

namespace D3DX11Effects
{

//////////////////////////////////////////////////////////////////////////
// Effect HLSL late resolve lists (state values)
//////////////////////////////////////////////////////////////////////////

static const RValue g_rvNULL[] =
{
    { "nullptr",  0 },
    RVALUE_END()
};


static const RValue g_rvBOOL[] =
{
    { "false",  0 },
    { "true",   1 },
    RVALUE_END()
};

static const RValue g_rvDEPTH_WRITE_MASK[] =
{
    { "ZERO",   D3D11_DEPTH_WRITE_MASK_ZERO     },
    { "ALL",    D3D11_DEPTH_WRITE_MASK_ALL      },
    RVALUE_END()
};

static const RValue g_rvFILL[] =
{
    { "WIREFRAME",  D3D11_FILL_WIREFRAME },
    { "SOLID",      D3D11_FILL_SOLID     },
    RVALUE_END()
};

static const RValue g_rvFILTER[] =
{
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_MAG_MIP_POINT                           ),
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_MAG_POINT_MIP_LINEAR                    ),
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_POINT_MAG_LINEAR_MIP_POINT              ),
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_POINT_MAG_MIP_LINEAR                    ),
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_LINEAR_MAG_MIP_POINT                    ),
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_LINEAR_MAG_POINT_MIP_LINEAR             ),
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_MAG_LINEAR_MIP_POINT                    ),
    RVALUE_ENTRY(D3D11_FILTER_,     MIN_MAG_MIP_LINEAR                          ),
    RVALUE_ENTRY(D3D11_FILTER_,     ANISOTROPIC                                 ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_MAG_MIP_POINT                ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_MAG_POINT_MIP_LINEAR         ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT   ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_POINT_MAG_MIP_LINEAR         ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_LINEAR_MAG_MIP_POINT         ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR  ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_MAG_LINEAR_MIP_POINT         ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_MIN_MAG_MIP_LINEAR               ),
    RVALUE_ENTRY(D3D11_FILTER_,     COMPARISON_ANISOTROPIC                      ),
    RVALUE_END()
};

static const RValue g_rvBLEND[] =
{
    { "ZERO",               D3D11_BLEND_ZERO             },
    { "ONE",                D3D11_BLEND_ONE              },
    { "SRC_COLOR",          D3D11_BLEND_SRC_COLOR        },
    { "INV_SRC_COLOR",      D3D11_BLEND_INV_SRC_COLOR    },
    { "SRC_ALPHA",          D3D11_BLEND_SRC_ALPHA        },
    { "INV_SRC_ALPHA",      D3D11_BLEND_INV_SRC_ALPHA    },
    { "DEST_ALPHA",         D3D11_BLEND_DEST_ALPHA       },
    { "INV_DEST_ALPHA",     D3D11_BLEND_INV_DEST_ALPHA   },
    { "DEST_COLOR",         D3D11_BLEND_DEST_COLOR       },
    { "INV_DEST_COLOR",     D3D11_BLEND_INV_DEST_COLOR   },
    { "SRC_ALPHA_SAT",      D3D11_BLEND_SRC_ALPHA_SAT    },
    { "BLEND_FACTOR",       D3D11_BLEND_BLEND_FACTOR     },
    { "INV_BLEND_FACTOR",   D3D11_BLEND_INV_BLEND_FACTOR },
    { "SRC1_COLOR",         D3D11_BLEND_SRC1_COLOR       },
    { "INV_SRC1_COLOR",     D3D11_BLEND_INV_SRC1_COLOR   },
    { "SRC1_ALPHA",         D3D11_BLEND_SRC1_ALPHA       },
    { "INV_SRC1_ALPHA",     D3D11_BLEND_INV_SRC1_ALPHA   },

    RVALUE_END()
};

static const RValue g_rvTADDRESS[] =
{
    { "CLAMP",          D3D11_TEXTURE_ADDRESS_CLAMP      },
    { "WRAP",           D3D11_TEXTURE_ADDRESS_WRAP       },
    { "MIRROR",         D3D11_TEXTURE_ADDRESS_MIRROR     },
    { "BORDER",         D3D11_TEXTURE_ADDRESS_BORDER     },
    { "MIRROR_ONCE",    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE },
    RVALUE_END()
};

static const RValue g_rvCULL[] =
{
    { "NONE",           D3D11_CULL_NONE     },
    { "FRONT",          D3D11_CULL_FRONT    },
    { "BACK",           D3D11_CULL_BACK     },
    RVALUE_END()
};

static const RValue g_rvCMP[] =
{
    { "NEVER",          D3D11_COMPARISON_NEVER        },
    { "LESS",           D3D11_COMPARISON_LESS         },
    { "EQUAL",          D3D11_COMPARISON_EQUAL        },
    { "LESS_EQUAL",     D3D11_COMPARISON_LESS_EQUAL    },
    { "GREATER",        D3D11_COMPARISON_GREATER      },
    { "NOT_EQUAL",      D3D11_COMPARISON_NOT_EQUAL     },
    { "GREATER_EQUAL",  D3D11_COMPARISON_GREATER_EQUAL },
    { "ALWAYS",         D3D11_COMPARISON_ALWAYS       },
    RVALUE_END()
};

static const RValue g_rvSTENCILOP[] =
{
    { "KEEP",       D3D11_STENCIL_OP_KEEP    },
    { "ZERO",       D3D11_STENCIL_OP_ZERO    },
    { "REPLACE",    D3D11_STENCIL_OP_REPLACE },
    { "INCR_SAT",   D3D11_STENCIL_OP_INCR_SAT },
    { "DECR_SAT",   D3D11_STENCIL_OP_DECR_SAT },
    { "INVERT",     D3D11_STENCIL_OP_INVERT  },
    { "INCR",       D3D11_STENCIL_OP_INCR    },
    { "DECR",       D3D11_STENCIL_OP_DECR    },
    RVALUE_END()
};

static const RValue g_rvBLENDOP[] =
{
    { "ADD",            D3D11_BLEND_OP_ADD         },
    { "SUBTRACT",       D3D11_BLEND_OP_SUBTRACT    },
    { "REV_SUBTRACT",   D3D11_BLEND_OP_REV_SUBTRACT },
    { "MIN",            D3D11_BLEND_OP_MIN         },
    { "MAX",            D3D11_BLEND_OP_MAX         },
    RVALUE_END()
};


//////////////////////////////////////////////////////////////////////////
// Effect HLSL states
//////////////////////////////////////////////////////////////////////////

#define strideof( s, m ) offsetof_fx(s,m[1]) - offsetof_fx(s,m[0])

const LValue g_lvGeneral[] =
{
    // RObjects
    { "RasterizerState",            EBT_Pass,           D3D_SVT_RASTERIZER,         1, 1, false, nullptr,                 ELHS_RasterizerBlock,           offsetof_fx(SPassBlock, BackingStore.pRasterizerBlock),                        0 },
    { "DepthStencilState",          EBT_Pass,           D3D_SVT_DEPTHSTENCIL,       1, 1, false, nullptr,                 ELHS_DepthStencilBlock,         offsetof_fx(SPassBlock, BackingStore.pDepthStencilBlock),                      0 },
    { "BlendState",                 EBT_Pass,           D3D_SVT_BLEND,              1, 1, false, nullptr,                 ELHS_BlendBlock,                offsetof_fx(SPassBlock, BackingStore.pBlendBlock),                             0 },
    { "RenderTargetView",           EBT_Pass,           D3D_SVT_RENDERTARGETVIEW,   1, 8, false, nullptr,                 ELHS_RenderTargetView,          offsetof_fx(SPassBlock, BackingStore.pRenderTargetViews),                      0 },
    { "DepthStencilView",           EBT_Pass,           D3D_SVT_DEPTHSTENCILVIEW,   1, 8, false, nullptr,                 ELHS_DepthStencilView,          offsetof_fx(SPassBlock, BackingStore.pDepthStencilView),                       0 },
    { "GenerateMips",               EBT_Pass,           D3D_SVT_TEXTURE,            1, 1, false, nullptr,                 ELHS_GenerateMips,              0,                                                                          0 },
    // Shaders
    { "VertexShader",               EBT_Pass,           D3D_SVT_VERTEXSHADER,       1, 1, false, g_rvNULL,                ELHS_VertexShaderBlock,         offsetof_fx(SPassBlock, BackingStore.pVertexShaderBlock),                      0 },
    { "PixelShader",                EBT_Pass,           D3D_SVT_PIXELSHADER,        1, 1, false, g_rvNULL,                ELHS_PixelShaderBlock,          offsetof_fx(SPassBlock, BackingStore.pPixelShaderBlock),                       0 },
    { "GeometryShader",             EBT_Pass,           D3D_SVT_GEOMETRYSHADER,     1, 1, false, g_rvNULL,                ELHS_GeometryShaderBlock,       offsetof_fx(SPassBlock, BackingStore.pGeometryShaderBlock),                    0 },
    // RObject config assignments
    { "DS_StencilRef",              EBT_Pass,           D3D_SVT_UINT,               1, 1, false, nullptr,                 ELHS_DS_StencilRef,             offsetof_fx(SPassBlock, BackingStore.StencilRef),                              0 },
    { "AB_BlendFactor",             EBT_Pass,           D3D_SVT_FLOAT,              4, 1, false, nullptr,                 ELHS_B_BlendFactor,             offsetof_fx(SPassBlock, BackingStore.BlendFactor),                             0 },
    { "AB_SampleMask",              EBT_Pass,           D3D_SVT_UINT,               1, 1, false, nullptr,                 ELHS_B_SampleMask,              offsetof_fx(SPassBlock, BackingStore.SampleMask),                              0 },

    { "FillMode",                   EBT_Rasterizer,     D3D_SVT_UINT,               1, 1, false, g_rvFILL,                ELHS_FillMode,                  offsetof_fx(SRasterizerBlock, BackingStore.FillMode),                          0 },
    { "CullMode",                   EBT_Rasterizer,     D3D_SVT_UINT,               1, 1, false, g_rvCULL,                ELHS_CullMode,                  offsetof_fx(SRasterizerBlock, BackingStore.CullMode),                          0 },
    { "FrontCounterClockwise",      EBT_Rasterizer,     D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_FrontCC,                   offsetof_fx(SRasterizerBlock, BackingStore.FrontCounterClockwise),             0 },
    { "DepthBias",                  EBT_Rasterizer,     D3D_SVT_UINT,               1, 1, false, nullptr,                 ELHS_DepthBias,                 offsetof_fx(SRasterizerBlock, BackingStore.DepthBias),                         0 },
    { "DepthBiasClamp",             EBT_Rasterizer,     D3D_SVT_FLOAT,              1, 1, false, nullptr,                 ELHS_DepthBiasClamp,            offsetof_fx(SRasterizerBlock, BackingStore.DepthBiasClamp),                    0 },
    { "SlopeScaledDepthBias",       EBT_Rasterizer,     D3D_SVT_FLOAT,              1, 1, false, nullptr,                 ELHS_SlopeScaledDepthBias,      offsetof_fx(SRasterizerBlock, BackingStore.SlopeScaledDepthBias),              0 },
    { "DepthClipEnable",            EBT_Rasterizer,     D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_DepthClipEnable,           offsetof_fx(SRasterizerBlock, BackingStore.DepthClipEnable),                   0 },
    { "ScissorEnable",              EBT_Rasterizer,     D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_ScissorEnable,             offsetof_fx(SRasterizerBlock, BackingStore.ScissorEnable),                     0 },
    { "MultisampleEnable",          EBT_Rasterizer,     D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_MultisampleEnable,         offsetof_fx(SRasterizerBlock, BackingStore.MultisampleEnable),                 0 },
    { "AntialiasedLineEnable",      EBT_Rasterizer,     D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_AntialiasedLineEnable,     offsetof_fx(SRasterizerBlock, BackingStore.AntialiasedLineEnable),             0 },
    
    { "DepthEnable",                EBT_DepthStencil,   D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_DepthEnable,               offsetof_fx(SDepthStencilBlock, BackingStore.DepthEnable),                     0 },
    { "DepthWriteMask",             EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvDEPTH_WRITE_MASK,    ELHS_DepthWriteMask,            offsetof_fx(SDepthStencilBlock, BackingStore.DepthWriteMask),                  0 },
    { "DepthFunc",                  EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvCMP,                 ELHS_DepthFunc,                 offsetof_fx(SDepthStencilBlock, BackingStore.DepthFunc),                       0 },
    { "StencilEnable",              EBT_DepthStencil,   D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_StencilEnable,             offsetof_fx(SDepthStencilBlock, BackingStore.StencilEnable),                   0 },
    { "StencilReadMask",            EBT_DepthStencil,   D3D_SVT_UINT8,              1, 1, false, nullptr,                 ELHS_StencilReadMask,           offsetof_fx(SDepthStencilBlock, BackingStore.StencilReadMask),                 0 },
    { "StencilWriteMask",           EBT_DepthStencil,   D3D_SVT_UINT8,              1, 1, false, nullptr,                 ELHS_StencilWriteMask,          offsetof_fx(SDepthStencilBlock, BackingStore.StencilWriteMask),                0 },
    { "FrontFaceStencilFail",       EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvSTENCILOP,           ELHS_FrontFaceStencilFailOp,    offsetof_fx(SDepthStencilBlock, BackingStore.FrontFace.StencilFailOp),         0 },
    { "FrontFaceStencilDepthFail",  EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvSTENCILOP,           ELHS_FrontFaceStencilDepthFailOp,offsetof_fx(SDepthStencilBlock, BackingStore.FrontFace.StencilDepthFailOp),   0 },
    { "FrontFaceStencilPass",       EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvSTENCILOP,           ELHS_FrontFaceStencilPassOp,    offsetof_fx(SDepthStencilBlock, BackingStore.FrontFace.StencilPassOp),         0 },
    { "FrontFaceStencilFunc",       EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvCMP,                 ELHS_FrontFaceStencilFunc,      offsetof_fx(SDepthStencilBlock, BackingStore.FrontFace.StencilFunc),           0 },
    { "BackFaceStencilFail",        EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvSTENCILOP,           ELHS_BackFaceStencilFailOp,     offsetof_fx(SDepthStencilBlock, BackingStore.BackFace.StencilFailOp),          0 },
    { "BackFaceStencilDepthFail",   EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvSTENCILOP,           ELHS_BackFaceStencilDepthFailOp,offsetof_fx(SDepthStencilBlock, BackingStore.BackFace.StencilDepthFailOp),     0 },
    { "BackFaceStencilPass",        EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvSTENCILOP,           ELHS_BackFaceStencilPassOp,     offsetof_fx(SDepthStencilBlock, BackingStore.BackFace.StencilPassOp),          0 },
    { "BackFaceStencilFunc",        EBT_DepthStencil,   D3D_SVT_UINT,               1, 1, false, g_rvCMP,                 ELHS_BackFaceStencilFunc,       offsetof_fx(SDepthStencilBlock, BackingStore.BackFace.StencilFunc),            0 },

    { "AlphaToCoverageEnable",      EBT_Blend,          D3D_SVT_BOOL,               1, 1, false, g_rvBOOL,                ELHS_AlphaToCoverage,           offsetof_fx(SBlendBlock, BackingStore.AlphaToCoverageEnable),                  0 },
    { "BlendEnable",                EBT_Blend,          D3D_SVT_BOOL,               1, 8, false, g_rvBOOL,                ELHS_BlendEnable,               offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].BlendEnable),            strideof(SBlendBlock, BackingStore.RenderTarget) },
    { "SrcBlend",                   EBT_Blend,          D3D_SVT_UINT,               1, 8, true,  g_rvBLEND,               ELHS_SrcBlend,                  offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].SrcBlend),               strideof(SBlendBlock, BackingStore.RenderTarget) },
    { "DestBlend",                  EBT_Blend,          D3D_SVT_UINT,               1, 8, true,  g_rvBLEND,               ELHS_DestBlend,                 offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].DestBlend),              strideof(SBlendBlock, BackingStore.RenderTarget) },
    { "BlendOp",                    EBT_Blend,          D3D_SVT_UINT,               1, 8, true,  g_rvBLENDOP,             ELHS_BlendOp,                   offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].BlendOp),                strideof(SBlendBlock, BackingStore.RenderTarget) },
    { "SrcBlendAlpha",              EBT_Blend,          D3D_SVT_UINT,               1, 8, true,  g_rvBLEND,               ELHS_SrcBlendAlpha,             offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].SrcBlendAlpha),          strideof(SBlendBlock, BackingStore.RenderTarget) },
    { "DestBlendAlpha",             EBT_Blend,          D3D_SVT_UINT,               1, 8, true,  g_rvBLEND,               ELHS_DestBlendAlpha,            offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].DestBlendAlpha),         strideof(SBlendBlock, BackingStore.RenderTarget) },
    { "BlendOpAlpha",               EBT_Blend,          D3D_SVT_UINT,               1, 8, true,  g_rvBLENDOP,             ELHS_BlendOpAlpha,              offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].BlendOpAlpha),           strideof(SBlendBlock, BackingStore.RenderTarget) },
    { "RenderTargetWriteMask",      EBT_Blend,          D3D_SVT_UINT8,              1, 8, false, nullptr,                 ELHS_RenderTargetWriteMask,     offsetof_fx(SBlendBlock, BackingStore.RenderTarget[0].RenderTargetWriteMask),  strideof(SBlendBlock, BackingStore.RenderTarget) },

    { "Filter",                     EBT_Sampler,        D3D_SVT_UINT,               1, 1, false, g_rvFILTER,              ELHS_Filter,                    offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.Filter),                   0 },
    { "AddressU",                   EBT_Sampler,        D3D_SVT_UINT,               1, 1, false, g_rvTADDRESS,            ELHS_AddressU,                  offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.AddressU),                 0 },
    { "AddressV",                   EBT_Sampler,        D3D_SVT_UINT,               1, 1, false, g_rvTADDRESS,            ELHS_AddressV,                  offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.AddressV),                 0 },
    { "AddressW",                   EBT_Sampler,        D3D_SVT_UINT,               1, 1, false, g_rvTADDRESS,            ELHS_AddressW,                  offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.AddressW),                 0 },
    { "MipLODBias",                 EBT_Sampler,        D3D_SVT_FLOAT,              1, 1, false, nullptr,                 ELHS_MipLODBias,                offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.MipLODBias),               0 },
    { "MaxAnisotropy",              EBT_Sampler,        D3D_SVT_UINT,               1, 1, false, nullptr,                 ELHS_MaxAnisotropy,             offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.MaxAnisotropy),            0 },
    { "ComparisonFunc",             EBT_Sampler,        D3D_SVT_UINT,               1, 1, false, g_rvCMP,                 ELHS_ComparisonFunc,            offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.ComparisonFunc),           0 },
    { "BorderColor",                EBT_Sampler,        D3D_SVT_FLOAT,              4, 1, false, nullptr,                 ELHS_BorderColor,               offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.BorderColor),              0 },
    { "MinLOD",                     EBT_Sampler,        D3D_SVT_FLOAT,              1, 1, false, nullptr,                 ELHS_MinLOD,                    offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.MinLOD),                   0 },
    { "MaxLOD",                     EBT_Sampler,        D3D_SVT_FLOAT,              1, 1, false, nullptr,                 ELHS_MaxLOD,                    offsetof_fx(SSamplerBlock, BackingStore.SamplerDesc.MaxLOD),                   0 },
    { "Texture",                    EBT_Sampler,        D3D_SVT_TEXTURE,            1, 1, false, g_rvNULL,                ELHS_Texture,                   offsetof_fx(SSamplerBlock, BackingStore.pTexture),                             0 },

    // D3D11 
    { "HullShader",                 EBT_Pass,           D3D11_SVT_HULLSHADER,         1, 1, false, g_rvNULL,              ELHS_HullShaderBlock,           offsetof_fx(SPassBlock, BackingStore.pHullShaderBlock),                       0 },
    { "DomainShader",               EBT_Pass,           D3D11_SVT_DOMAINSHADER,       1, 1, false, g_rvNULL,              ELHS_DomainShaderBlock,         offsetof_fx(SPassBlock, BackingStore.pDomainShaderBlock),                       0 },
    { "ComputeShader",              EBT_Pass,           D3D11_SVT_COMPUTESHADER,      1, 1, false, g_rvNULL,              ELHS_ComputeShaderBlock,        offsetof_fx(SPassBlock, BackingStore.pComputeShaderBlock),                       0 },
};

#define NUM_STATES (sizeof(g_lvGeneral) / sizeof(LValue))
#define MAX_VECTOR_SCALAR_INDEX 8

const uint32_t g_lvGeneralCount = NUM_STATES;

} // end namespace D3DX11Effects

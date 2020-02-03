//--------------------------------------------------------------------------------------
// File: D3DX11Effect.h
//
// Direct3D 11 Effect Types & APIs Header
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

#define D3DX11_EFFECTS_VERSION 1126

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#define NO_D3D11_DEBUG_NAME
#else
#include <d3d11_1.h>
#include <d3d11shader.h>
#endif

#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "dxguid.lib" )

#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// File contents:
//
// 1) Stateblock enums, structs, interfaces, flat APIs
// 2) Effect enums, structs, interfaces, flat APIs
//////////////////////////////////////////////////////////////////////////////

#ifndef D3DX11_BYTES_FROM_BITS
#define D3DX11_BYTES_FROM_BITS(x) (((x) + 7) / 8)
#endif // D3DX11_BYTES_FROM_BITS

#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL MAKE_HRESULT( 1, 0x876, 2156 )
#endif

struct D3DX11_STATE_BLOCK_MASK
{
    uint8_t VS;
    uint8_t VSSamplers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    uint8_t VSShaderResources[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    uint8_t VSConstantBuffers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    uint8_t VSInterfaces[D3DX11_BYTES_FROM_BITS(D3D11_SHADER_MAX_INTERFACES)];
    
    uint8_t HS;
    uint8_t HSSamplers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    uint8_t HSShaderResources[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    uint8_t HSConstantBuffers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    uint8_t HSInterfaces[D3DX11_BYTES_FROM_BITS(D3D11_SHADER_MAX_INTERFACES)];

    uint8_t DS;
    uint8_t DSSamplers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    uint8_t DSShaderResources[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    uint8_t DSConstantBuffers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    uint8_t DSInterfaces[D3DX11_BYTES_FROM_BITS(D3D11_SHADER_MAX_INTERFACES)];

    uint8_t GS;
    uint8_t GSSamplers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    uint8_t GSShaderResources[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    uint8_t GSConstantBuffers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    uint8_t GSInterfaces[D3DX11_BYTES_FROM_BITS(D3D11_SHADER_MAX_INTERFACES)];
    
    uint8_t PS;
    uint8_t PSSamplers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    uint8_t PSShaderResources[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    uint8_t PSConstantBuffers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    uint8_t PSInterfaces[D3DX11_BYTES_FROM_BITS(D3D11_SHADER_MAX_INTERFACES)];
    uint8_t PSUnorderedAccessViews;
    
    uint8_t CS;
    uint8_t CSSamplers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)];
    uint8_t CSShaderResources[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)];
    uint8_t CSConstantBuffers[D3DX11_BYTES_FROM_BITS(D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)];
    uint8_t CSInterfaces[D3DX11_BYTES_FROM_BITS(D3D11_SHADER_MAX_INTERFACES)];
    uint8_t CSUnorderedAccessViews;

    uint8_t IAVertexBuffers[D3DX11_BYTES_FROM_BITS(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)];
    uint8_t IAIndexBuffer;
    uint8_t IAInputLayout;
    uint8_t IAPrimitiveTopology;
    
    uint8_t OMRenderTargets;
    uint8_t OMDepthStencilState;
    uint8_t OMBlendState;
    
    uint8_t RSViewports;
    uint8_t RSScissorRects;
    uint8_t RSRasterizerState;
    
    uint8_t SOBuffers;
    
    uint8_t Predication;
};

//----------------------------------------------------------------------------
// D3DX11_EFFECT flags:
// -------------------------------------
//
// These flags are passed in when creating an effect, and affect
// the runtime effect behavior:
//
// (Currently none)
//
//
// These flags are set by the effect runtime:
//
// D3DX11_EFFECT_OPTIMIZED
//   This effect has been optimized. Reflection functions that rely on 
//   names/semantics/strings should fail. This is set when Optimize() is
//   called, but CEffect::IsOptimized() should be used to test for this.
//
// D3DX11_EFFECT_CLONE
//   This effect is a clone of another effect. Single CBs will never be 
//   updated when internal variable values are changed.
//   This flag is not set when the D3DX11_EFFECT_CLONE_FORCE_NONSINGLE flag 
//   is used in cloning.
//
//----------------------------------------------------------------------------

#define D3DX11_EFFECT_OPTIMIZED                         (1 << 21)
#define D3DX11_EFFECT_CLONE                             (1 << 22)

// Mask of valid D3DCOMPILE_EFFECT flags for D3DX11CreateEffect*
#define D3DX11_EFFECT_RUNTIME_VALID_FLAGS (0)

//----------------------------------------------------------------------------
// D3DX11_EFFECT_VARIABLE flags:
// ----------------------------
//
// These flags describe an effect variable (global or annotation),
// and are returned in D3DX11_EFFECT_VARIABLE_DESC::Flags.
//
// D3DX11_EFFECT_VARIABLE_ANNOTATION
//   Indicates that this is an annotation on a technique, pass, or global
//   variable. Otherwise, this is a global variable. Annotations cannot
//   be shared.
//
// D3DX11_EFFECT_VARIABLE_EXPLICIT_BIND_POINT
//   Indicates that the variable has been explicitly bound using the
//   register keyword.
//----------------------------------------------------------------------------

#define D3DX11_EFFECT_VARIABLE_ANNOTATION              (1 << 1)
#define D3DX11_EFFECT_VARIABLE_EXPLICIT_BIND_POINT     (1 << 2)

//----------------------------------------------------------------------------
// D3DX11_EFFECT_CLONE flags:
// ----------------------------
//
// These flags modify the effect cloning process and are passed into Clone.
//
// D3DX11_EFFECT_CLONE_FORCE_NONSINGLE
//   Ignore all "single" qualifiers on cbuffers.  All cbuffers will have their
//   own ID3D11Buffer's created in the cloned effect.
//----------------------------------------------------------------------------

#define D3DX11_EFFECT_CLONE_FORCE_NONSINGLE        	    (1 << 0)


//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectType //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DX11_EFFECT_TYPE_DESC:
//
// Retrieved by ID3DX11EffectType::GetDesc()
//----------------------------------------------------------------------------

struct D3DX11_EFFECT_TYPE_DESC
{
    LPCSTR  TypeName;               // Name of the type 
                                    // (e.g. "float4" or "MyStruct")

    D3D_SHADER_VARIABLE_CLASS    Class;  // (e.g. scalar, vector, object, etc.)
    D3D_SHADER_VARIABLE_TYPE     Type;   // (e.g. float, texture, vertexshader, etc.)
    
    uint32_t    Elements;           // Number of elements in this type
                                    // (0 if not an array) 
    uint32_t    Members;            // Number of members
                                    // (0 if not a structure)
    uint32_t    Rows;               // Number of rows in this type
                                    // (0 if not a numeric primitive)
    uint32_t    Columns;            // Number of columns in this type
                                    // (0 if not a numeric primitive)
    
    uint32_t    PackedSize;         // Number of bytes required to represent
                                    // this data type, when tightly packed
    uint32_t    UnpackedSize;       // Number of bytes occupied by this data
                                    // type, when laid out in a constant buffer
    uint32_t    Stride;             // Number of bytes to seek between elements,
                                    // when laid out in a constant buffer
};

typedef interface ID3DX11EffectType ID3DX11EffectType;
typedef interface ID3DX11EffectType *LPD3D11EFFECTTYPE;

// {4250D721-D5E5-491F-B62B-587C43186285}
DEFINE_GUID(IID_ID3DX11EffectType, 
            0x4250d721, 0xd5e5, 0x491f, 0xb6, 0x2b, 0x58, 0x7c, 0x43, 0x18, 0x62, 0x85);

#undef INTERFACE
#define INTERFACE ID3DX11EffectType

DECLARE_INTERFACE_(ID3DX11EffectType, IUnknown)
{
    // IUnknown

    // ID3DX11EffectType
    STDMETHOD_(bool, IsValid)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ _Out_ D3DX11_EFFECT_TYPE_DESC *pDesc) PURE;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeBySemantic)(THIS_ _In_z_ LPCSTR Semantic) PURE;
    STDMETHOD_(LPCSTR, GetMemberName)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(LPCSTR, GetMemberSemantic)(THIS_ _In_ uint32_t Index) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectVariable //////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DX11_EFFECT_VARIABLE_DESC:
//
// Retrieved by ID3DX11EffectVariable::GetDesc()
//----------------------------------------------------------------------------

struct D3DX11_EFFECT_VARIABLE_DESC
{
    LPCSTR      Name;               // Name of this variable, annotation, 
                                    // or structure member
    LPCSTR      Semantic;           // Semantic string of this variable
                                    // or structure member (nullptr for 
                                    // annotations or if not present)
    
    uint32_t    Flags;              // D3DX11_EFFECT_VARIABLE_* flags
    uint32_t    Annotations;        // Number of annotations on this variable
                                    // (always 0 for annotations)

    uint32_t    BufferOffset;       // Offset into containing cbuffer or tbuffer
                                    // (always 0 for annotations or variables
                                    // not in constant buffers)

    uint32_t    ExplicitBindPoint;  // Used if the variable has been explicitly bound
                                    // using the register keyword. Check Flags for
                                    // D3DX11_EFFECT_VARIABLE_EXPLICIT_BIND_POINT;
};

typedef interface ID3DX11EffectVariable ID3DX11EffectVariable;
typedef interface ID3DX11EffectVariable *LPD3D11EFFECTVARIABLE;

// {036A777D-B56E-4B25-B313-CC3DDAB71873}
DEFINE_GUID(IID_ID3DX11EffectVariable, 
            0x036a777d, 0xb56e, 0x4b25, 0xb3, 0x13, 0xcc, 0x3d, 0xda, 0xb7, 0x18, 0x73);

#undef INTERFACE
#define INTERFACE ID3DX11EffectVariable

// Forward defines
typedef interface ID3DX11EffectScalarVariable ID3DX11EffectScalarVariable;
typedef interface ID3DX11EffectVectorVariable ID3DX11EffectVectorVariable;
typedef interface ID3DX11EffectMatrixVariable ID3DX11EffectMatrixVariable;
typedef interface ID3DX11EffectStringVariable ID3DX11EffectStringVariable;
typedef interface ID3DX11EffectClassInstanceVariable ID3DX11EffectClassInstanceVariable;
typedef interface ID3DX11EffectInterfaceVariable ID3DX11EffectInterfaceVariable;
typedef interface ID3DX11EffectShaderResourceVariable ID3DX11EffectShaderResourceVariable;
typedef interface ID3DX11EffectUnorderedAccessViewVariable ID3DX11EffectUnorderedAccessViewVariable;
typedef interface ID3DX11EffectRenderTargetViewVariable ID3DX11EffectRenderTargetViewVariable;
typedef interface ID3DX11EffectDepthStencilViewVariable ID3DX11EffectDepthStencilViewVariable;
typedef interface ID3DX11EffectConstantBuffer ID3DX11EffectConstantBuffer;
typedef interface ID3DX11EffectShaderVariable ID3DX11EffectShaderVariable;
typedef interface ID3DX11EffectBlendVariable ID3DX11EffectBlendVariable;
typedef interface ID3DX11EffectDepthStencilVariable ID3DX11EffectDepthStencilVariable;
typedef interface ID3DX11EffectRasterizerVariable ID3DX11EffectRasterizerVariable;
typedef interface ID3DX11EffectSamplerVariable ID3DX11EffectSamplerVariable;

DECLARE_INTERFACE_(ID3DX11EffectVariable, IUnknown)
{
    // IUnknown

    // ID3DX11EffectVariable
    STDMETHOD_(bool, IsValid)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectType*, GetType)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ _Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberBySemantic)(THIS_ _In_z_ LPCSTR Semantic) PURE;
    
    STDMETHOD_(ID3DX11EffectVariable*, GetElement)(THIS_ _In_ uint32_t Index) PURE;

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetParentConstantBuffer)(THIS) PURE;
    
    STDMETHOD_(ID3DX11EffectScalarVariable*, AsScalar)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectVectorVariable*, AsVector)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectMatrixVariable*, AsMatrix)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectStringVariable*, AsString)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectClassInstanceVariable*, AsClassInstance)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectInterfaceVariable*, AsInterface)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectShaderResourceVariable*, AsShaderResource)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectUnorderedAccessViewVariable*, AsUnorderedAccessView)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectRenderTargetViewVariable*, AsRenderTargetView)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectDepthStencilViewVariable*, AsDepthStencilView)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectConstantBuffer*, AsConstantBuffer)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectShaderVariable*, AsShader)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectBlendVariable*, AsBlend)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectDepthStencilVariable*, AsDepthStencil)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectRasterizerVariable*, AsRasterizer)(THIS) PURE;
    STDMETHOD_(ID3DX11EffectSamplerVariable*, AsSampler)(THIS) PURE;
    
    STDMETHOD(SetRawValue)(THIS_ _In_reads_bytes_(ByteCount) const void *pData, _In_ uint32_t ByteOffset, _In_ uint32_t ByteCount) PURE;
    STDMETHOD(GetRawValue)(THIS_ _Out_writes_bytes_(ByteCount) void *pData, _In_ uint32_t ByteOffset, _In_ uint32_t ByteCount) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectScalarVariable ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectScalarVariable ID3DX11EffectScalarVariable;
typedef interface ID3DX11EffectScalarVariable *LPD3D11EFFECTSCALARVARIABLE;

// {921EF2E5-A65D-4E92-9FC6-4E9CC09A4ADE}
DEFINE_GUID(IID_ID3DX11EffectScalarVariable, 
            0x921ef2e5, 0xa65d, 0x4e92, 0x9f, 0xc6, 0x4e, 0x9c, 0xc0, 0x9a, 0x4a, 0xde);

#undef INTERFACE
#define INTERFACE ID3DX11EffectScalarVariable

DECLARE_INTERFACE_(ID3DX11EffectScalarVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectScalarVariable
    STDMETHOD(SetFloat)(THIS_ _In_ const float Value) PURE;
    STDMETHOD(GetFloat)(THIS_ _Out_ float *pValue) PURE;    
    
    STDMETHOD(SetFloatArray)(THIS_ _In_reads_(Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ _Out_writes_(Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    
    STDMETHOD(SetInt)(THIS_ _In_ const int Value) PURE;
    STDMETHOD(GetInt)(THIS_ _Out_ int *pValue) PURE;
    
    STDMETHOD(SetIntArray)(THIS_ _In_reads_(Count) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetIntArray)(THIS_ _Out_writes_(Count) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    
    STDMETHOD(SetBool)(THIS_ _In_ const bool Value) PURE;
    STDMETHOD(GetBool)(THIS_ _Out_ bool *pValue) PURE;
    
    STDMETHOD(SetBoolArray)(THIS_ _In_reads_(Count) const bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ _Out_writes_(Count) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectVectorVariable ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectVectorVariable ID3DX11EffectVectorVariable;
typedef interface ID3DX11EffectVectorVariable *LPD3D11EFFECTVECTORVARIABLE;

// {5E785D4A-D87B-48D8-B6E6-0F8CA7E7467A}
DEFINE_GUID(IID_ID3DX11EffectVectorVariable, 
            0x5e785d4a, 0xd87b, 0x48d8, 0xb6, 0xe6, 0x0f, 0x8c, 0xa7, 0xe7, 0x46, 0x7a);

#undef INTERFACE
#define INTERFACE ID3DX11EffectVectorVariable

DECLARE_INTERFACE_(ID3DX11EffectVectorVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectVectorVariable
    STDMETHOD(SetBoolVector) (THIS_ _In_reads_(4) const bool *pData) PURE;
    STDMETHOD(SetIntVector)  (THIS_ _In_reads_(4) const int *pData) PURE;
    STDMETHOD(SetFloatVector)(THIS_ _In_reads_(4) const float *pData) PURE;

    STDMETHOD(GetBoolVector) (THIS_ _Out_writes_(4) bool *pData) PURE;
    STDMETHOD(GetIntVector)  (THIS_ _Out_writes_(4) int *pData) PURE;
    STDMETHOD(GetFloatVector)(THIS_ _Out_writes_(4) float *pData) PURE;

    STDMETHOD(SetBoolVectorArray) (THIS_ _In_reads_(Count*4) const bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(SetIntVectorArray)  (THIS_ _In_reads_(Count*4) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(SetFloatVectorArray)(THIS_ _In_reads_(Count*4) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;

    STDMETHOD(GetBoolVectorArray) (THIS_ _Out_writes_(Count*4) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetIntVectorArray)  (THIS_ _Out_writes_(Count*4) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetFloatVectorArray)(THIS_ _Out_writes_(Count*4) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectMatrixVariable ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectMatrixVariable ID3DX11EffectMatrixVariable;
typedef interface ID3DX11EffectMatrixVariable *LPD3D11EFFECTMATRIXVARIABLE;

// {E1096CF4-C027-419A-8D86-D29173DC803E}
DEFINE_GUID(IID_ID3DX11EffectMatrixVariable, 
            0xe1096cf4, 0xc027, 0x419a, 0x8d, 0x86, 0xd2, 0x91, 0x73, 0xdc, 0x80, 0x3e);

#undef INTERFACE
#define INTERFACE ID3DX11EffectMatrixVariable

DECLARE_INTERFACE_(ID3DX11EffectMatrixVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectMatrixVariable
    STDMETHOD(SetMatrix)(THIS_ _In_reads_(16) const float *pData) PURE;
    STDMETHOD(GetMatrix)(THIS_ _Out_writes_(16) float *pData) PURE;
    
    STDMETHOD(SetMatrixArray)(THIS_ _In_reads_(Count*16) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ _Out_writes_(Count*16) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    
    STDMETHOD(SetMatrixPointerArray)(_In_reads_(Count*16) const float **ppData, uint32_t Offset, uint32_t Count) PURE;
    STDMETHOD(GetMatrixPointerArray)(_Out_writes_(Count*16) float **ppData, uint32_t Offset, uint32_t Count)  PURE;

    STDMETHOD(SetMatrixTranspose)(THIS_ _In_reads_(16) const float *pData) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ _Out_writes_(16) float *pData) PURE;
    
    STDMETHOD(SetMatrixTransposeArray)(THIS_ _In_reads_(Count*16) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ _Out_writes_(Count*16) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;

    STDMETHOD(SetMatrixTransposePointerArray)(_In_reads_(Count*16) const float **ppData, uint32_t Offset, uint32_t Count)  PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(_Out_writes_(Count*16) float **ppData, uint32_t Offset, uint32_t Count)  PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectStringVariable ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectStringVariable ID3DX11EffectStringVariable;
typedef interface ID3DX11EffectStringVariable *LPD3D11EFFECTSTRINGVARIABLE;

// {F355C818-01BE-4653-A7CC-60FFFEDDC76D}
DEFINE_GUID(IID_ID3DX11EffectStringVariable, 
            0xf355c818, 0x01be, 0x4653, 0xa7, 0xcc, 0x60, 0xff, 0xfe, 0xdd, 0xc7, 0x6d);

#undef INTERFACE
#define INTERFACE ID3DX11EffectStringVariable

DECLARE_INTERFACE_(ID3DX11EffectStringVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectStringVariable
    STDMETHOD(GetString)(THIS_ _Outptr_result_z_ LPCSTR *ppString) PURE;
    STDMETHOD(GetStringArray)(THIS_ _Out_writes_(Count) LPCSTR *ppStrings, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectClassInstanceVariable ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectClassInstanceVariable ID3DX11EffectClassInstanceVariable;
typedef interface ID3DX11EffectClassInstanceVariable *LPD3D11EFFECTCLASSINSTANCEVARIABLE;

// {926A8053-2A39-4DB4-9BDE-CF649ADEBDC1}
DEFINE_GUID(IID_ID3DX11EffectClassInstanceVariable, 
            0x926a8053, 0x2a39, 0x4db4, 0x9b, 0xde, 0xcf, 0x64, 0x9a, 0xde, 0xbd, 0xc1);

#undef INTERFACE
#define INTERFACE ID3DX11EffectClassInstanceVariable

DECLARE_INTERFACE_(ID3DX11EffectClassInstanceVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectClassInstanceVariable
    STDMETHOD(GetClassInstance)(_Outptr_ ID3D11ClassInstance** ppClassInstance) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectInterfaceVariable ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectInterfaceVariable ID3DX11EffectInterfaceVariable;
typedef interface ID3DX11EffectInterfaceVariable *LPD3D11EFFECTINTERFACEVARIABLE;

// {516C8CD8-1C80-40A4-B19B-0688792F11A5}
DEFINE_GUID(IID_ID3DX11EffectInterfaceVariable, 
            0x516c8cd8, 0x1c80, 0x40a4, 0xb1, 0x9b, 0x06, 0x88, 0x79, 0x2f, 0x11, 0xa5);

#undef INTERFACE
#define INTERFACE ID3DX11EffectInterfaceVariable

DECLARE_INTERFACE_(ID3DX11EffectInterfaceVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectInterfaceVariable
    STDMETHOD(SetClassInstance)(_In_ ID3DX11EffectClassInstanceVariable *pEffectClassInstance) PURE;
    STDMETHOD(GetClassInstance)(_Outptr_ ID3DX11EffectClassInstanceVariable **ppEffectClassInstance) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectShaderResourceVariable ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectShaderResourceVariable ID3DX11EffectShaderResourceVariable;
typedef interface ID3DX11EffectShaderResourceVariable *LPD3D11EFFECTSHADERRESOURCEVARIABLE;

// {350DB233-BBE0-485C-9BFE-C0026B844F89}
DEFINE_GUID(IID_ID3DX11EffectShaderResourceVariable, 
            0x350db233, 0xbbe0, 0x485c, 0x9b, 0xfe, 0xc0, 0x02, 0x6b, 0x84, 0x4f, 0x89);

#undef INTERFACE
#define INTERFACE ID3DX11EffectShaderResourceVariable

DECLARE_INTERFACE_(ID3DX11EffectShaderResourceVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectShaderResourceVariable
    STDMETHOD(SetResource)(THIS_ _In_ ID3D11ShaderResourceView *pResource) PURE;
    STDMETHOD(GetResource)(THIS_ _Outptr_ ID3D11ShaderResourceView **ppResource) PURE;
    
    STDMETHOD(SetResourceArray)(THIS_ _In_reads_(Count) ID3D11ShaderResourceView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetResourceArray)(THIS_ _Out_writes_(Count) ID3D11ShaderResourceView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectUnorderedAccessViewVariable ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectUnorderedAccessViewVariable ID3DX11EffectUnorderedAccessViewVariable;
typedef interface ID3DX11EffectUnorderedAccessViewVariable *LPD3D11EFFECTUNORDEREDACCESSVIEWVARIABLE;

// {79B4AC8C-870A-47D2-B05A-8BD5CC3EE6C9}
DEFINE_GUID(IID_ID3DX11EffectUnorderedAccessViewVariable, 
            0x79b4ac8c, 0x870a, 0x47d2, 0xb0, 0x5a, 0x8b, 0xd5, 0xcc, 0x3e, 0xe6, 0xc9);

#undef INTERFACE
#define INTERFACE ID3DX11EffectUnorderedAccessViewVariable

DECLARE_INTERFACE_(ID3DX11EffectUnorderedAccessViewVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectUnorderedAccessViewVariable
    STDMETHOD(SetUnorderedAccessView)(THIS_ _In_ ID3D11UnorderedAccessView *pResource) PURE;
    STDMETHOD(GetUnorderedAccessView)(THIS_ _Outptr_ ID3D11UnorderedAccessView **ppResource) PURE;

    STDMETHOD(SetUnorderedAccessViewArray)(THIS_ _In_reads_(Count) ID3D11UnorderedAccessView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetUnorderedAccessViewArray)(THIS_ _Out_writes_(Count) ID3D11UnorderedAccessView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectRenderTargetViewVariable //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectRenderTargetViewVariable ID3DX11EffectRenderTargetViewVariable;
typedef interface ID3DX11EffectRenderTargetViewVariable *LPD3D11EFFECTRENDERTARGETVIEWVARIABLE;

// {D5066909-F40C-43F8-9DB5-057C2A208552}
DEFINE_GUID(IID_ID3DX11EffectRenderTargetViewVariable, 
            0xd5066909, 0xf40c, 0x43f8, 0x9d, 0xb5, 0x05, 0x7c, 0x2a, 0x20, 0x85, 0x52);

#undef INTERFACE
#define INTERFACE ID3DX11EffectRenderTargetViewVariable

DECLARE_INTERFACE_(ID3DX11EffectRenderTargetViewVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectRenderTargetViewVariable
    STDMETHOD(SetRenderTarget)(THIS_ _In_ ID3D11RenderTargetView *pResource) PURE;
    STDMETHOD(GetRenderTarget)(THIS_ _Outptr_ ID3D11RenderTargetView **ppResource) PURE;
    
    STDMETHOD(SetRenderTargetArray)(THIS_ _In_reads_(Count) ID3D11RenderTargetView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetRenderTargetArray)(THIS_ _Out_writes_(Count) ID3D11RenderTargetView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectDepthStencilViewVariable //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectDepthStencilViewVariable ID3DX11EffectDepthStencilViewVariable;
typedef interface ID3DX11EffectDepthStencilViewVariable *LPD3D11EFFECTDEPTHSTENCILVIEWVARIABLE;

// {33C648AC-2E9E-4A2E-9CD6-DE31ACC5B347}
DEFINE_GUID(IID_ID3DX11EffectDepthStencilViewVariable, 
            0x33c648ac, 0x2e9e, 0x4a2e, 0x9c, 0xd6, 0xde, 0x31, 0xac, 0xc5, 0xb3, 0x47);

#undef INTERFACE
#define INTERFACE ID3DX11EffectDepthStencilViewVariable

DECLARE_INTERFACE_(ID3DX11EffectDepthStencilViewVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectDepthStencilViewVariable
    STDMETHOD(SetDepthStencil)(THIS_ _In_ ID3D11DepthStencilView *pResource) PURE;
    STDMETHOD(GetDepthStencil)(THIS_ _Outptr_ ID3D11DepthStencilView **ppResource) PURE;
    
    STDMETHOD(SetDepthStencilArray)(THIS_ _In_reads_(Count) ID3D11DepthStencilView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
    STDMETHOD(GetDepthStencilArray)(THIS_ _Out_writes_(Count) ID3D11DepthStencilView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectConstantBuffer ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectConstantBuffer ID3DX11EffectConstantBuffer;
typedef interface ID3DX11EffectConstantBuffer *LPD3D11EFFECTCONSTANTBUFFER;

// {2CB6C733-82D2-4000-B3DA-6219D9A99BF2}
DEFINE_GUID(IID_ID3DX11EffectConstantBuffer, 
            0x2cb6c733, 0x82d2, 0x4000, 0xb3, 0xda, 0x62, 0x19, 0xd9, 0xa9, 0x9b, 0xf2);

#undef INTERFACE
#define INTERFACE ID3DX11EffectConstantBuffer

DECLARE_INTERFACE_(ID3DX11EffectConstantBuffer, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectConstantBuffer
    STDMETHOD(SetConstantBuffer)(THIS_ _In_ ID3D11Buffer *pConstantBuffer) PURE;
    STDMETHOD(UndoSetConstantBuffer)(THIS) PURE;
    STDMETHOD(GetConstantBuffer)(THIS_ _Outptr_ ID3D11Buffer **ppConstantBuffer) PURE;
    
    STDMETHOD(SetTextureBuffer)(THIS_ _In_ ID3D11ShaderResourceView *pTextureBuffer) PURE;
    STDMETHOD(UndoSetTextureBuffer)(THIS) PURE;
    STDMETHOD(GetTextureBuffer)(THIS_ _Outptr_ ID3D11ShaderResourceView **ppTextureBuffer) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectShaderVariable ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DX11_EFFECT_SHADER_DESC:
//
// Retrieved by ID3DX11EffectShaderVariable::GetShaderDesc()
//----------------------------------------------------------------------------

struct D3DX11_EFFECT_SHADER_DESC
{
    const uint8_t *pInputSignature;         // Passed into CreateInputLayout,
                                            // valid on VS and GS only
    
    bool IsInline;                          // Is this an anonymous shader variable
                                            // resulting from an inline shader assignment?
    
    
    // -- The following fields are not valid after Optimize() --
    const uint8_t *pBytecode;                // Shader bytecode
    uint32_t BytecodeLength;
    
    LPCSTR SODecls[D3D11_SO_STREAM_COUNT];  // Stream out declaration string (for GS with SO)
    uint32_t RasterizedStream;
    
    uint32_t NumInputSignatureEntries;          // Number of entries in the input signature
    uint32_t NumOutputSignatureEntries;         // Number of entries in the output signature
    uint32_t NumPatchConstantSignatureEntries;  // Number of entries in the patch constant signature
};


typedef interface ID3DX11EffectShaderVariable ID3DX11EffectShaderVariable;
typedef interface ID3DX11EffectShaderVariable *LPD3D11EFFECTSHADERVARIABLE;

// {7508B344-020A-4EC7-9118-62CDD36C88D7}
DEFINE_GUID(IID_ID3DX11EffectShaderVariable, 
            0x7508b344, 0x020a, 0x4ec7, 0x91, 0x18, 0x62, 0xcd, 0xd3, 0x6c, 0x88, 0xd7);

#undef INTERFACE
#define INTERFACE ID3DX11EffectShaderVariable

DECLARE_INTERFACE_(ID3DX11EffectShaderVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectShaderVariable
    STDMETHOD(GetShaderDesc)(THIS_ _In_ uint32_t ShaderIndex, _Out_ D3DX11_EFFECT_SHADER_DESC *pDesc) PURE;
    
    STDMETHOD(GetVertexShader)(THIS_ _In_ uint32_t ShaderIndex, _Outptr_ ID3D11VertexShader **ppVS) PURE;
    STDMETHOD(GetGeometryShader)(THIS_ _In_ uint32_t ShaderIndex, _Outptr_ ID3D11GeometryShader **ppGS) PURE;
    STDMETHOD(GetPixelShader)(THIS_ _In_ uint32_t ShaderIndex, _Outptr_ ID3D11PixelShader **ppPS) PURE;
    STDMETHOD(GetHullShader)(THIS_ _In_ uint32_t ShaderIndex, _Outptr_ ID3D11HullShader **ppHS) PURE;
    STDMETHOD(GetDomainShader)(THIS_ _In_ uint32_t ShaderIndex, _Outptr_ ID3D11DomainShader **ppDS) PURE;
    STDMETHOD(GetComputeShader)(THIS_ _In_ uint32_t ShaderIndex, _Outptr_ ID3D11ComputeShader **ppCS) PURE;
    
    STDMETHOD(GetInputSignatureElementDesc)(THIS_ _In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
    STDMETHOD(GetOutputSignatureElementDesc)(THIS_ _In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
    STDMETHOD(GetPatchConstantSignatureElementDesc)(THIS_ _In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectBlendVariable /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectBlendVariable ID3DX11EffectBlendVariable;
typedef interface ID3DX11EffectBlendVariable *LPD3D11EFFECTBLENDVARIABLE;

// {D664F4D7-3B81-4805-B277-C1DF58C39F53}
DEFINE_GUID(IID_ID3DX11EffectBlendVariable, 
            0xd664f4d7, 0x3b81, 0x4805, 0xb2, 0x77, 0xc1, 0xdf, 0x58, 0xc3, 0x9f, 0x53);

#undef INTERFACE
#define INTERFACE ID3DX11EffectBlendVariable

DECLARE_INTERFACE_(ID3DX11EffectBlendVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectBlendVariable
    STDMETHOD(GetBlendState)(THIS_ _In_ uint32_t Index, _Outptr_ ID3D11BlendState **ppState) PURE;
    STDMETHOD(SetBlendState)(THIS_ _In_ uint32_t Index, _In_ ID3D11BlendState *pState) PURE;
    STDMETHOD(UndoSetBlendState)(THIS_ _In_ uint32_t Index) PURE; 
    STDMETHOD(GetBackingStore)(THIS_ _In_ uint32_t Index, _Out_ D3D11_BLEND_DESC *pDesc) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectDepthStencilVariable //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectDepthStencilVariable ID3DX11EffectDepthStencilVariable;
typedef interface ID3DX11EffectDepthStencilVariable *LPD3D11EFFECTDEPTHSTENCILVARIABLE;

// {69B5751B-61A5-48E5-BD41-D93988111563}
DEFINE_GUID(IID_ID3DX11EffectDepthStencilVariable, 
            0x69b5751b, 0x61a5, 0x48e5, 0xbd, 0x41, 0xd9, 0x39, 0x88, 0x11, 0x15, 0x63);

#undef INTERFACE
#define INTERFACE ID3DX11EffectDepthStencilVariable

DECLARE_INTERFACE_(ID3DX11EffectDepthStencilVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectDepthStencilVariable
    STDMETHOD(GetDepthStencilState)(THIS_ _In_ uint32_t Index, _Outptr_ ID3D11DepthStencilState **ppState) PURE;
    STDMETHOD(SetDepthStencilState)(THIS_ _In_ uint32_t Index, _In_ ID3D11DepthStencilState *pState) PURE;
    STDMETHOD(UndoSetDepthStencilState)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD(GetBackingStore)(THIS_ _In_ uint32_t Index, _Out_ D3D11_DEPTH_STENCIL_DESC *pDesc) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectRasterizerVariable ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectRasterizerVariable ID3DX11EffectRasterizerVariable;
typedef interface ID3DX11EffectRasterizerVariable *LPD3D11EFFECTRASTERIZERVARIABLE;

// {53A262F6-5F74-4151-A132-E3DD19A62C9D}
DEFINE_GUID(IID_ID3DX11EffectRasterizerVariable, 
            0x53a262f6, 0x5f74, 0x4151, 0xa1, 0x32, 0xe3, 0xdd, 0x19, 0xa6, 0x2c, 0x9d);

#undef INTERFACE
#define INTERFACE ID3DX11EffectRasterizerVariable

DECLARE_INTERFACE_(ID3DX11EffectRasterizerVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectRasterizerVariable
    STDMETHOD(GetRasterizerState)(THIS_ _In_ uint32_t Index, _Outptr_ ID3D11RasterizerState **ppState) PURE;
    STDMETHOD(SetRasterizerState)(THIS_ _In_ uint32_t Index, _In_ ID3D11RasterizerState *pState) PURE;
    STDMETHOD(UndoSetRasterizerState)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD(GetBackingStore)(THIS_ _In_ uint32_t Index, _Out_ D3D11_RASTERIZER_DESC *pDesc) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectSamplerVariable ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DX11EffectSamplerVariable ID3DX11EffectSamplerVariable;
typedef interface ID3DX11EffectSamplerVariable *LPD3D11EFFECTSAMPLERVARIABLE;

// {C6402E55-1095-4D95-8931-F92660513DD9}
DEFINE_GUID(IID_ID3DX11EffectSamplerVariable, 
            0xc6402e55, 0x1095, 0x4d95, 0x89, 0x31, 0xf9, 0x26, 0x60, 0x51, 0x3d, 0xd9);

#undef INTERFACE
#define INTERFACE ID3DX11EffectSamplerVariable

DECLARE_INTERFACE_(ID3DX11EffectSamplerVariable, ID3DX11EffectVariable)
{
    // IUnknown
    // ID3DX11EffectVariable

    // ID3DX11EffectSamplerVariable
    STDMETHOD(GetSampler)(THIS_ _In_ uint32_t Index, _Outptr_ ID3D11SamplerState **ppSampler) PURE;
    STDMETHOD(SetSampler)(THIS_ _In_ uint32_t Index, _In_ ID3D11SamplerState *pSampler) PURE;
    STDMETHOD(UndoSetSampler)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD(GetBackingStore)(THIS_ _In_ uint32_t Index, _Out_ D3D11_SAMPLER_DESC *pDesc) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectPass //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DX11_PASS_DESC:
//
// Retrieved by ID3DX11EffectPass::GetDesc()
//----------------------------------------------------------------------------

struct D3DX11_PASS_DESC
{
    LPCSTR Name;                    // Name of this pass (nullptr if not anonymous)    
    uint32_t Annotations;           // Number of annotations on this pass
    
    uint8_t *pIAInputSignature;     // Signature from VS or GS (if there is no VS)
                                    // or nullptr if neither exists
    size_t IAInputSignatureSize;    // Singature size in bytes                                
                                    
    uint32_t StencilRef;            // Specified in SetDepthStencilState()
    uint32_t SampleMask;            // Specified in SetBlendState()
    FLOAT BlendFactor[4];           // Specified in SetBlendState()
};

//----------------------------------------------------------------------------
// D3DX11_PASS_SHADER_DESC:
//
// Retrieved by ID3DX11EffectPass::Get**ShaderDesc()
//----------------------------------------------------------------------------

struct D3DX11_PASS_SHADER_DESC
{
    ID3DX11EffectShaderVariable *pShaderVariable;   // The variable that this shader came from.
                                                    // If this is an inline shader assignment,
                                                    //   the returned interface will be an 
                                                    //   anonymous shader variable, which is
                                                    //   not retrievable any other way.  It's
                                                    //   name in the variable description will
                                                    //   be "$Anonymous".
                                                    // If there is no assignment of this type in
                                                    //   the pass block, pShaderVariable != nullptr,
                                                    //   but pShaderVariable->IsValid() == false.
    
    uint32_t                    ShaderIndex;        // The element of pShaderVariable (if an array)
                                                    // or 0 if not applicable
};

typedef interface ID3DX11EffectPass ID3DX11EffectPass;
typedef interface ID3DX11EffectPass *LPD3D11EFFECTPASS;

// {3437CEC4-4AC1-4D87-8916-F4BD5A41380C}
DEFINE_GUID(IID_ID3DX11EffectPass, 
            0x3437cec4, 0x4ac1, 0x4d87, 0x89, 0x16, 0xf4, 0xbd, 0x5a, 0x41, 0x38, 0x0c);

#undef INTERFACE
#define INTERFACE ID3DX11EffectPass

DECLARE_INTERFACE_(ID3DX11EffectPass, IUnknown)
{
    // IUnknown

    // ID3DX11EffectPass
    STDMETHOD_(bool, IsValid)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ _Out_ D3DX11_PASS_DESC *pDesc) PURE;
    
    STDMETHOD(GetVertexShaderDesc)(THIS_ _Out_ D3DX11_PASS_SHADER_DESC *pDesc) PURE;
    STDMETHOD(GetGeometryShaderDesc)(THIS_ _Out_ D3DX11_PASS_SHADER_DESC *pDesc) PURE;
    STDMETHOD(GetPixelShaderDesc)(THIS_ _Out_ D3DX11_PASS_SHADER_DESC *pDesc) PURE;
    STDMETHOD(GetHullShaderDesc)(THIS_ _Out_ D3DX11_PASS_SHADER_DESC *pDesc) PURE;
    STDMETHOD(GetDomainShaderDesc)(THIS_ _Out_ D3DX11_PASS_SHADER_DESC *pDesc) PURE;
    STDMETHOD(GetComputeShaderDesc)(THIS_ _Out_ D3DX11_PASS_SHADER_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    
    STDMETHOD(Apply)(THIS_ _In_ uint32_t Flags, _In_ ID3D11DeviceContext* pContext) PURE;
    
    STDMETHOD(ComputeStateBlockMask)(THIS_ _Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectTechnique /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DX11_TECHNIQUE_DESC:
//
// Retrieved by ID3DX11EffectTechnique::GetDesc()
//----------------------------------------------------------------------------

struct D3DX11_TECHNIQUE_DESC
{
    LPCSTR  Name;                   // Name of this technique (nullptr if not anonymous)
    uint32_t    Passes;             // Number of passes contained within
    uint32_t    Annotations;        // Number of annotations on this technique
};

typedef interface ID3DX11EffectTechnique ID3DX11EffectTechnique;
typedef interface ID3DX11EffectTechnique *LPD3D11EFFECTTECHNIQUE;

// {51198831-1F1D-4F47-BD76-41CB0835B1DE}
DEFINE_GUID(IID_ID3DX11EffectTechnique, 
            0x51198831, 0x1f1d, 0x4f47, 0xbd, 0x76, 0x41, 0xcb, 0x08, 0x35, 0xb1, 0xde);

#undef INTERFACE
#define INTERFACE ID3DX11EffectTechnique

DECLARE_INTERFACE_(ID3DX11EffectTechnique, IUnknown)
{
    // IUnknown

    // ID3DX11EffectTechnique
    STDMETHOD_(bool, IsValid)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ _Out_ D3DX11_TECHNIQUE_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    
    STDMETHOD_(ID3DX11EffectPass*, GetPassByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectPass*, GetPassByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    
    STDMETHOD(ComputeStateBlockMask)(THIS_ _Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectTechnique /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DX11_GROUP_DESC:
//
// Retrieved by ID3DX11EffectTechnique::GetDesc()
//----------------------------------------------------------------------------

struct D3DX11_GROUP_DESC
{
    LPCSTR  Name;                   // Name of this group (only nullptr if global)
    uint32_t    Techniques;         // Number of techniques contained within
    uint32_t    Annotations;        // Number of annotations on this group
};

typedef interface ID3DX11EffectGroup ID3DX11EffectGroup;
typedef interface ID3DX11EffectGroup *LPD3D11EFFECTGROUP;

// {03074acf-97de-485f-b201-cb775264afd6}
DEFINE_GUID(IID_ID3DX11EffectGroup, 
            0x03074acf, 0x97de, 0x485f, 0xb2, 0x01, 0xcb, 0x77, 0x52, 0x64, 0xaf, 0xd6);

#undef INTERFACE
#define INTERFACE ID3DX11EffectGroup

DECLARE_INTERFACE_(ID3DX11EffectGroup, IUnknown) 
{
    // IUnknown

    // ID3DX11EffectGroup
    STDMETHOD_(bool, IsValid)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ _Out_ D3DX11_GROUP_DESC *pDesc) PURE;

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(THIS_ _In_z_ LPCSTR Name) PURE;

    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByName)(THIS_ _In_z_ LPCSTR Name) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DX11Effect //////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DX11_EFFECT_DESC:
//
// Retrieved by ID3DX11Effect::GetDesc()
//----------------------------------------------------------------------------

struct D3DX11_EFFECT_DESC
{
    uint32_t    ConstantBuffers;        // Number of constant buffers in this effect
    uint32_t    GlobalVariables;        // Number of global variables in this effect
    uint32_t    InterfaceVariables;     // Number of global interfaces in this effect
    uint32_t    Techniques;             // Number of techniques in this effect
    uint32_t    Groups;                 // Number of groups in this effect
};

typedef interface ID3DX11Effect ID3DX11Effect;
typedef interface ID3DX11Effect *LPD3D11EFFECT;

// {FA61CA24-E4BA-4262-9DB8-B132E8CAE319}
DEFINE_GUID(IID_ID3DX11Effect, 
            0xfa61ca24, 0xe4ba, 0x4262, 0x9d, 0xb8, 0xb1, 0x32, 0xe8, 0xca, 0xe3, 0x19);

#undef INTERFACE
#define INTERFACE ID3DX11Effect

DECLARE_INTERFACE_(ID3DX11Effect, IUnknown)
{
    // IUnknown

    // ID3DX11Effect
    STDMETHOD_(bool, IsValid)(THIS) PURE;

    STDMETHOD(GetDevice)(THIS_ _Outptr_ ID3D11Device** ppDevice) PURE;
    
    STDMETHOD(GetDesc)(THIS_ _Out_ D3DX11_EFFECT_DESC *pDesc) PURE;
    
    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetConstantBufferByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetConstantBufferByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    
    STDMETHOD_(ID3DX11EffectVariable*, GetVariableByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetVariableByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    STDMETHOD_(ID3DX11EffectVariable*, GetVariableBySemantic)(THIS_ _In_z_ LPCSTR Semantic) PURE;
    
    STDMETHOD_(ID3DX11EffectGroup*, GetGroupByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectGroup*, GetGroupByName)(THIS_ _In_z_ LPCSTR Name) PURE;

    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByIndex)(THIS_ _In_ uint32_t Index) PURE;
    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByName)(THIS_ _In_z_ LPCSTR Name) PURE;
    
    STDMETHOD_(ID3D11ClassLinkage*, GetClassLinkage)(THIS) PURE;

    STDMETHOD(CloneEffect)(THIS_ _In_ uint32_t Flags, _Outptr_ ID3DX11Effect** ppClonedEffect ) PURE;
    STDMETHOD(Optimize)(THIS) PURE;
    STDMETHOD_(bool, IsOptimized)(THIS) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// APIs //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//----------------------------------------------------------------------------
// D3DX11CreateEffectFromMemory
//
// Creates an effect instance from a compiled effect in memory
//
// Parameters:
//
// [in]
//
//  pData
//      Blob of compiled effect data
//  DataLength
//      Length of the data blob
//  FXFlags
//      Flags pertaining to Effect creation
//  pDevice
//      Pointer to the D3D11 device on which to create Effect resources
//  srcName [optional]
//      ASCII string to use for debug object naming
//
// [out]
//
//  ppEffect
//      Address of the newly created Effect interface
//
//----------------------------------------------------------------------------

HRESULT WINAPI D3DX11CreateEffectFromMemory( _In_reads_bytes_(DataLength) LPCVOID pData,
                                             _In_ SIZE_T DataLength,
                                             _In_ UINT FXFlags,
                                             _In_ ID3D11Device *pDevice,
                                             _Outptr_ ID3DX11Effect **ppEffect,
                                             _In_opt_z_ LPCSTR srcName = nullptr );

//----------------------------------------------------------------------------
// D3DX11CreateEffectFromFile
//
// Creates an effect instance from a compiled effect data file
//
// Parameters:
//
// [in]
//
//  pFileName
//      Compiled effect file
//  FXFlags
//      Flags pertaining to Effect creation
//  pDevice
//      Pointer to the D3D11 device on which to create Effect resources
//
// [out]
//
//  ppEffect
//      Address of the newly created Effect interface
//
//----------------------------------------------------------------------------

HRESULT WINAPI D3DX11CreateEffectFromFile( _In_z_ LPCWSTR pFileName,
                                           _In_ UINT FXFlags,
                                           _In_ ID3D11Device *pDevice,
                                           _Outptr_ ID3DX11Effect **ppEffect );

//----------------------------------------------------------------------------
// D3DX11CompileEffectFromMemory
//
// Complies an effect shader source file in memory and then creates an effect instance
//
// Parameters:
//
// [in]
//
//  pData
//      Pointer to FX shader as an ASCII string
//  DataLength
//      Length of the FX shader ASCII string
//  srcName [optional]
//      ASCII string to use for debug object naming
//  pDefines [optional]
//      A NULL-terminated array of shader macros
//  pInclude [optional]
//      A pointer to an include interface
//  HLSLFlags
//     HLSL compile options (see D3DCOMPILE flags)
//  FXFlags
//      Flags pertaining to Effect compilation (see D3DCOMPILE_EFFECT flags)
//  pDevice
//      Pointer to the D3D11 device on which to create Effect resources
//
// [out]
//
//  ppEffect
//      Address of the newly created Effect interface
//
//----------------------------------------------------------------------------

HRESULT D3DX11CompileEffectFromMemory( _In_reads_bytes_(DataLength) LPCVOID pData,
                                       _In_ SIZE_T DataLength, 
                                       _In_opt_z_ LPCSTR srcName,
                                       _In_opt_ const D3D_SHADER_MACRO *pDefines,
                                       _In_opt_ ID3DInclude *pInclude,
                                       _In_ UINT HLSLFlags,
                                       _In_ UINT FXFlags,
                                       _In_ ID3D11Device *pDevice,
                                       _Out_ ID3DX11Effect **ppEffect,
                                       _Outptr_opt_result_maybenull_ ID3DBlob **ppErrors );

//----------------------------------------------------------------------------
// D3DX11CompileEffectFromFile
//
// Complies an effect shader source file and then creates an effect instance
//
// Parameters:
//
// [in]
//
//  pFileName
//      FX shader source file
//  pDefines [optional]
//      A NULL-terminated array of shader macros
//  pInclude [optional]
//      A pointer to an include interface
//  HLSLFlags
//     HLSL compile options (see D3DCOMPILE flags)
//  FXFlags
//      Flags pertaining to Effect compilation (see D3DCOMPILE_EFFECT flags)
//  pDevice
//      Pointer to the D3D11 device on which to create Effect resources
//
// [out]
//
//  ppEffect
//      Address of the newly created Effect interface
//
//----------------------------------------------------------------------------

HRESULT D3DX11CompileEffectFromFile( _In_z_ LPCWSTR pFileName,
                                     _In_opt_ const D3D_SHADER_MACRO *pDefines,
                                     _In_opt_ ID3DInclude *pInclude,
                                     _In_ UINT HLSLFlags,
                                     _In_ UINT FXFlags,
                                     _In_ ID3D11Device *pDevice,
                                     _Out_ ID3DX11Effect **ppEffect,
                                     _Outptr_opt_result_maybenull_ ID3DBlob **ppErrors );


//----------------------------------------------------------------------------
// D3DX11DebugMute
//
// Controls the output of diagnostic information in DEBUG builds. No effect
// in RELEASE builds.
//
// Returns the previous state so you can do temporary suppression like:
//
//    bool oldmute = D3DX11DebugMute(true);
//    ...
//    D3DX11DebugMute(oldmute);
//
//----------------------------------------------------------------------------

bool D3DX11DebugMute(bool mute);

#ifdef __cplusplus
}
#endif //__cplusplus

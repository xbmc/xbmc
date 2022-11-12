//--------------------------------------------------------------------------------------
// File: EffectVariable.inl
//
// Direct3D 11 Effects Variable reflection template
// These templates define the many Effect variable types.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4127)

//////////////////////////////////////////////////////////////////////////
// Invalid variable forward defines
//////////////////////////////////////////////////////////////////////////

struct SEffectInvalidScalarVariable;
struct SEffectInvalidVectorVariable;
struct SEffectInvalidMatrixVariable;
struct SEffectInvalidStringVariable;
struct SEffectInvalidClassInstanceVariable;
struct SEffectInvalidInterfaceVariable;
struct SEffectInvalidShaderResourceVariable;
struct SEffectInvalidUnorderedAccessViewVariable;
struct SEffectInvalidRenderTargetViewVariable;
struct SEffectInvalidDepthStencilViewVariable;
struct SEffectInvalidConstantBuffer;
struct SEffectInvalidShaderVariable;
struct SEffectInvalidBlendVariable;
struct SEffectInvalidDepthStencilVariable;
struct SEffectInvalidRasterizerVariable;
struct SEffectInvalidSamplerVariable;
struct SEffectInvalidTechnique;
struct SEffectInvalidPass;
struct SEffectInvalidType;

extern SEffectInvalidScalarVariable g_InvalidScalarVariable;
extern SEffectInvalidVectorVariable g_InvalidVectorVariable;
extern SEffectInvalidMatrixVariable g_InvalidMatrixVariable;
extern SEffectInvalidStringVariable g_InvalidStringVariable;
extern SEffectInvalidClassInstanceVariable g_InvalidClassInstanceVariable;
extern SEffectInvalidInterfaceVariable g_InvalidInterfaceVariable;
extern SEffectInvalidShaderResourceVariable g_InvalidShaderResourceVariable;
extern SEffectInvalidUnorderedAccessViewVariable g_InvalidUnorderedAccessViewVariable;
extern SEffectInvalidRenderTargetViewVariable g_InvalidRenderTargetViewVariable;
extern SEffectInvalidDepthStencilViewVariable g_InvalidDepthStencilViewVariable;
extern SEffectInvalidConstantBuffer g_InvalidConstantBuffer;
extern SEffectInvalidShaderVariable g_InvalidShaderVariable;
extern SEffectInvalidBlendVariable g_InvalidBlendVariable;
extern SEffectInvalidDepthStencilVariable g_InvalidDepthStencilVariable;
extern SEffectInvalidRasterizerVariable g_InvalidRasterizerVariable;
extern SEffectInvalidSamplerVariable g_InvalidSamplerVariable;
extern SEffectInvalidTechnique g_InvalidTechnique;
extern SEffectInvalidPass g_InvalidPass;
extern SEffectInvalidType g_InvalidType;

enum ETemplateVarType
{
    ETVT_Bool,
    ETVT_Int,
    ETVT_Float,
    ETVT_bool
};

//////////////////////////////////////////////////////////////////////////
// Invalid effect variable struct definitions
//////////////////////////////////////////////////////////////////////////

struct SEffectInvalidType : public ID3DX11EffectType
{
    STDMETHOD_(bool, IsValid)() override { return false; }
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidType; }
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidType; }
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeBySemantic)(_In_z_ LPCSTR Semantic) override { UNREFERENCED_PARAMETER(Semantic); return &g_InvalidType; }
    STDMETHOD_(LPCSTR, GetMemberName)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return nullptr; }
    STDMETHOD_(LPCSTR, GetMemberSemantic)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return nullptr; }
    IUNKNOWN_IMP(SEffectInvalidType, ID3DX11EffectType, IUnknown);
};

template<typename IBaseInterface>
struct TEffectInvalidVariable : public IBaseInterface
{
public:
    STDMETHOD_(bool, IsValid)() override { return false; }
    STDMETHOD_(ID3DX11EffectType*, GetType)() override { return &g_InvalidType; }
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidScalarVariable; }
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidScalarVariable; }

    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidScalarVariable; }
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidScalarVariable; }
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberBySemantic)(_In_z_ LPCSTR Semantic) override { UNREFERENCED_PARAMETER(Semantic); return &g_InvalidScalarVariable; }

    STDMETHOD_(ID3DX11EffectVariable*, GetElement)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidScalarVariable; }

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetParentConstantBuffer)() override { return &g_InvalidConstantBuffer; }

    STDMETHOD_(ID3DX11EffectScalarVariable*, AsScalar)() override { return &g_InvalidScalarVariable; }
    STDMETHOD_(ID3DX11EffectVectorVariable*, AsVector)() override { return &g_InvalidVectorVariable; }
    STDMETHOD_(ID3DX11EffectMatrixVariable*, AsMatrix)() override { return &g_InvalidMatrixVariable; }
    STDMETHOD_(ID3DX11EffectStringVariable*, AsString)() override { return &g_InvalidStringVariable; }
    STDMETHOD_(ID3DX11EffectClassInstanceVariable*, AsClassInstance)() override { return &g_InvalidClassInstanceVariable; }
    STDMETHOD_(ID3DX11EffectInterfaceVariable*, AsInterface)() override { return &g_InvalidInterfaceVariable; }
    STDMETHOD_(ID3DX11EffectShaderResourceVariable*, AsShaderResource)() override { return &g_InvalidShaderResourceVariable; }
    STDMETHOD_(ID3DX11EffectUnorderedAccessViewVariable*, AsUnorderedAccessView)() override { return &g_InvalidUnorderedAccessViewVariable; }
    STDMETHOD_(ID3DX11EffectRenderTargetViewVariable*, AsRenderTargetView)() override { return &g_InvalidRenderTargetViewVariable; }
    STDMETHOD_(ID3DX11EffectDepthStencilViewVariable*, AsDepthStencilView)() override { return &g_InvalidDepthStencilViewVariable; }
    STDMETHOD_(ID3DX11EffectConstantBuffer*, AsConstantBuffer)() override { return &g_InvalidConstantBuffer; }
    STDMETHOD_(ID3DX11EffectShaderVariable*, AsShader)() override { return &g_InvalidShaderVariable; }
    STDMETHOD_(ID3DX11EffectBlendVariable*, AsBlend)() override { return &g_InvalidBlendVariable; }
    STDMETHOD_(ID3DX11EffectDepthStencilVariable*, AsDepthStencil)() override { return &g_InvalidDepthStencilVariable; }
    STDMETHOD_(ID3DX11EffectRasterizerVariable*, AsRasterizer)() override { return &g_InvalidRasterizerVariable; }
    STDMETHOD_(ID3DX11EffectSamplerVariable*, AsSampler)() override { return &g_InvalidSamplerVariable; }

    STDMETHOD(SetRawValue)(_In_reads_bytes_(Count) const void *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetRawValue)(_Out_writes_bytes_(Count) void *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
};

struct SEffectInvalidScalarVariable : public TEffectInvalidVariable<ID3DX11EffectScalarVariable>
{
public:

    STDMETHOD(SetFloat)(_In_ const float Value) override { UNREFERENCED_PARAMETER(Value); return E_FAIL; }
    STDMETHOD(GetFloat)(_Out_ float *pValue) override { UNREFERENCED_PARAMETER(pValue); return E_FAIL; }

    STDMETHOD(SetFloatArray)(_In_reads_(Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetFloatArray)(_Out_writes_(Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    STDMETHOD(SetInt)(_In_ const int Value) override { UNREFERENCED_PARAMETER(Value); return E_FAIL; }
    STDMETHOD(GetInt)(_Out_ int *pValue) override { UNREFERENCED_PARAMETER(pValue); return E_FAIL; }

    STDMETHOD(SetIntArray)(_In_reads_(Count) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetIntArray)(_Out_writes_(Count) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    STDMETHOD(SetBool)(_In_ const bool Value) override { UNREFERENCED_PARAMETER(Value); return E_FAIL; }
    STDMETHOD(GetBool)(_Out_ bool *pValue) override { UNREFERENCED_PARAMETER(pValue); return E_FAIL; }

    STDMETHOD(SetBoolArray)(_In_reads_(Count) const bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetBoolArray)(_Out_writes_(Count) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidScalarVariable, ID3DX11EffectScalarVariable, ID3DX11EffectVariable);
};


struct SEffectInvalidVectorVariable : public TEffectInvalidVariable<ID3DX11EffectVectorVariable>
{
public:
    STDMETHOD(SetFloatVector)(_In_reads_(4) const float *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; };
    STDMETHOD(SetIntVector)(_In_reads_(4) const int *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; };
    STDMETHOD(SetBoolVector)(_In_reads_(4) const bool *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; };

    STDMETHOD(GetFloatVector)(_Out_writes_(4) float *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; };
    STDMETHOD(GetIntVector)(_Out_writes_(4) int *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; };
    STDMETHOD(GetBoolVector)(_Out_writes_(4) bool *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; };

    STDMETHOD(SetBoolVectorArray) (_In_reads_(4*Count) const bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; };
    STDMETHOD(SetIntVectorArray)  (_In_reads_(4*Count) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; };
    STDMETHOD(SetFloatVectorArray)(_In_reads_(4*Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; };

    STDMETHOD(GetBoolVectorArray) (_Out_writes_(4*Count) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; };
    STDMETHOD(GetIntVectorArray)  (_Out_writes_(4*Count) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; };
    STDMETHOD(GetFloatVectorArray)(_Out_writes_(4*Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; };

    IUNKNOWN_IMP(SEffectInvalidVectorVariable, ID3DX11EffectVectorVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidMatrixVariable : public TEffectInvalidVariable<ID3DX11EffectMatrixVariable>
{
public:

    STDMETHOD(SetMatrix)(_In_reads_(16) const float *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; }
    STDMETHOD(GetMatrix)(_Out_writes_(16) float *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; }

    STDMETHOD(SetMatrixArray)(_In_reads_(16*Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetMatrixArray)(_Out_writes_(16*Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    STDMETHOD(SetMatrixPointerArray)(_In_reads_(16*Count) const float **ppData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetMatrixPointerArray)(_Out_writes_(16*Count) float **ppData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    STDMETHOD(SetMatrixTranspose)(_In_reads_(16) const float *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; }
    STDMETHOD(GetMatrixTranspose)(_Out_writes_(16) float *pData) override { UNREFERENCED_PARAMETER(pData); return E_FAIL; }

    STDMETHOD(SetMatrixTransposeArray)(_In_reads_(16*Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetMatrixTransposeArray)(_Out_writes_(16*Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    STDMETHOD(SetMatrixTransposePointerArray)(_In_reads_(16*Count) const float **ppData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetMatrixTransposePointerArray)(_Out_writes_(16*Count) float **ppData, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidMatrixVariable, ID3DX11EffectMatrixVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidStringVariable : public TEffectInvalidVariable<ID3DX11EffectStringVariable>
{
public:

    STDMETHOD(GetString)(_Outptr_result_z_ LPCSTR *ppString) override { UNREFERENCED_PARAMETER(ppString); return E_FAIL; }
    STDMETHOD(GetStringArray)(_Out_writes_(Count) LPCSTR *ppStrings, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppStrings); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidStringVariable, ID3DX11EffectStringVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidClassInstanceVariable : public TEffectInvalidVariable<ID3DX11EffectClassInstanceVariable>
{
public:

    STDMETHOD(GetClassInstance)(_Outptr_ ID3D11ClassInstance **ppClassInstance) override { UNREFERENCED_PARAMETER(ppClassInstance); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidClassInstanceVariable, ID3DX11EffectClassInstanceVariable, ID3DX11EffectVariable);
};


struct SEffectInvalidInterfaceVariable : public TEffectInvalidVariable<ID3DX11EffectInterfaceVariable>
{
public:

    STDMETHOD(SetClassInstance)(_In_ ID3DX11EffectClassInstanceVariable *pEffectClassInstance) override
        { UNREFERENCED_PARAMETER(pEffectClassInstance); return E_FAIL; }
    STDMETHOD(GetClassInstance)(_Outptr_ ID3DX11EffectClassInstanceVariable **ppEffectClassInstance) override
        { UNREFERENCED_PARAMETER(ppEffectClassInstance); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidInterfaceVariable, ID3DX11EffectInterfaceVariable, ID3DX11EffectVariable);
};


struct SEffectInvalidShaderResourceVariable : public TEffectInvalidVariable<ID3DX11EffectShaderResourceVariable>
{
public:

    STDMETHOD(SetResource)(_In_ ID3D11ShaderResourceView *pResource) override { UNREFERENCED_PARAMETER(pResource); return E_FAIL; }
    STDMETHOD(GetResource)(_Outptr_ ID3D11ShaderResourceView **ppResource) override { UNREFERENCED_PARAMETER(ppResource); return E_FAIL; }

    STDMETHOD(SetResourceArray)(_In_reads_(Count) ID3D11ShaderResourceView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetResourceArray)(_Out_writes_(Count) ID3D11ShaderResourceView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidShaderResourceVariable, ID3DX11EffectShaderResourceVariable, ID3DX11EffectVariable);
};


struct SEffectInvalidUnorderedAccessViewVariable : public TEffectInvalidVariable<ID3DX11EffectUnorderedAccessViewVariable>
{
public:

    STDMETHOD(SetUnorderedAccessView)(_In_ ID3D11UnorderedAccessView *pResource) override { UNREFERENCED_PARAMETER(pResource); return E_FAIL; }
    STDMETHOD(GetUnorderedAccessView)(_Outptr_ ID3D11UnorderedAccessView **ppResource) override { UNREFERENCED_PARAMETER(ppResource); return E_FAIL; }

    STDMETHOD(SetUnorderedAccessViewArray)(_In_reads_(Count) ID3D11UnorderedAccessView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetUnorderedAccessViewArray)(_Out_writes_(Count) ID3D11UnorderedAccessView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidUnorderedAccessViewVariable, ID3DX11EffectUnorderedAccessViewVariable, ID3DX11EffectVariable);
};


struct SEffectInvalidRenderTargetViewVariable : public TEffectInvalidVariable<ID3DX11EffectRenderTargetViewVariable>
{
public:

    STDMETHOD(SetRenderTarget)(_In_ ID3D11RenderTargetView *pResource) override { UNREFERENCED_PARAMETER(pResource); return E_FAIL; }
    STDMETHOD(GetRenderTarget)(_Outptr_ ID3D11RenderTargetView **ppResource) override { UNREFERENCED_PARAMETER(ppResource); return E_FAIL; }

    STDMETHOD(SetRenderTargetArray)(_In_reads_(Count) ID3D11RenderTargetView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetRenderTargetArray)(_Out_writes_(Count) ID3D11RenderTargetView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidRenderTargetViewVariable, ID3DX11EffectRenderTargetViewVariable, ID3DX11EffectVariable);
};


struct SEffectInvalidDepthStencilViewVariable : public TEffectInvalidVariable<ID3DX11EffectDepthStencilViewVariable>
{
public:

    STDMETHOD(SetDepthStencil)(_In_ ID3D11DepthStencilView *pResource) override { UNREFERENCED_PARAMETER(pResource); return E_FAIL; }
    STDMETHOD(GetDepthStencil)(_Outptr_ ID3D11DepthStencilView **ppResource) override { UNREFERENCED_PARAMETER(ppResource); return E_FAIL; }

    STDMETHOD(SetDepthStencilArray)(_In_reads_(Count) ID3D11DepthStencilView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }
    STDMETHOD(GetDepthStencilArray)(_Out_writes_(Count) ID3D11DepthStencilView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override
        { UNREFERENCED_PARAMETER(ppResources); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidDepthStencilViewVariable, ID3DX11EffectDepthStencilViewVariable, ID3DX11EffectVariable);
};


struct SEffectInvalidConstantBuffer : public TEffectInvalidVariable<ID3DX11EffectConstantBuffer>
{
public:

    STDMETHOD(SetConstantBuffer)(_In_ ID3D11Buffer *pConstantBuffer) override { UNREFERENCED_PARAMETER(pConstantBuffer); return E_FAIL; }
    STDMETHOD(GetConstantBuffer)(_Outptr_ ID3D11Buffer **ppConstantBuffer) override { UNREFERENCED_PARAMETER(ppConstantBuffer); return E_FAIL; }
    STDMETHOD(UndoSetConstantBuffer)() override { return E_FAIL; }

    STDMETHOD(SetTextureBuffer)(_In_ ID3D11ShaderResourceView *pTextureBuffer) override { UNREFERENCED_PARAMETER(pTextureBuffer); return E_FAIL; }
    STDMETHOD(GetTextureBuffer)(_Outptr_ ID3D11ShaderResourceView **ppTextureBuffer) override { UNREFERENCED_PARAMETER(ppTextureBuffer); return E_FAIL; }
    STDMETHOD(UndoSetTextureBuffer)() override { return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidConstantBuffer, ID3DX11EffectConstantBuffer, ID3DX11EffectVariable);
};

struct SEffectInvalidShaderVariable : public TEffectInvalidVariable<ID3DX11EffectShaderVariable>
{
public:

    STDMETHOD(GetShaderDesc)(_In_ uint32_t ShaderIndex, _Out_ D3DX11_EFFECT_SHADER_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    STDMETHOD(GetVertexShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11VertexShader **ppVS) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(ppVS); return E_FAIL; }
    STDMETHOD(GetGeometryShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11GeometryShader **ppGS) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(ppGS); return E_FAIL; }
    STDMETHOD(GetPixelShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11PixelShader **ppPS) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(ppPS); return E_FAIL; }
    STDMETHOD(GetHullShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11HullShader **ppHS) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(ppHS); return E_FAIL; }
    STDMETHOD(GetDomainShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11DomainShader **ppDS) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(ppDS); return E_FAIL; }
    STDMETHOD(GetComputeShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11ComputeShader **ppCS) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(ppCS); return E_FAIL; }

    STDMETHOD(GetInputSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(Element); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD(GetOutputSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(Element); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD(GetPatchConstantSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(ShaderIndex); UNREFERENCED_PARAMETER(Element); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidShaderVariable, ID3DX11EffectShaderVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidBlendVariable : public TEffectInvalidVariable<ID3DX11EffectBlendVariable>
{
public:

    STDMETHOD(GetBlendState)(_In_ uint32_t Index, _Outptr_ ID3D11BlendState **ppState) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(ppState); return E_FAIL; }
    STDMETHOD(SetBlendState)(_In_ uint32_t Index, _In_ ID3D11BlendState *pState) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pState); return E_FAIL; }
    STDMETHOD(UndoSetBlendState)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return E_FAIL; }
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_BLEND_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidBlendVariable, ID3DX11EffectBlendVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidDepthStencilVariable : public TEffectInvalidVariable<ID3DX11EffectDepthStencilVariable>
{
public:

    STDMETHOD(GetDepthStencilState)(_In_ uint32_t Index, _Outptr_ ID3D11DepthStencilState **ppState) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(ppState); return E_FAIL; }
    STDMETHOD(SetDepthStencilState)(_In_ uint32_t Index, _In_ ID3D11DepthStencilState *pState) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pState); return E_FAIL; }
    STDMETHOD(UndoSetDepthStencilState)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return E_FAIL; }
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_DEPTH_STENCIL_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidDepthStencilVariable, ID3DX11EffectDepthStencilVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidRasterizerVariable : public TEffectInvalidVariable<ID3DX11EffectRasterizerVariable>
{
public:

    STDMETHOD(GetRasterizerState)(_In_ uint32_t Index, _Outptr_ ID3D11RasterizerState **ppState) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(ppState); return E_FAIL; }
    STDMETHOD(SetRasterizerState)(_In_ uint32_t Index, _In_ ID3D11RasterizerState *pState) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pState); return E_FAIL; }
    STDMETHOD(UndoSetRasterizerState)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return E_FAIL; }
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_RASTERIZER_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidRasterizerVariable, ID3DX11EffectRasterizerVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidSamplerVariable : public TEffectInvalidVariable<ID3DX11EffectSamplerVariable>
{
public:

    STDMETHOD(GetSampler)(_In_ uint32_t Index, _Outptr_ ID3D11SamplerState **ppSampler) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(ppSampler); return E_FAIL; }
    STDMETHOD(SetSampler)(_In_ uint32_t Index, _In_ ID3D11SamplerState *pSampler) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pSampler); return E_FAIL; }
    STDMETHOD(UndoSetSampler)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return E_FAIL; }
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_SAMPLER_DESC *pDesc) override
        { UNREFERENCED_PARAMETER(Index); UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidSamplerVariable, ID3DX11EffectSamplerVariable, ID3DX11EffectVariable);
};

struct SEffectInvalidPass : public ID3DX11EffectPass
{
public:
    STDMETHOD_(bool, IsValid)() override { return false; }
    STDMETHOD(GetDesc)(_Out_ D3DX11_PASS_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    STDMETHOD(GetVertexShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD(GetGeometryShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD(GetPixelShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD(GetHullShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD(GetDomainShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }
    STDMETHOD(GetComputeShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidScalarVariable; }
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidScalarVariable; }

    STDMETHOD(Apply)(_In_ uint32_t Flags, _In_ ID3D11DeviceContext* pContext) override
        { UNREFERENCED_PARAMETER(Flags); UNREFERENCED_PARAMETER(pContext); return E_FAIL; }
    STDMETHOD(ComputeStateBlockMask)(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask) override { UNREFERENCED_PARAMETER(pStateBlockMask); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidPass, ID3DX11EffectPass, IUnknown);
};

struct SEffectInvalidTechnique : public ID3DX11EffectTechnique
{
public:
    STDMETHOD_(bool, IsValid)() override { return false; }
    STDMETHOD(GetDesc)(_Out_ D3DX11_TECHNIQUE_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidScalarVariable; }
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidScalarVariable; }

    STDMETHOD_(ID3DX11EffectPass*, GetPassByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidPass; }
    STDMETHOD_(ID3DX11EffectPass*, GetPassByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidPass; }

    STDMETHOD(ComputeStateBlockMask)(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask) override { UNREFERENCED_PARAMETER(pStateBlockMask); return E_FAIL; }

    IUNKNOWN_IMP(SEffectInvalidTechnique, ID3DX11EffectTechnique, IUnknown);
};

struct SEffectInvalidGroup : public ID3DX11EffectGroup
{
public:
    STDMETHOD_(bool, IsValid)() override { return false; }
    STDMETHOD(GetDesc)(_Out_ D3DX11_GROUP_DESC *pDesc) override { UNREFERENCED_PARAMETER(pDesc); return E_FAIL; }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidScalarVariable; }
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidScalarVariable; }

    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByIndex)(_In_ uint32_t Index) override { UNREFERENCED_PARAMETER(Index); return &g_InvalidTechnique; }
    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByName)(_In_z_ LPCSTR Name) override { UNREFERENCED_PARAMETER(Name); return &g_InvalidTechnique; }

    IUNKNOWN_IMP(SEffectInvalidGroup, ID3DX11EffectGroup, IUnknown);
};

//////////////////////////////////////////////////////////////////////////
// Helper routines
//////////////////////////////////////////////////////////////////////////

// This is an annoying warning that pops up in retail builds because 
// the code that jumps to "lExit" is conditionally not compiled.
// The only alternative is more #ifdefs in every function
#pragma warning( disable : 4102 ) // 'label' : unreferenced label

#define VERIFYPARAMETER(x) \
{ if (!(x)) { DPF(0, "%s: Parameter " #x " was nullptr.", pFuncName); \
    __BREAK_ON_FAIL; hr = E_INVALIDARG; goto lExit; } }

static HRESULT AnnotationInvalidSetCall(LPCSTR pFuncName)
{
    DPF(0, "%s: Annotations are readonly", pFuncName);
    return D3DERR_INVALIDCALL;
}

static HRESULT ObjectSetRawValue()
{
    DPF(0, "ID3DX11EffectVariable::SetRawValue: Objects do not support ths call; please use the specific object accessors instead.");
    return D3DERR_INVALIDCALL;
}

static HRESULT ObjectGetRawValue()
{
    DPF(0, "ID3DX11EffectVariable::GetRawValue: Objects do not support ths call; please use the specific object accessors instead.");
    return D3DERR_INVALIDCALL;
}

ID3DX11EffectConstantBuffer * NoParentCB();

ID3DX11EffectVariable * GetAnnotationByIndexHelper(_In_z_ const char *pClassName, _In_ uint32_t Index,
                                                   _In_ uint32_t  AnnotationCount, _In_reads_(AnnotationCount) SAnnotation *pAnnotations);

ID3DX11EffectVariable * GetAnnotationByNameHelper(_In_z_ const char *pClassName, _In_z_ LPCSTR Name,
                                                   _In_ uint32_t  AnnotationCount, _In_reads_(AnnotationCount) SAnnotation *pAnnotations);

template<typename SVarType>
_Success_(return)
bool GetVariableByIndexHelper(_In_ uint32_t Index, _In_ uint32_t  VariableCount, _In_reads_(VariableCount) SVarType *pVariables, 
                              _In_opt_ uint8_t *pBaseAddress, _Outptr_ SVarType **ppMember, _Outptr_ void **ppDataPtr)
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::GetMemberByIndex";

    if (Index >= VariableCount)
    {
        DPF(0, "%s: Invalid index (%u, total: %u)", pFuncName, Index, VariableCount);
        return false;
    }

    *ppMember = pVariables + Index;
    *ppDataPtr = pBaseAddress + (*ppMember)->Data.Offset;
    return true;
}

template<typename SVarType>
_Success_(return)
bool GetVariableByNameHelper(_In_z_ LPCSTR Name, _In_ uint32_t  VariableCount, _In_reads_(VariableCount) SVarType *pVariables, 
                             _In_opt_ uint8_t *pBaseAddress, _Outptr_ SVarType **ppMember, _Outptr_ void **ppDataPtr, _Out_ uint32_t* pIndex)
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::GetMemberByName";

    if (nullptr == Name)
    {
        DPF(0, "%s: Parameter Name was nullptr.", pFuncName);
        return false;
    }

    bool bHasSuper = false;

    for (uint32_t i = 0; i < VariableCount; ++ i)
    {
        *ppMember = pVariables + i;
        assert((*ppMember)->pName != 0);
        _Analysis_assume_((*ppMember)->pName != 0);
        if (strcmp((*ppMember)->pName, Name) == 0)
        {
            *ppDataPtr = pBaseAddress + (*ppMember)->Data.Offset;
            *pIndex = i;
            return true;
        }
        else if (i == 0 &&
                 (*ppMember)->pName[0] == '$' &&
                 strcmp((*ppMember)->pName, "$super") == 0)
        {
            bHasSuper = true;
        }
    }

    if (bHasSuper)
    {
        SVarType* pSuper = pVariables;

        return GetVariableByNameHelper<SVarType>(Name,
                                                 pSuper->pType->StructType.Members,
                                                 (SVarType*)pSuper->pType->StructType.pMembers,
                                                 pBaseAddress + pSuper->Data.Offset,
                                                 ppMember,
                                                 ppDataPtr,
                                                 pIndex);
    }

    DPF(0, "%s: Variable [%s] not found", pFuncName, Name);
    return false;
}

template<typename SVarType>
_Success_(return)
bool GetVariableBySemanticHelper(_In_z_ LPCSTR Semantic, _In_ uint32_t  VariableCount, _In_reads_(VariableCount) SVarType *pVariables, 
                                 _In_opt_ uint8_t *pBaseAddress, _Outptr_ SVarType **ppMember, _Outptr_ void **ppDataPtr, _Out_ uint32_t* pIndex)
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::GetMemberBySemantic";

    if (nullptr == Semantic)
    {
        DPF(0, "%s: Parameter Semantic was nullptr.", pFuncName);
        return false;
    }

    for (uint32_t i = 0; i < VariableCount; ++ i)
    {
        *ppMember = pVariables + i;
        if (nullptr != (*ppMember)->pSemantic &&
            _stricmp((*ppMember)->pSemantic, Semantic) == 0)
        {
            *ppDataPtr = pBaseAddress + (*ppMember)->Data.Offset;
            *pIndex = i;
            return true;
        }
    }

    DPF(0, "%s: Variable with semantic [%s] not found", pFuncName, Semantic);
    return false;
}

inline bool AreBoundsValid(_In_ uint32_t Offset, _In_ uint32_t Count, _In_ const void *pData, _In_ const SType *pType, _In_ uint32_t  TotalUnpackedSize)
{
    if (Count == 0) return true;
    uint32_t  singleElementSize = pType->GetTotalUnpackedSize(true);
    assert(singleElementSize <= pType->Stride);

    return ((Offset + Count >= Offset) &&
        ((Offset + Count) < ((uint32_t)-1) / pType->Stride) &&
        (Count * pType->Stride + (uint8_t*)pData >= (uint8_t*)pData) &&
        ((Offset + Count - 1) * pType->Stride + singleElementSize <= TotalUnpackedSize));
}

// Note that the branches in this code is based on template parameters and will be compiled out
template<ETemplateVarType SourceType, ETemplateVarType DestType, typename SRC_TYPE, bool ValidatePtr>
__forceinline HRESULT CopyScalarValue(_In_ SRC_TYPE SrcValue, _Out_ void *pDest, _In_z_ const char *pFuncName)
{
    HRESULT hr = S_OK;
#ifdef _DEBUG
    if (ValidatePtr)
        VERIFYPARAMETER(pDest);
#else
    UNREFERENCED_PARAMETER(pFuncName);
#endif

    switch (SourceType)
    {
    case ETVT_Bool:
        switch (DestType)
        {
        case ETVT_Bool:
            *(int*)pDest = (SrcValue != 0) ? -1 : 0;
            break;

        case ETVT_Int:
            *(int*)pDest = SrcValue ? 1 : 0;
            break;

        case ETVT_Float:
            *(float*)pDest = SrcValue ? 1.0f : 0.0f;
            break;

        case ETVT_bool:
            *(bool*)pDest = (SrcValue != 0) ? true : false;
            break;

        default:
            assert(0);
        }
        break;

    case ETVT_Int:
        switch (DestType)
        {
        case ETVT_Bool:
            *(int*)pDest = (SrcValue != 0) ? -1 : 0;
            break;

        case ETVT_Int:
            *(int*)pDest = (int) SrcValue;
            break;

        case ETVT_Float:
            *(float*)pDest = (float)(SrcValue);
            break;

        case ETVT_bool:
            *(bool*)pDest = (SrcValue != 0) ? true : false;
            break;

        default:
            assert(0);
        }
        break;

    case ETVT_Float:
        switch (DestType)
        {
        case ETVT_Bool:
            *(int*)pDest = (SrcValue != 0.0f) ? -1 : 0;
            break;

        case ETVT_Int:
            *(int*)pDest = (int) (SrcValue);
            break;

        case ETVT_Float:
            *(float*)pDest = (float) SrcValue;
            break;

        case ETVT_bool:
            *(bool*)pDest = (SrcValue != 0.0f) ? true : false;
            break;

        default:
            assert(0);
        }
        break;

    case ETVT_bool:
        switch (DestType)
        {
        case ETVT_Bool:
            *(int*)pDest = SrcValue ? -1 : 0;
            break;

        case ETVT_Int:
            *(int*)pDest = SrcValue ? 1 : 0;
            break;

        case ETVT_Float:
            *(float*)pDest = SrcValue ? 1.0f : 0.0f;
            break;

        case ETVT_bool:
            *(bool*)pDest = (SrcValue != 0) ? true : false;
            break;

        default:
            assert(0);
        }
        break;

    default:
        assert(0);
    }

lExit:
    return hr;
}

#pragma warning(push)
#pragma warning( disable : 6103 )
template<ETemplateVarType SourceType, ETemplateVarType DestType, typename SRC_TYPE, typename DEST_TYPE>
inline HRESULT SetScalarArray(_In_reads_(Count) const SRC_TYPE *pSrcValues, _Out_writes_(Count) DEST_TYPE *pDestValues,
                              _In_ uint32_t Offset, _In_ uint32_t Count, 
                              _In_ const SType *pType, _In_ uint32_t TotalUnpackedSize, _In_z_ const char *pFuncName)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG    
    VERIFYPARAMETER(pSrcValues);

#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pSrcValues, pType, TotalUnpackedSize))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#else
    UNREFERENCED_PARAMETER(TotalUnpackedSize);
    UNREFERENCED_PARAMETER(pFuncName);
#endif

    uint32_t i, j, delta = pType->NumericType.IsPackedArray ? 1 : SType::c_ScalarsPerRegister;
    pDestValues += Offset * delta;
    for (i = 0, j = 0; j < Count; i += delta, ++ j)
    {
        // pDestValues[i] = (DEST_TYPE)pSrcValues[j];
        CopyScalarValue<SourceType, DestType, SRC_TYPE, false>(pSrcValues[j], &pDestValues[i], "SetScalarArray");
    }

lExit:
    return hr;
}
#pragma warning(pop)

#pragma warning( disable : 6103 )
template<ETemplateVarType SourceType, ETemplateVarType DestType, typename SRC_TYPE, typename DEST_TYPE>
inline HRESULT GetScalarArray(_In_reads_(Count) SRC_TYPE *pSrcValues, _Out_writes_(Count) DEST_TYPE *pDestValues,
                              _In_ uint32_t Offset, _In_ uint32_t Count, 
                              _In_ const SType *pType, _In_ uint32_t  TotalUnpackedSize, _In_z_ const char *pFuncName)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG    
    VERIFYPARAMETER(pDestValues);

#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pDestValues, pType, TotalUnpackedSize))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#else
    UNREFERENCED_PARAMETER(TotalUnpackedSize);
    UNREFERENCED_PARAMETER(pFuncName);
#endif

    uint32_t i, j, delta = pType->NumericType.IsPackedArray ? 1 : SType::c_ScalarsPerRegister;
    pSrcValues += Offset * delta;
    for (i = 0, j = 0; j < Count; i += delta, ++ j)
    {
        // pDestValues[j] = (DEST_TYPE)pSrcValues[i];
        CopyScalarValue<SourceType, DestType, SRC_TYPE, false>(pSrcValues[i], &pDestValues[j], "GetScalarArray");
    }

lExit:
    return hr;
}


//////////////////////////////////////////////////////////////////////////
// TVariable - implements type casting and member/element retrieval
//////////////////////////////////////////////////////////////////////////

// requires that IBaseInterface contain SVariable's fields and support ID3DX11EffectVariable
template<typename IBaseInterface>
struct TVariable : public IBaseInterface
{
    STDMETHOD_(bool, IsValid)() override { return true; }

    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByIndex)(_In_ uint32_t Index)
    {
        SVariable *pMember;
        UDataPointer dataPtr;
        TTopLevelVariable<ID3DX11EffectVariable> *pTopLevelEntity2 = GetTopLevelEntity();

        if (((ID3DX11Effect*)pTopLevelEntity2->pEffect)->IsOptimized())
        {
            DPF(0, "ID3DX11EffectVariable::GetMemberByIndex: Cannot get members; effect has been Optimize()'ed");
            return &g_InvalidScalarVariable;
        }

        if (pType->VarType != EVT_Struct)
        {
            DPF(0, "ID3DX11EffectVariable::GetMemberByIndex: Variable is not a structure");
            return &g_InvalidScalarVariable;
        }

        if (!GetVariableByIndexHelper<SVariable>(Index, pType->StructType.Members, pType->StructType.pMembers, 
            Data.pNumeric, &pMember, &dataPtr.pGeneric))
        {
            return &g_InvalidScalarVariable;
        }

        return pTopLevelEntity2->pEffect->CreatePooledVariableMemberInterface(pTopLevelEntity2, pMember, dataPtr, false, Index);
    }

    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByName)(_In_z_ LPCSTR Name)
    {
        SVariable *pMember;
        UDataPointer dataPtr;
        uint32_t index;
        TTopLevelVariable<ID3DX11EffectVariable> *pTopLevelEntity2 = GetTopLevelEntity();

        if (pTopLevelEntity2->pEffect->IsOptimized())
        {
            DPF(0, "ID3DX11EffectVariable::GetMemberByName: Cannot get members; effect has been Optimize()'ed");
            return &g_InvalidScalarVariable;
        }

        if (pType->VarType != EVT_Struct)
        {
            DPF(0, "ID3DX11EffectVariable::GetMemberByName: Variable is not a structure");
            return &g_InvalidScalarVariable;
        }

        if (!GetVariableByNameHelper<SVariable>(Name, pType->StructType.Members, pType->StructType.pMembers, 
            Data.pNumeric, &pMember, &dataPtr.pGeneric, &index))
        {
            return &g_InvalidScalarVariable;

        }

        return pTopLevelEntity2->pEffect->CreatePooledVariableMemberInterface(pTopLevelEntity2, pMember, dataPtr, false, index);
    }

    STDMETHOD_(ID3DX11EffectVariable*, GetMemberBySemantic)(_In_z_ LPCSTR Semantic)
    {
        SVariable *pMember;
        UDataPointer dataPtr;
        uint32_t index;
        TTopLevelVariable<ID3DX11EffectVariable> *pTopLevelEntity2 = GetTopLevelEntity();

        if (pTopLevelEntity2->pEffect->IsOptimized())
        {
            DPF(0, "ID3DX11EffectVariable::GetMemberBySemantic: Cannot get members; effect has been Optimize()'ed");
            return &g_InvalidScalarVariable;
        }

        if (pType->VarType != EVT_Struct)
        {
            DPF(0, "ID3DX11EffectVariable::GetMemberBySemantic: Variable is not a structure");
            return &g_InvalidScalarVariable;
        }

        if (!GetVariableBySemanticHelper<SVariable>(Semantic, pType->StructType.Members, pType->StructType.pMembers, 
            Data.pNumeric, &pMember, &dataPtr.pGeneric, &index))
        {
            return &g_InvalidScalarVariable;

        }

        return pTopLevelEntity2->pEffect->CreatePooledVariableMemberInterface(pTopLevelEntity2, pMember, dataPtr, false, index);
    }

    STDMETHOD_(ID3DX11EffectVariable*, GetElement)(_In_ uint32_t Index)
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::GetElement";
        TTopLevelVariable<ID3DX11EffectVariable> *pTopLevelEntity2 = GetTopLevelEntity();
        UDataPointer dataPtr;

        if (pTopLevelEntity2->pEffect->IsOptimized())
        {
            DPF(0, "ID3DX11EffectVariable::GetElement: Cannot get element; effect has been Optimize()'ed");
            return &g_InvalidScalarVariable;
        }

        if (!IsArray())
        {
            DPF(0, "%s: This interface does not refer to an array", pFuncName);
            return &g_InvalidScalarVariable;
        }

        if (Index >= pType->Elements)
        {
            DPF(0, "%s: Invalid element index (%u, total: %u)", pFuncName, Index, pType->Elements);
            return &g_InvalidScalarVariable;
        }

        if (pType->BelongsInConstantBuffer())
        {
            dataPtr.pGeneric = Data.pNumeric + pType->Stride * Index;
        }
        else
        {
            dataPtr.pGeneric = GetBlockByIndex(pType->VarType, pType->ObjectType, Data.pGeneric, Index);
            if (nullptr == dataPtr.pGeneric)
            {
                DPF(0, "%s: Internal error", pFuncName);
                return &g_InvalidScalarVariable;
            }
        }

        return pTopLevelEntity2->pEffect->CreatePooledVariableMemberInterface(pTopLevelEntity2, (SVariable *) this, dataPtr, true, Index);
    }

    STDMETHOD_(ID3DX11EffectScalarVariable*, AsScalar)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsScalar";

        if (pType->VarType != EVT_Numeric || 
            pType->NumericType.NumericLayout != ENL_Scalar)
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidScalarVariable;
        }

        return (ID3DX11EffectScalarVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectVectorVariable*, AsVector)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsVector";

        if (pType->VarType != EVT_Numeric || 
            pType->NumericType.NumericLayout != ENL_Vector)
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidVectorVariable;
        }

        return (ID3DX11EffectVectorVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectMatrixVariable*, AsMatrix)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsMatrix";

        if (pType->VarType != EVT_Numeric || 
            pType->NumericType.NumericLayout != ENL_Matrix)
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidMatrixVariable;
        }

        return (ID3DX11EffectMatrixVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectStringVariable*, AsString)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsString";

        if (!pType->IsObjectType(EOT_String))
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidStringVariable;
        }

        return (ID3DX11EffectStringVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectClassInstanceVariable*, AsClassInstance)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsClassInstance";

        if (!pType->IsClassInstance() )
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidClassInstanceVariable;
        }
        else if( pMemberData == nullptr )
        {
            DPF(0, "%s: Non-global class instance variables (members of structs or classes) and class instances "
                   "inside tbuffers are not supported.", pFuncName );
            return &g_InvalidClassInstanceVariable;
        }

        return (ID3DX11EffectClassInstanceVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectInterfaceVariable*, AsInterface)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsInterface";

        if (!pType->IsInterface())
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidInterfaceVariable;
        }

        return (ID3DX11EffectInterfaceVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectShaderResourceVariable*, AsShaderResource)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsShaderResource";

        if (!pType->IsShaderResource())
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidShaderResourceVariable;
        }

        return (ID3DX11EffectShaderResourceVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectUnorderedAccessViewVariable*, AsUnorderedAccessView)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsUnorderedAccessView";

        if (!pType->IsUnorderedAccessView())
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidUnorderedAccessViewVariable;
        }

        return (ID3DX11EffectUnorderedAccessViewVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectRenderTargetViewVariable*, AsRenderTargetView)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsRenderTargetView";

        if (!pType->IsRenderTargetView())
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidRenderTargetViewVariable;
        }

        return (ID3DX11EffectRenderTargetViewVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectDepthStencilViewVariable*, AsDepthStencilView)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsDepthStencilView";

        if (!pType->IsDepthStencilView())
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidDepthStencilViewVariable;
        }

        return (ID3DX11EffectDepthStencilViewVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectConstantBuffer*, AsConstantBuffer)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsConstantBuffer";
        DPF(0, "%s: Invalid typecast", pFuncName);
        return &g_InvalidConstantBuffer;
    }

    STDMETHOD_(ID3DX11EffectShaderVariable*, AsShader)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsShader";

        if (!pType->IsShader())
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidShaderVariable;
        }

        return (ID3DX11EffectShaderVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectBlendVariable*, AsBlend)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsBlend";

        if (!pType->IsObjectType(EOT_Blend))
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidBlendVariable;
        }

        return (ID3DX11EffectBlendVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectDepthStencilVariable*, AsDepthStencil)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsDepthStencil";

        if (!pType->IsObjectType(EOT_DepthStencil))
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidDepthStencilVariable;
        }

        return (ID3DX11EffectDepthStencilVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectRasterizerVariable*, AsRasterizer)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsRasterizer";

        if (!pType->IsObjectType(EOT_Rasterizer))
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidRasterizerVariable;
        }

        return (ID3DX11EffectRasterizerVariable *) this;
    }

    STDMETHOD_(ID3DX11EffectSamplerVariable*, AsSampler)()
    {
        static LPCSTR pFuncName = "ID3DX11EffectVariable::AsSampler";

        if (!pType->IsSampler())
        {
            DPF(0, "%s: Invalid typecast", pFuncName);
            return &g_InvalidSamplerVariable;
        }

        return (ID3DX11EffectSamplerVariable *) this;
    }

    // Numeric variables should override this
    STDMETHOD(SetRawValue)(_In_reads_bytes_(Count) const void *pData, _In_ uint32_t Offset, _In_ uint32_t Count)
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return ObjectSetRawValue(); }
    STDMETHOD(GetRawValue)(_Out_writes_(Count) void *pData, _In_ uint32_t Offset, _In_ uint32_t Count)
        { UNREFERENCED_PARAMETER(pData); UNREFERENCED_PARAMETER(Offset); UNREFERENCED_PARAMETER(Count); return ObjectGetRawValue(); }
};

//////////////////////////////////////////////////////////////////////////
// TTopLevelVariable - functionality for annotations and global variables
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TTopLevelVariable : public SVariable, public IBaseInterface
{
    // Required to create member/element variable interfaces
    CEffect *pEffect;

    CEffect* GetEffect()
    {
        return pEffect;
    }

    TTopLevelVariable() noexcept :
        pEffect(nullptr)
    {
    }

    uint32_t  GetTotalUnpackedSize()
    {
        return ((SType*)pType)->GetTotalUnpackedSize(false);
    }

    STDMETHOD_(ID3DX11EffectType*, GetType)()
    {
        return (ID3DX11EffectType*)(SType*)pType;
    }

    TTopLevelVariable<ID3DX11EffectVariable> * GetTopLevelEntity()
    {
        return (TTopLevelVariable<ID3DX11EffectVariable> *)this;
    }

    bool IsArray()
    {
        return (pType->Elements > 0);
    }

};

//////////////////////////////////////////////////////////////////////////
// TMember - functionality for structure/array members of other variables
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TMember : public SVariable, public IBaseInterface
{
    // Indicates that this is a single element of a containing array
    uint32_t                                    IsSingleElement : 1;

    // Required to create member/element variable interfaces
    TTopLevelVariable<ID3DX11EffectVariable>    *pTopLevelEntity;

    TMember() noexcept :
        IsSingleElement(false),
        pTopLevelEntity(nullptr)
    {
    }

    CEffect* GetEffect()
    {
        return pTopLevelEntity->pEffect;
    }

    uint32_t  GetTotalUnpackedSize()
    {
        return pType->GetTotalUnpackedSize(IsSingleElement);
    }

    STDMETHOD_(ID3DX11EffectType*, GetType)() override
    {
        if (IsSingleElement)
        {
            return pTopLevelEntity->pEffect->CreatePooledSingleElementTypeInterface( pType );
        }
        else
        {
            return (ID3DX11EffectType*) pType;
        }
    }

    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc) override
    {
        HRESULT hr = S_OK;
        static LPCSTR pFuncName = "ID3DX11EffectVariable::GetDesc";

        VERIFYPARAMETER(pDesc != nullptr);

        pDesc->Name = pName;
        pDesc->Semantic = pSemantic;
        pDesc->Flags = 0;

        if (pTopLevelEntity->pEffect->IsReflectionData(pTopLevelEntity))
        {
            // Is part of an annotation
            assert(pTopLevelEntity->pEffect->IsReflectionData(Data.pGeneric));
            pDesc->Annotations = 0;
            pDesc->BufferOffset = 0;
            pDesc->Flags |= D3DX11_EFFECT_VARIABLE_ANNOTATION;
        }
        else
        {
            // Is part of a global variable
            assert(pTopLevelEntity->pEffect->IsRuntimeData(pTopLevelEntity));
            if (!pTopLevelEntity->pType->IsObjectType(EOT_String))
            {
                // strings are funny; their data is reflection data, so ignore those
                assert(pTopLevelEntity->pEffect->IsRuntimeData(Data.pGeneric));
            }
            
            pDesc->Annotations = ((TGlobalVariable<ID3DX11Effect>*)pTopLevelEntity)->AnnotationCount;

            SConstantBuffer *pCB = ((TGlobalVariable<ID3DX11Effect>*)pTopLevelEntity)->pCB;

            if (pType->BelongsInConstantBuffer())
            {   
                assert(pCB != 0);
                _Analysis_assume_(pCB != 0);
                UINT_PTR offset = Data.pNumeric - pCB->pBackingStore;
                assert(offset == (uint32_t)offset);
                pDesc->BufferOffset = (uint32_t)offset;
                assert(pDesc->BufferOffset >= 0 && pDesc->BufferOffset + GetTotalUnpackedSize() <= pCB->Size);
            }
            else
            {
                assert(pCB == nullptr);
                pDesc->BufferOffset = 0;
            }
        }

lExit:
        return hr;
    }

    TTopLevelVariable<ID3DX11EffectVariable> * GetTopLevelEntity()
    {
        return pTopLevelEntity;
    }

    bool IsArray()
    {
        return (pType->Elements > 0 && !IsSingleElement);
    }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override
    { return pTopLevelEntity->GetAnnotationByIndex(Index); }
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override
    { return pTopLevelEntity->GetAnnotationByName(Name); }

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetParentConstantBuffer)() override
    { return pTopLevelEntity->GetParentConstantBuffer(); }

    // Annotations should never be able to go down this codepath
    void DirtyVariable()
    {
        // make sure to call the global variable's version of dirty variable
        ((TGlobalVariable<ID3DX11EffectVariable>*)pTopLevelEntity)->DirtyVariable();
    }
};

//////////////////////////////////////////////////////////////////////////
// TAnnotation - functionality for top level annotations
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TAnnotation : public TVariable<TTopLevelVariable<IBaseInterface> >
{
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc) override
    {
        HRESULT hr = S_OK;
        static LPCSTR pFuncName = "ID3DX11EffectVariable::GetDesc";

        VERIFYPARAMETER(pDesc != nullptr);

        pDesc->Name = pName;
        pDesc->Semantic = pSemantic;
        pDesc->Flags = D3DX11_EFFECT_VARIABLE_ANNOTATION;
        pDesc->Annotations = 0;
        pDesc->BufferOffset = 0;
        pDesc->ExplicitBindPoint = 0;

lExit:
        return hr;

    }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override
    {
        UNREFERENCED_PARAMETER(Index);
        static LPCSTR pFuncName = "ID3DX11EffectVariable::GetAnnotationByIndex";
        DPF(0, "%s: Only variables may have annotations", pFuncName);
        return &g_InvalidScalarVariable;
    }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override
    {
        UNREFERENCED_PARAMETER(Name);
        static LPCSTR pFuncName = "ID3DX11EffectVariable::GetAnnotationByName";
        DPF(0, "%s: Only variables may have annotations", pFuncName);
        return &g_InvalidScalarVariable;
    }

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetParentConstantBuffer)() override
    { return NoParentCB(); }

    void DirtyVariable()
    {
        assert(0);
    }
};

//////////////////////////////////////////////////////////////////////////
// TGlobalVariable - functionality for top level global variables
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TGlobalVariable : public TVariable<TTopLevelVariable<IBaseInterface> >
{
    Timer           LastModifiedTime;

    // if numeric, pointer to the constant buffer where this variable lives
    SConstantBuffer *pCB;

    uint32_t        AnnotationCount;
    SAnnotation     *pAnnotations;

    TGlobalVariable() noexcept :
        LastModifiedTime(0),
        pCB(nullptr),
        AnnotationCount(0),
        pAnnotations(nullptr)
    {
    }

    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc)
    {
        HRESULT hr = S_OK;
        static LPCSTR pFuncName = "ID3DX11EffectVariable::GetDesc";

        VERIFYPARAMETER(pDesc != nullptr);

        pDesc->Name = pName;
        pDesc->Semantic = pSemantic;
        pDesc->Flags = 0;
        pDesc->Annotations = AnnotationCount;

        if (pType->BelongsInConstantBuffer())
        {
            assert(pCB != 0);
            _Analysis_assume_(pCB != 0);
            UINT_PTR offset = Data.pNumeric - pCB->pBackingStore;
            assert(offset == (uint32_t)offset);
            pDesc->BufferOffset = (uint32_t)offset;
            assert(pDesc->BufferOffset >= 0 && pDesc->BufferOffset + GetTotalUnpackedSize() <= pCB->Size );
        }
        else
        {
            assert(pCB == nullptr);
            pDesc->BufferOffset = 0;
        }

        if (ExplicitBindPoint != -1)
        {
            pDesc->ExplicitBindPoint = ExplicitBindPoint;
            pDesc->Flags |= D3DX11_EFFECT_VARIABLE_EXPLICIT_BIND_POINT;
        }
        else
        {
            pDesc->ExplicitBindPoint = 0;
        }

lExit:
        return hr;
    }

    // these are all well defined for global vars
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index)
    {
        return GetAnnotationByIndexHelper("ID3DX11EffectVariable", Index, AnnotationCount, pAnnotations);
    }

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name)
    {
        return GetAnnotationByNameHelper("ID3DX11EffectVariable", Name, AnnotationCount, pAnnotations);
    }

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetParentConstantBuffer)()
    { 
        if (nullptr != pCB)
        {
            assert(pType->BelongsInConstantBuffer());
            return (ID3DX11EffectConstantBuffer*)pCB; 
        }
        else
        {
            assert(!pType->BelongsInConstantBuffer());
            return &g_InvalidConstantBuffer;
        }
    }

    inline void DirtyVariable()
    {
        assert(pCB != 0);
        _Analysis_assume_(pCB != 0);
        pCB->IsDirty = true;
        LastModifiedTime = pEffect->GetCurrentTime();
    }

};

//////////////////////////////////////////////////////////////////////////
// TNumericVariable - implements raw set/get functionality
//////////////////////////////////////////////////////////////////////////

// IMPORTANT NOTE: All of these numeric & object aspect classes MUST NOT
// add data members to the base variable classes.  Otherwise type sizes 
// will disagree between object & numeric variables and we cannot eaily 
// create arrays of global variables using SGlobalVariable

// Requires that IBaseInterface have SVariable's members, GetTotalUnpackedSize() and DirtyVariable()
template<typename IBaseInterface, bool IsAnnotation>
struct TNumericVariable : public IBaseInterface
{
    STDMETHOD(SetRawValue)(_In_reads_bytes_(ByteCount) const void *pData, _In_ uint32_t ByteOffset, _In_ uint32_t ByteCount) override 
    {
        if (IsAnnotation)
        {
            return AnnotationInvalidSetCall("ID3DX11EffectVariable::SetRawValue");
        }
        else
        {
            HRESULT hr = S_OK;    

#ifdef _DEBUG
            static LPCSTR pFuncName = "ID3DX11EffectVariable::SetRawValue";

            VERIFYPARAMETER(pData);

            if ((ByteOffset + ByteCount < ByteOffset) ||
                (ByteCount + (uint8_t*)pData < (uint8_t*)pData) ||
                ((ByteOffset + ByteCount) > GetTotalUnpackedSize()))
            {
                // overflow of some kind
                DPF(0, "%s: Invalid range specified", pFuncName);
                VH(E_INVALIDARG);
            }
#endif

            DirtyVariable();
            memcpy(Data.pNumeric + ByteOffset, pData, ByteCount);

lExit:
            return hr;
        }
    }

    STDMETHOD(GetRawValue)(_Out_writes_bytes_(ByteCount) void *pData, _In_ uint32_t ByteOffset, _In_ uint32_t ByteCount) override
    {
        HRESULT hr = S_OK;    

#ifdef _DEBUG
        static LPCSTR pFuncName = "ID3DX11EffectVariable::GetRawValue";

        VERIFYPARAMETER(pData);

        if ((ByteOffset + ByteCount < ByteOffset) ||
            (ByteCount + (uint8_t*)pData < (uint8_t*)pData) ||
            ((ByteOffset + ByteCount) > GetTotalUnpackedSize()))
        {
            // overflow of some kind
            DPF(0, "%s: Invalid range specified", pFuncName);
            VH(E_INVALIDARG);
        }
#endif

        memcpy(pData, Data.pNumeric + ByteOffset, ByteCount);

lExit:
        return hr;
    }
};

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectScalarVariable (TFloatScalarVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface, bool IsAnnotation>
struct TFloatScalarVariable : public TNumericVariable<IBaseInterface, IsAnnotation>
{
    STDMETHOD(SetFloat)(_In_ const float Value) override;
    STDMETHOD(GetFloat)(_Out_ float *pValue) override;

    STDMETHOD(SetFloatArray)(_In_reads_(Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetFloatArray)(_Out_writes_(Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetInt)(_In_ const int Value) override;
    STDMETHOD(GetInt)(_Out_ int *pValue) override;

    STDMETHOD(SetIntArray)(_In_reads_(Count) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetIntArray)(_Out_writes_(Count) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetBool)(_In_ const bool Value) override;
    STDMETHOD(GetBool)(_Out_ bool *pValue) override;

    STDMETHOD(SetBoolArray)(_In_reads_(Count) const bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetBoolArray)(_Out_writes_(Count) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::SetFloat(float Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetFloat";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_Float, ETVT_Float, float, false>(Value, Data.pNumericFloat, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::GetFloat(float *pValue)
{
    return CopyScalarValue<ETVT_Float, ETVT_Float, float, true>(*Data.pNumericFloat, pValue, "ID3DX11EffectScalarVariable::GetFloat");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::SetFloatArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetFloatArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_Float, ETVT_Float, float, float>(pData, Data.pNumericFloat, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::GetFloatArray(float *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Float, ETVT_Float, float, float>(Data.pNumericFloat, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetFloatArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::SetInt(const int Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetInt";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_Int, ETVT_Float, int, false>(Value, Data.pNumericFloat, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::GetInt(int *pValue)
{
    return CopyScalarValue<ETVT_Float, ETVT_Int, float, true>(*Data.pNumericFloat, pValue, "ID3DX11EffectScalarVariable::GetInt");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::SetIntArray(const int *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetIntArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_Int, ETVT_Float, int, float>(pData, Data.pNumericFloat, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::GetIntArray(int *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Float, ETVT_Int, float, int>(Data.pNumericFloat, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetIntArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::SetBool(const bool Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetBool";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_bool, ETVT_Float, bool, false>(Value, Data.pNumericFloat, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::GetBool(bool *pValue)
{
    return CopyScalarValue<ETVT_Float, ETVT_bool, float, true>(*Data.pNumericFloat, pValue, "ID3DX11EffectScalarVariable::GetBool");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::SetBoolArray(const bool *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetBoolArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_bool, ETVT_Float, bool, float>(pData, Data.pNumericFloat, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TFloatScalarVariable<IBaseInterface, IsAnnotation>::GetBoolArray(bool *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Float, ETVT_bool, float, bool>(Data.pNumericFloat, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetBoolArray");
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectScalarVariable (TIntScalarVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface, bool IsAnnotation>
struct TIntScalarVariable : public TNumericVariable<IBaseInterface, IsAnnotation>
{
    STDMETHOD(SetFloat)(_In_ const float Value) override;
    STDMETHOD(GetFloat)(_Out_ float *pValue) override;

    STDMETHOD(SetFloatArray)(_In_reads_(Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetFloatArray)(_Out_writes_(Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetInt)(_In_ const int Value) override;
    STDMETHOD(GetInt)(_Out_ int *pValue) override;

    STDMETHOD(SetIntArray)(_In_reads_(Count) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetIntArray)(_Out_writes_(Count) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetBool)(_In_ const bool Value) override;
    STDMETHOD(GetBool)(_Out_ bool *pValue) override;

    STDMETHOD(SetBoolArray)(_In_reads_(Count) const bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetBoolArray)(_Out_writes_(Count) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::SetFloat(float Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetFloat";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_Float, ETVT_Int, float, false>(Value, Data.pNumericInt, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::GetFloat(float *pValue)
{
    return CopyScalarValue<ETVT_Int, ETVT_Float, int, true>(*Data.pNumericInt, pValue, "ID3DX11EffectScalarVariable::GetFloat");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::SetFloatArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetFloatArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_Float, ETVT_Int, float, int>(pData, Data.pNumericInt, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::GetFloatArray(float *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Int, ETVT_Float, int, float>(Data.pNumericInt, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetFloatArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::SetInt(const int Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetInt";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_Int, ETVT_Int, int, false>(Value, Data.pNumericInt, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::GetInt(int *pValue)
{
    return CopyScalarValue<ETVT_Int, ETVT_Int, int, true>(*Data.pNumericInt, pValue, "ID3DX11EffectScalarVariable::GetInt");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::SetIntArray(const int *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetIntArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_Int, ETVT_Int, int, int>(pData, Data.pNumericInt, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::GetIntArray(int *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Int, ETVT_Int, int, int>(Data.pNumericInt, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetIntArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::SetBool(const bool Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetBool";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_bool, ETVT_Int, bool, false>(Value, Data.pNumericInt, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::GetBool(bool *pValue)
{
    return CopyScalarValue<ETVT_Int, ETVT_bool, int, true>(*Data.pNumericInt, pValue, "ID3DX11EffectScalarVariable::GetBool");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::SetBoolArray(const bool *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetBoolArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_bool, ETVT_Int, bool, int>(pData, Data.pNumericInt, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TIntScalarVariable<IBaseInterface, IsAnnotation>::GetBoolArray(bool *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Int, ETVT_bool, int, bool>(Data.pNumericInt, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetBoolArray");
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectScalarVariable (TBoolScalarVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface, bool IsAnnotation>
struct TBoolScalarVariable : public TNumericVariable<IBaseInterface, IsAnnotation>
{
    STDMETHOD(SetFloat)(_In_ const float Value) override;
    STDMETHOD(GetFloat)(_Out_ float *pValue) override;

    STDMETHOD(SetFloatArray)(_In_reads_(Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetFloatArray)(_Out_writes_(Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetInt)(_In_ const int Value) override;
    STDMETHOD(GetInt)(_Out_ int *pValue) override;

    STDMETHOD(SetIntArray)(_In_reads_(Count) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetIntArray)(_Out_writes_(Count) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetBool)(_In_ const bool Value) override;
    STDMETHOD(GetBool)(_Out_ bool *pValue) override;

    STDMETHOD(SetBoolArray)(_In_reads_(Count) const bool *pData, uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetBoolArray)(_Out_writes_(Count) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::SetFloat(float Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetFloat";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_Float, ETVT_Bool, float, false>(Value, Data.pNumericBool, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::GetFloat(float *pValue)
{
    return CopyScalarValue<ETVT_Bool, ETVT_Float, BOOL, true>(*Data.pNumericBool, pValue, "ID3DX11EffectScalarVariable::GetFloat");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::SetFloatArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetFloatArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_Float, ETVT_Bool, float, BOOL>(pData, Data.pNumericBool, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::GetFloatArray(float *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Bool, ETVT_Float, BOOL, float>(Data.pNumericBool, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetFloatArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::SetInt(const int Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetInt";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_Int, ETVT_Bool, int, false>(Value, Data.pNumericBool, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::GetInt(int *pValue)
{
    return CopyScalarValue<ETVT_Bool, ETVT_Int, BOOL, true>(*Data.pNumericBool, pValue, "ID3DX11EffectScalarVariable::GetInt");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::SetIntArray(const int *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetIntArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_Int, ETVT_Bool, int, BOOL>(pData, Data.pNumericBool, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::GetIntArray(int *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Bool, ETVT_Int, BOOL, int>(Data.pNumericBool, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetIntArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::SetBool(const bool Value)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetBool";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return CopyScalarValue<ETVT_bool, ETVT_Bool, bool, false>(Value, Data.pNumericBool, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::GetBool(bool *pValue)
{
    return CopyScalarValue<ETVT_Bool, ETVT_bool, BOOL, true>(*Data.pNumericBool, pValue, "ID3DX11EffectScalarVariable::GetBool");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::SetBoolArray(const bool *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectScalarVariable::SetBoolArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return SetScalarArray<ETVT_bool, ETVT_Bool, bool, BOOL>(pData, Data.pNumericBool, Offset, Count, 
        pType, GetTotalUnpackedSize(), pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TBoolScalarVariable<IBaseInterface, IsAnnotation>::GetBoolArray(bool *pData, uint32_t Offset, uint32_t Count)
{
    return GetScalarArray<ETVT_Bool, ETVT_bool, BOOL, bool>(Data.pNumericBool, pData, Offset, Count, 
        pType, GetTotalUnpackedSize(), "ID3DX11EffectScalarVariable::GetBoolArray");
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectVectorVariable (TVectorVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType >
struct TVectorVariable : public TNumericVariable<IBaseInterface, IsAnnotation>
{
    STDMETHOD(SetBoolVector) (_In_reads_(4) const bool *pData) override; 
    STDMETHOD(SetIntVector)  (_In_reads_(4) const int *pData) override;
    STDMETHOD(SetFloatVector)(_In_reads_(4) const float *pData) override;

    STDMETHOD(GetBoolVector) (_Out_writes_(4) bool *pData) override; 
    STDMETHOD(GetIntVector)  (_Out_writes_(4) int *pData) override;
    STDMETHOD(GetFloatVector)(_Out_writes_(4) float *pData) override;


    STDMETHOD(SetBoolVectorArray) (_In_reads_(Count*4) const bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override; 
    STDMETHOD(SetIntVectorArray)  (_In_reads_(Count*4) const int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(SetFloatVectorArray)(_In_reads_(Count*4) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(GetBoolVectorArray) (_Out_writes_(Count*4) bool *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override; 
    STDMETHOD(GetIntVectorArray)  (_Out_writes_(Count*4) int *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetFloatVectorArray)(_Out_writes_(Count*4) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

// Note that branches in this code is based on template parameters and will be compiled out
#pragma warning (push)
#pragma warning (disable : 6101)
template <ETemplateVarType DestType, ETemplateVarType SourceType>
void __forceinline CopyDataWithTypeConversion(_Out_ void *pDest,
                                              _In_ const void *pSource,
                                              _In_ size_t dstVecSize, _In_ size_t srcVecSize,
                                              _In_ size_t elementCount, _In_ size_t vecCount)
{
    switch (SourceType)
    {
    case ETVT_Bool:
        switch (DestType)
        {
        case ETVT_Bool:
            for (size_t j=0; j<vecCount; j++)
            {
                memcpy(pDest, pSource, elementCount * SType::c_ScalarSize);

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Int:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((int*)pDest)[i] = ((bool*)pSource)[i] ? -1 : 0;

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Float:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((float*)pDest)[i] = ((bool*)pSource)[i] ? -1.0f : 0.0f;

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_bool:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((bool*)pDest)[i] = (((int*)pSource)[i] != 0) ? true : false;

                pDest = ((bool*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        default:
            assert(0);
        }
        break;

    case ETVT_Int:
        switch (DestType)
        {
        case ETVT_Bool:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((int*)pDest)[i] = (((int*)pSource)[i] != 0) ? -1 : 0;

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Int:
            for (size_t j=0; j<vecCount; j++)
            {
                memcpy(pDest, pSource, elementCount * SType::c_ScalarSize);

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Float:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((float*)pDest)[i] = (float)(((int*)pSource)[i]);

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_bool:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((bool*)pDest)[i] = (((int*)pSource)[i] != 0) ? true : false;

                pDest = ((bool*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        default:
            assert(0);
        }
        break;

    case ETVT_Float:
        switch (DestType)
        {
        case ETVT_Bool:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((int*)pDest)[i] = (((float*)pSource)[i] != 0.0f) ? -1 : 0;

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Int:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((int*)pDest)[i] = (int)((float*)pSource)[i];

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Float:
            for (size_t i=0; i<vecCount; i++)
            {
                memcpy( pDest, pSource, elementCount * SType::c_ScalarSize);

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        case ETVT_bool:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    ((bool*)pDest)[i] = (((float*)pSource)[i] != 0.0f) ? true : false;

                pDest = ((bool*) pDest) + dstVecSize;
                pSource = ((float*) pSource) + srcVecSize;
            }
            break;

        default:
            assert(0);
        }
        break;

    case ETVT_bool:
        switch (DestType)
        {
        case ETVT_Bool:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    reinterpret_cast<int*>(pDest)[i] = reinterpret_cast<const bool*>(pSource)[i] ? -1 : 0;

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((bool*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Int:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    reinterpret_cast<int*>(pDest)[i] = reinterpret_cast<const bool*>(pSource)[i] ? -1 : 0;

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((bool*) pSource) + srcVecSize;
            }
            break;

        case ETVT_Float:
            for (size_t j=0; j<vecCount; j++)
            {
                for (size_t i=0; i<elementCount; i++)
                    reinterpret_cast<float*>(pDest)[i] = reinterpret_cast<const bool*>(pSource)[i] ? -1.0f : 0.0f;

                pDest = ((float*) pDest) + dstVecSize;
                pSource = ((bool*) pSource) + srcVecSize;
            }
            break;

        case ETVT_bool:
            for (size_t j=0; j<vecCount; j++)
            {
                memcpy(pDest, pSource, elementCount);

                pDest = ((bool*) pDest) + dstVecSize;
                pSource = ((bool*) pSource) + srcVecSize;
            }
            break;

        default:
            assert(0);
        }
        break;

    default:
        assert(0);
    }
}
#pragma warning (pop)

// Float Vector

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType >::SetFloatVector(const float *pData)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetFloatVector";

#ifdef _DEBUG
    VERIFYPARAMETER(pData);
#endif

    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    CopyDataWithTypeConversion<BaseType, ETVT_Float>(Data.pVector, pData, 4, pType->NumericType.Columns, pType->NumericType.Columns, 1);

lExit:
    return hr;
}

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::GetFloatVector(float *pData)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetFloatVector";

#ifdef _DEBUG
    VERIFYPARAMETER(pData);
#endif

    CopyDataWithTypeConversion<ETVT_Float, BaseType>(pData, Data.pVector, pType->NumericType.Columns, 4, pType->NumericType.Columns, 1);

lExit:
    return hr;
}

// Int Vector

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType >::SetIntVector(const int *pData)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetIntVector";

#ifdef _DEBUG
    VERIFYPARAMETER(pData);
#endif

    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    CopyDataWithTypeConversion<BaseType, ETVT_Int>(Data.pVector, pData, 4, pType->NumericType.Columns, pType->NumericType.Columns, 1);

lExit:
    return hr;
}

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::GetIntVector(int *pData)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetIntVector";
    VERIFYPARAMETER(pData);
#endif

    CopyDataWithTypeConversion<ETVT_Int, BaseType>(pData, Data.pVector, pType->NumericType.Columns, 4, pType->NumericType.Columns, 1);

lExit:
    return hr;
}

// Bool Vector

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType >::SetBoolVector(const bool *pData)
{
    HRESULT hr = S_OK;

    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetBoolVector";

#ifdef _DEBUG
    VERIFYPARAMETER(pData);
#endif

    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    CopyDataWithTypeConversion<BaseType, ETVT_bool>(Data.pVector, pData, 4, pType->NumericType.Columns, pType->NumericType.Columns, 1);

lExit:
    return hr;
}

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::GetBoolVector(bool *pData)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetBoolVector";
    VERIFYPARAMETER(pData);
#endif

    CopyDataWithTypeConversion<ETVT_bool, BaseType>(pData, Data.pVector, pType->NumericType.Columns, 4, pType->NumericType.Columns, 1);

lExit:
    return hr;
}

// Vector Arrays /////////////////////////////////////////////////////////

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::SetFloatVectorArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetFloatVectorArray";

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    // ensure we don't write over the padding at the end of the vector array
    CopyDataWithTypeConversion<BaseType, ETVT_Float>(Data.pVector + Offset, pData, 4, pType->NumericType.Columns, pType->NumericType.Columns, std::max(std::min((int)Count, (int)pType->Elements - (int)Offset), 0));

lExit:
    return hr;
}

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::GetFloatVectorArray(float *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetFloatVectorArray";
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    // ensure we don't read past the end of the vector array
    CopyDataWithTypeConversion<ETVT_Float, BaseType>(pData, Data.pVector + Offset, pType->NumericType.Columns, 4, pType->NumericType.Columns, std::max(std::min((int)Count, (int)pType->Elements - (int)Offset), 0));

lExit:
    return hr;
}

// int

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::SetIntVectorArray(const int *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetIntVectorArray";

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    // ensure we don't write over the padding at the end of the vector array
    CopyDataWithTypeConversion<BaseType, ETVT_Int>(Data.pVector + Offset, pData, 4, pType->NumericType.Columns, pType->NumericType.Columns, std::max(std::min((int)Count, (int)pType->Elements - (int)Offset), 0));

lExit:
    return hr;
}

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::GetIntVectorArray(int *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetIntVectorArray";

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    // ensure we don't read past the end of the vector array
    CopyDataWithTypeConversion<ETVT_Int, BaseType>(pData, Data.pVector + Offset, pType->NumericType.Columns, 4, pType->NumericType.Columns, std::max(std::min((int)Count, (int)pType->Elements - (int)Offset), 0));

lExit:
    return hr;
}

// bool

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::SetBoolVectorArray(const bool *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetBoolVectorArray";

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    // ensure we don't write over the padding at the end of the vector array
    CopyDataWithTypeConversion<BaseType, ETVT_bool>(Data.pVector + Offset, pData, 4, pType->NumericType.Columns, pType->NumericType.Columns, std::max(std::min((int)Count, (int)pType->Elements - (int)Offset), 0));

lExit:
    return hr;
}

template<typename IBaseInterface, bool IsAnnotation, ETemplateVarType BaseType>
_Use_decl_annotations_
HRESULT TVectorVariable<IBaseInterface, IsAnnotation, BaseType>::GetBoolVectorArray(bool *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetBoolVectorArray";

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    // ensure we don't read past the end of the vector array
    CopyDataWithTypeConversion<ETVT_bool, BaseType>(pData, Data.pVector + Offset, pType->NumericType.Columns, 4, pType->NumericType.Columns, std::max(std::min((int)Count, (int)pType->Elements - (int)Offset), 0));

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectVector4Variable (TVectorVariable implementation) [OPTIMIZED]
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TVector4Variable : public TVectorVariable<IBaseInterface, false, ETVT_Float>
{
    STDMETHOD(SetFloatVector)(_In_reads_(4) const float *pData) override;
    STDMETHOD(GetFloatVector)(_Out_writes_(4) float *pData) override;

    STDMETHOD(SetFloatVectorArray)(_In_reads_(Count*4) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetFloatVectorArray)(_Out_writes_(Count*4) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TVector4Variable<IBaseInterface>::SetFloatVector(const float *pData)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetFloatVector";

#ifdef _DEBUG
    VERIFYPARAMETER(pData);
#endif

    DirtyVariable();
    Data.pVector[0] = ((CEffectVector4*) pData)[0];

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TVector4Variable<IBaseInterface>::GetFloatVector(float *pData)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetFloatVector";

#ifdef _DEBUG
    VERIFYPARAMETER(pData);
#endif

    memcpy(pData, Data.pVector, pType->NumericType.Columns * SType::c_ScalarSize);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TVector4Variable<IBaseInterface>::SetFloatVectorArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::SetFloatVectorArray";

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    DirtyVariable();
    // ensure we don't write over the padding at the end of the vector array
    memcpy(Data.pVector + Offset, pData,
           std::min<size_t>(Count * sizeof(CEffectVector4), pType->TotalSize - (Offset * sizeof(CEffectVector4))));

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TVector4Variable<IBaseInterface>::GetFloatVectorArray(float *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectVectorVariable::GetFloatVectorArray";

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pData, pType, GetTotalUnpackedSize()))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    // ensure we don't read past the end of the vector array
    memcpy(pData, Data.pVector + Offset,
           std::min<size_t>(Count * sizeof(CEffectVector4), pType->TotalSize - (Offset * sizeof(CEffectVector4)))); 

lExit:
    return hr;
}


//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectMatrixVariable (TMatrixVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface, bool IsAnnotation>
struct TMatrixVariable : public TNumericVariable<IBaseInterface, IsAnnotation>
{
    STDMETHOD(SetMatrix)(_In_reads_(16) const float *pData) override;
    STDMETHOD(GetMatrix)(_Out_writes_(16) float *pData) override;

    STDMETHOD(SetMatrixArray)(_In_reads_(Count*16) const float *pData, uint32_t Offset, uint32_t Count) override;
    STDMETHOD(GetMatrixArray)(_Out_writes_(Count*16) float *pData, uint32_t Offset, uint32_t Count) override;

    STDMETHOD(SetMatrixPointerArray)(_In_reads_(Count*16) const float **ppData, uint32_t Offset, uint32_t Count) override;
    STDMETHOD(GetMatrixPointerArray)(_Out_writes_(Count*16) float **ppData, uint32_t Offset, uint32_t Count) override;

    STDMETHOD(SetMatrixTranspose)(_In_reads_(16) const float *pData) override;
    STDMETHOD(GetMatrixTranspose)(_Out_writes_(16) float *pData) override;

    STDMETHOD(SetMatrixTransposeArray)(_In_reads_(Count*16) const float *pData, uint32_t Offset, uint32_t Count) override;
    STDMETHOD(GetMatrixTransposeArray)(_Out_writes_(Count*16) float *pData, uint32_t Offset, uint32_t Count) override;

    STDMETHOD(SetMatrixTransposePointerArray)(_In_reads_(Count*16) const float **ppData, uint32_t Offset, uint32_t Count) override;
    STDMETHOD(GetMatrixTransposePointerArray)(_Out_writes_(Count*16) float **ppData, uint32_t Offset, uint32_t Count) override;
};

#pragma warning (push)
#pragma warning (disable : 6101)
template<bool Transpose>
static void SetMatrixTransposeHelper(_In_ const SType *pType, _Out_writes_bytes_(64) uint8_t *pDestData, _In_reads_(16) const float* pMatrix)
{
    uint32_t registers, entries;
    
    if (Transpose)
    {
        // row major
        registers = pType->NumericType.Rows;
        entries = pType->NumericType.Columns;
    }
    else
    {
        // column major
        registers = pType->NumericType.Columns;
        entries = pType->NumericType.Rows;
    }
    _Analysis_assume_( registers <= 4 );
    _Analysis_assume_( entries <= 4 );

    for (size_t i = 0; i < registers; ++ i)
    {
        for (size_t j = 0; j < entries; ++ j)
        {
#pragma prefast(suppress:__WARNING_UNRELATED_LOOP_TERMINATION, "regs / entries <= 4")
            ((float*)pDestData)[j] = ((float*)pMatrix)[j * 4 + i];
        }
        pDestData += SType::c_RegisterSize;
    }
}

template<bool Transpose>
static void GetMatrixTransposeHelper(_In_ const SType *pType, _In_reads_bytes_(64) uint8_t *pSrcData, _Out_writes_(16) float* pMatrix)
{
    uint32_t registers, entries;

    if (Transpose)
    {
        // row major
        registers = pType->NumericType.Rows;
        entries = pType->NumericType.Columns;
    }
    else
    {
        // column major
        registers = pType->NumericType.Columns;
        entries = pType->NumericType.Rows;
    }
    _Analysis_assume_( registers <= 4 );
    _Analysis_assume_( entries <= 4 );

    for (size_t i = 0; i < registers; ++ i)
    {
        for (size_t j = 0; j < entries; ++ j)
        {
            ((float*)pMatrix)[j * 4 + i] = ((float*)pSrcData)[j];
        }
        pSrcData += SType::c_RegisterSize;
    }
}

template<bool Transpose, bool IsSetting, bool ExtraIndirection>
HRESULT DoMatrixArrayInternal(_In_ const SType *pType, _In_ uint32_t  TotalUnpackedSize,
                              _Out_ uint8_t *pEffectData,
                              void *pMatrixData,
                              _In_ uint32_t Offset, _In_ uint32_t Count, _In_z_ LPCSTR pFuncName)
{    
    HRESULT hr = S_OK;

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pMatrixData, pType, TotalUnpackedSize))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#else
    UNREFERENCED_PARAMETER(TotalUnpackedSize);
    UNREFERENCED_PARAMETER(pFuncName);
#endif

    if ((pType->NumericType.IsColumnMajor && Transpose) || (!pType->NumericType.IsColumnMajor && !Transpose))
    {
        // fast path
        uint32_t  dataSize;
        if (Transpose)
        {
            dataSize = ((pType->NumericType.Columns - 1) * 4 + pType->NumericType.Rows) * SType::c_ScalarSize;
        }
        else
        {
            dataSize = ((pType->NumericType.Rows - 1) * 4 + pType->NumericType.Columns) * SType::c_ScalarSize;
        }

        for (size_t i = 0; i < Count; ++ i)
        {
            CEffectMatrix *pMatrix;
            if (ExtraIndirection)
            {
                pMatrix = ((CEffectMatrix **)pMatrixData)[i];
                if (!pMatrix)
                {
                    continue;
                }
            }
            else
            {
                pMatrix = ((CEffectMatrix *)pMatrixData) + i;
            }

            if (IsSetting)
            {
                memcpy(pEffectData + pType->Stride * (i + Offset), pMatrix, dataSize);
            }
            else
            {
                memcpy(pMatrix, pEffectData + pType->Stride * (i + Offset), dataSize);
            }
        }
    }
    else
    {
        // slow path
        for (size_t i = 0; i < Count; ++ i)
        {
            CEffectMatrix *pMatrix;
            if (ExtraIndirection)
            {
                pMatrix = ((CEffectMatrix **)pMatrixData)[i];
                if (!pMatrix)
                {
                    continue;
                }
            }
            else
            {
                pMatrix = ((CEffectMatrix *)pMatrixData) + i;
            }

            if (IsSetting)
            {
                SetMatrixTransposeHelper<Transpose>(pType, pEffectData + pType->Stride * (i + Offset), (float*) pMatrix);
            }
            else
            {
                GetMatrixTransposeHelper<Transpose>(pType, pEffectData + pType->Stride * (i + Offset), (float*) pMatrix);
            }
        }
    }

lExit:
    return hr;
}
#pragma warning (pop)

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::SetMatrix(const float *pData)
{
    static LPCSTR pFuncName = "ID3DX11EffectMatrixVariable::SetMatrix";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return DoMatrixArrayInternal<false, true, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, const_cast<float*>(pData), 0, 1, pFuncName);
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::GetMatrix(float *pData)
{
    return DoMatrixArrayInternal<false, false, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, pData, 0, 1, "ID3DX11EffectMatrixVariable::GetMatrix");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::SetMatrixArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectMatrixVariable::SetMatrixArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return DoMatrixArrayInternal<false, true, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, const_cast<float*>(pData), Offset, Count, "ID3DX11EffectMatrixVariable::SetMatrixArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::GetMatrixArray(float *pData, uint32_t Offset, uint32_t Count)
{
    return DoMatrixArrayInternal<false, false, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, pData, Offset, Count, "ID3DX11EffectMatrixVariable::GetMatrixArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::SetMatrixPointerArray(const float **ppData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectMatrixVariable::SetMatrixPointerArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return DoMatrixArrayInternal<false, true, true>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, const_cast<float**>(ppData), Offset, Count, "ID3DX11EffectMatrixVariable::SetMatrixPointerArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::GetMatrixPointerArray(float **ppData, uint32_t Offset, uint32_t Count)
{
    return DoMatrixArrayInternal<false, false, true>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, ppData, Offset, Count, "ID3DX11EffectMatrixVariable::GetMatrixPointerArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::SetMatrixTranspose(const float *pData)
{
    static LPCSTR pFuncName = "ID3DX11EffectMatrixVariable::SetMatrixTranspose";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return DoMatrixArrayInternal<true, true, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, const_cast<float*>(pData), 0, 1, "ID3DX11EffectMatrixVariable::SetMatrixTranspose");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::GetMatrixTranspose(float *pData)
{
    return DoMatrixArrayInternal<true, false, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, pData, 0, 1, "ID3DX11EffectMatrixVariable::GetMatrixTranspose");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::SetMatrixTransposeArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectMatrixVariable::SetMatrixTransposeArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return DoMatrixArrayInternal<true, true, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, const_cast<float*>(pData), Offset, Count, "ID3DX11EffectMatrixVariable::SetMatrixTransposeArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::GetMatrixTransposeArray(float *pData, uint32_t Offset, uint32_t Count)
{
    return DoMatrixArrayInternal<true, false, false>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, pData, Offset, Count, "ID3DX11EffectMatrixVariable::GetMatrixTransposeArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::SetMatrixTransposePointerArray(const float **ppData, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectMatrixVariable::SetMatrixTransposePointerArray";
    if (IsAnnotation) return AnnotationInvalidSetCall(pFuncName);
    DirtyVariable();
    return DoMatrixArrayInternal<true, true, true>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, const_cast<float**>(ppData), Offset, Count, "ID3DX11EffectMatrixVariable::SetMatrixTransposePointerArray");
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TMatrixVariable<IBaseInterface, IsAnnotation>::GetMatrixTransposePointerArray(float **ppData, uint32_t Offset, uint32_t Count)
{
    return DoMatrixArrayInternal<true, false, true>(pType, GetTotalUnpackedSize(), 
        Data.pNumeric, ppData, Offset, Count, "ID3DX11EffectMatrixVariable::GetMatrixTransposePointerArray");
}

// Optimize commonly used fast paths
// (non-annotations only!)
template<typename IBaseInterface, bool IsColumnMajor>
struct TMatrix4x4Variable : public TMatrixVariable<IBaseInterface, false>
{
    STDMETHOD(SetMatrix)(_In_reads_(16) const float *pData) override;
    STDMETHOD(GetMatrix)(_Out_writes_(16) float *pData) override;

    STDMETHOD(SetMatrixArray)(_In_reads_(16*Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetMatrixArray)(_Out_writes_(16*Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetMatrixTranspose)(_In_reads_(16) const float *pData) override;
    STDMETHOD(GetMatrixTranspose)(_Out_writes_(16) float *pData) override;

    STDMETHOD(SetMatrixTransposeArray)(_In_reads_(16*Count) const float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetMatrixTransposeArray)(_Out_writes_(16*Count) float *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

inline static void Matrix4x4TransposeHelper(_In_reads_bytes_(64) const void *pSrc, _Out_writes_bytes_(64) void *pDst)
{
    uint8_t *pDestData = (uint8_t*)pDst;
    uint32_t *pMatrix = (uint32_t*)pSrc;

    ((uint32_t*)pDestData)[0 * 4 + 0] = pMatrix[0 * 4 + 0];
    ((uint32_t*)pDestData)[0 * 4 + 1] = pMatrix[1 * 4 + 0];
    ((uint32_t*)pDestData)[0 * 4 + 2] = pMatrix[2 * 4 + 0];
    ((uint32_t*)pDestData)[0 * 4 + 3] = pMatrix[3 * 4 + 0];

    ((uint32_t*)pDestData)[1 * 4 + 0] = pMatrix[0 * 4 + 1];
    ((uint32_t*)pDestData)[1 * 4 + 1] = pMatrix[1 * 4 + 1];
    ((uint32_t*)pDestData)[1 * 4 + 2] = pMatrix[2 * 4 + 1];
    ((uint32_t*)pDestData)[1 * 4 + 3] = pMatrix[3 * 4 + 1];

    ((uint32_t*)pDestData)[2 * 4 + 0] = pMatrix[0 * 4 + 2];
    ((uint32_t*)pDestData)[2 * 4 + 1] = pMatrix[1 * 4 + 2];
    ((uint32_t*)pDestData)[2 * 4 + 2] = pMatrix[2 * 4 + 2];
    ((uint32_t*)pDestData)[2 * 4 + 3] = pMatrix[3 * 4 + 2];

    ((uint32_t*)pDestData)[3 * 4 + 0] = pMatrix[0 * 4 + 3];
    ((uint32_t*)pDestData)[3 * 4 + 1] = pMatrix[1 * 4 + 3];
    ((uint32_t*)pDestData)[3 * 4 + 2] = pMatrix[2 * 4 + 3];
    ((uint32_t*)pDestData)[3 * 4 + 3] = pMatrix[3 * 4 + 3];
}

inline static void Matrix4x4Copy(_In_reads_bytes_(64) const void *pSrc, _Out_writes_bytes_(64) void *pDst)
{
#if 1
    // In tests, this path ended up generating faster code both on x86 and x64
    // T1 - Matrix4x4Copy - this path
    // T2 - Matrix4x4Transpose
    // T1: 1.88 T2: 1.92 - with 32 bit copies
    // T1: 1.85 T2: 1.80 - with 64 bit copies

    uint64_t *pDestData = (uint64_t*)pDst;
    uint64_t *pMatrix = (uint64_t*)pSrc;

    pDestData[0 * 4 + 0] = pMatrix[0 * 4 + 0];
    pDestData[0 * 4 + 1] = pMatrix[0 * 4 + 1];
    pDestData[0 * 4 + 2] = pMatrix[0 * 4 + 2];
    pDestData[0 * 4 + 3] = pMatrix[0 * 4 + 3];

    pDestData[1 * 4 + 0] = pMatrix[1 * 4 + 0];
    pDestData[1 * 4 + 1] = pMatrix[1 * 4 + 1];
    pDestData[1 * 4 + 2] = pMatrix[1 * 4 + 2];
    pDestData[1 * 4 + 3] = pMatrix[1 * 4 + 3];
#else
    uint32_t *pDestData = (uint32_t*)pDst;
    uint32_t *pMatrix = (uint32_t*)pSrc;

    pDestData[0 * 4 + 0] = pMatrix[0 * 4 + 0];
    pDestData[0 * 4 + 1] = pMatrix[0 * 4 + 1];
    pDestData[0 * 4 + 2] = pMatrix[0 * 4 + 2];
    pDestData[0 * 4 + 3] = pMatrix[0 * 4 + 3];

    pDestData[1 * 4 + 0] = pMatrix[1 * 4 + 0];
    pDestData[1 * 4 + 1] = pMatrix[1 * 4 + 1];
    pDestData[1 * 4 + 2] = pMatrix[1 * 4 + 2];
    pDestData[1 * 4 + 3] = pMatrix[1 * 4 + 3];

    pDestData[2 * 4 + 0] = pMatrix[2 * 4 + 0];
    pDestData[2 * 4 + 1] = pMatrix[2 * 4 + 1];
    pDestData[2 * 4 + 2] = pMatrix[2 * 4 + 2];
    pDestData[2 * 4 + 3] = pMatrix[2 * 4 + 3];

    pDestData[3 * 4 + 0] = pMatrix[3 * 4 + 0];
    pDestData[3 * 4 + 1] = pMatrix[3 * 4 + 1];
    pDestData[3 * 4 + 2] = pMatrix[3 * 4 + 2];
    pDestData[3 * 4 + 3] = pMatrix[3 * 4 + 3];
#endif
}


// Note that branches in this code is based on template parameters and will be compiled out
#pragma warning (push)
#pragma warning (disable : 6101)
template<bool IsColumnMajor, bool Transpose, bool IsSetting>
inline HRESULT DoMatrix4x4ArrayInternal(_In_ uint8_t *pEffectData,
                                        _When_(IsSetting, _In_reads_bytes_(64 * Count))
                                        _When_(!IsSetting, _Out_writes_bytes_(64 * Count))
                                        void *pMatrixData,
                                        _In_ uint32_t Offset, _In_ uint32_t Count
#ifdef _DEBUG
                                        , _In_ const SType *pType, _In_ uint32_t  TotalUnpackedSize, _In_z_ LPCSTR pFuncName
#endif

                                        )
{    
    HRESULT hr = S_OK;

#ifdef _DEBUG
#pragma warning( suppress : 6001 )
    if (!AreBoundsValid(Offset, Count, pMatrixData, pType, TotalUnpackedSize))
    {
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }

    assert(pType->NumericType.IsColumnMajor == IsColumnMajor && pType->Stride == (4 * SType::c_RegisterSize));
#endif

    if ((IsColumnMajor && Transpose) || (!IsColumnMajor && !Transpose))
    {
        // fast path
        for (size_t i = 0; i < Count; ++ i)
        {
            CEffectMatrix *pMatrix = ((CEffectMatrix *)pMatrixData) + i;

            if (IsSetting)
            {
                Matrix4x4Copy(pMatrix, pEffectData + 4 * SType::c_RegisterSize * (i + Offset));
            }
            else
            {
                Matrix4x4Copy(pEffectData + 4 * SType::c_RegisterSize * (i + Offset), pMatrix);
            }
        }
    }
    else
    {
        // slow path
        for (size_t i = 0; i < Count; ++ i)
        {
            CEffectMatrix *pMatrix = ((CEffectMatrix *)pMatrixData) + i;

            if (IsSetting)
            {
                Matrix4x4TransposeHelper((float*) pMatrix, pEffectData + 4 * SType::c_RegisterSize * (i + Offset));
            }
            else
            {
                Matrix4x4TransposeHelper(pEffectData + 4 * SType::c_RegisterSize * (i + Offset), (float*) pMatrix);
            }
        }
    }

lExit:
    return hr;
}
#pragma warning (pop)

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::SetMatrix(const float *pData)
{
    DirtyVariable();
    return DoMatrix4x4ArrayInternal<IsColumnMajor, false, true>(Data.pNumeric, const_cast<float*>(pData), 0, 1
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::SetMatrix");
#else
        );
#endif
}

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::GetMatrix(float *pData)
{
    return DoMatrix4x4ArrayInternal<IsColumnMajor, false, false>(Data.pNumeric, pData, 0, 1
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::GetMatrix");
#else
        );
#endif
}

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::SetMatrixArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    DirtyVariable();
    return DoMatrix4x4ArrayInternal<IsColumnMajor, false, true>(Data.pNumeric, const_cast<float*>(pData), Offset, Count
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::SetMatrixArray");
#else
        );
#endif
}

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::GetMatrixArray(float *pData, uint32_t Offset, uint32_t Count)
{
    return DoMatrix4x4ArrayInternal<IsColumnMajor, false, false>(Data.pNumeric, pData, Offset, Count
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::GetMatrixArray");
#else
        );
#endif
}

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::SetMatrixTranspose(const float *pData)
{
    DirtyVariable();
    return DoMatrix4x4ArrayInternal<IsColumnMajor, true, true>(Data.pNumeric, const_cast<float*>(pData), 0, 1
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::SetMatrixTranspose");
#else
        );
#endif
}

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::GetMatrixTranspose(float *pData)
{
    return DoMatrix4x4ArrayInternal<IsColumnMajor, true, false>(Data.pNumeric, pData, 0, 1
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::GetMatrixTranspose");
#else
        );
#endif
}

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::SetMatrixTransposeArray(const float *pData, uint32_t Offset, uint32_t Count)
{
    DirtyVariable();
    return DoMatrix4x4ArrayInternal<IsColumnMajor, true, true>(Data.pNumeric, const_cast<float*>(pData), Offset, Count
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::SetMatrixTransposeArray");
#else
        );
#endif
}

template<typename IBaseInterface, bool IsColumnMajor>
_Use_decl_annotations_
HRESULT TMatrix4x4Variable<IBaseInterface, IsColumnMajor>::GetMatrixTransposeArray(float *pData, uint32_t Offset, uint32_t Count)
{
    return DoMatrix4x4ArrayInternal<IsColumnMajor, true, false>(Data.pNumeric, pData, Offset, Count
#ifdef _DEBUG 
        , pType, GetTotalUnpackedSize(), "ID3DX11EffectMatrixVariable::GetMatrixTransposeArray");
#else
        );
#endif
}

#ifdef _DEBUG

// Useful object macro to check bounds and parameters
#define CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, Pointer) \
    HRESULT hr = S_OK; \
    VERIFYPARAMETER(Pointer) \
    uint32_t elements = IsArray() ? pType->Elements : 1; \
    \
    if ((Offset + Count < Offset) || (elements < Offset + Count)) \
    { \
        DPF(0, "%s: Invalid range specified", pFuncName); \
        VH(E_INVALIDARG); \
    } \

#define CHECK_OBJECT_SCALAR_BOUNDS(Index, Pointer) \
    HRESULT hr = S_OK; \
    VERIFYPARAMETER(Pointer) \
    uint32_t elements = IsArray() ? pType->Elements : 1; \
    \
    if (Index >= elements) \
    { \
        DPF(0, "%s: Invalid index specified", pFuncName); \
        VH(E_INVALIDARG); \
    } \

#define CHECK_SCALAR_BOUNDS(Index) \
    HRESULT hr = S_OK; \
    uint32_t elements = IsArray() ? pType->Elements : 1; \
    \
    if (Index >= elements) \
{ \
    DPF(0, "%s: Invalid index specified", pFuncName); \
    VH(E_INVALIDARG); \
} \

#else // _DEBUG

#define CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, Pointer) \
    HRESULT hr = S_OK; \

#define CHECK_OBJECT_SCALAR_BOUNDS(Index, Pointer) \
    HRESULT hr = S_OK; \

#define CHECK_SCALAR_BOUNDS(Index) \
    HRESULT hr = S_OK; \

#endif // _DEBUG

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectStringVariable (TStringVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface, bool IsAnnotation>
struct TStringVariable : public IBaseInterface
{
    STDMETHOD(GetString)(_Outptr_result_z_ LPCSTR *ppString) override;
    STDMETHOD(GetStringArray)( _Out_writes_(Count) LPCSTR *ppStrings, _In_ uint32_t Offset, _In_ uint32_t Count ) override;
};

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
HRESULT TStringVariable<IBaseInterface, IsAnnotation>::GetString(LPCSTR *ppString)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectStringVariable::GetString";

    VERIFYPARAMETER(ppString);

    if (GetTopLevelEntity()->pEffect->IsOptimized())
    {
        DPF(0, "%s: Effect has been Optimize()'ed; all string/reflection data has been deleted", pFuncName);
        return D3DERR_INVALIDCALL;
    }

    assert(Data.pString != 0);
    _Analysis_assume_(Data.pString != 0);

    *ppString = Data.pString->pString;

lExit:
    return hr;
}

template<typename IBaseInterface, bool IsAnnotation>
_Use_decl_annotations_
#pragma warning(suppress : 6054)
HRESULT TStringVariable<IBaseInterface, IsAnnotation>::GetStringArray( LPCSTR *ppStrings, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectStringVariable::GetStringArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppStrings);

    if (GetTopLevelEntity()->pEffect->IsOptimized())
    {
        DPF(0, "%s: Effect has been Optimize()'ed; all string/reflection data has been deleted", pFuncName);
        return D3DERR_INVALIDCALL;
    }

    assert(Data.pString != 0);
    _Analysis_assume_(Data.pString != 0);

    for (size_t i = 0; i < Count; ++ i)
    {
        ppStrings[i] = (Data.pString + Offset + i)->pString;
    }

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectClassInstanceVariable (TClassInstanceVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TClassInstanceVariable : public IBaseInterface
{
    STDMETHOD(GetClassInstance)(_Outptr_ ID3D11ClassInstance **ppClassInstance) override;
};

template<typename IBaseClassInstance>
HRESULT TClassInstanceVariable<IBaseClassInstance>::GetClassInstance(_Outptr_ ID3D11ClassInstance** ppClassInstance)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectClassInstanceVariable::GetClassInstance";

    assert( pMemberData != 0 && pMemberData->Data.pD3DClassInstance != 0);
    _Analysis_assume_( pMemberData != 0 && pMemberData->Data.pD3DClassInstance != 0);
    *ppClassInstance = pMemberData->Data.pD3DClassInstance;
    SAFE_ADDREF(*ppClassInstance);

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectInterfaceeVariable (TInterfaceVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TInterfaceVariable : public IBaseInterface
{
    STDMETHOD(SetClassInstance)(_In_ ID3DX11EffectClassInstanceVariable *pEffectClassInstance) override;
    STDMETHOD(GetClassInstance)(_Outptr_ ID3DX11EffectClassInstanceVariable **ppEffectClassInstance) override;
};

template<typename IBaseInterface>
HRESULT TInterfaceVariable<IBaseInterface>::SetClassInstance(_In_ ID3DX11EffectClassInstanceVariable *pEffectClassInstance)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectInterfaceVariable::SetClassInstance";

    // Note that we don't check if the types are compatible.  The debug layer will complain if it is.
    // IsValid() will not catch type mismatches.
    SClassInstanceGlobalVariable* pCI = (SClassInstanceGlobalVariable*)pEffectClassInstance;
    Data.pInterface->pClassInstance = pCI;

lExit:
    return hr;
}

template<typename IBaseInterface>
HRESULT TInterfaceVariable<IBaseInterface>::GetClassInstance(_Outptr_ ID3DX11EffectClassInstanceVariable **ppEffectClassInstance)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectInterfaceVariable::GetClassInstance";

#ifdef _DEBUG
    VERIFYPARAMETER(ppEffectClassInstance);
#endif

    *ppEffectClassInstance = Data.pInterface->pClassInstance;

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectShaderResourceVariable (TShaderResourceVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TShaderResourceVariable : public IBaseInterface
{
    STDMETHOD(SetResource)(_In_ ID3D11ShaderResourceView *pResource) override;
    STDMETHOD(GetResource)(_Outptr_ ID3D11ShaderResourceView **ppResource) override;

    STDMETHOD(SetResourceArray)(_In_reads_(Count) ID3D11ShaderResourceView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetResourceArray)(_Out_writes_(Count) ID3D11ShaderResourceView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

static LPCSTR GetTextureTypeNameFromEnum(_In_ EObjectType ObjectType)
{
    switch (ObjectType)
    {
    case EOT_Buffer:
        return "Buffer";
    case EOT_Texture:
        return "texture";
    case EOT_Texture1D:
    case EOT_Texture1DArray:
        return "Texture1D";
    case EOT_Texture2DMS:
    case EOT_Texture2DMSArray:
        return "Texture2DMS";
    case EOT_Texture2D:
    case EOT_Texture2DArray:
        return "Texture2D";
    case EOT_Texture3D:
        return "Texture3D";
    case EOT_TextureCube:
        return "TextureCube";
    case EOT_TextureCubeArray:
        return "TextureCubeArray";
    case EOT_RWTexture1D:
    case EOT_RWTexture1DArray:
        return "RWTexture1D";
    case EOT_RWTexture2D:
    case EOT_RWTexture2DArray:
        return "RWTexture2D";
    case EOT_RWTexture3D:
        return "RWTexture3D";
    case EOT_RWBuffer:
        return "RWBuffer";
    case EOT_ByteAddressBuffer:
        return "ByteAddressBuffer";
    case EOT_RWByteAddressBuffer:
        return "RWByteAddressBuffer";
    case EOT_StructuredBuffer:
        return "StructuredBuffe";
    case EOT_RWStructuredBuffer:
        return "RWStructuredBuffer";
    case EOT_RWStructuredBufferAlloc:
        return "RWStructuredBufferAlloc";
    case EOT_RWStructuredBufferConsume:
        return "RWStructuredBufferConsume";
    case EOT_AppendStructuredBuffer:
        return "AppendStructuredBuffer";
    case EOT_ConsumeStructuredBuffer:
        return "ConsumeStructuredBuffer";
    }
    return "<unknown texture format>";
}

static LPCSTR GetResourceDimensionNameFromEnum(_In_ D3D11_RESOURCE_DIMENSION ResourceDimension)
{
    switch (ResourceDimension)
    {
    case D3D11_RESOURCE_DIMENSION_BUFFER:
        return "Buffer";
    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        return "Texture1D";
    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        return "Texture2D";
    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        return "Texture3D";
    }
    return "<unknown texture format>";
}

static LPCSTR GetSRVDimensionNameFromEnum(_In_ D3D11_SRV_DIMENSION ViewDimension)
{
    switch (ViewDimension)
    {
    case D3D11_SRV_DIMENSION_BUFFER:
    case D3D11_SRV_DIMENSION_BUFFEREX:
        return "Buffer";
    case D3D11_SRV_DIMENSION_TEXTURE1D:
        return "Texture1D";
    case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
        return "Texture1DArray";
    case D3D11_SRV_DIMENSION_TEXTURE2D:
        return "Texture2D";
    case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        return "Texture2DArray";
    case D3D11_SRV_DIMENSION_TEXTURE2DMS:
        return "Texture2DMS";
    case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
        return "Texture2DMSArray";
    case D3D11_SRV_DIMENSION_TEXTURE3D:
        return "Texture3D";
    case D3D11_SRV_DIMENSION_TEXTURECUBE:
        return "TextureCube";
    }
    return "<unknown texture format>";
}

static LPCSTR GetUAVDimensionNameFromEnum(_In_ D3D11_UAV_DIMENSION ViewDimension)
{
    switch (ViewDimension)
    {
    case D3D11_UAV_DIMENSION_BUFFER:
        return "Buffer";
    case D3D11_UAV_DIMENSION_TEXTURE1D:
        return "RWTexture1D";
    case D3D11_UAV_DIMENSION_TEXTURE1DARRAY:
        return "RWTexture1DArray";
    case D3D11_UAV_DIMENSION_TEXTURE2D:
        return "RWTexture2D";
    case D3D11_UAV_DIMENSION_TEXTURE2DARRAY:
        return "RWTexture2DArray";
    case D3D11_UAV_DIMENSION_TEXTURE3D:
        return "RWTexture3D";
    }
    return "<unknown texture format>";
}

static LPCSTR GetRTVDimensionNameFromEnum(_In_ D3D11_RTV_DIMENSION ViewDimension)
{
    switch (ViewDimension)
    {
    case D3D11_RTV_DIMENSION_BUFFER:
        return "Buffer";
    case D3D11_RTV_DIMENSION_TEXTURE1D:
        return "Texture1D";
    case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
        return "Texture1DArray";
    case D3D11_RTV_DIMENSION_TEXTURE2D:
        return "Texture2D";
    case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
        return "Texture2DArray";
    case D3D11_RTV_DIMENSION_TEXTURE2DMS:
        return "Texture2DMS";
    case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
        return "Texture2DMSArray";
    case D3D11_RTV_DIMENSION_TEXTURE3D:
        return "Texture3D";
    }
    return "<unknown texture format>";
}

static LPCSTR GetDSVDimensionNameFromEnum(_In_ D3D11_DSV_DIMENSION ViewDimension)
{
    switch (ViewDimension)
    {
    case D3D11_DSV_DIMENSION_TEXTURE1D:
        return "Texture1D";
    case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:
        return "Texture1DArray";
    case D3D11_DSV_DIMENSION_TEXTURE2D:
        return "Texture2D";
    case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
        return "Texture2DArray";
    case D3D11_DSV_DIMENSION_TEXTURE2DMS:
        return "Texture2DMS";
    case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
        return "Texture2DMSArray";
    }
    return "<unknown texture format>";
}

static HRESULT ValidateTextureType(_In_ ID3D11ShaderResourceView *pView, _In_ EObjectType ObjectType, _In_z_ LPCSTR pFuncName)
{
    if (nullptr != pView)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        pView->GetDesc(&desc);
        switch (ObjectType)
        {
        case EOT_Texture:
            if (desc.ViewDimension != D3D11_SRV_DIMENSION_BUFFER && desc.ViewDimension != D3D11_SRV_DIMENSION_BUFFEREX)
                return S_OK;
            break;
        case EOT_Buffer:
            if (desc.ViewDimension != D3D11_SRV_DIMENSION_BUFFER && desc.ViewDimension != D3D11_SRV_DIMENSION_BUFFEREX)
                break;
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX && (desc.BufferEx.Flags & D3D11_BUFFEREX_SRV_FLAG_RAW))
            {
                DPF(0, "%s: Resource type mismatch; %s expected, ByteAddressBuffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                return E_INVALIDARG;
            }
            else
            {
                ID3D11Buffer* pBuffer = nullptr;
                pView->GetResource( (ID3D11Resource**)&pBuffer );
                assert( pBuffer != nullptr );
                D3D11_BUFFER_DESC BufDesc;
                pBuffer->GetDesc( &BufDesc );
                SAFE_RELEASE( pBuffer );
                if( BufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
                {
                    DPF(0, "%s: Resource type mismatch; %s expected, StructuredBuffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                    return E_INVALIDARG;
                }
                else
                {
                    return S_OK;
                }
            }
            break;
        case EOT_Texture1D:
        case EOT_Texture1DArray:
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE1D || 
                desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE1DARRAY)
                return S_OK;
            break;
        case EOT_Texture2D:
        case EOT_Texture2DArray:
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D ||
                desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY)
                return S_OK;
            break;
        case EOT_Texture2DMS:
        case EOT_Texture2DMSArray:
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DMS ||
                desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY)
                return S_OK;
            break;
        case EOT_Texture3D:
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE3D)
                return S_OK;
            break;
        case EOT_TextureCube:
        case EOT_TextureCubeArray:
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBE ||
                desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBEARRAY)
                return S_OK;
            break;
        case EOT_ByteAddressBuffer:
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX && (desc.BufferEx.Flags & D3D11_BUFFEREX_SRV_FLAG_RAW))
                return S_OK;
            break;
        case EOT_StructuredBuffer:
            if (desc.ViewDimension == D3D11_SRV_DIMENSION_BUFFEREX || desc.ViewDimension == D3D11_SRV_DIMENSION_BUFFER)
            {
                ID3D11Buffer* pBuffer = nullptr;
                pView->GetResource( (ID3D11Resource**)&pBuffer );
                assert( pBuffer != nullptr );
                D3D11_BUFFER_DESC BufDesc;
                pBuffer->GetDesc( &BufDesc );
                SAFE_RELEASE( pBuffer );
                if( BufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
                {
                    return S_OK;
                }
                else
                {
                    DPF(0, "%s: Resource type mismatch; %s expected, non-structured Buffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                    return E_INVALIDARG;
                }
            }
            break;
        default:
            assert(0); // internal error, should never get here
            return E_FAIL;
        }
        

        DPF(0, "%s: Resource type mismatch; %s expected, %s provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType), GetSRVDimensionNameFromEnum(desc.ViewDimension));
        return E_INVALIDARG;
    }
    return S_OK;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderResourceVariable<IBaseInterface>::SetResource(ID3D11ShaderResourceView *pResource)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectShaderResourceVariable::SetResource";

    VH(ValidateTextureType(pResource, pType->ObjectType, pFuncName));
#endif

    // Texture variables don't need to be dirtied.
    SAFE_ADDREF(pResource);
    SAFE_RELEASE(Data.pShaderResource->pShaderResource);
    Data.pShaderResource->pShaderResource = pResource;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderResourceVariable<IBaseInterface>::GetResource(ID3D11ShaderResourceView **ppResource)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectShaderResourceVariable::GetResource";

    VERIFYPARAMETER(ppResource);
#endif

    assert(Data.pShaderResource != 0 && Data.pShaderResource->pShaderResource != 0);
    _Analysis_assume_(Data.pShaderResource != 0 && Data.pShaderResource->pShaderResource != 0);
    *ppResource = Data.pShaderResource->pShaderResource;
    SAFE_ADDREF(*ppResource);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderResourceVariable<IBaseInterface>::SetResourceArray(ID3D11ShaderResourceView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderResourceVariable::SetResourceArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

#ifdef _DEBUG
    for (size_t i = 0; i < Count; ++ i)
    {
        VH(ValidateTextureType(ppResources[i], pType->ObjectType, pFuncName));
    }
#endif

    // Texture variables don't need to be dirtied.
    for (size_t i = 0; i < Count; ++ i)
    {
        SShaderResource *pResourceBlock = Data.pShaderResource + Offset + i;
        SAFE_ADDREF(ppResources[i]);
        SAFE_RELEASE(pResourceBlock->pShaderResource);
        pResourceBlock->pShaderResource = ppResources[i];
    }

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderResourceVariable<IBaseInterface>::GetResourceArray(ID3D11ShaderResourceView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderResourceVariable::GetResourceArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

    for (size_t i = 0; i < Count; ++ i)
    {
        ppResources[i] = (Data.pShaderResource + Offset + i)->pShaderResource;
        SAFE_ADDREF(ppResources[i]);
    }

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectUnorderedAccessViewVariable (TUnorderedAccessViewVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TUnorderedAccessViewVariable : public IBaseInterface
{
    STDMETHOD(SetUnorderedAccessView)(_In_ ID3D11UnorderedAccessView *pResource) override;
    STDMETHOD(GetUnorderedAccessView)(_Outptr_ ID3D11UnorderedAccessView **ppResource) override;

    STDMETHOD(SetUnorderedAccessViewArray)(_In_reads_(Count) ID3D11UnorderedAccessView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetUnorderedAccessViewArray)(_Out_writes_(Count) ID3D11UnorderedAccessView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};

static HRESULT ValidateTextureType(_In_ ID3D11UnorderedAccessView *pView, _In_ EObjectType ObjectType, _In_z_ LPCSTR pFuncName)
{
    if (nullptr != pView)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
        pView->GetDesc(&desc);
        switch (ObjectType)
        {
        case EOT_RWBuffer:
            if (desc.ViewDimension != D3D11_UAV_DIMENSION_BUFFER)
                break;
            if (desc.Buffer.Flags & D3D11_BUFFER_UAV_FLAG_RAW)
            {
                DPF(0, "%s: Resource type mismatch; %s expected, RWByteAddressBuffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                return E_INVALIDARG;
            }
            else
            {
                ID3D11Buffer* pBuffer = nullptr;
                pView->GetResource( (ID3D11Resource**)&pBuffer );
                assert( pBuffer != nullptr );
                D3D11_BUFFER_DESC BufDesc;
                pBuffer->GetDesc( &BufDesc );
                SAFE_RELEASE( pBuffer );
                if( BufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
                {
                    DPF(0, "%s: Resource type mismatch; %s expected, an RWStructuredBuffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                    return E_INVALIDARG;
                }
                else
                {
                    return S_OK;
                }
            }
            break;
        case EOT_RWTexture1D:
        case EOT_RWTexture1DArray:
            if (desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE1D || 
                desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE1DARRAY)
                return S_OK;
            break;
        case EOT_RWTexture2D:
        case EOT_RWTexture2DArray:
            if (desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE2D ||
                desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE2DARRAY)
                return S_OK;
            break;
        case EOT_RWTexture3D:
            if (desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE3D)
                return S_OK;
            break;
        case EOT_RWByteAddressBuffer:
            if (desc.ViewDimension == D3D11_UAV_DIMENSION_BUFFER && (desc.Buffer.Flags & D3D11_BUFFER_UAV_FLAG_RAW))
                return S_OK;
            break;
        case EOT_RWStructuredBuffer:
            if (desc.ViewDimension == D3D11_UAV_DIMENSION_BUFFER)
            {
                ID3D11Buffer* pBuffer = nullptr;
                pView->GetResource( (ID3D11Resource**)&pBuffer );
                assert( pBuffer != nullptr );
                D3D11_BUFFER_DESC BufDesc;
                pBuffer->GetDesc( &BufDesc );
                SAFE_RELEASE( pBuffer );
                if( BufDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
                {
                    return S_OK;
                }
                else
                {
                    DPF(0, "%s: Resource type mismatch; %s expected, non-structured Buffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                    return E_INVALIDARG;
                }
            }
            break;
        case EOT_RWStructuredBufferAlloc:
        case EOT_RWStructuredBufferConsume:
            if (desc.ViewDimension != D3D11_UAV_DIMENSION_BUFFER)
                break;
            if (desc.Buffer.Flags & D3D11_BUFFER_UAV_FLAG_COUNTER)
            {
                return S_OK;
            }
            else
            {
                DPF(0, "%s: Resource type mismatch; %s expected, non-Counter buffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                return E_INVALIDARG;
            }
            break;
        case EOT_AppendStructuredBuffer:
        case EOT_ConsumeStructuredBuffer:
            if (desc.ViewDimension != D3D11_UAV_DIMENSION_BUFFER)
                break;
            if (desc.Buffer.Flags & D3D11_BUFFER_UAV_FLAG_APPEND)
            {
                return S_OK;
            }
            else
            {
                DPF(0, "%s: Resource type mismatch; %s expected, non-Append buffer provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType));
                return E_INVALIDARG;
            }
            break;
        default:
            assert(0); // internal error, should never get here
            return E_FAIL;
        }


        DPF(0, "%s: Resource type mismatch; %s expected, %s provided.", pFuncName, GetTextureTypeNameFromEnum(ObjectType), GetUAVDimensionNameFromEnum(desc.ViewDimension));
        return E_INVALIDARG;
    }
    return S_OK;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TUnorderedAccessViewVariable<IBaseInterface>::SetUnorderedAccessView(ID3D11UnorderedAccessView *pResource)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectUnorderedAccessViewVariable::SetUnorderedAccessView";

    VH(ValidateTextureType(pResource, pType->ObjectType, pFuncName));
#endif

    // UAV variables don't need to be dirtied.
    SAFE_ADDREF(pResource);
    SAFE_RELEASE(Data.pUnorderedAccessView->pUnorderedAccessView);
    Data.pUnorderedAccessView->pUnorderedAccessView = pResource;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TUnorderedAccessViewVariable<IBaseInterface>::GetUnorderedAccessView(ID3D11UnorderedAccessView **ppResource)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectUnorderedAccessViewVariable::GetUnorderedAccessView";

    VERIFYPARAMETER(ppResource);
#endif

    assert(Data.pUnorderedAccessView != 0 && Data.pUnorderedAccessView->pUnorderedAccessView != 0);
    _Analysis_assume_(Data.pUnorderedAccessView != 0 && Data.pUnorderedAccessView->pUnorderedAccessView != 0);
    *ppResource = Data.pUnorderedAccessView->pUnorderedAccessView;
    SAFE_ADDREF(*ppResource);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TUnorderedAccessViewVariable<IBaseInterface>::SetUnorderedAccessViewArray(ID3D11UnorderedAccessView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectUnorderedAccessViewVariable::SetUnorderedAccessViewArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

#ifdef _DEBUG
    for (size_t i = 0; i < Count; ++ i)
    {
        VH(ValidateTextureType(ppResources[i], pType->ObjectType, pFuncName));
    }
#endif

    // Texture variables don't need to be dirtied.
    for (size_t i = 0; i < Count; ++ i)
    {
        SUnorderedAccessView *pResourceBlock = Data.pUnorderedAccessView + Offset + i;
        SAFE_ADDREF(ppResources[i]);
        SAFE_RELEASE(pResourceBlock->pUnorderedAccessView);
        pResourceBlock->pUnorderedAccessView = ppResources[i];
    }

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TUnorderedAccessViewVariable<IBaseInterface>::GetUnorderedAccessViewArray(ID3D11UnorderedAccessView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectUnorderedAccessViewVariable::GetUnorderedAccessViewArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

    for (size_t i = 0; i < Count; ++ i)
    {
        ppResources[i] = (Data.pUnorderedAccessView + Offset + i)->pUnorderedAccessView;
        SAFE_ADDREF(ppResources[i]);
    }

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectRenderTargetViewVariable (TRenderTargetViewVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TRenderTargetViewVariable : public IBaseInterface
{
    STDMETHOD(SetRenderTarget)(_In_ ID3D11RenderTargetView *pResource) override;
    STDMETHOD(GetRenderTarget)(_Outptr_ ID3D11RenderTargetView **ppResource) override;

    STDMETHOD(SetRenderTargetArray)(_In_reads_(Count) ID3D11RenderTargetView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetRenderTargetArray)(_Out_writes_(Count) ID3D11RenderTargetView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count) override;
};


template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRenderTargetViewVariable<IBaseInterface>::SetRenderTarget(ID3D11RenderTargetView *pResource)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectRenderTargetVariable::SetRenderTarget";
#endif

    // Texture variables don't need to be dirtied.
    SAFE_ADDREF(pResource);
    SAFE_RELEASE(Data.pRenderTargetView->pRenderTargetView);
    Data.pRenderTargetView->pRenderTargetView = pResource;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRenderTargetViewVariable<IBaseInterface>::GetRenderTarget(ID3D11RenderTargetView **ppResource)
{
    HRESULT hr = S_OK;

    assert(Data.pRenderTargetView->pRenderTargetView != 0);
    _Analysis_assume_(Data.pRenderTargetView->pRenderTargetView != 0);
    *ppResource = Data.pRenderTargetView->pRenderTargetView;
    SAFE_ADDREF(*ppResource);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRenderTargetViewVariable<IBaseInterface>::SetRenderTargetArray(ID3D11RenderTargetView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectRenderTargetVariable::SetRenderTargetArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

    // Texture variables don't need to be dirtied.
    for (size_t i = 0; i < Count; ++ i)
    {
        SRenderTargetView *pResourceBlock = Data.pRenderTargetView + Offset + i;
        SAFE_ADDREF(ppResources[i]);
        SAFE_RELEASE(pResourceBlock->pRenderTargetView);
        pResourceBlock->pRenderTargetView = ppResources[i];
    }

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRenderTargetViewVariable<IBaseInterface>::GetRenderTargetArray(ID3D11RenderTargetView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3DX11EffectRenderTargetVariable::GetRenderTargetArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

    for (size_t i = 0; i < Count; ++ i)
    {
        ppResources[i] = (Data.pRenderTargetView + Offset + i)->pRenderTargetView;
        SAFE_ADDREF(ppResources[i]);
    }

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectDepthStencilViewVariable (TDepthStencilViewVariable implementation)
//////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TDepthStencilViewVariable : public IBaseInterface
{
    STDMETHOD(SetDepthStencil)(_In_ ID3D11DepthStencilView *pResource)  override;
    STDMETHOD(GetDepthStencil)(_Outptr_ ID3D11DepthStencilView **ppResource)  override;

    STDMETHOD(SetDepthStencilArray)(_In_reads_(Count) ID3D11DepthStencilView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count)  override;
    STDMETHOD(GetDepthStencilArray)(_Out_writes_(Count) ID3D11DepthStencilView **ppResources, _In_ uint32_t Offset, _In_ uint32_t Count)  override;
};


template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TDepthStencilViewVariable<IBaseInterface>::SetDepthStencil(ID3D11DepthStencilView *pResource)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3D11DepthStencilViewVariable::SetDepthStencil";

#endif

    // Texture variables don't need to be dirtied.
    SAFE_ADDREF(pResource);
    SAFE_RELEASE(Data.pDepthStencilView->pDepthStencilView);
    Data.pDepthStencilView->pDepthStencilView = pResource;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TDepthStencilViewVariable<IBaseInterface>::GetDepthStencil(ID3D11DepthStencilView **ppResource)
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3D11DepthStencilViewVariable::GetDepthStencil";

    VERIFYPARAMETER(ppResource);
#endif

    assert(Data.pDepthStencilView->pDepthStencilView != 0);
    _Analysis_assume_(Data.pDepthStencilView->pDepthStencilView != 0);
    *ppResource = Data.pDepthStencilView->pDepthStencilView;
    SAFE_ADDREF(*ppResource);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TDepthStencilViewVariable<IBaseInterface>::SetDepthStencilArray(ID3D11DepthStencilView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3D11DepthStencilViewVariable::SetDepthStencilArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

    // Texture variables don't need to be dirtied.
    for (size_t i = 0; i < Count; ++ i)
    {
        SDepthStencilView *pResourceBlock = Data.pDepthStencilView + Offset + i;
        SAFE_ADDREF(ppResources[i]);
        SAFE_RELEASE(pResourceBlock->pDepthStencilView);
        pResourceBlock->pDepthStencilView = ppResources[i];
    }

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TDepthStencilViewVariable<IBaseInterface>::GetDepthStencilArray(ID3D11DepthStencilView **ppResources, uint32_t Offset, uint32_t Count)
{
    static LPCSTR pFuncName = "ID3D11DepthStencilViewVariable::GetDepthStencilArray";

    CHECK_OBJECT_ARRAY_BOUNDS(Offset, Count, ppResources);

    for (size_t i = 0; i < Count; ++ i)
    {
        ppResources[i] = (Data.pDepthStencilView + Offset + i)->pDepthStencilView;
        SAFE_ADDREF(ppResources[i]);
    }

lExit:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectShaderVariable (TShaderVariable implementation)
////////////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TShaderVariable : public IBaseInterface
{
    STDMETHOD(GetShaderDesc)(_In_ uint32_t ShaderIndex, _Out_ D3DX11_EFFECT_SHADER_DESC *pDesc)  override;

    STDMETHOD(GetVertexShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11VertexShader **ppVS)  override;
    STDMETHOD(GetGeometryShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11GeometryShader **ppGS)  override;
    STDMETHOD(GetPixelShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11PixelShader **ppPS)  override;
    STDMETHOD(GetHullShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11HullShader **ppHS)  override;
    STDMETHOD(GetDomainShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11DomainShader **ppDS)  override;
    STDMETHOD(GetComputeShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11ComputeShader **ppCS)  override;

    STDMETHOD(GetInputSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc)  override;
    STDMETHOD(GetOutputSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc)  override;
    STDMETHOD(GetPatchConstantSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc)  override;

    STDMETHOD_(bool, IsValid)();
};

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetShaderDesc(uint32_t ShaderIndex, D3DX11_EFFECT_SHADER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetShaderDesc";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, pDesc);

    hr = Data.pShader[ShaderIndex].GetShaderDesc(pDesc, false);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetVertexShader(uint32_t ShaderIndex, ID3D11VertexShader **ppVS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetVertexShader";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, ppVS);

    VH( Data.pShader[ShaderIndex].GetVertexShader(ppVS) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetGeometryShader(uint32_t ShaderIndex, ID3D11GeometryShader **ppGS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetGeometryShader";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, ppGS);

    VH( Data.pShader[ShaderIndex].GetGeometryShader(ppGS) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetPixelShader(uint32_t ShaderIndex, ID3D11PixelShader **ppPS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetPixelShader";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, ppPS);

    VH( Data.pShader[ShaderIndex].GetPixelShader(ppPS) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetHullShader(uint32_t ShaderIndex, ID3D11HullShader **ppHS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetHullShader";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, ppHS);

    VH( Data.pShader[ShaderIndex].GetHullShader(ppHS) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetDomainShader(uint32_t ShaderIndex, ID3D11DomainShader **ppDS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetDomainShader";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, ppDS);

    VH( Data.pShader[ShaderIndex].GetDomainShader(ppDS) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetComputeShader(uint32_t ShaderIndex, ID3D11ComputeShader **ppCS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetComputeShader";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, ppCS);

    VH( Data.pShader[ShaderIndex].GetComputeShader(ppCS) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetInputSignatureElementDesc(uint32_t ShaderIndex, uint32_t Element, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetInputSignatureElementDesc";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, pDesc);

    VH( Data.pShader[ShaderIndex].GetSignatureElementDesc(SShaderBlock::ST_Input, Element, pDesc) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetOutputSignatureElementDesc(uint32_t ShaderIndex, uint32_t Element, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetOutputSignatureElementDesc";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, pDesc);

    VH( Data.pShader[ShaderIndex].GetSignatureElementDesc(SShaderBlock::ST_Output, Element, pDesc) );

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TShaderVariable<IBaseInterface>::GetPatchConstantSignatureElementDesc(uint32_t ShaderIndex, uint32_t Element, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetPatchConstantSignatureElementDesc";

    CHECK_OBJECT_SCALAR_BOUNDS(ShaderIndex, pDesc);

    VH( Data.pShader[ShaderIndex].GetSignatureElementDesc(SShaderBlock::ST_PatchConstant, Element, pDesc) );

lExit:
    return hr;
}

template<typename IBaseInterface>
bool TShaderVariable<IBaseInterface>::IsValid()
{
    uint32_t numElements = IsArray()? pType->Elements : 1;
    bool valid = true;
    while( numElements > 0 && ( valid = Data.pShader[ numElements-1 ].IsValid ) )
        numElements--;
    return valid;
}

////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectBlendVariable (TBlendVariable implementation)
////////////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TBlendVariable : public IBaseInterface
{
public:
    STDMETHOD(GetBlendState)(_In_ uint32_t Index, _Outptr_ ID3D11BlendState **ppState)  override;
    STDMETHOD(SetBlendState)(_In_ uint32_t Index, _In_ ID3D11BlendState *pState)  override;
    STDMETHOD(UndoSetBlendState)(_In_ uint32_t Index)  override;
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_BLEND_DESC *pDesc)  override;
    STDMETHOD_(bool, IsValid)()  override;
};

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TBlendVariable<IBaseInterface>::GetBlendState(uint32_t Index, ID3D11BlendState **ppState)
{
    static LPCSTR pFuncName = "ID3DX11EffectBlendVariable::GetBlendState";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, ppState);

    assert(Data.pBlend[Index].pBlendObject != 0);
    _Analysis_assume_(Data.pBlend[Index].pBlendObject != 0);
    *ppState = Data.pBlend[Index].pBlendObject;
    SAFE_ADDREF(*ppState);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TBlendVariable<IBaseInterface>::SetBlendState(uint32_t Index, ID3D11BlendState *pState)
{
    static LPCSTR pFuncName = "ID3DX11EffectBlendState::SetBlendState";

    CHECK_SCALAR_BOUNDS(Index);

    if( !Data.pBlend[Index].IsUserManaged )
    {
        // Save original state object in case we UndoSet
        assert( pMemberData[Index].Type == MDT_BlendState );
        VB( pMemberData[Index].Data.pD3DEffectsManagedBlendState == nullptr );
        pMemberData[Index].Data.pD3DEffectsManagedBlendState = Data.pBlend[Index].pBlendObject;
        Data.pBlend[Index].pBlendObject = nullptr;
        Data.pBlend[Index].IsUserManaged = true;
    }

    SAFE_ADDREF( pState );
    SAFE_RELEASE( Data.pBlend[Index].pBlendObject );
    Data.pBlend[Index].pBlendObject = pState;
    Data.pBlend[Index].IsValid = true;
lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TBlendVariable<IBaseInterface>::UndoSetBlendState(uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectBlendState::UndoSetBlendState";

    CHECK_SCALAR_BOUNDS(Index);

    if( !Data.pBlend[Index].IsUserManaged )
    {
        return S_FALSE;
    }

    // Revert to original state object
    SAFE_RELEASE( Data.pBlend[Index].pBlendObject );
    Data.pBlend[Index].pBlendObject = pMemberData[Index].Data.pD3DEffectsManagedBlendState;
    pMemberData[Index].Data.pD3DEffectsManagedBlendState = nullptr;
    Data.pBlend[Index].IsUserManaged = false;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TBlendVariable<IBaseInterface>::GetBackingStore(uint32_t Index, D3D11_BLEND_DESC *pBlendDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectBlendVariable::GetBackingStore";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, pBlendDesc);

    if( Data.pBlend[Index].IsUserManaged )
    {
        if( Data.pBlend[Index].pBlendObject )
        {
            Data.pBlend[Index].pBlendObject->GetDesc( pBlendDesc );
        }
        else
        {
            *pBlendDesc = CD3D11_BLEND_DESC( D3D11_DEFAULT );
        }
    }
    else
    {
        SBlendBlock *pBlock = Data.pBlend + Index;
        if (pBlock->ApplyAssignments(GetTopLevelEntity()->pEffect))
        {
            pBlock->pAssignments[0].LastRecomputedTime = 0; // Force a recreate of this block the next time ApplyRenderStateBlock is called
        }

        memcpy( pBlendDesc, &pBlock->BackingStore, sizeof(D3D11_BLEND_DESC) );
    }

lExit:
    return hr;
}

template<typename IBaseInterface>
bool TBlendVariable<IBaseInterface>::IsValid()
{
    uint32_t numElements = IsArray()? pType->Elements : 1;
    bool valid = true;
    while( numElements > 0 && ( valid = Data.pBlend[ numElements-1 ].IsValid ) )
        numElements--;
    return valid;
}


////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectDepthStencilVariable (TDepthStencilVariable implementation)
////////////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TDepthStencilVariable : public IBaseInterface
{
public:
    STDMETHOD(GetDepthStencilState)(_In_ uint32_t Index, _Outptr_ ID3D11DepthStencilState **ppState)  override;
    STDMETHOD(SetDepthStencilState)(_In_ uint32_t Index, _In_ ID3D11DepthStencilState *pState)  override;
    STDMETHOD(UndoSetDepthStencilState)(_In_ uint32_t Index)  override;
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_DEPTH_STENCIL_DESC *pDesc) override;
    STDMETHOD_(bool, IsValid)()  override;
};

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TDepthStencilVariable<IBaseInterface>::GetDepthStencilState(uint32_t Index, ID3D11DepthStencilState **ppState)
{
    static LPCSTR pFuncName = "ID3DX11EffectDepthStencilVariable::GetDepthStencilState";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, ppState);

    assert(Data.pDepthStencil[Index].pDSObject != 0);
    _Analysis_assume_(Data.pDepthStencil[Index].pDSObject != 0);
    *ppState = Data.pDepthStencil[Index].pDSObject;
    SAFE_ADDREF(*ppState);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TDepthStencilVariable<IBaseInterface>::SetDepthStencilState(uint32_t Index, ID3D11DepthStencilState *pState)
{
    static LPCSTR pFuncName = "ID3DX11EffectDepthStencilState::SetDepthStencilState";

    CHECK_SCALAR_BOUNDS(Index);

    if( !Data.pDepthStencil[Index].IsUserManaged )
    {
        // Save original state object in case we UndoSet
        assert( pMemberData[Index].Type == MDT_DepthStencilState );
        VB( pMemberData[Index].Data.pD3DEffectsManagedDepthStencilState == nullptr );
        pMemberData[Index].Data.pD3DEffectsManagedDepthStencilState = Data.pDepthStencil[Index].pDSObject;
        Data.pDepthStencil[Index].pDSObject = nullptr;
        Data.pDepthStencil[Index].IsUserManaged = true;
    }

    SAFE_ADDREF( pState );
    SAFE_RELEASE( Data.pDepthStencil[Index].pDSObject );
    Data.pDepthStencil[Index].pDSObject = pState;
    Data.pDepthStencil[Index].IsValid = true;
lExit:
    return hr;
}

template<typename IBaseInterface>
HRESULT TDepthStencilVariable<IBaseInterface>::UndoSetDepthStencilState(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectDepthStencilState::UndoSetDepthStencilState";

    CHECK_SCALAR_BOUNDS(Index);

    if( !Data.pDepthStencil[Index].IsUserManaged )
    {
        return S_FALSE;
    }

    // Revert to original state object
    SAFE_RELEASE( Data.pDepthStencil[Index].pDSObject );
    Data.pDepthStencil[Index].pDSObject = pMemberData[Index].Data.pD3DEffectsManagedDepthStencilState;
    pMemberData[Index].Data.pD3DEffectsManagedDepthStencilState = nullptr;
    Data.pDepthStencil[Index].IsUserManaged = false;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TDepthStencilVariable<IBaseInterface>::GetBackingStore(uint32_t Index, D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectDepthStencilVariable::GetBackingStore";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, pDepthStencilDesc);

    if( Data.pDepthStencil[Index].IsUserManaged )
    {
        if( Data.pDepthStencil[Index].pDSObject )
        {
            Data.pDepthStencil[Index].pDSObject->GetDesc( pDepthStencilDesc );
        }
        else
        {
            *pDepthStencilDesc = CD3D11_DEPTH_STENCIL_DESC( D3D11_DEFAULT );
        }
    }
    else
    {
        SDepthStencilBlock *pBlock = Data.pDepthStencil + Index;
        if (pBlock->ApplyAssignments(GetTopLevelEntity()->pEffect))
        {
            pBlock->pAssignments[0].LastRecomputedTime = 0; // Force a recreate of this block the next time ApplyRenderStateBlock is called
        }

        memcpy(pDepthStencilDesc, &pBlock->BackingStore, sizeof(D3D11_DEPTH_STENCIL_DESC));
    }

lExit:
    return hr;
}

template<typename IBaseInterface>
bool TDepthStencilVariable<IBaseInterface>::IsValid()
{
    uint32_t numElements = IsArray()? pType->Elements : 1;
    bool valid = true;
    while( numElements > 0 && ( valid = Data.pDepthStencil[ numElements-1 ].IsValid ) )
        numElements--;
    return valid;
}

////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectRasterizerVariable (TRasterizerVariable implementation)
////////////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TRasterizerVariable : public IBaseInterface
{
public:

    STDMETHOD(GetRasterizerState)(_In_ uint32_t Index, _Outptr_ ID3D11RasterizerState **ppState)  override;
    STDMETHOD(SetRasterizerState)(_In_ uint32_t Index, _In_ ID3D11RasterizerState *pState)  override;
    STDMETHOD(UndoSetRasterizerState)(_In_ uint32_t Index)  override;
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_RASTERIZER_DESC *pDesc)  override;
    STDMETHOD_(bool, IsValid)()  override;
};

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRasterizerVariable<IBaseInterface>::GetRasterizerState(uint32_t Index, ID3D11RasterizerState **ppState)
{
    static LPCSTR pFuncName = "ID3DX11EffectRasterizerVariable::GetRasterizerState";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, ppState);

    assert(Data.pRasterizer[Index].pRasterizerObject != 0);
    _Analysis_assume_(Data.pRasterizer[Index].pRasterizerObject != 0);
    *ppState = Data.pRasterizer[Index].pRasterizerObject;
    SAFE_ADDREF(*ppState);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRasterizerVariable<IBaseInterface>::SetRasterizerState(uint32_t Index, ID3D11RasterizerState *pState)
{
    static LPCSTR pFuncName = "ID3DX11EffectRasterizerState::SetRasterizerState";

    CHECK_SCALAR_BOUNDS(Index);

    if( !Data.pRasterizer[Index].IsUserManaged )
    {
        // Save original state object in case we UndoSet
        assert( pMemberData[Index].Type == MDT_RasterizerState );
        VB( pMemberData[Index].Data.pD3DEffectsManagedRasterizerState == nullptr );
        pMemberData[Index].Data.pD3DEffectsManagedRasterizerState = Data.pRasterizer[Index].pRasterizerObject;
        Data.pRasterizer[Index].pRasterizerObject = nullptr;
        Data.pRasterizer[Index].IsUserManaged = true;
    }

    SAFE_ADDREF( pState );
    SAFE_RELEASE( Data.pRasterizer[Index].pRasterizerObject );
    Data.pRasterizer[Index].pRasterizerObject = pState;
    Data.pRasterizer[Index].IsValid = true;
lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRasterizerVariable<IBaseInterface>::UndoSetRasterizerState(uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectRasterizerState::UndoSetRasterizerState";

    CHECK_SCALAR_BOUNDS(Index);

    if( !Data.pRasterizer[Index].IsUserManaged )
    {
        return S_FALSE;
    }

    // Revert to original state object
    SAFE_RELEASE( Data.pRasterizer[Index].pRasterizerObject );
    Data.pRasterizer[Index].pRasterizerObject = pMemberData[Index].Data.pD3DEffectsManagedRasterizerState;
    pMemberData[Index].Data.pD3DEffectsManagedRasterizerState = nullptr;
    Data.pRasterizer[Index].IsUserManaged = false;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TRasterizerVariable<IBaseInterface>::GetBackingStore(uint32_t Index, D3D11_RASTERIZER_DESC *pRasterizerDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectRasterizerVariable::GetBackingStore";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, pRasterizerDesc);

    if( Data.pRasterizer[Index].IsUserManaged )
    {
        if( Data.pRasterizer[Index].pRasterizerObject )
        {
            Data.pRasterizer[Index].pRasterizerObject->GetDesc( pRasterizerDesc );
        }
        else
        {
            *pRasterizerDesc = CD3D11_RASTERIZER_DESC( D3D11_DEFAULT );
        }
    }
    else
    {
        SRasterizerBlock *pBlock = Data.pRasterizer + Index;
        if (pBlock->ApplyAssignments(GetTopLevelEntity()->pEffect))
        {
            pBlock->pAssignments[0].LastRecomputedTime = 0; // Force a recreate of this block the next time ApplyRenderStateBlock is called
        }

        memcpy(pRasterizerDesc, &pBlock->BackingStore, sizeof(D3D11_RASTERIZER_DESC));
    }

lExit:
    return hr;
}

template<typename IBaseInterface>
bool TRasterizerVariable<IBaseInterface>::IsValid()
{
    uint32_t numElements = IsArray()? pType->Elements : 1;
    bool valid = true;
    while( numElements > 0 && ( valid = Data.pRasterizer[ numElements-1 ].IsValid ) )
        numElements--;
    return valid;
}

////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectSamplerVariable (TSamplerVariable implementation)
////////////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TSamplerVariable : public IBaseInterface
{
public:

    STDMETHOD(GetSampler)(_In_ uint32_t Index, _Outptr_ ID3D11SamplerState **ppSampler) override;
    STDMETHOD(SetSampler)(_In_ uint32_t Index, _In_ ID3D11SamplerState *pSampler) override;
    STDMETHOD(UndoSetSampler)(_In_ uint32_t Index)  override;
    STDMETHOD(GetBackingStore)(_In_ uint32_t Index, _Out_ D3D11_SAMPLER_DESC *pDesc)  override;
};

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TSamplerVariable<IBaseInterface>::GetSampler(uint32_t Index, ID3D11SamplerState **ppSampler)
{
    static LPCSTR pFuncName = "ID3DX11EffectSamplerVariable::GetSampler";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, ppSampler);

    _Analysis_assume_( Data.pSampler[Index].pD3DObject != 0 );
    *ppSampler = Data.pSampler[Index].pD3DObject;
    SAFE_ADDREF(*ppSampler);

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TSamplerVariable<IBaseInterface>::SetSampler(uint32_t Index, ID3D11SamplerState *pSampler)
{
    static LPCSTR pFuncName = "ID3DX11EffectSamplerState::SetSampler";

    CHECK_SCALAR_BOUNDS(Index);

    // Replace all references to the old shader block with this one
    GetEffect()->ReplaceSamplerReference(&Data.pSampler[Index], pSampler);

    if( !Data.pSampler[Index].IsUserManaged )
    {
        // Save original state object in case we UndoSet
        assert( pMemberData[Index].Type == MDT_SamplerState );
        VB( pMemberData[Index].Data.pD3DEffectsManagedSamplerState == nullptr );
        pMemberData[Index].Data.pD3DEffectsManagedSamplerState = Data.pSampler[Index].pD3DObject;
        Data.pSampler[Index].pD3DObject = nullptr;
        Data.pSampler[Index].IsUserManaged = true;
    }

    SAFE_ADDREF( pSampler );
    SAFE_RELEASE( Data.pSampler[Index].pD3DObject );
    Data.pSampler[Index].pD3DObject = pSampler;
lExit:
    return hr;
}

template<typename IBaseInterface>
HRESULT TSamplerVariable<IBaseInterface>::UndoSetSampler(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectSamplerState::UndoSetSampler";

    CHECK_SCALAR_BOUNDS(Index);

    if( !Data.pSampler[Index].IsUserManaged )
    {
        return S_FALSE;
    }

    // Replace all references to the old shader block with this one
    GetEffect()->ReplaceSamplerReference(&Data.pSampler[Index], pMemberData[Index].Data.pD3DEffectsManagedSamplerState);

    // Revert to original state object
    SAFE_RELEASE( Data.pSampler[Index].pD3DObject );
    Data.pSampler[Index].pD3DObject = pMemberData[Index].Data.pD3DEffectsManagedSamplerState;
    pMemberData[Index].Data.pD3DEffectsManagedSamplerState = nullptr;
    Data.pSampler[Index].IsUserManaged = false;

lExit:
    return hr;
}

template<typename IBaseInterface>
_Use_decl_annotations_
HRESULT TSamplerVariable<IBaseInterface>::GetBackingStore(uint32_t Index, D3D11_SAMPLER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectSamplerVariable::GetBackingStore";

    CHECK_OBJECT_SCALAR_BOUNDS(Index, pDesc);

    if( Data.pSampler[Index].IsUserManaged )
    {
        if( Data.pSampler[Index].pD3DObject )
        {
            Data.pSampler[Index].pD3DObject->GetDesc( pDesc );
        }
        else
        {
            *pDesc = CD3D11_SAMPLER_DESC( D3D11_DEFAULT );
        }
    }
    else
    {
        SSamplerBlock *pBlock = Data.pSampler + Index;
        if (pBlock->ApplyAssignments(GetTopLevelEntity()->pEffect))
        {
            pBlock->pAssignments[0].LastRecomputedTime = 0; // Force a recreate of this block the next time ApplyRenderStateBlock is called
        }

        memcpy(pDesc, &pBlock->BackingStore.SamplerDesc, sizeof(D3D11_SAMPLER_DESC));
    }

lExit:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// TUncastableVariable
////////////////////////////////////////////////////////////////////////////////

template<typename IBaseInterface>
struct TUncastableVariable : public IBaseInterface
{
    STDMETHOD_(ID3DX11EffectScalarVariable*, AsScalar)() override;
    STDMETHOD_(ID3DX11EffectVectorVariable*, AsVector)() override;
    STDMETHOD_(ID3DX11EffectMatrixVariable*, AsMatrix)() override;
    STDMETHOD_(ID3DX11EffectStringVariable*, AsString)() override;
    STDMETHOD_(ID3DX11EffectClassInstanceVariable*, AsClassInstance)() override;
    STDMETHOD_(ID3DX11EffectInterfaceVariable*, AsInterface)() override;
    STDMETHOD_(ID3DX11EffectShaderResourceVariable*, AsShaderResource)() override;
    STDMETHOD_(ID3DX11EffectUnorderedAccessViewVariable*, AsUnorderedAccessView)() override;
    STDMETHOD_(ID3DX11EffectRenderTargetViewVariable*, AsRenderTargetView)() override;
    STDMETHOD_(ID3DX11EffectDepthStencilViewVariable*, AsDepthStencilView)() override;
    STDMETHOD_(ID3DX11EffectConstantBuffer*, AsConstantBuffer)() override;
    STDMETHOD_(ID3DX11EffectShaderVariable*, AsShader)() override;
    STDMETHOD_(ID3DX11EffectBlendVariable*, AsBlend)() override;
    STDMETHOD_(ID3DX11EffectDepthStencilVariable*, AsDepthStencil)() override;
    STDMETHOD_(ID3DX11EffectRasterizerVariable*, AsRasterizer)() override;
    STDMETHOD_(ID3DX11EffectSamplerVariable*, AsSampler)() override;
};

template<typename IBaseInterface>
ID3DX11EffectScalarVariable * TUncastableVariable<IBaseInterface>::AsScalar()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsScalar";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidScalarVariable;
}

template<typename IBaseInterface>
ID3DX11EffectVectorVariable * TUncastableVariable<IBaseInterface>::AsVector()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsVector";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidVectorVariable;
}

template<typename IBaseInterface>
ID3DX11EffectMatrixVariable * TUncastableVariable<IBaseInterface>::AsMatrix()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsMatrix";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidMatrixVariable;
}

template<typename IBaseInterface>
ID3DX11EffectStringVariable * TUncastableVariable<IBaseInterface>::AsString()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsString";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidStringVariable;
}

template<typename IBaseClassInstance>
ID3DX11EffectClassInstanceVariable * TUncastableVariable<IBaseClassInstance>::AsClassInstance()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsClassInstance";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidClassInstanceVariable;
}

template<typename IBaseInterface>
ID3DX11EffectInterfaceVariable * TUncastableVariable<IBaseInterface>::AsInterface()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsInterface";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidInterfaceVariable;
}

template<typename IBaseInterface>
ID3DX11EffectShaderResourceVariable * TUncastableVariable<IBaseInterface>::AsShaderResource()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsShaderResource";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidShaderResourceVariable;
}

template<typename IBaseInterface>
ID3DX11EffectUnorderedAccessViewVariable * TUncastableVariable<IBaseInterface>::AsUnorderedAccessView()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsUnorderedAccessView";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidUnorderedAccessViewVariable;
}

template<typename IBaseInterface>
ID3DX11EffectRenderTargetViewVariable * TUncastableVariable<IBaseInterface>::AsRenderTargetView()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsRenderTargetView";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidRenderTargetViewVariable;
}

template<typename IBaseInterface>
ID3DX11EffectDepthStencilViewVariable * TUncastableVariable<IBaseInterface>::AsDepthStencilView()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsDepthStencilView";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidDepthStencilViewVariable;
}

template<typename IBaseInterface>
ID3DX11EffectConstantBuffer * TUncastableVariable<IBaseInterface>::AsConstantBuffer()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsConstantBuffer";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidConstantBuffer;
}

template<typename IBaseInterface>
ID3DX11EffectShaderVariable * TUncastableVariable<IBaseInterface>::AsShader()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsShader";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidShaderVariable;
}

template<typename IBaseInterface>
ID3DX11EffectBlendVariable * TUncastableVariable<IBaseInterface>::AsBlend()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsBlend";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidBlendVariable;
}

template<typename IBaseInterface>
ID3DX11EffectDepthStencilVariable * TUncastableVariable<IBaseInterface>::AsDepthStencil()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsDepthStencil";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidDepthStencilVariable;
}

template<typename IBaseInterface>
ID3DX11EffectRasterizerVariable * TUncastableVariable<IBaseInterface>::AsRasterizer()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsRasterizer";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidRasterizerVariable;
}

template<typename IBaseInterface>
ID3DX11EffectSamplerVariable * TUncastableVariable<IBaseInterface>::AsSampler()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::AsSampler";
    DPF(0, "%s: Invalid typecast", pFuncName);
    return &g_InvalidSamplerVariable;
}

////////////////////////////////////////////////////////////////////////////////
// Macros to instantiate the myriad templates
////////////////////////////////////////////////////////////////////////////////

// generates a global variable, annotation, global variable member, and annotation member of each struct type
#define GenerateReflectionClasses(Type, BaseInterface) \
struct S##Type##GlobalVariable : public T##Type##Variable<TGlobalVariable<BaseInterface>, false> { IUNKNOWN_IMP(S##Type##GlobalVariable, BaseInterface, ID3DX11EffectVariable); }; \
struct S##Type##Annotation : public T##Type##Variable<TAnnotation<BaseInterface>, true> { IUNKNOWN_IMP(S##Type##Annotation, BaseInterface, ID3DX11EffectVariable);}; \
struct S##Type##GlobalVariableMember : public T##Type##Variable<TVariable<TMember<BaseInterface> >, false> { IUNKNOWN_IMP(S##Type##GlobalVariableMember, BaseInterface, ID3DX11EffectVariable); }; \
struct S##Type##AnnotationMember : public T##Type##Variable<TVariable<TMember<BaseInterface> >, true> { IUNKNOWN_IMP(S##Type##AnnotationMember, BaseInterface, ID3DX11EffectVariable); };

#define GenerateVectorReflectionClasses(Type, BaseType, BaseInterface) \
struct S##Type##GlobalVariable : public TVectorVariable<TGlobalVariable<BaseInterface>, false, BaseType> { IUNKNOWN_IMP(S##Type##GlobalVariable, BaseInterface, ID3DX11EffectVariable); }; \
struct S##Type##Annotation : public TVectorVariable<TAnnotation<BaseInterface>, true, BaseType> { IUNKNOWN_IMP(S##Type##Annotation, BaseInterface, ID3DX11EffectVariable);}; \
struct S##Type##GlobalVariableMember : public TVectorVariable<TVariable<TMember<BaseInterface> >, false, BaseType> { IUNKNOWN_IMP(S##Type##GlobalVariableMember, BaseInterface, ID3DX11EffectVariable);}; \
struct S##Type##AnnotationMember : public TVectorVariable<TVariable<TMember<BaseInterface> >, true, BaseType> { IUNKNOWN_IMP(S##Type##AnnotationMember, BaseInterface, ID3DX11EffectVariable);};

#define GenerateReflectionGlobalOnlyClasses(Type) \
struct S##Type##GlobalVariable : public T##Type##Variable<TGlobalVariable<ID3DX11Effect##Type##Variable> > { IUNKNOWN_IMP(S##Type##GlobalVariable, ID3DX11Effect##Type##Variable, ID3DX11EffectVariable); }; \
struct S##Type##GlobalVariableMember : public T##Type##Variable<TVariable<TMember<ID3DX11Effect##Type##Variable> > > { IUNKNOWN_IMP(S##Type##GlobalVariableMember, ID3DX11Effect##Type##Variable, ID3DX11EffectVariable); }; \

GenerateReflectionClasses(Numeric, ID3DX11EffectVariable);
GenerateReflectionClasses(FloatScalar, ID3DX11EffectScalarVariable);
GenerateReflectionClasses(IntScalar, ID3DX11EffectScalarVariable);
GenerateReflectionClasses(BoolScalar, ID3DX11EffectScalarVariable);
GenerateVectorReflectionClasses(FloatVector, ETVT_Float, ID3DX11EffectVectorVariable);
GenerateVectorReflectionClasses(BoolVector, ETVT_Bool, ID3DX11EffectVectorVariable);
GenerateVectorReflectionClasses(IntVector, ETVT_Int, ID3DX11EffectVectorVariable);
GenerateReflectionClasses(Matrix, ID3DX11EffectMatrixVariable);
GenerateReflectionClasses(String, ID3DX11EffectStringVariable);
GenerateReflectionGlobalOnlyClasses(ClassInstance);
GenerateReflectionGlobalOnlyClasses(Interface);
GenerateReflectionGlobalOnlyClasses(ShaderResource);
GenerateReflectionGlobalOnlyClasses(UnorderedAccessView);
GenerateReflectionGlobalOnlyClasses(RenderTargetView);
GenerateReflectionGlobalOnlyClasses(DepthStencilView);
GenerateReflectionGlobalOnlyClasses(Shader);
GenerateReflectionGlobalOnlyClasses(Blend);
GenerateReflectionGlobalOnlyClasses(DepthStencil);
GenerateReflectionGlobalOnlyClasses(Rasterizer);
GenerateReflectionGlobalOnlyClasses(Sampler);

// Optimized matrix classes
struct SMatrix4x4ColumnMajorGlobalVariable : public TMatrix4x4Variable<TGlobalVariable<ID3DX11EffectMatrixVariable>, true> { IUNKNOWN_IMP(SMatrix4x4ColumnMajorGlobalVariable, ID3DX11EffectMatrixVariable, ID3DX11EffectVariable); };
struct SMatrix4x4RowMajorGlobalVariable : public TMatrix4x4Variable<TGlobalVariable<ID3DX11EffectMatrixVariable>, false> { IUNKNOWN_IMP(SMatrix4x4RowMajorGlobalVariable, ID3DX11EffectMatrixVariable, ID3DX11EffectVariable); };

struct SMatrix4x4ColumnMajorGlobalVariableMember : public TMatrix4x4Variable<TVariable<TMember<ID3DX11EffectMatrixVariable> >, true> { IUNKNOWN_IMP(SMatrix4x4ColumnMajorGlobalVariableMember, ID3DX11EffectMatrixVariable, ID3DX11EffectVariable); };
struct SMatrix4x4RowMajorGlobalVariableMember : public TMatrix4x4Variable<TVariable<TMember<ID3DX11EffectMatrixVariable> >, false> { IUNKNOWN_IMP(SMatrix4x4RowMajorGlobalVariableMember, ID3DX11EffectMatrixVariable, ID3DX11EffectVariable); };

// Optimized vector classes
struct SFloatVector4GlobalVariable : public TVector4Variable<TGlobalVariable<ID3DX11EffectVectorVariable> > { IUNKNOWN_IMP(SFloatVector4GlobalVariable, ID3DX11EffectVectorVariable, ID3DX11EffectVariable); };
struct SFloatVector4GlobalVariableMember : public TVector4Variable<TVariable<TMember<ID3DX11EffectVectorVariable> > > { IUNKNOWN_IMP(SFloatVector4GlobalVariableMember, ID3DX11EffectVectorVariable, ID3DX11EffectVariable); };

// These 3 classes should never be used directly

// The "base" global variable struct (all global variables should be the same size in bytes,
// but we pick this as the default).  
struct SGlobalVariable : public TGlobalVariable<ID3DX11EffectVariable>
{

};

// The "base" annotation struct (all annotations should be the same size in bytes,
// but we pick this as the default).
struct SAnnotation : public TAnnotation<ID3DX11EffectVariable>
{

};

// The "base" variable member struct (all annotation/global variable members should be the
// same size in bytes, but we pick this as the default).
struct SMember : public TVariable<TMember<ID3DX11EffectVariable> >
{

};

// creates a new variable of the appropriate polymorphic type where pVar was
HRESULT PlacementNewVariable(_In_ void *pVar, _In_ SType *pType, _In_ bool IsAnnotation);
SMember * CreateNewMember(_In_ SType *pType, _In_ bool IsAnnotation);

#pragma warning(pop)

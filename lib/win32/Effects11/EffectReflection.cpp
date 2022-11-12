//--------------------------------------------------------------------------------------
// File: EffectReflection.cpp
//
// Direct3D 11 Effects public reflection APIs
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#include "pchfx.h"

namespace D3DX11Effects
{

SEffectInvalidType g_InvalidType;

SEffectInvalidScalarVariable g_InvalidScalarVariable;
SEffectInvalidVectorVariable g_InvalidVectorVariable;
SEffectInvalidMatrixVariable g_InvalidMatrixVariable;
SEffectInvalidStringVariable g_InvalidStringVariable;
SEffectInvalidClassInstanceVariable g_InvalidClassInstanceVariable;
SEffectInvalidInterfaceVariable g_InvalidInterfaceVariable;
SEffectInvalidShaderResourceVariable g_InvalidShaderResourceVariable;
SEffectInvalidUnorderedAccessViewVariable g_InvalidUnorderedAccessViewVariable;
SEffectInvalidRenderTargetViewVariable g_InvalidRenderTargetViewVariable;
SEffectInvalidDepthStencilViewVariable g_InvalidDepthStencilViewVariable;
SEffectInvalidConstantBuffer g_InvalidConstantBuffer;
SEffectInvalidShaderVariable g_InvalidShaderVariable;
SEffectInvalidBlendVariable g_InvalidBlendVariable;
SEffectInvalidDepthStencilVariable g_InvalidDepthStencilVariable;
SEffectInvalidRasterizerVariable g_InvalidRasterizerVariable;
SEffectInvalidSamplerVariable g_InvalidSamplerVariable;

SEffectInvalidPass g_InvalidPass;
SEffectInvalidTechnique g_InvalidTechnique;
SEffectInvalidGroup g_InvalidGroup;


//////////////////////////////////////////////////////////////////////////
// Helper routine implementations
//////////////////////////////////////////////////////////////////////////

ID3DX11EffectConstantBuffer * NoParentCB()
{
    DPF(0, "ID3DX11EffectVariable::GetParentConstantBuffer: Variable does not have a parent constant buffer");
    // have to typecast because the type of g_InvalidScalarVariable has not been declared yet
    return &g_InvalidConstantBuffer;
}

_Use_decl_annotations_
ID3DX11EffectVariable * GetAnnotationByIndexHelper(const char *pClassName, uint32_t Index, uint32_t  AnnotationCount, SAnnotation *pAnnotations)
{
    if (Index >= AnnotationCount)
    {
        DPF(0, "%s::GetAnnotationByIndex: Invalid index (%u, total: %u)", pClassName, Index, AnnotationCount);
        return &g_InvalidScalarVariable;
    }

    return pAnnotations + Index;
}

_Use_decl_annotations_
ID3DX11EffectVariable * GetAnnotationByNameHelper(const char *pClassName, LPCSTR Name, uint32_t  AnnotationCount, SAnnotation *pAnnotations)
{
    uint32_t  i;
    for (i = 0; i < AnnotationCount; ++ i)
    {
        if (strcmp(pAnnotations[i].pName, Name) == 0)
        {
            return pAnnotations + i;
        }
    }

    DPF(0, "%s::GetAnnotationByName: Annotation [%s] not found", pClassName, Name);
    return &g_InvalidScalarVariable;
}

//////////////////////////////////////////////////////////////////////////
// Effect routines to pool interfaces
//////////////////////////////////////////////////////////////////////////

ID3DX11EffectType * CEffect::CreatePooledSingleElementTypeInterface(_In_ SType *pType)
{
    if (IsOptimized())
    {
        DPF(0, "ID3DX11Effect: Cannot create new type interfaces since the effect has been Optimize()'ed");
        return &g_InvalidType;
    }

    for (size_t i = 0; i < m_pTypeInterfaces.GetSize(); ++ i)
    {
        if (m_pTypeInterfaces[i]->pType == pType)
        {
            return (SSingleElementType*)m_pTypeInterfaces[i];
        }
    }
    SSingleElementType *pNewType;
    if (nullptr == (pNewType = new SSingleElementType))
    {
        DPF(0, "ID3DX11Effect: Out of memory while trying to create new type interface");
        return &g_InvalidType;
    }

    pNewType->pType = pType;
    m_pTypeInterfaces.Add(pNewType);

    return pNewType;
}

// Create a member variable (via GetMemberBy* or GetElement)
_Use_decl_annotations_
ID3DX11EffectVariable * CEffect::CreatePooledVariableMemberInterface(TTopLevelVariable<ID3DX11EffectVariable> *pTopLevelEntity,
                                                                     const SVariable *pMember,
                                                                     const UDataPointer Data, bool IsSingleElement, uint32_t Index)
{
    bool IsAnnotation;

    if (IsOptimized())
    {
        DPF(0, "ID3DX11Effect: Cannot create new variable interfaces since the effect has been Optimize()'ed");
        return &g_InvalidScalarVariable;
    }

    for (size_t i = 0; i < m_pMemberInterfaces.GetSize(); ++ i)
    {
        if (m_pMemberInterfaces[i]->pType == pMember->pType && 
            m_pMemberInterfaces[i]->pName == pMember->pName &&
            m_pMemberInterfaces[i]->pSemantic == pMember->pSemantic &&
            m_pMemberInterfaces[i]->Data.pGeneric == Data.pGeneric &&
            m_pMemberInterfaces[i]->IsSingleElement == (uint32_t)IsSingleElement &&
            ((SMember*)m_pMemberInterfaces[i])->pTopLevelEntity == pTopLevelEntity)
        {
            return (ID3DX11EffectVariable *) m_pMemberInterfaces[i];
        }
    }

    // is this annotation or runtime data?
    if( pTopLevelEntity->pEffect->IsReflectionData(pTopLevelEntity) )
    {
        assert( pTopLevelEntity->pEffect->IsReflectionData(Data.pGeneric) );
        IsAnnotation = true;
    }
    else
    {
        // if the heap is empty, we are still loading the Effect, and thus creating a member for a variable initializer
        // ex. Interface myInt = myClassArray[2];
        if( pTopLevelEntity->pEffect->m_Heap.GetSize() > 0 )
        {
            assert( pTopLevelEntity->pEffect->IsRuntimeData(pTopLevelEntity) );
            if (!pTopLevelEntity->pType->IsObjectType(EOT_String))
            {
                // strings are funny; their data is reflection data, so ignore those
                assert( pTopLevelEntity->pEffect->IsRuntimeData(Data.pGeneric) );
            }
        }
        IsAnnotation = false;
    }

    SMember *pNewMember;

    if (nullptr == (pNewMember = CreateNewMember((SType*)pMember->pType, IsAnnotation)))
    {
        DPF(0, "ID3DX11Effect: Out of memory while trying to create new member variable interface");
        return &g_InvalidScalarVariable;
    }
    
    pNewMember->pType = pMember->pType;
    pNewMember->pName = pMember->pName;
    pNewMember->pSemantic = pMember->pSemantic;
    pNewMember->Data.pGeneric = Data.pGeneric;
    pNewMember->IsSingleElement = IsSingleElement;
    pNewMember->pTopLevelEntity = pTopLevelEntity;

    if( IsSingleElement && pMember->pMemberData )
    {
        assert( !IsAnnotation );
        // This is an element of a global variable array
        pNewMember->pMemberData = pMember->pMemberData + Index;
    }

    if (FAILED(m_pMemberInterfaces.Add(pNewMember)))
    {
        SAFE_DELETE(pNewMember);
        DPF(0, "ID3DX11Effect: Out of memory while trying to create new member variable interface");
        return &g_InvalidScalarVariable;
    }

    return (ID3DX11EffectVariable *) pNewMember;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectType (SType, SSingleElementType implementations)
//////////////////////////////////////////////////////////////////////////

static ID3DX11EffectType * GetTypeByIndexHelper(uint32_t Index, uint32_t  VariableCount, 
                                                SVariable *pVariables, uint32_t  SizeOfVariableType)
{
    static LPCSTR pFuncName = "ID3DX11EffectType::GetMemberTypeByIndex";

    if (Index >= VariableCount)
    {
        DPF(0, "%s: Invalid index (%u, total: %u)", pFuncName, Index, VariableCount);
        return &g_InvalidType;
    }

    SVariable *pVariable = (SVariable *)((uint8_t *)pVariables + Index * SizeOfVariableType);
    if (nullptr == pVariable->pName)
    {
        DPF(0, "%s: Cannot get member types; Effect has been Optimize()'ed", pFuncName);
        return &g_InvalidType;
    }

    return (ID3DX11EffectType *) pVariable->pType;
}

static ID3DX11EffectType * GetTypeByNameHelper(LPCSTR Name, uint32_t  VariableCount, 
                                              SVariable *pVariables, uint32_t  SizeOfVariableType)
{
    static LPCSTR pFuncName = "ID3DX11EffectType::GetMemberTypeByName";

    if (nullptr == Name)
    {
        DPF(0, "%s: Parameter Name was nullptr.", pFuncName);
        return &g_InvalidType;
    }

    uint32_t  i;
    SVariable *pVariable;

    for (i = 0; i < VariableCount; ++ i)
    {
        pVariable = (SVariable *)((uint8_t *)pVariables + i * SizeOfVariableType);
        if (nullptr == pVariable->pName)
        {
            DPF(0, "%s: Cannot get member types; Effect has been Optimize()'ed", pFuncName);
            return &g_InvalidType;
        }
        if (strcmp(pVariable->pName, Name) == 0)
        {
            return (ID3DX11EffectType *) pVariable->pType;
        }
    }

    DPF(0, "%s: Member type [%s] not found", pFuncName, Name);
    return &g_InvalidType;
}


static ID3DX11EffectType * GetTypeBySemanticHelper(LPCSTR Semantic, uint32_t  VariableCount, 
                                                  SVariable *pVariables, uint32_t  SizeOfVariableType)
{
    static LPCSTR pFuncName = "ID3DX11EffectType::GetMemberTypeBySemantic";

    if (nullptr == Semantic)
    {
        DPF(0, "%s: Parameter Semantic was nullptr.", pFuncName);
        return &g_InvalidType;
    }

    uint32_t  i;
    SVariable *pVariable;

    for (i = 0; i < VariableCount; ++ i)
    {
        pVariable = (SVariable *)((uint8_t *)pVariables + i * SizeOfVariableType);
        if (nullptr == pVariable->pName)
        {
            DPF(0, "%s: Cannot get member types; Effect has been Optimize()'ed", pFuncName);
            return &g_InvalidType;
        }
        if (nullptr != pVariable->pSemantic &&
            _stricmp(pVariable->pSemantic, Semantic) == 0)
        {
            return (ID3DX11EffectType *) pVariable->pType;
        }
    }

    DPF(0, "%s: Member type with semantic [%s] not found", pFuncName, Semantic);
    return &g_InvalidType;
}

ID3DX11EffectType * SType::GetMemberTypeByIndex(_In_ uint32_t Index)
{
    if (VarType != EVT_Struct)
    {
        DPF(0, "ID3DX11EffectType::GetMemberTypeByIndex: This interface does not refer to a structure");
        return &g_InvalidType;
    }

    return GetTypeByIndexHelper(Index, StructType.Members, StructType.pMembers, sizeof(SVariable));
}

ID3DX11EffectType * SType::GetMemberTypeByName(_In_z_ LPCSTR Name)
{
    if (VarType != EVT_Struct)
    {
        DPF(0, "ID3DX11EffectType::GetMemberTypeByName: This interface does not refer to a structure");
        return &g_InvalidType;
    }

    return GetTypeByNameHelper(Name, StructType.Members, StructType.pMembers, sizeof(SVariable));
}

ID3DX11EffectType * SType::GetMemberTypeBySemantic(_In_z_ LPCSTR Semantic)
{
    if (VarType != EVT_Struct)
    {
        DPF(0, "ID3DX11EffectType::GetMemberTypeBySemantic: This interface does not refer to a structure");
        return &g_InvalidType;
    }

    return GetTypeBySemanticHelper(Semantic, StructType.Members, StructType.pMembers, sizeof(SVariable));
}

LPCSTR SType::GetMemberName(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectType::GetMemberName";

    if (VarType != EVT_Struct)
    {
        DPF(0, "%s: This interface does not refer to a structure", pFuncName);
        return nullptr;
    }

    if (Index >= StructType.Members)
    {
        DPF(0, "%s: Invalid index (%u, total: %u)", pFuncName, Index, StructType.Members);
        return nullptr;
    }

    SVariable *pVariable = StructType.pMembers + Index;

    if (nullptr == pVariable->pName)
    {
        DPF(0, "%s: Cannot get member names; Effect has been Optimize()'ed", pFuncName);
        return nullptr;
    }

    return pVariable->pName;
}

LPCSTR SType::GetMemberSemantic(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectType::GetMemberSemantic";

    if (VarType != EVT_Struct)
    {
        DPF(0, "%s: This interface does not refer to a structure", pFuncName);
        return nullptr;
    }

    if (Index >= StructType.Members)
    {
        DPF(0, "%s: Invalid index (%u, total: %u)", pFuncName, Index, StructType.Members);
        return nullptr;
    }

    SVariable *pVariable = StructType.pMembers + Index;

    if (nullptr == pVariable->pName)
    {
        DPF(0, "%s: Cannot get member semantics; Effect has been Optimize()'ed", pFuncName);
        return nullptr;
    }

    return pVariable->pSemantic;
}

HRESULT SType::GetDescHelper(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc, _In_ bool IsSingleElement) const
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectType::GetDesc";

    VERIFYPARAMETER(pDesc);
    
    pDesc->TypeName = pTypeName;

    // intentionally return 0 so they know it's not a single element array
    pDesc->Elements = IsSingleElement ? 0 : Elements;
    pDesc->PackedSize = GetTotalPackedSize(IsSingleElement);
    pDesc->UnpackedSize = GetTotalUnpackedSize(IsSingleElement);
    pDesc->Stride = Stride;

    switch (VarType)
    {
    case EVT_Numeric:
        switch (NumericType.NumericLayout)
        {
        case ENL_Matrix:
            if (NumericType.IsColumnMajor)
            {
                pDesc->Class = D3D_SVC_MATRIX_COLUMNS;
            }
            else
            {
                pDesc->Class = D3D_SVC_MATRIX_ROWS;
            }
            break;
        case ENL_Vector:
            pDesc->Class = D3D_SVC_VECTOR;
            break;
        case ENL_Scalar:
            pDesc->Class = D3D_SVC_SCALAR;
            break;
        default:
            assert(0);
        }

        switch (NumericType.ScalarType)
        {
        case EST_Bool:
            pDesc->Type = D3D_SVT_BOOL;
            break;
        case EST_Int:
            pDesc->Type = D3D_SVT_INT;
            break;
        case EST_UInt:
            pDesc->Type = D3D_SVT_UINT;
            break;
        case EST_Float:
            pDesc->Type = D3D_SVT_FLOAT;
            break;
        default:
            assert(0);
        }

        pDesc->Rows = NumericType.Rows;
        pDesc->Columns = NumericType.Columns;
        pDesc->Members = 0;

        break;

    case EVT_Struct:
        pDesc->Rows = 0;
        pDesc->Columns = 0;
        pDesc->Members = StructType.Members;
        if( StructType.ImplementsInterface )
        {
            pDesc->Class = D3D_SVC_INTERFACE_CLASS;
        }
        else
        {
            pDesc->Class = D3D_SVC_STRUCT;
        }
        pDesc->Type = D3D_SVT_VOID;
        break;

    case EVT_Interface:
        pDesc->Rows = 0;
        pDesc->Columns = 0;
        pDesc->Members = 0;
        pDesc->Class = D3D_SVC_INTERFACE_POINTER;
        pDesc->Type = D3D_SVT_INTERFACE_POINTER;
        break;

    case EVT_Object:
        pDesc->Rows = 0;
        pDesc->Columns = 0;
        pDesc->Members = 0;
        pDesc->Class = D3D_SVC_OBJECT;            

        switch (ObjectType)
        {
        case EOT_String:
            pDesc->Type = D3D_SVT_STRING;
            break;
        case EOT_Blend:
            pDesc->Type = D3D_SVT_BLEND; 
            break;
        case EOT_DepthStencil:
            pDesc->Type = D3D_SVT_DEPTHSTENCIL;
            break;
        case EOT_Rasterizer:
            pDesc->Type = D3D_SVT_RASTERIZER;
            break;
        case EOT_PixelShader:
        case EOT_PixelShader5:
            pDesc->Type = D3D_SVT_PIXELSHADER;
            break;
        case EOT_VertexShader:
        case EOT_VertexShader5:
            pDesc->Type = D3D_SVT_VERTEXSHADER;
            break;
        case EOT_GeometryShader:
        case EOT_GeometryShaderSO:
        case EOT_GeometryShader5:
            pDesc->Type = D3D_SVT_GEOMETRYSHADER;
            break;
        case EOT_HullShader5:
            pDesc->Type = D3D_SVT_HULLSHADER;
            break;
        case EOT_DomainShader5:
            pDesc->Type = D3D_SVT_DOMAINSHADER;
            break;
        case EOT_ComputeShader5:
            pDesc->Type = D3D_SVT_COMPUTESHADER;
            break;
        case EOT_Texture:
            pDesc->Type = D3D_SVT_TEXTURE;
            break;
        case EOT_Texture1D:
            pDesc->Type = D3D_SVT_TEXTURE1D;
            break;
        case EOT_Texture1DArray:
            pDesc->Type = D3D_SVT_TEXTURE1DARRAY;
            break;
        case EOT_Texture2D:
            pDesc->Type = D3D_SVT_TEXTURE2D;
            break;
        case EOT_Texture2DArray:
            pDesc->Type = D3D_SVT_TEXTURE2DARRAY;
            break;
        case EOT_Texture2DMS:
            pDesc->Type = D3D_SVT_TEXTURE2DMS;
            break;
        case EOT_Texture2DMSArray:
            pDesc->Type = D3D_SVT_TEXTURE2DMSARRAY;
            break;
        case EOT_Texture3D:
            pDesc->Type = D3D_SVT_TEXTURE3D;
            break;
        case EOT_TextureCube:
            pDesc->Type = D3D_SVT_TEXTURECUBE;
            break;
        case EOT_TextureCubeArray:
            pDesc->Type = D3D_SVT_TEXTURECUBEARRAY;
            break;
        case EOT_Buffer:
            pDesc->Type = D3D_SVT_BUFFER;
            break;
        case EOT_Sampler:
            pDesc->Type = D3D_SVT_SAMPLER;
            break;
        case EOT_RenderTargetView:
            pDesc->Type = D3D_SVT_RENDERTARGETVIEW;
            break;
        case EOT_DepthStencilView:
            pDesc->Type = D3D_SVT_DEPTHSTENCILVIEW;
            break;
        case EOT_RWTexture1D:
            pDesc->Type = D3D_SVT_RWTEXTURE1D;
            break;
        case EOT_RWTexture1DArray:
            pDesc->Type = D3D_SVT_RWTEXTURE1DARRAY;
            break;
        case EOT_RWTexture2D:
            pDesc->Type = D3D_SVT_RWTEXTURE2D;
            break;
        case EOT_RWTexture2DArray:
            pDesc->Type = D3D_SVT_RWTEXTURE2DARRAY;
            break;
        case EOT_RWTexture3D:
            pDesc->Type = D3D_SVT_RWTEXTURE3D;
            break;
        case EOT_RWBuffer:
            pDesc->Type = D3D_SVT_RWBUFFER;
            break;
        case EOT_ByteAddressBuffer:
            pDesc->Type = D3D_SVT_BYTEADDRESS_BUFFER;
            break;
        case EOT_RWByteAddressBuffer:
            pDesc->Type = D3D_SVT_RWBYTEADDRESS_BUFFER;
            break;
        case EOT_StructuredBuffer:
            pDesc->Type = D3D_SVT_STRUCTURED_BUFFER;
            break;
        case EOT_RWStructuredBuffer:
        case EOT_RWStructuredBufferAlloc:
        case EOT_RWStructuredBufferConsume:
            pDesc->Type = D3D_SVT_RWSTRUCTURED_BUFFER;
            break;
        case EOT_AppendStructuredBuffer:
            pDesc->Type = D3D_SVT_APPEND_STRUCTURED_BUFFER;
            break;
        case EOT_ConsumeStructuredBuffer:
            pDesc->Type = D3D_SVT_CONSUME_STRUCTURED_BUFFER;
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

////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectShaderVariable (SAnonymousShader implementation)
////////////////////////////////////////////////////////////////////////////////

SAnonymousShader::SAnonymousShader(_In_opt_ SShaderBlock *pBlock) noexcept :
    pShaderBlock(pBlock)
{
}

bool SAnonymousShader::IsValid()
{
    return pShaderBlock && pShaderBlock->IsValid;
}

ID3DX11EffectType * SAnonymousShader::GetType()
{
    return (ID3DX11EffectType *) this;
}

HRESULT SAnonymousShader::GetDesc(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc)
{
    pDesc->Annotations = 0;
    pDesc->Flags = 0;

    pDesc->Name = "$Anonymous";
    pDesc->Semantic = nullptr;
    pDesc->BufferOffset = 0;

    return S_OK;
}

ID3DX11EffectVariable * SAnonymousShader::GetAnnotationByIndex(_In_ uint32_t Index)
{
    UNREFERENCED_PARAMETER(Index);
    DPF(0, "ID3DX11EffectVariable::GetAnnotationByIndex: Anonymous shaders cannot have annotations");
    return &g_InvalidScalarVariable;
}

ID3DX11EffectVariable * SAnonymousShader::GetAnnotationByName(_In_z_ LPCSTR Name)
{
    UNREFERENCED_PARAMETER(Name);
    DPF(0, "ID3DX11EffectVariable::GetAnnotationByName: Anonymous shaders cannot have annotations");
    return &g_InvalidScalarVariable;
}

ID3DX11EffectVariable * SAnonymousShader::GetMemberByIndex(_In_ uint32_t Index)
{
    UNREFERENCED_PARAMETER(Index);
    DPF(0, "ID3DX11EffectVariable::GetMemberByIndex: Variable is not a structure");
    return &g_InvalidScalarVariable;
}

ID3DX11EffectVariable * SAnonymousShader::GetMemberByName(_In_z_ LPCSTR Name)
{
    UNREFERENCED_PARAMETER(Name);
    DPF(0, "ID3DX11EffectVariable::GetMemberByName: Variable is not a structure");
    return &g_InvalidScalarVariable;
}

ID3DX11EffectVariable * SAnonymousShader::GetMemberBySemantic(_In_z_ LPCSTR Semantic)
{
    UNREFERENCED_PARAMETER(Semantic);
    DPF(0, "ID3DX11EffectVariable::GetMemberBySemantic: Variable is not a structure");
    return &g_InvalidScalarVariable;
}

ID3DX11EffectVariable * SAnonymousShader::GetElement(_In_ uint32_t Index)
{
    UNREFERENCED_PARAMETER(Index);
    DPF(0, "ID3DX11EffectVariable::GetElement: Anonymous shaders cannot have elements");
    return &g_InvalidScalarVariable;
}

ID3DX11EffectConstantBuffer * SAnonymousShader::GetParentConstantBuffer()
{
    return NoParentCB();
}

ID3DX11EffectShaderVariable * SAnonymousShader::AsShader()
{
    return (ID3DX11EffectShaderVariable *) this;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::SetRawValue(const void *pData, uint32_t Offset, uint32_t Count) 
{ 
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Count);
    return ObjectSetRawValue(); 
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetRawValue(void *pData, uint32_t Offset, uint32_t Count) 
{ 
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Count);
    return ObjectGetRawValue(); 
}

#define ANONYMOUS_SHADER_INDEX_CHECK() \
    HRESULT hr = S_OK; \
    if (0 != ShaderIndex) \
    { \
        DPF(0, "%s: Invalid index specified", pFuncName); \
        VH(E_INVALIDARG); \
    } \

HRESULT SAnonymousShader::GetShaderDesc(_In_ uint32_t ShaderIndex, _Out_ D3DX11_EFFECT_SHADER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetShaderDesc";

    ANONYMOUS_SHADER_INDEX_CHECK();

    hr = pShaderBlock->GetShaderDesc(pDesc, true);

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetVertexShader(uint32_t ShaderIndex, ID3D11VertexShader **ppVS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetVertexShader";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetVertexShader(ppVS) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetGeometryShader(uint32_t ShaderIndex, ID3D11GeometryShader **ppGS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetGeometryShader";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetGeometryShader(ppGS) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetPixelShader(uint32_t ShaderIndex, ID3D11PixelShader **ppPS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetPixelShader";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetPixelShader(ppPS) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetHullShader(uint32_t ShaderIndex, ID3D11HullShader **ppHS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetHullShader";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetHullShader(ppHS) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetDomainShader(uint32_t ShaderIndex, ID3D11DomainShader **ppDS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetDomainShader";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetDomainShader(ppDS) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetComputeShader(uint32_t ShaderIndex, ID3D11ComputeShader **ppCS)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetComputeShader";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetComputeShader(ppCS) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetInputSignatureElementDesc(uint32_t ShaderIndex, uint32_t Element, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetInputSignatureElementDesc";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetSignatureElementDesc(SShaderBlock::ST_Input, Element, pDesc) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetOutputSignatureElementDesc(uint32_t ShaderIndex, uint32_t Element, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetOutputSignatureElementDesc";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetSignatureElementDesc(SShaderBlock::ST_Output, Element, pDesc) );

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SAnonymousShader::GetPatchConstantSignatureElementDesc(uint32_t ShaderIndex, uint32_t Element, D3D11_SIGNATURE_PARAMETER_DESC *pDesc)
{
    static LPCSTR pFuncName = "ID3DX11EffectShaderVariable::GetPatchConstantSignatureElementDesc";

    ANONYMOUS_SHADER_INDEX_CHECK();

    VH( pShaderBlock->GetSignatureElementDesc(SShaderBlock::ST_PatchConstant, Element, pDesc) );

lExit:
    return hr;
}

HRESULT SAnonymousShader::GetDesc(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc)
{
    pDesc->Class = D3D_SVC_OBJECT;

    switch (pShaderBlock->GetShaderType())
    {
    case EOT_VertexShader:
    case EOT_VertexShader5:
        pDesc->TypeName = "vertexshader";
        pDesc->Type = D3D_SVT_VERTEXSHADER;
        break;
    case EOT_GeometryShader:
    case EOT_GeometryShader5:
        pDesc->TypeName = "geometryshader";
        pDesc->Type = D3D_SVT_GEOMETRYSHADER;
        break;
    case EOT_PixelShader:
    case EOT_PixelShader5:
        pDesc->TypeName = "pixelshader";
        pDesc->Type = D3D_SVT_PIXELSHADER;
        break;
    case EOT_HullShader5:
        pDesc->TypeName = "Hullshader";
        pDesc->Type = D3D_SVT_HULLSHADER;
        break;
    case EOT_DomainShader5:
        pDesc->TypeName = "Domainshader";
        pDesc->Type = D3D_SVT_DOMAINSHADER;
        break;
    case EOT_ComputeShader5:
        pDesc->TypeName = "Computeshader";
        pDesc->Type = D3D_SVT_COMPUTESHADER;
        break;
    }

    pDesc->Elements = 0;
    pDesc->Members = 0;
    pDesc->Rows = 0;
    pDesc->Columns = 0;
    pDesc->PackedSize = 0;
    pDesc->UnpackedSize = 0;
    pDesc->Stride = 0;

    return S_OK;
}

ID3DX11EffectType * SAnonymousShader::GetMemberTypeByIndex(_In_ uint32_t Index)
{
    UNREFERENCED_PARAMETER(Index);
    DPF(0, "ID3DX11EffectType::GetMemberTypeByIndex: This interface does not refer to a structure");
    return &g_InvalidType;
}

ID3DX11EffectType * SAnonymousShader::GetMemberTypeByName(_In_z_ LPCSTR Name)
{
    UNREFERENCED_PARAMETER(Name);
    DPF(0, "ID3DX11EffectType::GetMemberTypeByName: This interface does not refer to a structure");
    return &g_InvalidType;
}

ID3DX11EffectType * SAnonymousShader::GetMemberTypeBySemantic(_In_z_ LPCSTR Semantic)
{
    UNREFERENCED_PARAMETER(Semantic);
    DPF(0, "ID3DX11EffectType::GetMemberTypeBySemantic: This interface does not refer to a structure");
    return &g_InvalidType;
}

LPCSTR SAnonymousShader::GetMemberName(_In_ uint32_t Index)
{
    UNREFERENCED_PARAMETER(Index);
    DPF(0, "ID3DX11EffectType::GetMemberName: This interface does not refer to a structure");
    return nullptr;
}

LPCSTR SAnonymousShader::GetMemberSemantic(_In_ uint32_t Index)
{
    UNREFERENCED_PARAMETER(Index);
    DPF(0, "ID3DX11EffectType::GetMemberSemantic: This interface does not refer to a structure");
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectConstantBuffer (SConstantBuffer implementation)
//////////////////////////////////////////////////////////////////////////

bool SConstantBuffer::IsValid()
{
    return true;
}

ID3DX11EffectType * SConstantBuffer::GetType()
{
    return (ID3DX11EffectType *) this;
}

HRESULT SConstantBuffer::GetDesc(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc)
{
    pDesc->Annotations = AnnotationCount;
    pDesc->Flags = 0;

    pDesc->Name = pName;
    pDesc->Semantic = nullptr;
    pDesc->BufferOffset = 0;

    if (ExplicitBindPoint != static_cast<uint32_t>(- 1))
    {
        pDesc->ExplicitBindPoint = ExplicitBindPoint;
        pDesc->Flags |= D3DX11_EFFECT_VARIABLE_EXPLICIT_BIND_POINT;
    }
    else
    {
        pDesc->ExplicitBindPoint = 0;
    }

    return S_OK;
}

ID3DX11EffectVariable * SConstantBuffer::GetAnnotationByIndex(_In_ uint32_t Index)
{
    return GetAnnotationByIndexHelper("ID3DX11EffectVariable", Index, AnnotationCount, pAnnotations);
}

ID3DX11EffectVariable * SConstantBuffer::GetAnnotationByName(_In_z_ LPCSTR Name)
{
    return GetAnnotationByNameHelper("ID3DX11EffectVariable", Name, AnnotationCount, pAnnotations);
}

ID3DX11EffectVariable * SConstantBuffer::GetMemberByIndex(_In_ uint32_t Index)
{
    SGlobalVariable *pMember;
    UDataPointer dataPtr;

    if (IsEffectOptimized)
    {
        DPF(0, "ID3DX11EffectVariable::GetMemberByIndex: Cannot get members; effect has been Optimize()'ed");
        return &g_InvalidScalarVariable;
    }

    if (!GetVariableByIndexHelper<SGlobalVariable>(Index, VariableCount, (SGlobalVariable*)pVariables, 
        nullptr, &pMember, &dataPtr.pGeneric))
    {
        return &g_InvalidScalarVariable;
    }

    return (ID3DX11EffectVariable *) pMember;
}

ID3DX11EffectVariable * SConstantBuffer::GetMemberByName(_In_z_ LPCSTR Name)
{
    SGlobalVariable *pMember;
    UDataPointer dataPtr;
    uint32_t index;

    if (IsEffectOptimized)
    {
        DPF(0, "ID3DX11EffectVariable::GetMemberByName: Cannot get members; effect has been Optimize()'ed");
        return &g_InvalidScalarVariable;
    }

    if (!GetVariableByNameHelper<SGlobalVariable>(Name, VariableCount, (SGlobalVariable*)pVariables, 
        nullptr, &pMember, &dataPtr.pGeneric, &index))
    {
        return &g_InvalidScalarVariable;
    }

    return (ID3DX11EffectVariable *) pMember;
}

ID3DX11EffectVariable * SConstantBuffer::GetMemberBySemantic(_In_z_ LPCSTR Semantic)
{
    SGlobalVariable *pMember;
    UDataPointer dataPtr;
    uint32_t index;

    if (IsEffectOptimized)
    {
        DPF(0, "ID3DX11EffectVariable::GetMemberBySemantic: Cannot get members; effect has been Optimize()'ed");
        return &g_InvalidScalarVariable;
    }

    if (!GetVariableBySemanticHelper<SGlobalVariable>(Semantic, VariableCount, (SGlobalVariable*)pVariables, 
        nullptr, &pMember, &dataPtr.pGeneric, &index))
    {
        return &g_InvalidScalarVariable;
    }

    return (ID3DX11EffectVariable *) pMember;
}

ID3DX11EffectVariable * SConstantBuffer::GetElement(_In_ uint32_t Index)
{
    UNREFERENCED_PARAMETER(Index);
    static LPCSTR pFuncName = "ID3DX11EffectVariable::GetElement";
    DPF(0, "%s: This interface does not refer to an array", pFuncName);
    return &g_InvalidScalarVariable;
}

ID3DX11EffectConstantBuffer * SConstantBuffer::GetParentConstantBuffer()
{
    static LPCSTR pFuncName = "ID3DX11EffectVariable::GetParentConstantBuffer";
    DPF(0, "%s: Constant buffers do not have parent constant buffers", pFuncName);
    return &g_InvalidConstantBuffer;
}

ID3DX11EffectConstantBuffer * SConstantBuffer::AsConstantBuffer()
{
    return (ID3DX11EffectConstantBuffer *) this;
}

HRESULT SConstantBuffer::GetDesc(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc)
{
    pDesc->TypeName = IsTBuffer ? "tbuffer" : "cbuffer";
    pDesc->Class = D3D_SVC_OBJECT;
    pDesc->Type = IsTBuffer ? D3D_SVT_TBUFFER : D3D_SVT_CBUFFER;

    pDesc->Elements = 0;
    pDesc->Members = VariableCount;
    pDesc->Rows = 0;
    pDesc->Columns = 0;

    uint32_t  i;
    pDesc->PackedSize = 0;
    for (i = 0; i < VariableCount; ++ i)
    {
        pDesc->PackedSize += pVariables[i].pType->PackedSize;
    }

    pDesc->UnpackedSize = Size;
    assert(pDesc->UnpackedSize >= pDesc->PackedSize);

    pDesc->Stride = AlignToPowerOf2(pDesc->UnpackedSize, SType::c_RegisterSize);

    return S_OK;
}

ID3DX11EffectType * SConstantBuffer::GetMemberTypeByIndex(_In_ uint32_t Index)
{
    return GetTypeByIndexHelper(Index, VariableCount, pVariables, sizeof (SGlobalVariable));
}

ID3DX11EffectType * SConstantBuffer::GetMemberTypeByName(_In_z_ LPCSTR Name)
{
    return GetTypeByNameHelper(Name, VariableCount, pVariables, sizeof (SGlobalVariable));
}

ID3DX11EffectType * SConstantBuffer::GetMemberTypeBySemantic(_In_z_ LPCSTR Semantic)
{
    return GetTypeBySemanticHelper(Semantic, VariableCount, pVariables, sizeof (SGlobalVariable));
}

LPCSTR SConstantBuffer::GetMemberName(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectType::GetMemberName";

    if (IsEffectOptimized)
    {
        DPF(0, "%s: Cannot get member names; Effect has been Optimize()'ed", pFuncName);
        return nullptr;
    }

    if (Index >= VariableCount)
    {
        DPF(0, "%s: Invalid index (%u, total: %u)", pFuncName, Index, VariableCount);
        return nullptr;
    }

    return pVariables[Index].pName;
}

LPCSTR SConstantBuffer::GetMemberSemantic(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectType::GetMemberSemantic";

    if (IsEffectOptimized)
    {
        DPF(0, "%s: Cannot get member semantics; Effect has been Optimize()'ed", pFuncName);
        return nullptr;
    }

    if (Index >= VariableCount)
    {
        DPF(0, "%s: Invalid index (%u, total: %u)", pFuncName, Index, VariableCount);
        return nullptr;
    }

    return pVariables[Index].pSemantic;
}

_Use_decl_annotations_
HRESULT SConstantBuffer::SetRawValue(const void *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;    

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectVariable::SetRawValue";

    VERIFYPARAMETER(pData);

    if ((Offset + Count < Offset) ||
        (Count + (uint8_t*)pData < (uint8_t*)pData) ||
        ((Offset + Count) > Size))
    {
        // overflow of some kind
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    if (IsUsedByExpression)
    {
        uint32_t  i;
        for (i = 0; i < VariableCount; ++ i)
        {
            ((SGlobalVariable*)pVariables)[i].DirtyVariable();
        }
    }
    else
    {
        IsDirty = true;
    }

    memcpy(pBackingStore + Offset, pData, Count);

lExit:
    return hr;
}

_Use_decl_annotations_
HRESULT SConstantBuffer::GetRawValue(void *pData, uint32_t Offset, uint32_t Count)
{
    HRESULT hr = S_OK;    

#ifdef _DEBUG
    static LPCSTR pFuncName = "ID3DX11EffectVariable::GetRawValue";

    VERIFYPARAMETER(pData);

    if ((Offset + Count < Offset) ||
        (Count + (uint8_t*)pData < (uint8_t*)pData) ||
        ((Offset + Count) > Size))
    {
        // overflow of some kind
        DPF(0, "%s: Invalid range specified", pFuncName);
        VH(E_INVALIDARG);
    }
#endif

    memcpy(pData, pBackingStore + Offset, Count);

lExit:
    return hr;
}

bool SConstantBuffer::ClonedSingle() const
{
    return IsSingle && ( pEffect->m_Flags & D3DX11_EFFECT_CLONE );
}

HRESULT SConstantBuffer::SetConstantBuffer(_In_ ID3D11Buffer *pConstantBuffer)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectConstantBuffer::SetConstantBuffer";

    if (IsTBuffer)
    {
        DPF(0, "%s: This is a texture buffer; use SetTextureBuffer instead", pFuncName);
        VH(D3DERR_INVALIDCALL);
    }

    // Replace all references to the old shader block with this one
    pEffect->ReplaceCBReference(this, pConstantBuffer);

    if( !IsUserManaged )
    {
        // Save original cbuffer in case we UndoSet
        assert( pMemberData[0].Type == MDT_Buffer );
        VB( pMemberData[0].Data.pD3DEffectsManagedConstantBuffer == nullptr );
        pMemberData[0].Data.pD3DEffectsManagedConstantBuffer = pD3DObject;
        pD3DObject = nullptr;
        IsUserManaged = true;
        IsNonUpdatable = true;
    }

    SAFE_ADDREF( pConstantBuffer );
    SAFE_RELEASE( pD3DObject );
    pD3DObject = pConstantBuffer;

lExit:
    return hr;
}

HRESULT SConstantBuffer::GetConstantBuffer(_Outptr_ ID3D11Buffer **ppConstantBuffer)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectConstantBuffer::GetConstantBuffer";

    VERIFYPARAMETER(ppConstantBuffer);

    if (IsTBuffer)
    {
        DPF(0, "%s: This is a texture buffer; use GetTextureBuffer instead", pFuncName);
        VH(D3DERR_INVALIDCALL);
    }

    assert( pD3DObject );
    _Analysis_assume_( pD3DObject );
    *ppConstantBuffer = pD3DObject;
    SAFE_ADDREF(*ppConstantBuffer);

lExit:
    return hr;
}

HRESULT SConstantBuffer::UndoSetConstantBuffer() 
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectConstantBuffer::UndoSetConstantBuffer";

    if (IsTBuffer)
    {
        DPF(0, "%s: This is a texture buffer; use UndoSetTextureBuffer instead", pFuncName);
        VH(D3DERR_INVALIDCALL);
    }

    if( !IsUserManaged )
    {
        return S_FALSE;
    }

    // Replace all references to the old shader block with this one
    pEffect->ReplaceCBReference(this, pMemberData[0].Data.pD3DEffectsManagedConstantBuffer);

    // Revert to original cbuffer
    SAFE_RELEASE( pD3DObject );
    pD3DObject = pMemberData[0].Data.pD3DEffectsManagedConstantBuffer;
    pMemberData[0].Data.pD3DEffectsManagedConstantBuffer = nullptr;
    IsUserManaged = false;
    IsNonUpdatable = ClonedSingle();

lExit:
    return hr;
}

HRESULT SConstantBuffer::SetTextureBuffer(_In_ ID3D11ShaderResourceView *pTextureBuffer)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectConstantBuffer::SetTextureBuffer";

    if (!IsTBuffer)
    {
        DPF(0, "%s: This is a constant buffer; use SetConstantBuffer instead", pFuncName);
        VH(D3DERR_INVALIDCALL);
    }

    if( !IsUserManaged )
    {
        // Save original cbuffer and tbuffer in case we UndoSet
        assert( pMemberData[0].Type == MDT_Buffer );
        VB( pMemberData[0].Data.pD3DEffectsManagedConstantBuffer == nullptr );
        pMemberData[0].Data.pD3DEffectsManagedConstantBuffer = pD3DObject;
        pD3DObject = nullptr;
        assert( pMemberData[1].Type == MDT_ShaderResourceView );
        VB( pMemberData[1].Data.pD3DEffectsManagedTextureBuffer == nullptr );
        pMemberData[1].Data.pD3DEffectsManagedTextureBuffer = TBuffer.pShaderResource;
        TBuffer.pShaderResource = nullptr;
        IsUserManaged = true;
        IsNonUpdatable = true;
    }

    SAFE_ADDREF( pTextureBuffer );
    SAFE_RELEASE(pD3DObject); // won't be needing this anymore...
    SAFE_RELEASE( TBuffer.pShaderResource );
    TBuffer.pShaderResource = pTextureBuffer;

lExit:
    return hr;
}

HRESULT SConstantBuffer::GetTextureBuffer(_Outptr_ ID3D11ShaderResourceView **ppTextureBuffer)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectConstantBuffer::GetTextureBuffer";

    VERIFYPARAMETER(ppTextureBuffer);

    if (!IsTBuffer)
    {
        DPF(0, "%s: This is a constant buffer; use GetConstantBuffer instead", pFuncName);
        VH(D3DERR_INVALIDCALL);
    }

    assert( TBuffer.pShaderResource );
    _Analysis_assume_( TBuffer.pShaderResource );
    *ppTextureBuffer = TBuffer.pShaderResource;
    SAFE_ADDREF(*ppTextureBuffer);

lExit:
    return hr;
}

HRESULT SConstantBuffer::UndoSetTextureBuffer()
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectConstantBuffer::UndoSetTextureBuffer";

    if (!IsTBuffer)
    {
        DPF(0, "%s: This is a texture buffer; use UndoSetConstantBuffer instead", pFuncName);
        VH(D3DERR_INVALIDCALL);
    }

    if( !IsUserManaged )
    {
        return S_FALSE;
    }

    // Revert to original cbuffer
    SAFE_RELEASE( pD3DObject );
    pD3DObject = pMemberData[0].Data.pD3DEffectsManagedConstantBuffer;
    pMemberData[0].Data.pD3DEffectsManagedConstantBuffer = nullptr;
    SAFE_RELEASE( TBuffer.pShaderResource );
    TBuffer.pShaderResource = pMemberData[1].Data.pD3DEffectsManagedTextureBuffer;
    pMemberData[1].Data.pD3DEffectsManagedTextureBuffer = nullptr;
    IsUserManaged = false;
    IsNonUpdatable = ClonedSingle();

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectPass (CEffectPass implementation)
//////////////////////////////////////////////////////////////////////////

bool SPassBlock::IsValid()
{
    if( HasDependencies )
        return pEffect->ValidatePassBlock( this );
    return InitiallyValid;
}

HRESULT SPassBlock::GetDesc(_Out_ D3DX11_PASS_DESC *pDesc)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11EffectPass::GetDesc";

    VERIFYPARAMETER(pDesc);

    ZeroMemory(pDesc, sizeof(*pDesc));

    pDesc->Name = pName;
    pDesc->Annotations = AnnotationCount;
    
    SAssignment *pAssignment;
    SAssignment *pLastAssn;

    pEffect->IncrementTimer();

    pAssignment = pAssignments;
    pLastAssn = pAssignments + AssignmentCount;

    for(; pAssignment < pLastAssn; pAssignment++)
    {
        pEffect->EvaluateAssignment(pAssignment);
    }

    if( BackingStore.pVertexShaderBlock && BackingStore.pVertexShaderBlock->pInputSignatureBlob )
    {
        // pInputSignatureBlob can be null if we're setting a nullptr VS "SetVertexShader( nullptr )"
        pDesc->pIAInputSignature = (uint8_t*)BackingStore.pVertexShaderBlock->pInputSignatureBlob->GetBufferPointer();
        pDesc->IAInputSignatureSize = BackingStore.pVertexShaderBlock->pInputSignatureBlob->GetBufferSize();
    }

    pDesc->StencilRef = BackingStore.StencilRef;
    pDesc->SampleMask = BackingStore.SampleMask;
    pDesc->BlendFactor[0] = BackingStore.BlendFactor[0];
    pDesc->BlendFactor[1] = BackingStore.BlendFactor[1];
    pDesc->BlendFactor[2] = BackingStore.BlendFactor[2];
    pDesc->BlendFactor[3] = BackingStore.BlendFactor[3];

lExit:
    return hr;
}

extern SShaderBlock g_NullVS;
extern SShaderBlock g_NullGS;
extern SShaderBlock g_NullPS;
extern SShaderBlock g_NullHS;
extern SShaderBlock g_NullDS;
extern SShaderBlock g_NullCS;

SAnonymousShader g_AnonymousNullVS(&g_NullVS);
SAnonymousShader g_AnonymousNullGS(&g_NullGS);
SAnonymousShader g_AnonymousNullPS(&g_NullPS);
SAnonymousShader g_AnonymousNullHS(&g_NullHS);
SAnonymousShader g_AnonymousNullDS(&g_NullDS);
SAnonymousShader g_AnonymousNullCS(&g_NullCS);

template<EObjectType EShaderType>
HRESULT SPassBlock::GetShaderDescHelper(D3DX11_PASS_SHADER_DESC *pDesc)
{
    HRESULT hr = S_OK;
    uint32_t  i;
    LPCSTR pFuncName = nullptr;
    SShaderBlock *pShaderBlock = nullptr;

    ApplyPassAssignments();

#ifdef _PREFAST_
#pragma prefast(push)
#pragma prefast(disable:__WARNING_UNUSED_POINTER_ASSIGNMENT, "pFuncName used in DPF")
#endif
    switch (EShaderType)
    {
    case EOT_VertexShader:
    case EOT_VertexShader5:
        pFuncName = "ID3DX11EffectPass::GetVertexShaderDesc";
        pShaderBlock = BackingStore.pVertexShaderBlock;
        break;
    case EOT_PixelShader:
    case EOT_PixelShader5:
        pFuncName = "ID3DX11EffectPass::GetPixelShaderDesc";
        pShaderBlock = BackingStore.pPixelShaderBlock;
        break;
    case EOT_GeometryShader:
    case EOT_GeometryShader5:
        pFuncName = "ID3DX11EffectPass::GetGeometryShaderDesc";
        pShaderBlock = BackingStore.pGeometryShaderBlock;
        break;
    case EOT_HullShader5:
        pFuncName = "ID3DX11EffectPass::GetHullShaderDesc";
        pShaderBlock = BackingStore.pHullShaderBlock;
        break;
    case EOT_DomainShader5:
        pFuncName = "ID3DX11EffectPass::GetDomainShaderDesc";
        pShaderBlock = BackingStore.pDomainShaderBlock;
        break;
    case EOT_ComputeShader5:
        pFuncName = "ID3DX11EffectPass::GetComputeShaderDesc";
        pShaderBlock = BackingStore.pComputeShaderBlock;
        break;
#ifdef _PREFAST_
#pragma prefast(pop)
#endif
    default:
        assert(0);
    }

    VERIFYPARAMETER(pDesc);

    // in case of error (or in case the assignment doesn't exist), return something reasonable
    pDesc->pShaderVariable = &g_InvalidShaderVariable;
    pDesc->ShaderIndex = 0;

    if (nullptr != pShaderBlock)
    {
        uint32_t elements, varCount, anonymousShaderCount;
        SGlobalVariable *pVariables;
        SAnonymousShader *pAnonymousShaders;

        if (pShaderBlock == &g_NullVS)
        {
            pDesc->pShaderVariable = &g_AnonymousNullVS;
            pDesc->ShaderIndex = 0;
            // we're done
            goto lExit;
        }
        else if (pShaderBlock == &g_NullGS)
        {
            pDesc->pShaderVariable = &g_AnonymousNullGS;
            pDesc->ShaderIndex = 0;
            // we're done
            goto lExit;
        }
        else if (pShaderBlock == &g_NullPS)
        {
            pDesc->pShaderVariable = &g_AnonymousNullPS;
            pDesc->ShaderIndex = 0;
            // we're done
            goto lExit;
        }
        else if (pShaderBlock == &g_NullHS)
        {
            pDesc->pShaderVariable = &g_AnonymousNullHS;
            pDesc->ShaderIndex = 0;
            // we're done
            goto lExit;
        }
        else if (pShaderBlock == &g_NullDS)
        {
            pDesc->pShaderVariable = &g_AnonymousNullDS;
            pDesc->ShaderIndex = 0;
            // we're done
            goto lExit;
        }
        else if (pShaderBlock == &g_NullCS)
        {
            pDesc->pShaderVariable = &g_AnonymousNullCS;
            pDesc->ShaderIndex = 0;
            // we're done
            goto lExit;
        }
        else 
        {
            VB( pEffect->IsRuntimeData(pShaderBlock) );
            varCount = pEffect->m_VariableCount;
            pVariables = pEffect->m_pVariables;
            anonymousShaderCount = pEffect->m_AnonymousShaderCount;
            pAnonymousShaders = pEffect->m_pAnonymousShaders;
        }

        for (i = 0; i < varCount; ++ i)
        {
            elements = std::max<uint32_t>(1, pVariables[i].pType->Elements);
            // make sure the variable type matches, and don't forget about GeometryShaderSO's
            if (pVariables[i].pType->IsShader())
            {
                if (pShaderBlock >= pVariables[i].Data.pShader && pShaderBlock < pVariables[i].Data.pShader + elements)
                {
                    pDesc->pShaderVariable = (ID3DX11EffectShaderVariable *)(pVariables + i);
                    pDesc->ShaderIndex = (uint32_t)(UINT_PTR)(pShaderBlock - pVariables[i].Data.pShader);
                    // we're done
                    goto lExit;
                }
            }
        }

        for (i = 0; i < anonymousShaderCount; ++ i)
        {
            if (pShaderBlock == pAnonymousShaders[i].pShaderBlock)
            {
                VB(EShaderType == pAnonymousShaders[i].pShaderBlock->GetShaderType())
                pDesc->pShaderVariable = (pAnonymousShaders + i);
                pDesc->ShaderIndex = 0;
                // we're done
                goto lExit;
            }
        }

        DPF(0, "%s: Internal error; shader not found", pFuncName);
        VH( E_FAIL );
    }

lExit:
    return hr;
}

HRESULT SPassBlock::GetVertexShaderDesc(_Out_ D3DX11_PASS_SHADER_DESC *pDesc)
{
    return GetShaderDescHelper<EOT_VertexShader>(pDesc);
}

HRESULT SPassBlock::GetPixelShaderDesc(_Out_ D3DX11_PASS_SHADER_DESC *pDesc)
{
    return GetShaderDescHelper<EOT_PixelShader>(pDesc);
}

HRESULT SPassBlock::GetGeometryShaderDesc(_Out_ D3DX11_PASS_SHADER_DESC *pDesc)
{
    return GetShaderDescHelper<EOT_GeometryShader>(pDesc);
}

HRESULT SPassBlock::GetHullShaderDesc(_Out_ D3DX11_PASS_SHADER_DESC *pDesc)
{
    return GetShaderDescHelper<EOT_HullShader5>(pDesc);
}

HRESULT SPassBlock::GetDomainShaderDesc(_Out_ D3DX11_PASS_SHADER_DESC *pDesc)
{
    return GetShaderDescHelper<EOT_DomainShader5>(pDesc);
}

HRESULT SPassBlock::GetComputeShaderDesc(_Out_ D3DX11_PASS_SHADER_DESC *pDesc)
{
    return GetShaderDescHelper<EOT_ComputeShader5>(pDesc);
}

ID3DX11EffectVariable * SPassBlock::GetAnnotationByIndex(_In_ uint32_t Index)
{
    return GetAnnotationByIndexHelper("ID3DX11EffectPass", Index, AnnotationCount, pAnnotations);
}

ID3DX11EffectVariable * SPassBlock::GetAnnotationByName(_In_z_ LPCSTR Name)
{
    return GetAnnotationByNameHelper("ID3DX11EffectPass", Name, AnnotationCount, pAnnotations);
}

HRESULT SPassBlock::Apply(_In_ uint32_t Flags, _In_ ID3D11DeviceContext* pContext)

{
    UNREFERENCED_PARAMETER(Flags);
    HRESULT hr = S_OK;

    // Flags are unused, so should be 0


    assert( pEffect->m_pContext == nullptr );
    pEffect->m_pContext = pContext;
    pEffect->ApplyPassBlock(this);
    pEffect->m_pContext = nullptr;

    return hr;
}

HRESULT SPassBlock::ComputeStateBlockMask(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask)
{
    HRESULT hr = S_OK;
    
    // flags indicating whether the following shader types were caught by assignment checks or not
    bool bVS = false, bGS = false, bPS = false, bHS = false, bDS = false, bCS = false;

    for (size_t i = 0; i < AssignmentCount; ++ i)
    {
        bool bShader = false;
        
        switch (pAssignments[i].LhsType)
        {
        case ELHS_VertexShaderBlock:
            bVS = true;
            bShader = true;
            break;
        case ELHS_GeometryShaderBlock:
            bGS = true;
            bShader = true;
            break;
        case ELHS_PixelShaderBlock:
            bPS = true;
            bShader = true;
            break;
        case ELHS_HullShaderBlock:
            bHS = true;
            bShader = true;
            break;
        case ELHS_DomainShaderBlock:
            bDS = true;
            bShader = true;
            break;
        case ELHS_ComputeShaderBlock:
            bCS = true;
            bShader = true;
            break;

        case ELHS_RasterizerBlock:
            pStateBlockMask->RSRasterizerState = 1;
            break;
        case ELHS_BlendBlock:
            pStateBlockMask->OMBlendState = 1;
            break;
        case ELHS_DepthStencilBlock:
            pStateBlockMask->OMDepthStencilState = 1;
            break;

        default:            
            // ignore this assignment (must be a scalar/vector assignment associated with a state object)
            break;
        }

        if (bShader)
        {
            for (size_t j = 0; j < pAssignments[i].MaxElements; ++ j)
            {
                // compute state block mask for the union of ALL shaders
                VH( pAssignments[i].Source.pShader[j].ComputeStateBlockMask(pStateBlockMask) );
            }
        }
    }

    // go over the state block objects in case there was no corresponding assignment
    if (nullptr != BackingStore.pRasterizerBlock)
    {
        pStateBlockMask->RSRasterizerState = 1;
    }
    if (nullptr != BackingStore.pBlendBlock)
    {
        pStateBlockMask->OMBlendState = 1;
    }
    if (nullptr != BackingStore.pDepthStencilBlock)
    {
        pStateBlockMask->OMDepthStencilState = 1;
    }

    // go over the shaders only if an assignment didn't already catch them
    if (false == bVS && nullptr != BackingStore.pVertexShaderBlock)
    {
        VH( BackingStore.pVertexShaderBlock->ComputeStateBlockMask(pStateBlockMask) );
    }
    if (false == bGS && nullptr != BackingStore.pGeometryShaderBlock)
    {
        VH( BackingStore.pGeometryShaderBlock->ComputeStateBlockMask(pStateBlockMask) );
    }
    if (false == bPS && nullptr != BackingStore.pPixelShaderBlock)
    {
        VH( BackingStore.pPixelShaderBlock->ComputeStateBlockMask(pStateBlockMask) );
    }
    if (false == bHS && nullptr != BackingStore.pHullShaderBlock)
    {
        VH( BackingStore.pHullShaderBlock->ComputeStateBlockMask(pStateBlockMask) );
    }
    if (false == bDS && nullptr != BackingStore.pDomainShaderBlock)
    {
        VH( BackingStore.pDomainShaderBlock->ComputeStateBlockMask(pStateBlockMask) );
    }
    if (false == bCS && nullptr != BackingStore.pComputeShaderBlock)
    {
        VH( BackingStore.pComputeShaderBlock->ComputeStateBlockMask(pStateBlockMask) );
    }
    
lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectTechnique (STechnique implementation)
//////////////////////////////////////////////////////////////////////////

bool STechnique::IsValid()
{ 
    if( HasDependencies )
    {
        for( size_t i = 0; i < PassCount; i++ )
        {
            if( !((SPassBlock*)pPasses)[i].IsValid() )
                return false;
        }
        return true;
    }
    return InitiallyValid;
}

HRESULT STechnique::GetDesc(_Out_ D3DX11_TECHNIQUE_DESC *pDesc)
{
    HRESULT hr = S_OK;

    static LPCSTR pFuncName = "ID3DX11EffectTechnique::GetDesc";

    VERIFYPARAMETER(pDesc);

    pDesc->Name = pName;
    pDesc->Annotations = AnnotationCount;
    pDesc->Passes = PassCount;

lExit:
    return hr;
}

ID3DX11EffectVariable * STechnique::GetAnnotationByIndex(_In_ uint32_t Index)
{
    return GetAnnotationByIndexHelper("ID3DX11EffectTechnique", Index, AnnotationCount, pAnnotations);
}

ID3DX11EffectVariable * STechnique::GetAnnotationByName(_In_z_ LPCSTR Name)
{
    return GetAnnotationByNameHelper("ID3DX11EffectTechnique", Name, AnnotationCount, pAnnotations);
}

ID3DX11EffectPass * STechnique::GetPassByIndex(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectTechnique::GetPassByIndex";

    if (Index >= PassCount)
    {
        DPF(0, "%s: Invalid pass index (%u, total: %u)", pFuncName, Index, PassCount);
        return &g_InvalidPass;
    }

    return (ID3DX11EffectPass *)(pPasses + Index);
}

ID3DX11EffectPass * STechnique::GetPassByName(_In_z_ LPCSTR Name)
{
    static LPCSTR pFuncName = "ID3DX11EffectTechnique::GetPassByName";

    uint32_t  i;

    for (i = 0; i < PassCount; ++ i)
    {
        if (nullptr != pPasses[i].pName &&
            strcmp(pPasses[i].pName, Name) == 0)
        {
            break;
        }
    }

    if (i == PassCount)
    {
        DPF(0, "%s: Pass [%s] not found", pFuncName, Name);
        return &g_InvalidPass;
    }

    return (ID3DX11EffectPass *)(pPasses + i);
}

HRESULT STechnique::ComputeStateBlockMask(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask)
{
    HRESULT hr = S_OK;
    uint32_t i;

    _Analysis_assume_( PassCount == 0 || pPasses != 0 );
    for (i = 0; i < PassCount; ++ i)
    {
        VH( ((SPassBlock*)pPasses)[i].ComputeStateBlockMask(pStateBlockMask) );
    }

lExit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11EffectGroup (SGroup implementation)
//////////////////////////////////////////////////////////////////////////

bool SGroup::IsValid()
{ 
    if( HasDependencies )
    {
        for( size_t i = 0; i < TechniqueCount; i++ )
        {
            if( !((STechnique*)pTechniques)[i].IsValid() )
                return false;
        }
        return true;
    }
    return InitiallyValid;
}

HRESULT SGroup::GetDesc(_Out_ D3DX11_GROUP_DESC *pDesc)
{
    HRESULT hr = S_OK;

    static LPCSTR pFuncName = "ID3DX11EffectGroup::GetDesc";

    VERIFYPARAMETER(pDesc);

    pDesc->Name = pName;
    pDesc->Annotations = AnnotationCount;
    pDesc->Techniques = TechniqueCount;

lExit:
    return hr;
}

ID3DX11EffectVariable * SGroup::GetAnnotationByIndex(_In_ uint32_t Index)
{
    return GetAnnotationByIndexHelper("ID3DX11EffectGroup", Index, AnnotationCount, pAnnotations);
}

ID3DX11EffectVariable * SGroup::GetAnnotationByName(_In_z_ LPCSTR Name)
{
    return GetAnnotationByNameHelper("ID3DX11EffectGroup", Name, AnnotationCount, pAnnotations);
}

ID3DX11EffectTechnique * SGroup::GetTechniqueByIndex(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11EffectGroup::GetTechniqueByIndex";

    if (Index >= TechniqueCount)
    {
        DPF(0, "%s: Invalid pass index (%u, total: %u)", pFuncName, Index, TechniqueCount);
        return &g_InvalidTechnique;
    }

    return (ID3DX11EffectTechnique *)(pTechniques + Index);
}

ID3DX11EffectTechnique * SGroup::GetTechniqueByName(_In_z_ LPCSTR Name)
{
    static LPCSTR pFuncName = "ID3DX11EffectGroup::GetTechniqueByName";

    uint32_t  i;

    for (i = 0; i < TechniqueCount; ++ i)
    {
        if (nullptr != pTechniques[i].pName &&
            strcmp(pTechniques[i].pName, Name) == 0)
        {
            break;
        }
    }

    if (i == TechniqueCount)
    {
        DPF(0, "%s: Technique [%s] not found", pFuncName, Name);
        return &g_InvalidTechnique;
    }

    return (ID3DX11EffectTechnique *)(pTechniques + i);
}

//////////////////////////////////////////////////////////////////////////
// ID3DX11Effect Public Reflection APIs (CEffect)
//////////////////////////////////////////////////////////////////////////

HRESULT CEffect::GetDevice(_Outptr_ ID3D11Device **ppDevice)
{
    HRESULT hr = S_OK;
    static LPCSTR pFuncName = "ID3DX11Effect::GetDevice";
    VERIFYPARAMETER(ppDevice);

    m_pDevice->AddRef();
    *ppDevice = m_pDevice;

lExit:
    return hr;
}

HRESULT CEffect::GetDesc(_Out_ D3DX11_EFFECT_DESC *pDesc)
{
    HRESULT hr = S_OK;

    static LPCSTR pFuncName = "ID3DX11Effect::GetDesc";

    VERIFYPARAMETER(pDesc);

    pDesc->ConstantBuffers = m_CBCount;
    pDesc->GlobalVariables = m_VariableCount;
    pDesc->Techniques = m_TechniqueCount;
    pDesc->Groups = m_GroupCount;
    pDesc->InterfaceVariables = m_InterfaceCount;

lExit:
    return hr;    
}

ID3DX11EffectConstantBuffer * CEffect::GetConstantBufferByIndex(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetConstantBufferByIndex";

    if (Index < m_CBCount)
    {
        return m_pCBs + Index;
    }

    DPF(0, "%s: Invalid constant buffer index", pFuncName);
    return &g_InvalidConstantBuffer;
}

ID3DX11EffectConstantBuffer * CEffect::GetConstantBufferByName(_In_z_ LPCSTR Name)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetConstantBufferByName";

    if (IsOptimized())
    {
        DPF(0, "%s: Cannot get constant buffer interfaces by name since the effect has been Optimize()'ed", pFuncName);
        return &g_InvalidConstantBuffer;
    }

    if (nullptr == Name)
    {
        DPF(0, "%s: Parameter Name was nullptr.", pFuncName);
        return &g_InvalidConstantBuffer;
    }

    for (uint32_t i = 0; i < m_CBCount; ++ i)
    {
        if (strcmp(m_pCBs[i].pName, Name) == 0)
        {
            return m_pCBs + i;
        }
    }

    DPF(0, "%s: Constant Buffer [%s] not found", pFuncName, Name);
    return &g_InvalidConstantBuffer;
}

ID3DX11EffectVariable * CEffect::GetVariableByIndex(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetVariableByIndex";

    if (Index < m_VariableCount)
    {
        return m_pVariables + Index;
    }

    DPF(0, "%s: Invalid variable index", pFuncName);
    return &g_InvalidScalarVariable;
}

ID3DX11EffectVariable * CEffect::GetVariableByName(_In_z_ LPCSTR Name)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetVariableByName";

    if (IsOptimized())
    {
        DPF(0, "%s: Cannot get variable interfaces by name since the effect has been Optimize()'ed", pFuncName);
        return &g_InvalidScalarVariable;
    }

    if (nullptr == Name)
    {
        DPF(0, "%s: Parameter Name was nullptr.", pFuncName);
        return &g_InvalidScalarVariable;
    }

    for (uint32_t i = 0; i < m_VariableCount; ++ i)
    {
        if (strcmp(m_pVariables[i].pName, Name) == 0)
        {
            return m_pVariables + i;
        }
    }

    DPF(0, "%s: Variable [%s] not found", pFuncName, Name);
    return &g_InvalidScalarVariable;
}

ID3DX11EffectVariable * CEffect::GetVariableBySemantic(_In_z_ LPCSTR Semantic)
{    
    static LPCSTR pFuncName = "ID3DX11Effect::GetVariableBySemantic";

    if (IsOptimized())
    {
        DPF(0, "%s: Cannot get variable interfaces by semantic since the effect has been Optimize()'ed", pFuncName);
        return &g_InvalidScalarVariable;
    }

    if (nullptr == Semantic)
    {
        DPF(0, "%s: Parameter Semantic was nullptr.", pFuncName);
        return &g_InvalidScalarVariable;
    }

    uint32_t  i;

    for (i = 0; i < m_VariableCount; ++ i)
    {
        if (nullptr != m_pVariables[i].pSemantic && 
            _stricmp(m_pVariables[i].pSemantic, Semantic) == 0)
        {
            return (ID3DX11EffectVariable *)(m_pVariables + i);
        }
    }

    DPF(0, "%s: Variable with semantic [%s] not found", pFuncName, Semantic);
    return &g_InvalidScalarVariable;
}

ID3DX11EffectTechnique * CEffect::GetTechniqueByIndex(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetTechniqueByIndex";

    if( Index < m_TechniqueCount )
    {
        for( size_t i=0; i < m_GroupCount; i++ )
        {
            if( Index < m_pGroups[i].TechniqueCount )
            {
                return (ID3DX11EffectTechnique *)(m_pGroups[i].pTechniques + Index);
            }
            Index -= m_pGroups[i].TechniqueCount;
        }
        assert( false );
    }
    DPF(0, "%s: Invalid technique index (%u)", pFuncName, Index);
    return &g_InvalidTechnique;
}

ID3DX11EffectTechnique * CEffect::GetTechniqueByName(_In_z_ LPCSTR Name)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetTechniqueByName";
    const size_t MAX_GROUP_TECHNIQUE_SIZE = 256;
    char NameCopy[MAX_GROUP_TECHNIQUE_SIZE];

    if (IsOptimized())
    {
        DPF(0, "ID3DX11Effect::GetTechniqueByName: Cannot get technique interfaces by name since the effect has been Optimize()'ed");
        return &g_InvalidTechnique;
    }

    if (nullptr == Name)
    {
        DPF(0, "%s: Parameter Name was nullptr.", pFuncName);
        return &g_InvalidTechnique;
    }

    if( FAILED( strcpy_s( NameCopy, MAX_GROUP_TECHNIQUE_SIZE, Name ) ) )
    {
        DPF( 0, "Group|Technique name has a length greater than %zu.", MAX_GROUP_TECHNIQUE_SIZE );
        return &g_InvalidTechnique;
    }

    char* pDelimiter = strchr( NameCopy, '|' );
    if( pDelimiter == nullptr )
    {
        if ( m_pNullGroup == nullptr )
        {
            DPF( 0, "The effect contains no default group." );
            return &g_InvalidTechnique;
        }

        return m_pNullGroup->GetTechniqueByName( Name );
    }

    // separate group name and technique name
    *pDelimiter = 0; 

    return GetGroupByName( NameCopy )->GetTechniqueByName( pDelimiter + 1 );
}

ID3D11ClassLinkage * CEffect::GetClassLinkage()
{
    SAFE_ADDREF( m_pClassLinkage );
    return m_pClassLinkage;
}

ID3DX11EffectGroup * CEffect::GetGroupByIndex(_In_ uint32_t Index)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetGroupByIndex";

    if( Index < m_GroupCount )
    {
        return (ID3DX11EffectGroup *)(m_pGroups + Index);
    }
    DPF(0, "%s: Invalid group index (%u)", pFuncName, Index);
    return &g_InvalidGroup;
}

ID3DX11EffectGroup * CEffect::GetGroupByName(_In_z_ LPCSTR Name)
{
    static LPCSTR pFuncName = "ID3DX11Effect::GetGroupByName";

    if (IsOptimized())
    {
        DPF(0, "ID3DX11Effect::GetGroupByName: Cannot get group interfaces by name since the effect has been Optimize()'ed");
        return &g_InvalidGroup;
    }

    if (nullptr == Name || Name[0] == 0 )
    {
        return m_pNullGroup ? (ID3DX11EffectGroup *)m_pNullGroup : &g_InvalidGroup;
    }

    uint32_t i = 0;
    for (; i < m_GroupCount; ++ i)
    {
        if (nullptr != m_pGroups[i].pName && 
            strcmp(m_pGroups[i].pName, Name) == 0)
        {
            break;
        }
    }

    if (i == m_GroupCount)
    {
        DPF(0, "%s: Group [%s] not found", pFuncName, Name);
        return &g_InvalidGroup;
    }

    return (ID3DX11EffectGroup *)(m_pGroups + i);
}

}

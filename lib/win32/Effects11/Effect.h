//--------------------------------------------------------------------------------------
// File: Effect.h
//
//  Direct3D 11 Effects Header for ID3DX11Effect Implementation
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

#include "EffectBinaryFormat.h"
#include "IUnknownImp.h"

#ifdef _DEBUG
extern void __cdecl D3DXDebugPrintf(UINT lvl, _In_z_ _Printf_format_string_ LPCSTR szFormat, ...);
#define DPF D3DXDebugPrintf
#else
#define DPF
#endif

#pragma warning(push)
#pragma warning(disable : 4481)
// VS 2010 considers 'override' to be a extension, but it's part of C++11 as of VS 2012

//////////////////////////////////////////////////////////////////////////

using namespace D3DX11Core;

namespace D3DX11Effects
{

//////////////////////////////////////////////////////////////////////////
// Forward defines
//////////////////////////////////////////////////////////////////////////

struct SBaseBlock;
struct SShaderBlock;
struct SPassBlock;
struct SClassInstance;
struct SInterface;
struct SShaderResource;
struct SUnorderedAccessView;
struct SRenderTargetView;
struct SDepthStencilView;
struct SSamplerBlock;
struct SDepthStencilBlock;
struct SBlendBlock;
struct SRasterizerBlock;
struct SString;
struct SD3DShaderVTable;
struct SClassInstanceGlobalVariable;

struct SAssignment;
struct SVariable;
struct SGlobalVariable;
struct SAnnotation;
struct SConstantBuffer;

class CEffect;
class CEffectLoader;

enum ELhsType : int;

// Allows the use of 32-bit and 64-bit timers depending on platform type
typedef size_t Timer;

//////////////////////////////////////////////////////////////////////////
// Reflection & Type structures
//////////////////////////////////////////////////////////////////////////

// CEffectMatrix is used internally instead of float arrays
struct CEffectMatrix 
{
    union 
    {
        struct 
        {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };
};

struct CEffectVector4 
{
    float x;
    float y;
    float z;
    float w;
};

union UDataPointer
{
    void                    *pGeneric;
    uint8_t                 *pNumeric; 
    float                   *pNumericFloat;
    uint32_t                *pNumericDword;
    int                     *pNumericInt;
    BOOL                    *pNumericBool;
    SString                 *pString;
    SShaderBlock            *pShader;
    SBaseBlock              *pBlock;
    SBlendBlock             *pBlend;
    SDepthStencilBlock      *pDepthStencil;
    SRasterizerBlock        *pRasterizer;
    SInterface              *pInterface;
    SShaderResource         *pShaderResource;
    SUnorderedAccessView    *pUnorderedAccessView;
    SRenderTargetView       *pRenderTargetView;
    SDepthStencilView       *pDepthStencilView;
    SSamplerBlock           *pSampler;
    CEffectVector4          *pVector;
    CEffectMatrix           *pMatrix;
    UINT_PTR                Offset;
};

enum EMemberDataType
{
    MDT_ClassInstance,
    MDT_BlendState,
    MDT_DepthStencilState,
    MDT_RasterizerState,
    MDT_SamplerState,
    MDT_Buffer,
    MDT_ShaderResourceView,
};

struct SMemberDataPointer
{
    EMemberDataType             Type;
    union
    {
        IUnknown                *pGeneric;
        ID3D11ClassInstance     *pD3DClassInstance;
        ID3D11BlendState        *pD3DEffectsManagedBlendState;
        ID3D11DepthStencilState *pD3DEffectsManagedDepthStencilState;
        ID3D11RasterizerState   *pD3DEffectsManagedRasterizerState;
        ID3D11SamplerState      *pD3DEffectsManagedSamplerState;
        ID3D11Buffer            *pD3DEffectsManagedConstantBuffer;
        ID3D11ShaderResourceView*pD3DEffectsManagedTextureBuffer;
    } Data;
};

struct SType : public ID3DX11EffectType
{   
    static const UINT_PTR c_InvalidIndex = (uint32_t) -1;
    static const uint32_t c_ScalarSize = sizeof(uint32_t);

    // packing rule constants
    static const uint32_t c_ScalarsPerRegister = 4;
    static const uint32_t c_RegisterSize = c_ScalarsPerRegister * c_ScalarSize; // must be a power of 2!!    
    
    EVarType    VarType;        // numeric, object, struct
    uint32_t    Elements;       // # of array elements (0 for non-arrays)
    char        *pTypeName;     // friendly name of the type: "VS_OUTPUT", "float4", etc.

    // *Size and stride values are always 0 for object types
    // *Annotations adhere to packing rules (even though they do not reside in constant buffers)
    //      for consistency's sake
    //
    // Packing rules:
    // *Structures and array elements are always register aligned
    // *Single-row values (or, for column major matrices, single-column) are greedily
    //  packed unless doing so would span a register boundary, in which case they are
    //  register aligned

    uint32_t    TotalSize;      // Total size of this data type in a constant buffer from
                                // start to finish (padding in between elements is included,
                                // but padding at the end is not since that would require
                                // knowledge of the following data type).

    uint32_t    Stride;         // Number of bytes to advance between elements.
                                // Typically a multiple of 16 for arrays, vectors, matrices.
                                // For scalars and small vectors/matrices, this can be 4 or 8.    

    uint32_t    PackedSize;     // Size, in bytes, of this data typed when fully packed

    union
    {        
        SBinaryNumericType  NumericType;
        EObjectType         ObjectType;         // not all values of EObjectType are valid here (e.g. constant buffer)
        struct
        {
            SVariable   *pMembers;              // array of type instances describing structure members
            uint32_t    Members;
            BOOL        ImplementsInterface;    // true if this type implements an interface
            BOOL        HasSuperClass;          // true if this type has a parent class
        }               StructType;
        void*           InterfaceType;          // nothing for interfaces
    };


    SType() noexcept :
       VarType(EVT_Invalid),
       Elements(0),
       pTypeName(nullptr),
       TotalSize(0),
       Stride(0),
       PackedSize(0),
       StructType{}
    {
        static_assert(sizeof(NumericType) <= sizeof(StructType), "SType union issue");
        static_assert(sizeof(ObjectType) <= sizeof(StructType), "SType union issue");
        static_assert(sizeof(InterfaceType) <= sizeof(StructType), "SType union issue");
    }

    bool IsEqual(SType *pOtherType) const;
    
    bool IsObjectType(EObjectType ObjType) const
    {
        return IsObjectTypeHelper(VarType, ObjectType, ObjType);
    }
    bool IsShader() const
    {
        return IsShaderHelper(VarType, ObjectType);
    }
    bool BelongsInConstantBuffer() const
    {
        return (VarType == EVT_Numeric) || (VarType == EVT_Struct);
    }
    bool IsStateBlockObject() const
    {
        return IsStateBlockObjectHelper(VarType, ObjectType);
    }
    bool IsClassInstance() const
    {
        return (VarType == EVT_Struct) && StructType.ImplementsInterface;
    }
    bool IsInterface() const
    {
        return IsInterfaceHelper(VarType, ObjectType);
    }
    bool IsShaderResource() const
    {
        return IsShaderResourceHelper(VarType, ObjectType);
    }
    bool IsUnorderedAccessView() const
    {
        return IsUnorderedAccessViewHelper(VarType, ObjectType);
    }
    bool IsSampler() const
    {
        return IsSamplerHelper(VarType, ObjectType);
    }
    bool IsRenderTargetView() const
    {
        return IsRenderTargetViewHelper(VarType, ObjectType);
    }
    bool IsDepthStencilView() const
    {
        return IsDepthStencilViewHelper(VarType, ObjectType);
    }

    uint32_t GetTotalUnpackedSize(_In_ bool IsSingleElement) const; 
    uint32_t GetTotalPackedSize(_In_ bool IsSingleElement) const; 
    HRESULT GetDescHelper(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc, _In_ bool IsSingleElement) const;

    STDMETHOD_(bool, IsValid)() override { return true; }
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc) override { return GetDescHelper(pDesc, false); }
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByName)(_In_z_ LPCSTR Name) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeBySemantic)(_In_z_ LPCSTR Semantic) override;
    STDMETHOD_(LPCSTR, GetMemberName)(_In_ uint32_t Index) override;
    STDMETHOD_(LPCSTR, GetMemberSemantic)(_In_ uint32_t Index) override;

    IUNKNOWN_IMP(SType, ID3DX11EffectType, IUnknown);
};

// Represents a type structure for a single element.
// It seems pretty trivial, but it has a different virtual table which enables
// us to accurately represent a type that consists of a single element
struct SSingleElementType : public ID3DX11EffectType
{
    SType *pType;

    STDMETHOD_(bool, IsValid)() override { return true; }
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc) override { return ((SType*)pType)->GetDescHelper(pDesc, true); }
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByIndex)(uint32_t Index) override { return ((SType*)pType)->GetMemberTypeByIndex(Index); }
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByName)(LPCSTR Name) override { return ((SType*)pType)->GetMemberTypeByName(Name); }
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeBySemantic)(LPCSTR Semantic) override { return ((SType*)pType)->GetMemberTypeBySemantic(Semantic); }
    STDMETHOD_(LPCSTR, GetMemberName)(uint32_t Index) override { return ((SType*)pType)->GetMemberName(Index); }
    STDMETHOD_(LPCSTR, GetMemberSemantic)(uint32_t Index) override { return ((SType*)pType)->GetMemberSemantic(Index); }

    IUNKNOWN_IMP(SSingleElementType, ID3DX11EffectType, IUnknown);

    SSingleElementType() noexcept :
        pType(nullptr)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
// Block definitions
//////////////////////////////////////////////////////////////////////////

void * GetBlockByIndex(EVarType VarType, EObjectType ObjectType, void *pBaseBlock, uint32_t Index);

struct SBaseBlock
{
    EBlockType      BlockType;

    bool            IsUserManaged:1;

    uint32_t        AssignmentCount;
    SAssignment     *pAssignments;

    SBaseBlock() noexcept;

    bool ApplyAssignments(CEffect *pEffect);

    inline SSamplerBlock *AsSampler() const
    {
        assert( BlockType == EBT_Sampler );
        return (SSamplerBlock*) this;
    }

    inline SDepthStencilBlock *AsDepthStencil() const
    {
        assert( BlockType == EBT_DepthStencil );
        return (SDepthStencilBlock*) this;
    }

    inline SBlendBlock *AsBlend() const
    {
        assert( BlockType == EBT_Blend );
        return (SBlendBlock*) this;
    }

    inline SRasterizerBlock *AsRasterizer() const
    {
        assert( BlockType == EBT_Rasterizer );
        return (SRasterizerBlock*) this;
    }

    inline SPassBlock *AsPass() const
    {
        assert( BlockType == EBT_Pass );
        return (SPassBlock*) this;
    }
};

struct STechnique : public ID3DX11EffectTechnique
{
    char        *pName;

    uint32_t    PassCount;
    SPassBlock  *pPasses;

    uint32_t    AnnotationCount;
    SAnnotation *pAnnotations;

    bool        InitiallyValid;
    bool        HasDependencies;

    STechnique() noexcept;

    STDMETHOD_(bool, IsValid)() override;
    STDMETHOD(GetDesc)(_Out_ D3DX11_TECHNIQUE_DESC *pDesc) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD_(ID3DX11EffectPass*, GetPassByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectPass*, GetPassByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD(ComputeStateBlockMask)(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask) override;

    IUNKNOWN_IMP(STechnique, ID3DX11EffectTechnique, IUnknown);
};

struct SGroup : public ID3DX11EffectGroup
{
    char        *pName;

    uint32_t    TechniqueCount;
    STechnique  *pTechniques;

    uint32_t    AnnotationCount;
    SAnnotation *pAnnotations;

    bool        InitiallyValid;
    bool        HasDependencies;

    SGroup() noexcept;

    STDMETHOD_(bool, IsValid)() override;
    STDMETHOD(GetDesc)(_Out_ D3DX11_GROUP_DESC *pDesc) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByName)(_In_z_ LPCSTR Name) override;

    IUNKNOWN_IMP(SGroup, ID3DX11EffectGroup, IUnknown);
};

struct SPassBlock : SBaseBlock, public ID3DX11EffectPass
{
    struct
    {
        ID3D11BlendState*       pBlendState;
        FLOAT                   BlendFactor[4];
        uint32_t                SampleMask;
        ID3D11DepthStencilState *pDepthStencilState;
        uint32_t                StencilRef;
        union
        {
            D3D11_SO_DECLARATION_ENTRY  *pEntry;
            char                        *pEntryDesc;
        }                       GSSODesc;

        // Pass assignments can write directly into these
        SBlendBlock             *pBlendBlock;
        SDepthStencilBlock      *pDepthStencilBlock;
        SRasterizerBlock        *pRasterizerBlock;
        uint32_t                RenderTargetViewCount;
        SRenderTargetView       *pRenderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        SDepthStencilView       *pDepthStencilView;
        SShaderBlock            *pVertexShaderBlock;
        SShaderBlock            *pPixelShaderBlock;
        SShaderBlock            *pGeometryShaderBlock;
        SShaderBlock            *pComputeShaderBlock;
        SShaderBlock            *pDomainShaderBlock;
        SShaderBlock            *pHullShaderBlock;
    }           BackingStore;

    char        *pName;

    uint32_t    AnnotationCount;
    SAnnotation *pAnnotations;

    CEffect     *pEffect;

    bool        InitiallyValid;         // validity of all state objects and shaders in pass upon BindToDevice
    bool        HasDependencies;        // if pass expressions or pass state blocks have dependencies on variables (if true, IsValid != InitiallyValid possibly)

    SPassBlock() noexcept;

    void ApplyPassAssignments();
    bool CheckShaderDependencies( _In_ const SShaderBlock* pBlock );
    bool CheckDependencies();

    template<EObjectType EShaderType>
    HRESULT GetShaderDescHelper(_Out_ D3DX11_PASS_SHADER_DESC *pDesc);

    STDMETHOD_(bool, IsValid)() override;
    STDMETHOD(GetDesc)(_Out_ D3DX11_PASS_DESC *pDesc) override;

    STDMETHOD(GetVertexShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override;
    STDMETHOD(GetGeometryShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override;
    STDMETHOD(GetPixelShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override;
    STDMETHOD(GetHullShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override;
    STDMETHOD(GetDomainShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override;
    STDMETHOD(GetComputeShaderDesc)(_Out_ D3DX11_PASS_SHADER_DESC *pDesc) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD(Apply)(_In_ uint32_t Flags, _In_ ID3D11DeviceContext* pContext) override;
    
    STDMETHOD(ComputeStateBlockMask)(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask) override;

    IUNKNOWN_IMP(SPassBlock, ID3DX11EffectPass, IUnknown);
};

struct SDepthStencilBlock : SBaseBlock
{
    ID3D11DepthStencilState *pDSObject;
    D3D11_DEPTH_STENCIL_DESC BackingStore;
    bool                     IsValid;

    SDepthStencilBlock() noexcept;
};

struct SBlendBlock : SBaseBlock
{
    ID3D11BlendState        *pBlendObject;
    D3D11_BLEND_DESC        BackingStore;
    bool                    IsValid;

    SBlendBlock() noexcept;
};

struct SRasterizerBlock : SBaseBlock
{
    ID3D11RasterizerState   *pRasterizerObject;
    D3D11_RASTERIZER_DESC   BackingStore;
    bool                    IsValid;

    SRasterizerBlock() noexcept;
};

struct SSamplerBlock : SBaseBlock
{
    ID3D11SamplerState      *pD3DObject;
    struct
    {
        D3D11_SAMPLER_DESC  SamplerDesc;
        // Sampler "TEXTURE" assignments can write directly into this
        SShaderResource     *pTexture;
    } BackingStore;

    SSamplerBlock() noexcept;
};

struct SInterface
{
    SClassInstanceGlobalVariable* pClassInstance;

    SInterface() noexcept :
        pClassInstance(nullptr)
    {
    }
};

struct SShaderResource
{
    ID3D11ShaderResourceView *pShaderResource;

    SShaderResource() noexcept :
        pShaderResource(nullptr)
    {
    }
};

struct SUnorderedAccessView
{
    ID3D11UnorderedAccessView *pUnorderedAccessView;

    SUnorderedAccessView() noexcept :
        pUnorderedAccessView(nullptr)
    {
    }
};

struct SRenderTargetView
{
    ID3D11RenderTargetView *pRenderTargetView;

    SRenderTargetView() noexcept :
        pRenderTargetView(nullptr)
    {
    }
};

struct SDepthStencilView
{
    ID3D11DepthStencilView *pDepthStencilView;

    SDepthStencilView() noexcept :
        pDepthStencilView(nullptr)
    {
    }
};


template<class T, class D3DTYPE> struct SShaderDependency
{
    uint32_t    StartIndex;
    uint32_t    Count;

    T       *ppFXPointers;              // Array of ptrs to FX objects (CBs, SShaderResources, etc)
    D3DTYPE *ppD3DObjects;              // Array of ptrs to matching D3D objects

    SShaderDependency() noexcept :
        StartIndex(0),
        Count(0),
        ppFXPointers(nullptr),
        ppD3DObjects(nullptr)
    {
    }
};

typedef SShaderDependency<SConstantBuffer*, ID3D11Buffer*> SShaderCBDependency;
typedef SShaderDependency<SSamplerBlock*, ID3D11SamplerState*> SShaderSamplerDependency;
typedef SShaderDependency<SShaderResource*, ID3D11ShaderResourceView*> SShaderResourceDependency;
typedef SShaderDependency<SUnorderedAccessView*, ID3D11UnorderedAccessView*> SUnorderedAccessViewDependency;
typedef SShaderDependency<SInterface*, ID3D11ClassInstance*> SInterfaceDependency;

// Shader VTables are used to eliminate branching in ApplyShaderBlock.
// The effect owns one D3DShaderVTables for each shader stage
struct SD3DShaderVTable
{
    void ( __stdcall ID3D11DeviceContext::*pSetShader)(ID3D11DeviceChild* pShader, ID3D11ClassInstance*const* ppClassInstances, uint32_t NumClassInstances);
    void ( __stdcall ID3D11DeviceContext::*pSetConstantBuffers)(uint32_t StartConstantSlot, uint32_t NumBuffers, ID3D11Buffer *const *pBuffers);
    void ( __stdcall ID3D11DeviceContext::*pSetSamplers)(uint32_t Offset, uint32_t NumSamplers, ID3D11SamplerState*const* pSamplers);
    void ( __stdcall ID3D11DeviceContext::*pSetShaderResources)(uint32_t Offset, uint32_t NumResources, ID3D11ShaderResourceView *const *pResources);
    HRESULT ( __stdcall ID3D11Device::*pCreateShader)(const void *pShaderBlob, size_t ShaderBlobSize, ID3D11ClassLinkage* pClassLinkage, ID3D11DeviceChild **ppShader);
};


struct SShaderBlock
{
    enum ESigType
    {
        ST_Input,
        ST_Output,
        ST_PatchConstant,
    };

    struct SInterfaceParameter
    {
        char                        *pName;
        uint32_t                        Index;
    };

    // this data is classified as reflection-only and will all be discarded at runtime
    struct SReflectionData
    {
        uint8_t                     *pBytecode;
        uint32_t                    BytecodeLength;
        char                        *pStreamOutDecls[4];        // set with ConstructGSWithSO
        uint32_t                    RasterizedStream;           // set with ConstructGSWithSO
        BOOL                        IsNullGS;
        ID3D11ShaderReflection      *pReflection;
        uint32_t                    InterfaceParameterCount;    // set with BindInterfaces (used for function interface parameters)
        SInterfaceParameter         *pInterfaceParameters;      // set with BindInterfaces (used for function interface parameters)
    };

    bool                            IsValid;
    SD3DShaderVTable                *pVT;                

    // This value is nullptr if the shader is nullptr or was never initialized
    SReflectionData                 *pReflectionData;

    ID3D11DeviceChild               *pD3DObject;

    uint32_t                        CBDepCount;
    SShaderCBDependency             *pCBDeps;

    uint32_t                        SampDepCount;
    SShaderSamplerDependency        *pSampDeps;

    uint32_t                        InterfaceDepCount;
    SInterfaceDependency            *pInterfaceDeps;

    uint32_t                        ResourceDepCount;
    SShaderResourceDependency       *pResourceDeps;

    uint32_t                        UAVDepCount;
    SUnorderedAccessViewDependency  *pUAVDeps;

    uint32_t                        TBufferDepCount;
    SConstantBuffer                 **ppTbufDeps;

    ID3DBlob                        *pInputSignatureBlob;   // The input signature is separated from the bytecode because it 
                                                            // is always available, even after Optimize() has been called.

    SShaderBlock(SD3DShaderVTable *pVirtualTable = nullptr) noexcept;

    EObjectType GetShaderType();

    HRESULT OnDeviceBind();

    // Public API helpers
    HRESULT ComputeStateBlockMask(_Inout_ D3DX11_STATE_BLOCK_MASK *pStateBlockMask);

    HRESULT GetShaderDesc(_Out_ D3DX11_EFFECT_SHADER_DESC *pDesc, _In_ bool IsInline);

    HRESULT GetVertexShader(_Outptr_ ID3D11VertexShader **ppVS);
    HRESULT GetGeometryShader(_Outptr_ ID3D11GeometryShader **ppGS);
    HRESULT GetPixelShader(_Outptr_ ID3D11PixelShader **ppPS);
    HRESULT GetHullShader(_Outptr_ ID3D11HullShader **ppHS);
    HRESULT GetDomainShader(_Outptr_ ID3D11DomainShader **ppDS);
    HRESULT GetComputeShader(_Outptr_ ID3D11ComputeShader **ppCS);

    HRESULT GetSignatureElementDesc(_In_ ESigType SigType, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc);
};



struct SString
{
    char *pString;

    SString() noexcept :
        pString(nullptr)
    {
    }
};



//////////////////////////////////////////////////////////////////////////
// Global Variable & Annotation structure/interface definitions
//////////////////////////////////////////////////////////////////////////

//
// This is a general structure that can describe
// annotations, variables, and structure members
//
struct SVariable
{
    // For annotations/variables/variable members:
    // 1) If numeric, pointer to data (for variables: points into backing store,
    //      for annotations, points into reflection heap)
    // OR
    // 2) If object, pointer to the block. If object array, subsequent array elements are found in
    //      contiguous blocks; the Nth block is found by ((<SpecificBlockType> *) pBlock) + N
    //      (this is because variables that are arrays of objects have their blocks allocated contiguously)
    //
    // For structure members:
    //    Offset of this member (in bytes) from parent structure (structure members must be numeric/struct)
    UDataPointer            Data;
    union
    {
        uint32_t            MemberDataOffsetPlus4;  // 4 added so that 0 == nullptr can represent "unused"
        SMemberDataPointer  *pMemberData;
    };

    SType                   *pType;
    char                    *pName;
    char                    *pSemantic;
    uint32_t                ExplicitBindPoint;

    SVariable() noexcept :
        Data{},
        pMemberData(nullptr),
        pType(nullptr),
        pName(nullptr),
        pSemantic(nullptr),
        ExplicitBindPoint(uint32_t(-1))
    {
    }
};

// Template definitions for all of the various ID3DX11EffectVariable specializations
#include "EffectVariable.inl"


////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectShaderVariable (SAnonymousShader implementation)
////////////////////////////////////////////////////////////////////////////////

struct SAnonymousShader : public TUncastableVariable<ID3DX11EffectShaderVariable>, public ID3DX11EffectType
{
    SShaderBlock    *pShaderBlock;

    SAnonymousShader(_In_opt_ SShaderBlock *pBlock = nullptr) noexcept;

    // ID3DX11EffectShaderVariable interface
    STDMETHOD_(bool, IsValid)() override;
    STDMETHOD_(ID3DX11EffectType*, GetType)() override;
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByName)(_In_z_ LPCSTR Name) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberBySemantic)(_In_z_ LPCSTR Semantic) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetElement)(_In_ uint32_t Index) override;

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetParentConstantBuffer)() override;

    // other casts are handled by TUncastableVariable
    STDMETHOD_(ID3DX11EffectShaderVariable*, AsShader)() override;

    STDMETHOD(SetRawValue)(_In_reads_bytes_(Count) const void *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetRawValue)(_Out_writes_bytes_(Count) void *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(GetShaderDesc)(_In_ uint32_t ShaderIndex, _Out_ D3DX11_EFFECT_SHADER_DESC *pDesc) override;

    STDMETHOD(GetVertexShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11VertexShader **ppVS) override;
    STDMETHOD(GetGeometryShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11GeometryShader **ppGS) override;
    STDMETHOD(GetPixelShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11PixelShader **ppPS) override;
    STDMETHOD(GetHullShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11HullShader **ppHS) override;
    STDMETHOD(GetDomainShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11DomainShader **ppDS) override;
    STDMETHOD(GetComputeShader)(_In_ uint32_t ShaderIndex, _Outptr_ ID3D11ComputeShader **ppCS) override;

    STDMETHOD(GetInputSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) override;
    STDMETHOD(GetOutputSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) override;
    STDMETHOD(GetPatchConstantSignatureElementDesc)(_In_ uint32_t ShaderIndex, _In_ uint32_t Element, _Out_ D3D11_SIGNATURE_PARAMETER_DESC *pDesc) override;

    // ID3DX11EffectType interface
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByName)(_In_z_ LPCSTR Name) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeBySemantic)(_In_z_ LPCSTR Semantic) override;

    STDMETHOD_(LPCSTR, GetMemberName)(_In_ uint32_t Index) override;
    STDMETHOD_(LPCSTR, GetMemberSemantic)(_In_ uint32_t Index) override;

    IUNKNOWN_IMP(SAnonymousShader, ID3DX11EffectShaderVariable, ID3DX11EffectVariable);
};

////////////////////////////////////////////////////////////////////////////////
// ID3DX11EffectConstantBuffer (SConstantBuffer implementation)
////////////////////////////////////////////////////////////////////////////////

struct SConstantBuffer : public TUncastableVariable<ID3DX11EffectConstantBuffer>, public ID3DX11EffectType
{
    ID3D11Buffer            *pD3DObject;
    SShaderResource         TBuffer;            // nullptr iff IsTbuffer == false

    uint8_t                 *pBackingStore;
    uint32_t                Size;               // in bytes

    char                    *pName;

    uint32_t                AnnotationCount;
    SAnnotation             *pAnnotations;

    uint32_t                VariableCount;      // # of variables contained in this cbuffer
    SGlobalVariable         *pVariables;        // array of size [VariableCount], points into effect's contiguous variable list
    uint32_t                ExplicitBindPoint;  // Used when a CB has been explicitly bound (register(bXX)). -1 if not

    bool                    IsDirty:1;          // Set when any member is updated; cleared on CB apply    
    bool                    IsTBuffer:1;        // true iff TBuffer.pShaderResource != nullptr
    bool                    IsUserManaged:1;    // Set if you don't want effects to update this buffer
    bool                    IsEffectOptimized:1;// Set if the effect has been optimized
    bool                    IsUsedByExpression:1;// Set if used by any expressions
    bool                    IsUserPacked:1;     // Set if the elements have user-specified offsets
    bool                    IsSingle:1;         // Set to true if you want to share this CB with cloned Effects
    bool                    IsNonUpdatable:1;   // Set to true if you want to share this CB with cloned Effects

    union
    {
        // These are used to store the original ID3D11Buffer* for use in UndoSetConstantBuffer
        uint32_t            MemberDataOffsetPlus4;  // 4 added so that 0 == nullptr can represent "unused"
        SMemberDataPointer  *pMemberData;
    };

    CEffect                 *pEffect;

    SConstantBuffer() noexcept :
        pD3DObject(nullptr),
        TBuffer{},
        pBackingStore(nullptr),
        Size(0),
        pName(nullptr),
        AnnotationCount(0),
        pAnnotations(nullptr),
        VariableCount(0),
        pVariables(nullptr),
        ExplicitBindPoint(uint32_t(-1)),
        IsDirty(false),
        IsTBuffer(false),
        IsUserManaged(false),
        IsEffectOptimized(false),
        IsUsedByExpression(false),
        IsUserPacked(false),
        IsSingle(false),
        IsNonUpdatable(false),
        pMemberData(nullptr),
        pEffect(nullptr)
    {
    }

    bool ClonedSingle() const;

    // ID3DX11EffectConstantBuffer interface
    STDMETHOD_(bool, IsValid)() override;
    STDMETHOD_(ID3DX11EffectType*, GetType)() override;
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_VARIABLE_DESC *pDesc) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetAnnotationByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberByName)(_In_z_ LPCSTR Name) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetMemberBySemantic)(_In_z_ LPCSTR Semantic) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetElement)(_In_ uint32_t Index) override;

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetParentConstantBuffer)() override;

    // other casts are handled by TUncastableVariable
    STDMETHOD_(ID3DX11EffectConstantBuffer*, AsConstantBuffer)() override;

    STDMETHOD(SetRawValue)(_In_reads_bytes_(Count) const void *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;
    STDMETHOD(GetRawValue)(_Out_writes_bytes_(Count) void *pData, _In_ uint32_t Offset, _In_ uint32_t Count) override;

    STDMETHOD(SetConstantBuffer)(_In_ ID3D11Buffer *pConstantBuffer) override;
    STDMETHOD(GetConstantBuffer)(_Outptr_ ID3D11Buffer **ppConstantBuffer) override;
    STDMETHOD(UndoSetConstantBuffer)() override;

    STDMETHOD(SetTextureBuffer)(_In_ ID3D11ShaderResourceView *pTextureBuffer) override;
    STDMETHOD(GetTextureBuffer)(_Outptr_ ID3D11ShaderResourceView **ppTextureBuffer) override;
    STDMETHOD(UndoSetTextureBuffer)() override;

    // ID3DX11EffectType interface
    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_TYPE_DESC *pDesc) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeByName)(_In_z_ LPCSTR Name) override;
    STDMETHOD_(ID3DX11EffectType*, GetMemberTypeBySemantic)(_In_z_ LPCSTR Semantic) override;

    STDMETHOD_(LPCSTR, GetMemberName)(_In_ uint32_t Index) override;
    STDMETHOD_(LPCSTR, GetMemberSemantic)(_In_ uint32_t Index) override;

    IUNKNOWN_IMP(SConstantBuffer, ID3DX11EffectConstantBuffer, ID3DX11EffectVariable);
};


//////////////////////////////////////////////////////////////////////////
// Assignments
//////////////////////////////////////////////////////////////////////////

enum ERuntimeAssignmentType
{
    ERAT_Invalid,
    // [Destination] refers to the destination location, which is always the backing store of the pass/state block. 
    // [Source] refers to the current source of data, always coming from either a constant buffer's 
    //  backing store (for numeric assignments), an object variable's block array, or an anonymous (unowned) block

    // Numeric variables:
    ERAT_Constant,                  // Source is unused.
                                    // No dependencies; this assignment can be safely removed after load.
    ERAT_NumericVariable,           // Source points to the CB's backing store where the value lives.
                                    // 1 dependency: the variable itself.
    ERAT_NumericConstIndex,         // Source points to the CB's backing store where the value lives, offset by N.
                                    // 1 dependency: the variable array being indexed.
    ERAT_NumericVariableIndex,      // Source points to the last used element of the variable in the CB's backing store.
                                    // 2 dependencies: the index variable followed by the array variable.

    // Object variables:
    ERAT_ObjectInlineShader,        // An anonymous, immutable shader block pointer is copied to the destination immediately.
                                    // No dependencies; this assignment can be safely removed after load.
    ERAT_ObjectVariable,            // A pointer to the block owned by the object variable is copied to the destination immediately.
                                    // No dependencies; this assignment can be safely removed after load.
    ERAT_ObjectConstIndex,          // A pointer to the Nth block owned by an object variable is copied to the destination immediately.
                                    // No dependencies; this assignment can be safely removed after load.
    ERAT_ObjectVariableIndex,       // Source points to the first block owned by an object variable array
                                    // (the offset from this, N, is taken from another variable).
                                    // 1 dependency: the variable being used to index the array.
};

struct SAssignment
{
    struct SDependency
    {
        SGlobalVariable *pVariable;

        SDependency() noexcept :
            pVariable(nullptr)
        {
        }
    };

    ELhsType                LhsType;            // PS, VS, DepthStencil etc.

    // The value of SAssignment.AssignmentType determines how the other fields behave
    // (DependencyCount, pDependencies, Destination, and Source)
    ERuntimeAssignmentType  AssignmentType;      

    Timer                   LastRecomputedTime;

    // see comments in ERuntimeAssignmentType for how dependencies and data pointers are handled
    uint32_t                DependencyCount;
    SDependency             *pDependencies;

    UDataPointer            Destination;        // This value never changes after load, and always refers to the backing store
    UDataPointer            Source;             // This value, on the other hand, can change if variable- or expression- driven

    uint32_t                DataSize : 16;      // Size of the data element to be copied in bytes (if numeric) or
                                                // stride of the block type (if object)
    uint32_t                MaxElements : 16;   // Max allowable index (needed because we don't store object arrays as dependencies,
                                                // and therefore have no way of getting their Element count)

    bool IsObjectAssignment()                   // True for Shader and RObject assignments (the type that appear in pass blocks)
    {
        return IsObjectAssignmentHelper(LhsType);
    }

    SAssignment() noexcept :
        LhsType(ELHS_Invalid),
        AssignmentType(ERAT_Invalid),
        LastRecomputedTime(0),
        DependencyCount(0),
        pDependencies(nullptr),
        Destination{0},
        Source{0},
        DataSize(0),
        MaxElements(0)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
// Private effect heaps
//////////////////////////////////////////////////////////////////////////

// Used to efficiently reallocate data
// 1) For every piece of data that needs reallocation, move it to its new location
// and add an entry into the table
// 2) For everyone that references one of these data blocks, do a quick table lookup
// to find the old pointer and then replace it with the new one
struct SPointerMapping
{
    void *pOld;
    void *pNew;

    static bool AreMappingsEqual(const SPointerMapping &pMap1, const SPointerMapping &pMap2)
    {
        return (pMap1.pOld == pMap2.pOld);
    }

    uint32_t Hash()
    {
        // hash the pointer itself 
        // (using the pointer as a hash would be very bad)
        return ComputeHash((uint8_t*)&pOld, sizeof(pOld));
    }
};

typedef CEffectHashTableWithPrivateHeap<SPointerMapping, SPointerMapping::AreMappingsEqual> CPointerMappingTable;

// Assist adding data to a block of memory
class CEffectHeap
{
protected:
    uint8_t    *m_pData;
    uint32_t    m_dwBufferSize;
    uint32_t    m_dwSize;

    template <bool bCopyData>
    HRESULT AddDataInternal(_In_reads_bytes_(dwSize) const void *pData, _In_ uint32_t dwSize, _Outptr_ void **ppPointer);

public:
    HRESULT ReserveMemory(uint32_t dwSize);
    uint32_t GetSize();
    uint8_t* GetDataStart() { return m_pData; }

    // AddData and AddString append existing data to the buffer - they change m_dwSize. Users are 
    //   not expected to modify the data pointed to by the return pointer
    HRESULT AddString(_In_z_ const char *pString, _Outptr_result_z_ char **ppPointer);
    HRESULT AddData(_In_reads_(dwSize) const void *pData, _In_ uint32_t dwSize, _Outptr_ void **ppPointer);

    // Allocate behaves like a standard new - it will allocate memory, move m_dwSize. The caller is 
    //   expected to use the returned pointer
    void* Allocate(uint32_t dwSize);

    // Move data from the general heap and optional free memory
    HRESULT MoveData(_Inout_updates_bytes_(size) void **ppData, _In_ uint32_t size);
    HRESULT MoveString(_Inout_updates_z_(1)  char **ppStringData);
    HRESULT MoveInterfaceParameters(_In_ uint32_t InterfaceCount, _Inout_updates_(1) SShaderBlock::SInterfaceParameter **ppInterfaces);
    HRESULT MoveEmptyDataBlock(_Inout_updates_(1) void **ppData, _In_ uint32_t size);

    bool IsInHeap(_In_ void *pData) const
    {
        return (pData >= m_pData && pData < (m_pData + m_dwBufferSize));
    }

    CEffectHeap() noexcept;
    ~CEffectHeap();
};

class CEffectReflection
{
public:
    // Single memory block support
    CEffectHeap m_Heap;
};


class CEffect : public ID3DX11Effect
{
    friend struct SBaseBlock;
    friend struct SPassBlock;
    friend class CEffectLoader;
    friend struct SConstantBuffer;
    friend struct TSamplerVariable<TGlobalVariable<ID3DX11EffectSamplerVariable>>;
    friend struct TSamplerVariable<TVariable<TMember<ID3DX11EffectSamplerVariable>>>;
    
protected:

    uint32_t                m_RefCount;
    uint32_t                m_Flags;

    // Private heap - all pointers should point into here
    CEffectHeap             m_Heap;

    // Reflection object
    CEffectReflection       *m_pReflection;

    // global variables in the effect (aka parameters)
    uint32_t                m_VariableCount;
    SGlobalVariable         *m_pVariables;

    // anonymous shader variables (one for every inline shader assignment)
    uint32_t                m_AnonymousShaderCount;
    SAnonymousShader        *m_pAnonymousShaders;

    // techniques within this effect (the actual data is located in each group)
    uint32_t                m_TechniqueCount;

    // groups within this effect
    uint32_t                m_GroupCount;
    SGroup                  *m_pGroups;
    SGroup                  *m_pNullGroup;

    uint32_t                m_ShaderBlockCount;
    SShaderBlock            *m_pShaderBlocks;

    uint32_t                m_DepthStencilBlockCount;
    SDepthStencilBlock      *m_pDepthStencilBlocks;

    uint32_t                m_BlendBlockCount;
    SBlendBlock             *m_pBlendBlocks;

    uint32_t                m_RasterizerBlockCount;
    SRasterizerBlock        *m_pRasterizerBlocks;

    uint32_t                m_SamplerBlockCount;
    SSamplerBlock           *m_pSamplerBlocks;

    uint32_t                m_MemberDataCount;
    SMemberDataPointer      *m_pMemberDataBlocks;

    uint32_t                m_InterfaceCount;
    SInterface              *m_pInterfaces;

    uint32_t                m_CBCount;
    SConstantBuffer         *m_pCBs;

    uint32_t                m_StringCount;
    SString                 *m_pStrings;

    uint32_t                m_ShaderResourceCount;
    SShaderResource         *m_pShaderResources;

    uint32_t                m_UnorderedAccessViewCount;
    SUnorderedAccessView    *m_pUnorderedAccessViews;

    uint32_t                m_RenderTargetViewCount;
    SRenderTargetView       *m_pRenderTargetViews;

    uint32_t                m_DepthStencilViewCount;
    SDepthStencilView       *m_pDepthStencilViews; 

    Timer                   m_LocalTimer;
    
    // temporary index variable for assignment evaluation
    uint32_t                m_FXLIndex;

    ID3D11Device            *m_pDevice;
    ID3D11DeviceContext     *m_pContext;
    ID3D11ClassLinkage      *m_pClassLinkage;

    // Master lists of reflection interfaces
    CEffectVectorOwner<SSingleElementType> m_pTypeInterfaces;
    CEffectVectorOwner<SMember>            m_pMemberInterfaces;

    //////////////////////////////////////////////////////////////////////////    
    // String & Type pooling

    typedef SType *LPSRUNTIMETYPE;
    static bool AreTypesEqual(const LPSRUNTIMETYPE &pType1, const LPSRUNTIMETYPE &pType2) { return (pType1->IsEqual(pType2)); }
    static bool AreStringsEqual(const LPCSTR &pStr1, const LPCSTR &pStr2) { return strcmp(pStr1, pStr2) == 0; }

    typedef CEffectHashTableWithPrivateHeap<SType *, AreTypesEqual> CTypeHashTable;
    typedef CEffectHashTableWithPrivateHeap<LPCSTR, AreStringsEqual> CStringHashTable;

    // These are used to pool types & type-related strings
    // until Optimize() is called
    CTypeHashTable          *m_pTypePool;
    CStringHashTable        *m_pStringPool;
    CDataBlockStore         *m_pPooledHeap;
    // After Optimize() is called, the type/string pools should be deleted and all
    // remaining data should be migrated into the optimized type heap
    CEffectHeap             *m_pOptimizedTypeHeap;

    // Pools a string or type and modifies the pointer
    void AddStringToPool(const char **ppString);
    void AddTypeToPool(SType **ppType);

    HRESULT OptimizeTypes(_Inout_ CPointerMappingTable *pMappingTable, _In_ bool Cloning = false);


    //////////////////////////////////////////////////////////////////////////    
    // Runtime (performance critical)
    
    void ApplyShaderBlock(_In_ SShaderBlock *pBlock);
    bool ApplyRenderStateBlock(_In_ SBaseBlock *pBlock);
    bool ApplySamplerBlock(_In_ SSamplerBlock *pBlock);
    void ApplyPassBlock(_Inout_ SPassBlock *pBlock);
    bool EvaluateAssignment(_Inout_  SAssignment *pAssignment);
    bool ValidateShaderBlock(_Inout_ SShaderBlock* pBlock );
    bool ValidatePassBlock(_Inout_ SPassBlock* pBlock );
    
    //////////////////////////////////////////////////////////////////////////    
    // Non-runtime functions (not performance critical)    

    SGlobalVariable *FindLocalVariableByName(_In_z_ LPCSTR pVarName);      // Looks in the current effect only
    SGlobalVariable *FindVariableByName(_In_z_ LPCSTR pVarName);
    SVariable *FindVariableByNameWithParsing(_In_z_ LPCSTR pVarName);
    SConstantBuffer *FindCB(_In_z_ LPCSTR pName);
    void ReplaceCBReference(_In_ SConstantBuffer *pOldBufferBlock, _In_ ID3D11Buffer *pNewBuffer); // Used by user-managed CBs
    void ReplaceSamplerReference(_In_ SSamplerBlock *pOldSamplerBlock, _In_ ID3D11SamplerState *pNewSampler);
    void AddRefAllForCloning( _In_ CEffect* pEffectSource );
    HRESULT CopyMemberInterfaces( _In_ CEffect* pEffectSource );
    HRESULT CopyStringPool( _In_ CEffect* pEffectSource, _Inout_ CPointerMappingTable& mappingTable );
    HRESULT CopyTypePool( _In_ CEffect* pEffectSource, _Inout_ CPointerMappingTable& mappingTableTypes, _Inout_ CPointerMappingTable& mappingTableStrings );
    HRESULT CopyOptimizedTypePool( _In_ CEffect* pEffectSource, _Inout_ CPointerMappingTable& mappingTableTypes );
    HRESULT RecreateCBs();
    HRESULT FixupMemberInterface( _Inout_ SMember* pMember, _In_ CEffect* pEffectSource, _Inout_ CPointerMappingTable& mappingTableStrings );

    void ValidateIndex(_In_ uint32_t Elements);

    void IncrementTimer();    
    void HandleLocalTimerRollover();

    friend struct SConstantBuffer;

public:
    CEffect( uint32_t Flags = 0 ) noexcept;
    virtual ~CEffect();
    void ReleaseShaderRefection();

    // Initialize must be called after the effect is created
    HRESULT LoadEffect(_In_reads_bytes_(cbEffectBuffer) const void *pEffectBuffer, _In_ uint32_t cbEffectBuffer);

    // Once the effect is fully loaded, call BindToDevice to attach it to a device
    HRESULT BindToDevice(_In_ ID3D11Device *pDevice, _In_z_ LPCSTR srcName );

    Timer GetCurrentTime() const { return m_LocalTimer; }
    
    bool IsReflectionData(void *pData) const { return m_pReflection->m_Heap.IsInHeap(pData); }
    bool IsRuntimeData(void *pData) const { return m_Heap.IsInHeap(pData); }

    //////////////////////////////////////////////////////////////////////////    
    // Public interface

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, _COM_Outptr_ LPVOID *ppv) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // ID3DX11Effect
    STDMETHOD_(bool, IsValid)() override { return true; }

    STDMETHOD(GetDevice)(_Outptr_ ID3D11Device** ppDevice) override;    

    STDMETHOD(GetDesc)(_Out_ D3DX11_EFFECT_DESC *pDesc) override;

    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetConstantBufferByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectConstantBuffer*, GetConstantBufferByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD_(ID3DX11EffectVariable*, GetVariableByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetVariableByName)(_In_z_ LPCSTR Name) override;
    STDMETHOD_(ID3DX11EffectVariable*, GetVariableBySemantic)(_In_z_ LPCSTR Semantic) override;

    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectTechnique*, GetTechniqueByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD_(ID3DX11EffectGroup*, GetGroupByIndex)(_In_ uint32_t Index) override;
    STDMETHOD_(ID3DX11EffectGroup*, GetGroupByName)(_In_z_ LPCSTR Name) override;

    STDMETHOD_(ID3D11ClassLinkage*, GetClassLinkage)() override;

    STDMETHOD(CloneEffect)(_In_ uint32_t Flags, _Outptr_ ID3DX11Effect** ppClonedEffect) override;
    STDMETHOD(Optimize)() override;
    STDMETHOD_(bool, IsOptimized)() override;

    //////////////////////////////////////////////////////////////////////////    
    // New reflection helpers

    ID3DX11EffectType * CreatePooledSingleElementTypeInterface(_In_ SType *pType);
    ID3DX11EffectVariable * CreatePooledVariableMemberInterface(_In_ TTopLevelVariable<ID3DX11EffectVariable> *pTopLevelEntity,
                                                                _In_ const SVariable *pMember,
                                                                _In_ const UDataPointer Data, _In_ bool IsSingleElement, _In_ uint32_t Index);

};

}

#pragma warning(pop)

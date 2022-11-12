//--------------------------------------------------------------------------------------
// File: EffectBinaryFormat.h
//
// Direct3D11 Effects Binary Format
// This is the binary file interface shared between the Effects 
// compiler and runtime.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

namespace D3DX11Effects
{

//////////////////////////////////////////////////////////////////////////
// Version Control
//////////////////////////////////////////////////////////////////////////

#define D3DX11_FXL_VERSION(_Major,_Minor) (('F' << 24) | ('X' << 16) | ((_Major) << 8) | (_Minor))

struct EVersionTag
{
    const char* m_pName;
    DWORD       m_Version;
    uint32_t    m_Tag;
};

// versions must be listed in ascending order
static const EVersionTag g_EffectVersions[] = 
{
    { "fx_4_0", D3DX11_FXL_VERSION(4,0),   0xFEFF1001 },
    { "fx_4_1", D3DX11_FXL_VERSION(4,1),   0xFEFF1011 },
    { "fx_5_0", D3DX11_FXL_VERSION(5,0),   0xFEFF2001 },
};


//////////////////////////////////////////////////////////////////////////
// Reflection & Type structures
//////////////////////////////////////////////////////////////////////////

// Enumeration of the possible left-hand side values of an assignment,
// divided up categorically by the type of block they may appear in
enum ELhsType : int
{
    ELHS_Invalid,

    // Pass block assignment types

    ELHS_PixelShaderBlock,          // SBlock *pValue points to the block to apply
    ELHS_VertexShaderBlock,
    ELHS_GeometryShaderBlock,
    ELHS_RenderTargetView,
    ELHS_DepthStencilView,

    ELHS_RasterizerBlock,
    ELHS_DepthStencilBlock,
    ELHS_BlendBlock,

    ELHS_GenerateMips,              // This is really a call to D3D::GenerateMips

    // Various SAssignment.Value.*

    ELHS_DS_StencilRef,             // SAssignment.Value.pdValue
    ELHS_B_BlendFactor,             // D3D11_BLEND_CONFIG.BlendFactor, points to a float4
    ELHS_B_SampleMask,              // D3D11_BLEND_CONFIG.SampleMask

    ELHS_GeometryShaderSO,          // When setting SO assignments, GeometryShaderSO precedes the actual GeometryShader assn

    ELHS_ComputeShaderBlock,   
    ELHS_HullShaderBlock,
    ELHS_DomainShaderBlock,

    // Rasterizer

    ELHS_FillMode = 0x20000,
    ELHS_CullMode,
    ELHS_FrontCC,
    ELHS_DepthBias,
    ELHS_DepthBiasClamp,
    ELHS_SlopeScaledDepthBias,
    ELHS_DepthClipEnable,
    ELHS_ScissorEnable,
    ELHS_MultisampleEnable,
    ELHS_AntialiasedLineEnable,

    // Sampler

    ELHS_Filter = 0x30000,
    ELHS_AddressU,
    ELHS_AddressV,
    ELHS_AddressW,
    ELHS_MipLODBias,
    ELHS_MaxAnisotropy,
    ELHS_ComparisonFunc,
    ELHS_BorderColor,
    ELHS_MinLOD,
    ELHS_MaxLOD,
    ELHS_Texture,

    // DepthStencil

    ELHS_DepthEnable = 0x40000,
    ELHS_DepthWriteMask,
    ELHS_DepthFunc,
    ELHS_StencilEnable,
    ELHS_StencilReadMask,
    ELHS_StencilWriteMask,
    ELHS_FrontFaceStencilFailOp,
    ELHS_FrontFaceStencilDepthFailOp,
    ELHS_FrontFaceStencilPassOp,
    ELHS_FrontFaceStencilFunc,
    ELHS_BackFaceStencilFailOp,
    ELHS_BackFaceStencilDepthFailOp,
    ELHS_BackFaceStencilPassOp,
    ELHS_BackFaceStencilFunc,

    // BlendState

    ELHS_AlphaToCoverage = 0x50000,
    ELHS_BlendEnable,
    ELHS_SrcBlend,
    ELHS_DestBlend,
    ELHS_BlendOp,
    ELHS_SrcBlendAlpha,
    ELHS_DestBlendAlpha,
    ELHS_BlendOpAlpha,
    ELHS_RenderTargetWriteMask,
};

enum EBlockType
{
    EBT_Invalid,
    EBT_DepthStencil,
    EBT_Blend,
    EBT_Rasterizer,
    EBT_Sampler,
    EBT_Pass
};

enum EVarType
{
    EVT_Invalid,
    EVT_Numeric,
    EVT_Object,
    EVT_Struct,
    EVT_Interface,
    EVT_Count,
};

enum EScalarType
{
    EST_Invalid,
    EST_Float,
    EST_Int,
    EST_UInt,
    EST_Bool,
    EST_Count
};

enum ENumericLayout
{
    ENL_Invalid,
    ENL_Scalar,
    ENL_Vector,
    ENL_Matrix,
    ENL_Count
};

enum EObjectType
{
    EOT_Invalid,
    EOT_String,
    EOT_Blend,
    EOT_DepthStencil,
    EOT_Rasterizer,
    EOT_PixelShader,
    EOT_VertexShader,
    EOT_GeometryShader,              // Regular geometry shader
    EOT_GeometryShaderSO,            // Geometry shader with a attached StreamOut decl
    EOT_Texture,
    EOT_Texture1D,
    EOT_Texture1DArray,
    EOT_Texture2D,
    EOT_Texture2DArray,
    EOT_Texture2DMS,
    EOT_Texture2DMSArray,
    EOT_Texture3D,
    EOT_TextureCube,
    EOT_ConstantBuffer,
    EOT_RenderTargetView,
    EOT_DepthStencilView,
    EOT_Sampler,
    EOT_Buffer,
    EOT_TextureCubeArray,
    EOT_Count,
    EOT_PixelShader5,
    EOT_VertexShader5,
    EOT_GeometryShader5,
    EOT_ComputeShader5,
    EOT_HullShader5,
    EOT_DomainShader5,
    EOT_RWTexture1D,
    EOT_RWTexture1DArray,
    EOT_RWTexture2D,
    EOT_RWTexture2DArray,
    EOT_RWTexture3D,
    EOT_RWBuffer,
    EOT_ByteAddressBuffer,
    EOT_RWByteAddressBuffer,
    EOT_StructuredBuffer,
    EOT_RWStructuredBuffer,
    EOT_RWStructuredBufferAlloc,
    EOT_RWStructuredBufferConsume,
    EOT_AppendStructuredBuffer,
    EOT_ConsumeStructuredBuffer,
};

inline bool IsObjectTypeHelper(EVarType InVarType,
                                     EObjectType InObjType,
                                     EObjectType TargetObjType)
{
    return (InVarType == EVT_Object) && (InObjType == TargetObjType);
}

inline bool IsSamplerHelper(EVarType InVarType,
                                  EObjectType InObjType)
{
    return (InVarType == EVT_Object) && (InObjType == EOT_Sampler);
}

inline bool IsStateBlockObjectHelper(EVarType InVarType,
                                           EObjectType InObjType)
{
    return (InVarType == EVT_Object) && ((InObjType == EOT_Blend) || (InObjType == EOT_DepthStencil) || (InObjType == EOT_Rasterizer) || IsSamplerHelper(InVarType, InObjType));
}

inline bool IsShaderHelper(EVarType InVarType,
                                 EObjectType InObjType)
{
    return (InVarType == EVT_Object) && ((InObjType == EOT_VertexShader) ||
                                         (InObjType == EOT_VertexShader5) ||
                                         (InObjType == EOT_HullShader5) ||
                                         (InObjType == EOT_DomainShader5) ||
                                         (InObjType == EOT_ComputeShader5) ||
                                         (InObjType == EOT_GeometryShader) ||
                                         (InObjType == EOT_GeometryShaderSO) ||
                                         (InObjType == EOT_GeometryShader5) ||
                                         (InObjType == EOT_PixelShader) ||
                                         (InObjType == EOT_PixelShader5));
}

inline bool IsShader5Helper(EVarType InVarType,
                                  EObjectType InObjType)
{
    return (InVarType == EVT_Object) && ((InObjType == EOT_VertexShader5) ||
                                         (InObjType == EOT_HullShader5) ||
                                         (InObjType == EOT_DomainShader5) ||
                                         (InObjType == EOT_ComputeShader5) ||
                                         (InObjType == EOT_GeometryShader5) ||
                                         (InObjType == EOT_PixelShader5));
}

inline bool IsInterfaceHelper(EVarType InVarType, EObjectType InObjType)
{
    UNREFERENCED_PARAMETER(InObjType);
    return (InVarType == EVT_Interface);
}

inline bool IsShaderResourceHelper(EVarType InVarType,
                                         EObjectType InObjType)
{
    return (InVarType == EVT_Object) && ((InObjType == EOT_Texture) ||
                                         (InObjType == EOT_Texture1D) || 
                                         (InObjType == EOT_Texture1DArray) ||
                                         (InObjType == EOT_Texture2D) || 
                                         (InObjType == EOT_Texture2DArray) ||
                                         (InObjType == EOT_Texture2DMS) || 
                                         (InObjType == EOT_Texture2DMSArray) ||
                                         (InObjType == EOT_Texture3D) || 
                                         (InObjType == EOT_TextureCube) ||
                                         (InObjType == EOT_TextureCubeArray) || 
                                         (InObjType == EOT_Buffer) ||
                                         (InObjType == EOT_StructuredBuffer) ||
                                         (InObjType == EOT_ByteAddressBuffer));
}

inline bool IsUnorderedAccessViewHelper(EVarType InVarType,
                                              EObjectType InObjType)
{
    return (InVarType == EVT_Object) &&
        ((InObjType == EOT_RWTexture1D) ||
         (InObjType == EOT_RWTexture1DArray) ||
         (InObjType == EOT_RWTexture2D) ||
         (InObjType == EOT_RWTexture2DArray) ||
         (InObjType == EOT_RWTexture3D) ||
         (InObjType == EOT_RWBuffer) ||
         (InObjType == EOT_RWByteAddressBuffer) ||
         (InObjType == EOT_RWStructuredBuffer) ||
         (InObjType == EOT_RWStructuredBufferAlloc) ||
         (InObjType == EOT_RWStructuredBufferConsume) ||
         (InObjType == EOT_AppendStructuredBuffer) ||
         (InObjType == EOT_ConsumeStructuredBuffer));
}

inline bool IsRenderTargetViewHelper(EVarType InVarType,
                                           EObjectType InObjType)
{
    return (InVarType == EVT_Object) && (InObjType == EOT_RenderTargetView);
}

inline bool IsDepthStencilViewHelper(EVarType InVarType,
                                           EObjectType InObjType)
{
    return (InVarType == EVT_Object) && (InObjType == EOT_DepthStencilView);
}

inline bool IsObjectAssignmentHelper(ELhsType LhsType)
{
    switch(LhsType)
    {
    case ELHS_VertexShaderBlock:
    case ELHS_HullShaderBlock:
    case ELHS_DepthStencilView:
    case ELHS_GeometryShaderBlock:
    case ELHS_PixelShaderBlock:
    case ELHS_ComputeShaderBlock:
    case ELHS_DepthStencilBlock:
    case ELHS_RasterizerBlock:
    case ELHS_BlendBlock:
    case ELHS_Texture:
    case ELHS_RenderTargetView:
    case ELHS_DomainShaderBlock:
        return true;
    }
    return false;
}




// Effect file format structures /////////////////////////////////////////////
// File format:
//   File header (SBinaryHeader Header)
//   Unstructured data block (uint8_t[Header.cbUnstructured))
//   Structured data block
//     ConstantBuffer (SBinaryConstantBuffer CB) * Header.Effect.cCBs
//       uint32_t  NumAnnotations
//       Annotation data (SBinaryAnnotation) * (NumAnnotations) *this structure is variable sized
//       Variable data (SBinaryNumericVariable Var) * (CB.cVariables)
//         uint32_t  NumAnnotations
//         Annotation data (SBinaryAnnotation) * (NumAnnotations) *this structure is variable sized
//     Object variables (SBinaryObjectVariable Var) * (Header.cObjectVariables) *this structure is variable sized
//       uint32_t  NumAnnotations
//       Annotation data (SBinaryAnnotation) * (NumAnnotations) *this structure is variable sized
//     Interface variables (SBinaryInterfaceVariable Var) * (Header.cInterfaceVariables) *this structure is variable sized
//       uint32_t  NumAnnotations
//       Annotation data (SBinaryAnnotation) * (NumAnnotations) *this structure is variable sized
//     Groups (SBinaryGroup Group) * Header.cGroups
//       uint32_t  NumAnnotations
//       Annotation data (SBinaryAnnotation) * (NumAnnotations) *this structure is variable sized
//       Techniques (SBinaryTechnique Technique) * Group.cTechniques
//         uint32_t  NumAnnotations
//         Annotation data (SBinaryAnnotation) * (NumAnnotations) *this structure is variable sized
//         Pass (SBinaryPass Pass) * Technique.cPasses
//           uint32_t  NumAnnotations
//           Annotation data (SBinaryAnnotation) * (NumAnnotations) *this structure is variable sized
//           Pass assignments (SBinaryAssignment) * Pass.cAssignments

struct SBinaryHeader
{
    struct SVarCounts
    {
        uint32_t  cCBs;
        uint32_t  cNumericVariables;
        uint32_t  cObjectVariables;
    };

    uint32_t    Tag;    // should be equal to c_EffectFileTag
                        // this is used to identify ASCII vs Binary files

    SVarCounts  Effect;
    SVarCounts  Pool;
    
    uint32_t    cTechniques;
    uint32_t    cbUnstructured;

    uint32_t    cStrings;
    uint32_t    cShaderResources;

    uint32_t    cDepthStencilBlocks;
    uint32_t    cBlendStateBlocks;
    uint32_t    cRasterizerStateBlocks;
    uint32_t    cSamplers;
    uint32_t    cRenderTargetViews;
    uint32_t    cDepthStencilViews;

    uint32_t    cTotalShaders;
    uint32_t    cInlineShaders; // of the aforementioned shaders, the number that are defined inline within pass blocks

    inline bool RequiresPool() const
    {
        return (Pool.cCBs != 0) ||
               (Pool.cNumericVariables != 0) ||
               (Pool.cObjectVariables != 0);
    }
};

struct SBinaryHeader5 : public SBinaryHeader
{
    uint32_t  cGroups;
    uint32_t  cUnorderedAccessViews;
    uint32_t  cInterfaceVariables;
    uint32_t  cInterfaceVariableElements;
    uint32_t  cClassInstanceElements;
};

// Constant buffer definition
struct SBinaryConstantBuffer
{
    // private flags
    static const uint32_t   c_IsTBuffer = (1 << 0);
    static const uint32_t   c_IsSingle = (1 << 1);

    uint32_t                oName;                // Offset to constant buffer name
    uint32_t                Size;                 // Size, in bytes
    uint32_t                Flags;
    uint32_t                cVariables;           // # of variables inside this buffer
    uint32_t                ExplicitBindPoint;    // Defined if the effect file specifies a bind point using the register keyword
                                              // otherwise, -1
};

struct SBinaryAnnotation
{
    uint32_t  oName;                // Offset to variable name
    uint32_t  oType;                // Offset to type information (SBinaryType)

    // For numeric annotations:
    // uint32_t  oDefaultValue;     // Offset to default initializer value
    //
    // For string annotations:
    // uint32_t  oStringOffsets[Elements]; // Elements comes from the type data at oType
};

struct SBinaryNumericVariable
{
    uint32_t  oName;                // Offset to variable name
    uint32_t  oType;                // Offset to type information (SBinaryType)
    uint32_t  oSemantic;            // Offset to semantic information
    uint32_t Offset;               // Offset in parent constant buffer
    uint32_t  oDefaultValue;        // Offset to default initializer value
    uint32_t  Flags;                // Explicit bind point
};

struct SBinaryInterfaceVariable
{
    uint32_t  oName;                // Offset to variable name
    uint32_t  oType;                // Offset to type information (SBinaryType)
    uint32_t  oDefaultValue;        // Offset to default initializer array (SBinaryInterfaceInitializer[Elements])
    uint32_t  Flags;
};

struct SBinaryInterfaceInitializer
{
    uint32_t  oInstanceName;
    uint32_t  ArrayIndex;
};

struct SBinaryObjectVariable
{
    uint32_t  oName;                // Offset to variable name
    uint32_t  oType;                // Offset to type information (SBinaryType)
    uint32_t  oSemantic;            // Offset to semantic information
    uint32_t  ExplicitBindPoint;    // Used when a variable has been explicitly bound (register(XX)). -1 if not

    // Initializer data:
    //
    // The type structure pointed to by oType gives you Elements, 
    // VarType (must be EVT_Object), and ObjectType
    //
    // For ObjectType == EOT_Blend, EOT_DepthStencil, EOT_Rasterizer, EOT_Sampler
    // struct 
    // {
    //   uint32_t  cAssignments;
    //   SBinaryAssignment Assignments[cAssignments];
    // } Blocks[Elements]
    //
    // For EObjectType == EOT_Texture*, EOT_Buffer
    // <nothing>
    //
    // For EObjectType == EOT_*Shader, EOT_String
    // uint32_t  oData[Elements]; // offsets to a shader data block or a nullptr-terminated string
    //
    // For EObjectType == EOT_GeometryShaderSO
    //   SBinaryGSSOInitializer[Elements]
    //
    // For EObjectType == EOT_*Shader5
    //   SBinaryShaderData5[Elements]
};

struct SBinaryGSSOInitializer
{
    uint32_t  oShader;              // Offset to shader bytecode data block
    uint32_t  oSODecl;              // Offset to StreamOutput decl string
};

struct SBinaryShaderData5
{
    uint32_t  oShader;              // Offset to shader bytecode data block
    uint32_t  oSODecls[4];          // Offset to StreamOutput decl strings
    uint32_t  cSODecls;             // Count of valid oSODecls entries.
    uint32_t  RasterizedStream;     // Which stream is used for rasterization
    uint32_t  cInterfaceBindings;   // Count of interface bindings.
    uint32_t  oInterfaceBindings;   // Offset to SBinaryInterfaceInitializer[cInterfaceBindings].
};

struct SBinaryType
{
    uint32_t    oTypeName;      // Offset to friendly type name ("float4", "VS_OUTPUT")
    EVarType    VarType;        // Numeric, Object, or Struct
    uint32_t    Elements;       // # of array elements (0 for non-arrays)
    uint32_t    TotalSize;      // Size in bytes; not necessarily Stride * Elements for arrays 
                                // because of possible gap left in final register
    uint32_t    Stride;         // If an array, this is the spacing between elements.
                                // For unpacked arrays, always divisible by 16-bytes (1 register).
                                // No support for packed arrays    
    uint32_t    PackedSize;     // Size, in bytes, of this data typed when fully packed

    struct SBinaryMember
    {
        uint32_t    oName;          // Offset to structure member name ("m_pFoo")
        uint32_t    oSemantic;      // Offset to semantic ("POSITION0")
        uint32_t    Offset;         // Offset, in bytes, relative to start of parent structure
        uint32_t    oType;          // Offset to member's type descriptor
    };

    // the data that follows depends on the VarType:
    // Numeric: SType::SNumericType
    // Object:  EObjectType
    // Struct:  
    //   struct
    //   {
    //        uint32_t          cMembers;
    //        SBinaryMembers    Members[cMembers];
    //   } MemberInfo
    //   struct
    //   {
    //        uint32_t              oBaseClassType;  // Offset to type information (SBinaryType)
    //        uint32_t              cInterfaces;
    //        uint32_t              oInterfaceTypes[cInterfaces];
    //   } SBinaryTypeInheritance
    // Interface: (nothing)
};

struct SBinaryNumericType
{
    ENumericLayout  NumericLayout   : 3;    // scalar (1x1), vector (1xN), matrix (NxN)
    EScalarType     ScalarType      : 5;    // float32, int32, int8, etc.
    uint32_t        Rows            : 3;    // 1 <= Rows <= 4
    uint32_t        Columns         : 3;    // 1 <= Columns <= 4
    uint32_t        IsColumnMajor   : 1;    // applies only to matrices
    uint32_t        IsPackedArray   : 1;    // if this is an array, indicates whether elements should be greedily packed
};

struct SBinaryTypeInheritance
{
    uint32_t oBaseClass;            // Offset to base class type info or 0 if no base class.
    uint32_t cInterfaces;

    // Followed by uint32_t[cInterfaces] with offsets to the type
    // info of each interface.
};

struct SBinaryGroup
{
    uint32_t  oName;
    uint32_t  cTechniques;
};

struct SBinaryTechnique
{
    uint32_t  oName;
    uint32_t  cPasses;
};

struct SBinaryPass
{
    uint32_t  oName;
    uint32_t  cAssignments;
};

enum ECompilerAssignmentType
{
    ECAT_Invalid,                   // Assignment-specific data (always in the unstructured blob)
    ECAT_Constant,                  // -N SConstant structures
    ECAT_Variable,                  // -nullptr terminated string with variable name ("foo")
    ECAT_ConstIndex,                // -SConstantIndex structure
    ECAT_VariableIndex,             // -SVariableIndex structure
    ECAT_ExpressionIndex,           // -SIndexedObjectExpression structure
    ECAT_Expression,                // -Data block containing FXLVM code
    ECAT_InlineShader,              // -Data block containing shader
    ECAT_InlineShader5,             // -Data block containing shader with extended 5.0 data (SBinaryShaderData5)
};

struct SBinaryAssignment
{
    uint32_t iState;                // index into g_lvGeneral
    uint32_t Index;                 // the particular index to assign to (see g_lvGeneral to find the # of valid indices)
    ECompilerAssignmentType AssignmentType;
    uint32_t  oInitializer;         // Offset of assignment-specific data

    struct SConstantIndex
    {
        uint32_t  oArrayName;
        uint32_t Index;
    };

    struct SVariableIndex
    {
        uint32_t  oArrayName;
        uint32_t  oIndexVarName;
    };

    struct SIndexedObjectExpression
    {   
        uint32_t  oArrayName;
        uint32_t  oCode;
    };

    struct SInlineShader
    {
        uint32_t  oShader;
        uint32_t  oSODecl;
    };
};

struct SBinaryConstant
{
    EScalarType Type;
    union
    {
        BOOL    bValue;
        INT     iValue;
        float   fValue;
    };
};

static_assert( sizeof(SBinaryHeader) == 76, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryHeader::SVarCounts) == 12, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryHeader5) == 96, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryConstantBuffer) == 20, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryAnnotation) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryNumericVariable) == 24, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryInterfaceVariable) == 16, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryInterfaceInitializer) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryObjectVariable) == 16, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryGSSOInitializer) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryShaderData5) == 36, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryType) == 24, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryType::SBinaryMember) == 16, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryNumericType) == 4, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryTypeInheritance) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryGroup) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryTechnique) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryPass) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryAssignment) == 16, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryAssignment::SConstantIndex) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryAssignment::SVariableIndex) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryAssignment::SIndexedObjectExpression) == 8, "FX11 binary size mismatch" );
static_assert( sizeof(SBinaryAssignment::SInlineShader) == 8, "FX11 binary size mismatch" );

} // end namespace D3DX11Effects


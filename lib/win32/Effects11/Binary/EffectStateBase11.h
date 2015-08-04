//--------------------------------------------------------------------------------------
// File: EffectStateBase11.h
//
// Direct3D 11 Effects States Header
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

namespace D3DX11Effects
{

//////////////////////////////////////////////////////////////////////////
// Effect HLSL states and late resolve lists
//////////////////////////////////////////////////////////////////////////

struct RValue
{
    const char  *m_pName;
    uint32_t    m_Value;
};

#define RVALUE_END()    { nullptr, 0U }
#define RVALUE_ENTRY(prefix, x)         { #x, (uint32_t)prefix##x }

enum ELhsType;

struct LValue
{
    const char      *m_pName;           // name of the LHS side of expression
    EBlockType      m_BlockType;        // type of block it can appear in
    D3D_SHADER_VARIABLE_TYPE m_Type;    // data type allows
    uint32_t        m_Cols;             // number of [m_Type]'s required (1 for a scalar, 4 for a vector)
    uint32_t        m_Indices;          // max index allowable (if LHS is an array; otherwise this is 1)
    bool            m_VectorScalar;     // can be both vector and scalar (setting as a scalar sets all m_Indices values simultaneously)
    const RValue    *m_pRValue;         // pointer to table of allowable RHS "late resolve" values
    ELhsType        m_LhsType;          // ELHS_* enum value that corresponds to this entry
    uint32_t        m_Offset;           // offset into the given block type where this value should be written
    uint32_t        m_Stride;           // for vectors, byte stride between two consecutive values. if 0, m_Type's size is used
};

#define LVALUE_END()    { nullptr, D3D_SVT_UINT, 0, 0, 0, nullptr }

extern const LValue g_lvGeneral[];
extern const uint32_t   g_lvGeneralCount;

} // end namespace D3DX11Effects

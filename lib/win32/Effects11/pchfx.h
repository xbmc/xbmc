//--------------------------------------------------------------------------------------
// File: pchfx.h
//
// Direct3D 11 shader effects precompiled header
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/p/?LinkId=271568
//--------------------------------------------------------------------------------------

#pragma once

#pragma warning(disable : 4102 4127 4201 4505 4616 4706 6326)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#include <D3DCompiler_x.h>
#define DCOMMON_H_INCLUDED
#define NO_D3D11_DEBUG_NAME
#else
#include <d3d11_1.h>
#include <D3DCompiler.h>
#endif

#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8 0x0602
#endif

#undef DEFINE_GUID
#include "INITGUID.h"

#include "d3dx11effect.h"

#define UNUSED -1

//////////////////////////////////////////////////////////////////////////

#define offsetof_fx( a, b ) (uint32_t)offsetof( a, b )

#include "d3dxGlobal.h"

#include <cstddef>
#include <cstdlib>

#include "Effect.h"
#include "EffectStateBase11.h"
#include "EffectLoad.h"


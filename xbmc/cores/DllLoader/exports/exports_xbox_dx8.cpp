/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
 
#include "stdafx.h"
#include "../DllLoader.h"
#ifdef HAS_XFONT
#include <xfont.h>
#endif
#ifdef _XBOX
#include <XGraphics.h>
#endif
#include "emu_dx8.h"

Export export_xbox_dx8[] =
{
  { "d3dSetTextureStageState",                      -1, d3dSetTextureStageState,                      NULL },
  { "d3dSetRenderState",                            -1, d3dSetRenderState,                            NULL },
  { "OutputDebug",                                  -1, OutputDebug,                                  NULL },
  { "d3dGetRenderState",                            -1, d3dGetRenderState,                            NULL },
  { "d3dSetTransform",                              -1, d3dSetTransform,                              NULL },
  { "d3dCreateTexture",                             -1, d3dCreateTexture,                             NULL },
  { "d3dDrawIndexedPrimitive",                      -1, d3dDrawIndexedPrimitive,                      NULL },
#ifdef _XBOX
  { "D3DTexture_LockRect@20",                       -1, D3DTexture_LockRect,                          NULL },
  { "D3DVertexBuffer_Lock2@8",                      -1, D3DVertexBuffer_Lock2,                        NULL },
  { "D3DResource_Release@4",                        -1, D3DResource_Release,                          NULL },
  { "D3DDevice_SetStreamSource@12",                 -1, D3DDevice_SetStreamSource,                    NULL },
  { "D3DDevice_SetVertexShader@4",                  -1, D3DDevice_SetVertexShader,                    NULL },
  { "D3DDevice_DrawVerticesUP@16",                  -1, D3DDevice_DrawVerticesUP,                     NULL },
  { "D3DDevice_DrawVertices@12",                    -1, D3DDevice_DrawVertices,                       NULL },
  { "D3DDevice_SetTexture@8",                       -1, D3DDevice_SetTexture,                         NULL },
  { "GetLastError",                                 -1, GetLastError,                                 NULL },
  { "D3DDevice_CreateVertexBuffer2@4",              -1, D3DDevice_CreateVertexBuffer2,                NULL },
  { "D3DDevice_CreateTexture2@28",                  -1, D3DDevice_CreateTexture2,                     NULL },
  { "D3D__TextureState",                            -1, D3D__TextureState,                            NULL },
  { "@D3DDevice_SetRenderState_Simple@8",           -1, D3DDevice_SetRenderState_Simple,              NULL },
  { "D3D__RenderState",                             -1, &D3D__RenderState,                            NULL },
  { "D3D__DirtyFlags",                              -1, &D3D__DirtyFlags,                             NULL },
  { "D3DDevice_SetRenderState_FillMode@4",          -1, D3DDevice_SetRenderState_FillMode,            NULL },
  { "D3DDevice_SetRenderState_CullMode@4",          -1, D3DDevice_SetRenderState_CullMode,            NULL },
  { "D3DDevice_SetRenderState_ZEnable@4",           -1, D3DDevice_SetRenderState_ZEnable,             NULL },
  { "D3DDevice_SetTransform@8",                     -1, D3DDevice_SetTransform,                       NULL },
  { "D3DDevice_GetTransform@8",                     -1, D3DDevice_GetTransform,                       NULL },
  { "D3DXMatrixLookAtLH@16",                        -1, D3DXMatrixLookAtLH,                           NULL },
  { "D3DXMatrixPerspectiveFovLH@20",                -1, D3DXMatrixPerspectiveFovLH,                   NULL },
  { "D3DXMatrixMultiply@12",                        -1, D3DXMatrixMultiply,                           NULL },
  { "D3DXMatrixRotationYawPitchRoll@16",            -1, D3DXMatrixRotationYawPitchRoll,               NULL },
  { "D3DDevice_Clear@24",                           -1, D3DDevice_Clear,                              NULL },
  { "D3DDevice_GetDeviceCaps@4",                    -1, D3DDevice_GetDeviceCaps,                      NULL },
  { "D3DDevice_Swap@4",                             -1, D3DDevice_Swap,                               NULL },
  { "D3DDevice_SetMaterial@4",                      -1, D3DDevice_SetMaterial,                        NULL },
  { "D3DXMatrixScaling@16",                         -1, D3DXMatrixScaling,                            NULL },
  { "D3DXMatrixTranslation@16",                     -1, D3DXMatrixTranslation,                        NULL },
  { "D3DXCreateTextureFromFileA@12",                -1, d3dXCreateTextureFromFileA,                   NULL },
  { "D3DDevice_SetLight@8",                         -1, D3DDevice_SetLight,                           NULL },
  { "D3DDevice_LightEnable@8",                      -1, D3DDevice_LightEnable,                        NULL },
  { "D3DXVec3Normalize@8",                          -1, D3DXVec3Normalize,                            NULL },

  { "D3DDevice_GetBackBuffer2@4",                   -1, D3DDevice_GetBackBuffer2,                     NULL },
  { "D3DSurface_GetDesc@8",                         -1, D3DSurface_GetDesc,                           NULL },
  { "D3DXCreateTexture@32",                         -1, D3DXCreateTexture,                            NULL },
  { "D3DXCreateTextureFromFileInMemory@16",         -1, D3DXCreateTextureFromFileInMemory,            NULL },
  { "D3DBaseTexture_GetLevelCount@4",               -1, D3DBaseTexture_GetLevelCount,                 NULL },
  { "D3DTexture_GetSurfaceLevel2@8",                -1, D3DTexture_GetSurfaceLevel2,                  NULL },
  { "D3DTexture_GetSurfaceLevel2@8",                -1, D3DTexture_GetSurfaceLevel2,                  NULL },
  { "D3DSurface_LockRect@16",                       -1, D3DSurface_LockRect,                          NULL },

  { "D3DDevice_SetRenderState_YuvEnable@4",         -1, D3DDevice_SetRenderState_YuvEnable,           NULL },
  { "D3DDevice_SetRenderState_Dxt1NoiseEnable@4",   -1, D3DDevice_SetRenderState_Dxt1NoiseEnable,     NULL },
  { "D3DDevice_SetRenderState_SampleAlpha@4",       -1, D3DDevice_SetRenderState_SampleAlpha,         NULL },
  { "D3DDevice_SetRenderState_LineWidth@4",         -1, D3DDevice_SetRenderState_LineWidth,           NULL },
  { "D3DDevice_SetRenderState_ShadowFunc@4",        -1, D3DDevice_SetRenderState_ShadowFunc,          NULL },
  { "D3DDevice_SetRenderState_MultiSampleRenderTargetMode@4",-1, D3DDevice_SetRenderState_MultiSampleRenderTargetMode, NULL },
  { "D3DDevice_SetRenderState_MultiSampleMode@4",   -1, D3DDevice_SetRenderState_MultiSampleMode,     NULL },
  { "D3DDevice_SetRenderState_MultiSampleMask@4",   -1, D3DDevice_SetRenderState_MultiSampleMask,     NULL },
  { "D3DDevice_SetRenderState_MultiSampleAntiAlias@4",-1, D3DDevice_SetRenderState_MultiSampleAntiAlias, NULL },
  { "D3DDevice_SetRenderState_EdgeAntiAlias@4",     -1, D3DDevice_SetRenderState_EdgeAntiAlias,       NULL },
  { "D3DDevice_SetRenderState_LogicOp@4",           -1, D3DDevice_SetRenderState_LogicOp,             NULL },
  { "D3DDevice_SetRenderState_ZBias@4",             -1, D3DDevice_SetRenderState_ZBias,               NULL },
  { "D3DDevice_SetRenderState_TextureFactor@4",     -1, D3DDevice_SetRenderState_TextureFactor,       NULL },
  { "D3DDevice_SetRenderState_FrontFace@4",         -1, D3DDevice_SetRenderState_FrontFace,           NULL },
  { "D3DDevice_SetRenderState_OcclusionCullEnable@4",-1, D3DDevice_SetRenderState_OcclusionCullEnable, NULL },
  { "D3DDevice_SetRenderState_StencilFail@4",       -1, D3DDevice_SetRenderState_StencilFail,         NULL },
  { "D3DDevice_SetRenderState_StencilEnable@4",     -1, D3DDevice_SetRenderState_StencilEnable,       NULL },
  { "D3DDevice_SetRenderState_NormalizeNormals@4",  -1, D3DDevice_SetRenderState_NormalizeNormals,    NULL },
  { "D3DDevice_SetRenderState_TwoSidedLighting@4",  -1, D3DDevice_SetRenderState_TwoSidedLighting,    NULL },
  { "D3DDevice_SetRenderState_BackFillMode@4",      -1, D3DDevice_SetRenderState_BackFillMode,        NULL },
  { "D3DDevice_SetRenderState_FogColor@4",          -1, D3DDevice_SetRenderState_FogColor,            NULL },
  { "D3DDevice_SetRenderState_VertexBlend@4",       -1, D3DDevice_SetRenderState_VertexBlend,         NULL },
  { "D3DDevice_SetRenderState_PSTextureModes@4",    -1, D3DDevice_SetRenderState_PSTextureModes,      NULL },
  { "D3DDevice_SetTextureState_BumpEnv@12",         -1, D3DDevice_SetTextureState_BumpEnv,            NULL },
  { "D3DDevice_SetTextureState_ColorKeyColor@8",    -1, D3DDevice_SetTextureState_ColorKeyColor,      NULL },
  { "D3DDevice_SetTextureState_BorderColor@8",      -1, D3DDevice_SetTextureState_BorderColor,        NULL },
  { "D3DDevice_SetTextureState_TexCoordIndex@8",    -1, D3DDevice_SetTextureState_TexCoordIndex,      NULL },
  { "D3DDevice_DrawIndexedVerticesUP@20",           -1, D3DDevice_DrawIndexedVerticesUP,              NULL },
  { "D3DDevice_SetRenderState_StencilCullEnable@4", -1, D3DDevice_SetRenderState_StencilCullEnable,   NULL },
  { "D3DDevice_SetRenderState_RopZCmpAlwaysRead@4", -1, D3DDevice_SetRenderState_RopZCmpAlwaysRead,   NULL },
  { "D3DDevice_SetRenderState_RopZRead@4",          -1, D3DDevice_SetRenderState_RopZRead,            NULL },
  { "D3DDevice_SetRenderState_DoNotCullUncompressed@4",-1, D3DDevice_SetRenderState_DoNotCullUncompressed, NULL },

  { "D3DDevice_CopyRects@20",                       -1, D3DDevice_CopyRects,                          NULL },
  { "D3DDevice_SetPixelShader@4",                   -1, D3DDevice_SetPixelShader,                     NULL },
  { "D3DDevice_GetPersistedSurface2@0",             -1, D3DDevice_GetPersistedSurface2,               NULL },
  { "D3DResource_AddRef@4",                         -1, D3DResource_AddRef,                           NULL },

  { "D3DDevice_Release@0",                          -1, D3DDevice_Release,                            NULL },

// XFONT Support
  { "XFONT_SetTextColor@8",                         -1, XFONT_SetTextColor,                           NULL },
  { "XFONT_SetBkColor@8",                           -1, XFONT_SetBkColor,                             NULL },
  { "XFONT_SetBkMode@8",                            -1, XFONT_SetBkMode,                              NULL },
  { "XFONT_TextOut@24",                             -1, XFONT_TextOut,                                NULL },
  { "XFONT_OpenDefaultFont@4",                      -1, XFONT_OpenDefaultFont,                        NULL },

  { "XGBuffer_Release@4",                           -1, XGBuffer_Release,                             NULL },
  { "XGAssembleShader@44",                          -1, XGAssembleShader,                             NULL },
  { "D3DDevice_DeletePixelShader@4",                -1, D3DDevice_DeletePixelShader,                  NULL },
  { "D3DDevice_CreatePixelShader@8",                -1, D3DDevice_CreatePixelShader,                  NULL },
  { "D3DDevice_SetRenderTarget@8",                  -1, D3DDevice_SetRenderTarget,                    NULL },
  { "D3DDevice_GetRenderTarget2@0",                 -1, D3DDevice_GetRenderTarget2,                   NULL },
  { "D3DDevice_CreateIndexBuffer2@4",               -1, D3DDevice_CreateIndexBuffer2,                 NULL },
  { "D3DDevice_SetIndices@8",                       -1, D3DDevice_SetIndices,                         NULL },
  { "D3DDevice_CreateVertexShader@16",              -1, D3DDevice_CreateVertexShader,                 NULL },
  { "D3DDevice_DeleteVertexShader@4",               -1, D3DDevice_DeleteVertexShader,                 NULL },
  { "@D3DDevice_SetVertexShaderConstant4@8",        -1, D3DDevice_SetVertexShaderConstant4,           NULL },
  { "@D3DDevice_SetVertexShaderConstant1@8",        -1, D3DDevice_SetVertexShaderConstant1,           NULL },
  { "D3DXMatrixTranspose@8",                        -1, D3DXMatrixTranspose,                          NULL },
  { "D3DXMatrixInverse@12",                         -1, D3DXMatrixInverse,                            NULL },
  { "D3DXMatrixLookAtLH@16",                        -1, D3DXMatrixLookAtLH,                           NULL },
  { "@D3DDevice_SetVertexShaderConstant1Fast@8",    -1, D3DDevice_SetVertexShaderConstant1Fast,       NULL },
  { "@D3DDevice_SetVertexShaderConstantNotInlineFast@12",   -1, D3DDevice_SetVertexShaderConstantNotInlineFast, NULL },
  { "D3DXCreateCubeTextureFromFileA@12",            -1, d3dXCreateCubeTextureFromFileA,               NULL },
  { "D3DXCreateTextureFromFileExA@56",              -1, d3dXCreateTextureFromFileExA,                 NULL },
  { "D3DDevice_GetDepthStencilSurface2@0",          -1, D3DDevice_GetDepthStencilSurface2,            NULL },
  { "D3DXMatrixOrthoLH@20",                         -1, D3DXMatrixOrthoLH,                            NULL },
  { "Direct3D_GetDeviceCaps@12",                    -1, Direct3D_GetDeviceCaps,                       NULL },
  { "D3DSurface_GetDesc@8",                         -1, D3DSurface_GetDesc,                           NULL },
  { "D3DDevice_SetScissors@12",                     -1, D3DDevice_SetScissors,                        NULL },
  { "D3DDevice_DrawIndexedVertices@12",             -1, D3DDevice_DrawIndexedVertices,                NULL },
  { "D3DDevice_BeginVisibilityTest@0",              -1, D3DDevice_BeginVisibilityTest,                NULL },
  { "D3DDevice_EndVisibilityTest@4",                -1, D3DDevice_EndVisibilityTest,                  NULL },
  { "D3DDevice_GetVisibilityTestResult@12",         -1, D3DDevice_GetVisibilityTestResult,            NULL },
  { "D3DXCreateMatrixStack@8",                      -1, D3DXCreateMatrixStack,                        NULL },
  { "D3DDevice_CreateSurface2@16",                  -1, D3DDevice_CreateSurface2,                     NULL },
  { "D3DDevice_SetViewport@4",                      -1, D3DDevice_SetViewport,                        NULL },
#endif
  { NULL,                                         NULL, NULL,                                         NULL }
};

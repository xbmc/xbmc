
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"
#include <xfont.h>
#include <XGraphics.h>
#include "emu_dx8.h"

void export_xbox_dx8()
{
  g_dlls.xbox_dx8.AddExport("D3DVertexBuffer_Lock2@8", (unsigned long)D3DVertexBuffer_Lock2);
  g_dlls.xbox_dx8.AddExport("D3DTexture_LockRect@20", (unsigned long)D3DTexture_LockRect);
  g_dlls.xbox_dx8.AddExport("D3DResource_Release@4", (unsigned long)D3DResource_Release);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetStreamSource@12", (unsigned long)D3DDevice_SetStreamSource);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetVertexShader@4", (unsigned long)D3DDevice_SetVertexShader);
  g_dlls.xbox_dx8.AddExport("D3DDevice_DrawVerticesUP@16", (unsigned long)D3DDevice_DrawVerticesUP);
  g_dlls.xbox_dx8.AddExport("D3DDevice_DrawVertices@12", (unsigned long)D3DDevice_DrawVertices);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetTexture@8", (unsigned long)D3DDevice_SetTexture);
  g_dlls.xbox_dx8.AddExport("GetLastError", (unsigned long)GetLastError);
  g_dlls.xbox_dx8.AddExport("D3DDevice_CreateVertexBuffer2@4", (unsigned long)D3DDevice_CreateVertexBuffer2);
  g_dlls.xbox_dx8.AddExport("D3DDevice_CreateTexture2@28", (unsigned long)D3DDevice_CreateTexture2);
  g_dlls.xbox_dx8.AddExport("D3D__TextureState", (unsigned long)D3D__TextureState);
  g_dlls.xbox_dx8.AddExport("@D3DDevice_SetRenderState_Simple@8", (unsigned long)D3DDevice_SetRenderState_Simple);
  g_dlls.xbox_dx8.AddExport("D3D__RenderState", (unsigned long)D3D__RenderState);
  g_dlls.xbox_dx8.AddExport("D3D__DirtyFlags", (unsigned long)D3D__DirtyFlags);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_FillMode@4", (unsigned long)D3DDevice_SetRenderState_FillMode);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_CullMode@4", (unsigned long)D3DDevice_SetRenderState_CullMode);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_ZEnable@4", (unsigned long)D3DDevice_SetRenderState_ZEnable);
  g_dlls.xbox_dx8.AddExport("d3dSetTextureStageState", (unsigned long)d3dSetTextureStageState);
  g_dlls.xbox_dx8.AddExport("d3dSetRenderState", (unsigned long)d3dSetRenderState);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetTransform@8", (unsigned long)D3DDevice_SetTransform);
  g_dlls.xbox_dx8.AddExport("D3DDevice_GetTransform@8", (unsigned long)D3DDevice_GetTransform);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixLookAtLH@16", (unsigned long)D3DXMatrixLookAtLH);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixPerspectiveFovLH@20", (unsigned long)D3DXMatrixPerspectiveFovLH);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixMultiply@12", (unsigned long)D3DXMatrixMultiply);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixRotationYawPitchRoll@16", (unsigned long)D3DXMatrixRotationYawPitchRoll);
  g_dlls.xbox_dx8.AddExport("D3DDevice_Clear@24", (unsigned long)D3DDevice_Clear);
  g_dlls.xbox_dx8.AddExport("D3DDevice_GetDeviceCaps@4", (unsigned long)D3DDevice_GetDeviceCaps);
  g_dlls.xbox_dx8.AddExport("D3DDevice_Swap@4", (unsigned long)D3DDevice_Swap);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetMaterial@4", (unsigned long)D3DDevice_SetMaterial);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixScaling@16", (unsigned long)D3DXMatrixScaling);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixTranslation@16", (unsigned long)D3DXMatrixTranslation);
  g_dlls.xbox_dx8.AddExport("D3DXCreateTextureFromFileA@12", (unsigned long)D3DXCreateTextureFromFileA);
  g_dlls.xbox_dx8.AddExport("OutputDebug", (unsigned long)OutputDebug);
  g_dlls.xbox_dx8.AddExport("d3dGetRenderState", (unsigned long)d3dGetRenderState);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetLight@8", (unsigned long)D3DDevice_SetLight);
  g_dlls.xbox_dx8.AddExport("D3DDevice_LightEnable@8", (unsigned long)D3DDevice_LightEnable);
  g_dlls.xbox_dx8.AddExport("D3DXVec3Normalize@8", (unsigned long)D3DXVec3Normalize);

  g_dlls.xbox_dx8.AddExport("D3DDevice_GetBackBuffer2@4", (unsigned long)D3DDevice_GetBackBuffer2);
  g_dlls.xbox_dx8.AddExport("D3DSurface_GetDesc@8", (unsigned long)D3DSurface_GetDesc);
  g_dlls.xbox_dx8.AddExport("D3DXCreateTexture@32", (unsigned long)D3DXCreateTexture);
  g_dlls.xbox_dx8.AddExport("D3DXCreateTextureFromFileInMemory@16", (unsigned long)D3DXCreateTextureFromFileInMemory);
  g_dlls.xbox_dx8.AddExport("D3DBaseTexture_GetLevelCount@4", (unsigned long)D3DBaseTexture_GetLevelCount);
  g_dlls.xbox_dx8.AddExport("D3DTexture_GetSurfaceLevel2@8", (unsigned long)D3DTexture_GetSurfaceLevel2);
  g_dlls.xbox_dx8.AddExport("D3DTexture_GetSurfaceLevel2@8", (unsigned long)D3DTexture_GetSurfaceLevel2);
  g_dlls.xbox_dx8.AddExport("D3DSurface_LockRect@16", (unsigned long)D3DSurface_LockRect);

  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_YuvEnable@4", (unsigned long)D3DDevice_SetRenderState_YuvEnable);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_Dxt1NoiseEnable@4", (unsigned long)D3DDevice_SetRenderState_Dxt1NoiseEnable);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_SampleAlpha@4", (unsigned long)D3DDevice_SetRenderState_SampleAlpha);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_LineWidth@4", (unsigned long)D3DDevice_SetRenderState_LineWidth);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_ShadowFunc@4", (unsigned long)D3DDevice_SetRenderState_ShadowFunc);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_MultiSampleRenderTargetMode@4", (unsigned long)D3DDevice_SetRenderState_MultiSampleRenderTargetMode);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_MultiSampleMode@4", (unsigned long)D3DDevice_SetRenderState_MultiSampleMode);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_MultiSampleMask@4", (unsigned long)D3DDevice_SetRenderState_MultiSampleMask);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_MultiSampleAntiAlias@4", (unsigned long)D3DDevice_SetRenderState_MultiSampleAntiAlias);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_EdgeAntiAlias@4", (unsigned long)D3DDevice_SetRenderState_EdgeAntiAlias);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_LogicOp@4", (unsigned long)D3DDevice_SetRenderState_LogicOp);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_ZBias@4", (unsigned long)D3DDevice_SetRenderState_ZBias);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_TextureFactor@4", (unsigned long)D3DDevice_SetRenderState_TextureFactor);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_FrontFace@4", (unsigned long)D3DDevice_SetRenderState_FrontFace);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_OcclusionCullEnable@4", (unsigned long)D3DDevice_SetRenderState_OcclusionCullEnable);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_StencilFail@4", (unsigned long)D3DDevice_SetRenderState_StencilFail);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_StencilEnable@4", (unsigned long)D3DDevice_SetRenderState_StencilEnable);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_NormalizeNormals@4", (unsigned long)D3DDevice_SetRenderState_NormalizeNormals);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_TwoSidedLighting@4", (unsigned long)D3DDevice_SetRenderState_TwoSidedLighting);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_BackFillMode@4", (unsigned long)D3DDevice_SetRenderState_BackFillMode);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_FogColor@4", (unsigned long)D3DDevice_SetRenderState_FogColor);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_VertexBlend@4", (unsigned long)D3DDevice_SetRenderState_VertexBlend);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_PSTextureModes@4", (unsigned long)D3DDevice_SetRenderState_PSTextureModes);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetTextureState_BumpEnv@12", (unsigned long)D3DDevice_SetTextureState_BumpEnv);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetTextureState_ColorKeyColor@8", (unsigned long)D3DDevice_SetTextureState_ColorKeyColor);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetTextureState_BorderColor@8", (unsigned long)D3DDevice_SetTextureState_BorderColor);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetTextureState_TexCoordIndex@8", (unsigned long)D3DDevice_SetTextureState_TexCoordIndex);
  g_dlls.xbox_dx8.AddExport("D3DDevice_DrawIndexedVerticesUP@20", (unsigned long)D3DDevice_DrawIndexedVerticesUP);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_StencilCullEnable@4", (unsigned long)D3DDevice_SetRenderState_StencilCullEnable);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_RopZCmpAlwaysRead@4", (unsigned long)D3DDevice_SetRenderState_RopZCmpAlwaysRead);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_RopZRead@4", (unsigned long)D3DDevice_SetRenderState_RopZRead);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderState_DoNotCullUncompressed@4", (unsigned long)D3DDevice_SetRenderState_DoNotCullUncompressed);

  g_dlls.xbox_dx8.AddExport("D3DDevice_CopyRects@20", (unsigned long)D3DDevice_CopyRects);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetPixelShader@4", (unsigned long)D3DDevice_SetPixelShader);
  g_dlls.xbox_dx8.AddExport("D3DDevice_GetPersistedSurface2@0", (unsigned long)D3DDevice_GetPersistedSurface2);
  g_dlls.xbox_dx8.AddExport("D3DResource_AddRef@4", (unsigned long)D3DResource_AddRef);

  g_dlls.xbox_dx8.AddExport("D3DDevice_Release@0", (unsigned long)D3DDevice_Release);

// XFONT Support
  g_dlls.xbox_dx8.AddExport("XFONT_SetTextColor@8", (unsigned long)XFONT_SetTextColor);
  g_dlls.xbox_dx8.AddExport("XFONT_SetBkColor@8", (unsigned long)XFONT_SetBkColor);
  g_dlls.xbox_dx8.AddExport("XFONT_SetBkMode@8", (unsigned long)XFONT_SetBkMode);
  g_dlls.xbox_dx8.AddExport("XFONT_TextOut@24", (unsigned long)XFONT_TextOut);
  g_dlls.xbox_dx8.AddExport("XFONT_OpenDefaultFont@4", (unsigned long)XFONT_OpenDefaultFont);

  g_dlls.xbox_dx8.AddExport("XGBuffer_Release@4", (unsigned long)XGBuffer_Release);
  g_dlls.xbox_dx8.AddExport("XGAssembleShader@44", (unsigned long)XGAssembleShader);
  g_dlls.xbox_dx8.AddExport("D3DDevice_DeletePixelShader@4", (unsigned long)D3DDevice_DeletePixelShader);
  g_dlls.xbox_dx8.AddExport("D3DDevice_CreatePixelShader@8", (unsigned long)D3DDevice_CreatePixelShader);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetRenderTarget@8", (unsigned long)D3DDevice_SetRenderTarget);
  g_dlls.xbox_dx8.AddExport("D3DDevice_GetRenderTarget2@0", (unsigned long)D3DDevice_GetRenderTarget2);
  g_dlls.xbox_dx8.AddExport("D3DDevice_CreateIndexBuffer2@4", (unsigned long)D3DDevice_CreateIndexBuffer2);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetIndices@8", (unsigned long)D3DDevice_SetIndices);
  g_dlls.xbox_dx8.AddExport("D3DDevice_CreateVertexShader@16", (unsigned long)D3DDevice_CreateVertexShader);
  g_dlls.xbox_dx8.AddExport("D3DDevice_DeleteVertexShader@4", (unsigned long)D3DDevice_DeleteVertexShader);
  g_dlls.xbox_dx8.AddExport("@D3DDevice_SetVertexShaderConstant4@8", (unsigned long)D3DDevice_SetVertexShaderConstant4);
  g_dlls.xbox_dx8.AddExport("@D3DDevice_SetVertexShaderConstant1@8", (unsigned long)D3DDevice_SetVertexShaderConstant1);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixTranspose@8", (unsigned long)D3DXMatrixTranspose);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixInverse@12", (unsigned long)D3DXMatrixInverse);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixLookAtLH@16", (unsigned long)D3DXMatrixLookAtLH);
  g_dlls.xbox_dx8.AddExport("@D3DDevice_SetVertexShaderConstant1Fast@8", (unsigned long)D3DDevice_SetVertexShaderConstant1Fast);
  g_dlls.xbox_dx8.AddExport("@D3DDevice_SetVertexShaderConstantNotInlineFast@12", (unsigned long)D3DDevice_SetVertexShaderConstantNotInlineFast);
  g_dlls.xbox_dx8.AddExport("D3DXCreateCubeTextureFromFileA@12", (unsigned long)D3DXCreateCubeTextureFromFileA);
  g_dlls.xbox_dx8.AddExport("d3dSetTransform", (unsigned long)d3dSetTransform);
  g_dlls.xbox_dx8.AddExport("D3DXCreateTextureFromFileExA@56", (unsigned long)D3DXCreateTextureFromFileExA);
  g_dlls.xbox_dx8.AddExport("D3DDevice_GetDepthStencilSurface2@0", (unsigned long)D3DDevice_GetDepthStencilSurface2);
  g_dlls.xbox_dx8.AddExport("D3DXMatrixOrthoLH@20", (unsigned long)D3DXMatrixOrthoLH);
  g_dlls.xbox_dx8.AddExport("Direct3D_GetDeviceCaps@12", (unsigned long)Direct3D_GetDeviceCaps);
  g_dlls.xbox_dx8.AddExport("D3DSurface_GetDesc@8", (unsigned long)D3DSurface_GetDesc);
  g_dlls.xbox_dx8.AddExport("D3DDevice_SetScissors@12", (unsigned long)D3DDevice_SetScissors);
  g_dlls.xbox_dx8.AddExport("d3dCreateTexture", (unsigned long)d3dCreateTexture);
  g_dlls.xbox_dx8.AddExport("D3DDevice_DrawIndexedVertices@12", (unsigned long)D3DDevice_DrawIndexedVertices);
  g_dlls.xbox_dx8.AddExport("d3dDrawIndexedPrimitive", (unsigned long)d3dDrawIndexedPrimitive);
  g_dlls.xbox_dx8.AddExport("D3DDevice_BeginVisibilityTest@0", (unsigned long)D3DDevice_BeginVisibilityTest);
  g_dlls.xbox_dx8.AddExport("D3DDevice_EndVisibilityTest@4", (unsigned long)D3DDevice_EndVisibilityTest);
  g_dlls.xbox_dx8.AddExport("D3DDevice_GetVisibilityTestResult@12", (unsigned long)D3DDevice_GetVisibilityTestResult);
  g_dlls.xbox_dx8.AddExport("D3DXCreateMatrixStack@8", (unsigned long)&D3DXCreateMatrixStack);
  g_dlls.xbox_dx8.AddExport("D3DDevice_CreateSurface2@16", (unsigned long)D3DDevice_CreateSurface2);
}

void export_xbox___dx8()
{
  g_dlls.xbox___dx8.AddExport("_D3DXCreateTexture@32", (unsigned long)&D3DXCreateTexture);
  g_dlls.xbox___dx8.AddExport("_D3DXCreateTextureFromFileInMemory@16", (unsigned long)&D3DXCreateTextureFromFileInMemory);
}


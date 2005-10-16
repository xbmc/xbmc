

#include "..\..\..\stdafx.h"
#include "emu_dx8.h"

extern "C" void OutputDebug(char* strDebug)
{
  OutputDebugString(strDebug);
}
extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ)
{
  g_graphicsContext.Get3DDevice()->SetTextureStageState(x, (D3DTEXTURESTAGESTATETYPE)dwY, dwZ);
}

extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ)
{
  g_graphicsContext.Get3DDevice()->SetRenderState((D3DRENDERSTATETYPE)dwY, dwZ);
}
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ)
{
  g_graphicsContext.Get3DDevice()->GetRenderState((D3DRENDERSTATETYPE)dwY, dwZ);
}
extern "C" void d3dSetTransform( DWORD dwY, D3DMATRIX* dwZ )
{
  g_graphicsContext.Get3DDevice()->SetTransform( (D3DTRANSFORMSTATETYPE)dwY, dwZ );
}

extern "C" bool d3dCreateTexture(unsigned int width, unsigned int height, LPDIRECT3DTEXTURE8 *pTexture)
{
  return (D3D_OK == g_graphicsContext.Get3DDevice()->CreateTexture(width, height, 1, 0, D3DFMT_LIN_A8R8G8B8 , 0, pTexture));
}

extern "C" void d3dDrawIndexedPrimitive(D3DPRIMITIVETYPE primType, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primCount)
{
  g_graphicsContext.Get3DDevice()->DrawIndexedPrimitive(primType, minIndex, numVertices, startIndex, primCount);
}
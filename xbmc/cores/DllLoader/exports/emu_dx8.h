
#ifndef _EMU_DX8_H
#define _EMU_DX8_H

extern "C" void OutputDebug(char* strDebug);
extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
extern "C" void d3dSetTransform(DWORD dwY, D3DMATRIX* dwZ);
extern "C" bool d3dCreateTexture(unsigned int width, unsigned int height, LPDIRECT3DTEXTURE8 *pTexture);
extern "C" void d3dDrawIndexedPrimitive(D3DPRIMITIVETYPE primType, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primCount);

#endif // _EMU_DX8_H


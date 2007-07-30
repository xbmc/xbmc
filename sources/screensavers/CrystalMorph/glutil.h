#pragma once

#include <xtl.h>
#pragma comment (lib, "lib/xbox_dx8.lib" )

extern "C" void d3dSetTransform(DWORD dwY, D3DMATRIX* dwZ);
extern LPDIRECT3DDEVICE8 g_pd3dDevice;


void glInit();
void glPopMatrix();
void glPushMatrix();
void glSetMatrix(D3DXMATRIX * matrix);
void glScalef(float x, float y, float z);
void glTranslatef(float x, float y, float z);
void glRotatef(float x, float y, float z);
void glApply();

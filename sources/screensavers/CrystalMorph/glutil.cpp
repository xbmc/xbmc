#include "glutil.h"

#define MAX_STACK_SIZE 15

D3DXMATRIX g_matrixStack[MAX_STACK_SIZE];

int g_iCurrentMatrix = 0;

void glInit()
{
	D3DXMatrixIdentity(&g_matrixStack[0]);
	g_iCurrentMatrix = 0;
}

void glSetMatrix(D3DXMATRIX * matrix)
{
	memcpy(&g_matrixStack[g_iCurrentMatrix],matrix,sizeof(D3DXMATRIX));
}

void glPopMatrix()
{
	g_iCurrentMatrix--;
}
void glPushMatrix()
{
	memcpy(&g_matrixStack[g_iCurrentMatrix+1],g_matrixStack[g_iCurrentMatrix],sizeof(D3DXMATRIX));
	g_iCurrentMatrix++;
}

void glScalef(float x, float y, float z)
{
	D3DXMATRIX temp;
	D3DXMatrixScaling( &temp, x, y, z );
	D3DXMatrixMultiply(&g_matrixStack[g_iCurrentMatrix],&temp,&g_matrixStack[g_iCurrentMatrix]);
}
void glTranslatef(float x, float y, float z)
{
	D3DXMATRIX temp;
	D3DXMatrixTranslation(&temp, x, y, z);
	D3DXMatrixMultiply(&g_matrixStack[g_iCurrentMatrix],&temp,&g_matrixStack[g_iCurrentMatrix]);
}

void glRotatef(float x, float y, float z)
{
	D3DXMATRIX temp;
	D3DXMatrixRotationYawPitchRoll(&temp, x, y, z);
	D3DXMatrixMultiply(&g_matrixStack[g_iCurrentMatrix],&temp,&g_matrixStack[g_iCurrentMatrix]);
}

void glApply()
{
	d3dSetTransform(D3DTS_WORLD, &g_matrixStack[g_iCurrentMatrix]);
}
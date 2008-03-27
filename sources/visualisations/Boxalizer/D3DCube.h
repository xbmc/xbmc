#include <xtl.h>
#pragma comment (lib, "lib/xbox_dx8.lib" )
#define NoVs 30

class CD3DCube
{
public:
	void CleanUp();
	bool Init(LPDIRECT3DDEVICE8 pD3DDevice, float fX1, float fY1, float fZ1, float fHeight, float fWidth, float fDepth);
	bool Set(float fHeight);
	void Render(LPDIRECT3DTEXTURE8 pTexture);
	
protected:
	void		SetColourMulti();

	LPDIRECT3DVERTEXBUFFER8 m_pVertexBuffer;
	LPDIRECT3DDEVICE8		m_pd3dDevice;
	bool					m_bInited;
	float					m_fX, m_fY, m_fZ;
	float					m_fHeight, m_fWidth, m_fDepth;

	struct CUSTOMVERTEX
	{
		float x, y, z;
		DWORD colour;
		float tu, tv;
	};
	CUSTOMVERTEX cvVertices[NoVs];//14];
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)


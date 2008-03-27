#include "D3DCube.h"

class CBoxLine
{
public:
	void CleanUp();
	bool Init(LPDIRECT3DDEVICE8 pD3DDevice, float fLineZ);
	void Render(LPDIRECT3DTEXTURE8 pTexture);
	bool Set(float *pFreqData);
	bool CheckVisible(float fCamPos);

protected:
	CD3DCube			*m_pCubes;
	LPDIRECT3DDEVICE8	m_pd3dDevice;

	float				m_fBaseHeight;
	float				m_fBarWidth;
	float				m_fBaseZ;
};

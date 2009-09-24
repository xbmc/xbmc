#include "BoxLine.h"

struct LineListLine
{
	CBoxLine		myLine;
	LineListLine	*next;
	LineListLine	*prev;
};

class CBoxalizer
{
public:
	void CleanUp();
	bool Init(LPDIRECT3DDEVICE8 pd3dDevice);
	bool Set(float *pFreqData);
	void Render(float fCamZ);

	float GetNextZ();
	DWORD GetLastRowTime();

protected:
	LineListLine		*AddLine();
	void				DelLine(LineListLine *myListItem);

	LPDIRECT3DDEVICE8	m_pd3dDevice;
	LPDIRECT3DTEXTURE8	m_pTexture;
	LineListLine		*myLines;
	LineListLine		*m_lllCurrentActive;

	DWORD				m_dwLastNewBlock;
	float				m_fNextZ;
};

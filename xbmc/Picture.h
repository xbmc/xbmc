#pragma once
#include "lib/cximage/ximage.h"
#include "graphiccontext.h"
#include "stdstring.h"
using namespace std;

class CPicture
{
public:
	CPicture(void);
	virtual ~CPicture(void);
	IDirect3DTexture8*	Load(const CStdString& strFilename, int iRotate=0);
  bool                CreateThumnail(const CStdString& strFileName);
	DWORD								GetWidth() const;
	DWORD								GetHeight() const;

private:
	IDirect3DTexture8*	GetTexture( CxImage& image );
	void								Free();
	bool								m_bSectionLoaded;
	DWORD								m_dwHeight;
	DWORD								m_dwWidth;
};

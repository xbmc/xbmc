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
	IDirect3DTexture8*	Load(const CStdString& strFilename, int iRotate=0, int iMaxWidth=128, int iMaxHeight=128, bool bRGB=true);

  bool                CreateThumnail(const CStdString& strFileName);
  bool                CreateAlbumThumbnail(const CStdString& strFileName, const CStdString& strAlbum);
	bool                Convert(const CStdString& strSource,const CStdString& strDest);
	DWORD								GetWidth() const;
	DWORD								GetHeight() const;
	void								RenderImage(IDirect3DTexture8* pTexture, int x, int y, int width, int height, int iTextureWidth, int iTextureHeight, bool bRGB=true);

protected:
	bool								DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, int nMaxWidth, int nMaxHeight, bool bCacheFile=true);

private:
  struct VERTEX 
	{ 
    D3DXVECTOR4 p;
		D3DCOLOR col; 
		FLOAT tu, tv; 
	};
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;

	IDirect3DTexture8*				GetTexture( CxImage& image );
	IDirect3DTexture8*				GetYUY2Texture( CxImage& image );
	void											Free();
	bool											m_bSectionLoaded;
	DWORD											m_dwHeight;
	DWORD											m_dwWidth;
};

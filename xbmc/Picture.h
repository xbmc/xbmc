#pragma once
#include "lib/cximage/ximage.h"

class CPicture
{
public:
	CPicture(void);
	virtual ~CPicture(void);
	IDirect3DTexture8*	Load(const CStdString& strFilename, int &iOriginalWidth, int &iOriginalHeight, int iMaxWidth=128, int iMaxHeight=128, bool bCreateThumb = false);
	CxImage*						LoadImage(const CStdString& strFilename, int &iOriginalWidth, int &iOriginalHeight, int iMaxWidth=128, int iMaxHeight=128);

  bool                CreateThumbFromImage(const CStdString &strFileName, const CStdString& strThumbnailImage, CxImage& image, int iMaxWidth, int iMaxHeight, bool bWrongFormat = false);
  bool                CreateThumnail(const CStdString& strFileName);
  bool                CreateAlbumThumbnail(const CStdString& strFileName, const CStdString& strAlbum);
	bool								CreateAlbumThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName);
	int									DetectFileType(const BYTE* pBuffer, int nBufSize);
	bool                Convert(const CStdString& strSource,const CStdString& strDest);
	DWORD								GetWidth() const;
	DWORD								GetHeight() const;
	long								GetExifOrientation() const {return m_ExifOrientation;};
	void								RenderImage(IDirect3DTexture8* pTexture, float x, float y, float width, float height, int iTextureWidth, int iTextureHeight, int iTextureLeft=0, int iTextureTop=0, DWORD dwAlpha=0xFF);

	void								CreateFolderThumb(CStdString &strFolder, CStdString *strThumbs);
protected:
	IDirect3DTexture8*	LoadNative(const CStdString& strFilename);
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
	void											Free();
	bool											m_bSectionLoaded;
	DWORD											m_dwHeight;
	DWORD											m_dwWidth;
	long											m_ExifOrientation;
};

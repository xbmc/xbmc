#pragma once
#include "DllImageLib.h"

class CPicture
{
public:
  CPicture(void);
  virtual ~CPicture(void);
  IDirect3DTexture8* Load(const CStdString& strFilename, int iMaxWidth = 128, int iMaxHeight = 128);

  bool CreateThumbnail(const CStdString& strFileName);
  bool CreateExifThumbnail(const CStdString &strFile, const CStdString &strCachedThumb);
  bool CreateAlbumThumbnail(const CStdString& strFileName, const CStdString& strAlbum);
  bool CreateAlbumThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName);
  bool CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName);
  bool Convert(const CStdString& strSource, const CStdString& strDest);
  int ConvertFile(const CStdString& srcFile, const CStdString& destFile, float rotateDegrees, int width, int height, unsigned int quality);

  ImageInfo GetInfo() const { return m_info; };
  unsigned int GetWidth() const { return m_info.width; };
  unsigned int GetHeight() const { return m_info.height; };
  unsigned int GetOriginalWidth() const { return m_info.originalwidth; };
  unsigned int GetOriginalHeight() const { return m_info.originalheight; };
  long GetExifOrientation() const { return m_info.rotation; };

  void RenderImage(IDirect3DTexture8* pTexture, float x, float y, float width, float height, int iTextureWidth, int iTextureHeight, int iTextureLeft = 0, int iTextureTop = 0, DWORD dwAlpha = 0xFF);

  void CreateFolderThumb(CStdString &strFolder, CStdString *strThumbs);
  bool DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName);

protected:
  
private:
  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  DllImageLib m_dll;

  ImageInfo m_info;
};

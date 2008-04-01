#pragma once
#include "DllImageLib.h"

#ifdef _XBOX
#define RESAMPLE_CACHED_IMAGES
#endif

class CPicture
{
public:
  CPicture(void);
  virtual ~CPicture(void);
  IDirect3DTexture8* Load(const CStdString& strFilename, int iMaxWidth = 128, int iMaxHeight = 128);

  bool CreateThumbnailFromMemory(const BYTE* pBuffer, int nBufSize, const CStdString& strExtension, const CStdString& strThumbFileName);
  bool CreateThumbnailFromSurface(BYTE* pBuffer, int width, int height, int stride, const CStdString &strThumbFileName);
  bool CreateThumbnailFromSwizzledTexture(LPDIRECT3DTEXTURE8 &texture, int width, int height, const CStdString &thumb);
  int ConvertFile(const CStdString& srcFile, const CStdString& destFile, float rotateDegrees, int width, int height, unsigned int quality);

  ImageInfo GetInfo() const { return m_info; };
  unsigned int GetWidth() const { return m_info.width; };
  unsigned int GetHeight() const { return m_info.height; };
  unsigned int GetOriginalWidth() const { return m_info.originalwidth; };
  unsigned int GetOriginalHeight() const { return m_info.originalheight; };
  const EXIFINFO *GetExifInfo() const { return &m_info.exifInfo; };

  void CreateFolderThumb(const CStdString *strThumbs, const CStdString &folderThumbnail);
  bool DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName, bool checkExistence = false);
  bool CacheImage(const CStdString& sourceFileName, const CStdString& destFileName);

  // caches a skin image as a thumbnail image
  bool CacheSkinImage(const CStdString &srcFile, const CStdString &destFile);

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

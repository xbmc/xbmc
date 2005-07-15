#pragma once

class CPicture
{
public:
  struct ImageInfo
  {
    unsigned int width;
    unsigned int height;
    unsigned int originalwidth;
    unsigned int originalheight;
    int rotation;
    LPDIRECT3DTEXTURE8 texture;
  };

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
protected:
  bool DoCreateThumbnail(const CStdString& strFileName, const CStdString& strThumbFileName);

private:
  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  struct ImageDLL
  {
    bool (__cdecl*  LoadImage)(const char *, unsigned int, unsigned int, ImageInfo *);
    bool (__cdecl*  CreateThumbnail)(const char *, const char *);
    bool (__cdecl*  CreateThumbnailFromMemory)(BYTE *, unsigned int, const char *, const char *);
    bool (__cdecl*  CreateFolderThumbnail)(const char **, const char *);
    bool (__cdecl*  CreateExifThumbnail)(const char *, const char *);
    bool (__cdecl*  CreateThumbnailFromSurface)(BYTE *, unsigned int, unsigned int, unsigned int, const char *);
    int  (__cdecl*  ConvertFile)(const char *, const char *, float, int, int, unsigned int);
  };
  bool LoadDLL();
  bool m_bDllLoaded;
  ImageDLL m_dll;

  void Free();

  ImageInfo m_info;
};

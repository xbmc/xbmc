#pragma once

class CAutoTexBuffer;

class CTextureBundle
{
  struct FileHeader_t
  {
    DWORD Offset;
    DWORD UnpackedSize;
    DWORD PackedSize;
  };

  HANDLE m_hFile;
  FILETIME m_TimeStamp;
  OVERLAPPED m_Ovl[2];
  std::map<CStdString, FileHeader_t> m_FileHeaders;
  std::map<CStdString, FileHeader_t>::iterator m_CurFileHeader[2];
  BYTE* m_PreLoadBuffer[2];
  int m_PreloadIdx;
  int m_LoadIdx;
  bool m_themeBundle;

  bool OpenBundle();
  HRESULT LoadFile(const CStdString& Filename, CAutoTexBuffer& UnpackedBuf);

public:
  CTextureBundle(void);
  ~CTextureBundle(void);

  void Cleanup();

  void SetThemeBundle(bool themeBundle);
  bool HasFile(const CStdString& Filename);
  void GetTexturesFromPath(const CStdString &path, std::vector<CStdString> &textures);
  bool PreloadFile(const CStdString& Filename);

  HRESULT LoadTexture(LPDIRECT3DDEVICE8 pDevice, const CStdString& Filename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8* ppTexture,
                      LPDIRECT3DPALETTE8* ppPalette);

  int LoadAnim(LPDIRECT3DDEVICE8 pDevice, const CStdString& Filename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8** ppTextures,
               LPDIRECT3DPALETTE8* ppPalette, int& nLoops, int** ppDelays);
};


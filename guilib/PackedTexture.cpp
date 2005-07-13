#include "include.h"
#include <XGraphics.h>
#include "PackedTexture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HRESULT LoadPackedTexture(LPDIRECT3DDEVICE8 pDevice, const char* szFilename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8* ppTexture,
                          LPDIRECT3DPALETTE8* ppPalette)
{
  *ppTexture = NULL; *ppPalette = NULL;

  HANDLE hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return E_FAIL;

  XPR_HEADER XPRHeader;
  DWORD n;
  if (!ReadFile(hFile, &XPRHeader, sizeof(XPR_HEADER), &n, NULL) || n < sizeof(XPR_HEADER))
  {
    CloseHandle(hFile);
    return E_INVALIDARG;
  }

  int Version = (XPRHeader.dwMagic >> 24) - '0';
  XPRHeader.dwMagic -= Version << 24;

  if (XPRHeader.dwMagic != XPR_MAGIC_VALUE)
  {
    CloseHandle(hFile);
    return E_INVALIDARG;
  }

  DWORD ResHeaderSize = XPRHeader.dwHeaderSize - sizeof(XPR_HEADER);
  char* HeaderBuf = new char[ResHeaderSize];
  if (!ReadFile(hFile, HeaderBuf, ResHeaderSize, &n, 0) || n != ResHeaderSize)
    goto PackedLoadError;
  char* Next = HeaderBuf;

  D3DTexture* pTex = (D3DTexture*)(new char[sizeof(D3DTexture) + sizeof(DWORD)]);
  D3DPalette* pPal = 0;

  DWORD ResDataSize = XPRHeader.dwTotalSize - XPRHeader.dwHeaderSize;
  void* ResData = XPhysicalAlloc(ResDataSize, MAXULONG_PTR, 128, PAGE_READWRITE | PAGE_WRITECOMBINE);
  if (!ResData)
    goto PackedLoadError;

  WORD RealSize[2];

  switch (Version)
  {
  case 0:  // tex, [pal], realsize
    {
      if (Next - HeaderBuf + sizeof(D3DTexture) + 4 > (int)ResHeaderSize)
        goto PackedLoadError;

      memcpy(pTex, Next, sizeof(D3DTexture));
      Next += sizeof(D3DTexture);

      if (ResHeaderSize >= sizeof(D3DTexture) + sizeof(D3DPalette) + 4)
      {
        pPal = new D3DPalette;
        memcpy(pPal, Next, sizeof(D3DPalette));
        Next += sizeof(D3DPalette);
      }

      memcpy(RealSize, Next, 4);
      Next += 4;
    }
    break;

  case 1:  // flags, [pal], {tex, realsize}, {tex, realsize} ...
    {
      enum XPR_FLAGS
      {
        XPRFLAG_PALETTE = 0x00000001,
        XPRFLAG_ANIM = 0x00000002,
      };

      DWORD flags = *(DWORD*)Next;
      Next += sizeof(DWORD);
      if (flags & XPRFLAG_ANIM)
        goto PackedLoadError;

      if (flags & XPRFLAG_PALETTE)
      {
        pPal = new D3DPalette;
        memcpy(pPal, Next, sizeof(D3DPalette));
        Next += sizeof(D3DPalette);
      }
      if (Next - HeaderBuf + sizeof(D3DTexture) + 4 > (int)ResHeaderSize)
        goto PackedLoadError;

      memcpy(pTex, Next, sizeof(D3DTexture));
      Next += sizeof(D3DTexture);

      memcpy(RealSize, Next, 4);
      Next += 4;
    }
    break;

  default:
    goto PackedLoadError;
  }

  delete [] HeaderBuf;
  HeaderBuf = 0;

  if (!ReadFile(hFile, ResData, ResDataSize, &n, NULL) || n != ResDataSize)
    goto PackedLoadError;

  CloseHandle(hFile);
  hFile = INVALID_HANDLE_VALUE;

  if ((pTex->Common & D3DCOMMON_TYPE_MASK) != D3DCOMMON_TYPE_TEXTURE)
    goto PackedLoadError;

  *ppTexture = (LPDIRECT3DTEXTURE8)pTex;
  (*ppTexture)->Register(ResData);
  *(DWORD*)(pTex + 1) = (DWORD)ResData;
  if (pPal)
  {
    *ppPalette = (LPDIRECT3DPALETTE8)pPal;
    (*ppPalette)->Register(ResData);
  }

  pInfo->Width = RealSize[0];
  pInfo->Height = RealSize[1];
  pInfo->Depth = 0;
  pInfo->MipLevels = 1;
  D3DSURFACE_DESC desc;
  (*ppTexture)->GetLevelDesc(0, &desc);
  pInfo->Format = desc.Format;

  return S_OK;

PackedLoadError:
  delete [] pTex;
  if (pPal) delete pPal;
  if (HeaderBuf) delete [] HeaderBuf;
  if (ResData) D3D_FreeContiguousMemory(ResData);
  if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
  return E_FAIL;
}

int LoadPackedAnim(LPDIRECT3DDEVICE8 pDevice, const char* szFilename, D3DXIMAGE_INFO* pInfo, LPDIRECT3DTEXTURE8** ppTextures,
                   LPDIRECT3DPALETTE8* ppPalette, int& nLoops, int** ppDelays)
{
  *ppTextures = NULL; *ppPalette = NULL; *ppDelays = NULL;

  HANDLE hFile = CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (hFile == INVALID_HANDLE_VALUE)
    return 0;

  XPR_HEADER XPRHeader;
  DWORD n;
  if (!ReadFile(hFile, &XPRHeader, sizeof(XPR_HEADER), &n, NULL) || n < sizeof(XPR_HEADER))
  {
    CloseHandle(hFile);
    return 0;
  }

  int Version = (XPRHeader.dwMagic >> 24) - '0';
  XPRHeader.dwMagic -= Version << 24;

  if (XPRHeader.dwMagic != XPR_MAGIC_VALUE)
  {
    CloseHandle(hFile);
    return 0;
  }

  DWORD ResHeaderSize = XPRHeader.dwHeaderSize - sizeof(XPR_HEADER);
  char* HeaderBuf = new char[ResHeaderSize];
  if (!ReadFile(hFile, HeaderBuf, ResHeaderSize, &n, 0) || n != ResHeaderSize)
    goto PackedAnimError;
  char* Next = HeaderBuf;

  D3DTexture** ppTex = 0;
  D3DPalette* pPal = 0;

  DWORD ResDataSize = XPRHeader.dwTotalSize - XPRHeader.dwHeaderSize;
  void* ResData = XPhysicalAlloc(ResDataSize, MAXULONG_PTR, 128, PAGE_READWRITE | PAGE_WRITECOMBINE);
  if (!ResData)
    goto PackedAnimError;

  int nTextures;
  struct AnimInfo_t
  {
    DWORD nLoops;
    WORD RealSize[2];
  }
  *pAnimInfo;

  switch (Version)
  {
  case 1:  // flags, animinfo, [pal], {tex, delay}, {tex, delay} ...
    {
      enum XPR_FLAGS
      {
        XPRFLAG_PALETTE = 0x00000001,
        XPRFLAG_ANIM = 0x00000002,
      };

      if (ResHeaderSize < sizeof(DWORD) + sizeof(AnimInfo_t))
        goto PackedAnimError;

      DWORD flags = *(DWORD*)Next;
      Next += sizeof(DWORD);
      if (!(flags & XPRFLAG_ANIM))
        goto PackedAnimError;

      pAnimInfo = (AnimInfo_t*)Next;
      Next += sizeof(AnimInfo_t);
      nLoops = pAnimInfo->nLoops;

      if (flags & XPRFLAG_PALETTE)
      {
        pPal = new D3DPalette;
        memcpy(pPal, Next, sizeof(D3DPalette));
        Next += sizeof(D3DPalette);
      }
      if (Next - HeaderBuf >= (int)ResHeaderSize)
        goto PackedAnimError;

      nTextures = (ResHeaderSize - (Next - HeaderBuf)) / (sizeof(D3DTexture) + sizeof(DWORD));
      ppTex = new D3DTexture * [nTextures];
      *ppDelays = new int[nTextures];
      for (int i = 0; i < nTextures; ++i)
      {
        ppTex[i] = (D3DTexture*)(new char[sizeof(D3DTexture) + sizeof(DWORD)]);
        memcpy(ppTex[i], Next, sizeof(D3DTexture));
        Next += sizeof(D3DTexture);

        (*ppDelays)[i] = *(int*)Next;
        Next += sizeof(int);
      }
    }
    break;

  default:
    goto PackedAnimError;
  }

  if (!ReadFile(hFile, ResData, ResDataSize, &n, NULL) || n != ResDataSize)
    goto PackedAnimError;

  CloseHandle(hFile);
  hFile = INVALID_HANDLE_VALUE;

  *ppTextures = new LPDIRECT3DTEXTURE8[nTextures];
  for (int i = 0; i < nTextures; ++i)
  {
    if ((ppTex[i]->Common & D3DCOMMON_TYPE_MASK) != D3DCOMMON_TYPE_TEXTURE)
      goto PackedAnimError;

    (*ppTextures)[i] = (LPDIRECT3DTEXTURE8)ppTex[i];
    (*ppTextures)[i]->Register(ResData);
    *(DWORD*)(ppTex[i] + 1) = 0;
  }
  *(DWORD*)(ppTex[0] + 1) = (DWORD)ResData;

  delete [] ppTex;
  ppTex = 0;

  if (pPal)
  {
    *ppPalette = (LPDIRECT3DPALETTE8)pPal;
    (*ppPalette)->Register(ResData);
  }

  pInfo->Width = pAnimInfo->RealSize[0];
  pInfo->Height = pAnimInfo->RealSize[1];
  pInfo->Depth = 0;
  pInfo->MipLevels = 1;
  pInfo->Format = D3DFMT_UNKNOWN;

  // don't free while animinfo in use!
  delete [] HeaderBuf;
  HeaderBuf = 0;

  return nTextures;

PackedAnimError:
  if (ppTex)
  {
    for (int i = 0; i < nTextures; ++i)
      delete [] ppTex[i];
    delete [] ppTex;
  }
  if (pPal) delete pPal;
  if (*ppDelays) delete [] *ppDelays;
  if (HeaderBuf) delete [] HeaderBuf;
  if (ResData) D3D_FreeContiguousMemory(ResData);
  if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
  return 0;
}

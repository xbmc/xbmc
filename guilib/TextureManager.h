#ifndef GUILIB_TEXTUREMANAGER_H
#define GUILIB_TEXTUREMANAGER_H
#include "gui3d.h"

#include <vector>
#include "stdstring.h"
using namespace std;

#pragma once

class CGUITextureManager
{
public:
  CGUITextureManager(void);
  virtual ~CGUITextureManager(void);

  LPDIRECT3DTEXTURE8 GetTexture(const CStdString& strTextureName, DWORD dwColorKey=0);
  void               ReleaseTexture(const CStdString& strTextureName);
  void               Cleanup();
protected:
  
  class CTexture
  {
    public:
      CTexture::CTexture(){};
      virtual CTexture::~CTexture(){};
      CStdString              strTextureName;
      LPDIRECT3DTEXTURE8  m_pTexture;
      int                 m_iReferenceCount;
  };

  vector<CTexture> m_vecTextures;
  typedef   vector<CTexture>::iterator ivecTextures;

};
extern CGUITextureManager g_TextureManager;
#endif
#ifndef GUILIB_TEXTUREMANAGER_H
#define GUILIB_TEXTUREMANAGER_H
#include "gui3d.h"

#include <vector>
#include "stdstring.h"
using namespace std;

#pragma once
class CTexture
{
  public:
    CTexture();
    CTexture(LPDIRECT3DTEXTURE8 pTexture,int iWidth, int iHeight, int iDelay=100);
    virtual   ~CTexture();
    bool                Release();
    LPDIRECT3DTEXTURE8  GetTexture(int& iWidth, int& iHeight);
    int                 GetDelay() const;
    int                 GetRef() const;
    void                Dump() const;
  protected:
    LPDIRECT3DTEXTURE8  m_pTexture;
    int                 m_iReferenceCount;
    int                 m_iDelay;
		int									m_iWidth;
		int									m_iHeight;
};

class CTextureMap
{
  public:
    CTextureMap();
    CTextureMap(const CStdString& strTextureName);
    virtual   ~CTextureMap();
    const CStdString&   GetName() const;
    int                 size() const;
    LPDIRECT3DTEXTURE8  GetTexture(int iPicture,int& iWidth, int& iHeight);
    int                 GetDelay(int iPicture=0) const;
    void                Add(CTexture* pTexture);
    bool                Release(int iPicture=0);
    int                 IsEmpty() const;
    void                Dump() const;
  protected:  
    CStdString          m_strTextureName;
    vector<CTexture*>   m_vecTexures;
    typedef vector<CTexture*>::iterator ivecTextures; 
};

class CGUITextureManager
{
public:
  CGUITextureManager(void);
  virtual ~CGUITextureManager(void);

  int                Load(const CStdString& strTextureName,DWORD dwColorKey=0);
  LPDIRECT3DTEXTURE8 GetTexture(const CStdString& strTextureName, int iItem,int& iWidth, int& iHeight);
  int                GetDelay(const CStdString& strTextureName, int iPicture=0) const;
  void               ReleaseTexture(const CStdString& strTextureName, int iPicture=0);
  void               Cleanup();
  void               Dump() const;
  
protected:
  vector<CTextureMap*> m_vecTextures;
  typedef   vector<CTextureMap*>::iterator ivecTextures;

};
extern CGUITextureManager g_TextureManager;
#endif
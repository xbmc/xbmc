/*!
	\file TextureManager.h
	\brief 
	*/

#ifndef GUILIB_TEXTUREMANAGER_H
#define GUILIB_TEXTUREMANAGER_H

#include "TextureBundle.h"

#pragma once
/*!
	\ingroup textures
	\brief 
	*/
class CTexture
{
  public:
    CTexture();
    CTexture(LPDIRECT3DTEXTURE8 pTexture,int iWidth, int iHeight, bool bPacked, int iDelay=100, LPDIRECT3DPALETTE8 pPalette = NULL);
    virtual   ~CTexture();
    bool                Release();
    LPDIRECT3DTEXTURE8  GetTexture(int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal);
    int                 GetDelay() const;
    int                 GetRef() const;
    void                Dump() const;
    void                SetDelay(int iDelay);
		void                Flush();
    void                SetLoops(int iLoops);
    int                 GetLoops() const;
  protected:
		void FreeTexture();

    LPDIRECT3DTEXTURE8  m_pTexture;
		LPDIRECT3DPALETTE8  m_pPalette;
    int                 m_iReferenceCount;
    int                 m_iDelay;
		int									m_iWidth;
		int									m_iHeight;
    int                 m_iLoops;
		bool                m_bPacked;
};

/*!
	\ingroup textures
	\brief 
	*/
class CTextureMap
{
  public:
    CTextureMap();
    CTextureMap(const CStdString& strTextureName);
    virtual   ~CTextureMap();
    const CStdString&   GetName() const;
    int                 size() const;
    LPDIRECT3DTEXTURE8  GetTexture(int iPicture,int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal);
    int                 GetDelay(int iPicture=0) const;
    int                 GetLoops(int iPicture=0) const;
    void                Add(CTexture* pTexture);
    bool                Release(int iPicture=0);
    bool                IsEmpty() const;
    void                Dump() const;
		void							  Flush();
  protected:  
    CStdString          m_strTextureName;
    vector<CTexture*>   m_vecTexures;
    typedef vector<CTexture*>::iterator ivecTextures; 
};

/*!
	\ingroup textures
	\brief 
	*/
class CGUITextureManager
{
public:
  CGUITextureManager(void);
  virtual ~CGUITextureManager(void);

	void               StartPreLoad();
	void               PreLoad(const CStdString& strTextureName);
	void               EndPreLoad();
	void               FlushPreLoad();
  int                Load(const CStdString& strTextureName,DWORD dwColorKey=0);
  LPDIRECT3DTEXTURE8 GetTexture(const CStdString& strTextureName, int iItem,int& iWidth, int& iHeight, LPDIRECT3DPALETTE8& pPal);
  int                GetDelay(const CStdString& strTextureName, int iPicture=0) const;
  int                GetLoops(const CStdString& strTextureName, int iPicture=0) const;
  void               ReleaseTexture(const CStdString& strTextureName, int iPicture=0);
  void               Cleanup();
  void               Dump() const;
  void							 Flush();
protected:
  vector<CTextureMap*> m_vecTextures;
  typedef   vector<CTextureMap*>::iterator ivecTextures;
	CTextureBundle	m_TexBundle;
	list<CStdString> m_PreLoadNames;
	list<CStdString>::iterator m_iNextPreload;
};

/*!
	\ingroup textures
	\brief 
	*/
extern CGUITextureManager g_TextureManager;
#endif

#include "texturemanager.h"
#include "graphiccontext.h"

CGUITextureManager g_TextureManager;

CGUITextureManager::CGUITextureManager(void)
{
}

CGUITextureManager::~CGUITextureManager(void)
{
  Cleanup();
}


LPDIRECT3DTEXTURE8 CGUITextureManager::GetTexture(const CStdString& strTextureName, DWORD dwColorKey)
{
  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    CTexture& texture=m_vecTextures[i];
    if (texture.strTextureName==strTextureName)
    {
      texture.m_iReferenceCount++;
      return  texture.m_pTexture;
    }
  }

  LPDIRECT3DTEXTURE8      pTexture;

  CStdString strPath=g_graphicsContext.GetMediaDir();
	strPath+="\\media\\";
  strPath+=strTextureName;
  if (strTextureName.c_str()[1] == ':')
    strPath=strTextureName;

  if ( D3DXCreateTextureFromFileEx(g_graphicsContext.Get3DDevice(), strPath.c_str(),
		 D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
		 D3DX_FILTER_NONE , D3DX_FILTER_NONE, dwColorKey, NULL, NULL, &pTexture)!=D3D_OK)
	{
		OutputDebugString("Texture manager unable to find file: ");
		OutputDebugString(strPath.c_str());
		OutputDebugString("\n");
		return NULL;
	}

  CTexture newTexture;
  newTexture.strTextureName     = strTextureName;
  newTexture.m_iReferenceCount = 1;
  newTexture.m_pTexture        = pTexture;
  return pTexture;
}

void CGUITextureManager::ReleaseTexture(const CStdString& strTextureName)
{
  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTexture& texture=*i;
    if (texture.strTextureName==strTextureName)
    {
      texture.m_iReferenceCount--;
      if (texture.m_iReferenceCount==0)
      {
        texture.m_pTexture->Release();
        m_vecTextures.erase(i);
        return;
      }
    }
  }
}

void CGUITextureManager::Cleanup()
{
  ivecTextures i;
  i = m_vecTextures.begin();
  while (i != m_vecTextures.end())
  {
    CTexture& texture=*i;
    texture.m_pTexture->Release();
    i=m_vecTextures.erase(i);
  }
}
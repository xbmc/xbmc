#include "include.h"
#include "GUILargeImage.h"
#include "TextureManager.h"
#include "../xbmc/GUILargeTextureManager.h"

// TODO: Override SetFileName() to not FreeResources() immediately.  Instead, keep the current image around until we
//       actually have a new image in AllocResources().

CGUILargeImage::CGUILargeImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture)
: CGUIImage(dwParentID, dwControlId, posX, posY, width, height, texture, 0), m_fallbackImage(dwParentID, dwControlId, posX, posY, width, height, texture)
{
  m_fallbackImage.SetFileName(m_image.file);
  ControlType = GUICONTROL_LARGE_IMAGE;
  m_usingBundledTexture = false;
}

CGUILargeImage::~CGUILargeImage(void)
{

}

void CGUILargeImage::AllocateOnDemand()
{
  // if we're hidden, we can free our resources and return
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && IsAllocated())
      FreeResources();
    m_bWasVisible = false;
    return;
  }

  // either visible or delayed - we need the resources allocated in either case
  if (!m_texturesAllocated || !m_vecTextures.size())
    AllocResources();
}

void CGUILargeImage::PreAllocResources()
{
  FreeResources();
  if (!m_image.diffuse.IsEmpty())
    g_TextureManager.PreLoad(m_image.diffuse);
}

void CGUILargeImage::AllocResources()
{
  if (m_strFileName.IsEmpty())
    return;
  if (m_vecTextures.size())
    FreeTextures();
  // don't call CGUIControl::AllocTextures(), as this resets m_hasRendered, which we don't want
  m_bInvalidated = true;
  m_bAllocated = true;

  // first check our textureManager for bundled files
  int iImages = g_TextureManager.Load(m_strFileName, m_dwColorKey, true);
  if (iImages)
  {
    m_texturesAllocated = true;
    for (int i = 0; i < iImages; i++)
    {
      LPDIRECT3DTEXTURE8 texture = g_TextureManager.GetTexture(m_strFileName, i, m_iTextureWidth, m_iTextureHeight, m_pPalette, m_linearTexture);
      m_vecTextures.push_back(texture);
    }
    m_usingBundledTexture = true;
  }
  else
  { // use our large image background loader
    LPDIRECT3DTEXTURE8 texture = g_largeTextureManager.GetImage(m_strFileName, m_iTextureWidth, m_iTextureHeight, m_orientation, !m_texturesAllocated);
    m_texturesAllocated = true;

    if (!texture)
      return;

    m_usingBundledTexture = false;
    m_vecTextures.push_back(texture);
  }

#ifdef HAS_XBOX_D3D
  m_linearTexture = true;
#else
  m_linearTexture = false;
#endif

  CalculateSize();

  LoadDiffuseImage();
}

void CGUILargeImage::FreeResources()
{
  m_fallbackImage.FreeResources();
  CGUIImage::FreeResources();
}

void CGUILargeImage::FreeTextures()
{
  if (m_texturesAllocated)
  {
    if (m_usingBundledTexture)
      g_TextureManager.ReleaseTexture(m_strFileName);
    else
      g_largeTextureManager.ReleaseImage(m_strFileName);
  }

  if (m_diffuseTexture)
    g_TextureManager.ReleaseTexture(m_image.diffuse);
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;

  m_vecTextures.erase(m_vecTextures.begin(), m_vecTextures.end());
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_texturesAllocated = false;
}

int CGUILargeImage::GetOrientation() const
{
  if (m_usingBundledTexture) return m_image.orientation;
  return m_orientation;
}

void CGUILargeImage::SetFileName(const CStdString& strFileName)
{
  // no fallback is required - it's handled at rendertime
  if (m_strFileName.Equals(strFileName)) return;
  // Don't completely free resources here - we may be just changing
  // filenames mid-animation
  FreeTextures();  // TODO: perhaps freetextures might be done better after a fade or something?
  m_strFileName = strFileName;
  // Don't allocate resources here as this is done at render time
}

void CGUILargeImage::Render()
{
  if (!m_vecTextures.size())
    m_fallbackImage.Render();
  else
    m_fallbackImage.FreeResources();
  CGUIImage::Render();
}

// ideally we have just one texture file so that we know what it is.  Having the ability to load from either folder
// is IMO a beneficial thing to keep around.

// perhaps we could separate out the fallback texture from the <info> texture, allocate both in AllocResources(), and
// render the one that applies?  That then allows some nifty blending as needed as well.

// I seem to be liking this solution...


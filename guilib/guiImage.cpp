#include "include.h"
#include "guiImage.h"
#include "TextureManager.h"
#include "../xbmc/Settings.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "../xbmc/Util.h"
#ifdef HAS_SDL
#include <SDL/SDL_rotozoom.h>
#endif

#define MIX_ALPHA(a,c) (((a * (c >> 24)) / 255) << 24) | (c & 0x00ffffff)

CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, DWORD dwColorKey)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  memset(m_alpha, 0xff, 4);
  m_image = texture;
  g_infoManager.ParseLabel(m_image.file, m_multiInfo);
  if (m_multiInfo.size() == 1 && !m_multiInfo[0].m_info)
    m_multiInfo.clear();  // no info here at all - just a standard texture
  if (m_multiInfo.size())
    m_image.file.Empty(); // have multiinfo, so no fallback texture
  m_strFileName = m_image.file;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  m_dwColorKey = dwColorKey;
  m_iCurrentImage = 0;
  m_dwFrameCounter = (DWORD) -1;
  m_aspectRatio = ASPECT_RATIO_STRETCH;
  m_aspectAlign = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_bWasVisible = m_visible == VISIBLE;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_texturesAllocated = false;
  m_singleInfo = 0;
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;
}

CGUIImage::CGUIImage(const CGUIImage &left)
    : CGUIControl(left)
{
  m_strFileName = left.m_strFileName;
  m_dwColorKey = left.m_dwColorKey;
  m_aspectRatio = left.m_aspectRatio;
  m_aspectAlign = left.m_aspectAlign;
  // defaults
  m_iCurrentImage = 0;
  m_dwFrameCounter = (DWORD) -1;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_iTextureWidth = 0;
  m_iTextureHeight = 0;
  memcpy(m_alpha, left.m_alpha, 4);
  m_pPalette = NULL;
  ControlType = GUICONTROL_IMAGE;
  m_bDynamicResourceAlloc=false;
  m_texturesAllocated = false;
  m_multiInfo = left.m_multiInfo;
  m_singleInfo = left.m_singleInfo;
  m_image = left.m_image;
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;
}

CGUIImage::~CGUIImage(void)
{

}

void CGUIImage::UpdateVisibility()
{
  CGUIControl::UpdateVisibility();

  // check for conditional information before we free and
  // alloc as this does free and allocation as well
  if (m_multiInfo.size())
  {
    SetFileName(g_infoManager.GetMultiInfo(m_multiInfo, m_dwParentID, true));
  }
  if (m_singleInfo)
  {
    SetFileName(g_infoManager.GetImage(m_singleInfo, m_dwParentID));
  }

  AllocateOnDemand();
}

void CGUIImage::AllocateOnDemand()
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
  if (!m_texturesAllocated)
    AllocResources();
}

void CGUIImage::Render()
{
  // we need the checks for visibility and resource allocation here as
  // most controls use CGUIImage's to do their rendering (where UpdateVisibility doesn't apply)
  AllocateOnDemand();

  if (!IsVisible()) return;

  if (m_vecTextures.size())
  {
    Process();
    if (m_bInvalidated) CalculateSize();
    // scale to screen output position
    if (m_fNW > m_width || m_fNH > m_height)
    {
      if (!g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
      {
        CGUIControl::Render();
        return;
      }
    }

#ifndef HAS_SDL
    LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();
    // Set state to render the image
#ifdef ALLOW_TEXTURE_COMPRESSION
#ifdef HAS_XBOX_D3D
    if (!m_linearTexture)
      p3DDevice->SetPalette( 0, m_pPalette);
    if (m_diffusePalette)
      p3DDevice->SetPalette( 1, m_diffusePalette);
#endif
#endif
    p3DDevice->SetTexture( 0, m_vecTextures[m_iCurrentImage] );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    p3DDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    if (m_diffuseTexture)
    {
      p3DDevice->SetTexture( 1, m_diffuseTexture );
      p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
      p3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
      p3DDevice->SetTextureStageState( 1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
    }

    p3DDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
    p3DDevice->SetRenderState( D3DRS_ALPHAREF, 0 );
    p3DDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
    p3DDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    p3DDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    p3DDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    p3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    p3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    p3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    p3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    p3DDevice->SetRenderState( D3DRS_LIGHTING, FALSE);

#ifdef HAS_XBOX_D3D
    p3DDevice->SetRenderState( D3DRS_YUVENABLE, FALSE);
#endif

    p3DDevice->SetVertexShader( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2 );
#endif

#ifdef HAS_SDL_OPENGL
    CGLTexture* texture = m_vecTextures[m_iCurrentImage];
    glActiveTextureARB(GL_TEXTURE0_ARB);
    texture->LoadToGPU();
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glEnable(GL_TEXTURE_2D);
    
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);          // Turn Blending On
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
       
    // diffuse coloring
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    VerifyGLState();

    if (m_diffuseTexture)
    {
      m_diffuseTexture->LoadToGPU();
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glBindTexture(GL_TEXTURE_2D, m_diffuseTexture->id);
      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
      glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE1);
      glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
      glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
      glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
      VerifyGLState();
    }
    //glDisable(GL_TEXTURE_2D); // uncomment these 2 lines to switch to wireframe rendering
    //glBegin(GL_LINE_LOOP);
    glBegin(GL_QUADS);
#endif
    
    float uLeft, uRight, vTop, vBottom;

#ifdef ALLOW_TEXTURE_COMPRESSION
    if (!m_linearTexture)
    {
      uLeft = m_image.border.left / m_iImageWidth;
      uRight = m_fU - m_image.border.right / m_iImageWidth;
      vTop = m_image.border.top / m_iImageHeight;
      vBottom = m_fV - m_image.border.bottom / m_iImageHeight;
    }
    else
    {
#endif
      uLeft = m_image.border.left;
      uRight = m_fU - m_image.border.right;
      vTop = m_image.border.top;
      vBottom = m_fV - m_image.border.bottom;
#ifdef ALLOW_TEXTURE_COMPRESSION
    }
#endif

#ifdef HAS_XBOX_D3D
    p3DDevice->Begin(D3DPT_QUADLIST);
#endif

    // TODO: The diffuse coloring applies to all vertices, which will
    //       look weird for stuff with borders, as will the -ve height/width
    //       for flipping

    // left segment
    if (m_image.border.left)
    {
      if (m_image.border.top)
        Render(m_fX, m_fY, m_fX + m_image.border.left, m_fY + m_image.border.top, 0, 0, uLeft, vTop);
      Render(m_fX, m_fY + m_image.border.top, m_fX + m_image.border.left, m_fY + m_fNH - m_image.border.bottom, 0, vTop, uLeft, vBottom);
      if (m_image.border.bottom)
        Render(m_fX, m_fY + m_fNH - m_image.border.bottom, m_fX + m_image.border.left, m_fY + m_fNH, 0, vBottom, uLeft, m_fV); 
    }
    // middle segment
    if (m_image.border.top)
      Render(m_fX + m_image.border.left, m_fY, m_fX + m_fNW - m_image.border.right, m_fY + m_image.border.top, uLeft, 0, uRight, vTop);
    Render(m_fX + m_image.border.left, m_fY + m_image.border.top, m_fX + m_fNW - m_image.border.right, m_fY + m_fNH - m_image.border.bottom, uLeft, vTop, uRight, vBottom);
    if (m_image.border.bottom)
      Render(m_fX + m_image.border.left, m_fY + m_fNH - m_image.border.bottom, m_fX + m_fNW - m_image.border.right, m_fY + m_fNH, uLeft, vBottom, uRight, m_fV); 
    // right segment
    if (m_image.border.right)
    { // have a left border
      if (m_image.border.top)
        Render(m_fX + m_fNW - m_image.border.right, m_fY, m_fX + m_fNW, m_fY + m_image.border.top, uRight, 0, m_fU, vTop);
      Render(m_fX + m_fNW - m_image.border.right, m_fY + m_image.border.top, m_fX + m_fNW, m_fY + m_fNH - m_image.border.bottom, uRight, vTop, m_fU, vBottom);
      if (m_image.border.bottom)
        Render(m_fX + m_fNW - m_image.border.right, m_fY + m_fNH - m_image.border.bottom, m_fX + m_fNW, m_fY + m_fNH, uRight, vBottom, m_fU, m_fV); 
    } 
#ifdef ALLOW_TEXTURE_COMPRESSION
#ifdef HAS_XBOX_D3D
    p3DDevice->End();
    if (g_graphicsContext.RectIsAngled(m_fX, m_fY, m_fX + m_fNW, m_fY + m_fNH))
    {
      p3DDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE );
      p3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS, TRUE );
      p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME );
      p3DDevice->Begin(D3DPT_QUADLIST);
      Render(m_fX, m_fY, m_fX + m_fNW, m_fY + m_fNH, 0, 0, m_fU, m_fV);
      p3DDevice->End();
      p3DDevice->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
      p3DDevice->SetRenderState( D3DRS_EDGEANTIALIAS, FALSE );
      p3DDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    }
    if (!m_linearTexture)
      p3DDevice->SetPalette( 0, NULL);
    if (m_diffusePalette)
      p3DDevice->SetPalette( 1, NULL);
#endif
#endif

#ifdef HAS_SDL_OPENGL      
    glEnd();
    if (m_diffuseTexture)
    {
      glDisable(GL_TEXTURE_2D);
      glActiveTextureARB(GL_TEXTURE0_ARB);
    }
#endif

#ifndef HAS_SDL
    // unset the texture and palette or the texture caching crashes because the runtime still has a reference
    p3DDevice->SetTexture( 0, NULL );
    if (m_diffuseTexture)
      p3DDevice->SetTexture( 1, NULL );
#endif
      
    if (m_fNW > m_width || m_fNH > m_height)
      g_graphicsContext.RestoreClipRegion();
  }
  CGUIControl::Render();
}

void CGUIImage::Render(float left, float top, float right, float bottom, float u1, float v1, float u2, float v2)
{
#ifndef HAS_SDL
  LPDIRECT3DDEVICE8 p3DDevice = g_graphicsContext.Get3DDevice();
#endif  

  // flip the texture as necessary.  Diffuse just gets flipped according to m_image.orientation.
  // Main texture gets flipped according to GetOrientation().
  CRect diffuse(u1, v1, u2, v2);
  OrientateTexture(diffuse, m_image.orientation);
  CRect texture(u1, v1, u2, v2);
  int textureOrientation = GetOrientation();
  OrientateTexture(texture, textureOrientation);

  CRect vertex(left, top, right, bottom);
  g_graphicsContext.ClipRect(vertex, texture, m_diffuseTexture ? &diffuse : NULL);

  if (vertex.IsEmpty())
    return; // nothing to render
  /*
  float x1 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y1) + 0.5f);// - 0.5f;
  float y1 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y1) + 0.5f);// - 0.5f;
  float x2 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y1) + 0.5f);// - 0.5f;
  float y2 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y1) + 0.5f);// - 0.5f;
  float x3 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y2) + 0.5f);// - 0.5f;
  float y3 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y2) + 0.5f);// - 0.5f;
  float x4 = floor(g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y2) + 0.5f);// - 0.5f;
  float y4 = floor(g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y2) + 0.5f);// - 0.5f;
  */

  float x1 = g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y1);
  float y1 = g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y1);
  float x2 = g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y1);
  float y2 = g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y1);
  float x3 = g_graphicsContext.ScaleFinalXCoord(vertex.x2, vertex.y2);
  float y3 = g_graphicsContext.ScaleFinalYCoord(vertex.x2, vertex.y2);
  float x4 = g_graphicsContext.ScaleFinalXCoord(vertex.x1, vertex.y2);
  float y4 = g_graphicsContext.ScaleFinalYCoord(vertex.x1, vertex.y2);

  float z1 = g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y1);
  float z2 = g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y1);
  float z3 = g_graphicsContext.ScaleFinalZCoord(vertex.x2, vertex.y2);
  float z4 = g_graphicsContext.ScaleFinalZCoord(vertex.x1, vertex.y2);

  if (y3 == y1) y3 += 1.0f; if (x3 == x1) x3 += 1.0f;
  if (y4 == y2) y4 += 1.0f; if (x4 == x2) x4 += 1.0f;

  diffuse.x1 *= m_diffuseScaleU; diffuse.x2 *= m_diffuseScaleU;
  diffuse.y1 *= m_diffuseScaleV; diffuse.y2 *= m_diffuseScaleV;

#ifdef HAS_XBOX_D3D
  D3DCOLOR color = m_diffuseColor;
  if (m_alpha[0] != 0xFF) color = MIX_ALPHA(m_alpha[0],m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y1);
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x1, diffuse.y1);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x1, y1, z1, 1 );

  color = m_diffuseColor;
  if (m_alpha[1] != 0xFF) color = MIX_ALPHA(m_alpha[1],m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  if (textureOrientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y2);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y1);
  if (m_image.orientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x1, diffuse.y2);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x2, diffuse.y1);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x2, y2, z2, 1 );

  color =  m_diffuseColor;
  if (m_alpha[2] != 0xFF) color = MIX_ALPHA(m_alpha[2], m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y2);
  p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x2, diffuse.y2);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x3, y3, z3, 1 );

  color =  m_diffuseColor;
  if (m_alpha[3] != 0xFF) color = MIX_ALPHA(m_alpha[3], m_diffuseColor);
  p3DDevice->SetVertexDataColor(D3DVSDE_DIFFUSE, g_graphicsContext.MergeAlpha(color));
  if (textureOrientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x2, texture.y1);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD0, texture.x1, texture.y2);
  if (m_image.orientation & 4)
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x2, diffuse.y1);
  else
    p3DDevice->SetVertexData2f( D3DVSDE_TEXCOORD1, diffuse.x1, diffuse.y2);
  p3DDevice->SetVertexData4f( D3DVSDE_VERTEX, x4, y4, z4, 1 );

#elif !defined(HAS_SDL)
  struct CUSTOMVERTEX {
      FLOAT x, y, z;
      DWORD color;
      FLOAT tu, tv;   // Texture coordinates
      FLOAT tu2, tv2;
  };

  CUSTOMVERTEX verts[4];
  verts[0].x = x1; verts[0].y = y1; verts[0].z = z1;
  verts[0].tu = texture.x1;   verts[0].tv = texture.y1;
  verts[0].tu2 = diffuse.x1;  verts[0].tv2 = diffuse.y1;
  DWORD color = m_diffuseColor;
  if (m_alpha[0] != 0xFF) color = MIX_ALPHA(m_alpha[0],m_diffuseColor);
  verts[0].color = g_graphicsContext.MergeAlpha(color);

  verts[1].x = x2; verts[1].y = y2; verts[1].z = z2;
  if (textureOrientation & 4)
  {
    verts[1].tu = texture.x1;
    verts[1].tv = texture.y2;
  }
  else
  {
    verts[1].tu = texture.x2;
    verts[1].tv = texture.y1;
  }
  if (m_image.orientation & 4)
  {
    verts[1].tu2 = diffuse.x1;
    verts[1].tv2 = diffuse.y2;
  }
  else
  {
    verts[1].tu2 = diffuse.x2;
    verts[1].tv2 = diffuse.y1;
  }

  color = m_diffuseColor;
  if (m_alpha[1] != 0xFF) color = MIX_ALPHA(m_alpha[1],m_diffuseColor);
  verts[1].color = g_graphicsContext.MergeAlpha(color);

  verts[2].x = x3; verts[2].y = y3; verts[2].z = z3;
  verts[2].tu = texture.x2;   verts[2].tv = texture.y2;
  verts[2].tu2 = diffuse.x2;  verts[2].tv2 = diffuse.y2;
  color = m_diffuseColor;
  if (m_alpha[2] != 0xFF) color = MIX_ALPHA(m_alpha[2],m_diffuseColor);
  verts[2].color = g_graphicsContext.MergeAlpha(color);

  verts[3].x = x4; verts[3].y = y4; verts[3].z = z4;
  if (textureOrientation & 4)
  {
    verts[3].tu = texture.x2;
    verts[3].tv = texture.y1;
  }
  else
  {
    verts[3].tu = texture.x1;
    verts[3].tv = texture.y2;
  }
  if (m_image.orientation & 4)
  {
    verts[3].tu2 = diffuse.x2;
    verts[3].tv2 = diffuse.y1;
  }
  else
  {
    verts[3].tu2 = diffuse.x1;
    verts[3].tv2 = diffuse.y2;
  }
  color = m_diffuseColor;
  if (m_alpha[3] != 0xFF) color = MIX_ALPHA(m_alpha[3],m_diffuseColor);
  verts[3].color = g_graphicsContext.MergeAlpha(color);

  p3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(CUSTOMVERTEX));
#elif defined(HAS_SDL_2D)
#define USE_NEW_SDL_SCALING
  SDL_Surface* surface = m_vecTextures[m_iCurrentImage]; 
#ifdef USE_NEW_SDL_SCALING
  float x[4] = { x1, x2, x3, x4 };
  float y[4] = { y1, y2, y3, y4 };
  float u[2] = { u1, u2 };
  float v[2] = { v1, v2 };
  DWORD c[4] = { g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[0],m_diffuseColor)),
                 g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[1],m_diffuseColor)),
                 g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[2],m_diffuseColor)),                   g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[3],m_diffuseColor)) };
  
  // cache texture based on:
  // 1.  Bounding box
  // 2.  Diffuse color
  int b[4];
  CalcBoundingBox(x, y, 4, b);
  CCachedTexture &cached = m_vecCachedTextures[m_iCurrentImage];
  if (!cached.surface || cached.width != b[2] || cached.height != b[3] || c[0]  != cached.diffuseColor)
  { // need to re-render the surface
    RenderWithEffects(surface, x, y, u, v, c, m_diffuseTexture, m_diffuseScaleU, m_diffuseScaleV, cached);
  }
  if (cached.surface)
  {
    SDL_Rect dst = { (Sint16)b[0], (Sint16)b[1], 0, 0 };
    g_graphicsContext.BlitToScreen(cached.surface, NULL, &dst);
  }
#else
  DWORD colour = g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[0],m_diffuseColor));
  if (colour & 0xff000000)
  {
    SDL_Surface* zoomed = m_vecCachedTextures[m_iCurrentImage].surface;
    double zoomX = (double) (x3 - x1 + 1) / texture->w / (u2 - u1);
    double zoomY = (double) (y3 - y1 + 1) / texture->h / (v2 - v1);
    if (zoomed == NULL ||
        (int) ((double) texture->w * zoomX) != zoomed->w ||
        (int) ((double) texture->h * zoomY) != zoomed->h)
    { 
      if (zoomed != NULL)
        SDL_FreeSurface(zoomed);
      zoomed = zoomSurface(texture, zoomX, zoomY, 1);
      m_vecCachedTextures[m_iCurrentImage].surface = zoomed;
    }
    SDL_Rect dst = { (Sint16) x1, (Sint16) y1, 0, 0 };
    g_graphicsContext.BlitToScreen(zoomed, NULL,  &dst);
  }
#endif
#elif defined(HAS_SDL_OPENGL)
  // set all the attributes we need to...
  
  // Top-left vertex (corner)
  DWORD color = g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[0],m_diffuseColor));
  glColor4ub((GLubyte)((color >> 16) & 0xff), (GLubyte)((color >> 8) & 0xff), (GLubyte)(color & 0xff), (GLubyte)(color >> 24)); 
  glMultiTexCoord2f(GL_TEXTURE0_ARB, texture.x1, texture.y1);
  if (m_diffuseTexture)
    glMultiTexCoord2f(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y1);
  glVertex3f(x1, y1, z1);
  
  // Bottom-left vertex (corner)
  color = g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[1],m_diffuseColor));  
  glColor4ub((GLubyte)((color >> 16) & 0xff), (GLubyte)((color >> 8) & 0xff), (GLubyte)(color & 0xff), (GLubyte)(color >> 24)); 
  glMultiTexCoord2f(GL_TEXTURE0_ARB, texture.x2, texture.y1);
  if (m_diffuseTexture)
    glMultiTexCoord2f(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y1);
  glVertex3f(x2, y2, z2);
  
  // Bottom-right vertex (corner)
  color = g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[2],m_diffuseColor));  
  glColor4ub((GLubyte)((color >> 16) & 0xff), (GLubyte)((color >> 8) & 0xff), (GLubyte)(color & 0xff), (GLubyte)(color >> 24)); 
  glMultiTexCoord2f(GL_TEXTURE0_ARB, texture.x2, texture.y2);
  if (m_diffuseTexture)
    glMultiTexCoord2f(GL_TEXTURE1_ARB, diffuse.x2, diffuse.y2);
  glVertex3f(x3, y3, z3);
  
  // Top-right vertex (corner)
  color = g_graphicsContext.MergeAlpha(MIX_ALPHA(m_alpha[3],m_diffuseColor));  
  glColor4ub((GLubyte)((color >> 16) & 0xff), (GLubyte)((color >> 8) & 0xff), (GLubyte)(color & 0xff), (GLubyte)(color >> 24)); 
  glMultiTexCoord2f(GL_TEXTURE0_ARB, texture.x1, texture.y2);
  if (m_diffuseTexture)
    glMultiTexCoord2f(GL_TEXTURE1_ARB, diffuse.x1, diffuse.y2);
  glVertex3f(x4, y4, z4);
#endif
}

bool CGUIImage::OnAction(const CAction &action)
{
  return false;
}

bool CGUIImage::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_REFRESH_THUMBS)
  {
    if (m_singleInfo || m_multiInfo.size())
      FreeResources();
    return true;
  }
  return CGUIControl::OnMessage(message);
}

void CGUIImage::PreAllocResources()
{
  FreeResources();
  g_TextureManager.PreLoad(m_strFileName);
  if (!m_image.diffuse.IsEmpty())
    g_TextureManager.PreLoad(m_image.diffuse);
}

void CGUIImage::AllocResources()
{
  if (m_strFileName.IsEmpty())
    return;
  FreeTextures();
  CGUIControl::AllocResources();

  m_dwFrameCounter = 0;
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;

  int iImages = g_TextureManager.Load(m_strFileName, m_dwColorKey);
  // set allocated to true even if we couldn't load the image to save
  // use hitting the disk every frame
  m_texturesAllocated = true;
  if (!iImages) return ;
  for (int i = 0; i < iImages; i++)
  {
#ifndef HAS_SDL  
    LPDIRECT3DTEXTURE8 pTexture;
#elif defined(HAS_SDL_2D)
    SDL_Surface* pTexture;
#elif defined(HAS_SDL_OPENGL)
    CGLTexture* pTexture;
#endif

    pTexture = g_TextureManager.GetTexture(m_strFileName, i, m_iTextureWidth, m_iTextureHeight, m_pPalette, m_linearTexture);

#ifndef HAS_XBOX_D3D
    m_linearTexture = false;
#endif
    m_vecTextures.push_back(pTexture);
#ifdef HAS_SDL_2D
    m_vecCachedTextures.push_back(CCachedTexture());
#endif
  }

  CalculateSize();

  LoadDiffuseImage();
}

void CGUIImage::LoadDiffuseImage()
{
  m_diffuseScaleU = m_diffuseScaleV = 1.0f;
  // load the diffuse texture (if necessary)
  if (!m_image.diffuse.IsEmpty())
  {
    int width, height;
    bool linearTexture;
    g_TextureManager.Load(m_image.diffuse, 0);
    m_diffuseTexture = g_TextureManager.GetTexture(m_image.diffuse, 0, width, height, m_diffusePalette, linearTexture);

    if (m_diffuseTexture)
    { // calculate scaling for the texcoords (vs texcoords of main texture)
#ifdef HAS_XBOX_D3D
      if (linearTexture)
      {
        m_diffuseScaleU = float(width) / m_fU;
        m_diffuseScaleV = float(height) / m_fV;
      }
      else
#endif
      {
#ifndef HAS_SDL      
        D3DSURFACE_DESC desc;
        m_diffuseTexture->GetLevelDesc(0, &desc);
        m_diffuseScaleU = float(width) / float(desc.Width) / m_fU;
        m_diffuseScaleV = float(height) / float(desc.Height) / m_fV;
#elif defined(HAS_SDL_2D)
        m_diffuseScaleU = float(width) / float(m_diffuseTexture->w) / m_fU;
        m_diffuseScaleV = float(height) / float(m_diffuseTexture->h) / m_fV;		  	
#elif defined(HAS_SDL_OPENGL)
        m_diffuseScaleU = float(width) / float(m_diffuseTexture->textureWidth) / m_fU;
        m_diffuseScaleV = float(height) / float(m_diffuseTexture->textureHeight) / m_fV;		  	
#endif        
      }
    }
  }
}

void CGUIImage::FreeTextures()
{
  for (int i = 0; i < (int)m_vecTextures.size(); ++i)
  {
    g_TextureManager.ReleaseTexture(m_strFileName, i);
#ifdef HAS_SDL_2D
    if (m_vecCachedTextures[i].surface)
      SDL_FreeSurface(m_vecCachedTextures[i].surface);
    m_vecCachedTextures[i].surface = NULL;
#endif    
  }

  if (m_diffuseTexture)
    g_TextureManager.ReleaseTexture(m_image.diffuse);
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;

  m_vecTextures.clear();
#ifdef HAS_SDL_2D
  m_vecCachedTextures.clear();
#endif
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_texturesAllocated = false;
}

void CGUIImage::FreeResources()
{
  FreeTextures();
  CGUIControl::FreeResources();
}

void CGUIImage::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_bDynamicResourceAlloc=bOnOff;
}

void CGUIImage::CalculateSize()
{
  if (m_vecTextures.size() == 0) return ;

  m_fX = m_posX;
  m_fY = m_posY;
  if (!m_linearTexture)
  {
    if (0 == m_iImageWidth || 0 == m_iImageHeight)
    {
#ifndef HAS_SDL    
      D3DSURFACE_DESC desc;
      m_vecTextures[m_iCurrentImage]->GetLevelDesc(0, &desc);

      m_iImageWidth = desc.Width;
      m_iImageHeight = desc.Height;
#elif defined(HAS_SDL_2D)
      m_iImageWidth = m_vecTextures[m_iCurrentImage]->w;
      m_iImageHeight = m_vecTextures[m_iCurrentImage]->h;
#elif defined(HAS_SDL_OPENGL)
      m_iImageWidth = m_vecTextures[m_iCurrentImage]->textureWidth;
      m_iImageHeight = m_vecTextures[m_iCurrentImage]->textureHeight;
#endif	      
    }

    if (0 == m_iTextureWidth || 0 == m_iTextureHeight)
    {
      m_iTextureWidth = m_iImageWidth;
      m_iTextureHeight = m_iImageHeight;

      if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

      if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
    }
  }
  else
  {
    if (0 == m_iTextureWidth || 0 == m_iTextureHeight)
    {
#ifndef HAS_SDL    
      D3DSURFACE_DESC desc;
      m_vecTextures[m_iCurrentImage]->GetLevelDesc(0, &desc);

      m_iTextureWidth = desc.Width;
      m_iTextureHeight = desc.Height;
#elif defined(HAS_SDL_2D)
      m_iTextureWidth = m_vecTextures[m_iCurrentImage]->w;
      m_iTextureHeight = m_vecTextures[m_iCurrentImage]->h;
#elif defined(HAS_SDL_OPENGL)
      m_iImageWidth = m_vecTextures[m_iCurrentImage]->textureWidth;
      m_iImageHeight = m_vecTextures[m_iCurrentImage]->textureHeight;
#endif      

      if (m_iTextureHeight > g_graphicsContext.GetHeight() )
        m_iTextureHeight = g_graphicsContext.GetHeight();

      if (m_iTextureWidth > g_graphicsContext.GetWidth() )
        m_iTextureWidth = g_graphicsContext.GetWidth();
    }
  }

  if (m_width == 0)
    m_width = (float)m_iTextureWidth;
  if (m_height == 0)
    m_height = (float)m_iTextureHeight;


  m_fNW = m_width;
  m_fNH = m_height;

  if (m_aspectRatio != ASPECT_RATIO_STRETCH && m_iTextureWidth && m_iTextureHeight)
  {
    // to get the pixel ratio, we must use the SCALED output sizes
    float pixelRatio = g_graphicsContext.GetScalingPixelRatio();

    float fSourceFrameRatio = ((float)m_iTextureWidth) / ((float)m_iTextureHeight);
    float fOutputFrameRatio = fSourceFrameRatio / pixelRatio;

    // maximize the thumbnails width
    m_fNW = m_width;
    m_fNH = m_fNW / fOutputFrameRatio;

    if ((m_aspectRatio == CGUIImage::ASPECT_RATIO_SCALE && m_fNH < m_height) ||
        (m_aspectRatio == CGUIImage::ASPECT_RATIO_KEEP && m_fNH > m_height))
    {
      m_fNH = m_height;
      m_fNW = m_fNH * fOutputFrameRatio;
    }
    if (m_aspectRatio == CGUIImage::ASPECT_RATIO_CENTER)
    { // keep original size + center
      m_fNW = (float)m_iTextureWidth;
      m_fNH = (float)m_iTextureHeight;
    }

    // calculate placement
    if (m_aspectAlign & ASPECT_ALIGN_LEFT)
      m_fX = m_posX;
    else if (m_aspectAlign & ASPECT_ALIGN_RIGHT)
      m_fX = m_posX + m_width - m_fNW;
    else
      m_fX = m_posX + (m_width - m_fNW) * 0.5f;
    if (m_aspectAlign & ASPECT_ALIGNY_TOP)
      m_fY = m_posY;
    else if (m_aspectAlign & ASPECT_ALIGNY_BOTTOM)
      m_fY = m_posY + m_height - m_fNH;
    else
      m_fY = m_posY + (m_height - m_fNH) * 0.5f;
  }

  if (!m_linearTexture)
  {
    m_fU = float(m_iTextureWidth) / float(m_iImageWidth);
    m_fV = float(m_iTextureHeight) / float(m_iImageHeight);
  }
  else
  {
    m_fU = float(m_iTextureWidth);
    m_fV = float(m_iTextureHeight);
  }
}

bool CGUIImage::CanFocus() const
{
  return false;
}

void CGUIImage::Process()
{
  if (m_vecTextures.size() <= 1)
    return ;

  if (!m_bWasVisible)
  {
    m_iCurrentLoop = 0;
    m_iCurrentImage = 0;
    m_dwFrameCounter = 0;
    m_bWasVisible = true;
    return ;
  }

  m_dwFrameCounter++;
  DWORD dwDelay = g_TextureManager.GetDelay(m_strFileName, m_iCurrentImage);
  int iMaxLoops = g_TextureManager.GetLoops(m_strFileName, m_iCurrentImage);
  if (!dwDelay) dwDelay = 100;
  if (m_dwFrameCounter*40 >= dwDelay)
  {
    m_dwFrameCounter = 0;
    if (m_iCurrentImage + 1 >= (int)m_vecTextures.size() )
    {
      if (iMaxLoops > 0)
      {
        if (m_iCurrentLoop + 1 < iMaxLoops)
        {
          m_iCurrentLoop++;
          m_iCurrentImage = 0;
        }
      }
      else
      {
        // 0 == loop forever
        m_iCurrentImage = 0;
      }
    }
    else
    {
      m_iCurrentImage++;
    }
  }
}

int CGUIImage::GetTextureWidth() const
{
  return m_iTextureWidth;
}

int CGUIImage::GetTextureHeight() const
{
  return m_iTextureHeight;
}

void CGUIImage::SetAspectRatio(GUIIMAGE_ASPECT_RATIO ratio, DWORD align)
{
  if (m_aspectRatio != ratio || m_aspectAlign != align)
  {
    m_aspectRatio = ratio;
    m_aspectAlign = align;
    Update();
  }
}

void CGUIImage::PythonSetColorKey(DWORD dwColorKey)
{
  m_dwColorKey = dwColorKey;
}

void CGUIImage::SetFileName(const CStdString& strFileName)
{
  CStdString strTransFileName = _P(strFileName);
  if (strTransFileName.IsEmpty() && !m_image.file.IsEmpty())
    return SetFileName(m_image.file);

  if (m_strFileName.Equals(strTransFileName)) return;
  // Don't completely free resources here - we may be just changing
  // filenames mid-animation
  FreeTextures();
  m_strFileName = strTransFileName;
  // Don't allocate resources here as this is done at render time
}

void CGUIImage::SetAlpha(unsigned char alpha)
{
  SetAlpha(alpha, alpha, alpha, alpha);
}

void CGUIImage::SetAlpha(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3)
{
  m_alpha[0] = a0;
  m_alpha[1] = a1;
  m_alpha[2] = a2;
  m_alpha[3] = a3;
}

void CGUIImage::SetInfo(int info)
{
  m_singleInfo = info;
}

bool CGUIImage::IsAllocated() const
{
  if (!m_texturesAllocated) return false;
  return CGUIControl::IsAllocated();
}

#ifdef _DEBUG
void CGUIImage::DumpTextureUse()
{
  if (m_texturesAllocated && m_vecTextures.size())
  {
    if (GetID())
      CLog::Log(LOGDEBUG, "Image control %lu using texture %s", GetID(), m_strFileName.c_str());
    else
      CLog::Log(LOGDEBUG, "Using texture %s", m_strFileName.c_str());
  }
}
#endif

#ifdef HAS_SDL_2D
void CGUIImage::CalcBoundingBox(float *x, float *y, int n, int *b)
{
  b[0] = (int)x[0], b[2] = 1;
  b[1] = (int)y[0], b[3] = 1;
  for (int i = 1; i < 4; ++i)
  {
    if (x[i] < b[0]) b[0] = (int)x[i];
    if (y[i] < b[1]) b[1] = (int)y[i];
    if (x[i]+1 > b[0] + b[2]) b[2] = (int)x[i]+1 - b[0];
    if (y[i]+1 > b[1] + b[3]) b[3] = (int)y[i]+1 - b[1];
  }
}

#define CLAMP(a,b,c) (a < b) ? b : ((a > c) ? c : a)

void CGUIImage::GetTexel(float tu, float tv, SDL_Surface *src, BYTE *texel)
{
  int pu1 = (int)floor(tu);
  int pv1 = (int)floor(tv);
  int pu2 = pu1+1, pv2 = pv1+1;
  float du = tu - pu1;
  float dv = tv - pv1;
  pu1 = CLAMP(pu1,0,src->w-1);
  pu2 = CLAMP(pu2,0,src->w-1);
  pv1 = CLAMP(pv1,0,src->h-1);
  pv2 = CLAMP(pv2,0,src->h-1);
  BYTE *tex1 = (BYTE *)src->pixels + pv1 * src->pitch + pu1 * 4;
  BYTE *tex2 = (BYTE *)src->pixels + pv1 * src->pitch + pu2 * 4;
  BYTE *tex3 = (BYTE *)src->pixels + pv2 * src->pitch + pu1 * 4;
  BYTE *tex4 = (BYTE *)src->pixels + pv2 * src->pitch + pu2 * 4;
  // average these bytes
  texel[0] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
  texel[1] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
  texel[2] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
  texel[3] = (BYTE)((*tex1++ * (1-du) + *tex2++ * du)*(1-dv) + (*tex3++ * (1-du) + *tex4++ * du) * dv);
}

#define MODULATE(a,b) (b ? ((int)a*(b+1)) >> 8 : 0)
#define ALPHA_BLEND(a,b,c) (c ? ((int)a*(c+1) + b*(255-c)) >> 8 : b)

void CGUIImage::RenderWithEffects(SDL_Surface *src, float *x, float *y, float *u, float *v, DWORD *c, SDL_Surface *diffuse, float diffuseScaleU, float diffuseScaleV, CCachedTexture &dst)
{
  // renders the surface from u[0],v[0] -> u[1],v[1] into the parallelogram defined by x[0],y[0]...x[3],y[3]
  // we create a new surface of the appropriate size, and render into it the resized and rotated texture,
  // with diffuse modulation etc. as necessary.
  
  // first create our surface
  if (dst.surface)
    SDL_FreeSurface(dst.surface);
  // calculate the bounding box
  int b[4];
  CalcBoundingBox(x, y, 4, b);
  dst.width = b[2]; dst.height = b[3];
  dst.diffuseColor = c[0];
  
  // create a new texture this size
  dst.surface = SDL_CreateRGBSurface(SDL_HWSURFACE, dst.width+1, dst.height+1, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  if (!dst.surface)
    return; // can't create surface

  // vectors of parallelogram in screenspace
  float ax = x[1] - x[0];
  float ay = y[1] - y[0];
  float bx = x[3] - x[0];
  float by = y[3] - y[0];

  // as we're mapping to a parallelogram, don't assume that the vectors are orthogonal (allows for skewed textures later, and no more computation)
  // we want the coordinate vector wrt the basis vectors [ax,ay] and [bx,by], so invert the matrix [ax bx;ay by]
  u[0] *= src->w;
  v[0] *= src->h;
  u[1] *= src->w;
  v[1] *= src->h;
  
  float det_m = ax * by - ay * bx;
  float m[2][2];
  m[0][0] = by / det_m * (u[1] - u[0]);
  m[0][1] = -bx / det_m * (u[1] - u[0]);
  m[1][0] = -ay / det_m * (v[1] - v[0]);
  m[1][1] = ax / det_m * (v[1] - v[0]);
  
  // we render from these values in our texture.  Note that u[0] may be bigger than u[1] when flipping
  const float minU = min(u[0], u[1]) - 0.5f;
  const float maxU = max(u[0], u[1]) + 0.5f;
  const float minV = min(v[0], v[1]) - 0.5f;
  const float maxV = max(v[0], v[1]) + 0.5f;
  
  SDL_LockSurface(dst.surface);
  SDL_LockSurface(src);
  if (diffuse)
  {
    SDL_LockSurface(diffuse);
    diffuseScaleU *= diffuse->w / src->w;
    diffuseScaleV *= diffuse->h / src->h;
  }
  
  // for speed, find the bounding box of our x, y
  // for every pixel in the bounding box, find the corresponding texel
  for (int sy = 0; sy <= dst.height; ++sy)
  {
    for (int sx = 0; sx <= dst.width; ++sx)
    {
      // find where this pixel corresponds in our texture
      float tu = m[0][0] * (sx + b[0] - x[0] - 0.5f) + m[0][1] * (sy + b[1] - y[0] - 0.5f) + u[0];
      float tv = m[1][0] * (sx + b[0] - x[0] - 0.5f) + m[1][1] * (sy + b[1] - y[0] - 0.5f) + v[0];
      if (tu > minU && tu < maxU && tv > minV && tv < maxV)
      { // in the texture - render it to screen
        BYTE tex[4];
        GetTexel(tu, tv, src, tex);
        if (diffuse)
        {
          BYTE diff[4];
          GetTexel(tu * diffuseScaleU, tv * diffuseScaleV, diffuse, diff);
          tex[0] = MODULATE(tex[0], diff[0]);
          tex[1] = MODULATE(tex[1], diff[1]);
          tex[2] = MODULATE(tex[2], diff[2]);
          tex[3] = MODULATE(tex[3], diff[3]);
        }
        // currently we just use a single color
        BYTE *diffuse = (BYTE *)c;
        BYTE *screen = (BYTE *)dst.surface->pixels + sy * dst.surface->pitch + sx*4;
        screen[0] = MODULATE(tex[0], diffuse[0]);
        screen[1] = MODULATE(tex[1], diffuse[1]);
        screen[2] = MODULATE(tex[2], diffuse[2]);
        screen[3] = MODULATE(tex[3], diffuse[3]);
      }
    }
  }
  SDL_UnlockSurface(src);
  SDL_UnlockSurface(dst.surface);
  if (diffuse)
    SDL_UnlockSurface(diffuse);
}
#endif

void CGUIImage::OrientateTexture(CRect &rect, int orientation)
{
  switch (orientation & 3)
  {
  case 0:
    // default
    break;
  case 1:
    // flip in X direction
    rect.x1 = m_fU - rect.x1;
    rect.x2 = m_fU - rect.x2;
    break;
  case 2:
    // rotate 180 degrees
    rect.x1 = m_fU - rect.x1;
    rect.x2 = m_fU - rect.x2;
    rect.y1 = m_fV - rect.y1;
    rect.y2 = m_fV - rect.y2;
    break;
  case 3:
    // flip in Y direction
    rect.y1 = m_fV - rect.y1;
    rect.y2 = m_fV - rect.y2;
    break;
  }
  if (orientation & 4)
  {
    swap(rect.x1, rect.y1);
    swap(rect.x2, rect.y2);
  }
}


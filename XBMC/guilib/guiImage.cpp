#include "guiimage.h"
#include "texturemanager.h"
#include "../xbmc/settings.h"



CGUIImage::CGUIImage(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexture,DWORD dwColorKey)
:CGUIControl(dwParentID, dwControlId,dwPosX, dwPosY, dwWidth, dwHeight)
{
  m_colDiffuse	= 0xFFFFFFFF;  
  
  m_pVB=NULL;
  m_strFileName=strTexture;
  m_iTextureWidth=0;
  m_iTextureHeight=0;
  m_dwColorKey=dwColorKey;
  m_iBitmap=0;
  m_dwItems=1;
	m_iCurrentImage=0;
	m_dwFrameCounter=-1;
  m_bKeepAspectRatio=false;
  m_iCurrentLoop=0;
  m_iRenderWidth=dwWidth;
  m_iRenderHeight=dwHeight;
	m_iImageWidth = 0;
	m_iImageHeight = 0;
	m_bWasVisible = m_bVisible;
}


CGUIImage::~CGUIImage(void)
{
}

void CGUIImage::Render(DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight)
{
  if (!m_pVB) return;
  if (m_vecTextures.size()==0) return;

  CGUIImage::VERTEX* vertex=NULL;
  m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

  float x=(float)dwPosX;
  float y=(float)dwPosY;

#ifdef ALLOW_TEXTURE_COMPRESSION
	if (0==m_iImageWidth|| 0==m_iImageHeight)
	{
		D3DSURFACE_DESC desc;
		m_vecTextures[m_iCurrentImage]->GetLevelDesc(0,&desc);

		m_iImageWidth = desc.Width;
		m_iImageHeight = desc.Height;
	}

	if (0==m_iTextureWidth|| 0==m_iTextureHeight)
	{
		m_iTextureWidth  = m_iImageWidth/m_dwItems;
		m_iTextureHeight = m_iImageHeight;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#else
  if (0==m_iTextureWidth|| 0==m_iTextureWidth)
  {
    D3DSURFACE_DESC desc;
    m_vecTextures[m_iCurrentImage]->GetLevelDesc(0,&desc);

	  m_iTextureWidth  = (DWORD) desc.Width/m_dwItems;
	  m_iTextureHeight = (DWORD) desc.Height;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#endif

  if (dwWidth && m_dwItems>1)
  {
    m_iTextureWidth=m_dwWidth;
  }


  if (dwWidth==0) 
    dwWidth=m_iTextureWidth;
  if (dwHeight==0) 
    dwHeight=m_iTextureHeight;

  float nw =(float)dwWidth;
  float nh=(float)dwHeight;


	if (CalibrationEnabled())
	{
		g_graphicsContext.Correct(x, y);
	}
#ifdef ALLOW_TEXTURE_COMPRESSION
	float uoffs = float(m_iBitmap * m_dwWidth) / float(m_iImageWidth);
	float u = float(m_iTextureWidth) / float(m_iImageWidth);
	float v = float(m_iTextureHeight) / float(m_iImageHeight);

	vertex[0].p = D3DXVECTOR4( x - 0.5f,	y - 0.5f,		0, 0 );
	vertex[0].tu = uoffs;
	vertex[0].tv = 0;

	vertex[1].p = D3DXVECTOR4( x+nw - 0.5f,	y - 0.5f,		0, 0 );
	vertex[1].tu = uoffs+u;
	vertex[1].tv = 0;

	vertex[2].p = D3DXVECTOR4( x+nw - 0.5f,	y+nh - 0.5f,	0, 0 );
	vertex[2].tu = uoffs+u;
	vertex[2].tv = v;

	vertex[3].p = D3DXVECTOR4( x - 0.5f,	y+nh - 0.5f,	0, 0 );
	vertex[3].tu = uoffs;
	vertex[3].tv = v;
#else
  int iXOffset=m_iBitmap*m_dwWidth;

  vertex[0].p = D3DXVECTOR4( x - 0.5f,	y - 0.5f,		0, 0 );
  vertex[0].tu = (float)iXOffset;
  vertex[0].tv = 0;

  vertex[1].p = D3DXVECTOR4( x+nw - 0.5f,	y - 0.5f,		0, 0 );
  vertex[1].tu = (float)iXOffset+m_iTextureWidth;
  vertex[1].tv = 0;

  vertex[2].p = D3DXVECTOR4( x+nw - 0.5f,	y+nh - 0.5f,	0, 0 );
  vertex[2].tu = (float)iXOffset+m_iTextureWidth;
  vertex[2].tv = (float)m_iTextureHeight;

  vertex[3].p = D3DXVECTOR4( x - 0.5f,	y+nh - 0.5f,	0, 0 );
  vertex[3].tu = (float)iXOffset;
  vertex[3].tv = (float)m_iTextureHeight;
#endif


 
  vertex[0].col = m_colDiffuse;
	vertex[1].col = m_colDiffuse;
	vertex[2].col = m_colDiffuse;
	vertex[3].col = m_colDiffuse;
  m_pVB->Unlock();  

	Render();
	Update();
}

void CGUIImage::Render()
{
	if (!m_bVisible)
	{
		m_bWasVisible = false;
		return;
	}
	if (!m_vecTextures.size())
		return ;
	if (!m_pVB)
		return ;

	Process();

	// Set state to render the image
#ifdef ALLOW_TEXTURE_COMPRESSION
  g_graphicsContext.Get3DDevice()->SetPalette( 0, m_pPalette);
#endif
	g_graphicsContext.Get3DDevice()->SetTexture( 0, m_vecTextures[m_iCurrentImage] );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	g_graphicsContext.Get3DDevice()->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );

	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ZENABLE,      FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGENABLE,    FALSE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	g_graphicsContext.Get3DDevice()->SetRenderState( D3DRS_YUVENABLE, FALSE);
	g_graphicsContext.Get3DDevice()->SetVertexShader( FVF_VERTEX );
	// Render the image
	g_graphicsContext.Get3DDevice()->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
	g_graphicsContext.Get3DDevice()->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

	// unset the texture and palette or the texture caching crashes because the runtime still has a reference
	g_graphicsContext.Get3DDevice()->SetTexture( 0, NULL);
#ifdef ALLOW_TEXTURE_COMPRESSION
	g_graphicsContext.Get3DDevice()->SetPalette( 0, NULL);
#endif
}

void CGUIImage::OnAction(const CAction &action) 
{
}

bool CGUIImage::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUIImage::AllocResources()
{
  FreeResources();

	m_dwFrameCounter=0;
	m_iCurrentImage=0;
  m_iCurrentLoop=0;
  // Create a vertex buffer for rendering the image
  g_graphicsContext.Get3DDevice()->CreateVertexBuffer( 4*sizeof(CGUIImage::VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pVB );

  int iImages = g_TextureManager.Load(m_strFileName, m_dwColorKey);
  if (!iImages) return;
  for (int i=0; i < iImages; i++)
  {
    LPDIRECT3DTEXTURE8 pTexture;
		pTexture=g_TextureManager.GetTexture(m_strFileName,i, m_iTextureWidth,m_iTextureHeight,m_pPalette);
    m_vecTextures.push_back(pTexture);
  }

  // Set state to render the image
  Update();
}

void CGUIImage::FreeResources()
{
  if (m_pVB!=NULL)
	{
		m_pVB->Release();
		m_pVB=NULL;
	}

  for (int i=0; i < (int)m_vecTextures.size(); ++i)
  {
    g_TextureManager.ReleaseTexture(m_strFileName,i);
  }

  m_vecTextures.erase(m_vecTextures.begin(),m_vecTextures.end());
	m_iCurrentImage=0;
  m_iCurrentLoop=0;
}


void CGUIImage::Update()
{
  if (!m_pVB) return;
  if (m_vecTextures.size()==0) return;

  CGUIImage::VERTEX* vertex=NULL;
  m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

  float x=(float)m_dwPosX;
  float y=(float)m_dwPosY;
#ifdef ALLOW_TEXTURE_COMPRESSION
	if (0==m_iImageWidth|| 0==m_iImageHeight)
	{
		D3DSURFACE_DESC desc;
		m_vecTextures[m_iCurrentImage]->GetLevelDesc(0,&desc);

		m_iImageWidth = desc.Width;
		m_iImageHeight = desc.Height;
	}

  if (0==m_iTextureWidth|| 0==m_iTextureHeight)
  {
	  m_iTextureWidth  = m_iImageWidth/m_dwItems;
		m_iTextureHeight = m_iImageHeight;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#else
  D3DSURFACE_DESC desc;
  m_vecTextures[m_iCurrentImage]->GetLevelDesc(0,&desc);

  if (0==m_iTextureWidth|| 0==m_iTextureHeight)
  {
	  m_iTextureWidth  = (DWORD) desc.Width/m_dwItems;
	  m_iTextureHeight = (DWORD) desc.Height;

    if (m_iTextureHeight > (int)g_graphicsContext.GetHeight() )
        m_iTextureHeight = (int)g_graphicsContext.GetHeight();

    if (m_iTextureWidth > (int)g_graphicsContext.GetWidth() )
        m_iTextureWidth = (int)g_graphicsContext.GetWidth();
  }
#endif
  if (m_dwWidth && m_dwItems>1)
  {
    m_iTextureWidth=m_dwWidth;
  }


  if (m_dwWidth==0) 
    m_dwWidth=m_iTextureWidth;
  if (m_dwHeight==0) 
    m_dwHeight=m_iTextureHeight;


  float nw =(float)m_dwWidth;
  float nh=(float)m_dwHeight;

  if (m_bKeepAspectRatio && m_iTextureWidth && m_iTextureHeight)
  {
    int iResolution=g_stSettings.m_ScreenResolution;
    float fSourceFrameRatio = ((float)m_iTextureWidth) / ((float)m_iTextureHeight);
    float fOutputFrameRatio = fSourceFrameRatio / g_settings.m_ResInfo[iResolution].fPixelRatio; 
    if (iResolution == HDTV_1080i) fOutputFrameRatio *= 2;

    // maximize the thumbnails width
    float fNewWidth  = (float)m_dwWidth;
    float fNewHeight = fNewWidth/fOutputFrameRatio;

    if (fNewHeight > m_dwHeight)
    {
      fNewHeight = (float)m_dwHeight;
      fNewWidth = fNewHeight*fOutputFrameRatio;
    }
    // this shouldnt happen, but just make sure that everything still fits onscreen
    if (fNewWidth > m_dwWidth || fNewHeight > m_dwHeight)
    {
      fNewWidth=(float)m_dwWidth;
      fNewHeight=(float)m_dwHeight;
    }
    nw=fNewWidth;
    nh=fNewHeight;
  }


  m_iRenderWidth=(int)nw;
  m_iRenderHeight=(int)nh;

	if (CalibrationEnabled())
	{
		g_graphicsContext.Correct(x, y);
	}
#ifdef ALLOW_TEXTURE_COMPRESSION
	float uoffs = float(m_iBitmap * m_dwWidth) / float(m_iImageWidth);
	float u = float(m_iTextureWidth) / float(m_iImageWidth);
	float v = float(m_iTextureHeight) / float(m_iImageHeight);

	vertex[0].p = D3DXVECTOR4( x - 0.5f,	y - 0.5f,		0, 0 );
	vertex[0].tu = uoffs;
	vertex[0].tv = 0;
	
	vertex[1].p = D3DXVECTOR4( x+nw - 0.5f,	y - 0.5f,		0, 0 );
	vertex[1].tu = uoffs+u;
	vertex[1].tv = 0;
	
	vertex[2].p = D3DXVECTOR4( x+nw - 0.5f,	y+nh - 0.5f,	0, 0 );
	vertex[2].tu = uoffs+u;
	vertex[2].tv = v;
	
	vertex[3].p = D3DXVECTOR4( x - 0.5f,	y+nh - 0.5f,	0, 0 );
	vertex[3].tu = uoffs;
	vertex[3].tv = v;


#else
  int iXOffset=m_iBitmap*m_dwWidth;

  vertex[0].p = D3DXVECTOR4( x - 0.5f,	y - 0.5f,		0, 0 );
  vertex[0].tu = (float)iXOffset;
  vertex[0].tv = 0;

  vertex[1].p = D3DXVECTOR4( x+nw - 0.5f,	y - 0.5f,		0, 0 );
  vertex[1].tu = (float)iXOffset+m_iTextureWidth;
  vertex[1].tv = 0;

  vertex[2].p = D3DXVECTOR4( x+nw - 0.5f,	y+nh - 0.5f,	0, 0 );
  vertex[2].tu = (float)iXOffset+m_iTextureWidth;
  vertex[2].tv = (float)m_iTextureHeight;

  vertex[3].p = D3DXVECTOR4( x - 0.5f,	y+nh - 0.5f,	0, 0 );
  vertex[3].tu = (float)iXOffset;
  vertex[3].tv = (float)m_iTextureHeight;
#endif
 
  vertex[0].col = m_colDiffuse;
	vertex[1].col = m_colDiffuse;
	vertex[2].col = m_colDiffuse;
	vertex[3].col = m_colDiffuse;
  m_pVB->Unlock();  
}

bool CGUIImage::CanFocus() const
{
  return false;
}

void CGUIImage::Select(int iBitmap)
{
  m_iBitmap=iBitmap;
  Update();
}

void CGUIImage::SetItems(int iItems)
{
  m_dwItems=iItems;
}

void CGUIImage::Process()
{
	if (m_vecTextures.size() <= 1)
		return;

	if (!m_bWasVisible)
	{
		m_iCurrentLoop = 0;
		m_iCurrentImage = 0;
		m_dwFrameCounter = 0;
		m_bWasVisible = true;
		return;
	}

	m_dwFrameCounter++;
  DWORD dwDelay    = g_TextureManager.GetDelay(m_strFileName,m_iCurrentImage);
  int   iMaxLoops  = g_TextureManager.GetLoops(m_strFileName,m_iCurrentImage);
	if (!dwDelay) dwDelay=100;
	if (m_dwFrameCounter*40 >= dwDelay)
	{
		m_dwFrameCounter=0;
    if (m_iCurrentImage+1 >= (int)m_vecTextures.size() )
		{
      if (iMaxLoops > 0)
      {
        if (m_iCurrentLoop+1 < iMaxLoops)
        {
          m_iCurrentLoop++;
			    m_iCurrentImage=0;
        }
      }
      else
      {
        // 0 == loop forever
			  m_iCurrentImage=0;
      }
		}
    else
    {
		  m_iCurrentImage++;
    }
	}
}
void CGUIImage::SetTextureWidth(int iWidth)
{
	m_iTextureWidth=iWidth;
	Update();
}
void CGUIImage::SetTextureHeight(int iHeight)
{
	m_iTextureHeight=iHeight;
	Update();
}
int	CGUIImage::GetTextureWidth() const
{
	return m_iTextureWidth;
}
int CGUIImage::GetTextureHeight() const
{
	return m_iTextureHeight;
}

void CGUIImage::SetKeepAspectRatio(bool bOnOff)
{
  m_bKeepAspectRatio=bOnOff;
}
bool CGUIImage::GetKeepAspectRatio() const
{
  return m_bKeepAspectRatio;
}


int CGUIImage::GetRenderWidth() const
{
  return m_iRenderWidth;
}
int CGUIImage::GetRenderHeight() const
{
  return m_iRenderHeight;
}
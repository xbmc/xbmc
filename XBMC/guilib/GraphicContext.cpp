#include "graphiccontext.h"

#define WIDE_SCREEN_COMPENSATIONY (FLOAT)1.2
#define WIDE_SCREEN_COMPENSATIONX (FLOAT)0.85

CGraphicContext g_graphicsContext;

CGraphicContext::CGraphicContext(void)
{
	InitializeCriticalSection(&m_critSection);
  m_iScreenWidth=720;
  m_iScreenHeight=576;
	m_iScreenOffsetX=0;
	m_iScreenOffsetY=0;
  m_pd3dDevice=NULL;
  m_dwID=0;
  m_strMediaDir="D:\\media";
	m_bShowPreviewWindow=false;
}

CGraphicContext::~CGraphicContext(void)
{
	DeleteCriticalSection(&m_critSection);
}


LPDIRECT3DDEVICE8 CGraphicContext::Get3DDevice()
{
  return m_pd3dDevice;
}

int CGraphicContext::GetWidth() const
{
  return m_iScreenWidth;
}

int  CGraphicContext::GetHeight() const
{
  return m_iScreenHeight;
}

void CGraphicContext::Set(LPDIRECT3DDEVICE8 p3dDevice, int iWidth, int iHeight, int iOffsetX, int iOffsetY, bool bWideScreen)
{
	m_iScreenOffsetX=iOffsetX;
	m_iScreenOffsetY=iOffsetY;
  m_pd3dDevice=p3dDevice;
  m_iScreenWidth=iWidth;
  m_iScreenHeight=iHeight;
	m_bWidescreen=bWideScreen;
}

void CGraphicContext::SetMediaDir(const CStdString& strMediaDir)
{
  m_strMediaDir=strMediaDir;
}

const CStdString& CGraphicContext::GetMediaDir() const
{
  return m_strMediaDir;
}

void CGraphicContext::SendMessage(CGUIMessage& message)
{
  if (!m_pCallback) return;
  m_pCallback->SendMessage(message);
}

void CGraphicContext::setMessageSender(IMsgSenderCallback* pCallback)
{
  m_pCallback=pCallback;
}

DWORD CGraphicContext::GetNewID()
{
  m_dwID++;
  return  m_dwID;
}

bool CGraphicContext::IsWidescreen() const
{
	return m_bWidescreen;
}


void CGraphicContext::Correct(FLOAT& fCoordinateX, FLOAT& fCoordinateY, FLOAT& fCoordinateX2, FLOAT& fCoordinateY2)  const
{
	fCoordinateX  += (float)m_iScreenOffsetX;
	fCoordinateY  += (float)m_iScreenOffsetY;
	
	fCoordinateX2 += (float)m_iScreenOffsetX;
	fCoordinateY2 += (float)m_iScreenOffsetY;

}

void CGraphicContext::SetViewPort(float fx, float fy , float fwidth, float fheight)
{
	Correct(fx,fy,fwidth,fheight);
	D3DVIEWPORT8 newviewport;
	Get3DDevice()->GetViewport(&m_oldviewport);
	newviewport.X      = (DWORD)fx;
	newviewport.Y			 = (DWORD)fy;
	newviewport.Width  = (DWORD)(fwidth);
	newviewport.Height = (DWORD)(fheight);
	newviewport.MinZ   = 0.0f;
	newviewport.MaxZ   = 1.0f;
	Get3DDevice()->SetViewport(&newviewport);
}

void CGraphicContext::RestoreViewPort()
{
	Get3DDevice()->SetViewport(&m_oldviewport);
}

const RECT&	CGraphicContext::GetViewWindow() const
{
	return m_videoRect;
}
void CGraphicContext::SetViewWindow(const RECT&	rc) 
{
	m_videoRect.left  = rc.left;
	m_videoRect.top   = rc.top;
	m_videoRect.right = rc.right;
	m_videoRect.bottom= rc.bottom;
	if (m_bShowPreviewWindow && !m_bFullScreenVideo)
	{
		D3DRECT d3dRC;
		d3dRC.x1=rc.left;
		d3dRC.x2=rc.right;
		d3dRC.y1=rc.top;
		d3dRC.y2=rc.bottom;
		Get3DDevice()->Clear( 1, &d3dRC, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0x00010001, 1.0f, 0L );
	}
}

void CGraphicContext::SetFullScreenVideo(bool bOnOff)
{
	Lock();
	m_bFullScreenVideo=bOnOff;
	Unlock();
}

bool CGraphicContext::IsFullScreenVideo() const
{
	return m_bFullScreenVideo;
}

void CGraphicContext::SetOffset(int iXoffset, int iYoffset)
{
	m_iScreenOffsetX=iXoffset;
	m_iScreenOffsetY=iYoffset;
}
void CGraphicContext::Lock()
{
	EnterCriticalSection(&m_critSection);
}

void CGraphicContext::Unlock()
{
	LeaveCriticalSection(&m_critSection);
}

void CGraphicContext::EnablePreviewWindow(bool bEnable)
{
	m_bShowPreviewWindow=bEnable;
}
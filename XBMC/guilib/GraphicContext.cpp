#include "graphiccontext.h"


CGraphicContext g_graphicsContext;

CGraphicContext::CGraphicContext(void)
{
  m_iScreenWidth=720;
  m_iScreenHeight=576;
  m_pd3dDevice=NULL;
  m_dwID=0;
  m_strMediaDir="D:\\media";
}

CGraphicContext::~CGraphicContext(void)
{
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

void CGraphicContext::Set(LPDIRECT3DDEVICE8 p3dDevice, int iWidth, int iHeight)
{
  m_pd3dDevice=p3dDevice;
  m_iScreenWidth=iWidth;
  m_iScreenHeight=iHeight;
}

void CGraphicContext::SetMediaDir(const CStdString& strMediaDir)
{
  m_strMediaDir=strMediaDir;
}

const CStdString& CGraphicContext::GetMediaDir()
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
#ifndef GUILIB_GRAPHICCONTEXT_H
#define GUILIB_GRAPHICCONTEXT_H

#pragma once
#include "gui3d.h"
#include "GUIMessage.h"
#include "IMsgSenderCallback.h"

#include <string>
using namespace std;

class CGraphicContext
{
public:
  CGraphicContext(void);
  ~CGraphicContext(void);
  LPDIRECT3DDEVICE8 Get3DDevice();
  void              Set(LPDIRECT3DDEVICE8 p3dDevice, int iWidth, int iHeight);
  int               GetWidth() const;
  int               GetHeight() const;
  void              SendMessage(CGUIMessage& message);
  void              setMessageSender(IMsgSenderCallback* pCallback);
  DWORD             GetNewID();
  const string&     GetMediaDir();
  void              SetMediaDir(const string& strMediaDir);

protected:
  IMsgSenderCallback*     m_pCallback;
  LPDIRECT3DDEVICE8       m_pd3dDevice;
  int                     m_iScreenHeight;
  int                     m_iScreenWidth;
  DWORD                   m_dwID;
  string                  m_strMediaDir;
};

extern CGraphicContext g_graphicsContext;
#endif
#ifndef CGUILIB_GUICHECKMARK_CONTROL_H
#define CGUILIB_GUICHECKMARK_CONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "guiimage.h"
#include "stdstring.h"
using namespace std;

class CGUICheckMarkControl: public CGUIControl
{
public:
  CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureCheckMark,DWORD dwCheckWidth, DWORD dwCheckHeight,DWORD dwAlign=XBFONT_RIGHT);
  virtual ~CGUICheckMarkControl(void);
  virtual void Render();
  virtual void OnKey(const CKey& key) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void SetDisabledColor(D3DCOLOR color);

	void SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor);
	void SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor);

protected:
  CGUIImage     m_imgCheckMark;
  DWORD         m_dwTextColor	;
  CGUIFont*     m_pFont;
  wstring       m_strLabel;
  DWORD         m_dwDisabledColor;
	DWORD					m_dwAlign;  
};
#endif
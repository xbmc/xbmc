#pragma once
#include "guibuttoncontrol.h"

class CGUIRadioButtonControl :
  public CGUIButtonControl
{
public:
  CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus, const CStdString& strRadioFocus,const CStdString& strRadioNoFocus);
  virtual ~CGUIRadioButtonControl(void);
  virtual void Render();
  virtual void OnKey(const CKey& key) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources();
	const	CStdString& GetTexutureRadioFocusName() const { return m_imgRadioFocus.GetFileName(); };
	const	CStdString& GetTexutureRadioNoFocusName() const { return m_imgRadioNoFocus.GetFileName(); };
protected:
  virtual void       Update();
  CGUIImage    m_imgRadioFocus;
  CGUIImage    m_imgRadioNoFocus;
};

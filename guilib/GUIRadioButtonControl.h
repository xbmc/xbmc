/*!
	\file GUIRadioButtonControl.h
	\brief 
	*/

#pragma once
#include "guibuttoncontrol.h"

/*!
	\ingroup controls
	\brief 
	*/
class CGUIRadioButtonControl :
  public CGUIButtonControl
{
public:
  CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId, 
	  int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
	  const CStdString& strTextureFocus,const CStdString& strTextureNoFocus,
	  DWORD dwTextOffsetX, DWORD dwTextOffsetY,
	  const CStdString& strRadioFocus,const CStdString& strRadioNoFocus);

  virtual ~CGUIRadioButtonControl(void);
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
	virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
	const	CStdString& GetTexutureRadioFocusName() const { return m_imgRadioFocus.GetFileName(); };
	const	CStdString& GetTexutureRadioNoFocusName() const { return m_imgRadioNoFocus.GetFileName(); };
protected:
  virtual void       Update();
  CGUIImage    m_imgRadioFocus;
  CGUIImage    m_imgRadioNoFocus;
};

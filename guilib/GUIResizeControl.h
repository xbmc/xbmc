/*!
	\file GUIRESIZEControl.h
	\brief 
	*/

#ifndef GUILIB_GUIRESIZECONTROL_H
#define GUILIB_GUIRESIZECONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guiimage.h"
#include "stdstring.h"
using namespace std;

#define DIRECTION_NONE	0
#define DIRECTION_UP	1
#define DIRECTION_DOWN	2
#define DIRECTION_LEFT	3
#define DIRECTION_RIGHT	4

/*!
	\ingroup controls
	\brief 
	*/
class CGUIResizeControl : public CGUIControl
{
public:
	CGUIResizeControl(DWORD dwParentID, DWORD dwControlId,
		int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
		const CStdString& strTextureFocus,const CStdString& strTextureNoFocus);

	virtual ~CGUIResizeControl(void);
	  
	virtual void		Render();
	virtual void		OnAction(const CAction &action);
	virtual void		OnUp();
	virtual void		OnDown();
	virtual void		OnLeft();
	virtual void		OnRight();
	virtual void		OnMouseDrag();
	virtual void		OnMouseClick(DWORD dwButton);
	virtual void		PreAllocResources();
	virtual void		AllocResources();
	virtual void		FreeResources();
	virtual void		SetPosition(int iPosX, int iPosY);
	virtual void		SetAlpha(DWORD dwAlpha);
	virtual void		SetColourDiffuse(D3DCOLOR colour);
	const CStdString&	GetTextureFocusName() const { return m_imgFocus.GetFileName(); };
	const CStdString&	GetTextureNoFocusName() const { return m_imgNoFocus.GetFileName(); };
	void				SetLimits(int iX1, int iY1, int iX2, int iY2);
	virtual void		EnableCalibration(bool bOnOff);

protected:
	virtual void		Update() ;
	void				UpdateSpeed(int nDirection);
	void				Resize(int iX, int iY);
	CGUIImage			m_imgFocus;
	CGUIImage			m_imgNoFocus;
	DWORD				m_dwFrameCounter;
	DWORD				m_dwLastMoveTime;
	int					m_nDirection;
	float				m_fSpeed;
	float				m_fAnalogSpeed;
	float				m_fMaxSpeed;
	float				m_fAcceleration;
	int					m_iX1, m_iX2, m_iY1, m_iY2;
};
#endif

/*!
	\file GUISliderControl.h
	\brief 
	*/

#ifndef GUILIB_GUIsliderCONTROL_H
#define GUILIB_GUIsliderCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guiImage.h"
#include "stdstring.h"
using namespace std;

#define SPIN_CONTROL_TYPE_INT    1
#define SPIN_CONTROL_TYPE_FLOAT  2
#define SPIN_CONTROL_TYPE_TEXT   3

/*!
	\ingroup controls
	\brief 
	*/
class CGUISliderControl :
  public CGUIControl
{
public:
	CGUISliderControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strBackGroundTexture,const CStdString& strMidTexture,const CStdString& strMidTextureFocus,int iType);
	virtual ~CGUISliderControl(void);
	virtual void Render();
	virtual bool CanFocus() const;  
	virtual void 	OnAction(const CAction &action);
	virtual void PreAllocResources();
	virtual void AllocResources();
	virtual void FreeResources();
	virtual void SetRange(int iStart, int iEnd);
	virtual void SetFloatRange(float fStart, float fEnd);
	virtual bool OnMessage(CGUIMessage& message);
	void				SetPercentage(int iPercent);
	int 				GetPercentage() const;
	void				SetIntValue(int iValue);
	int 				GetIntValue() const;
	void				SetFloatValue(float fValue);
	float 				GetFloatValue() const;
	void				SetFloatInterval(float fInterval);
	int 				GetType() const;
	const CStdString& GetBackGroundTextureName() const { return m_guiBackground.GetFileName();};
	const CStdString& GetBackTextureMidName() const { return m_guiMid.GetFileName();};
  	DWORD	GetControlOffsetX() const { return m_dwControlOffsetX;};
	DWORD	GetControlOffsetY() const { return m_dwControlOffsetY;};
  	void	SetControlOffsetX(DWORD dwControlOffsetX) { m_dwControlOffsetX=dwControlOffsetX;};
	void	SetControlOffsetY(DWORD dwControlOffsetY) { m_dwControlOffsetY=dwControlOffsetY;};

protected:
	virtual void	Update() ;
	CGUIImage		m_guiBackground;
	CGUIImage		m_guiMid;
	CGUIImage		m_guiMidFocus;
	int				m_iPercent;
	int				m_iType;
	int				m_iStart;
	int				m_iEnd;
	float			m_fStart;
	float			m_fEnd;
	int				m_iValue;
	float			m_fValue;
	float			m_fInterval;
	DWORD			m_dwControlOffsetX;
	DWORD			m_dwControlOffsetY;
};
#endif

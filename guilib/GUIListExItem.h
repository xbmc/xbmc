/*!
	\file GUIListExItem.h
	\brief 
	*/

#ifndef GUILIB_GUIListExItem_H
#define GUILIB_GUIListExItem_H

#pragma once
#include "GUIButtonControl.h"
#include "GUIItem.h"

/*!
	\ingroup controls
	\brief 
	*/
class CGUIListExItem : public CGUIItem
{
public:
	class RenderContext : public CGUIItem::RenderContext
	{
	public:
		RenderContext()
		{
			m_pButton = NULL;
			m_pFont = NULL;
			m_bActive = FALSE;
			m_dwTextNormalColour = m_dwTextSelectedColour = 0x00000000;
		};
		virtual ~RenderContext(){};

		CGUIButtonControl*	m_pButton;
		CGUIFont*			m_pFont;
		DWORD				m_dwTextNormalColour;
		DWORD				m_dwTextSelectedColour;
		bool				m_bActive;
	};
	
	CGUIListExItem(CStdString& aItemName);
	virtual ~CGUIListExItem(void);
	virtual void OnPaint(CGUIItem::RenderContext* pContext);
	void SetIcon(CGUIImage* pImage);
	void SetIcon(INT aWidth, INT aHeight, const CStdString& aTexture);
	DWORD GetFramesFocused() { return m_dwFocusedDuration; };

protected:
	void RenderText(FLOAT fPosX, FLOAT fPosY, FLOAT fMaxWidth, DWORD dwTextColor, WCHAR* wszText, CGUIFont* pFont);
protected:
	CGUIImage*	m_pIcon;
	DWORD		m_dwFocusedDuration;
	DWORD		m_dwFrameCounter;
};
#endif

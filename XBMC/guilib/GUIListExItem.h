/*!
	\file GUIListExItem.h
	\brief 
	*/

#ifndef GUILIB_GUIListExItem_H
#define GUILIB_GUIListExItem_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "guiimage.h"
#include "guiButtonControl.h"
#include "guiitem.h"
#include "stdstring.h"
using namespace std;

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
			m_dwTextNormalColour = m_dwTextSelectedColour = 0x00000000;
		};
		virtual ~RenderContext(){};

		CGUIButtonControl*	m_pButton;
		CGUIFont*			m_pFont;
		DWORD				m_dwTextNormalColour;
		DWORD				m_dwTextSelectedColour;
	};
	
	CGUIListExItem(CStdString& aItemName);
	virtual void OnPaint(CGUIItem::RenderContext* pContext);
	void SetIcon(CGUIImage* pImage);
	void SetIcon(INT aWidth, INT aHeight, const CStdString& aTexture);

protected:
	void RenderText(FLOAT fPosX, FLOAT fPosY, FLOAT fMaxWidth, DWORD dwTextColor, WCHAR* wszText, CGUIFont* pFont);
protected:
	CGUIImage* m_pIcon;
};
#endif

/*!
	\file GUIItem.h
	\brief 
	*/

#ifndef GUILIB_GUIItem_H
#define GUILIB_GUIItem_H

#pragma once
#include "stdstring.h"
using namespace std;

class CGUIItem
{
public:
	class RenderContext
	{
	public:
		RenderContext()
		{
			m_iPositionX = m_iPositionY = 0;
			m_bFocused = false;
		};
		virtual ~RenderContext(){};

		int m_iPositionX;
		int m_iPositionY;
		bool  m_bFocused;
	};
	
	CGUIItem(CStdString& aItemName);
	virtual ~CGUIItem(void);
	virtual void OnPaint(CGUIItem::RenderContext* pContext)=0;
	virtual void GetDisplayText(CStdString& aString)
	{
		aString = m_strName;
	};

	CStdString GetName();
	void SetCookie(DWORD aCookie);
	DWORD GetCookie();

protected:
	DWORD			m_dwCookie;
	CStdString		m_strName;
};
#endif

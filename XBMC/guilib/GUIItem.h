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
			m_dwPositionX = m_dwPositionY = 0;
			m_bFocused = false;
		};
		virtual ~RenderContext(){};

		DWORD m_dwPositionX;
		DWORD m_dwPositionY;
		bool  m_bFocused;
	};
	
	CGUIItem(CStdString& aItemName);
	virtual void OnPaint(CGUIItem::RenderContext* pContext)=0;

	CStdString GetName();
	void SetCookie(DWORD aCookie);
	DWORD GetCookie();

protected:
	DWORD			m_dwCookie;
	CStdString		m_strName;
};
#endif

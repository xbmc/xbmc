/*!
	\file GUIRAMControl.h
	\brief 
	*/

#ifndef GUILIB_GUIRAMControl_H
#define GUILIB_GUIRAMControl_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "guiimage.h"
#include "stdstring.h"
#include "..\XBMC\Utils\MediaMonitor.h"

#include <vector>

using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUIRAMControl : public CGUIControl, public IMediaObserver
{
public:

	struct Movie
	{
		CStdString	strFilepath;
		CStdString	strTitle;
		CGUIImage*	pImage;
	};

	typedef std::vector<Movie> RECENTMOVIES;

	CGUIRAMControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFontName, const CStdString& strFont2Name, D3DCOLOR dwTitleColor, D3DCOLOR dwNormalColor);
	virtual ~CGUIRAMControl(void);
  
	virtual void		Render();

	virtual void OnMediaUpdate(	INT nIndex, CStdString& strFilepath,
								CStdString& strTitle, CStdString& strImagePath);

	DWORD				GetTitleTextColor() const { return m_dwTitleColor;};
	DWORD				GetNormalTextColor() const { return m_dwTextColor;};
	const CStdString&	GetFontName() const { return m_pFont->GetFontName(); };
	const CStdString&	GetFont2Name() const { return m_pFont2->GetFontName(); };

protected:

	CMediaMonitor*	m_pMonitor;
	RECENTMOVIES	m_movies;
	FLOAT			m_fFontHeight;
	FLOAT			m_fFont2Height;

	DWORD			m_dwThumbnailWidth;
	DWORD			m_dwThumbnailHeight;
	DWORD			m_dwThumbnailSpaceX;
	DWORD			m_dwThumbnailSpaceY;
	DWORD			m_dwTextSpaceY;

	D3DCOLOR		m_dwTitleColor;
	D3DCOLOR		m_dwTextColor;
	CGUIFont*		m_pFont;
	CGUIFont*		m_pFont2;
};
#endif

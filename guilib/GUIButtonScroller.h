#pragma once

#include "GUIImage.h"

class CButton
{
public:
	wstring			strLabel;
	CStdString	strExecute;
	int					iIcon;
};

class CGUIButtonScroller :
	public CGUIControl
{
public:
	CGUIButtonScroller(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, int iGap, int iSlots, int iDefaultSlot, int iMovementRange, bool bHorizontal, int iAlpha, bool bWrapAround, bool bSmoothScrolling, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus, int iTextXOffset, int iTextYOffset, DWORD dwAlign=XBFONT_LEFT);
	virtual ~CGUIButtonScroller(void);
  virtual void    OnAction(const CAction &action);
	virtual bool		OnMessage(CGUIMessage &message);
	virtual void		OnUp();
	virtual void		OnLeft();
	virtual void		OnRight();
	virtual void		OnDown();
  virtual void		OnMouseOver();
  virtual void		OnMouseClick(DWORD dwButton);
  virtual void		OnMouseWheel();
	virtual void		Render();
	virtual void		PreAllocResources();
	virtual void		AllocResources();
	virtual void		FreeResources();
	void						ClearButtons();
	void						AddButton(const wstring &strLabel, const CStdString &strExecute, const int iIcon);
	void						SetActiveButton(int iButton);
	int							GetActiveButton();
	void						SetFont(const CStdString &strFont, DWORD dwColor);
	CStdString			GetTextureFocusName() const { return m_imgFocus.GetFileName(); };
	CStdString			GetTextureNoFocusName() const { return m_imgNoFocus.GetFileName(); };
	const char *		GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
	DWORD						GetTextColor() const { return m_dwTextColor; };
	DWORD						GetTextAlign() const { return m_dwTextAlignment; };
	int							GetTextOffsetX() const { return m_iTextOffsetX; };
	int							GetTextOffsetY() const { return m_iTextOffsetY; };
	int							GetButtonGap() const { return m_iButtonGap; };
	int							GetNumSlots() const { return m_iNumSlots; };
	int							GetDefaultSlot() const { return m_iDefaultSlot+1; };
	int							GetMovementRange() const { return m_iMovementRange; };
	bool						GetHorizontal() const { return m_bHorizontal; };
	int							GetAlpha() const { return m_iAlpha; };
	bool						GetWrapAround() const { return m_bWrapAround; };
	bool						GetSmoothScrolling() const { return m_bSmoothScrolling; };

protected:
	void						OnChangeFocus();
	int							GetNext(int iCurrent);
	int							GetPrevious(int iCurrent);
	int							GetButton(int iOffset);
	void						DoUp();
	void						DoDown();
	void						RenderItem(int &iPosX, int &iPosY, int &iOffset, bool bText);
	void						Update();
	void						GetScrollZone(float &fStartAlpha, float &fEndAlpha);
private:
	// saved variables from the xml (as this control is user editable...)
	int m_iXMLNumSlots;
	int m_iXMLDefaultSlot;
	int m_iXMLPosX;
	int m_iXMLPosY;
	int m_dwXMLWidth;
	int m_dwXMLHeight;

	int m_iButtonGap;					// gap between buttons
	int m_iNumSlots;					// number of button slots available
	int m_iDefaultSlot;				// default highlight position
	int m_iMovementRange;			// amoung that we can move the highlight
	bool m_bHorizontal;				// true if it's a horizontal button bar
	int m_iAlpha;							// amount of alpha (0..100)
	bool m_bWrapAround;				// whether the buttons wrap around or not
	bool m_bSmoothScrolling;	// whether there is smooth scrolling or not

	int m_iCurrentSlot;				// currently highlighted slot

	int m_iOffset;						// first item in the list
	int m_iScrollOffset;			// offset when scrolling
	bool m_bScrollUp;					// true if we're scrolling up (or left)
	bool m_bScrollDown;				// true if scrolling down (or right)
	bool m_bMoveUp;						// true if we're scrolling up (or left)
	bool m_bMoveDown;					// true if scrolling down (or right)
	float m_fScrollSpeed;			// speed of scrolling
	// stuff we need for the buttons...
	vector<CButton*>						m_vecButtons;
	typedef vector<CButton*>::iterator ivecButtons;
  CGUIImage						m_imgFocus;
  CGUIImage						m_imgNoFocus;  
//  DWORD								m_dwFrameCounter;
	CGUIFont*						m_pFont;
	D3DCOLOR						m_dwTextColor;
  int									m_iTextOffsetX;
  int									m_iTextOffsetY;
	DWORD								m_dwTextAlignment;
	int									m_iSlowScrollCount;
};

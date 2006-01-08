/*!
\file GUIRAMControl.h
\brief 
*/

#ifndef GUILIB_GUIRAMControl_H
#define GUILIB_GUIRAMControl_H

#pragma once
#include "GUIButtonControl.h"
#include "..\XBMC\Utils\MediaMonitor.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIRAMControl : public CGUIControl, public IMediaObserver
{
public:

  class Movie
  {
  public:
    Movie()
    {
      bValid = false;
    }

    CStdString strFilepath;
    CStdString strTitle;
    CGUIImage* pImage;
    int nAlpha;
    bool bValid;
  };

  CGUIRAMControl(DWORD dwParentID, DWORD dwControlId,
                 int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                 const CLabelInfo& labelInfo, const CLabelInfo& titleInfo);

  virtual ~CGUIRAMControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);

  virtual void OnMediaUpdate( INT nIndex, CStdString& strFilepath,
                              CStdString& strTitle, CStdString& strImagePath);

  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();

  const CLabelInfo& GetTitleInfo() const { return m_title; };
  const CLabelInfo& GetLabelInfo() const { return m_label; };

  void SetTextSpacing(DWORD dwTextSpaceY) { m_dwTextSpaceY = dwTextSpaceY; };
  DWORD GetTextSpacing() const { return m_dwTextSpaceY; };

  void SetThumbAttributes(DWORD dwThumbWidth, DWORD dwThumbHeight, DWORD dwThumbSpaceX, DWORD dwThumbSpaceY, CStdString& strDefaultThumb)
  {
    m_dwThumbnailWidth = dwThumbWidth;
    m_dwThumbnailHeight = dwThumbHeight;
    m_dwThumbnailSpaceX = dwThumbSpaceX;
    m_dwThumbnailSpaceY = dwThumbSpaceY;
    m_strDefaultThumb = strDefaultThumb;
  };

  void GetThumbAttributes(DWORD& dwThumbWidth, DWORD& dwThumbHeight, DWORD& dwThumbSpaceX, DWORD& dwThumbSpaceY, CStdString& strDefaultThumb) const
  {
    dwThumbWidth = m_dwThumbnailWidth;
    dwThumbHeight = m_dwThumbnailHeight;
    dwThumbSpaceX = m_dwThumbnailSpaceX;
    dwThumbSpaceY = m_dwThumbnailSpaceY;
    strDefaultThumb = m_strDefaultThumb;
  };

protected:

  void UpdateAllTitles();
  void UpdateTitle(CStdString& strFilepath, INT nIndex);
  void PlayMovie(CFileItem& item);

  CMediaMonitor* m_pMonitor;
  Movie m_current[RECENT_MOVIES];
  Movie m_new[RECENT_MOVIES];
  CGUIButtonControl* m_pTextButton[RECENT_MOVIES];
  INT m_iSelection;
  DWORD m_dwCounter;

  FLOAT m_fFontHeight;
  FLOAT m_fFont2Height;

  DWORD m_dwThumbnailWidth;
  DWORD m_dwThumbnailHeight;
  DWORD m_dwThumbnailSpaceX;
  DWORD m_dwThumbnailSpaceY;
  DWORD m_dwTextSpaceY;
  CStdString m_strDefaultThumb;

  CLabelInfo m_title;
  CLabelInfo m_label;
};
#endif

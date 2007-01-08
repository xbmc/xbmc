/*!
\file GUIRAMControl.h
\brief 
*/

#ifdef HAS_RAM_CONTROL
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
                 float posX, float posY, float width, float height,
                 const CLabelInfo& labelInfo, const CLabelInfo& titleInfo);

  virtual ~CGUIRAMControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action);

  virtual void OnMediaUpdate( INT nIndex, CStdString& strFilepath,
                              CStdString& strTitle, CStdString& strImagePath);

  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();

  void SetTextSpacing(float textSpaceY) { m_textSpaceY = textSpaceY; };

  void SetThumbAttributes(float thumbWidth, float thumbHeight, float thumbSpaceX, float thumbSpaceY, CStdString& strDefaultThumb)
  {
    m_thumbnailWidth = thumbWidth;
    m_thumbnailHeight = thumbHeight;
    m_thumbnailSpaceX = thumbSpaceX;
    m_thumbnailSpaceY = thumbSpaceY;
    m_strDefaultThumb = strDefaultThumb;
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

  float m_thumbnailWidth;
  float m_thumbnailHeight;
  float m_thumbnailSpaceX;
  float m_thumbnailSpaceY;
  float m_textSpaceY;
  CStdString m_strDefaultThumb;

  CLabelInfo m_title;
  CLabelInfo m_label;
};
#endif
#endif

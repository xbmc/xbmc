#pragma once

#include "GUIWindow.h"
#include "Utils/Thread.h"
#include "SlideShowPicture.h"

class CGUIWindowSlideShow;

class CBackgroundPicLoader : public CThread
{
public:
  CBackgroundPicLoader();
  ~CBackgroundPicLoader();

  void Create(CGUIWindowSlideShow *pCallback);
  void LoadPic(int iPic, int iSlideNumber, const CStdString &strFileName, const int maxWidth, const int maxHeight);
  bool IsLoading() { return m_bLoadPic;};

private:
  void Process();
  int m_iPic;
  int m_iSlideNumber;
  CStdString m_strFileName;
  int m_maxWidth;
  int m_maxHeight;
  bool m_bLoadPic;
  CGUIWindowSlideShow *m_pCallback;
};

class CGUIWindowSlideShow : public CGUIWindow
{
public:
  CGUIWindowSlideShow(void);
  virtual ~CGUIWindowSlideShow(void);

  void Reset();
  void Add(const CStdString& strPicture);
  bool IsPlaying() const;
  void ShowNext();
  void ShowPrevious();
  void Select(const CStdString& strPicture);
  vector<CStdString> GetSlideShowContents();
  CStdString GetCurrentSlide();
  bool GetCurrentSlideInfo(int &width, int &height);
  void RunSlideShow(const CStdString& strPath, bool bRecursive = false);
  void StartSlideShow();
  bool InSlideShow() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void FreeResources();
  void OnLoadPic(int iPic, int iSlideNumber, D3DTexture *pTexture, int iWidth, int iHeight, int iOriginalWidth, int iOriginalHeight, int iRotate, bool bFullSize);
  int NumSlides();
private:
  void AddItems(const CStdString &strPath, bool bRecursive);
  void RenderPause();
  void RenderErrorMessage();
  void Rotate();
  void Zoom(int iZoom);
  void Move(float fX, float fY);
  void Shuffle();

  int m_iCurrentSlide;
  int m_iNextSlide;
  int m_iRotate;
  int m_iZoomFactor;

  bool m_bSlideShow;
  bool m_bPause;
  bool m_bErrorMessage;

  vector<CStdString> m_vecSlides;
  typedef vector<CStdString>::iterator ivecSlides;

  CSlideShowPic m_Image[2];
  int m_iCurrentPic;
  // background loader
  CBackgroundPicLoader* m_pBackgroundLoader;
  bool m_bWaitForNextPic;
  bool m_bLoadNextPic;
  bool m_bReloadImage;

};

#pragma once

#include "GUIWindow.h"
#include "GUIImage.h"
#include "Utils/Thread.h"

enum DISPLAY_EFFECT { EFFECT_NONE = 0, EFFECT_FLOAT, EFFECT_ZOOM, EFFECT_RANDOM, EFFECT_NO_TIMEOUT };
enum TRANSISTION_EFFECT { TRANSISTION_NONE = 0, FADEIN_FADEOUT, CROSSFADE, TRANSISTION_ZOOM, TRANSISTION_ROTATE };

struct TRANSISTION
{
  TRANSISTION_EFFECT type;
  int start;
  int length;
};

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

class CSlideShowPic
{
public:
  CSlideShowPic();
  ~CSlideShowPic();

  void SetTexture(int iSlideNumber, D3DTexture *pTexture, int iWidth, int iHeight, int iRotate, DISPLAY_EFFECT dispEffect = EFFECT_RANDOM, TRANSISTION_EFFECT transEffect = FADEIN_FADEOUT);
  void UpdateTexture(D3DTexture *pTexture, int iWidth, int iHeight);
  bool IsLoaded() const { return m_bIsLoaded;};
  void UnLoad() {m_bIsLoaded = false;};
  void Render();
  void Close();
  bool IsFinished() const { return m_bIsFinished;};
  bool DrawNextImage() const { return m_bDrawNextImage;};

  int GetWidth() const { return (int)m_fWidth;};
  int GetHeight() const { return (int)m_fHeight;};

  void Keep();
  void StartTransistion();
  int GetTransistionTime(int iType) const;
  void SetTransistionTime(int iType, int iTime);

  int SlideNumber() const { return m_iSlideNumber;};

  void Zoom(int iZoomAmount);
  void Rotate(int iRotateAmount);
  void Pause(bool bPause) { if (!m_bDrawNextImage) m_bPause = bPause;};

  void SetOriginalSize(int iOriginalWidth, int iOriginalHeight, bool bFullSize);
bool FullSize() const { return m_bFullSize;};
  int GetOriginalWidth();
  int GetOriginalHeight();

  void Move(float dX, float dY);
  float GetZoom() const { return m_fZoomAmount;};

private:
  void Process();
  void Render(float *x, float *y, D3DTexture *pTexture, DWORD dwColor, _D3DFILLMODE fillmode = D3DFILL_SOLID );

  struct VERTEX
  {
    D3DXVECTOR4 p;
    D3DCOLOR col;
    FLOAT tu, tv;
  };
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

  int m_iOriginalWidth;
  int m_iOriginalHeight;
  int m_iSlideNumber;
  bool m_bIsLoaded;
  bool m_bIsFinished;
  bool m_bDrawNextImage;
  CStdString m_strFileName;
  IDirect3DTexture8* m_pImage;
  float m_fWidth;
  float m_fHeight;
  DWORD m_dwAlpha;
  // stuff relative to middle position
  float m_fPosX;
  float m_fPosY;
  float m_fPosZ;
  float m_fVelocityX;
  float m_fVelocityY;
  float m_fVelocityZ;
  float m_fZoomAmount;
  float m_fZoomLeft;
  float m_fZoomTop;
  // transistion and display effects
  DISPLAY_EFFECT m_displayEffect;
  TRANSISTION m_transistionStart;
  TRANSISTION m_transistionEnd;
  TRANSISTION m_transistionTemp; // used for rotations + zooms
  float m_fAngle; // angle (between 0 and 2pi to display the image)
  float m_fTransistionAngle;
  float m_fTransistionZoom;
  int m_iCounter;
  int m_iTotalFrames;
  bool m_bPause;
  bool m_bNoEffect;
  bool m_bFullSize;
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
  bool m_bShowInfo;
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

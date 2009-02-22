//
// C++ Interface: karaokewindowbackground
//
// Description: 
//
//
// Author: Team XBMC <>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KARAOKEWINDOWBACKGROUND_H
#define KARAOKEWINDOWBACKGROUND_H

class CGUIWindow;
class CGUIImage;
class CGUIVisualisationControl;
class CImage;

class CKaraokeWindowBackground
{
public:
  CKaraokeWindowBackground();
  ~CKaraokeWindowBackground();

  virtual void Init(CGUIWindow * wnd);

  // Start a specific background
  virtual void StartEmpty();
  virtual void StartVisualisation();
  virtual void StartImage();

  virtual void Stop();

  // Forwarders
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();

private:
  enum BackgroundMode
  {
      BACKGROUND_NONE,
      BACKGROUND_VISUALISATION,
      BACKGROUND_IMAGE,
      BACKGROUND_VIDEO
  };

  // for visualization background
  CGUIVisualisationControl * m_VisControl;
  CGUIImage                * m_ImgControl;

  BackgroundMode             m_mode;

  CGUIWindow               * m_parentWindow;

  CImage                   * m_Image;
};

#endif

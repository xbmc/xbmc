#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIWindow.h"
#include "utils/CriticalSection.h"
#include "GUIDialogSlider.h"

class CGUITextLayout; // forward

class CGUIWindowFullScreen :
      public CGUIWindow, public ISliderCallback
{
public:
  CGUIWindowFullScreen(void);
  virtual ~CGUIWindowFullScreen(void);
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void FrameMove();
  virtual void Render();
  virtual void OnWindowLoaded();
  void ChangetheTimeCode(int remote);

  virtual void OnSliderChange(void *data, CGUISliderControl *slider);
protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual void OnDeinitWindow(int nextWindow) {}; // no out window animation for fullscreen video

private:
  void RenderTTFSubtitles();
  void SeekChapter(int iChapter);
  void ToggleOSD();

  enum SEEK_TYPE { SEEK_ABSOLUTE, SEEK_RELATIVE };
  enum SEEK_DIRECTION { SEEK_FORWARD, SEEK_BACKWARD };

  /*! \brief Seek to the current time code stamp, either relative or absolute
   \param type - whether the seek is absolute or relative
   \param direction - if relative seeking, which direction to seek
   */
  void SeekToTimeCodeStamp(SEEK_TYPE type, SEEK_DIRECTION direction = SEEK_FORWARD);

  /*! \brief Convert the current timecode into a time in seconds to seek
   */
  double GetTimeCodeStamp();

  /*! \brief pop up a slider dialog for a particular action
   \param action id of the action the slider responds to
   \param label id of the label to display
   \param value value to set on the slider
   \param min minimum value the slider may take
   \param delta change value to advance the slider by with each click
   \param max maximal value the slider may take
   \param modal true if we should wait for the slider to finish. Defaults to false
   */
  void ShowSlider(int action, int label, float value, float min, float delta, float max, bool modal = false);

  bool m_bShowViewModeInfo;
  unsigned int m_dwShowViewModeTimeout;

  bool m_bShowCurrentTime;

  bool m_timeCodeShow;
  unsigned int m_timeCodeTimeout;
  int m_timeCodeStamp[6];
  int m_timeCodePosition;
  
  int m_sliderAction; ///< \brief set to the action id for a slider being displayed \sa ShowSlider

  CCriticalSection m_fontLock;
  CGUITextLayout* m_subsLayout;
};

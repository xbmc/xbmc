#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/ISliderCallback.h"
#include "input/Key.h"

/*! \brief Player controller class to handle user actions.

 Handles actions that are normally suited to fullscreen playback, such as
 altering subtitles and audio tracks, changing aspect ratio, subtitle placement,
 and placement of the video on screen.
 */
class CPlayerController : public ISliderCallback
{
public:
  CPlayerController();
  virtual ~CPlayerController();

  /*! \brief Perform a player control action if appropriate.
  \param action the action to perform.
  \return true if the action is considered handled, false if it should be handled elsewhere.
  */
  bool OnAction(const CAction &action);

  /*! \brief Callback from the slider dialog.
   \sa CGUIDialogSlider
   */
  virtual void OnSliderChange(void *data, CGUISliderControl *slider);

private:
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

  int m_sliderAction; ///< \brief set to the action id for a slider being displayed \sa ShowSlider
};

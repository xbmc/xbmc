/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/ISliderCallback.h"
#include "input/actions/interfaces/IActionListener.h"
#include "utils/MovingSpeed.h"

/*! \brief Player controller class to handle user actions.

 Handles actions that are normally suited to fullscreen playback, such as
 altering subtitles and audio tracks, changing aspect ratio, subtitle placement,
 and placement of the video on screen.
 */
class CPlayerController : public ISliderCallback, public KODI::ACTION::IActionListener
{
public:
  static CPlayerController& GetInstance();

  /*! \brief Perform a player control action if appropriate.
  \param action the action to perform.
  \return true if the action is considered handled, false if it should be handled elsewhere.
  */
  bool OnAction(const CAction &action) override;

  /*! \brief Callback from the slider dialog.
   \sa CGUIDialogSlider
   */
  void OnSliderChange(void *data, CGUISliderControl *slider) override;

protected:
  CPlayerController();
  CPlayerController(const CPlayerController&) = delete;
  CPlayerController& operator=(CPlayerController const&) = delete;
  ~CPlayerController() override;

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

  int m_sliderAction = 0; ///< \brief set to the action id for a slider being displayed \sa ShowSlider
  UTILS::MOVING_SPEED::CMovingSpeed m_movingSpeed;
};

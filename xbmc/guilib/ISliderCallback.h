/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CGUISliderControl;

/*!
 \brief Interface class for callback from the slider dialog

 Used to pass feedback from the slider dialog to a caller.  Users of the
 slider dialog should derive from this class if they wish to respond to changes
 in the slider by the user as they happen.  OnSliderChange is called in response
 to the user moving the slider.  The caller may then update the text on the slider
 and update anything that should be changed as the slider is adjusted.

 \sa CGUIDialogSlider
 */
class ISliderCallback
{
public:
  virtual ~ISliderCallback() = default;

  /*!
   \brief Callback function called whenever the user moves the slider

   \param data pointer of callbackData
   \param slider pointer to the slider control
   */
  virtual void OnSliderChange(void *data, CGUISliderControl *slider) = 0;
};

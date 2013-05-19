#pragma once

/*
 *      Copyright (C) 2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

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
  virtual ~ISliderCallback() {}
  
  /*!
   \brief Callback function called whenever the user moves the slider

   \param data pointer of callbackData
   \param slider pointer to the slider control
   */
  virtual void OnSliderChange(void *data, CGUISliderControl *slider) = 0;
};

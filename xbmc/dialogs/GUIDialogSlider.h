#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "guilib/GUIDialog.h"
#include "guilib/ISliderCallback.h"

class CGUIDialogSlider : public CGUIDialog
{
public:
  CGUIDialogSlider();
  virtual ~CGUIDialogSlider(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  /*! \brief Show the slider dialog and wait for the user to change the value
   Shows the slider until the user is happy with the adjusted value.  Calls back with each change to the callback function
   allowing changes to take place immediately.
   \param label description of what is being changed by the slider
   \param value start value of the slider
   \param min minimal value the slider may take
   \param delta amount the slider advances for a single click
   \param max maximal value the slider may take
   \param callback callback class that implements ISliderCallback::OnSliderChange
   \param callbackData pointer to callback-specific data (defaults to NULL)
   \sa ISliderCallback, Display
   */
  static void ShowAndGetInput(const std::string &label, float value, float min, float delta, float max, ISliderCallback *callback, void *callbackData = NULL);

  /*! \brief Show the slider dialog as a response to user input
   Shows the slider with the given values for a short period of time, used for UI feedback of a set user action.
   This function is asynchronous.
   \param label id of the description label for the slider
   \param value start value of the slider
   \param min minimal value the slider may take
   \param delta amount the slider advances for a single click
   \param max maximal value the slider may take
   \param callback callback class that implements ISliderCallback::OnSliderChange
   \sa ISliderCallback, ShowAndGetInput
   */
  static void Display(int label, float value, float min, float delta, float max, ISliderCallback *callback);
protected:
  void SetSlider(const std::string &label, float value, float min, float delta, float max, ISliderCallback *callback, void *callbackData);
  virtual void OnWindowLoaded();

  ISliderCallback *m_callback;
  void *m_callbackData;
};


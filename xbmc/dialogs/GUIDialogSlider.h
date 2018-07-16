/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "guilib/ISliderCallback.h"

class CGUIDialogSlider : public CGUIDialog
{
public:
  CGUIDialogSlider();
  ~CGUIDialogSlider(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;

  void SetModalityType(DialogModalityType type);

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
  void OnWindowLoaded() override;

  ISliderCallback *m_callback;
  void *m_callbackData;
};


/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogBoxBase.h"
#include "utils/Variant.h"

namespace KODI
{
  namespace MESSAGING
  {
    namespace HELPERS
    {
      struct DialogYesNoMessage;
    }
  }
}

class CGUIDialogYesNo :
      public CGUIDialogBoxBase
{
public:
  explicit CGUIDialogYesNo(int overrideId = -1);
  ~CGUIDialogYesNo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;

  void Reset();

  enum DialogResult
  {
    DIALOG_RESULT_CANCEL = -1,
    DIALOG_RESULT_NO = 0,
    DIALOG_RESULT_YES = 1,
    DIALOG_RESULT_CUSTOM = 2,
  };

  DialogResult GetResult() const;

  enum TimeOut
  {
    NO_TIMEOUT = 0
  };

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param line0 Localized label id or string for line 1 of the dialog message
   \param line1 Localized label id or string for line 2 of the dialog message
   \param line2 Localized label id or string for line 3 of the dialog message
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const CVariant& heading,
                              const CVariant& line0,
                              const CVariant& line1,
                              const CVariant& line2,
                              bool& bCanceled);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param line0 Localized label id or string for line 1 of the dialog message
   \param line1 Localized label id or string for line 2 of the dialog message
   \param line2 Localized label id or string for line 3 of the dialog message
   \param iNoLabel Localized label id or string for the no button
   \param iYesLabel Localized label id or string for the yes button
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const CVariant& heading,
                              const CVariant& line0,
                              const CVariant& line1,
                              const CVariant& line2,
                              const CVariant& noLabel = "",
                              const CVariant& yesLabel = "");

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param line0 Localized label id or string for line 1 of the dialog message
   \param line1 Localized label id or string for line 2 of the dialog message
   \param line2 Localized label id or string for line 3 of the dialog message
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \param iNoLabel Localized label id or string for the no button
   \param iYesLabel Localized label id or string for the yes button
   \param autoCloseTime Time in ms before the dialog becomes automatically closed
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const CVariant& heading,
                              const CVariant& line0,
                              const CVariant& line1,
                              const CVariant& line2,
                              bool& bCanceled,
                              const CVariant& noLabel,
                              const CVariant& yesLabel,
                              unsigned int autoCloseTime);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param text Localized label id or string for the dialog message
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const CVariant& heading, const CVariant& text);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param text Localized label id or string for the dialog message
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \param iNoLabel Localized label id or string for the no button
   \param iYesLabel Localized label id or string for the yes button
   \param autoCloseTime Time in ms before the dialog becomes automatically closed
   \param defaultButtonId Specifies the default focused button
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const CVariant& heading,
                              const CVariant& text,
                              bool& bCanceled,
                              const CVariant& noLabel,
                              const CVariant& yesLabel,
                              unsigned int autoCloseTime,
                              int defaultButtonId = CONTROL_NO_BUTTON);

  /*! \brief Show a yes-no dialog with 3rd custom button, then wait for user to dismiss it.
  \param heading Localized label id or string for the heading of the dialog
  \param text Localized label id or string for the dialog message
  \param noLabel Localized label id or string for the no button
  \param yesLabel Localized label id or string for the yes button
  \param customLabel Localized label id or string for the custom button
  \param autoCloseTime Time in ms before the dialog becomes automatically closed
  \param defaultButtonId Specifies the default focused button
  \return action that closed the dialog
  */
  static DialogResult ShowAndGetInput(const CVariant& heading,
                                      const CVariant& text,
                                      const CVariant& noLabel,
                                      const CVariant& yesLabel,
                                      const CVariant& customLabel,
                                      unsigned int autoCloseTime,
                                      int defaultButtonId = CONTROL_NO_BUTTON);

  /*!
    \brief Open a Yes/No dialog and wait for input

    \param[in] options  a struct of type DialogYesNoMessage containing
                        the options to set for this dialog.

    \returns action that closed the dialog
    \sa KODI::MESSAGING::HELPERS::DialogYesNoMessage
  */
  DialogResult ShowAndGetInput(const KODI::MESSAGING::HELPERS::DialogYesNoMessage& options);

protected:
  void OnInitWindow() override;
  int GetDefaultLabelID(int controlId) const override;

  bool m_bCanceled;
  bool m_bCustom;
  int m_defaultButtonId;
};

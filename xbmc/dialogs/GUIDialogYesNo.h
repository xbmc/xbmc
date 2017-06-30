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
  CGUIDialogYesNo(int overrideId = -1);
  ~CGUIDialogYesNo(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;

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
  static bool ShowAndGetInput(CVariant heading, CVariant line0, CVariant line1, CVariant line2, bool &bCanceled);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param line0 Localized label id or string for line 1 of the dialog message
   \param line1 Localized label id or string for line 2 of the dialog message
   \param line2 Localized label id or string for line 3 of the dialog message
   \param iNoLabel Localized label id or string for the no button
   \param iYesLabel Localized label id or string for the yes button
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(CVariant heading, CVariant line0, CVariant line1, CVariant line2, CVariant noLabel = "", CVariant yesLabel = "");

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
  static bool ShowAndGetInput(CVariant heading, CVariant line0, CVariant line1, CVariant line2, bool &bCanceled, CVariant noLabel, CVariant yesLabel, unsigned int autoCloseTime);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param text Localized label id or string for the dialog message
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(CVariant heading, CVariant text);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading Localized label id or string for the heading of the dialog
   \param text Localized label id or string for the dialog message
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \param iNoLabel Localized label id or string for the no button
   \param iYesLabel Localized label id or string for the yes button
   \param autoCloseTime Time in ms before the dialog becomes automatically closed
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(CVariant heading, CVariant text, bool &bCanceled, CVariant noLabel, CVariant yesLabel, unsigned int autoCloseTime);

  /*!
    \brief Open a Yes/No dialog and wait for input

    \param[in] options  a struct of type DialogYesNoMessage containing
                        the options to set for this dialog.

    \returns -1 for cancelled, 0 for No and 1 for Yes
    \sa KODI::MESSAGING::HELPERS::DialogYesNoMessage
  */
  int ShowAndGetInput(const KODI::MESSAGING::HELPERS::DialogYesNoMessage& options);

protected:
  void OnInitWindow() override;
  int GetDefaultLabelID(int controlId) const override;

  bool m_bCanceled;
};

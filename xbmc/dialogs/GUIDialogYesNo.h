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

#include <string>
#include "GUIDialogBoxBase.h"

class CGUIDialogYesNo :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogYesNo(int overrideId = -1);
  virtual ~CGUIDialogYesNo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading The heading of the dialog
   \param line0 Localized label id for line 1 of the dialog message
   \param line1 Localized label id for line 2 of the dialog message
   \param line2 Localized label id for line 3 of the dialog message
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, bool &bCanceled);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading The heading of the dialog
   \param line0 Localized label id for line 1 of the dialog message
   \param line1 Localized label id for line 2 of the dialog message
   \param line2 Localized label id for line 3 of the dialog message
   \param iNoLabel Localized label id for the no button
   \param iYesLabel Localized label id for the yes button
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel = -1, int iYesLabel = -1);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading The heading of the dialog
   \param line0 Localized label id for line 1 of the dialog message
   \param line1 Localized label id for line 2 of the dialog message
   \param line2 Localized label id for line 3 of the dialog message
   \param iNoLabel Localized label id for the no button
   \param iYesLabel Localized label id for the yes button
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \param autoCloseTime Time in ms before the dialog becomes automatically closed
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(int heading, int line0, int line1, int line2, int iNoLabel, int iYesLabel, bool &bCanceled, unsigned int autoCloseTime = 0);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading The heading of the dialog
   \param line0 String for line 1 of the dialog message
   \param line1 String for line 2 of the dialog message
   \param line2 String for line 3 of the dialog message
   \param iNoLabel Label for the no button
   \param iYesLabel Label for the yes button
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const std::string &heading, const std::string &line0, const std::string &line1, const std::string &line2, const std::string &noLabel = "", const std::string &yesLabel = "");

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading The heading of the dialog
   \param line0 String for line 1 of the dialog message
   \param line1 String for line 2 of the dialog message
   \param line2 String for line 3 of the dialog message
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \param iNoLabel Label for the no button
   \param iYesLabel Label for the yes button
   \param autoCloseTime Time in ms before the dialog becomes automatically closed
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const std::string &heading, const std::string &line0, const std::string &line1, const std::string &line2, bool &bCanceled, const std::string &noLabel = "", const std::string &yesLabel = "", unsigned int autoCloseTime = 0);

  /*! \brief Show a yes-no dialog, then wait for user to dismiss it.
   \param heading The heading of the dialog
   \param text The dialog message
   \param bCanceled Holds true if the dialog was canceled otherwise false
   \param iNoLabel Label for the no button
   \param iYesLabel Label for the yes button
   \param autoCloseTime Time in ms before the dialog becomes automatically closed
   \return true if user selects Yes, otherwise false if user selects No.
   */
  static bool ShowAndGetInput(const std::string &heading, const std::string &text, bool &bCanceled, const std::string &noLabel, const std::string &yesLabel, unsigned int autoCloseTime = 0);

protected:
  virtual int GetDefaultLabelID(int controlId) const;

  bool m_bCanceled;
};

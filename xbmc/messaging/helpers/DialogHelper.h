/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Variant.h"

#include <array>
#include <string>

namespace KODI
{
namespace MESSAGING
{
namespace HELPERS
{

enum class DialogResponse
{
  CANCELLED,
  YES,
  NO,
  CUSTOM
};

/*! \struct DialogYesNoMessage DialogHelper.h "messaging/helpers/DialogHelper.h"
    \brief Payload sent for message TMSG_GUI_DIALOG_YESNO

    \sa ShowDialogText
    \sa ShowDialogLines
*/
struct DialogYesNoMessage
{
  CVariant heading; //!< Heading to be displayed in the dialog box
  CVariant text;  //!< Body text to be displayed, this is mutually exclusive with lines below
  std::array<CVariant, 3> lines;  //!< Body text to be displayed, specified as three lines. This is mutually exclusive with the text above
  CVariant yesLabel;  //!< Text to show on the yes button
  CVariant noLabel; //!< Text to show on the no button
  CVariant customLabel; //!< Text to show on the 3rd custom button
  uint32_t autoclose{0}; //!< Time in milliseconds before autoclosing the dialog, 0 means don't autoclose
};

/*!
  \brief This is a helper method to send a threadmessage to open a Yes/No dialog box

  \param[in]  heading           The text to display as the dialog box header
  \param[in]  text              The text to display in the dialog body
  \param[in]  noLabel           The text to display on the No button
                                defaults to No
  \param[in]  yesLabel          The text to display on the Yes button
                                defaults to Yes
  \param[in]  autoCloseTimeout  The time before the dialog closes
                                defaults to 0 show indefinitely
  \return -1 on cancelled, 0 on no and 1 on yes
  \sa ShowYesNoDialogLines
  \sa CGUIDialogYesNo::ShowAndGetInput
  \sa DialogYesNoMessage
*/
DialogResponse ShowYesNoDialogText(CVariant heading, CVariant text, CVariant noLabel = CVariant(),
                                   CVariant yesLabel = CVariant(), uint32_t autoCloseTimeout = 0);

/*!
\brief This is a helper method to send a threadmessage to open a Yes/No dialog box with a custom button

\param[in]  heading           The text to display as the dialog box header
\param[in]  text              The text to display in the dialog body
\param[in]  noLabel           The text to display on the No button
                              defaults to No
\param[in]  yesLabel          The text to display on the Yes button
                              defaults to Yes
\param[in]  customLabel       The text to display on the optional 3rd custom button
                              defaults to empty and button not shown
\param[in]  autoCloseTimeout  The time before the dialog closes
                              defaults to 0 show indefinitely
\return -1 on cancelled, 0 on no, 1 on yes and 2 on 3rd custom response
\sa ShowYesNoDialogLines
\sa CGUIDialogYesNo::ShowAndGetInput
\sa DialogYesNoMessage
*/
DialogResponse ShowYesNoCustomDialog(CVariant heading, CVariant text, CVariant noLabel = CVariant(), CVariant yesLabel = CVariant(),
                                        CVariant customLabel = CVariant(), uint32_t autoCloseTimeout = 0);

/*!
  \brief This is a helper method to send a threadmessage to open a Yes/No dialog box

  \param[in]  heading           The text to display as the dialog box header
  \param[in]  line0             The text to display on the first line
  \param[in]  line1             The text to display on the second line
  \param[in]  line2             The text to display on the third line
  \param[in]  noLabel           The text to display on the No button
                                defaults to No
  \param[in]  yesLabel          The text to display on the Yes button
                                defaults to Yes
  \param[in]  autoCloseTimeout  The time before the dialog closes
                                defaults to 0 show indefinitely
  \return -1 on cancelled, 0 on no and 1 on yes
  \sa ShowYesNoDialogText
  \sa CGUIDialogYesNo::ShowAndGetInput
  \sa DialogYesNoMessage
*/
DialogResponse ShowYesNoDialogLines(CVariant heading, CVariant line0, CVariant line1 = CVariant(),
                                    CVariant line2 = CVariant(), CVariant noLabel = CVariant(),
                                    CVariant yesLabel = CVariant(), uint32_t autoCloseTimeout = 0);

}
}
}

/*
*      Copyright (C) 2005-2015 Team Kodi
*      http://kodi.tv
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
*  along with Kodi; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
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

/*! \struct DialogOkMessage DialogHelper.h "messaging/helpers/DialogHelper.h"
    \brief Payload sent for message TMSG_GUI_DIALOG_OK

    \sa ShowOKDialogText
    \sa ShowOKDialogLines
*/
struct DialogOKMessage
{
  CVariant heading; //!< Heading to be displayed in the dialog box
  CVariant text;  //!< Body text to be displayed, this is mutually exclusive with lines below
  std::array<CVariant, 3> lines;  //!< Body text to be displayed, specified as three lines. This is mutually exclusive with the text above
  bool show = true; //!< bool to see if the dialog needs to be shown
};

/*!
  \brief This is a helper method to send a threadmessage to open a Ok dialog box

  \param[in]  heading           The text to display as the dialog box header
  \param[in]  text              The text to display in the dialog body
  \return if it's confirmed
  \sa ShowOKDialogLines
  \sa CGUIDialogOK::ShowAndGetInput
  \sa DialogOKMessage
*/
void UpdateOKDialogText(CVariant heading, CVariant text);

/*!
\brief This is a helper method to send a threadmessage to update a Ok dialog text
\param[in]  heading           The text to display as the dialog box header
\param[in]  text              The text to display in the dialog body
\sa UpdateOKDialogLines
\sa CGUIDialogOK::ShowAndGetInput
\sa DialogOKMessage
*/
bool ShowOKDialogText(CVariant heading, CVariant text);

/*!
  \brief This is a helper method to send a threadmessage to open a OK dialog box

  \param[in]  heading           The text to display as the dialog box header
  \param[in]  line0             The text to display on the first line
  \param[in]  line1             The text to display on the second line
  \param[in]  line2             The text to display on the third line
  \return if it's confirmed
  \sa ShowOKDialogText
  \sa CGUIDialogOK::ShowAndGetInput
  \sa DialogOKMessage
*/
bool ShowOKDialogLines(CVariant heading, CVariant line0, CVariant line1 = CVariant(),
                                    CVariant line2 = CVariant());

}
}
}

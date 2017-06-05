#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions.h"
#include "../../AddonBase.h"

namespace kodi
{
namespace gui
{
namespace dialogs
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_dialogs_TextViewer Dialog Text Viewer
  /// \ingroup cpp_kodi_gui
  /// @{
  /// @brief \cpp_namespace{ kodi::gui::dialogs::TextViewer }
  /// **Text viewer dialog**
  ///
  /// The text viewer dialog can be used to display descriptions, help texts or
  /// other larger texts.
  ///
  /// In order to achieve a line break is a <b>\\n</b> directly in the text or
  /// in the <em>"./resources/language/resource.language.??_??/strings.po"</em>
  /// to call with <b>std::string kodi::general::GetLocalizedString(...);</b>.
  ///
  /// It has the header \ref TextViewer.h "#include <kodi/gui/dialogs/TextViewer.h>"
  /// be included to enjoy it.
  ///
  namespace TextViewer
  {
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_dialogs_TextViewer
    /// @brief Show info text dialog
    ///
    /// @param[in] heading  Small heading text
    /// @param[in] text     Showed text on dialog
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/gui/dialogs/TextViewer.h>
    ///
    /// kodi::gui::dialogs::TextViewer::Show("The Wizard of Oz (1939 film)",
    ///  "The Wizard of Oz is a 1939 American musical comedy-drama fantasy film "
    ///  "produced by Metro-Goldwyn-Mayer, and the most well-known and commercially "
    ///  "successful adaptation based on the 1900 novel The Wonderful Wizard of Oz "
    ///  "by L. Frank Baum. The film stars Judy Garland as Dorothy Gale. The film"
    ///  "co-stars Terry the dog, billed as Toto; Ray Bolger, Jack Haley, Bert Lahr, "
    ///  "Frank Morgan, Billie Burke, Margaret Hamilton, with Charley Grapewin and "
    ///  "Clara Blandick, and the Singer Midgets as the Munchkins.\n"
    ///  "\n"
    ///  "Notable for its use of Technicolor, fantasy storytelling, musical score and "
    ///  "unusual characters, over the years it has become an icon of American popular "
    ///  "culture. It was nominated for six Academy Awards, including Best Picture but "
    ///  "lost to Gone with the Wind. It did win in two other categories including Best "
    ///  "Original Song for \"Over the Rainbow\". However, the film was a box office "
    ///  "disappointment on its initial release, earning only $3,017,000 on a $2,777,000 "
    ///  "budget, despite receiving largely positive reviews. It was MGM's most "
    ///  "expensive production at that time, and did not completely recoup the studio's "
    ///  "investment and turn a profit until theatrical re-releases starting in 1949.\n"
    ///  "\n"
    ///  "The 1956 broadcast television premiere of the film on CBS re-introduced the "
    ///  "film to the wider public and eventually made the presentation an annual "
    ///  "tradition, making it one of the most known films in cinema history. The "
    ///  "film was named the most-viewed motion picture on television syndication by "
    ///  "the Library of Congress who also included the film in its National Film "
    ///  "Registry in its inaugural year in 1989. Designation on the registry calls "
    ///  "for efforts to preserve it for being \"culturally, historically, and "
    ///  "aesthetically significant\". It is also one of the few films on UNESCO's "
    ///  "Memory of the World Register.\n"
    ///  "\n"
    ///  "The Wizard of Oz is often ranked on best-movie lists in critics' and public "
    ///  "polls. It is the source of many quotes referenced in modern popular culture. "
    ///  "It was directed primarily by Victor Fleming (who left production to take "
    ///  "over direction on the troubled Gone with the Wind production). Noel Langley, "
    ///  "Florence Ryerson and Edgar Allan Woolf received credit for the screenplay, "
    ///  "but there were uncredited contributions by others. The songs were written "
    ///  "by Edgar \"Yip\" Harburg (lyrics) and Harold Arlen (music). The incidental "
    ///  "music, based largely on the songs, was composed by Herbert Stothart, with "
    ///  "interspersed renderings from classical composers.\n");
    /// ~~~~~~~~~~~~~
    ///
    inline void Show(const std::string& heading, const std::string& text)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogTextViewer->open(CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), text.c_str());
    }
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */

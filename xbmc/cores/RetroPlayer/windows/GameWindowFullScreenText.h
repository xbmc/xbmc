/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>
#include <vector>

class CGUIDialog;
class CGUIMessage;
class CGUIWindow;

namespace KODI
{
namespace RETRO
{
  class CGameWindowFullScreenText
  {
  public:
    CGameWindowFullScreenText(CGUIWindow &fullscreenWindow);
    ~CGameWindowFullScreenText() = default;

    // Window functions
    void OnWindowLoaded();
    void FrameMove();

    /*!
     * \brief Get a line of text
     */
    const std::string &GetText(unsigned int lineIndex) const;

    /*!
     * \brief Set a line of text
     */
    void SetText(unsigned int lineIndex, std::string line);

    /*!
     * \brief Get entire text
     */
    const std::vector<std::string> &GetText() const;

    /*!
    * \brief Set entire text
    */
    void SetText(std::vector<std::string> text);

  private:
    // Window functions
    void UploadText();
    void Show();
    void Hide();

    /*!
     * \brief Translate line index to the control ID in the skin
     *
     * \param lineIndex The line in the string vector
     *
     * \return The ID of the line's label control in the skin
     */
    static int GetControlID(unsigned int lineIndex);

    // Window functions required by GUIMessage macros
    //! @todo Change macros into functions
    int GetID() const;
    bool OnMessage(CGUIMessage &message);

    // Construction parameters
    CGUIWindow &m_fullscreenWindow;

    // Window state
    bool m_bShowText = false;
    bool m_bTextChanged = true;
    bool m_bTextVisibilityChanged = true;

    // Text
    std::vector<std::string> m_lines;
  };
}
}

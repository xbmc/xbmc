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

#pragma once

#include "guilib/GUIControl.h"
#include "guilib/GUIFont.h"
#include "input/Key.h"

#include "Alternative.h"
#include "Tuple.h"
#include "ListItem.h"
#include "swighelper.h"


// hardcoded offsets for button controls (and controls that use button controls)
// ideally they should be dynamically read in as with all the other properties.
#define CONTROL_TEXT_OFFSET_X 10
#define CONTROL_TEXT_OFFSET_Y 2

namespace XBMCAddon
{
  namespace xbmcgui
  {

    //
    /// \defgroup python_xbmcgui_control Control
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief <b>Code based skin access.</b>
    ///
    /// Kodi is noted as having a very flexible and robust framework for its
    /// GUI, making theme-skinning and personal customization very accessible.
    /// Users can create their own skin (or modify an existing skin) and share
    /// it with others. Confluence is the official skin.
    ///
    /// Kodi includes a new GUI library written from scratch. This library
    /// allows you to skin/change everything you see in Kodi, from the images,
    /// the sizes and positions of all controls, colours, fonts, and text,
    /// through to altering navigation and even adding new functionality. The
    /// skin system is quite complex, and this portion of the manual is dedicated
    /// to providing in depth information on how it all works, along with tips
    /// to make the experience a little more pleasant.
    ///
    ///-------------------------------------------------------------------------
    //
    class Control : public AddonClass
    {
    protected:
      Control() : iControlId(0), iParentId(0), dwPosX(0), dwPosY(0), dwWidth(0),
                  dwHeight(0), iControlUp(0), iControlDown(0), iControlLeft(0),
                  iControlRight(0), pGUIControl(NULL) {}

    public:
      virtual ~Control();

#ifndef SWIG
      virtual CGUIControl* Create();
#endif

      // currently we only accept messages from a button or controllist with a select action
      virtual bool canAcceptMessages(int actionId) { return false; }

      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Returns the control's current id as an integer.
      ///
      /// @return                       int - Current id
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// id = self.button.getId()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      int getId() { return iControlId; }

      inline bool operator==(const Control& other) const { return iControlId == other.iControlId; }
      inline bool operator>(const Control& other) const { return iControlId > other.iControlId; }
      inline bool operator<(const Control& other) const { return iControlId < other.iControlId; }

      // hack this because it returns a tuple
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Returns the control's current position as a x,y integer tuple.
      ///
      /// @return                       Current position as a x,y integer tuple
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// pos = self.button.getPosition()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      std::vector<int> getPosition();

      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Returns the control's current X position.
      ///
      /// @return                       int - Current X position
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// posX = self.button.getX()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      int getX() { return dwPosX; }

      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Returns the control's current Y position.
      ///
      /// @return                       int - Current Y position
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// posY = self.button.getY()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      int getY() { return dwPosY; }

      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Returns the control's current height as an integer.
      ///
      /// @return                       int - Current height
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// height = self.button.getHeight()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      int getHeight() { return dwHeight; }

      // getWidth() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Returns the control's current width as an integer.
      ///
      /// @return                       int - Current width
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// width = self.button.getWidth()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      int getWidth() { return dwWidth; }

      // setEnabled() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the control's enabled/disabled state.
      ///
      /// @param[in] enabled            bool - True=enabled / False=disabled.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setEnabled(False)n
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setEnabled(bool enabled);

      // setVisible() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the control's visible/hidden state.
      ///
      /// @param[in] visible            bool - True=visible / False=hidden.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setVisible(False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setVisible(bool visible);

      // setVisibleCondition() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the control's visible condition.
      ///
      /// Allows Kodi to control the visible status of the control.
      ///
      /// [List of Conditions](http://kodi.wiki/view/List_of_Boolean_Conditions)
      ///
      /// @param[in] visible            string - Visible condition
      /// @param[in] allowHiddenFocus   [opt] bool - True=gains focus even if
      ///                               hidden
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setVisibleCondition(visible[,allowHiddenFocus])
      /// self.button.setVisibleCondition('[Control.IsVisible(41) + !Control.IsVisible(12)]', True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setVisibleCondition(const char* visible, bool allowHiddenFocus = false);

      // setEnableCondition() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the control's enabled condition.
      ///
      /// Allows Kodi to control the enabled status of the control.
      ///
      /// [List of Conditions](http://kodi.wiki/view/List_of_Boolean_Conditions)
      ///
      /// @param[in] enable             string - Enable condition.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setEnableCondition(enable)
      /// self.button.setEnableCondition('System.InternetState')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setEnableCondition(const char* enable);

      // setAnimations() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the control's animations.
      ///
      /// <b>[(event,attr,)*]</b>: list - A list of tuples consisting of event
      /// and attributes pairs.
      ///
      /// [Animating your skin](http://kodi.wiki/view/Animating_Your_Skin)
      ///
      /// @param[in] event              string - The event to animate.
      /// @param[in] attr               string - The whole attribute string
      ///                               separated by spaces.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setAnimations([(event, attr,)*])
      /// self.button.setAnimations([('focus', 'effect=zoom end=90,247,220,56 time=0',)])
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setAnimations(const std::vector< Tuple<String,String> >& eventAttr);

      // setPosition() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls position.
      ///
      /// @param[in] x                  integer - x coordinate of control.
      /// @param[in] y                  integer - y coordinate of control.
      ///
      /// @note You may use negative integers. (e.g sliding a control into view)
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setPosition(100, 250)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setPosition(long x, long y);

      // setWidth() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls width.
      ///
      /// @param[in] width                integer - width of control.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.image.setWidth(100)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setWidth(long width);

      // setHeight() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls height.
      ///
      /// @param[in] height               integer - height of control.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.image.setHeight(100)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setHeight(long height);

      // setNavigation() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls navigation.
      ///
      /// @param[in] up                 control object - control to navigate to on up.
      /// @param[in] down               control object - control to navigate to on down.
      /// @param[in] left               control object - control to navigate to on left.
      /// @param[in] right              control object - control to navigate to on right.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      /// @note Same as controlUp(), controlDown(), controlLeft(), controlRight().
      ///       Set to self to disable navigation for that direction.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setNavigation(self.button1, self.button2, self.button3, self.button4)
      /// ...
      /// ~~~~~~~~~~~~~
      //
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setNavigation(const Control* up, const Control* down,
                                 const Control* left, const Control* right);

      // controlUp() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls up navigation.
      ///
      /// @param[in] control            control object - control to navigate to on up.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlUp(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void controlUp(const Control* up);

      // controlDown() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls down navigation.
      ///
      /// @param[in] control            control object - control to navigate to on down.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlDown(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void controlDown(const Control* control);

      // controlLeft() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls left navigation.
      ///
      /// @param[in] control            control object - control to navigate to on left.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlLeft(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void controlLeft(const Control* control);

      // controlRight() Method
      ///
      /// \ingroup python_xbmcgui_control
      /// @brief Set's the controls right navigation.
      ///
      /// @param[in] control            control object - control to navigate to on right.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlRight(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void controlRight(const Control* control);

#ifndef SWIG
      int iControlId;
      int iParentId;
      int dwPosX;
      int dwPosY;
      int dwWidth;
      int dwHeight;
      int iControlUp;
      int iControlDown;
      int iControlLeft;
      int iControlRight;
      CGUIControl* pGUIControl;
#endif

    };
    /// @}

    ///
    /// \defgroup python_xbmcgui_control_spin Subclass - ControlSpin
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief __Used for cycling up/down controls.__
    ///
    /// The spin control is used for when a list of options can be chosen (such
    /// as a page up/down control). You can choose the position, size, and look
    /// of the spin control.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @warning **Not working yet**.
    /// You can't create this object, it is returned by objects like ControlTextBox and ControlList.
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    ///
    class ControlSpin : public Control
    {
    public:
      virtual ~ControlSpin();

      ///
      /// \ingroup python_xbmcgui_control_spin
      /// @brief Set's textures for this control.
      ///
      /// Texture are image files that are used for example in the skin
      ///
      /// @warning **Not working yet**.
      ///
      /// @param[in] up                 label - for the up arrow
      ///                               when it doesn't have focus.
      /// @param[in] down               label - for the down button
      ///                               when it is not focused.
      /// @param[in] upFocus            label - for the up button
      ///                               when it has focus.
      /// @param[in] downFocus          label - for the down button
      ///                               when it has focus.
      /// @param[in] upDisabled         label - for the up arrow
      ///                               when the button is disabled.
      /// @param[in] downDisabled       label - for the up arrow
      ///                               when the button is disabled.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setTextures(up, down, upFocus, downFocus, upDisabled, downDisabled)
      ///
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setTextures(const char* up, const char* down,
                       const char* upFocus,
                       const char* downFocus,
                       const char* upDisabled, const char* downDisabled);
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef SWIG
      color_t color;
      std::string strTextureUp;
      std::string strTextureDown;
      std::string strTextureUpFocus;
      std::string strTextureDownFocus;
      std::string strTextureUpDisabled;
      std::string strTextureDownDisabled;
#endif

    private:
      ControlSpin();

      friend class Window;
      friend class ControlList;

    };
    /// @}

    ///
    /// \defgroup python_xbmcgui_control_label Subclass - ControlLabel
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief <b>Used to show some lines of text.</b>
    ///
    /// <b><c>ControlLabel(x, y, width, height, label[, font, textColor,
    ///              disabledColor, alignment, hasPath, angle])</c></b>
    ///
    /// The label control is used for displaying text in Kodi. You can choose
    /// the font, size, colour, location and contents of the text to be
    /// displayed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] label                string or unicode - text string.
    /// @param[in] font                 [opt] string - font used for label
    ///                                 text. (e.g. 'font13')
    /// @param[in] textColor            [opt] hexstring - color of enabled
    ///                                 label's label. (e.g. '0xFFFFFFFF')
    /// @param[in] disabledColor        [opt] hexstring - color of disabled
    ///                                 label's label. (e.g. '0xFFFF3300')
    /// @param[in] alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Defination name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// @param[in] hasPath              [opt] bool - True=stores a
    ///                                 path / False=no path
    /// @param[in] angle                [opt] integer - angle of control.
    ///                                 (<b>+</b> rotates CCW, <b>-</b> rotates C)
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// # ControlLabel(x, y, width, height, label[, font, textColor,
    /// #              disabledColor, alignment, hasPath, angle])
    /// self.label = xbmcgui.ControlLabel(100, 250, 125, 75, 'Status', angle=45)
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlLabel : public Control
    {
    public:
      ControlLabel(long x, long y, long width, long height, const String& label,
                  const char* font = NULL, const char* textColor = NULL,
                  const char* disabledColor = NULL,
                  long alignment = XBFONT_LEFT,
                  bool hasPath = false, long angle = 0);

      virtual ~ControlLabel();

      ///
      /// \ingroup python_xbmcgui_control_label
      /// @brief Returns the text value for this label.
      ///
      /// @return                       This label
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// label = self.label.getLabel()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      String getLabel();

      ///
      /// \ingroup python_xbmcgui_control_label
      /// @brief Set's text for this label.
      ///
      /// @param[in] label              string or unicode - text string.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.label.setLabel('Status')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);
#ifndef SWIG
      ControlLabel() :
        bHasPath(false),
        iAngle  (0)
      {}

      std::string strFont;
      std::string strText;
      color_t textColor;
      color_t disabledColor;
      uint32_t align;
      bool bHasPath;
      int iAngle;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

#endif
    };
    /// @}

    // ControlEdit class
    ///
    /// \defgroup python_xbmcgui_control_edit Subclass - ControlEdit
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used as an input control for the osd keyboard and other input fields.**
    ///
    /// <b>`ControlEdit(x, y, width, height, label[, font, textColor,
    ///              disabledColor, alignment, focusTexture, noFocusTexture])`</b>
    ///
    /// The edit control allows a user to input text in Kodi. You can choose the
    /// font, size, colour, location and header of the text to be displayed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] label                string or unicode - text string.
    /// @param[in] font                 [opt] string - font used for label text.
    ///                                 (e.g. 'font13')
    /// @param[in] textColor            [opt] hexstring - color of enabled
    ///                                 label's label. (e.g. '0xFFFFFFFF')
    /// @param[in] disabledColor        [opt] hexstring - color of disabled
    ///                                 label's label. (e.g. '0xFFFF3300')
    /// @param[in] alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Defination name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// @param[in] focusTexture         [opt] string - filename for focus texture.
    /// @param[in] noFocusTexture       [opt] string - filename for no focus texture.
    /// @param[in] isPassword           [opt] bool - True=mask text value with `****`.
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    /// optional arguments.\n
    /// Once you use a keyword, all following arguments require the keyword.\n
    /// After you create the control, you need to add it to the window with
    /// addControl().\n
    ///
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.edit = xbmcgui.ControlEdit(100, 250, 125, 75, 'Status')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlEdit : public Control
    {
    public:
      ControlEdit(long x, long y, long width, long height, const String& label,
                  const char* font = NULL, const char* textColor = NULL,
                  const char* disabledColor = NULL,
                  long _alignment = XBFONT_LEFT, const char* focusTexture = NULL,
                  const char* noFocusTexture = NULL, bool isPassword = false);


      // setLabel() Method
      ///
      /// \ingroup python_xbmcgui_control_edit
      /// @brief Set's text heading for this edit control.
      ///
      /// @param[in] label              string or unicode - text string.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.edit.setLabel('Status')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);

      // getLabel() Method
      ///
      /// \ingroup python_xbmcgui_control_edit
      /// @brief Returns the text heading for this edit control.
      ///
      /// @return                       Heading text
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// label = self.edit.getLabel()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      String getLabel();

      // setText() Method
      ///
      /// \ingroup python_xbmcgui_control_edit
      /// @brief Set's text value for this edit control.
      ///
      /// @param[in] value              string or unicode - text string.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.edit.setText('online')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setText(const String& text);

      // getText() Method
      ///
      /// \ingroup python_xbmcgui_control_edit
      /// @brief Returns the text value for this edit control.
      ///
      /// @return                       Text value of control
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// value = self.edit.getText()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      String getText();

#ifndef SWIG
      ControlEdit() :
        bIsPassword (false)
      {}

      std::string strFont;
      std::string strText;
      std::string strTextureFocus;
      std::string strTextureNoFocus;
      color_t textColor;
      color_t disabledColor;
      uint32_t align;
      bool bIsPassword;

      SWIGHIDDENVIRTUAL CGUIControl* Create();
#endif
    };
    /// @}

    // ControlList class
    ///
    /// \defgroup python_xbmcgui_control_list Subclass - ControlList
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used for a scrolling lists of items. Replaces the list control.**
    ///
    /// <b><c>ControlList(x, y, width, height[, font, textColor, buttonTexture, buttonFocusTexture,\n
    ///             selectedColor, imageWidth, imageHeight, itemTextXOffset, itemTextYOffset,\n
    ///             itemHeight, space, alignmentY, shadowColor])</c></b>
    ///
    /// The list container is one of several containers used to display items
    /// from file lists in various ways. The list container is very
    /// flexible - it's only restriction is that it is a list - i.e. a single
    /// column or row of items. The layout of the items is very flexible and
    /// is up to the skinner.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                        integer - x coordinate of control.
    /// @param[in] y                        integer - y coordinate of control.
    /// @param[in] width                    integer - width of control.
    /// @param[in] height                   integer - height of control.
    /// @param[in] font                     [opt] string - font used for items label. (e.g. 'font13')
    /// @param[in] textColor                [opt] hexstring - color of items label. (e.g. '0xFFFFFFFF')
    /// @param[in] buttonTexture            [opt] string - filename for focus texture.
    /// @param[in] buttonFocusTexture       [opt] string - filename for no focus texture.
    /// @param[in] selectedColor            [opt] integer - x offset of label.
    /// @param[in] imageWidth               [opt] integer - width of items icon or thumbnail.
    /// @param[in] imageHeight              [opt] integer - height of items icon or thumbnail.
    /// @param[in] itemTextXOffset          [opt] integer - x offset of items label.
    /// @param[in] itemTextYOffset          [opt] integer - y offset of items label.
    /// @param[in] itemHeight               [opt] integer - height of items.
    /// @param[in] space                    [opt] integer - space between items.
    /// @param[in] alignmentY               [opt] integer - Y-axis alignment of items label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Defination name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// @param[in] shadowColor              [opt] hexstring - color of items
    ///                                     label's shadow. (e.g. '0xFF000000')
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments.\n
    ///       Once you use a keyword, all following arguments require the
    ///       keyword.\n
    ///       After you create the control, you need to add it to the window
    ///       with addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.cList = xbmcgui.ControlList(100, 250, 200, 250, 'font14', space=5)
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlList : public Control
    {
      void internAddListItem(AddonClass::Ref<ListItem> listitem, bool sendMessage);

    public:
      ControlList(long x, long y, long width, long height, const char* font = NULL,
                  const char* textColor = NULL, const char* buttonTexture = NULL,
                  const char* buttonFocusTexture = NULL,
                  const char* selectedColor = NULL,
                  long _imageWidth=10, long _imageHeight=10, long _itemTextXOffset = CONTROL_TEXT_OFFSET_X,
                  long _itemTextYOffset = CONTROL_TEXT_OFFSET_Y, long _itemHeight = 27, long _space = 2,
                  long _alignmentY = XBFONT_CENTER_Y);

      virtual ~ControlList();

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Add a new item to this list control.
      ///
      /// @param[in] item                     string, unicode or ListItem - item to add.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.addItem('Reboot Kodi')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void addItem(const Alternative<String, const XBMCAddon::xbmcgui::ListItem* > & item, bool sendMessage = true);

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Adds a list of listitems or strings to this list control.
      ///
      /// @param[in] items                      List - list of strings, unicode objects or ListItems to add.
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      /// Large lists benefit considerably, than using the standard addItem()
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.addItems(items=listitems)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void addItems(const std::vector<Alternative<String,
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      const XBMCAddon::xbmcgui::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      ListItem* > > & items);

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Select an item by index number.
      ///
      /// @param[in] item                     integer - index number of the item to select.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.selectItem(12)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void selectItem(long item);

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Remove an item by index number.
      ///
      /// @param[in] index                    integer - index number of the item to remove.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.removeItem(12)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void removeItem(int index);

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Clear all ListItems in this control list.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.reset()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void reset();

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Returns the associated ControlSpin object.
      ///
      /// @warning Not working completely yet\n
      ///        After adding this control list to a window it is not possible to change
      ///        the settings of this spin control.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// ctl = cList.getSpinControl()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      Control* getSpinControl();

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Returns the position of the selected item as an integer.
      ///
      /// @note Returns -1 for empty lists.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// pos = cList.getSelectedPosition()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long getSelectedPosition();

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Returns the selected item as a ListItem object.
      ///
      /// @return                       The selected item
      ///
      ///
      /// @note Same as getSelectedPosition(), but instead of an integer a ListItem object
      ///        is returned. Returns None for empty lists.\n
      ///        See windowexample.py on how to use this.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// item = cList.getSelectedItem()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual XBMCAddon::xbmcgui::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      ListItem* getSelectedItem();


      // setImageDimensions() method
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Sets the width/height of items icon or thumbnail.
      ///
      /// @param[in] imageWidth               [opt] integer - width of items icon or thumbnail.
      /// @param[in] imageHeight              [opt] integer - height of items icon or thumbnail.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setImageDimensions(18, 18)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setImageDimensions(long imageWidth, long imageHeight);

      // setItemHeight() method
      ///
      /// setItemHeight(itemHeight) -- Sets the height of items.
      ///
      /// @param[in] itemHeight               integer - height of items.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setItemHeight(25)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setItemHeight(long height);

      // setSpace() method
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Set's the space between items.
      ///
      /// @param[in] space                    [opt] integer - space between items.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setSpace(5)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setSpace(int space);

      // setPageControlVisible() method
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Sets the spin control's visible/hidden state.
      ///
      /// @param[in] visible                  boolean - True=visible / False=hidden.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setPageControlVisible(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setPageControlVisible(bool visible);

      // size() method
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Returns the total number of items in this list control as an integer.
      ///
      /// @return                       Total number of items
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cnt = cList.size()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long size();


      // getItemHeight() Method
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Returns the control's current item height as an integer.
      ///
      /// @return                       Current item heigh
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// item_height = self.cList.getItemHeight()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long getItemHeight();

      // getSpace() Method
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Returns the control's space between items as an integer.
      ///
      /// @return                       Space between items
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// gap = self.cList.getSpace()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long getSpace();

      // getListItem() method
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Returns a given ListItem in this List.
      ///
      /// @param[in] index              integer - index number of item to return.
      /// @return                       List item
      /// @throw ValueError             if index is out of range.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem = cList.getListItem(6)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual XBMCAddon::xbmcgui::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      ListItem* getListItem(int index);

      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief Fills a static list with a list of listitems.
      ///
      /// @param[in] items                      List - list of listitems to add.
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setStaticContent(items=listitems)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setStaticContent(const ListItemList* items);

#ifndef SWIG
      void sendLabelBind(int tail);

      SWIGHIDDENVIRTUAL bool canAcceptMessages(int actionId)
      { return ((actionId == ACTION_SELECT_ITEM) | (actionId == ACTION_MOUSE_LEFT_CLICK)); }

      // This is called from AddonWindow.cpp but shouldn't be available
      //  to the scripting languages.
      ControlList() :
        imageHeight     (0),
        imageWidth      (0),
        itemHeight      (0),
        space           (0),
        itemTextOffsetX (0),
        itemTextOffsetY (0)
      {}

      std::vector<AddonClass::Ref<ListItem> > vecItems;
      std::string strFont;
      AddonClass::Ref<ControlSpin> pControlSpin;

      color_t textColor;
      color_t selectedColor;
      std::string strTextureButton;
      std::string strTextureButtonFocus;

      int imageHeight;
      int imageWidth;
      int itemHeight;
      int space;

      int itemTextOffsetX;
      int itemTextOffsetY;
      uint32_t alignmentY;

      SWIGHIDDENVIRTUAL CGUIControl* Create();
#endif
    };
    /// @}

    // ControlFadeLabel class
    ///
    /// \defgroup python_xbmcgui_control_fadelabel Subclass - ControlFadeLabel
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used to show multiple pieces of text in the same position, by
    /// fading from one to the other.**
    ///
    /// <b><c>ControlFadeLabel(x, y, width, height[, font, textColor, alignment])</c></b>
    ///
    /// The fade label control is used for displaying multiple pieces of text
    /// in the same space in Kodi. You can choose the font, size, colour,
    /// location and contents of the text to be displayed. The first piece of
    /// information to display fades in over 50 frames, then scrolls off to
    /// the left. Once it is finished scrolling off screen, the second piece
    /// of information fades in and the process repeats. A fade label control
    /// is not supported in a list container.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] font                 [opt] string - font used for label text. (e.g. 'font13')
    /// @param[in] textColor            [opt] hexstring - color of fadelabel's labels. (e.g. '0xFFFFFFFF')
    /// @param[in] alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Defination name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments.\n
    ///       Once you use a keyword, all following arguments require the
    ///       keyword.\n
    ///       After you create the control, you need to add it to the window
    ///       with addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.fadelabel = xbmcgui.ControlFadeLabel(100, 250, 200, 50, textColor='0xFFFFFFFF')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlFadeLabel : public Control
    {
    public:
      ControlFadeLabel(long x, long y, long width, long height,
                       const char* font = NULL,
                       const char* textColor = NULL,
                       long _alignment = XBFONT_LEFT);

      // addLabel() Method
      ///
      /// \ingroup python_xbmcgui_control_fadelabel
      /// @brief Add a label to this control for scrolling.
      ///
      /// @param[in] label                string or unicode - text string to add.
      ///
      /// @note To remove added text use `reset()` for them.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.fadelabel.addLabel('This is a line of text that can scroll.')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
 #ifndef DOXYGEN_SHOULD_SKIP_THIS
       virtual
 #endif /* DOXYGEN_SHOULD_SKIP_THIS */
       void addLabel(const String& label);

      ///
      /// \ingroup python_xbmcgui_control_fadelabel
      /// @brief Set scrolling. If set to false, the labels won't scroll.
      /// Defaults to true.
      ///
      /// @param[in] scroll                boolean - True = enabled / False = disabled
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.fadelabel.setScrolling(False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setScrolling(bool scroll);

      ///
      /// \ingroup python_xbmcgui_control_label
      /// @brief Clear this fade label.
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.fadelabel.reset()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void reset();

#ifndef SWIG
      std::string strFont;
      color_t textColor;
      std::vector<std::string> vecLabels;
      uint32_t align;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      ControlFadeLabel() {}
#endif
    };
    /// @}

    // ControlTextBox class
    ///
    /// \defgroup python_xbmcgui_control_textbox Subclass - ControlTextBox
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used to show a multi-page piece of text.**
    ///
    /// <b><c>ControlTextBox(x, y, width, height[, font, textColor])</c></b>
    ///
    /// The text box is used for showing a large multipage piece of text in Kodi.
    /// You can choose the position, size, and look of the text.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] font                 [opt] string - font used for text. (e.g. 'font13')
    /// @param[in] textColor            [opt] hexstring - color of textbox's text. (e.g. '0xFFFFFFFF')
    ///
    /// @note You can use the above as keywords for arguments and skip certain optional arguments.\n
    ///        Once you use a keyword, all following arguments require the keyword.\n
    ///        After you create the control, you need to add it to the window with addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// # ControlTextBox(x, y, width, height[, font, textColor])
    /// self.textbox = xbmcgui.ControlTextBox(100, 250, 300, 300, textColor='0xFFFFFFFF')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlTextBox : public Control
    {
    public:
      ControlTextBox(long x, long y, long width, long height,
                     const char* font = NULL,
                     const char* textColor = NULL);

      // SetText() Method
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief Set's the text for this textbox.
      ///
      /// @param[in] text                 string or unicode - text string.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setText(text)
      /// self.textbox.setText('This is a line of text that can wrap.')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setText(const String& text);

      // getText() Method
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief Returns the text value for this textbox.
      ///
      /// @return                       To get text from box
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getText()
      /// text = self.text.getText()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      String getText();

      // reset() Method
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief Clear's this textbox.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # reset()
      /// self.textbox.reset()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void reset();

      // scroll() Method
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief Scrolls to the given position.
      ///
      /// @param[in] id                 integer - position to scroll to.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # scroll(position)
      /// self.textbox.scroll(10)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void scroll(long id);

      // autoScroll() Method
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief Set autoscrolling times.
      ///
      /// @param[in] delay                 integer - Scroll delay (in ms)
      /// @param[in] time                  integer - Scroll time (in ms)
      /// @param[in] repeat                integer - Repeat time
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.textbox.autoScroll(1, 2, 1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void autoScroll(int delay, int time, int repeat);

#ifndef SWIG
      std::string strFont;
      color_t textColor;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      ControlTextBox() {}
#endif
    };
    /// @}

    // ControlImage class
    ///
    /// \defgroup python_xbmcgui_control_image Subclass - ControlImage
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used to show an image.**
    ///
    /// <b><c>ControlImage(x, y, width, height, filename[, aspectRatio, colorDiffuse])</c></b>
    ///
    /// The image control is used for displaying images in Kodi. You can choose
    /// the position, size, transparency and contents of the image to be
    /// displayed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] filename             string - image filename.
    /// @param[in] aspectRatio          [opt] integer - (values 0 = stretch
    ///                                 (default), 1 = scale up (crops),
    ///                                 2 = scale down (black bar)
    /// @param[in] colorDiffuse         hexString - (example, '0xC0FF0000' (red tint))
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments.\n
    ///       Once you use a keyword, all following arguments require the
    ///       keyword.\n
    ///       After you create the control, you need to add it to the window with
    ///       addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// # ControlImage(x, y, width, height, filename[, aspectRatio, colorDiffuse])
    /// self.image = xbmcgui.ControlImage(100, 250, 125, 75, aspectRatio=2)
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlImage : public Control
    {
    public:
      ControlImage(long x, long y, long width, long height,
                   const char* filename, long aspectRatio = 0,
                   const char* colorDiffuse = NULL);

      ///
      /// \ingroup python_xbmcgui_control_image
      /// @brief Changes the image.
      ///
      /// @param[in] filename             string - image filename.
      /// @param[in] useCache             [opt] bool - True=use cache (default) /
      ///                                 False=don't use cache.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setImage(filename[, useCache])
      /// self.image.setImage('special://home/scripts/test.png')
      /// self.image.setImage('special://home/scripts/test.png', False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setImage(const char* imageFilename, const bool useCache = true);

      ///
      /// \ingroup python_xbmcgui_control_image
      /// @brief Changes the images color.
      ///
      /// @param[in] colorDiffuse         hexString - (example, '0xC0FF0000'
      ///                                 (red tint))
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setColorDiffuse(colorDiffuse)
      /// self.image.setColorDiffuse('0xC0FF0000')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setColorDiffuse(const char* hexString);

#ifndef SWIG
      ControlImage() :
        aspectRatio (0)
      {}

      std::string strFileName;
      int aspectRatio;
      color_t colorDiffuse;

      SWIGHIDDENVIRTUAL CGUIControl* Create();
#endif
    };
    /// @}

    // ControlImage class
    ///
    /// \defgroup python_xbmcgui_control_progress Subclass - ControlProgress
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used to show the progress of a particular operation.**
    ///
    /// <b><c>ControlProgress(x, y, width, height, filename[, texturebg, textureleft, texturemid, textureright, textureoverlay])</c></b>
    ///
    /// The progress control is used to show the progress of an item that may
    /// take a long time, or to show how far through a movie you are. You can
    /// choose the position, size, and look of the progress control.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] filename             string - image filename.
    /// @param[in] texturebg            [opt] string - specifies the image file
    ///                                 whichshould be displayed in the
    ///                                 background of the progress control.
    /// @param[in] textureleft          [opt] string - specifies the image file
    ///                                 whichshould be displayed for the left
    ///                                 side of the progress bar. This is
    ///                                 rendered on the left side of the bar.
    /// @param[in] texturemid           [opt] string - specifies the image file
    ///                                 which should be displayed for the middl
    ///                                 portion of the progress bar. This is
    ///                                 the `fill` texture used to fill up the
    ///                                 bar. It's positioned on the right of
    ///                                 the `<lefttexture>` texture, and fills
    ///                                 the gap between the `<lefttexture>` and
    ///                                 `<righttexture>` textures, depending on
    ///                                 how far progressed the item is.
    /// @param[in] textureright         [opt] string - specifies the image file
    ///                                 which should be displayed for the right
    ///                                 side of the progress bar. This is
    ///                                 rendered on the right side of the bar.
    /// @param[in] textureoverlay       [opt] string - specifies the image file
    ///                                 which should be displayed over the top of
    ///                                 all other images in the progress bar. It
    ///                                 is centered vertically and horizontally
    ///                                 within the space taken up by the
    ///                                 background image.
    ///
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments.\n
    ///       Once you use a keyword, all following arguments require the
    ///       keyword.\n
    ///       After you create the control, you need to add it to the window
    ///       with addControl().
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// # ControlProgress(x, y, width, height, filename[, texturebg, textureleft, texturemid, textureright, textureoverlay])
    /// self.image = xbmcgui.ControlProgress(100, 250, 250, 30, 'special://home/scripts/test.png')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlProgress : public Control
    {
    public:
      ControlProgress(long x, long y, long width, long height,
                      const char* texturebg = NULL,
                      const char* textureleft = NULL,
                      const char* texturemid = NULL,
                      const char* textureright = NULL,
                      const char* textureoverlay = NULL);

      ///
      /// \ingroup python_xbmcgui_control_progress
      /// @brief Sets the percentage of the progressbar to show.
      ///
      /// @param[in] percent             float - percentage of the bar to show.
      ///
      ///
      /// @note valid range for percent is 0-100
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setPercent(percent)
      /// self.progress.setPercent(60)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setPercent(float pct);

      ///
      /// \ingroup python_xbmcgui_control_progress
      /// @brief Returns a float of the percent of the progress.
      ///
      /// @return                       Percent position
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getPercent()
      /// print self.progress.getPercent()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
 #ifndef DOXYGEN_SHOULD_SKIP_THIS
       virtual
 #endif /* DOXYGEN_SHOULD_SKIP_THIS */
       float getPercent();

#ifndef SWIG
      std::string strTextureLeft;
      std::string strTextureMid;
      std::string strTextureRight;
      std::string strTextureBg;
      std::string strTextureOverlay;
      int aspectRatio;
      color_t colorDiffuse;

      SWIGHIDDENVIRTUAL CGUIControl* Create();
      ControlProgress() :
        aspectRatio (0)
      {}
#endif
    };
    /// @}

    // ControlButton class
    ///
    ///
    /// \defgroup python_xbmcgui_control_button Subclass - ControlButton
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief <b>A standard push button control.</b>
    ///
    /// <b><c>ControlButton(x, y, width, height, label[, focusTexture, noFocusTexture, textOffsetX, textOffsetY,
    ///               alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor])</c></b>
    ///
    /// The button control is used for creating push buttons in Kodi. You can
    /// choose the position, size, and look of the button, as well as choosing
    /// what action(s) should be performed when pushed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] label                string or unicode - text string.
    /// @param[in] focusTexture         [opt] string - filename for focus
    ///                                 texture.
    /// @param[in] noFocusTexture       [opt] string - filename for no focus
    ///                                 texture.
    /// @param[in] textOffsetX          [opt] integer - x offset of label.
    /// @param[in] textOffsetY          [opt] integer - y offset of label.
    /// @param[in] alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Defination name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// @param[in] font                 [opt] string - font used for label text.
    ///                                 (e.g. 'font13')
    /// @param[in] textColor            [opt] hexstring - color of enabled
    ///                                 button's label. (e.g. '0xFFFFFFFF')
    /// @param[in] disabledColor        [opt] hexstring - color of disabled
    ///                                 button's label. (e.g. '0xFFFF3300')
    /// @param[in] angle                [opt] integer - angle of control.
    ///                                 (+ rotates CCW, - rotates CW)
    /// @param[in] shadowColor          [opt] hexstring - color of button's
    ///                                 label's shadow. (e.g. '0xFF000000')
    /// @param[in] focusedColor         [opt] hexstring - color of focused
    ///                                 button's label. (e.g. '0xFF00FFFF')
    ///
    /// @note You can use the above as keywords for arguments and skip
    ///       certain optional arguments.\n
    ///       Once you use a keyword, all following arguments require
    ///       the keyword.\n
    ///       After you create the control, you need to add it to the
    ///       window with addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// # ControlButton(x, y, width, height, label[, focusTexture, noFocusTexture, textOffsetX, textOffsetY,
    /// #               alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor])
    /// self.button = xbmcgui.ControlButton(100, 250, 200, 50, 'Status', font='font14')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlButton : public Control
    {
    public:
      ControlButton(long x, long y, long width, long height, const String& label,
                    const char* focusTexture = NULL, const char* noFocusTexture = NULL,
                    long textOffsetX = CONTROL_TEXT_OFFSET_X,
                    long textOffsetY = CONTROL_TEXT_OFFSET_Y,
                    long alignment = (XBFONT_LEFT | XBFONT_CENTER_Y),
                    const char* font = NULL, const char* textColor = NULL,
                    const char* disabledColor = NULL, long angle = 0,
                    const char* shadowColor = NULL, const char* focusedColor = NULL);

      // setLabel() Method
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief Set's this buttons text attributes.
      ///
      /// @param[in] label                [opt] string or unicode - text string.
      /// @param[in] font                 [opt] string - font used for label text. (e.g. 'font13')
      /// @param[in] textColor            [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')
      /// @param[in] disabledColor        [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
      /// @param[in] shadowColor          [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')
      /// @param[in] focusedColor         [opt] hexstring - color of focused button's label. (e.g. '0xFFFFFF00')
      /// @param[in] label2               [opt] string or unicode - text string.
      ///
      /// @note You can use the above as keywords for arguments and skip certain
      ///       optional arguments.\n
      ///       Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setLabel([label, font, textColor, disabledColor, shadowColor, focusedColor])
      /// self.button.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);

      // setDisabledColor() Method
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief Set's this buttons disabled color.
      ///
      /// @param[in] disabledColor        hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setDisabledColor(disabledColor)
      /// self.button.setDisabledColor('0xFFFF3300')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setDisabledColor(const char* color);

      // getLabel() Method
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief Returns the buttons label as a unicode string.
      ///
      /// @return                       Unicode string
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getLabel()
      /// label = self.button.getLabel()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      String getLabel();

      // getLabel2() Method
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief Returns the buttons label2 as a unicode string.
      ///
      /// @return                       Unicode string of label 2
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getLabel2()
      /// label = self.button.getLabel2()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      String getLabel2();
#ifndef SWIG
      SWIGHIDDENVIRTUAL bool canAcceptMessages(int actionId) { return true; }

      int textOffsetX;
      int textOffsetY;
      color_t align;
      std::string strFont;
      color_t textColor;
      color_t disabledColor;
      int iAngle;
      int shadowColor;
      int focusedColor;
      std::string strText;
      std::string strText2;
      std::string strTextureFocus;
      std::string strTextureNoFocus;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      ControlButton() :
        textOffsetX (0),
        textOffsetY (0),
        iAngle      (0),
        shadowColor (0),
        focusedColor(0)
      {}
#endif
    };
    /// @}

    // ControlCheckMark class
    ///
    ///
    /// \defgroup python_xbmcgui_control_checkmark Subclass - ControlCheckMark
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Check mark control class (deprecated).**
    ///
    /// <b><c>ControlCheckMark(x, y, width, height, label[, focusTexture, noFocusTexture,
    ///                  checkWidth, checkHeight, alignment, font, textColor, disabledColor])</c></b>
    ///
    /// Used on the XLink Kai Host dialog. Similar to Radio Button but does not
    /// have the button texture, and the check-mark is on the left.
    ///
    /// @warning **Depreciated control!**\n
    ///          These control are not recommended to be used inside skins and
    ///          will be removed from the skinning engine at some point. Or may
    ///          not work at all.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] label                string or unicode - text string.
    /// @param[in] focusTexture         [opt] string - filename for focus texture.
    /// @param[in] noFocusTexture       [opt] string - filename for no focus texture.
    /// @param[in] checkWidth           [opt] integer - width of checkmark.
    /// @param[in] checkHeight          [opt] integer - height of checkmark.
    /// @param[in] alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Defination name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// @param[in] font                 [opt] string - font used for label text. (e.g. 'font13')
    /// @param[in] textColor            [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')
    /// @param[in] disabledColor        [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')
    ///
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments.\n
    ///       Once you use a keyword, all following arguments require the
    ///       keyword.\n
    ///       After you create the control, you need to add it to the window with
    ///       addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.checkmark = xbmcgui.ControlCheckMark(100, 250, 200, 50, 'Status', font='font14')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlCheckMark : public Control
    {
    public:

      ControlCheckMark(long x, long y, long width, long height, const String& label,
                       const char* focusTexture = NULL, const char* noFocusTexture = NULL,
                       long checkWidth = 30, long checkHeight = 30,
                       long _alignment = XBFONT_RIGHT, const char* font = NULL,
                       const char* textColor = NULL, const char* disabledColor = NULL);

      // getSelected() Method
      ///
      /// \ingroup python_xbmcgui_control_checkmark
      /// @brief Returns the selected status for this checkmark as a bool.
      ///
      /// @return                       Selected status
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getSelected()
      /// selected = self.checkmark.getSelected()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      bool getSelected();

      // setSelected() Method
      ///
      /// \ingroup python_xbmcgui_control_checkmark
      /// @brief Sets this checkmark status to on or off.
      ///
      /// @param[in] isOn               bool - True=selected (on) / False=not
      ///                               selected (off)
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setSelected(isOn)
      /// self.checkmark.setSelected(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setSelected(bool selected);

      // setLabel() Method
      ///
      /// \ingroup python_xbmcgui_control_checkmark
      /// @brief Set's this controls text attributes.
      ///
      /// @param[in] label              string or unicode - text string.
      /// @param[in] font               [opt] string - font used for label
      ///                               text. (e.g. 'font13')
      /// @param[in] textColor          [opt] hexstring - color of enabled
      ///                               checkmark's label. (e.g. '0xFFFFFFFF')
      /// @param[in] disabledColor      [opt] hexstring - color of disabled
      ///                               checkmark's label. (e.g. '0xFFFF3300')
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setLabel(label[, font, textColor, disabledColor])
      /// self.checkmark.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);

      // setDisabledColor() Method
      ///
      /// \ingroup python_xbmcgui_control_checkmark
      /// @brief Set's this controls disabled color.
      ///
      /// @param[in] disabledColor      hexstring - color of disabled checkmark's
      ///                               label. (e.g. '0xFFFF3300')
      ///
      ///
      ///----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setDisabledColor(disabledColor)
      /// self.checkmark.setDisabledColor('0xFFFF3300')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setDisabledColor(const char* color);

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool canAcceptMessages(int actionId) { return true; }

      std::string strFont;
      int checkWidth;
      int checkHeight;
      uint32_t align;
      color_t textColor;
      color_t disabledColor;
      std::string strTextureFocus;
      std::string strTextureNoFocus;
      std::string strText;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      ControlCheckMark() :
        checkWidth  (0),
        checkHeight (0)
      {}
#endif
    };
    /// @}

    // ControlGroup class
    ///
    /// \defgroup python_xbmcgui_control_group Subclass - ControlGroup
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief <b>Used to group controls together..</b>
    ///
    /// <b><c>ControlGroup(x, y, width, height)</c></b>
    ///
    /// The group control is one of the most important controls. It allows you
    /// to group controls together, applying attributes to all of them at once.
    /// It also remembers the last navigated button in the group, so you can set
    /// the <b>`<onup>`</b> of a control to a group of controls to have it always
    /// go back to the one you were at before. It also allows you to position
    /// controls more accurately relative to each other, as any controls within
    /// a group take their coordinates from the group's top left corner (or from
    /// elsewhere if you use the <b>"r"</b> attribute). You can have as many
    /// groups as you like within the skin, and groups within groups are handled
    /// with no issues.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.group = xbmcgui.ControlGroup(100, 250, 125, 75)
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlGroup : public Control
    {
    public:
      ControlGroup(long x, long y, long width, long height);

#ifndef SWIG
      SWIGHIDDENVIRTUAL CGUIControl* Create();

      inline ControlGroup() {}
#endif
    };
    /// @}

    // ControlRadioButton class
    ///
    /// \defgroup python_xbmcgui_control_radiobutton Subclass - ControlRadioButton
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief <b>For control a radio button (as used for on/off settings).</b>
    ///
    /// <b><c>ControlRadioButton(x, y, width, height, label[, focusOnTexture, noFocusOnTexture,\n
    ///                   focusOffTexture, noFocusOffTexture, focusTexture, noFocusTexture,\n
    ///                   textOffsetX, textOffsetY, alignment, font, textColor, disabledColor])</c></b>
    ///
    /// The radio button control is used for creating push button on/off
    /// settings in Kodi. You can choose the position, size, and look of the
    /// button. When the user clicks on the radio button, the state will change,
    /// toggling the extra textures (textureradioon and textureradiooff). Used
    /// for settings controls.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control.
    /// @param[in] y                    integer - y coordinate of control.
    /// @param[in] width                integer - width of control.
    /// @param[in] height               integer - height of control.
    /// @param[in] label                string or unicode - text string.
    /// @param[in] focusOnTexture       [opt] string - filename for radio ON
    ///                                 focused texture.
    /// @param[in] noFocusOnTexture     [opt] string - filename for radio ON not
    ///                                 focused texture.
    /// @param[in] focusOfTexture       [opt] string - filename for radio OFF
    ///                                 focused texture.
    /// @param[in] noFocusOffTexture    [opt] string - filename for radio OFF
    ///                                 not focused texture.
    /// @param[in] focusTexture         [opt] string - filename for radio ON
    ///                                 texture (deprecated, use focusOnTexture
    ///                                 and noFocusOnTexture).
    /// @param[in] noFocusTexture       [opt] string - filename for radio OFF
    ///                                 texture (deprecated, use focusOffTexture
    ///                                 and noFocusOffTexture).
    /// @param[in] textOffsetX          [opt] integer - horizontal text offset
    /// @param[in] textOffsetY          [opt] integer - vertical text offset
    /// @param[in] alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Defination name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// @param[in] font                 [opt] string - font used for label text.
    ///                                 (e.g. 'font13')
    /// @param[in] textColor            [opt] hexstring - color of enabled
    ///                                 checkmark's label. (e.g. '0xFFFFFFFF')
    /// @param[in] disabledColor        [opt] hexstring - color of disabled
    ///                                 checkmark's label. (e.g. '0xFFFF3300')
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments.\n
    ///       Once you use a keyword, all following arguments require the
    ///       keyword.\n
    ///       After you create the control, you need to add it to the window with
    ///       addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.radiobutton = xbmcgui.ControlRadioButton(100, 250, 200, 50, 'Enable', font='font14')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    class ControlRadioButton : public Control
    {
    public:
      ControlRadioButton(long x, long y, long width, long height, const String& label,
                         const char* focusOnTexture = NULL, const char* noFocusOnTexture = NULL,
                         const char* focusOffTexture = NULL, const char* noFocusOffTexture = NULL,
                         const char* focusTexture = NULL, const char* noFocusTexture = NULL,
                         long textOffsetX = CONTROL_TEXT_OFFSET_X,
                         long textOffsetY = CONTROL_TEXT_OFFSET_Y,
                         long _alignment = (XBFONT_LEFT | XBFONT_CENTER_Y),
                         const char* font = NULL, const char* textColor = NULL,
                         const char* disabledColor = NULL, long angle = 0,
                         const char* shadowColor = NULL, const char* focusedColor = NULL,
                         const char* disabledOnTexture = NULL, const char* disabledOffTexture = NULL);

      // setSelected() Method
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief <b>Sets the radio buttons's selected status.</b>
      ///
      /// @param[in] selected           bool - True=selected (on) / False=not
      ///                               selected (off)
      ///
      /// @note You can use the above as keywords for arguments and skip certain
      /// optional arguments.\n
      /// Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.radiobutton.setSelected(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setSelected(bool selected);

      // isSelected() Method
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief Returns the radio buttons's selected status.
      ///
      /// @return                       True if selected on
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// is = self.radiobutton.isSelected()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      bool isSelected();

      // setLabel() Method
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief Set's the radio buttons text attributes.
      ///
      /// @param[in] label              string or unicode - text string.
      /// @param[in] font               [opt] string - font used for label
      ///                               text. (e.g. 'font13')
      /// @param[in] textColor          [opt] hexstring - color of enabled radio
      ///                               button's label. (e.g. '0xFFFFFFFF')
      /// @param[in] disabledColor      [opt] hexstring - color of disabled
      ///                               radio button's label. (e.g. '0xFFFF3300')
      /// @param[in] shadowColor        [opt] hexstring - color of radio
      ///                               button's label's shadow.
      ///                               (e.g. '0xFF000000')
      /// @param[in] focusedColor       [opt] hexstring - color of focused radio
      ///                               button's label. (e.g. '0xFFFFFF00')
      ///
      ///
      /// @note You can use the above as keywords for arguments and skip certain
      ///       optional arguments.\n
      ///       Once you use a keyword, all following arguments require the
      ///       keyword.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor])
      /// self.radiobutton.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);

      // setRadioDimension() Method
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief Sets the radio buttons's radio texture's position and size.
      ///
      /// @param[in] x                  integer - x coordinate of radio texture.
      /// @param[in] y                  integer - y coordinate of radio texture.
      /// @param[in] width              integer - width of radio texture.
      /// @param[in] height             integer - height of radio texture.
      ///
      ///
      /// @note You can use the above as keywords for arguments and skip certain
      /// optional arguments.\n
      /// Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.radiobutton.setRadioDimension(x=100, y=5, width=20, height=20)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setRadioDimension(long x, long y, long width, long height);

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool canAcceptMessages(int actionId) { return true; }

      std::string strFont;
      std::string strText;
      std::string strTextureFocus;
      std::string strTextureNoFocus;
      std::string strTextureRadioOnFocus;
      std::string strTextureRadioOnNoFocus;
      std::string strTextureRadioOffFocus;
      std::string strTextureRadioOffNoFocus;
      std::string strTextureRadioOnDisabled;
      std::string strTextureRadioOffDisabled;
      color_t textColor;
      color_t disabledColor;
      int textOffsetX;
      int textOffsetY;
     uint32_t align;
      int iAngle;
      color_t shadowColor;
      color_t focusedColor;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      ControlRadioButton() :
        textOffsetX (0),
        textOffsetY (0),
        iAngle      (0)
      {}
#endif
    };
    /// @}

    //
    /// \defgroup python_xbmcgui_control_slider Subclass - ControlSlider
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief <b>Used for a volume slider.</b>
    ///
    /// <b>`ControlSlider(x, y, width, height[, textureback, texture, texturefocus])`</b>
    ///
    /// The slider control is used for things where a sliding bar best represents
    /// the operation at hand (such as a volume control or seek control). You can
    /// choose the position, size, and look of the slider control.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param[in] x                    integer - x coordinate of control
    /// @param[in] y                    integer - y coordinate of control
    /// @param[in] width                integer - width of control
    /// @param[in] height               integer - height of control
    /// @param[in] textureback          [opt] string - image filename
    /// @param[in] texture              [opt] string - image filename
    /// @param[in] texturefocus         [opt] string - image filename
    ///
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments.\n
    ///       Once you use a keyword, all following arguments require the
    ///       keyword.\n
    ///       After you create the control, you need to add it to the window
    ///       with addControl().
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.slider = xbmcgui.ControlSlider(100, 250, 350, 40)
    /// ...
    /// ~~~~~~~~~~~~~
    //
    class ControlSlider : public Control
    {
    public:
      ControlSlider(long x, long y, long width, long height,
                    const char* textureback = NULL,
                    const char* texture = NULL,
                    const char* texturefocus = NULL);

      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief Returns a float of the percent of the slider.
      ///
      /// @return                       float - Percent of slider
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// print self.slider.getPercent()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      float getPercent();

      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief Sets the percent of the slider.
      ///
      /// @param[in] pct                float - Percent value of slider
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.slider.setPercent(50)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setPercent(float pct);

#ifndef SWIG
      std::string strTextureBack;
      std::string strTexture;
      std::string strTextureFoc;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      inline ControlSlider() {}
#endif
    };
    /// @}
  }
}

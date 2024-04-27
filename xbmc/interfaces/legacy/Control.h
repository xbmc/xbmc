/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Alternative.h"
#include "ListItem.h"
#include "Tuple.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIFont.h"
#include "input/actions/ActionIDs.h"
#include "swighelper.h"
#include "utils/ColorUtils.h"

#include <vector>


// hardcoded offsets for button controls (and controls that use button controls)
// ideally they should be dynamically read in as with all the other properties.
#define CONTROL_TEXT_OFFSET_X 10
#define CONTROL_TEXT_OFFSET_Y 2

namespace XBMCAddon
{
  namespace xbmcgui
  {

    /// \defgroup python_xbmcgui_control Control
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief **Code based skin access.**
    ///
    /// Offers classes and functions that manipulate the add-on gui controls.
    ///
    ///-------------------------------------------------------------------------
    ///
    /// \python_class{ Control() }
    ///
    /// **Code based skin access.**
    ///
    /// Kodi is noted as having a very flexible and robust framework for its
    /// GUI, making theme-skinning and personal customization very accessible.
    /// Users can create their own skin (or modify an existing skin) and share
    /// it with others.
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
      Control() = default;

    public:
      ~Control() override;

#ifndef SWIG
      virtual CGUIControl* Create();
#endif

      // currently we only accept messages from a button or controllist with a select action
      virtual bool canAcceptMessages(int actionId) { return false; }

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ getId() }
      /// Returns the control's current id as an integer.
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
      getId()
#else
      virtual int getId() { return iControlId; }
#endif

      inline bool operator==(const Control& other) const { return iControlId == other.iControlId; }
      inline bool operator>(const Control& other) const { return iControlId > other.iControlId; }
      inline bool operator<(const Control& other) const { return iControlId < other.iControlId; }

      // hack this because it returns a tuple
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ getPosition() }
      /// Returns the control's current position as a x,y integer tuple.
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
      getPosition();
#else
      virtual std::vector<int> getPosition();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ getX() }
      /// Returns the control's current X position.
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
      getX();
#else
      int getX() { return dwPosX; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ getY() }
      /// Returns the control's current Y position.
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
      getY();
#else
      int getY() { return dwPosY; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ getHeight() }
      /// Returns the control's current height as an integer.
      ///
      /// @return                       int - Current height
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// height = self.button.getHeight()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getHeight();
#else
      virtual int getHeight() { return dwHeight; }
#endif

      // getWidth() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ getWidth() }
      /// Returns the control's current width as an integer.
      ///
      /// @return                       int - Current width
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// width = self.button.getWidth()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getWidth();
#else
      virtual int getWidth() { return dwWidth; }
#endif

      // setEnabled() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setEnabled(enabled) }
      /// Sets the control's enabled/disabled state.
      ///
      /// @param enabled            bool - True=enabled / False=disabled.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setEnabled(False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setEnabled(...);
#else
      virtual void setEnabled(bool enabled);
#endif

      // setVisible() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setVisible(visible) }
      /// Sets the control's visible/hidden state.
      /// \anchor python_xbmcgui_control_setVisible
      ///
      /// @param visible            bool - True=visible / False=hidden.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 You can now define the visible state of a control before it being
      /// added to a window. This value will be taken into account when the control is later
      /// added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setVisible(False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setVisible(...);
#else
      virtual void setVisible(bool visible);
#endif

      // isVisible() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ isVisible() }
      /// Get the control's visible/hidden state with respect to the container/window
      ///
      /// @note If a given control is set visible (c.f. \ref python_xbmcgui_control_setVisible "setVisible()"
      /// but was not yet added to a window, this method will return `False` (the control is not visible yet since
      /// it was not added to the window).
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// if self.button.isVisible():
      ///     ...
      /// ~~~~~~~~~~~~~
      ///
      isVisible(...);
#else
      virtual bool isVisible();
#endif

      // setVisibleCondition() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setVisibleCondition(visible[,allowHiddenFocus]) }
      /// Sets the control's visible condition.
      ///
      /// Allows Kodi to control the visible status of the control.
      ///
      /// [List of Conditions](http://kodi.wiki/view/List_of_Boolean_Conditions)
      ///
      /// @param visible            string - Visible condition
      /// @param allowHiddenFocus   [opt] bool - True=gains focus even if
      ///                               hidden
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setVisibleCondition(visible[,allowHiddenFocus])
      /// self.button.setVisibleCondition('[Control.IsVisible(41) + !Control.IsVisible(12)]', True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setVisibleCondition(...);
#else
      virtual void setVisibleCondition(const char* visible, bool allowHiddenFocus = false);
#endif

      // setEnableCondition() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setEnableCondition(enable) }
      /// Sets the control's enabled condition.
      ///
      /// Allows Kodi to control the enabled status of the control.
      ///
      /// [List of Conditions](http://kodi.wiki/view/List_of_Boolean_Conditions)
      ///
      /// @param enable             string - Enable condition.
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
      setEnableCondition(...);
#else
      virtual void setEnableCondition(const char* enable);
#endif

      // setAnimations() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setAnimations([(event, attr,)*]) }
      /// Sets the control's animations.
      ///
      /// <b>[(event,attr,)*]</b>: list - A list of tuples consisting of event
      /// and attributes pairs.
      ///
      /// [Animating your skin](http://kodi.wiki/view/Animating_Your_Skin)
      ///
      /// @param event              string - The event to animate.
      /// @param attr               string - The whole attribute string
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
      setAnimations(...);
#else
      virtual void setAnimations(const std::vector< Tuple<String,String> >& eventAttr);
#endif

      // setPosition() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setPosition(x, y) }
      /// Sets the controls position.
      ///
      /// @param x                  integer - x coordinate of control.
      /// @param y                  integer - y coordinate of control.
      ///
      /// @note You may use negative integers. (e.g sliding a control into view)
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setPosition(100, 250)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setPosition(...);
#else
      virtual void setPosition(long x, long y);
#endif

      // setWidth() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setWidth(width) }
      /// Sets the controls width.
      ///
      /// @param width                integer - width of control.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.image.setWidth(100)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setWidth(...);
#else
      virtual void setWidth(long width);
#endif

      // setHeight() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setHeight(height) }
      /// Sets the controls height.
      ///
      /// @param height               integer - height of control.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.image.setHeight(100)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setHeight(...);
#else
      virtual void setHeight(long height);
#endif

      // setNavigation() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ setNavigation(up, down, left, right) }
      /// Sets the controls navigation.
      ///
      /// @param up                 control object - control to navigate to on up.
      /// @param down               control object - control to navigate to on down.
      /// @param left               control object - control to navigate to on left.
      /// @param right              control object - control to navigate to on right.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      /// @note Same as controlUp(), controlDown(), controlLeft(), controlRight().
      ///       Set to self to disable navigation for that direction.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.setNavigation(self.button1, self.button2, self.button3, self.button4)
      /// ...
      /// ~~~~~~~~~~~~~
      //
      setNavigation(...);
#else
      virtual void setNavigation(const Control* up, const Control* down,
                                 const Control* left, const Control* right);
#endif

      // controlUp() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ controlUp(control) }
      /// Sets the controls up navigation.
      ///
      /// @param control            control object - control to navigate to on up.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlUp(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      controlUp(...);
#else
      virtual void controlUp(const Control* up);
#endif

      // controlDown() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ controlDown(control) }
      /// Sets the controls down navigation.
      ///
      /// @param control            control object - control to navigate to on down.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlDown(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      controlDown(...);
#else
      virtual void controlDown(const Control* control);
#endif

      // controlLeft() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ controlLeft(control) }
      /// Sets the controls left navigation.
      ///
      /// @param control            control object - control to navigate to on left.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlLeft(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      controlLeft(...);
#else
      virtual void controlLeft(const Control* control);
#endif

      // controlRight() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control
      /// @brief \python_func{ controlRight(control) }
      /// Sets the controls right navigation.
      ///
      /// @param control            control object - control to navigate to on right.
      /// @throw TypeError              if one of the supplied arguments is not a
      ///                               control type.
      /// @throw ReferenceError         if one of the controls is not added to a
      ///                               window.
      ///
      ///
      /// @note You can also use setNavigation(). Set to self to disable navigation.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.button.controlRight(self.button1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      controlRight(...);
#else
      virtual void controlRight(const Control* control);
#endif

#ifndef SWIG
      int iControlId = 0;
      int iParentId = 0;
      int dwPosX = 0;
      int dwPosY = 0;
      int dwWidth = 0;
      int dwHeight = 0;
      int iControlUp = 0;
      int iControlDown = 0;
      int iControlLeft = 0;
      int iControlRight = 0;
      std::string m_label{};
      bool m_visible{true};
      CGUIControl* pGUIControl = nullptr;
#endif

    };
    /// @}

    /// \defgroup python_xbmcgui_control_spin Subclass - ControlSpin
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used for cycling up/down controls.**
    ///
    /// Offers classes and functions that manipulate the add-on gui controls.
    ///
    ///-------------------------------------------------------------------------
    ///
    /// \python_class{ ControlSpin() }
    ///
    /// **Code based skin access.**
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
    ///-------------------------------------------------------------------------
    ///
    ///
    class ControlSpin : public Control
    {
    public:
      ~ControlSpin() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_spin
      /// @brief \python_func{ setTextures(up, down, upFocus, downFocus) }
      /// Sets textures for this control.
      ///
      /// Texture are image files that are used for example in the skin
      ///
      /// @warning **Not working yet**.
      ///
      /// @param up                 label - for the up arrow
      ///                               when it doesn't have focus.
      /// @param down               label - for the down button
      ///                               when it is not focused.
      /// @param upFocus            label - for the up button
      ///                               when it has focus.
      /// @param downFocus          label - for the down button
      ///                               when it has focus.
      /// @param upDisabled         label - for the up arrow
      ///                               when the button is disabled.
      /// @param downDisabled       label - for the up arrow
      ///                               when the button is disabled.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setTextures(up, down, upFocus, downFocus, upDisabled, downDisabled)
      ///
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setTextures(...);
#else
      virtual void setTextures(const char* up, const char* down,
                               const char* upFocus,
                               const char* downFocus,
                               const char* upDisabled, const char* downDisabled);
#endif

#ifndef SWIG
      UTILS::COLOR::Color color;
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

    /// \defgroup python_xbmcgui_control_label Subclass - ControlLabel
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used to show some lines of text.**
    ///
    /// \python_class{ ControlLabel(x, y, width, height, label[, font, textColor,
    ///                             disabledColor, alignment, hasPath, angle]) }
    ///
    /// The label control is used for displaying text in Kodi. You can choose
    /// the font, size, colour, location and contents of the text to be
    /// displayed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param label                string or unicode - text string.
    /// @param font                 [opt] string - font used for label
    ///                                 text. (e.g. 'font13')
    /// @param textColor            [opt] hexstring - color of enabled
    ///                                 label's label. (e.g. '0xFFFFFFFF')
    /// @param disabledColor        [opt] hexstring - color of disabled
    ///                                 label's label. (e.g. '0xFFFF3300')
    /// @param alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Definition name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// | XBFONT_TRUNCATED_LEFT | 0x00000020 | Truncated text from left
    /// @param hasPath              [opt] bool - True=stores a
    ///                                 path / False=no path
    /// @param angle                [opt] integer - angle of control.
    ///                                 (<b>+</b> rotates CCW, <b>-</b> rotates C)
    ///
    ///
    ///-------------------------------------------------------------------------
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

      ~ControlLabel() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_label
      /// @brief \python_func{ getLabel() }
      /// Returns the text value for this label.
      ///
      /// @return                       This label
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// label = self.label.getLabel()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getLabel();
#else
      virtual String getLabel();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_label
      /// @brief \python_func{ setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor, label2]) }
      /// Sets text for this label.
      ///
      /// @param label              string or unicode - text string.
      /// @param font               [opt] string - font used for label text.
      ///                               (e.g. 'font13')
      /// @param textColor          [opt] hexstring - color of enabled
      ///                               label's label. (e.g. '0xFFFFFFFF')
      /// @param disabledColor      [opt] hexstring - color of disabled
      ///                               label's label. (e.g. '0xFFFF3300')
      /// @param shadowColor        [opt] hexstring - color of button's
      ///                               label's shadow. (e.g. '0xFF000000')
      /// @param focusedColor       [opt] hexstring - color of focused
      ///                               button's label. (e.g. '0xFF00FFFF')
      /// @param label2             [opt] string or unicode - text string.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.label.setLabel('Status')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setLabel(...);
#else
      virtual void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);
#endif

#ifndef SWIG
      ControlLabel() = default;

      std::string strFont;
      std::string strText;
      UTILS::COLOR::Color textColor;
      UTILS::COLOR::Color disabledColor;
      uint32_t align;
      bool bHasPath = false;
      int iAngle = 0;

      CGUIControl* Create() override;

#endif
    };
    /// @}

    // ControlEdit class
    /// \defgroup python_xbmcgui_control_edit Subclass - ControlEdit
    /// \ingroup python_xbmcgui_control
    /// @{
    /// **Used as an input control for the osd keyboard and other input fields.**
    ///
    /// \python_class{ ControlEdit(x, y, width, height, label[, font, textColor,
    ///              disabledColor, alignment, focusTexture, noFocusTexture]) }
    ///
    /// The edit control allows a user to input text in Kodi. You can choose the
    /// font, size, colour, location and header of the text to be displayed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param label                string or unicode - text string.
    /// @param font                 [opt] string - font used for label text.
    ///                                 (e.g. 'font13')
    /// @param textColor            [opt] hexstring - color of enabled
    ///                                 label's label. (e.g. '0xFFFFFFFF')
    /// @param disabledColor        [opt] hexstring - color of disabled
    ///                                 label's label. (e.g. '0xFFFF3300')
    /// @param alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Definition name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// | XBFONT_TRUNCATED_LEFT | 0x00000020 | Truncated text from left
    /// @param focusTexture         [opt] string - filename for focus texture.
    /// @param noFocusTexture       [opt] string - filename for no focus texture.
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
    /// @python_v18 Deprecated **isPassword**
    /// @python_v19 Removed **isPassword**
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
                  const char* noFocusTexture = NULL);


      // setLabel() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_edit
      /// @brief \python_func{ setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor, label2]) }
      /// Sets text heading for this edit control.
      ///
      /// @param label              string or unicode - text string.
      /// @param font               [opt] string - font used for label text.
      ///                               (e.g. 'font13')
      /// @param textColor          [opt] hexstring - color of enabled
      ///                               label's label. (e.g. '0xFFFFFFFF')
      /// @param disabledColor      [opt] hexstring - color of disabled
      ///                               label's label. (e.g. '0xFFFF3300')
      /// @param shadowColor        [opt] hexstring - color of button's
      ///                               label's shadow. (e.g. '0xFF000000')
      /// @param focusedColor       [opt] hexstring - color of focused
      ///                               button's label. (e.g. '0xFF00FFFF')
      /// @param label2             [opt] string or unicode - text string.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.edit.setLabel('Status')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setLabel(...);
#else
      virtual void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);
#endif

      // getLabel() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_edit
      /// @brief \python_func{ getLabel() }
      /// Returns the text heading for this edit control.
      ///
      /// @return                       Heading text
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// label = self.edit.getLabel()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getLabel();
#else
      virtual String getLabel();
#endif

      // setText() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_edit
      /// @brief \python_func{ setText(value) }
      /// Sets text value for this edit control.
      ///
      /// @param value              string or unicode - text string.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.edit.setText('online')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setText(...);
#else
      virtual void setText(const String& text);
#endif

      // getText() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_edit
      /// @brief \python_func{ getText() }
      /// Returns the text value for this edit control.
      ///
      /// @return                       Text value of control
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// value = self.edit.getText()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getText();
#else
      virtual String getText();
#endif

#ifndef SWIG
      ControlEdit() = default;

      std::string strFont;
      std::string strText;
      std::string strTextureFocus;
      std::string strTextureNoFocus;
      UTILS::COLOR::Color textColor;
      UTILS::COLOR::Color disabledColor;
      uint32_t align;

      CGUIControl* Create() override;
#endif

      // setType() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_edit
      /// @brief \python_func{ setType(type, heading) }
      /// Sets the type of this edit control.
      ///
      /// @param type              integer - type of the edit control.
      /// | Param                                         | Definition                                  |
      /// |-----------------------------------------------|:--------------------------------------------|
      /// | xbmcgui.INPUT_TYPE_TEXT                       | (standard keyboard)
      /// | xbmcgui.INPUT_TYPE_NUMBER                     | (format: #)
      /// | xbmcgui.INPUT_TYPE_DATE                       | (format: DD/MM/YYYY)
      /// | xbmcgui.INPUT_TYPE_TIME                       | (format: HH:MM)
      /// | xbmcgui.INPUT_TYPE_IPADDRESS                  | (format: #.#.#.#)
      /// | xbmcgui.INPUT_TYPE_PASSWORD                   | (input is masked)
      /// | xbmcgui.INPUT_TYPE_PASSWORD_MD5               | (input is masked, return md5 hash of input)
      /// | xbmcgui.INPUT_TYPE_SECONDS                    | (format: SS or MM:SS or HH:MM:SS or MM min)
      /// | xbmcgui.INPUT_TYPE_PASSWORD_NUMBER_VERIFY_NEW | (numeric input is masked)
      /// @param heading           string or unicode - heading that will be used for to numeric or
      ///                                              keyboard dialog when the edit control is clicked.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 New option added to mask numeric input.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.edit.setType(xbmcgui.INPUT_TYPE_TIME, 'Please enter the time')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setType(...);
#else
      virtual void setType(int type, const String& heading);
#endif
    };
    /// @}

    // ControlList class
    /// \defgroup python_xbmcgui_control_list Subclass - ControlList
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used for a scrolling lists of items. Replaces the list control.**
    ///
    /// \python_class{ ControlList(x, y, width, height[, font, textColor, buttonTexture, buttonFocusTexture,
    ///             selectedColor, imageWidth, imageHeight, itemTextXOffset, itemTextYOffset,
    ///             itemHeight, space, alignmentY, shadowColor]) }
    ///
    /// The list container is one of several containers used to display items
    /// from file lists in various ways. The list container is very
    /// flexible - it's only restriction is that it is a list - i.e. a single
    /// column or row of items. The layout of the items is very flexible and
    /// is up to the skinner.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                        integer - x coordinate of control.
    /// @param y                        integer - y coordinate of control.
    /// @param width                    integer - width of control.
    /// @param height                   integer - height of control.
    /// @param font                     [opt] string - font used for items label. (e.g. 'font13')
    /// @param textColor                [opt] hexstring - color of items label. (e.g. '0xFFFFFFFF')
    /// @param buttonTexture            [opt] string - filename for focus texture.
    /// @param buttonFocusTexture       [opt] string - filename for no focus texture.
    /// @param selectedColor            [opt] integer - x offset of label.
    /// @param imageWidth               [opt] integer - width of items icon or thumbnail.
    /// @param imageHeight              [opt] integer - height of items icon or thumbnail.
    /// @param itemTextXOffset          [opt] integer - x offset of items label.
    /// @param itemTextYOffset          [opt] integer - y offset of items label.
    /// @param itemHeight               [opt] integer - height of items.
    /// @param space                    [opt] integer - space between items.
    /// @param alignmentY               [opt] integer - Y-axis alignment of items label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Definition name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// | XBFONT_TRUNCATED_LEFT | 0x00000020 | Truncated text from left
    /// @param shadowColor              [opt] hexstring - color of items
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
      void internAddListItem(const AddonClass::Ref<ListItem>& listitem, bool sendMessage);

    public:
      ControlList(long x, long y, long width, long height, const char* font = NULL,
                  const char* textColor = NULL, const char* buttonTexture = NULL,
                  const char* buttonFocusTexture = NULL,
                  const char* selectedColor = NULL,
                  long _imageWidth=10, long _imageHeight=10, long _itemTextXOffset = CONTROL_TEXT_OFFSET_X,
                  long _itemTextYOffset = CONTROL_TEXT_OFFSET_Y, long _itemHeight = 27, long _space = 2,
                  long _alignmentY = XBFONT_CENTER_Y);

      ~ControlList() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ addItem(item) }
      /// Add a new item to this list control.
      ///
      /// @param item                     string, unicode or ListItem - item to add.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.addItem('Reboot Kodi')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addItem(...);
#else
      virtual void addItem(const Alternative<String, const XBMCAddon::xbmcgui::ListItem* > & item, bool sendMessage = true);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ addItems(items) }
      /// Adds a list of listitems or strings to this list control.
      ///
      /// @param items                      List - list of strings, unicode objects or ListItems to add.
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      /// Large lists benefit considerably, than using the standard addItem()
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.addItems(items=listitems)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addItems(...);
#else
      virtual void addItems(const std::vector<Alternative<String, const XBMCAddon::xbmcgui::ListItem* > > & items);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ selectItem(item) }
      /// Select an item by index number.
      ///
      /// @param item                     integer - index number of the item to select.
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
      selectItem(...);
#else
      virtual void selectItem(long item);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ removeItem(index) }
      /// Remove an item by index number.
      ///
      /// @param index                    integer - index number of the item to remove.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v13 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.removeItem(12)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      removeItem(...);
#else
      virtual void removeItem(int index);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ reset() }
      /// Clear all ListItems in this control list.
      ///
      /// @warning Calling `reset()` will destroy any `ListItem` objects in the
      ///          `ControlList` if not hold by any other class. Make sure you
      ///          you don't call `addItems()` with the previous `ListItem` references
      ///          after calling `reset()`. If you need to preserve the `ListItem` objects after
      ///          `reset()` make sure you store them as members of your `WindowXML` class (see examples).
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Examples:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.reset()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      /// The below example shows you how you can reset the `ControlList` but this time avoiding `ListItem` object
      /// destruction. The example assumes `self` as a `WindowXMLDialog` instance containing a `ControlList`
      /// with id = 800. The class preserves the `ListItem` objects in a class member variable.
      ///
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # Get all the ListItem objects in the control
      /// self.list_control = self.getControl(800) # ControlList object
      /// self.listitems = [self.list_control.getListItem(item) for item in range(0, self.list_control.size())]
      /// # Reset the ControlList control
      /// self.list_control.reset()
      /// #
      /// # do something with your ListItem objects here (e.g. sorting.)
      /// # ...
      /// #
      /// # Add them again to the ControlList
      /// self.list_control.addItems(self.listitems)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      reset();
#else
      virtual void reset();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ getSpinControl() }
      /// Returns the associated ControlSpin object.
      ///
      /// @warning Not working completely yet\n
      ///        After adding this control list to a window it is not possible to change
      ///        the settings of this spin control.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// ctl = cList.getSpinControl()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getSpinControl();
#else
      virtual Control* getSpinControl();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ getSelectedPosition() }
      /// Returns the position of the selected item as an integer.
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
      getSelectedPosition();
#else
      virtual long getSelectedPosition();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ getSelectedItem() }
      /// Returns the selected item as a ListItem object.
      ///
      /// @return                       The selected item
      ///
      ///
      /// @note Same as getSelectedPosition(), but instead of an integer a ListItem object
      ///        is returned. Returns None for empty lists.\n
      ///        See windowexample.py on how to use this.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// item = cList.getSelectedItem()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getSelectedItem();
#else
      virtual XBMCAddon::xbmcgui::ListItem* getSelectedItem();
#endif

      // setImageDimensions() method
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ setImageDimensions(imageWidth, imageHeight) }
      /// Sets the width/height of items icon or thumbnail.
      ///
      /// @param imageWidth               [opt] integer - width of items icon or thumbnail.
      /// @param imageHeight              [opt] integer - height of items icon or thumbnail.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setImageDimensions(18, 18)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setImageDimensions(...);
#else
      virtual void setImageDimensions(long imageWidth,long imageHeight);
#endif

      // setItemHeight() method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @brief \python_func{ setItemHeight(itemHeight) }
      /// Sets the height of items.
      ///
      /// @param itemHeight               integer - height of items.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setItemHeight(25)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setItemHeight(...);
#else
      virtual void setItemHeight(long height);
#endif

      // setSpace() method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ setSpace(space) }
      /// Sets the space between items.
      ///
      /// @param space                    [opt] integer - space between items.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setSpace(5)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setSpace(...);
#else
      virtual void setSpace(int space);
#endif

      // setPageControlVisible() method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ setPageControlVisible(visible) }
      /// Sets the spin control's visible/hidden state.
      ///
      /// @param visible                  boolean - True=visible / False=hidden.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setPageControlVisible(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setPageControlVisible(...);
#else
      virtual void setPageControlVisible(bool visible);
#endif

      // size() method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ size() }
      /// Returns the total number of items in this list control as an integer.
      ///
      /// @return                       Total number of items
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cnt = cList.size()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      size();
#else
      virtual long size();
#endif

      // getItemHeight() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ getItemHeight() }
      /// Returns the control's current item height as an integer.
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
      getItemHeight();
#else
      virtual long getItemHeight();
#endif

      // getSpace() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ getSpace() }
      /// Returns the control's space between items as an integer.
      ///
      /// @return                       Space between items
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// gap = self.cList.getSpace()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getSpace();
#else
      virtual long getSpace();
#endif

      // getListItem() method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ getListItem(index) }
      /// Returns a given ListItem in this List.
      ///
      /// @param index              integer - index number of item to return.
      /// @return                       List item
      /// @throw ValueError             if index is out of range.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem = cList.getListItem(6)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getListItem(...);
#else
      virtual XBMCAddon::xbmcgui::ListItem* getListItem(int index);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_list
      /// @brief \python_func{ setStaticContent(items) }
      /// Fills a static list with a list of listitems.
      ///
      /// @param items                      List - list of listitems to add.
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// cList.setStaticContent(items=listitems)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setStaticContent(...);
#else
      virtual void setStaticContent(const ListItemList* items);
#endif

#ifndef SWIG
      void sendLabelBind(int tail);

      bool canAcceptMessages(int actionId) override
      { return ((actionId == ACTION_SELECT_ITEM) | (actionId == ACTION_MOUSE_LEFT_CLICK)); }

      // This is called from AddonWindow.cpp but shouldn't be available
      //  to the scripting languages.
      ControlList() = default;

      std::vector<AddonClass::Ref<ListItem> > vecItems;
      std::string strFont;
      AddonClass::Ref<ControlSpin> pControlSpin;

      UTILS::COLOR::Color textColor;
      UTILS::COLOR::Color selectedColor;
      std::string strTextureButton;
      std::string strTextureButtonFocus;

      int imageHeight = 0;
      int imageWidth = 0;
      int itemHeight = 0;
      int space = 0;

      int itemTextOffsetX = 0;
      int itemTextOffsetY = 0;
      uint32_t alignmentY;

      CGUIControl* Create() override;
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
    /// \python_class{ ControlFadeLabel(x, y, width, height[, font, textColor, alignment]) }
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
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param font                 [opt] string - font used for label text. (e.g. 'font13')
    /// @param textColor            [opt] hexstring - color of fadelabel's labels. (e.g. '0xFFFFFFFF')
    /// @param alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Definition name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// | XBFONT_TRUNCATED_LEFT | 0x00000020 | Truncated text from left
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
#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcgui_control_fadelabel
      /// @brief \python_func{ addLabel(label) }
      /// Add a label to this control for scrolling.
      ///
      /// @param label                string or unicode - text string to add.
      ///
      /// @note To remove added text use `reset()` for them.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.fadelabel.addLabel('This is a line of text that can scroll.')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addLabel(...);
#else
      virtual void addLabel(const String& label);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_fadelabel
      /// @brief \python_func{ setScrolling(scroll) }
      /// Set scrolling. If set to false, the labels won't scroll.
      /// Defaults to true.
      ///
      /// @param scroll                boolean - True = enabled / False = disabled
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.fadelabel.setScrolling(False)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setScrolling(...);
#else
      virtual void setScrolling(bool scroll);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_label
      /// @brief \python_func{ reset() }
      /// Clear this fade label.
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.fadelabel.reset()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      reset();
#else
      virtual void reset();
#endif

#ifndef SWIG
      std::string strFont;
      UTILS::COLOR::Color textColor;
      std::vector<std::string> vecLabels;
      uint32_t align;

      CGUIControl* Create() override;

      ControlFadeLabel() = default;
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
    /// \python_class{ ControlTextBox(x, y, width, height[, font, textColor]) }
    ///
    /// The text box is used for showing a large multipage piece of text in Kodi.
    /// You can choose the position, size, and look of the text.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param font                 [opt] string - font used for text. (e.g. 'font13')
    /// @param textColor            [opt] hexstring - color of textbox's text. (e.g. '0xFFFFFFFF')
    ///
    /// @note You can use the above as keywords for arguments and skip certain optional arguments.\n
    ///        Once you use a keyword, all following arguments require the keyword.\n
    ///        After you create the control, you need to add it to the window with addControl().
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// # ControlTextBox(x, y, width, height[, font, textColor])
    /// self.textbox = xbmcgui.ControlTextBox(100, 250, 300, 300, textColor='0xFFFFFFFF')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    /// As stated above, the GUI control is only created once added to a window. The example
    /// below shows how a ControlTextBox can be created, added to the current window and
    /// have some of its properties changed.
    ///
    /// **Extended example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// textbox = xbmcgui.ControlTextBox(100, 250, 300, 300, textColor='0xFFFFFFFF')
    /// window = xbmcgui.Window(xbmcgui.getCurrentWindowId())
    /// window.addControl(textbox)
    /// textbox.setText("My Text Box")
    /// textbox.scroll()
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
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief \python_func{ setText(text) }
      /// Sets the text for this textbox.
      /// \anchor python_xbmcgui_control_textbox_settext
      ///
      /// @param text                 string  - text string.
      ///
      ///-----------------------------------------------------------------------
      ///
      /// @python_v19 setText can now be used before adding the control to the window (the defined
      /// value is taken into consideration when the control is created)
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setText(text)
      /// self.textbox.setText('This is a line of text that can wrap.')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setText(...);
#else
      virtual void setText(const String& text);
#endif

      // getText() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief \python_func{ getText() }
      /// Returns the text value for this textbox.
      ///
      /// @return                       To get text from box
      ///
      ///-----------------------------------------------------------------------
      ///
      /// @python_v19 getText() can now be used before adding the control to the window
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getText()
      /// text = self.text.getText()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getText();
#else
      virtual String getText();
#endif

      // reset() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief \python_func{ reset() }
      /// Clear's this textbox.
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 reset() will reset any text defined for this control even before you add the control to the window
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # reset()
      /// self.textbox.reset()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      reset();
#else
      virtual void reset();
#endif

      // scroll() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief \python_func{ scroll(id) }
      /// Scrolls to the given position.
      ///
      /// @param id                 integer - position to scroll to.
      ///
      /// @note scroll() only works after the control is added to a window.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # scroll(position)
      /// self.textbox.scroll(10)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      scroll(...);
#else
      virtual void scroll(long id);
#endif

      // autoScroll() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_textbox
      /// @brief \python_func{ autoScroll(delay, time, repeat) }
      /// Set autoscrolling times.
      ///
      /// @param delay                 integer - Scroll delay (in ms)
      /// @param time                  integer - Scroll time (in ms)
      /// @param repeat                integer - Repeat time
      ///
      /// @note autoScroll only works after you add the control to a window.
      ///
      ///-----------------------------------------------------------------------
      ///
      /// @python_v15 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.textbox.autoScroll(1, 2, 1)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      autoScroll(...);
#else
      virtual void autoScroll(int delay, int time, int repeat);
#endif

#ifndef SWIG
      std::string strFont;
      UTILS::COLOR::Color textColor;

      CGUIControl* Create() override;

      ControlTextBox() = default;
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
    /// \python_class{ ControlImage(x, y, width, height, filename[, aspectRatio, colorDiffuse]) }
    ///
    /// The image control is used for displaying images in Kodi. You can choose
    /// the position, size, transparency and contents of the image to be
    /// displayed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param filename             string - image filename.
    /// @param aspectRatio          [opt] integer - (values 0 = stretch
    ///                                 (default), 1 = scale up (crops),
    ///                                 2 = scale down (black bar)
    /// @param colorDiffuse         hexString - (example, '0xC0FF0000' (red tint))
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

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_image
      /// @brief \python_func{ setImage(filename[, useCache]) }
      /// Changes the image.
      ///
      /// @param filename             string - image filename.
      /// @param useCache             [opt] bool - True=use cache (default) /
      ///                                 False=don't use cache.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v13 Added new option **useCache**.
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
      setImage(...);
#else
      virtual void setImage(const char* imageFilename, const bool useCache = true);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_image
      /// @brief \python_func{ setColorDiffuse(colorDiffuse) }
      /// Changes the images color.
      ///
      /// @param colorDiffuse         hexString - (example, '0xC0FF0000'
      ///                                 (red tint))
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setColorDiffuse(colorDiffuse)
      /// self.image.setColorDiffuse('0xC0FF0000')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setColorDiffuse(...);
#else
      virtual void setColorDiffuse(const char* hexString);
#endif

#ifndef SWIG
      ControlImage() = default;

      std::string strFileName;
      int aspectRatio = 0;
      UTILS::COLOR::Color colorDiffuse;

      CGUIControl* Create() override;
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
    /// \python_class{ ControlProgress(x, y, width, height, filename[, texturebg, textureleft, texturemid, textureright, textureoverlay]) }
    ///
    /// The progress control is used to show the progress of an item that may
    /// take a long time, or to show how far through a movie you are. You can
    /// choose the position, size, and look of the progress control.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param filename             string - image filename.
    /// @param texturebg            [opt] string - specifies the image file
    ///                                 whichshould be displayed in the
    ///                                 background of the progress control.
    /// @param textureleft          [opt] string - specifies the image file
    ///                                 whichshould be displayed for the left
    ///                                 side of the progress bar. This is
    ///                                 rendered on the left side of the bar.
    /// @param texturemid           [opt] string - specifies the image file
    ///                                 which should be displayed for the middl
    ///                                 portion of the progress bar. This is
    ///                                 the `fill` texture used to fill up the
    ///                                 bar. It's positioned on the right of
    ///                                 the `<lefttexture>` texture, and fills
    ///                                 the gap between the `<lefttexture>` and
    ///                                 `<righttexture>` textures, depending on
    ///                                 how far progressed the item is.
    /// @param textureright         [opt] string - specifies the image file
    ///                                 which should be displayed for the right
    ///                                 side of the progress bar. This is
    ///                                 rendered on the right side of the bar.
    /// @param textureoverlay       [opt] string - specifies the image file
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

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_progress
      /// @brief \python_func{ setPercent(percent) }
      /// Sets the percentage of the progressbar to show.
      ///
      /// @param percent             float - percentage of the bar to show.
      ///
      ///
      /// @note valid range for percent is 0-100
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setPercent(percent)
      /// self.progress.setPercent(60)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setPercent(...);
#else
      virtual void setPercent(float pct);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_progress
      /// @brief \python_func{ getPercent() }
      /// Returns a float of the percent of the progress.
      ///
      /// @return                       Percent position
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getPercent()
      /// print(self.progress.getPercent())
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getPercent();
#else
      virtual float getPercent();
#endif

#ifndef SWIG
      std::string strTextureLeft;
      std::string strTextureMid;
      std::string strTextureRight;
      std::string strTextureBg;
      std::string strTextureOverlay;
      int aspectRatio = 0;
      UTILS::COLOR::Color colorDiffuse;

      CGUIControl* Create() override;
      ControlProgress() = default;
#endif
    };
    /// @}

    // ControlButton class
    ///
    /// \defgroup python_xbmcgui_control_button Subclass - ControlButton
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief <b>A standard push button control.</b>
    ///
    /// \python_class{ ControlButton(x, y, width, height, label[, focusTexture, noFocusTexture, textOffsetX, textOffsetY,
    ///               alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor]) }
    ///
    /// The button control is used for creating push buttons in Kodi. You can
    /// choose the position, size, and look of the button, as well as choosing
    /// what action(s) should be performed when pushed.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param label                string or unicode - text string.
    /// @param focusTexture         [opt] string - filename for focus
    ///                                 texture.
    /// @param noFocusTexture       [opt] string - filename for no focus
    ///                                 texture.
    /// @param textOffsetX          [opt] integer - x offset of label.
    /// @param textOffsetY          [opt] integer - y offset of label.
    /// @param alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Definition name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// | XBFONT_TRUNCATED_LEFT | 0x00000020 | Truncated text from left
    /// @param font                 [opt] string - font used for label text.
    ///                                 (e.g. 'font13')
    /// @param textColor            [opt] hexstring - color of enabled
    ///                                 button's label. (e.g. '0xFFFFFFFF')
    /// @param disabledColor        [opt] hexstring - color of disabled
    ///                                 button's label. (e.g. '0xFFFF3300')
    /// @param angle                [opt] integer - angle of control.
    ///                                 (+ rotates CCW, - rotates CW)
    /// @param shadowColor          [opt] hexstring - color of button's
    ///                                 label's shadow. (e.g. '0xFF000000')
    /// @param focusedColor         [opt] hexstring - color of focused
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
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief \python_func{ setLabel([label, font, textColor, disabledColor, shadowColor, focusedColor, label2]) }
      /// Sets this buttons text attributes.
      ///
      /// @param label                [opt] string or unicode - text string.
      /// @param font                 [opt] string - font used for label text. (e.g. 'font13')
      /// @param textColor            [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')
      /// @param disabledColor        [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
      /// @param shadowColor          [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')
      /// @param focusedColor         [opt] hexstring - color of focused button's label. (e.g. '0xFFFFFF00')
      /// @param label2               [opt] string or unicode - text string.
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
      setLabel(...);
#else
      virtual void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);
#endif

      // setDisabledColor() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief \python_func{ setDisabledColor(disabledColor) }
      /// Sets this buttons disabled color.
      ///
      /// @param disabledColor        hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
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
      setDisabledColor(...);
#else
      virtual void setDisabledColor(const char* color);
#endif

      // getLabel() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief \python_func{ getLabel() }
      /// Returns the buttons label as a unicode string.
      ///
      /// @return                       Unicode string
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getLabel()
      /// label = self.button.getLabel()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getLabel();
#else
      virtual String getLabel();
#endif

      // getLabel2() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_button
      /// @brief \python_func{ getLabel2() }
      /// Returns the buttons label2 as a string.
      ///
      /// @return                       string of label 2
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getLabel2()
      /// label = self.button.getLabel2()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getLabel2();
#else
      virtual String getLabel2();
#endif

#ifndef SWIG
      bool canAcceptMessages(int actionId) override { return true; }

      int textOffsetX = 0;
      int textOffsetY = 0;
      UTILS::COLOR::Color align;
      std::string strFont;
      UTILS::COLOR::Color textColor;
      UTILS::COLOR::Color disabledColor;
      int iAngle = 0;
      int shadowColor = 0;
      int focusedColor = 0;
      std::string strText;
      std::string strText2;
      std::string strTextureFocus;
      std::string strTextureNoFocus;

      CGUIControl* Create() override;

      ControlButton() = default;
#endif
    };
    /// @}

    // ControlGroup class
    ///
    /// \defgroup python_xbmcgui_control_group Subclass - ControlGroup
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used to group controls together..**
    ///
    /// \python_class{ ControlGroup(x, y, width, height) }
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
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
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
      CGUIControl* Create() override;

      inline ControlGroup() = default;
#endif
    };
    /// @}

    // ControlRadioButton class
    ///
    /// \defgroup python_xbmcgui_control_radiobutton Subclass - ControlRadioButton
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **A radio button control (as used for on/off settings).**
    ///
    /// \python_class{ ControlRadioButton(x, y, width, height, label[, focusOnTexture, noFocusOnTexture,
    ///                   focusOffTexture, noFocusOffTexture, focusTexture, noFocusTexture,
    ///                   textOffsetX, textOffsetY, alignment, font, textColor, disabledColor]) }
    ///
    /// The radio button control is used for creating push button on/off
    /// settings in Kodi. You can choose the position, size, and look of the
    /// button, as well as the focused and unfocused radio textures. Used
    /// for settings controls.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control.
    /// @param y                    integer - y coordinate of control.
    /// @param width                integer - width of control.
    /// @param height               integer - height of control.
    /// @param label                string or unicode - text string.
    /// @param focusOnTexture       [opt] string - filename for radio ON
    ///                             focused texture.
    /// @param noFocusOnTexture     [opt] string - filename for radio ON not
    ///                             focused texture.
    /// @param focusOfTexture       [opt] string - filename for radio OFF
    ///                             focused texture.
    /// @param noFocusOffTexture    [opt] string - filename for radio OFF
    ///                             not focused texture.
    /// @param focusTexture         [opt] string - filename for focused button
    ///                             texture.
    /// @param noFocusTexture       [opt] string - filename for not focused button
    ///                             texture.
    /// @param textOffsetX          [opt] integer - horizontal text offset
    /// @param textOffsetY          [opt] integer - vertical text offset
    /// @param alignment            [opt] integer - alignment of label
    /// - \ref kodi_gui_font_alignment "Flags for alignment" used as bits to have several together:
    /// | Definition name   |   Bitflag  | Description                         |
    /// |-------------------|:----------:|:------------------------------------|
    /// | XBFONT_LEFT       | 0x00000000 | Align X left
    /// | XBFONT_RIGHT      | 0x00000001 | Align X right
    /// | XBFONT_CENTER_X   | 0x00000002 | Align X center
    /// | XBFONT_CENTER_Y   | 0x00000004 | Align Y center
    /// | XBFONT_TRUNCATED  | 0x00000008 | Truncated text
    /// | XBFONT_JUSTIFIED  | 0x00000010 | Justify text
    /// | XBFONT_TRUNCATED_LEFT | 0x00000020 | Truncated text from left
    /// @param font                 [opt] string - font used for label text.
    ///                             (e.g. 'font13')
    /// @param textColor            [opt] hexstring - color of label when control
    ///                             is enabled.
    ///                             radiobutton's label. (e.g. '0xFFFFFFFF')
    /// @param disabledColor        [opt] hexstring - color of label when control
    ///                             is disabled. (e.g. '0xFFFF3300')
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
    /// @python_v13 New function added.
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
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief \python_func{ setSelected(selected) }
      /// **Sets the radio buttons's selected status.**
      ///
      /// @param selected           bool - True=selected (on) / False=not
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
      setSelected(...);
#else
      virtual void setSelected(bool selected);
#endif

      // isSelected() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief \python_func{ isSelected() }
      /// Returns the radio buttons's selected status.
      ///
      /// @return                       True if selected on
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// is = self.radiobutton.isSelected()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      isSelected();
#else
      virtual bool isSelected();
#endif

      // setLabel() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief \python_func{ setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor]) }
      /// Sets the radio buttons text attributes.
      ///
      /// @param label              string or unicode - text string.
      /// @param font               [opt] string - font used for label
      ///                               text. (e.g. 'font13')
      /// @param textColor          [opt] hexstring - color of enabled radio
      ///                               button's label. (e.g. '0xFFFFFFFF')
      /// @param disabledColor      [opt] hexstring - color of disabled
      ///                               radio button's label. (e.g. '0xFFFF3300')
      /// @param shadowColor        [opt] hexstring - color of radio
      ///                               button's label's shadow.
      ///                               (e.g. '0xFF000000')
      /// @param focusedColor       [opt] hexstring - color of focused radio
      ///                               button's label. (e.g. '0xFFFFFF00')
      ///
      ///
      /// @note You can use the above as keywords for arguments and skip certain
      ///       optional arguments.\n
      ///       Once you use a keyword, all following arguments require the
      ///       keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor])
      /// self.radiobutton.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setLabel(...);
#else
      virtual void setLabel(const String& label = emptyString,
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);
#endif

      // setRadioDimension() Method
#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_radiobutton
      /// @brief \python_func{ setRadioDimension(x, y, width, height) }
      /// Sets the radio buttons's radio texture's position and size.
      ///
      /// @param x                  integer - x coordinate of radio texture.
      /// @param y                  integer - y coordinate of radio texture.
      /// @param width              integer - width of radio texture.
      /// @param height             integer - height of radio texture.
      ///
      ///
      /// @note You can use the above as keywords for arguments and skip certain
      /// optional arguments.\n
      /// Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.radiobutton.setRadioDimension(x=100, y=5, width=20, height=20)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setRadioDimension(...);
#else
      virtual void setRadioDimension(long x, long y, long width, long height);
#endif

#ifndef SWIG
      bool canAcceptMessages(int actionId) override { return true; }

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
      UTILS::COLOR::Color textColor;
      UTILS::COLOR::Color disabledColor;
      int textOffsetX = 0;
      int textOffsetY = 0;
     uint32_t align;
      int iAngle = 0;
      UTILS::COLOR::Color shadowColor;
      UTILS::COLOR::Color focusedColor;

      CGUIControl* Create() override;

      ControlRadioButton() = default;
#endif
    };
    /// @}

    /// \defgroup python_xbmcgui_control_slider Subclass - ControlSlider
    /// \ingroup python_xbmcgui_control
    /// @{
    /// @brief **Used for a volume slider.**
    ///
    /// \python_class{ ControlSlider(x, y, width, height[, textureback, texture, texturefocus, orientation, texturebackdisabled, texturedisabled]) }
    ///
    /// The slider control is used for things where a sliding bar best represents
    /// the operation at hand (such as a volume control or seek control). You can
    /// choose the position, size, and look of the slider control.
    ///
    /// @note This class include also all calls from \ref python_xbmcgui_control "Control"
    ///
    /// @param x                    integer - x coordinate of control
    /// @param y                    integer - y coordinate of control
    /// @param width                integer - width of control
    /// @param height               integer - height of control
    /// @param textureback          [opt] string - image filename
    /// @param texture              [opt] string - image filename
    /// @param texturefocus         [opt] string - image filename
    /// @param orientation          [opt] integer - orientation of slider (xbmcgui.HORIZONTAL / xbmcgui.VERTICAL (default))
    /// @param texturebackdisabled  [opt] string - image filename
    /// @param texturedisabled      [opt] string - image filename
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
    /// @python_v17 **orientation** option added.
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// self.slider = xbmcgui.ControlSlider(100, 250, 350, 40)
    /// ...
    /// ~~~~~~~~~~~~~
    class ControlSlider : public Control
    {
    public:
      ControlSlider(long x,
                    long y,
                    long width,
                    long height,
                    const char* textureback = NULL,
                    const char* texture = NULL,
                    const char* texturefocus = NULL,
                    int orientation = 1,
                    const char* texturebackdisabled = NULL,
                    const char* texturedisabled = NULL);

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief \python_func{ getPercent() }
      /// Returns a float of the percent of the slider.
      ///
      /// @return                       float - Percent of slider
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// print(self.slider.getPercent())
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getPercent();
#else
      virtual float getPercent();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief \python_func{ setPercent(pct) }
      /// Sets the percent of the slider.
      ///
      /// @param pct                float - Percent value of slider
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
      setPercent(...);
#else
      virtual void setPercent(float pct);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief \python_func{ getInt() }
      /// Returns the value of the slider.
      ///
      /// @return                   int - value of slider
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// print(self.slider.getInt())
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getInt();
#else
      virtual int getInt();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief \python_func{ setInt(value, min, delta, max) }
      /// Sets the range, value and step size of the slider.
      ///
      /// @param value              int - value of slider
      /// @param min                int - min of slider
      /// @param delta              int - step size of slider
      /// @param max                int - max of slider
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.slider.setInt(450, 200, 10, 900)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setInt(...);
#else
      virtual void setInt(int value, int min, int delta, int max);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief \python_func{ getFloat() }
      /// Returns the value of the slider.
      ///
      /// @return                   float - value of slider
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// print(self.slider.getFloat())
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getFloat();
#else
      virtual float getFloat();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_control_slider
      /// @brief \python_func{ setFloat(value, min, delta, max) }
      /// Sets the range, value and step size of the slider.
      ///
      /// @param value              float - value of slider
      /// @param min                float - min of slider
      /// @param delta              float - step size of slider
      /// @param max                float - max of slider
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.slider.setFloat(15.0, 10.0, 1.0, 20.0)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setFloat(...);
#else
      virtual void setFloat(float value, float min, float delta, float max);
#endif

#ifndef SWIG
      std::string strTextureBack;
      std::string strTextureBackDisabled;
      std::string strTexture;
      std::string strTextureFoc;
      std::string strTextureDisabled;
      int iOrientation;

      CGUIControl* Create() override;

      inline ControlSlider() = default;
#endif
    };
    /// @}
  }
}

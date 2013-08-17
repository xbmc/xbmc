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
#include "guilib/Key.h"

#include "Tuple.h"
#include "ListItem.h"
#include "swighelper.h"
#include "WindowException.h"

#include "ListItem.h"

// hardcoded offsets for button controls (and controls that use button controls)
// ideally they should be dynamically read in as with all the other properties.
#define CONTROL_TEXT_OFFSET_X 10
#define CONTROL_TEXT_OFFSET_Y 2

namespace XBMCAddon
{
  namespace xbmcgui
  {

    /**
     * Parent for control classes. The problem here is that Python uses 
     * references to this class in a dynamic typing way. For example,
     * you will find this type of python code frequently:
     *
     * window.getControl( 100 ).setLabel( "Stupid Dynamic Type")
     *
     * Notice that the 'getControl' call returns a 'Control' object.
     * In a dynamically typed language, the subsequent call to setLabel
     * works if the specific type of control has the method. The script
     * writer is often in a position to know more than the code about
     * the specific Control type (in the example, that control id 100
     * is a 'ControlLabel') where the C++ code is not.
     *
     * SWIG doesn't support this type of dynamic typing. The 'Control'
     * wrapper that's returned will wrap a ControlLabel but will not
     * have the 'setLabel' method on it. The only way to handle this is
     * to add all possible subclass methods to the parent class. This is
     * ugly but the alternative is nearly as ugly. It's particularly ugly
     * here because the majority of the methods are unique to the 
     * particular subclass.
     *
     * If anyone thinks they have a solution then let me know. The alternative
     * would be to have a set of 'getContol' methods, each one coresponding
     * to a type so that the downcast can be done in the native code. IOW
     * rather than a simple 'getControl' there would be a 'getControlLabel',
     * 'getControlRadioButton', 'getControlButton', etc.
     *
     * TODO:This later solution should be implemented for future scripting 
     * languages while the former will remain as deprecated functionality 
     * for Python. 
     */
    // We don't need the SWIGHIDDENVIRTUAL since this is not a director.
    class Control : public AddonClass
    {
    protected:
    public:
      Control(const char* classname) : AddonClass(classname),
                                       iControlId(0), iParentId(0), dwPosX(0), dwPosY(0), dwWidth(0),
                                       dwHeight(0), iControlUp(0), iControlDown(0), iControlLeft(0),
                                       iControlRight(0), pGUIControl(NULL) {}

      virtual ~Control();

#ifndef SWIG
      virtual CGUIControl* Create() throw (WindowException);
#endif

      // currently we only accept messages from a button or controllist with a select action
      virtual bool canAcceptMessages(int actionId) { return false; }

      /**
       * setLabel() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) DECL_UNIMP("Control");
      /**
       * reset() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void reset() DECL_UNIMP("Control");
      /**
       * removeItem() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void removeItem(int index) DECL_UNIMP2("Control",WindowException);
      /**
       * setSelected() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setSelected(bool selected) DECL_UNIMP("Control");
      /**
       * setPercent() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setPercent(float pct) DECL_UNIMP("Control");
      /**
       * setDisabledColor() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setDisabledColor(const char* color) DECL_UNIMP("Control");
      /**
       * getPercent() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual float getPercent() DECL_UNIMP("Control");
      /**
       * getLabel() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual String getLabel() DECL_UNIMP("Control");
      /**
       * getText() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual String getText() DECL_UNIMP("Control");
      /**
       * size() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual long size() DECL_UNIMP("Control");
      /**
       * setTextures() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setTextures(const char* up, const char* down, 
                               const char* upFocus, 
                               const char* downFocus) DECL_UNIMP("Control");
      /**
       * setText() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setText(const String& text) DECL_UNIMP("Control");
      /**
       * setStaticContent() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setStaticContent(const ListItemList* items) DECL_UNIMP("Control");
      /**
       * setSpace() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setSpace(int space) DECL_UNIMP("Control");
      /**
       * setRadioDimension() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setRadioDimension(long x, long y, long width, long height) DECL_UNIMP("Control");
      /**
       * setPageControlVisible() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setPageControlVisible(bool visible) DECL_UNIMP("Control");
      /**
       * setItemHeight() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setItemHeight(long height) DECL_UNIMP("Control");
      /**
       * setImageDimensions() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setImageDimensions(long imageWidth,long imageHeight) DECL_UNIMP("Control");
      /**
       * setImage() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setImage(const char* imageFilename) DECL_UNIMP("Control");
      /**
       * setColorDiffuse() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void setColorDiffuse(const char* hexString) DECL_UNIMP("Control");
      /**
       * selectItem() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void selectItem(long item) DECL_UNIMP("Control");
      /**
       * scroll() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void scroll(long id) DECL_UNIMP("Control");
      /**
       * isSelected() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual bool isSelected() DECL_UNIMP("Control");
      /**
       * getSpinControl() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual Control* getSpinControl() DECL_UNIMP("Control");
      /**
       * getSpace() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual long getSpace() DECL_UNIMP("Control");
      /**
       * getSelectedPosition() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual long getSelectedPosition() DECL_UNIMP("Control");
      /**
       * getSelectedItem() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual XBMCAddon::xbmcgui::ListItem* getSelectedItem() DECL_UNIMP("Control");
      /**
       * getSelected() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual bool getSelected() DECL_UNIMP("Control");
      /**
       * getListItem() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual XBMCAddon::xbmcgui::ListItem* getListItem(int index) DECL_UNIMP2("Control",WindowException);
      /**
       * getLabel2() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual String getLabel2() DECL_UNIMP("Control");
      /**
       * getItemHeight() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual long getItemHeight() DECL_UNIMP("Control");
      /**
       * addLabel() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void addLabel(const String& label) DECL_UNIMP("Control");

      // These need to be here for the stubbed out addItem
      //   and addItems methods
      /**
       * addItemStream() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void addItemStream(const String& fileOrUrl, bool sendMessage = true) DECL_UNIMP2("Control",WindowException);
      /**
       * addListItem() is only defined in subclasses of Control. See the specific
       *  subclass for the appropriate documentation.
       */
      virtual void addListItem(const XBMCAddon::xbmcgui::ListItem* listitem, bool sendMessage = true) DECL_UNIMP2("Control",WindowException);

      /**
       * getId() -- Returns the control's current id as an integer.\n
       * \n
       * example:\n
       *   - id = self.button.getId()n\n
       */
      virtual int getId() { return iControlId; }

      inline bool operator==(const Control& other) const { return iControlId == other.iControlId; }
      inline bool operator>(const Control& other) const { return iControlId > other.iControlId; }
      inline bool operator<(const Control& other) const { return iControlId < other.iControlId; }

      // hack this because it returns a tuple
      /**
       * getPosition() -- Returns the control's current position as a x,y integer tuple.\n
       * \n
       * example:\n
       *   - pos = self.button.getPosition()\n
       */
      virtual std::vector<int> getPosition();
      virtual int getX() { return dwPosX; }
      virtual int getY() { return dwPosY; }

      /**
       * getHeight() -- Returns the control's current height as an integer.\n
       * \n
       * example:\n
       *   - height = self.button.getHeight()\n
       */
      virtual int getHeight() { return dwHeight; }

      // getWidth() Method
      /**
       * getWidth() -- Returns the control's current width as an integer.\n
       * \n
       * example:\n
       *   - width = self.button.getWidth()\n
       */
      virtual int getWidth() { return dwWidth; }

      // setEnabled() Method
      /**
       * setEnabled(enabled) -- Set's the control's enabled/disabled state.\n
       * \n
       * enabled        : bool - True=enabled / False=disabled.\n
       * \n
       * example:\n
       *   - self.button.setEnabled(False)n\n
       */
      virtual void setEnabled(bool enabled);

      // setVisible() Method
      /**
       * setVisible(visible) -- Set's the control's visible/hidden state.\n
       * \n
       * visible        : bool - True=visible / False=hidden.\n
       * \n
       * example:\n
       *   - self.button.setVisible(False)\n
       */
      virtual void setVisible(bool visible);

      // setVisibleCondition() Method
      /**
       * setVisibleCondition(visible[,allowHiddenFocus]) -- Set's the control's visible condition.\n
       *     Allows XBMC to control the visible status of the control.\n
       * \n
       * visible          : string - Visible condition.\n
       * allowHiddenFocus : bool - True=gains focus even if hidden.\n
       * \n
       * List of Conditions - http://wiki.xbmc.org/index.php?title=List_of_Boolean_Conditions \n
       * \n
       * example:\n
       *   - self.button.setVisibleCondition('[Control.IsVisible(41) + !Control.IsVisible(12)]', True)n\n
       */
      virtual void setVisibleCondition(const char* visible, bool allowHiddenFocus = false);

      // setEnableCondition() Method
      /**
       * setEnableCondition(enable) -- Set's the control's enabled condition.\n
       *     Allows XBMC to control the enabled status of the control.\n
       * \n
       * enable           : string - Enable condition.\n
       * \n
       * List of Conditions - http://wiki.xbmc.org/index.php?title=List_of_Boolean_Conditions \n
       * \n
       * example:\n
       *   - self.button.setEnableCondition('System.InternetState')\n
       */
      virtual void setEnableCondition(const char* enable);

      // setAnimations() Method
      /**
       * setAnimations([(event, attr,)*]) -- Set's the control's animations.\n
       * \n
       * [(event,attr,)*] : list - A list of tuples consisting of event and attributes pairs.\n
       *   - event        : string - The event to animate.\n
       *   - attr         : string - The whole attribute string separated by spaces.\n
       * \n
       * Animating your skin - http://wiki.xbmc.org/?title=Animating_Your_Skin \n
       * \n
       * example:\n
       *   - self.button.setAnimations([('focus', 'effect=zoom end=90,247,220,56 time=0',)])n\n
       */
      virtual void setAnimations(const std::vector< Tuple<String,String> >& eventAttr) throw (WindowException);

      // setPosition() Method
      /**
       * setPosition(x, y) -- Set's the controls position.\n
       * \n
       * x              : integer - x coordinate of control.\n
       * y              : integer - y coordinate of control.\n
       * \n
       * *Note, You may use negative integers. (e.g sliding a control into view)\n
       * \n
       * example:\n
       *   - self.button.setPosition(100, 250)n\n
       */
      virtual void setPosition(long x, long y);

      // setWidth() Method
      /**
       * setWidth(width) -- Set's the controls width.\n
       * \n
       * width          : integer - width of control.\n
       * \n
       * example:\n
       *   - self.image.setWidth(100)\n
       */
      virtual void setWidth(long width);

      // setHeight() Method
      /**
       * setHeight(height) -- Set's the controls height.\n
       * \n
       * height         : integer - height of control.\n
       * \n
       * example:\n
       *   - self.image.setHeight(100)\n
       */
      virtual void setHeight(long height);

      // setNavigation() Method
      /**
       * setNavigation(up, down, left, right) -- Set's the controls navigation.\n
       * \n
       * up             : control object - control to navigate to on up.\n
       * down           : control object - control to navigate to on down.\n
       * left           : control object - control to navigate to on left.\n
       * right          : control object - control to navigate to on right.\n
       * \n
       * *Note, Same as controlUp(), controlDown(), controlLeft(), controlRight().\n
       *        Set to self to disable navigation for that direction.\n
       * \n
       * Throws: TypeError, if one of the supplied arguments is not a control type.\n
       *         ReferenceError, if one of the controls is not added to a window.\n
       * \n
       * example:\n
       *   - self.button.setNavigation(self.button1, self.button2, self.button3, self.button4)\n
       */
      virtual void setNavigation(const Control* up, const Control* down,
                                 const Control* left, const Control* right) 
        throw (WindowException);

      // controlUp() Method
      /**
       * controlUp(control) -- Set's the controls up navigation.\n
       * \n
       * control        : control object - control to navigate to on up.\n
       * \n
       * *Note, You can also use setNavigation(). Set to self to disable navigation.\n
       * \n
       * Throws: TypeError, if one of the supplied arguments is not a control type.\n
       *         ReferenceError, if one of the controls is not added to a window.\n
       * \n
       * example:\n
       *   - self.button.controlUp(self.button1)\n
       */
      virtual void controlUp(const Control* up) throw (WindowException);

      // controlDown() Method
      /**
       * controlDown(control) -- Set's the controls down navigation.\n
       * \n
       * control        : control object - control to navigate to on down.\n
       * \n
       * *Note, You can also use setNavigation(). Set to self to disable navigation.\n
       * \n
       * Throws: TypeError, if one of the supplied arguments is not a control type.\n
       *         ReferenceError, if one of the controls is not added to a window.\n
       * \n
       * example:\n
       *   - self.button.controlDown(self.button1)\n
       */
      virtual void controlDown(const Control* control) throw (WindowException);

      // controlLeft() Method
      /**
       * controlLeft(control) -- Set's the controls left navigation.\n
       * \n
       * control        : control object - control to navigate to on left.\n
       * \n
       * *Note, You can also use setNavigation(). Set to self to disable navigation.\n
       * \n
       * Throws: TypeError, if one of the supplied arguments is not a control type.\n
       *         ReferenceError, if one of the controls is not added to a window.\n
       * \n
       * example:\n
       *   - self.button.controlLeft(self.button1)\n
       */
      virtual void controlLeft(const Control* control) throw (WindowException);

      // controlRight() Method
      /**
       * controlRight(control) -- Set's the controls right navigation.\n
       * \n
       * control        : control object - control to navigate to on right.\n
       * \n
       * *Note, You can also use setNavigation(). Set to self to disable navigation.\n
       * \n
       * Throws: TypeError, if one of the supplied arguments is not a control type.\n
       *         ReferenceError, if one of the controls is not added to a window.\n
       * \n
       * example:\n
       *   - self.button.controlRight(self.button1)n\n
       */
      virtual void controlRight(const Control* control) throw (WindowException);

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

    /**
     * ControlSpin class.\n
     * \n
     *  - Not working yet -.\n
     * \n
     * you can't create this object, it is returned by objects like ControlTextBox and ControlList.\n
     */
    class ControlSpin : public Control
    {
    public:
      virtual ~ControlSpin();

      /**
       * setTextures(up, down, upFocus, downFocus) -- Set's textures for this control.\n
       * \n
       * texture are image files that are used for example in the skin\n
       */
      virtual void setTextures(const char* up, const char* down, 
                               const char* upFocus, 
                               const char* downFocus) throw(UnimplementedException);
#ifndef SWIG
      color_t color;
      std::string strTextureUp;
      std::string strTextureDown;
      std::string strTextureUpFocus;
      std::string strTextureDownFocus;
#endif

    private:
      ControlSpin();

      friend class Window;
      friend class ControlList;

    };

    /**
     * ControlLabel class.\n
     * \n
     * ControlLabel(x, y, width, height, label[, font, textColor, \n
     *              disabledColor, alignment, hasPath, angle])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * label          : string or unicode - text string.\n
     * font           : [opt] string - font used for label text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of enabled label's label. (e.g. '0xFFFFFFFF')\n
     * disabledColor  : [opt] hexstring - color of disabled label's label. (e.g. '0xFFFF3300')\n
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n
     * hasPath        : [opt] bool - True=stores a path / False=no path.\n
     * angle          : [opt] integer - angle of control. (+ rotates CCW, - rotates C\n
     * \n
     * example:\n
     *   - self.label = xbmcgui.ControlLabel(100, 250, 125, 75, 'Status', angle=45)n\n
     */
    class ControlLabel : public Control
    {
    public:
      ControlLabel(long x, long y, long width, long height, const String& label,
                  const char* font = NULL, const char* textColor = NULL, 
                  const char* disabledColor = NULL,
                  long alignment = XBFONT_LEFT, 
                  bool hasPath = false, long angle = 0);

      virtual ~ControlLabel();

      /**
       * getLabel() -- Returns the text value for this label.\n
       * \n
       * example:\n
       *   - label = self.label.getLabel()n\n
       */
      virtual String getLabel() throw(UnimplementedException);

      /**
       * setLabel(label) -- Set's text for this label.\n
       * \n
       * label          : string or unicode - text string.\n
       * \n
       * example:\n
       *   - self.label.setLabel('Status')\n
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) throw(UnimplementedException);
#ifndef SWIG
      ControlLabel() : 
        Control ("ControlLabel"),
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

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);
#endif
    };

    // ControlEdit class
    /**
     * ControlEdit class.\n
     * \n
     * ControlEdit(x, y, width, height, label[, font, textColor, \n
     *              disabledColor, alignment, focusTexture, noFocusTexture])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * label          : string or unicode - text string.\n
     * font           : [opt] string - font used for label text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of enabled label's label. (e.g. '0xFFFFFFFF')\n
     * disabledColor  : [opt] hexstring - color of disabled label's label. (e.g. '0xFFFF3300')\n
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n
     * focusTexture   : [opt] string - filename for focus texture.\n
     * noFocusTexture : [opt] string - filename for no focus texture.\n
     * isPassword     : [opt] bool - if true, mask text value.\n
     * \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.edit = xbmcgui.ControlEdit(100, 250, 125, 75, 'Status')\n
     */
    class ControlEdit : public Control
    {
    public:
      ControlEdit(long x, long y, long width, long height, const String& label,
                  const char* font = NULL, const char* textColor = NULL, 
                  const char* disabledColor = NULL,
                  long _alignment = XBFONT_LEFT, const char* focusTexture = NULL,
                  const char* noFocusTexture = NULL, bool isPassword = false);


      // setLabel() Method
      /**
       * setLabel(label) -- Set's text heading for this edit control.\n
       * \n
       * label          : string or unicode - text string.\n
       * \n
       * example:\n
       *   - self.edit.setLabel('Status')n\n
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) throw(UnimplementedException);

      // getLabel() Method
      /**
       * getLabel() -- Returns the text heading for this edit control.\n
       * \n
       * example:\n
       *   - label = self.edit.getLabel()\n
       */
      virtual String getLabel() throw(UnimplementedException);

      // setText() Method
      /**
       * setText(value) -- Set's text value for this edit control.\n
       * \n
       * value          : string or unicode - text string.\n
       * \n
       * example:\n
       *   - self.edit.setText('online')n\n
       */
      virtual void setText(const String& text) throw(UnimplementedException);

      // getText() Method
      /**
       * getText() -- Returns the text value for this edit control.\n
       * \n
       * example:\n
       *   - value = self.edit.getText()\n
       */
      virtual String getText() throw(UnimplementedException);

#ifndef SWIG
      ControlEdit() :
        Control     ("ControlEdit"),
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

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);
#endif
    };

    /**
     * ControlList class.\n
     * \n
     * ControlList(x, y, width, height[, font, textColor, buttonTexture, buttonFocusTexture,\n
     *             selectedColor, imageWidth, imageHeight, itemTextXOffset, itemTextYOffset,\n
     *             itemHeight, space, alignmentY])n"//, shadowColor])\n
     * \n
     * x                  : integer - x coordinate of control.\n
     * y                  : integer - y coordinate of control.\n
     * width              : integer - width of control.\n
     * height             : integer - height of control.\n
     * font               : [opt] string - font used for items label. (e.g. 'font13')\n
     * textColor          : [opt] hexstring - color of items label. (e.g. '0xFFFFFFFF')\n
     * buttonTexture      : [opt] string - filename for focus texture.\n
     * buttonFocusTexture : [opt] string - filename for no focus texture.\n
     * selectedColor      : [opt] integer - x offset of label.\n
     * imageWidth         : [opt] integer - width of items icon or thumbnail.\n
     * imageHeight        : [opt] integer - height of items icon or thumbnail.\n
     * itemTextXOffset    : [opt] integer - x offset of items label.\n
     * itemTextYOffset    : [opt] integer - y offset of items label.\n
     * itemHeight         : [opt] integer - height of items.\n
     * space              : [opt] integer - space between items.\n
     * alignmentY         : [opt] integer - Y-axis alignment of items label - *Note, see xbfont.h\n
     * //"shadowColor        : [opt] hexstring - color of items label's shadow. (e.g. '0xFF000000')\n
     * \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.cList = xbmcgui.ControlList(100, 250, 200, 250, 'font14', space=5)\n
     */
    class ControlList : public Control 
    {
      void internAddListItem(AddonClass::Ref<ListItem> listitem, bool sendMessage) throw(WindowException);

    public:
      ControlList(long x, long y, long width, long height, const char* font = NULL,
                  const char* textColor = NULL, const char* buttonTexture = NULL,
                  const char* buttonFocusTexture = NULL,
                  const char* selectedColor = NULL,
                  long _imageWidth=10, long _imageHeight=10, long _itemTextXOffset = CONTROL_TEXT_OFFSET_X,
                  long _itemTextYOffset = CONTROL_TEXT_OFFSET_Y, long _itemHeight = 27, long _space = 2, 
                  long _alignmentY = XBFONT_CENTER_Y);

      virtual ~ControlList();

      /**
       * addItem(item) -- Add a new item to this list control.\n
       * \n
       * item               : string, unicode or ListItem - item to add.\n
       * \n
       * example:\n
       *   - cList.addItem('Reboot XBMC')\n
       */
      virtual void addItemStream(const String& fileOrUrl, bool sendMessage = true) throw(UnimplementedException,WindowException);
      virtual void addListItem(const XBMCAddon::xbmcgui::ListItem* listitem, bool sendMessage = true) throw(UnimplementedException,WindowException);

      /**
       * selectItem(item) -- Select an item by index number.\n
       * \n
       * item               : integer - index number of the item to select.\n
       * \n
       * example:\n
       *   - cList.selectItem(12)\n
       */
      virtual void selectItem(long item) throw(UnimplementedException);

      /**
       * removeItem(index) -- Remove an item by index number.\n
       *\n
       * index              : integer - index number of the item to remove.\n
       *\n
       * example:\n
       *   - cList.removeItem(12)\n
       */
      virtual void removeItem(int index) throw (UnimplementedException,WindowException);

      /**
       * reset() -- Clear all ListItems in this control list.\n
       * \n
       * example:\n
       *   - cList.reset()n\n
       */
      virtual void reset() throw (UnimplementedException);

      /**
       * getSpinControl() -- returns the associated ControlSpin object.\n
       * \n
       * *Note, Not working completely yet -\n
       *        After adding this control list to a window it is not possible to change\n
       *        the settings of this spin control.\n
       * \n
       * example:\n
       *   - ctl = cList.getSpinControl()\n
       */
      virtual Control* getSpinControl() throw (UnimplementedException);

      /**
       * getSelectedPosition() -- Returns the position of the selected item as an integer.\n
       * \n
       * *Note, Returns -1 for empty lists.\n
       * \n
       * example:\n
       *   - pos = cList.getSelectedPosition()\n
       */
      virtual long getSelectedPosition() throw (UnimplementedException);

      /**
       * getSelectedItem() -- Returns the selected item as a ListItem object.\n
       * \n
       * *Note, Same as getSelectedPosition(), but instead of an integer a ListItem object\n
       *        is returned. Returns None for empty lists.\n
       *        See windowexample.py on how to use this.\n
       * \n
       * example:\n
       *   - item = cList.getSelectedItem()\n
       */
      virtual XBMCAddon::xbmcgui::ListItem* getSelectedItem() throw (UnimplementedException);


      // setImageDimensions() method
      /**
       * setImageDimensions(imageWidth, imageHeight) -- Sets the width/height of items icon or thumbnail.\n
       * \n
       * imageWidth         : [opt] integer - width of items icon or thumbnail.\n
       * imageHeight        : [opt] integer - height of items icon or thumbnail.\n
       * \n
       * example:\n
       *   - cList.setImageDimensions(18, 18)n\n
       */
      virtual void setImageDimensions(long imageWidth,long imageHeight) throw (UnimplementedException);

      // setItemHeight() method
      /**
       * setItemHeight(itemHeight) -- Sets the height of items.\n
       * \n
       * itemHeight         : integer - height of items.\n
       * \n
       * example:\n
       *   - cList.setItemHeight(25)\n
       */
      virtual void setItemHeight(long height) throw (UnimplementedException);

      // setSpace() method
      /**
       * setSpace(space) -- Set's the space between items.\n
       * \n
       * space              : [opt] integer - space between items.\n
       * \n
       * example:\n
       *   - cList.setSpace(5)\n
       */
      virtual void setSpace(int space) throw (UnimplementedException);

      // setPageControlVisible() method
      /**
       * setPageControlVisible(visible) -- Sets the spin control's visible/hidden state.\n
       * \n
       * visible            : boolean - True=visible / False=hidden.\n
       * \n
       * example:\n
       *   - cList.setPageControlVisible(True)\n
       */
      virtual void setPageControlVisible(bool visible) throw(UnimplementedException);

      // size() method
      /**
       * size() -- Returns the total number of items in this list control as an integer.\n
       * \n
       * example:\n
       *   - cnt = cList.size()\n
       */
      virtual long size() throw (UnimplementedException);


      // getItemHeight() Method
      /**
       * getItemHeight() -- Returns the control's current item height as an integer.\n
       * \n
       * example:\n
       *   - item_height = self.cList.getItemHeight()n\n
       */
      virtual long getItemHeight() throw(UnimplementedException);

      // getSpace() Method
      /**
       * getSpace() -- Returns the control's space between items as an integer.\n
       * \n
       * example:\n
       *   - gap = self.cList.getSpace()n\n
       */
      virtual long getSpace() throw (UnimplementedException);

      // getListItem() method
      /**
       * getListItem(index) -- Returns a given ListItem in this List.\n
       * \n
       * index           : integer - index number of item to return.\n
       * \n
       * *Note, throws a ValueError if index is out of range.\n
       * \n
       * example:\n
       *   - listitem = cList.getListItem(6)n\n
       */
      virtual XBMCAddon::xbmcgui::ListItem* getListItem(int index) throw (UnimplementedException,WindowException);

      /**
       * setStaticContent(items) -- Fills a static list with a list of listitems.\n
       * \n
       * items                : List - list of listitems to add.\n
       * \n
       * *Note, You can use the above as keywords for arguments.\n
       * \n
       * example:\n
       *   - cList.setStaticContent(items=listitems)n\n
       */
      virtual void setStaticContent(const ListItemList* items) throw (UnimplementedException);

#ifndef SWIG
      void sendLabelBind(int tail);

      SWIGHIDDENVIRTUAL bool canAcceptMessages(int actionId) 
      { return ((actionId == ACTION_SELECT_ITEM) | (actionId == ACTION_MOUSE_LEFT_CLICK)); }

      // This is called from AddonWindow.cpp but shouldn't be available
      //  to the scripting languages.
      ControlList() :
        Control("ControlList"),
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

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);
#endif
    };

    // ControlFadeLabel class
    /**
     * ControlFadeLabel class.\n
     * Control that scroll's labl\n
     * \n
     * ControlFadeLabel(x, y, width, height[, font, textColor, alignment])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * font           : [opt] string - font used for label text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of fadelabel's labels. (e.g. '0xFFFFFFFF')\n
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n
     * \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.fadelabel = xbmcgui.ControlFadeLabel(100, 250, 200, 50, textColor='0xFFFFFFFF')\n
     */
    class ControlFadeLabel : public Control
    {
    public:
      ControlFadeLabel(long x, long y, long width, long height, 
                       const char* font = NULL, 
                       const char* textColor = NULL, 
                       long _alignment = XBFONT_LEFT);

      // addLabel() Method
      /**
       * addLabel(label) -- Add a label to this control for scrolling.\n
       * \n
       * label          : string or unicode - text string.\n
       * \n
       * example:\n
       *   - self.fadelabel.addLabel('This is a line of text that can scroll.')\n
       */
       virtual void addLabel(const String& label) throw (UnimplementedException);

      /**
       * reset() -- Clear this fade label.\n
       * \n
       * example:\n
       *   - self.fadelabel.reset()n\n
       */
      virtual void reset() throw (UnimplementedException);

#ifndef SWIG
      std::string strFont;
      color_t textColor;
      std::vector<std::string> vecLabels;
      uint32_t align;

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);

      ControlFadeLabel() : Control("ControlFadeLabel") {}
#endif
    };

    /**
     * ControlTextBox class.\n
     * \n
     * ControlTextBox(x, y, width, height[, font, textColor])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * font           : [opt] string - font used for text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of textbox's text. (e.g. '0xFFFFFFFF')\n
     * \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.textbox = xbmcgui.ControlTextBox(100, 250, 300, 300, textColor='0xFFFFFFFF')\n
     */
    class ControlTextBox : public Control
    {
    public:
      ControlTextBox(long x, long y, long width, long height, 
                     const char* font = NULL, 
                     const char* textColor = NULL);

      // SetText() Method
      /**
       * setText(text) -- Set's the text for this textbox.\n
       * \n
       * text           : string or unicode - text string.\n
       * \n
       * example:\n
       *   - self.textbox.setText('This is a line of text that can wrap.')\n
       */
      virtual void setText(const String& text) throw(UnimplementedException);

      // reset() Method
      /**
       * reset() -- Clear's this textbox.\n
       * \n
       * example:\n
       *   - self.textbox.reset()n\n
       */
      virtual void reset() throw(UnimplementedException);

      // scroll() Method
      /**
       * scroll(position) -- Scrolls to the given position.\n
       * \n
       * id           : integer - position to scroll to.\n
       * \n
       * example:\n
       *   - self.textbox.scroll(10)\n
       */
      virtual void scroll(long id) throw(UnimplementedException);

#ifndef SWIG
      std::string strFont;
      color_t textColor;

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);

      ControlTextBox() : Control("ControlTextBox") {}
#endif
    };

    // ControlImage class
    /**
     * ControlImage class.\n
     * \n
     * ControlImage(x, y, width, height, filename[, aspectRatio, colorDiffuse])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * filename       : string - image filename.\n
     * aspectRatio    : [opt] integer - (values 0 = stretch (default), 1 = scale up (crops), 2 = scale down (black bar\n
     * colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))\n
     * \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.image = xbmcgui.ControlImage(100, 250, 125, 75, aspectRatio=2)\n
     */
    class ControlImage : public Control
    {
    public:
      ControlImage(long x, long y, long width, long height, 
                   const char* filename, long aspectRatio = 0,
                   const char* colorDiffuse = NULL);

      /**
       * setImage(filename) -- Changes the image.\n
       * \n
       * filename       : string - image filename.\n
       * \n
       * example:\n
       *   - self.image.setImage('special://home/scripts/test.png')\n
       */
      virtual void setImage(const char* imageFilename) throw (UnimplementedException);

      /**
       * setColorDiffuse(colorDiffuse) -- Changes the images color.\n
       * \n
       * colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))\n
       * \n
       * example:\n
       *   - self.image.setColorDiffuse('0xC0FF0000')\n
       */
      virtual void setColorDiffuse(const char* hexString) throw (UnimplementedException);

#ifndef SWIG
      ControlImage() :
        Control     ("ControlImage"),
        aspectRatio (0)
      {}

      std::string strFileName;
      int aspectRatio;
      color_t colorDiffuse;

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);
#endif
    };

    class ControlProgress : public Control
    {
    public:
      ControlProgress(long x, long y, long width, long height, 
                      const char* texturebg = NULL,
                      const char* textureleft = NULL,
                      const char* texturemid = NULL,
                      const char* textureright = NULL,
                      const char* textureoverlay = NULL);

      /**
       * setPercent(percent) -- Sets the percentage of the progressbar to show.\n
       * \n
       * percent       : float - percentage of the bar to show.\n
       * \n
       * *Note, valid range for percent is 0-100\n
       * \n
       * example:\n
       *   - self.progress.setPercent(60)\n
       */
      virtual void setPercent(float pct) throw (UnimplementedException);

      /**
       * getPercent() -- Returns a float of the percent of the progress.\n
       * \n
       * example:\n
       *   - print self.progress.getValue()\n
       */
       virtual float getPercent() throw (UnimplementedException);

#ifndef SWIG
      std::string strTextureLeft;
      std::string strTextureMid;
      std::string strTextureRight;
      std::string strTextureBg;
      std::string strTextureOverlay;
      int aspectRatio;
      color_t colorDiffuse;

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);
      ControlProgress() :
        Control     ("ControlProgress"),
        aspectRatio (0)
      {}
#endif
    };

    // ControlButton class
    /**
     * ControlButton class.\n
     * \n
     * ControlButton(x, y, width, height, label[, focusTexture, noFocusTexture, textOffsetX, textOffsetY,\n
     *               alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * label          : string or unicode - text string.\n
     * focusTexture   : [opt] string - filename for focus texture.\n
     * noFocusTexture : [opt] string - filename for no focus texture.\n
     * textOffsetX    : [opt] integer - x offset of label.\n
     * textOffsetY    : [opt] integer - y offset of label.\n
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n
     * font           : [opt] string - font used for label text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')\n
     * disabledColor  : [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')\n
     * angle          : [opt] integer - angle of control. (+ rotates CCW, - rotates CW)\n
     * shadowColor    : [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')\n
     * focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFF00FFFF')\n
     * \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.button = xbmcgui.ControlButton(100, 250, 200, 50, 'Status', font='font14')\n
     */
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
      /**
       * setLabel([label, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's this buttons text attributes.\n
       * \n
       * label          : [opt] string or unicode - text string.\n
       * font           : [opt] string - font used for label text. (e.g. 'font13')\n
       * textColor      : [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')\n
       * disabledColor  : [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')\n
       * shadowColor    : [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')\n
       * focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFFFFFF00')\n
       * label2         : [opt] string or unicode - text string.\n
       * \n
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:\n
       *   - self.button.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')\n
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) throw (UnimplementedException);

      // setDisabledColor() Method
      /**
       * setDisabledColor(disabledColor) -- Set's this buttons disabled color.\n
       * \n
       * disabledColor  : hexstring - color of disabled button's label. (e.g. '0xFFFF3300')\n
       * \n
       * example:\n
       *   - self.button.setDisabledColor('0xFFFF3300')\n
       */
      virtual void setDisabledColor(const char* color) throw (UnimplementedException);

      // getLabel() Method
      /**
       * getLabel() -- Returns the buttons label as a unicode string.\n
       * \n
       * example:\n
       *   - label = self.button.getLabel()\n
       */
      virtual String getLabel() throw (UnimplementedException);

      // getLabel2() Method
      /**
       * getLabel2() -- Returns the buttons label2 as a unicode string.\n
       * \n
       * example:\n
       *   - label = self.button.getLabel2()\n
       */
      virtual String getLabel2() throw (UnimplementedException);
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

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);

      ControlButton() :
        Control     ("ControlButton"),
        textOffsetX (0),
        textOffsetY (0),
        iAngle      (0),
        shadowColor (0),
        focusedColor(0)
      {}
#endif
    };

    // ControlCheckMark class
    /**
     * ControlCheckMark class.\n
     * \n
     * ControlCheckMark(x, y, width, height, label[, focusTexture, noFocusTexture,\n
     *                  checkWidth, checkHeight, alignment, font, textColor, disabledColor])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * label          : string or unicode - text string.\n
     * focusTexture   : [opt] string - filename for focus texture.\n
     * noFocusTexture : [opt] string - filename for no focus texture.\n
     * checkWidth     : [opt] integer - width of checkmark.\n
     * checkHeight    : [opt] integer - height of checkmark.\n
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h\n
     * font           : [opt] string - font used for label text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')\n
     * disabledColor  : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')\n
     * \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.checkmark = xbmcgui.ControlCheckMark(100, 250, 200, 50, 'Status', font='font14')\n
     */
    class ControlCheckMark : public Control
    {
    public:

      ControlCheckMark(long x, long y, long width, long height, const String& label,
                       const char* focusTexture = NULL, const char* noFocusTexture = NULL, 
                       long checkWidth = 30, long checkHeight = 30,
                       long _alignment = XBFONT_RIGHT, const char* font = NULL, 
                       const char* textColor = NULL, const char* disabledColor = NULL);

      // getSelected() Method
      /**
       * getSelected() -- Returns the selected status for this checkmark as a bool.\n
       * \n
       * example:\n
       *   - selected = self.checkmark.getSelected()\n
       */
      virtual bool getSelected() throw (UnimplementedException);

      // setSelected() Method
      /**
       * setSelected(isOn) -- Sets this checkmark status to on or off.\n
       * \n
       * isOn           : bool - True=selected (on) / False=not selected (off)\n
       * \n
       * example:\n
       *   - self.checkmark.setSelected(True)\n
       */
      virtual void setSelected(bool selected) throw (UnimplementedException);

      // setLabel() Method
      /**
       * setLabel(label[, font, textColor, disabledColor]) -- Set's this controls text attributes.\n
       * \n
       * label          : string or unicode - text string.\n
       * font           : [opt] string - font used for label text. (e.g. 'font13')\n
       * textColor      : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')\n
       * disabledColor  : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')\n
       * \n
       * example:\n
       *   - self.checkmark.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300')\n
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) throw (UnimplementedException);

      // setDisabledColor() Method
      /**
       * setDisabledColor(disabledColor) -- Set's this controls disabled color.\n
       * \n
       * disabledColor  : hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')\n
       * \n
       * example:\n
       *   - self.checkmark.setDisabledColor('0xFFFF3300')\n
       */
      virtual void setDisabledColor(const char* color) throw (UnimplementedException);

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

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);

      ControlCheckMark() :
        Control     ("ControlCheckMark"),
        checkWidth  (0),
        checkHeight (0)
      {}
#endif
    };

    // ControlGroup class
    /**
     * ControlGroup class.\n
     * \n
     * ControlGroup(x, y, width, height\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * example:\n
     *   - self.group = xbmcgui.ControlGroup(100, 250, 125, 75)\n
     */
    class ControlGroup : public Control 
    {
    public:
      ControlGroup(long x, long y, long width, long height);

#ifndef SWIG
      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);

      ControlGroup() : Control("ControlGroup") {}
#endif
    };

    class ControlRadioButton : public Control
    {
    public:
      ControlRadioButton(long x, long y, long width, long height, const String& label,
                         const char* focusTexture = NULL, const char* noFocusTexture = NULL, 
                         long textOffsetX = CONTROL_TEXT_OFFSET_X, 
                         long textOffsetY = CONTROL_TEXT_OFFSET_Y, 
                         long _alignment = (XBFONT_LEFT | XBFONT_CENTER_Y), 
                         const char* font = NULL, const char* textColor = NULL,
                         const char* disabledColor = NULL, long angle = 0,
                         const char* shadowColor = NULL, const char* focusedColor = NULL,
                         const char* TextureRadioFocus = NULL, 
                         const char* TextureRadioNoFocus = NULL);

      // setSelected() Method
      /**
       * setSelected(selected) -- Sets the radio buttons's selected status.\n
       * \n
       * selected            : bool - True=selected (on) / False=not selected (off)\n
       * \n
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:\n
       *   - self.radiobutton.setSelected(True)\n
       */
      virtual void setSelected(bool selected) throw (UnimplementedException);

      // isSelected() Method
      /**
       * isSelected() -- Returns the radio buttons's selected status.\n
       * \n
       * example:\n
       *   - is = self.radiobutton.isSelected()n\n
       */
      virtual bool isSelected() throw (UnimplementedException);

      // setLabel() Method
      /**
       * setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's the radio buttons text attributes.\n
       * \n
       * label          : string or unicode - text string.\n
       * font           : [opt] string - font used for label text. (e.g. 'font13')\n
       * textColor      : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')\n
       * disabledColor  : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')\n
       * shadowColor    : [opt] hexstring - color of radio button's label's shadow. (e.g. '0xFF000000')\n
       * focusedColor   : [opt] hexstring - color of focused radio button's label. (e.g. '0xFFFFFF00')\n
       * \n
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:\n
       *   - self.radiobutton.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')\n
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) throw (UnimplementedException);

      // setRadioDimension() Method
      /**
       * setRadioDimension(x, y, width, height) -- Sets the radio buttons's radio texture's position and size.\n
       * \n
       * x                   : integer - x coordinate of radio texture.\n
       * y                   : integer - y coordinate of radio texture.\n
       * width               : integer - width of radio texture.\n
       * height              : integer - height of radio texture.\n
       * \n
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:\n
       *   - self.radiobutton.setRadioDimension(x=100, y=5, width=20, height=20)\n
       */
      virtual void setRadioDimension(long x, long y, long width, long height) throw (UnimplementedException);

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool canAcceptMessages(int actionId) { return true; }

      std::string strFont;
      std::string strText;
      std::string strTextureFocus;
      std::string strTextureNoFocus;
      std::string strTextureRadioFocus;
      std::string strTextureRadioNoFocus;
      color_t textColor;
      color_t disabledColor;
      int textOffsetX;
      int textOffsetY; 
     uint32_t align;
      int iAngle;
      color_t shadowColor;
      color_t focusedColor;

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);

      ControlRadioButton() :
        Control     ("ControlRadioButton"),
        textOffsetX (0),
        textOffsetY (0),
        iAngle      (0)
      {}
#endif
    };
	
    /**
     * ControlSlider class.\n
     * \n
     * ControlSlider(x, y, width, height[, textureback, texture, texturefocus])\n
     * \n
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * textureback    : [opt] string - image filename.\n
     * texture        : [opt] string - image filename.\n
     * texturefocus   : [opt] string - image filename.n"            \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * \n
     * example:\n
     *   - self.slider = xbmcgui.ControlSlider(100, 250, 350, 40)\n
     */
    class ControlSlider : public Control
    {
    public:
      ControlSlider(long x, long y, long width, long height, 
                    const char* textureback = NULL, 
                    const char* texture = NULL,
                    const char* texturefocus = NULL);

      /**
       * getPercent() -- Returns a float of the percent of the slider.\n
       * \n
       * example:\n
       *   - print self.slider.getPercent()\n
       */
      virtual float getPercent() throw (UnimplementedException);

      /**
       * setPercent(50) -- Sets the percent of the slider.\n
       * \n
       * example:\n
       * self.slider.setPercent(50)\n
       */
      virtual void setPercent(float pct) throw (UnimplementedException);

#ifndef SWIG
      std::string strTextureBack;
      std::string strTexture;
      std::string strTextureFoc;    

      SWIGHIDDENVIRTUAL CGUIControl* Create() throw (WindowException);

      ControlSlider() : Control("ControlSlider") {}
#endif
    };
  }
}


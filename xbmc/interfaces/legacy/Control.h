/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) DECL_UNIMP("Control");
      virtual void reset() DECL_UNIMP("Control");
      virtual void setSelected(bool selected) DECL_UNIMP("Control");
      virtual void setPercent(float pct) DECL_UNIMP("Control");
      virtual void setDisabledColor(const char* color) DECL_UNIMP("Control");
      virtual float getPercent() DECL_UNIMP("Control");
      virtual String getLabel() DECL_UNIMP("Control");
      virtual String getText() DECL_UNIMP("Control");
      virtual long size() DECL_UNIMP("Control");
      virtual void setTextures(const char* up, const char* down, 
                               const char* upFocus, 
                               const char* downFocus) DECL_UNIMP("Control");
      virtual void setText(const String& text) DECL_UNIMP("Control");
      virtual void setStaticContent(const ListItemList* items) DECL_UNIMP("Control");
      virtual void setSpace(int space) DECL_UNIMP("Control");
      virtual void setRadioDimension(long x, long y, long width, long height) DECL_UNIMP("Control");
      virtual void setPageControlVisible(bool visible) DECL_UNIMP("Control");
      virtual void setItemHeight(long height) DECL_UNIMP("Control");
      virtual void setImageDimensions(long imageWidth,long imageHeight) DECL_UNIMP("Control");
      virtual void setImage(const char* imageFilename) DECL_UNIMP("Control");
      virtual void setColorDiffuse(const char* hexString) DECL_UNIMP("Control");
      virtual void selectItem(long item) DECL_UNIMP("Control");
      virtual void scroll(long id) DECL_UNIMP("Control");
      virtual bool isSelected() DECL_UNIMP("Control");
      virtual Control* getSpinControl() DECL_UNIMP("Control");
      virtual long getSpace() DECL_UNIMP("Control");
      virtual long getSelectedPosition() DECL_UNIMP("Control");
      virtual XBMCAddon::xbmcgui::ListItem* getSelectedItem() DECL_UNIMP("Control");
      virtual bool getSelected() DECL_UNIMP("Control");
      virtual XBMCAddon::xbmcgui::ListItem* getListItem(int index) DECL_UNIMP2("Control",WindowException);
      virtual String getLabel2() DECL_UNIMP("Control");
      virtual long getItemHeight() DECL_UNIMP("Control");
      virtual void addLabel(const String& label) DECL_UNIMP("Control");

      // These need to be here for the stubbed out addItem
      //   and addItems methods
      virtual void addItemStream(const String& fileOrUrl, bool sendMessage = true) DECL_UNIMP2("Control",WindowException);
      virtual void addListItem(const XBMCAddon::xbmcgui::ListItem* listitem, bool sendMessage = true) DECL_UNIMP2("Control",WindowException);

      /**
       * getId() -- Returns the control's current id as an integer.
       * 
       * example:
       *   - id = self.button.getId()\n
       */
      virtual int getId() { return iControlId; }

      inline bool operator==(const Control& other) const { return iControlId == other.iControlId; }
      inline bool operator>(const Control& other) const { return iControlId > other.iControlId; }
      inline bool operator<(const Control& other) const { return iControlId < other.iControlId; }

      // hack this because it returns a tuple
      /**
       * getPosition() -- Returns the control's current position as a x,y integer tuple.
       * 
       * example:
       *   - pos = self.button.getPosition()
       */
      virtual std::vector<int> getPosition();
      virtual int getX() { return dwPosX; }
      virtual int getY() { return dwPosY; }

      /**
       * getHeight() -- Returns the control's current height as an integer.
       * 
       * example:
       *   - height = self.button.getHeight()
       */
      virtual int getHeight() { return dwHeight; }

      // getWidth() Method
      /**
       * getWidth() -- Returns the control's current width as an integer.
       * 
       * example:
       *   - width = self.button.getWidth()
       */
      virtual int getWidth() { return dwWidth; }

      // setEnabled() Method
      /**
       * setEnabled(enabled) -- Set's the control's enabled/disabled state.
       * 
       * enabled        : bool - True=enabled / False=disabled.
       * 
       * example:
       *   - self.button.setEnabled(False)\n
       */
      virtual void setEnabled(bool enabled);

      // setVisible() Method
      /**
       * setVisible(visible) -- Set's the control's visible/hidden state.
       * 
       * visible        : bool - True=visible / False=hidden.
       * 
       * example:
       *   - self.button.setVisible(False)
       */
      virtual void setVisible(bool visible);

      // setVisibleCondition() Method
      /**
       * setVisibleCondition(visible[,allowHiddenFocus]) -- Set's the control's visible condition.
       *     Allows XBMC to control the visible status of the control.
       * 
       * visible          : string - Visible condition.
       * allowHiddenFocus : bool - True=gains focus even if hidden.
       * 
       * List of Conditions - http://wiki.xbmc.org/index.php?title=List_of_Boolean_Conditions 
       * 
       * example:
       *   - self.button.setVisibleCondition('[Control.IsVisible(41) + !Control.IsVisible(12)]', True)\n
       */
      virtual void setVisibleCondition(const char* visible, bool allowHiddenFocus = false);

      // setEnableCondition() Method
      /**
       * setEnableCondition(enable) -- Set's the control's enabled condition.
       *     Allows XBMC to control the enabled status of the control.
       * 
       * enable           : string - Enable condition.
       * 
       * List of Conditions - http://wiki.xbmc.org/index.php?title=List_of_Boolean_Conditions 
       * 
       * example:
       *   - self.button.setEnableCondition('System.InternetState')
       */
      virtual void setEnableCondition(const char* enable);

      // setAnimations() Method
      /**
       * setAnimations([(event, attr,)*]) -- Set's the control's animations.
       * 
       * [(event,attr,)*] : list - A list of tuples consisting of event and attributes pairs.
       *   - event        : string - The event to animate.
       *   - attr         : string - The whole attribute string separated by spaces.
       * 
       * Animating your skin - http://wiki.xbmc.org/?title=Animating_Your_Skin 
       * 
       * example:
       *   - self.button.setAnimations([('focus', 'effect=zoom end=90,247,220,56 time=0',)])\n
       */
      virtual void setAnimations(const std::vector< Tuple<String,String> >& eventAttr) throw (WindowException);

      // setPosition() Method
      /**
       * setPosition(x, y) -- Set's the controls position.
       * 
       * x              : integer - x coordinate of control.
       * y              : integer - y coordinate of control.
       * 
       * *Note, You may use negative integers. (e.g sliding a control into view)
       * 
       * example:
       *   - self.button.setPosition(100, 250)\n
       */
      virtual void setPosition(long x, long y);

      // setWidth() Method
      /**
       * setWidth(width) -- Set's the controls width.
       * 
       * width          : integer - width of control.
       * 
       * example:
       *   - self.image.setWidth(100)
       */
      virtual void setWidth(long width);

      // setHeight() Method
      /**
       * setHeight(height) -- Set's the controls height.
       * 
       * height         : integer - height of control.
       * 
       * example:
       *   - self.image.setHeight(100)
       */
      virtual void setHeight(long height);

      // setNavigation() Method
      /**
       * setNavigation(up, down, left, right) -- Set's the controls navigation.
       * 
       * up             : control object - control to navigate to on up.
       * down           : control object - control to navigate to on down.
       * left           : control object - control to navigate to on left.
       * right          : control object - control to navigate to on right.
       * 
       * *Note, Same as controlUp(), controlDown(), controlLeft(), controlRight().
       *        Set to self to disable navigation for that direction.
       * 
       * Throws: TypeError, if one of the supplied arguments is not a control type.
       *         ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.setNavigation(self.button1, self.button2, self.button3, self.button4)
       */
      virtual void setNavigation(const Control* up, const Control* down,
                                 const Control* left, const Control* right) 
        throw (WindowException);

      // controlUp() Method
      /**
       * controlUp(control) -- Set's the controls up navigation.
       * 
       * control        : control object - control to navigate to on up.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws: TypeError, if one of the supplied arguments is not a control type.
       *         ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlUp(self.button1)
       */
      virtual void controlUp(const Control* up) throw (WindowException);

      // controlDown() Method
      /**
       * controlDown(control) -- Set's the controls down navigation.
       * 
       * control        : control object - control to navigate to on down.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws: TypeError, if one of the supplied arguments is not a control type.
       *         ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlDown(self.button1)
       */
      virtual void controlDown(const Control* control) throw (WindowException);

      // controlLeft() Method
      /**
       * controlLeft(control) -- Set's the controls left navigation.
       * 
       * control        : control object - control to navigate to on left.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws: TypeError, if one of the supplied arguments is not a control type.
       *         ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlLeft(self.button1)
       */
      virtual void controlLeft(const Control* control) throw (WindowException);

      // controlRight() Method
      /**
       * controlRight(control) -- Set's the controls right navigation.
       * 
       * control        : control object - control to navigate to on right.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws: TypeError, if one of the supplied arguments is not a control type.
       *         ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlRight(self.button1)\n
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
     * ControlSpin class.
     * 
     *  - Not working yet -.
     * 
     * you can't create this object, it is returned by objects like ControlTextBox and ControlList.
     */
    class ControlSpin : public Control
    {
    public:
      virtual ~ControlSpin();

      /**
       * setTextures(up, down, upFocus, downFocus) -- Set's textures for this control.
       * 
       * texture are image files that are used for example in the skin
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
     * ControlLabel class.
     * 
     * ControlLabel(x, y, width, height, label[, font, textColor, 
     *              disabledColor, alignment, hasPath, angle])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * label          : string or unicode - text string.
     * font           : [opt] string - font used for label text. (e.g. 'font13')
     * textColor      : [opt] hexstring - color of enabled label's label. (e.g. '0xFFFFFFFF')
     * disabledColor  : [opt] hexstring - color of disabled label's label. (e.g. '0xFFFF3300')
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h
     * hasPath        : [opt] bool - True=stores a path / False=no path.
     * angle          : [opt] integer - angle of control. (+ rotates CCW, - rotates C
     * 
     * example:
     *   - self.label = xbmcgui.ControlLabel(100, 250, 125, 75, 'Status', angle=45)\n
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
       * getLabel() -- Returns the text value for this label.
       * 
       * example:
       *   - label = self.label.getLabel()\n
       */
      virtual String getLabel() throw(UnimplementedException);

      /**
       * setLabel(label) -- Set's text for this label.
       * 
       * label          : string or unicode - text string.
       * 
       * example:
       *   - self.label.setLabel('Status')
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString) throw(UnimplementedException);
#ifndef SWIG
      ControlLabel() : Control("ControlLabel") {}

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
     * ControlEdit class.
     * 
     * ControlEdit(x, y, width, height, label[, font, textColor, 
     *              disabledColor, alignment, focusTexture, noFocusTexture])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * label          : string or unicode - text string.
     * font           : [opt] string - font used for label text. (e.g. 'font13')
     * textColor      : [opt] hexstring - color of enabled label's label. (e.g. '0xFFFFFFFF')
     * disabledColor  : [opt] hexstring - color of disabled label's label. (e.g. '0xFFFF3300')
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h
     * focusTexture   : [opt] string - filename for focus texture.
     * noFocusTexture : [opt] string - filename for no focus texture.
     * isPassword     : [opt] bool - if true, mask text value.
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.edit = xbmcgui.ControlEdit(100, 250, 125, 75, 'Status')
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
       * setLabel(label) -- Set's text heading for this edit control.
       * 
       * label          : string or unicode - text string.
       * 
       * example:
       *   - self.edit.setLabel('Status')\n
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
       * getLabel() -- Returns the text heading for this edit control.
       * 
       * example:
       *   - label = self.edit.getLabel()
       */
      virtual String getLabel() throw(UnimplementedException);

      // setText() Method
      /**
       * setText(value) -- Set's text value for this edit control.
       * 
       * value          : string or unicode - text string.
       * 
       * example:
       *   - self.edit.setText('online')\n
       */
      virtual void setText(const String& text) throw(UnimplementedException);

      // getText() Method
      /**
       * getText() -- Returns the text value for this edit control.
       * 
       * example:
       *   - value = self.edit.getText()
       */
      virtual String getText() throw(UnimplementedException);

#ifndef SWIG
      ControlEdit() : Control("ControlEdit") {}

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
     * ControlList class.
     * 
     * ControlList(x, y, width, height[, font, textColor, buttonTexture, buttonFocusTexture,
     *             selectedColor, imageWidth, imageHeight, itemTextXOffset, itemTextYOffset,
     *             itemHeight, space, alignmentY])\n"//, shadowColor])
     * 
     * x                  : integer - x coordinate of control.
     * y                  : integer - y coordinate of control.
     * width              : integer - width of control.
     * height             : integer - height of control.
     * font               : [opt] string - font used for items label. (e.g. 'font13')
     * textColor          : [opt] hexstring - color of items label. (e.g. '0xFFFFFFFF')
     * buttonTexture      : [opt] string - filename for focus texture.
     * buttonFocusTexture : [opt] string - filename for no focus texture.
     * selectedColor      : [opt] integer - x offset of label.
     * imageWidth         : [opt] integer - width of items icon or thumbnail.
     * imageHeight        : [opt] integer - height of items icon or thumbnail.
     * itemTextXOffset    : [opt] integer - x offset of items label.
     * itemTextYOffset    : [opt] integer - y offset of items label.
     * itemHeight         : [opt] integer - height of items.
     * space              : [opt] integer - space between items.
     * alignmentY         : [opt] integer - Y-axis alignment of items label - *Note, see xbfont.h
     * //"shadowColor        : [opt] hexstring - color of items label's shadow. (e.g. '0xFF000000')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.cList = xbmcgui.ControlList(100, 250, 200, 250, 'font14', space=5)
     */
    class ControlList : public Control 
    {
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
       * addItem(item) -- Add a new item to this list control.
       * 
       * item               : string, unicode or ListItem - item to add.
       * 
       * example:
       *   - cList.addItem('Reboot XBMC')
       */
      virtual void addItemStream(const String& fileOrUrl, bool sendMessage = true) throw(UnimplementedException,WindowException);
      virtual void addListItem(const XBMCAddon::xbmcgui::ListItem* listitem, bool sendMessage = true) throw(UnimplementedException,WindowException);

      /**
       * selectItem(item) -- Select an item by index number.
       * 
       * item               : integer - index number of the item to select.
       * 
       * example:
       *   - cList.selectItem(12)
       */
      virtual void selectItem(long item) throw(UnimplementedException);

      /**
       * reset() -- Clear all ListItems in this control list.
       * 
       * example:
       *   - cList.reset()\n
       */
      virtual void reset() throw (UnimplementedException);

      /**
       * getSpinControl() -- returns the associated ControlSpin object.
       * 
       * *Note, Not working completely yet -
       *        After adding this control list to a window it is not possible to change
       *        the settings of this spin control.
       * 
       * example:
       *   - ctl = cList.getSpinControl()
       */
      virtual Control* getSpinControl() throw (UnimplementedException);

      /**
       * getSelectedPosition() -- Returns the position of the selected item as an integer.
       * 
       * *Note, Returns -1 for empty lists.
       * 
       * example:
       *   - pos = cList.getSelectedPosition()
       */
      virtual long getSelectedPosition() throw (UnimplementedException);

      /**
       * getSelectedItem() -- Returns the selected item as a ListItem object.
       * 
       * *Note, Same as getSelectedPosition(), but instead of an integer a ListItem object
       *        is returned. Returns None for empty lists.
       *        See windowexample.py on how to use this.
       * 
       * example:
       *   - item = cList.getSelectedItem()
       */
      virtual XBMCAddon::xbmcgui::ListItem* getSelectedItem() throw (UnimplementedException);


      // setImageDimensions() method
      /**
       * setImageDimensions(imageWidth, imageHeight) -- Sets the width/height of items icon or thumbnail.
       * 
       * imageWidth         : [opt] integer - width of items icon or thumbnail.
       * imageHeight        : [opt] integer - height of items icon or thumbnail.
       * 
       * example:
       *   - cList.setImageDimensions(18, 18)\n
       */
      virtual void setImageDimensions(long imageWidth,long imageHeight) throw (UnimplementedException);

      // setItemHeight() method
      /**
       * setItemHeight(itemHeight) -- Sets the height of items.
       * 
       * itemHeight         : integer - height of items.
       * 
       * example:
       *   - cList.setItemHeight(25)
       */
      virtual void setItemHeight(long height) throw (UnimplementedException);

      // setSpace() method
      /**
       * setSpace(space) -- Set's the space between items.
       * 
       * space              : [opt] integer - space between items.
       * 
       * example:
       *   - cList.setSpace(5)
       */
      virtual void setSpace(int space) throw (UnimplementedException);

      // setPageControlVisible() method
      /**
       * setPageControlVisible(visible) -- Sets the spin control's visible/hidden state.
       * 
       * visible            : boolean - True=visible / False=hidden.
       * 
       * example:
       *   - cList.setPageControlVisible(True)
       */
      virtual void setPageControlVisible(bool visible) throw(UnimplementedException);

      // size() method
      /**
       * size() -- Returns the total number of items in this list control as an integer.
       * 
       * example:
       *   - cnt = cList.size()
       */
      virtual long size() throw (UnimplementedException);


      // getItemHeight() Method
      /**
       * getItemHeight() -- Returns the control's current item height as an integer.
       * 
       * example:
       *   - item_height = self.cList.getItemHeight()\n
       */
      virtual long getItemHeight() throw(UnimplementedException);

      // getSpace() Method
      /**
       * getSpace() -- Returns the control's space between items as an integer.
       * 
       * example:
       *   - gap = self.cList.getSpace()\n
       */
      virtual long getSpace() throw (UnimplementedException);

      // getListItem() method
      /**
       * getListItem(index) -- Returns a given ListItem in this List.
       * 
       * index           : integer - index number of item to return.
       * 
       * *Note, throws a ValueError if index is out of range.
       * 
       * example:
       *   - listitem = cList.getListItem(6)\n
       */
      virtual XBMCAddon::xbmcgui::ListItem* getListItem(int index) throw (UnimplementedException,WindowException);

      /**
       * setStaticContent(items) -- Fills a static list with a list of listitems.
       * 
       * items                : List - list of listitems to add.
       * 
       * *Note, You can use the above as keywords for arguments.
       * 
       * example:
       *   - cList.setStaticContent(items=listitems)\n
       */
      virtual void setStaticContent(const ListItemList* items) throw (UnimplementedException);

#ifndef SWIG
      void sendLabelBind(int tail);

      SWIGHIDDENVIRTUAL bool canAcceptMessages(int actionId) 
      { return ((actionId == ACTION_SELECT_ITEM) | (actionId == ACTION_MOUSE_LEFT_CLICK)); }

      // This is called from AddonWindow.cpp but shouldn't be available
      //  to the scripting languages.
      ControlList() : Control("ControlList") {}

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
     * ControlFadeLabel class.
     * Control that scroll's labl
     * 
     * ControlFadeLabel(x, y, width, height[, font, textColor, alignment])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * font           : [opt] string - font used for label text. (e.g. 'font13')
     * textColor      : [opt] hexstring - color of fadelabel's labels. (e.g. '0xFFFFFFFF')
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.fadelabel = xbmcgui.ControlFadeLabel(100, 250, 200, 50, textColor='0xFFFFFFFF')
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
       * addLabel(label) -- Add a label to this control for scrolling.
       * 
       * label          : string or unicode - text string.
       * 
       * example:
       *   - self.fadelabel.addLabel('This is a line of text that can scroll.')
       */
       virtual void addLabel(const String& label) throw (UnimplementedException);

      /**
       * reset() -- Clear this fade label.
       * 
       * example:
       *   - self.fadelabel.reset()\n
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
     * ControlTextBox class.
     * 
     * ControlTextBox(x, y, width, height[, font, textColor])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * font           : [opt] string - font used for text. (e.g. 'font13')
     * textColor      : [opt] hexstring - color of textbox's text. (e.g. '0xFFFFFFFF')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.textbox = xbmcgui.ControlTextBox(100, 250, 300, 300, textColor='0xFFFFFFFF')
     */
    class ControlTextBox : public Control
    {
    public:
      ControlTextBox(long x, long y, long width, long height, 
                     const char* font = NULL, 
                     const char* textColor = NULL);

      // SetText() Method
      /**
       * setText(text) -- Set's the text for this textbox.
       * 
       * text           : string or unicode - text string.
       * 
       * example:
       *   - self.textbox.setText('This is a line of text that can wrap.')
       */
      virtual void setText(const String& text) throw(UnimplementedException);

      // reset() Method
      /**
       * reset() -- Clear's this textbox.
       * 
       * example:
       *   - self.textbox.reset()\n
       */
      virtual void reset() throw(UnimplementedException);

      // scroll() Method
      /**
       * scroll(position) -- Scrolls to the given position.
       * 
       * id           : integer - position to scroll to.
       * 
       * example:
       *   - self.textbox.scroll(10)
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
     * ControlImage class.
     * 
     * ControlImage(x, y, width, height, filename[, aspectRatio, colorDiffuse])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * filename       : string - image filename.
     * aspectRatio    : [opt] integer - (values 0 = stretch (default), 1 = scale up (crops), 2 = scale down (black bar
     * colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.image = xbmcgui.ControlImage(100, 250, 125, 75, aspectRatio=2)
     */
    class ControlImage : public Control
    {
    public:
      ControlImage(long x, long y, long width, long height, 
                   const char* filename, long aspectRatio = 0,
                   const char* colorDiffuse = NULL);

      /**
       * setImage(filename) -- Changes the image.
       * 
       * filename       : string - image filename.
       * 
       * example:
       *   - self.image.setImage('special://home/scripts/test.png')
       */
      virtual void setImage(const char* imageFilename) throw (UnimplementedException);

      /**
       * setColorDiffuse(colorDiffuse) -- Changes the images color.
       * 
       * colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))
       * 
       * example:
       *   - self.image.setColorDiffuse('0xC0FF0000')
       */
      virtual void setColorDiffuse(const char* hexString) throw (UnimplementedException);

#ifndef SWIG
      ControlImage() : Control("ControlImage") {}

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
       * setPercent(percent) -- Sets the percentage of the progressbar to show.
       * 
       * percent       : float - percentage of the bar to show.
       * 
       * *Note, valid range for percent is 0-100
       * 
       * example:
       *   - self.progress.setPercent(60)
       */
      virtual void setPercent(float pct) throw (UnimplementedException);

      /**
       * getPercent() -- Returns a float of the percent of the progress.
       * 
       * example:
       *   - print self.progress.getValue()
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
      ControlProgress() : Control("ControlProgress") {}
#endif
    };

    // ControlButton class
    /**
     * ControlButton class.
     * 
     * ControlButton(x, y, width, height, label[, focusTexture, noFocusTexture, textOffsetX, textOffsetY,
     *               alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * label          : string or unicode - text string.
     * focusTexture   : [opt] string - filename for focus texture.
     * noFocusTexture : [opt] string - filename for no focus texture.
     * textOffsetX    : [opt] integer - x offset of label.
     * textOffsetY    : [opt] integer - y offset of label.
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h
     * font           : [opt] string - font used for label text. (e.g. 'font13')
     * textColor      : [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')
     * disabledColor  : [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
     * angle          : [opt] integer - angle of control. (+ rotates CCW, - rotates CW)
     * shadowColor    : [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')
     * focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFF00FFFF')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.button = xbmcgui.ControlButton(100, 250, 200, 50, 'Status', font='font14')
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
       * setLabel([label, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's this buttons text attributes.
       * 
       * label          : [opt] string or unicode - text string.
       * font           : [opt] string - font used for label text. (e.g. 'font13')
       * textColor      : [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')
       * disabledColor  : [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
       * shadowColor    : [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')
       * focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFFFFFF00')
       * label2         : [opt] string or unicode - text string.
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - self.button.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')
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
       * setDisabledColor(disabledColor) -- Set's this buttons disabled color.
       * 
       * disabledColor  : hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
       * 
       * example:
       *   - self.button.setDisabledColor('0xFFFF3300')
       */
      virtual void setDisabledColor(const char* color) throw (UnimplementedException);

      // getLabel() Method
      /**
       * getLabel() -- Returns the buttons label as a unicode string.
       * 
       * example:
       *   - label = self.button.getLabel()
       */
      virtual String getLabel() throw (UnimplementedException);

      // getLabel2() Method
      /**
       * getLabel2() -- Returns the buttons label2 as a unicode string.
       * 
       * example:
       *   - label = self.button.getLabel2()
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

      ControlButton() : Control("ControlButton") {}
#endif
    };

    // ControlCheckMark class
    /**
     * ControlCheckMark class.
     * 
     * ControlCheckMark(x, y, width, height, label[, focusTexture, noFocusTexture,
     *                  checkWidth, checkHeight, alignment, font, textColor, disabledColor])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * label          : string or unicode - text string.
     * focusTexture   : [opt] string - filename for focus texture.
     * noFocusTexture : [opt] string - filename for no focus texture.
     * checkWidth     : [opt] integer - width of checkmark.
     * checkHeight    : [opt] integer - height of checkmark.
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h
     * font           : [opt] string - font used for label text. (e.g. 'font13')
     * textColor      : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')
     * disabledColor  : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.checkmark = xbmcgui.ControlCheckMark(100, 250, 200, 50, 'Status', font='font14')
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
       * getSelected() -- Returns the selected status for this checkmark as a bool.
       * 
       * example:
       *   - selected = self.checkmark.getSelected()
       */
      virtual bool getSelected() throw (UnimplementedException);

      // setSelected() Method
      /**
       * setSelected(isOn) -- Sets this checkmark status to on or off.
       * 
       * isOn           : bool - True=selected (on) / False=not selected (off)
       * 
       * example:
       *   - self.checkmark.setSelected(True)
       */
      virtual void setSelected(bool selected) throw (UnimplementedException);

      // setLabel() Method
      /**
       * setLabel(label[, font, textColor, disabledColor]) -- Set's this controls text attributes.
       * 
       * label          : string or unicode - text string.
       * font           : [opt] string - font used for label text. (e.g. 'font13')
       * textColor      : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')
       * disabledColor  : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')
       * 
       * example:
       *   - self.checkmark.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300')
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
       * setDisabledColor(disabledColor) -- Set's this controls disabled color.
       * 
       * disabledColor  : hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')
       * 
       * example:
       *   - self.checkmark.setDisabledColor('0xFFFF3300')
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

      ControlCheckMark() : Control("ControlCheckMark") {}
#endif
    };

    // ControlGroup class
    /**
     * ControlGroup class.
     * 
     * ControlGroup(x, y, width, height
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * example:
     *   - self.group = xbmcgui.ControlGroup(100, 250, 125, 75)
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
       * setSelected(selected) -- Sets the radio buttons's selected status.
       * 
       * selected            : bool - True=selected (on) / False=not selected (off)
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - self.radiobutton.setSelected(True)
       */
      virtual void setSelected(bool selected) throw (UnimplementedException);

      // isSelected() Method
      /**
       * isSelected() -- Returns the radio buttons's selected status.
       * 
       * example:
       *   - is = self.radiobutton.isSelected()\n
       */
      virtual bool isSelected() throw (UnimplementedException);

      // setLabel() Method
      /**
       * setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's the radio buttons text attributes.
       * 
       * label          : string or unicode - text string.
       * font           : [opt] string - font used for label text. (e.g. 'font13')
       * textColor      : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')
       * disabledColor  : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')
       * shadowColor    : [opt] hexstring - color of radio button's label's shadow. (e.g. '0xFF000000')
       * focusedColor   : [opt] hexstring - color of focused radio button's label. (e.g. '0xFFFFFF00')
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - self.radiobutton.setLabel('Status', 'font14', '0xFFFFFFFF', '0xFFFF3300', '0xFF000000')
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
       * setRadioDimension(x, y, width, height) -- Sets the radio buttons's radio texture's position and size.
       * 
       * x                   : integer - x coordinate of radio texture.
       * y                   : integer - y coordinate of radio texture.
       * width               : integer - width of radio texture.
       * height              : integer - height of radio texture.
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - self.radiobutton.setRadioDimension(x=100, y=5, width=20, height=20)
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

      ControlRadioButton() : Control("ControlRadioButton") {}
#endif
    };
	
    /**
     * ControlSlider class.
     * 
     * ControlSlider(x, y, width, height[, textureback, texture, texturefocus])
     * 
     * x              : integer - x coordinate of control.
     * y              : integer - y coordinate of control.
     * width          : integer - width of control.
     * height         : integer - height of control.
     * textureback    : [opt] string - image filename.
     * texture        : [opt] string - image filename.
     * texturefocus   : [opt] string - image filename.\n"            
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.slider = xbmcgui.ControlSlider(100, 250, 350, 40)
     */
    class ControlSlider : public Control
    {
    public:
      ControlSlider(long x, long y, long width, long height, 
                    const char* textureback = NULL, 
                    const char* texture = NULL,
                    const char* texturefocus = NULL);

      /**
       * getPercent() -- Returns a float of the percent of the slider.
       * 
       * example:
       *   - print self.slider.getPercent()
       */
      virtual float getPercent() throw (UnimplementedException);

      /**
       * setPercent(50) -- Sets the percent of the slider.
       * 
       * example:
       * self.slider.setPercent(50)
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


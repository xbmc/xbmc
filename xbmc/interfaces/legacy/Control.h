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
    /**
     * Control class.
     * 
     * Base class for all controls.
     */
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

      /**
       * getId() -- Returns the control's current id as an integer.
       * 
       * example:
       *   - id = self.button.getId()
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
       *   - self.button.setEnabled(False)n
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
       * visible          : string - Visible condition.\n
       * allowHiddenFocus : bool - True=gains focus even if hidden.
       * 
       * List of Conditions - http://kodi.wiki/view/List_of_Boolean_Conditions
       * 
       * example:
       *   - self.button.setVisibleCondition('[Control.IsVisible(41) + !Control.IsVisible(12)]', True)
       */
      virtual void setVisibleCondition(const char* visible, bool allowHiddenFocus = false);

      // setEnableCondition() Method
      /**
       * setEnableCondition(enable) -- Set's the control's enabled condition.
       *     Allows XBMC to control the enabled status of the control.
       * 
       * enable           : string - Enable condition.
       * 
       * List of Conditions - http://kodi.wiki/view/List_of_Boolean_Conditions
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
       * Animating your skin - http://kodi.wiki/view/Animating_Your_Skin
       * 
       * example:
       *   - self.button.setAnimations([('focus', 'effect=zoom end=90,247,220,56 time=0',)])
       */
      virtual void setAnimations(const std::vector< Tuple<String,String> >& eventAttr);

      // setPosition() Method
      /**
       * setPosition(x, y) -- Set's the controls position.
       * 
       * x              : integer - x coordinate of control.\n
       * y              : integer - y coordinate of control.
       * 
       * *Note, You may use negative integers. (e.g sliding a control into view)
       * 
       * example:
       *   - self.button.setPosition(100, 250)
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
       * up             : control object - control to navigate to on up.\n
       * down           : control object - control to navigate to on down.\n
       * left           : control object - control to navigate to on left.\n
       * right          : control object - control to navigate to on right.
       * 
       * *Note, Same as controlUp(), controlDown(), controlLeft(), controlRight().
       *        Set to self to disable navigation for that direction.
       * 
       * Throws:
       *     - TypeError, if one of the supplied arguments is not a control type.
       *     - ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.setNavigation(self.button1, self.button2, self.button3, self.button4)
       */
      virtual void setNavigation(const Control* up, const Control* down,
                                 const Control* left, const Control* right);

      // controlUp() Method
      /**
       * controlUp(control) -- Set's the controls up navigation.
       * 
       * control        : control object - control to navigate to on up.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws: 
       *      - TypeError, if one of the supplied arguments is not a control type.
       *      - ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlUp(self.button1)
       */
      virtual void controlUp(const Control* up);

      // controlDown() Method
      /**
       * controlDown(control) -- Set's the controls down navigation.
       * 
       * control        : control object - control to navigate to on down.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws: 
       *    - TypeError, if one of the supplied arguments is not a control type.
       *    - ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlDown(self.button1)
       */
      virtual void controlDown(const Control* control);

      // controlLeft() Method
      /**
       * controlLeft(control) -- Set's the controls left navigation.
       * 
       * control        : control object - control to navigate to on left.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws:
       *    - TypeError, if one of the supplied arguments is not a control type.
       *    - ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlLeft(self.button1)
       */
      virtual void controlLeft(const Control* control);

      // controlRight() Method
      /**
       * controlRight(control) -- Set's the controls right navigation.
       * 
       * control        : control object - control to navigate to on right.
       * 
       * *Note, You can also use setNavigation(). Set to self to disable navigation.
       * 
       * Throws: 
       *    - TypeError, if one of the supplied arguments is not a control type.
       *    - ReferenceError, if one of the controls is not added to a window.
       * 
       * example:
       *   - self.button.controlRight(self.button1)
       */
      virtual void controlRight(const Control* control);

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
                               const char* downFocus);
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
     * 
     * example:
     *   - self.label = xbmcgui.ControlLabel(100, 250, 125, 75, 'Status', angle=45)
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
       *   - label = self.label.getLabel()
       */
      virtual String getLabel();

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

    // ControlEdit class
    /**
     * ControlEdit class.
     * 
     * ControlEdit(x, y, width, height, label[, font, textColor, 
     *              disabledColor, alignment, focusTexture, noFocusTexture])
     * 
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
     * isPassword     : [opt] bool - if true, mask text value.
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
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
       *   - self.edit.setLabel('Status')
       */
      virtual void setLabel(const String& label = emptyString, 
                            const char* font = NULL,
                            const char* textColor = NULL,
                            const char* disabledColor = NULL,
                            const char* shadowColor = NULL,
                            const char* focusedColor = NULL,
                            const String& label2 = emptyString);

      // getLabel() Method
      /**
       * getLabel() -- Returns the text heading for this edit control.
       * 
       * example:
       *   - label = self.edit.getLabel()
       */
      virtual String getLabel();

      // setText() Method
      /**
       * setText(value) -- Set's text value for this edit control.
       * 
       * value          : string or unicode - text string.
       * 
       * example:
       *   - self.edit.setText('online')
       */
      virtual void setText(const String& text);

      // getText() Method
      /**
       * getText() -- Returns the text value for this edit control.
       * 
       * example:
       *   - value = self.edit.getText()
       */
      virtual String getText();

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

    /**
     * ControlList class.
     * 
     * ControlList(x, y, width, height[, font, textColor, buttonTexture, buttonFocusTexture,
     *             selectedColor, imageWidth, imageHeight, itemTextXOffset, itemTextYOffset,
     *             itemHeight, space, alignmentY])n"//, shadowColor])
     * 
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
     * //"shadowColor        : [opt] hexstring - color of items label's shadow. (e.g. '0xFF000000')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().\n
     * 
     * example:
     *   - self.cList = xbmcgui.ControlList(100, 250, 200, 250, 'font14', space=5)
     */
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

      /**
       * addItem(item) -- Add a new item to this list control.
       * 
       * item               : string, unicode or ListItem - item to add.
       * 
       * example:
       *   - cList.addItem('Reboot XBMC')
       */
      virtual void addItem(const Alternative<String, const XBMCAddon::xbmcgui::ListItem* > & item, bool sendMessage = true);

      /**
       * addItems(items) -- Adds a list of listitems or strings to this list control.
       * 
       * items                : List - list of strings, unicode objects or ListItems to add.
       * 
       * *Note, You can use the above as keywords for arguments.
       * 
       * Large lists benefit considerably, than using the standard addItem()
       * 
       * example:
       *   - cList.addItems(items=listitems)
       */
      virtual void addItems(const std::vector<Alternative<String, const XBMCAddon::xbmcgui::ListItem* > > & items);

      /**
       * selectItem(item) -- Select an item by index number.
       * 
       * item               : integer - index number of the item to select.
       * 
       * example:
       *   - cList.selectItem(12)
       */
      virtual void selectItem(long item);

      /**
       * removeItem(index) -- Remove an item by index number.
       *
       * index              : integer - index number of the item to remove.
       *
       * example:
       *   - cList.removeItem(12)
       */
      virtual void removeItem(int index);

      /**
       * reset() -- Clear all ListItems in this control list.
       * 
       * example:
       *   - cList.reset()
       */
      virtual void reset();

      /**
       * getSpinControl() -- returns the associated ControlSpin object.
       * 
       * *Note, Not working completely yet -\n
       *        After adding this control list to a window it is not possible to change\n
       *        the settings of this spin control.
       * 
       * example:
       *   - ctl = cList.getSpinControl()
       */
      virtual Control* getSpinControl();

      /**
       * getSelectedPosition() -- Returns the position of the selected item as an integer.
       * 
       * *Note, Returns -1 for empty lists.
       * 
       * example:
       *   - pos = cList.getSelectedPosition()
       */
      virtual long getSelectedPosition();

      /**
       * getSelectedItem() -- Returns the selected item as a ListItem object.
       * 
       * *Note, Same as getSelectedPosition(), but instead of an integer a ListItem object\n
       *        is returned. Returns None for empty lists.\n
       *        See windowexample.py on how to use this.
       * 
       * example:
       *   - item = cList.getSelectedItem()
       */
      virtual XBMCAddon::xbmcgui::ListItem* getSelectedItem();


      // setImageDimensions() method
      /**
       * setImageDimensions(imageWidth, imageHeight) -- Sets the width/height of items icon or thumbnail.
       * 
       * imageWidth         : [opt] integer - width of items icon or thumbnail.\n
       * imageHeight        : [opt] integer - height of items icon or thumbnail.
       * 
       * example:
       *   - cList.setImageDimensions(18, 18)
       */
      virtual void setImageDimensions(long imageWidth,long imageHeight);

      // setItemHeight() method
      /**
       * setItemHeight(itemHeight) -- Sets the height of items.
       * 
       * itemHeight         : integer - height of items.
       * 
       * example:
       *   - cList.setItemHeight(25)
       */
      virtual void setItemHeight(long height);

      // setSpace() method
      /**
       * setSpace(space) -- Set's the space between items.
       * 
       * space              : [opt] integer - space between items.
       * 
       * example:
       *   - cList.setSpace(5)
       */
      virtual void setSpace(int space);

      // setPageControlVisible() method
      /**
       * setPageControlVisible(visible) -- Sets the spin control's visible/hidden state.
       * 
       * visible            : boolean - True=visible / False=hidden.
       * 
       * example:
       *   - cList.setPageControlVisible(True)
       */
      virtual void setPageControlVisible(bool visible);

      // size() method
      /**
       * size() -- Returns the total number of items in this list control as an integer.
       * 
       * example:
       *   - cnt = cList.size()
       */
      virtual long size();


      // getItemHeight() Method
      /**
       * getItemHeight() -- Returns the control's current item height as an integer.
       * 
       * example:
       *   - item_height = self.cList.getItemHeight()
       */
      virtual long getItemHeight();

      // getSpace() Method
      /**
       * getSpace() -- Returns the control's space between items as an integer.
       * 
       * example:
       *   - gap = self.cList.getSpace()
       */
      virtual long getSpace();

      // getListItem() method
      /**
       * getListItem(index) -- Returns a given ListItem in this List.
       * 
       * index           : integer - index number of item to return.
       * 
       * *Note, throws a ValueError if index is out of range.
       * 
       * example:
       *   - listitem = cList.getListItem(6)
       */
      virtual XBMCAddon::xbmcgui::ListItem* getListItem(int index);

      /**
       * setStaticContent(items) -- Fills a static list with a list of listitems.
       * 
       * items                : List - list of listitems to add.
       * 
       * *Note, You can use the above as keywords for arguments.
       * 
       * example:
       *   - cList.setStaticContent(items=listitems)
       */
      virtual void setStaticContent(const ListItemList* items);

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

    // ControlFadeLabel class
    /**
     * ControlFadeLabel class.\n
     * Control that scroll's labl
     * 
     * ControlFadeLabel(x, y, width, height[, font, textColor, alignment])
     * 
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * font           : [opt] string - font used for label text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of fadelabel's labels. (e.g. '0xFFFFFFFF')\n
     * alignment      : [opt] integer - alignment of label - *Note, see xbfont.h
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
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
       virtual void addLabel(const String& label);

      /**
       * reset() -- Clear this fade label.
       * 
       * example:
       *   - self.fadelabel.reset()
       */
      virtual void reset();

#ifndef SWIG
      std::string strFont;
      color_t textColor;
      std::vector<std::string> vecLabels;
      uint32_t align;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      ControlFadeLabel() {}
#endif
    };

    /**
     * ControlTextBox class.
     * 
     * ControlTextBox(x, y, width, height[, font, textColor])
     * 
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * font           : [opt] string - font used for text. (e.g. 'font13')\n
     * textColor      : [opt] hexstring - color of textbox's text. (e.g. '0xFFFFFFFF')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
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
      virtual void setText(const String& text);

      // getText() Method
      /**
       * getText() -- Returns the text value for this textbox.
       *
       * example:
       *   - text = self.text.getText()
       */
      virtual String getText();

      // reset() Method
      /**
       * reset() -- Clear's this textbox.
       * 
       * example:
       *   - self.textbox.reset()
       */
      virtual void reset();

      // scroll() Method
      /**
       * scroll(position) -- Scrolls to the given position.
       * 
       * id           : integer - position to scroll to.
       * 
       * example:
       *   - self.textbox.scroll(10)
       */
      virtual void scroll(long id);

      // autoScroll() Method
      /**
       * autoScroll(delay, time, repeat) -- Set autoscrolling times.
       *
       * delay           : integer - Scroll delay (in ms)
       * time            : integer - Scroll time (in ms)
       * repeat          : integer - Repeat time
       *
       * example:
       *   - self.textbox.autoScroll(1, 2, 1)
       */
      virtual void autoScroll(int delay, int time, int repeat);

#ifndef SWIG
      std::string strFont;
      color_t textColor;

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      ControlTextBox() {}
#endif
    };

    // ControlImage class
    /**
     * ControlImage class.
     * 
     * ControlImage(x, y, width, height, filename[, aspectRatio, colorDiffuse])
     * 
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * filename       : string - image filename.\n
     * aspectRatio    : [opt] integer - (values 0 = stretch (default), 1 = scale up (crops), 2 = scale down (black bar\n
     * colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
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
       * setImage(filename, useCache) -- Changes the image.
       * 
       * filename       : string - image filename.
       * useCache       : [opt] bool - true/use cache, false/don't use cache
       * 
       * example:
       *   - self.image.setImage('special://home/scripts/test.png')
       */
      virtual void setImage(const char* imageFilename, const bool useCache = true);

      /**
       * setColorDiffuse(colorDiffuse) -- Changes the images color.
       * 
       * colorDiffuse   : hexString - (example, '0xC0FF0000' (red tint))
       * 
       * example:
       *   - self.image.setColorDiffuse('0xC0FF0000')
       */
      virtual void setColorDiffuse(const char* hexString);

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
      virtual void setPercent(float pct);

      /**
       * getPercent() -- Returns a float of the percent of the progress.
       * 
       * example:
       *   - print self.progress.getValue()
       */
       virtual float getPercent();

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

    // ControlButton class
    /**
     * ControlButton class.
     * 
     * ControlButton(x, y, width, height, label[, focusTexture, noFocusTexture, textOffsetX, textOffsetY,
     *               alignment, font, textColor, disabledColor, angle, shadowColor, focusedColor])
     * 
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
     * focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFF00FFFF')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
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
       * label          : [opt] string or unicode - text string.\n
       * font           : [opt] string - font used for label text. (e.g. 'font13')\n
       * textColor      : [opt] hexstring - color of enabled button's label. (e.g. '0xFFFFFFFF')\n
       * disabledColor  : [opt] hexstring - color of disabled button's label. (e.g. '0xFFFF3300')\n
       * shadowColor    : [opt] hexstring - color of button's label's shadow. (e.g. '0xFF000000')\n
       * focusedColor   : [opt] hexstring - color of focused button's label. (e.g. '0xFFFFFF00')\n
       * label2         : [opt] string or unicode - text string.
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
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
                            const String& label2 = emptyString);

      // setDisabledColor() Method
      /**
       * setDisabledColor(disabledColor) -- Set's this buttons disabled color.
       * 
       * disabledColor  : hexstring - color of disabled button's label. (e.g. '0xFFFF3300')
       * 
       * example:
       *   - self.button.setDisabledColor('0xFFFF3300')
       */
      virtual void setDisabledColor(const char* color);

      // getLabel() Method
      /**
       * getLabel() -- Returns the buttons label as a unicode string.
       * 
       * example:
       *   - label = self.button.getLabel()
       */
      virtual String getLabel();

      // getLabel2() Method
      /**
       * getLabel2() -- Returns the buttons label2 as a unicode string.
       * 
       * example:
       *   - label = self.button.getLabel2()
       */
      virtual String getLabel2();
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

    // ControlCheckMark class
    /**
     * ControlCheckMark class.
     * 
     * ControlCheckMark(x, y, width, height, label[, focusTexture, noFocusTexture,
     *                  checkWidth, checkHeight, alignment, font, textColor, disabledColor])
     * 
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
     * disabledColor  : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
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
      virtual bool getSelected();

      // setSelected() Method
      /**
       * setSelected(isOn) -- Sets this checkmark status to on or off.
       * 
       * isOn           : bool - True=selected (on) / False=not selected (off)
       * 
       * example:
       *   - self.checkmark.setSelected(True)
       */
      virtual void setSelected(bool selected);

      // setLabel() Method
      /**
       * setLabel(label[, font, textColor, disabledColor]) -- Set's this controls text attributes.
       * 
       * label          : string or unicode - text string.\n
       * font           : [opt] string - font used for label text. (e.g. 'font13')\n
       * textColor      : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')\n
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
                            const String& label2 = emptyString);

      // setDisabledColor() Method
      /**
       * setDisabledColor(disabledColor) -- Set's this controls disabled color.
       * 
       * disabledColor  : hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')
       * 
       * example:
       *   - self.checkmark.setDisabledColor('0xFFFF3300')
       */
      virtual void setDisabledColor(const char* color);

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

    // ControlGroup class
    /**
     * ControlGroup class.
     * 
     * ControlGroup(x, y, width, height
     * 
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.
     * example:
     *   - self.group = xbmcgui.ControlGroup(100, 250, 125, 75)
     */
    class ControlGroup : public Control 
    {
    public:
      ControlGroup(long x, long y, long width, long height);

#ifndef SWIG
      SWIGHIDDENVIRTUAL CGUIControl* Create();

      inline ControlGroup() {}
#endif
    };

    // ControlRadioButton class
    /**
     * ControlRadioButton class.
     * 
     * ControlRadioButton(x, y, width, height, label[, focusOnTexture, noFocusOnTexture,\n
     *                   focusOffTexture, noFocusOffTexture, focusTexture, noFocusTexture,\n
     *                   textOffsetX, textOffsetY, alignment, font, textColor, disabledColor])
     * 
     * x                 : integer - x coordinate of control.\n
     * y                 : integer - y coordinate of control.\n
     * width             : integer - width of control.\n
     * height            : integer - height of control.\n
     * label             : string or unicode - text string.\n
     * focusOnTexture    : [opt] string - filename for radio ON focused texture.\n
     * noFocusOnTexture  : [opt] string - filename for radio ON not focused texture.\n
     * focusOfTexture    : [opt] string - filename for radio OFF focused texture.\n
     * noFocusOffTexture : [opt] string - filename for radio OFF not focused texture.\n
     * focusTexture      : [opt] string - filename for radio ON texture (deprecated, use focusOnTexture and noFocusOnTexture).\n
     * noFocusTexture    : [opt] string - filename for radio OFF texture (deprecated, use focusOffTexture and noFocusOffTexture).\n
     * textOffsetX       : [opt] integer - horizontal text offset\n
     * textOffsetY       : [opt] integer - vertical text offset\n
     * alignment         : [opt] integer - alignment of label - *Note, see xbfont.h\n
     * font              : [opt] string - font used for label text. (e.g. 'font13')\n
     * textColor         : [opt] hexstring - color of enabled checkmark's label. (e.g. '0xFFFFFFFF')\n
     * disabledColor     : [opt] hexstring - color of disabled checkmark's label. (e.g. '0xFFFF3300')
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     *        After you create the control, you need to add it to the window with addControl().
     * 
     * example:
     *   - self.radiobutton = xbmcgui.ControlRadioButton(100, 250, 200, 50, 'Enable', font='font14')
     */
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
                         const char* shadowColor = NULL, const char* focusedColor = NULL);

      // setSelected() Method
      /**
       * setSelected(selected) -- Sets the radio buttons's selected status.
       * 
       * selected            : bool - True=selected (on) / False=not selected (off)
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - self.radiobutton.setSelected(True)
       */
      virtual void setSelected(bool selected);

      // isSelected() Method
      /**
       * isSelected() -- Returns the radio buttons's selected status.
       * 
       * example:
       *   - is = self.radiobutton.isSelected()
       */
      virtual bool isSelected();

      // setLabel() Method
      /**
       * setLabel(label[, font, textColor, disabledColor, shadowColor, focusedColor]) -- Set's the radio buttons text attributes.
       * 
       * label          : string or unicode - text string.\n
       * font           : [opt] string - font used for label text. (e.g. 'font13')\n
       * textColor      : [opt] hexstring - color of enabled radio button's label. (e.g. '0xFFFFFFFF')\n
       * disabledColor  : [opt] hexstring - color of disabled radio button's label. (e.g. '0xFFFF3300')\n
       * shadowColor    : [opt] hexstring - color of radio button's label's shadow. (e.g. '0xFF000000')\n
       * focusedColor   : [opt] hexstring - color of focused radio button's label. (e.g. '0xFFFFFF00')
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
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
                            const String& label2 = emptyString);

      // setRadioDimension() Method
      /**
       * setRadioDimension(x, y, width, height) -- Sets the radio buttons's radio texture's position and size.
       * 
       * x                   : integer - x coordinate of radio texture.\n
       * y                   : integer - y coordinate of radio texture.\n
       * width               : integer - width of radio texture.\n
       * height              : integer - height of radio texture.
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - self.radiobutton.setRadioDimension(x=100, y=5, width=20, height=20)
       */
      virtual void setRadioDimension(long x, long y, long width, long height);

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
	
    /**
     * ControlSlider class.
     * 
     * ControlSlider(x, y, width, height[, textureback, texture, texturefocus])
     * 
     * x              : integer - x coordinate of control.\n
     * y              : integer - y coordinate of control.\n
     * width          : integer - width of control.\n
     * height         : integer - height of control.\n
     * textureback    : [opt] string - image filename.\n
     * texture        : [opt] string - image filename.\n
     * texturefocus   : [opt] string - image filename.n"
     *
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
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
      virtual float getPercent();

      /**
       * setPercent(50) -- Sets the percent of the slider.
       * 
       * example:
       *   - self.slider.setPercent(50)
       */
      virtual void setPercent(float pct);

#ifndef SWIG
      std::string strTextureBack;
      std::string strTexture;
      std::string strTextureFoc;    

      SWIGHIDDENVIRTUAL CGUIControl* Create();

      inline ControlSlider() {}
#endif
    };
  }
}


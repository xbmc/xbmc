/*!
	\file GUISelectButtonControl.h
	\brief 
	*/

#ifndef GUILIB_GUIWINDOWSELECTCONTROL_H
#define GUILIB_GUIWINDOWSELECTCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "stdstring.h"
#include "guiButtonControl.h"
using namespace std;

/*!
	\ingroup controls
	\brief Button with multi selection choice.

	Behaves like a normal button control, but when pressing,
	it can show multiple strings. The user can choose one by
	moving left or right. \n
	\n
	Messages the button reactes on: \n

	- GUI_MSG_LABEL_ADD \n
	Add a label to the control. Use CGUIMessage::SetLabel
	to set the label text.
	- GUI_MSG_LABEL_RESET \n
	Remove all labels from the control.
	- GUI_MSG_ITEM_SELECTED \n
	After sending this message the CGUIMessage::GetParam1
	contains the selected label as an integer.
	\note The order of the items depends on the order they have been added to 
	the control using GUI_MSG_LABEL_ADD.
	- GUI_MSG_ITEM_SELECT \n
	Send this message with CGUIMessage::SetParam1() set to the label
	to be selected. \n
	\n
	Example entry to define a select button in a window or as reference control: \n
	\verbatim
    <control>
      <description>default select button</description
      <type>selectbutton</type>
      <id>6</id>
      <posX>60</posX>
      <posY>192</posY>
      <width>130</width>
      <height>32</height>
      <label>132</label>
      <font>font13</font>
      <textureFocus>button-focus.png</textureFocus>
      <textureNoFocus>button-nofocus.jpg</textureNoFocus>
      <texturebg>button-focus.png</texturebg>
      <textureLeft>scroll-left.png</textureLeft>
      <textureRight>scroll-right.png</textureRight>
      <font>font13</font>
      <textcolor>ffffffff</textcolor>
      <colordiffuse>ffffffff</colordiffuse>
      <disabledcolor>60ffffff</disabledcolor>
      <onleft>50</onleft>
      <onright>50</onright>
      <onup>3</onup>
      <ondown>7</ondown>
    </control>
		\endverbatim

	\sa CGUIMessage
	*/
class CGUISelectButtonControl : public CGUIButtonControl
{
public:
  CGUISelectButtonControl(DWORD dwParentID, DWORD dwControlId, 
													DWORD dwPosX, DWORD dwPosY, 
													DWORD dwWidth, DWORD dwHeight, 
													const CStdString& strButtonFocus, const CStdString& strButton,
													const CStdString& strSelectBackground,
													const CStdString& strSelectArrowLeft, const CStdString& strSelectArrowLeftFocus,
													const CStdString& strSelectArrowRight, const CStdString& strSelectArrowRightFocus);
  virtual ~CGUISelectButtonControl(void);
  virtual void						Render();
  virtual void						OnAction(const CAction &action) ;
  virtual bool						OnMessage(CGUIMessage& message);

	virtual void PreAllocResources();
  virtual void						AllocResources();
  virtual void						FreeResources();
	///	\brief Return the texture of the left arrow that is shown, when the control is in select mode
	const CStdString				GetTextureLeft() const { return m_imgLeft.GetFileName();};
	///	\brief Return the texture of the left arrow focused that is shown, when the control is in select mode
	const CStdString				GetTextureLeftFocus() const { return m_imgLeftFocus.GetFileName();};
	///	\brief Return the texture of the right arrow that is shown, when the control is in select mode
	const CStdString				GetTextureRight() const { return m_imgRight.GetFileName();};
	///	\brief Return the texture of the right arrow focused that is shown, when the control is in select mode
	const CStdString				GetTextureRightFocus() const { return m_imgRightFocus.GetFileName();};
	///	\brief Return the texture of the background that is shown, when the control is in select mode
	const CStdString				GetTextureBackground() const { return m_imgBackground.GetFileName();};

protected:
  virtual void						Update();
	bool										m_bShowSelect;
	CGUIImage								m_imgBackground;
	CGUIImage								m_imgLeft;
	CGUIImage								m_imgLeftFocus;
	CGUIImage								m_imgRight;
	CGUIImage								m_imgRightFocus;
	vector<wstring>					m_vecItems;
	int											m_iCurrentItem;
	int											m_iDefaultItem;
	int											m_iStartFrame;
	bool										m_bLeftSelected;
	bool										m_bRightSelected;
	DWORD										m_dwTicks;
};
#endif

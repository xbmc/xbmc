/*!
\file GUIEmbeddedBrowserWindowObserver.h
\brief
*/

#ifndef GUILIB_GUIEMBEDDEDBROWSERWINDOWOBSERVER_H
#define GUILIB_GUIEMBEDDEDBROWSERWINDOWOBSERVER_H

#pragma once

/*
 *      Copyright (C) 2010 Team XBMC
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

#include "StdString.h"

#include "../lib/llqtwebkit/llqtwebkit.h"

class CGUIEmbeddedBrowserWindowObserver : public LLEmbeddedBrowserWindowObserver
{
public:
  CGUIEmbeddedBrowserWindowObserver();
  void init();
  void reset(void);
  void reshape(float widthIn, float heightIn);
  void idle();
  void Render(float xPos, float yPos, float width, float height);
  LLQtWebKit::EKeyboardModifier getLLQtWebKitKeyboardModifierCode();
  void mouseButton(int button, int state, int xIn, int yIn);
  void mouseMove(int xIn , int yIn);
  void keyboard(unsigned char keyIn, bool isDown);
  void keyboardSpecial(int specialIn, bool isDown);
  void onPageChanged( const EventType &eventIn);
  void onNavigateBegin(const EventType &eventIn);
  void onNavigateComplete(const EventType &eventIn);
  void onUpdateProgress(const EventType &eventIn);
  void onStatusTextChange(const EventType &eventIn);
  void onTitleChange(const EventType &eventIn);
  void onLocationChange(const EventType &eventIn);
  void onClickLinkHref(const EventType& eventIn);
  void SetTexture(unsigned int *appTexture);
  void SetXPosition(float xPos);
  void SetYPosition(float yPos);
  void SetWidth(float width);
  void SetHeight(float height);
  void SetParams(float xPos, float yPos, float width, float height);
  float getXPos();
  float getYPos();
  float getAppWindowWidth();
  float getAppWindowHeight();
  CStdString getAppWindowName();
  void* getNativeWindowHandle();
  void Back();
  void Forward();
  void Reload();
  void Stop();
  void Home();
  void Go(const CStdString &url);
private:
  float m_xPos;
  float m_yPos;
  float m_appWindowWidth;
  float m_appWindowHeight;
  float m_browserWindowWidth;
  float m_browserWindowHeight;
  float m_appTextureWidth;
  float m_appTextureHeight;
  unsigned int m_appTexture;
  int m_browserWindowId;
  CStdString m_appWindowName;
  CStdString m_homeUrl;
  bool m_needsUpdate;
};

/* TODO: See if there's a better way for this than using a global */
extern CGUIEmbeddedBrowserWindowObserver g_webBrowserObserver;

#endif

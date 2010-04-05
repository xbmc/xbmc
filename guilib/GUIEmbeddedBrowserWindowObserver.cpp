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

#include "GUIEmbeddedBrowserWindowObserver.h"
#include "GraphicContext.h"
#include "GUIDialogWebBrowserOSD.h"
#include "GUIWindowManager.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/File.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"

#include <cstdlib> // TODO: Use CLog::Log instead of std::cout.
#include <cstdio>
#include <GL/gl.h>

/* Set this to a youtube vid until keyboard input is working */
#define DEFAULT_HOMEURL "http://www.youtube.com/watch?v=Ohjkj6zOucs&hd=1&fs=0"
#define SCROLL_VALUE_INCREMENTS      10
#define SCROLL_VALUE_PAGE_INCREMENTS 500

CGUIEmbeddedBrowserWindowObserver g_webBrowserObserver;

CGUIEmbeddedBrowserWindowObserver::CGUIEmbeddedBrowserWindowObserver() :
  m_xPos(0),
  m_yPos(0),
  m_appWindowWidth(800),
  m_appWindowHeight(900),
  m_browserWindowWidth(m_appWindowWidth),
  m_browserWindowHeight(m_appWindowHeight),
  m_appTextureWidth(-1),
  m_appTextureHeight(-1),
  m_appTexture(0),
  m_browserWindowId(0),
  m_appWindowName("XBMC Web Browser"),
  m_homeUrl(),
  m_needsUpdate(true)
{}

void CGUIEmbeddedBrowserWindowObserver::init()
{
  g_graphicsContext.CaptureStateBlock();

//   glClearColor( 0.0f, 0.0f, 0.0f, 0.5f);
//   glEnable(GL_COLOR_MATERIAL);
//   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
//   glEnable(GL_TEXTURE_2D);
//   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//   glEnable(GL_CULL_FACE);

  // calculate texture size required
  m_appTextureWidth = m_browserWindowWidth;
  m_appTextureHeight = m_browserWindowHeight;

  // create the texture used to display the browser data
  glGenTextures(1, &m_appTexture);
  glBindTexture(GL_TEXTURE_2D, m_appTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_appTextureWidth, m_appTextureHeight,
    0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  g_graphicsContext.ApplyStateBlock();

  // set a home url
  m_homeUrl =
    CSpecialProtocol::TranslatePath("special://xbmc/tools/testpage.html");
  if (!XFILE::CFile::Exists(m_homeUrl))
  {
    m_homeUrl = DEFAULT_HOMEURL;
  }

  // create a single browser window and set things up.
  CStdString applicationDir =
    CSpecialProtocol::TranslatePath("special://masterprofile");

  CStdString componentDir = applicationDir;

  CStdString profileDir =
    CSpecialProtocol::TranslatePath(
      "special://masterprofile/web_browser_profile");

  LLQtWebKit::getInstance()->init(applicationDir, componentDir, profileDir,
    getNativeWindowHandle());

  /* set host language test (in reality, string will be language code passed
   * into client)
   * IMPORTANT: must be called before createBrowserWindow(...)
   */
  LLQtWebKit::getInstance()->setHostLanguage("EN-AB-CD-EF");

  // enable Javascript
  LLQtWebKit::getInstance()->enableJavascript(true);

  // enable Plugins
  LLQtWebKit::getInstance()->enablePlugins(true);

  // make a browser window
  m_browserWindowId = LLQtWebKit::getInstance()->createBrowserWindow(
    m_browserWindowWidth, m_browserWindowHeight);

  // tell LLQtWebKit about the size of the browser window
  LLQtWebKit::getInstance()->setSize(m_browserWindowId, m_browserWindowWidth,
    m_browserWindowHeight);

  // observer events that LLQtWebKit emits
  LLQtWebKit::getInstance()->addObserver(m_browserWindowId, this);

  // append details to agent string
  LLQtWebKit::getInstance()->setBrowserAgentId(m_appWindowName);

  // don't flip bitmap
  LLQtWebKit::getInstance()->flipWindow(m_browserWindowId, false);

  // target name we open in external browser
  LLQtWebKit::getInstance()->setExternalTargetName(m_browserWindowId,
    "XBMC Web Browser");

  // turn on option to catch JavaScript window.open commands and open in same window
  LLQtWebKit::getInstance()->setWindowOpenBehavior(m_browserWindowId,
    LLQtWebKit::WOB_REDIRECT_TO_SELF);

  // go to the "home page"
  LLQtWebKit::getInstance()->navigateTo(m_browserWindowId, m_homeUrl);
}

void CGUIEmbeddedBrowserWindowObserver::reset(void)
{
  // unhook observer
  LLQtWebKit::getInstance()->remObserver(m_browserWindowId, this);

  // clean up
  LLQtWebKit::getInstance()->reset();
}

void CGUIEmbeddedBrowserWindowObserver::reshape(float widthIn, float heightIn)
{
  if ( heightIn == 0 )
    heightIn = 1;

  LLQtWebKit::getInstance()->setSize(m_browserWindowId,
    static_cast<int>(widthIn), static_cast<int>(heightIn));
  m_needsUpdate = true;

  g_graphicsContext.CaptureStateBlock();

//   glMatrixMode(GL_PROJECTION);
//   glLoadIdentity();

  g_graphicsContext.SetViewPort(m_xPos, m_yPos, widthIn, heightIn);
//   glOrtho(0.0f, widthIn, heightIn, 0.0f, -1.0f, 1.0f);

  // we use these elsewhere so save
  m_appWindowWidth = m_browserWindowWidth = widthIn;
  m_appWindowHeight = m_browserWindowHeight = heightIn;

//   glMatrixMode(GL_MODELVIEW);
//   glLoadIdentity();

  g_graphicsContext.ApplyStateBlock();

  m_needsUpdate = true;
  idle();
}

void CGUIEmbeddedBrowserWindowObserver::idle()
{
  LLQtWebKit::getInstance()->pump(100);

  // onPageChanged event sets this
  if (m_needsUpdate)
    /* grab a page but don't reset 'needs update' flag until we've written it to
     * the texture in display()
     */
    LLQtWebKit::getInstance()->grabBrowserWindow(m_browserWindowId);
}

void CGUIEmbeddedBrowserWindowObserver::Render(float xPos, float yPos,
  float width, float height)
{
  m_xPos = xPos;
  m_yPos = yPos;
  reshape(width, height);

  g_graphicsContext.CaptureStateBlock();

  // clear screen
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
// 
//   glLoadIdentity();

  // use the browser texture
  glBindTexture(GL_TEXTURE_2D, m_appTexture);

  // valid window ?
  if (m_browserWindowId)
  {
    // needs to be updated?
    if (m_needsUpdate)
    {
      // grab the page
      const unsigned char* pixels =
        LLQtWebKit::getInstance()->getBrowserWindowPixels(m_browserWindowId);
      if (pixels)
      {
        // write them into the texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            /* because sometimes the rowspan != width * bytes per pixel
             * (mBrowserWindowWidth)
             */
          LLQtWebKit::getInstance()->getBrowserRowSpan(m_browserWindowId) /
            LLQtWebKit::getInstance()->getBrowserDepth( m_browserWindowId),
            m_browserWindowHeight,
#ifdef _WINDOWS
          LLQtWebKit::getInstance()->getBrowserDepth(m_browserWindowId ) == 3
            ? GL_RGBA : GL_RGBA,
#elif defined(__APPLE__)
          GL_RGBA,
#else
          GL_RGBA,
#endif
          GL_UNSIGNED_BYTE, pixels);
      }

      // flag as already updated
      m_needsUpdate = false;
    }
  }

  // scale the texture so that it fits the screen
  GLfloat textureScaleX =
    (GLfloat)m_browserWindowWidth / (GLfloat)m_appTextureWidth;
  GLfloat textureScaleY =
    (GLfloat)m_browserWindowHeight / (GLfloat)m_appTextureHeight;

  // draw the single quad full screen (orthographic)
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glScalef(textureScaleX, textureScaleY, 1.0f);

  glEnable(GL_TEXTURE_2D);
  glColor3f(1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2d(m_appWindowWidth, 0);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2d(0, 0);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2d(0, m_appWindowHeight);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2d(m_appWindowWidth, m_appWindowHeight);
  glEnd();

  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  g_graphicsContext.ApplyStateBlock();
}

void CGUIEmbeddedBrowserWindowObserver::mouseButton(int button, int state,
  int xIn, int yIn)
{
// TODO: Implement this with XBMC classes
//   // texture is scaled to fit the screen so we scale mouse coords in the same way
//   xIn = ( xIn * m_browserWindowWidth ) / m_appWindowWidth;
//   yIn = ( yIn * m_browserWindowHeight ) / m_appWindowHeight;
// 
//   if ( button == GLUT_LEFT_BUTTON )
//   {
//     if ( state == GLUT_DOWN )
//     {
//       // send event to LLQtWebKit
//       LLQtWebKit::getInstance()->mouseEvent m_browserWindowId,
//         LLQtWebKit::ME_MOUSE_DOWN, LLQtWebKit::MB_MOUSE_BUTTON_LEFT, xIn, yIn,
//         getLLQtWebKitKeyboardModifierCode());
//     }
//     else
//     if ( state == GLUT_UP )
//     {
//       // send event to LLQtWebKit
//       LLQtWebKit::getInstance()->mouseEvent(m_browserWindowId,
//         LLQtWebKit::ME_MOUSE_UP, LLQtWebKit::MB_MOUSE_BUTTON_LEFT, xIn, yIn,
//         getLLQtWebKitKeyboardModifierCode());
// 
//       // this seems better than sending focus on mouse down (still need to improve this)
//       LLQtWebKit::getInstance()->focusBrowser(m_browserWindowId, true);
//     }
//   }
}

void CGUIEmbeddedBrowserWindowObserver::mouseMove(int xIn , int yIn)
{
  // texture is scaled to fit the screen so we scale mouse coords in the same way
  xIn = (xIn * m_browserWindowWidth) / m_appWindowWidth;
  yIn = (yIn * m_browserWindowHeight) / m_appWindowHeight;

  // send event to LLQtWebKit
  LLQtWebKit::getInstance()->mouseEvent(m_browserWindowId,
    LLQtWebKit::ME_MOUSE_MOVE, LLQtWebKit::MB_MOUSE_BUTTON_LEFT, xIn, yIn,
    LLQtWebKit::KM_MODIFIER_NONE);
}

EVENT_RESULT CGUIEmbeddedBrowserWindowObserver::mouseEvent(const CPoint &point,
  const CMouseEvent &event)
{
  // adjust mouse coordinates for inputting to browser
  int x = (point.x * m_browserWindowWidth) / m_appWindowWidth;
  int y = (point.y * m_browserWindowHeight) / m_appWindowHeight;

  // translate our mouse events to LLQtWebKit mouse events
  int id = event.m_id;
  LLQtWebKit::EMouseEvent llevent = LLQtWebKit::ME_MOUSE_MOVE;
  LLQtWebKit::EMouseButton llmousebutton = LLQtWebKit::MB_MOUSE_BUTTON_LEFT;
  if (id == ACTION_MOUSE_LEFT_CLICK)
  {
    llevent = LLQtWebKit::ME_MOUSE_DOWN;
  }
  else if (id == ACTION_MOUSE_RIGHT_CLICK)
  {
    llevent = LLQtWebKit::ME_MOUSE_DOWN;
    llmousebutton = LLQtWebKit::MB_MOUSE_BUTTON_RIGHT;
  }
  else if (id == ACTION_MOUSE_MIDDLE_CLICK)
  {
    llevent = LLQtWebKit::ME_MOUSE_DOWN;
    llmousebutton = LLQtWebKit::MB_MOUSE_BUTTON_MIDDLE;
  }
  else if (id == ACTION_MOUSE_DOUBLE_CLICK)
  {
    llevent = LLQtWebKit::ME_MOUSE_DOUBLE_CLICK;
  }

  // Send a mouse event to LLQtWebKit
  if (llevent == LLQtWebKit::ME_MOUSE_DOWN)
  {
    if (LLQtWebKit::getInstance()->mouseEvent(m_browserWindowId, llevent,
      llmousebutton, x, y, LLQtWebKit::KM_MODIFIER_NONE) &&
      LLQtWebKit::getInstance()->mouseEvent(m_browserWindowId,
        LLQtWebKit::ME_MOUSE_UP, llmousebutton, x, y,
        LLQtWebKit::KM_MODIFIER_NONE))
      return EVENT_RESULT_HANDLED;
  }
  else if (LLQtWebKit::getInstance()->mouseEvent(m_browserWindowId, llevent,
    llmousebutton, x, y, LLQtWebKit::KM_MODIFIER_NONE))
    return EVENT_RESULT_HANDLED;
  return EVENT_RESULT_UNHANDLED;
}

bool CGUIEmbeddedBrowserWindowObserver::keyboard(const CAction &action)
{
  int id = action.GetID();
  uint32_t key = 0;
  bool do_scroll = false;
  int value = 0;
  LLQtWebKit::EOrientation orientation;
  if (id == ACTION_MOVE_LEFT)
  {
    do_scroll = true;
    value = LLQtWebKit::getInstance()->scrollBarValue(m_browserWindowId,
      LLQtWebKit::O_HORIZONTAL);
    value -= SCROLL_VALUE_INCREMENTS;
    orientation = LLQtWebKit::O_HORIZONTAL;
  }
  else if (id == ACTION_MOVE_RIGHT)
  {
    do_scroll = true;
    value = LLQtWebKit::getInstance()->scrollBarValue(m_browserWindowId,
      LLQtWebKit::O_HORIZONTAL);
    value += SCROLL_VALUE_INCREMENTS;
    orientation = LLQtWebKit::O_HORIZONTAL;
  }
  else if (id == ACTION_MOVE_UP)
  {
    do_scroll = true;
    value = LLQtWebKit::getInstance()->scrollBarValue(m_browserWindowId,
      LLQtWebKit::O_VERTICAL);
    value -= SCROLL_VALUE_INCREMENTS;
    orientation = LLQtWebKit::O_VERTICAL;
  }
  else if (id == ACTION_MOVE_DOWN)
  {
    do_scroll = true;
    value = LLQtWebKit::getInstance()->scrollBarValue(m_browserWindowId,
      LLQtWebKit::O_VERTICAL);
    value += SCROLL_VALUE_INCREMENTS;
    orientation = LLQtWebKit::O_VERTICAL;
  }
  else if (id == ACTION_PAGE_UP)
  {
    do_scroll = true;
    value = LLQtWebKit::getInstance()->scrollBarValue(m_browserWindowId,
      LLQtWebKit::O_VERTICAL);
    value -= SCROLL_VALUE_PAGE_INCREMENTS;
    orientation = LLQtWebKit::O_VERTICAL;
  }
  else if (id == ACTION_PAGE_DOWN)
  {
    do_scroll = true;
    value = LLQtWebKit::getInstance()->scrollBarValue(m_browserWindowId,
      LLQtWebKit::O_VERTICAL);
    value += SCROLL_VALUE_PAGE_INCREMENTS;
    orientation = LLQtWebKit::O_VERTICAL;
  }
  else if (id == ACTION_BACKSPACE)
  {
    key = LLQtWebKit::KEY_BACKSPACE;
  }

  // Check if we're scrolling
  if (do_scroll)
  {
    // Scroll first
    bool retval = LLQtWebKit::getInstance()->setScrollBarValue(
      m_browserWindowId, orientation, value);

    /* Display the web browser OSD if the window can't be scrolled any farther
     * up or to the left.
     */
    if (value < 0)
    {
      CGUIDialogWebBrowserOSD *pOSD =
        (CGUIDialogWebBrowserOSD *)g_windowManager.GetWindow(
          WINDOW_DIALOG_WEB_BROWSER_OSD);
      if (pOSD)
        pOSD->DoModal();
    }

    // Return the return value from LLQtWebKit::setScrollBarValue
    return retval;
  }

  CStdString str = "";
  wchar_t wc = action.GetUnicode();
  CStdStringW wc2(&wc);
  g_charsetConverter.wToUTF8(wc2, str);

  // send event to LLQtWebKit
  if (LLQtWebKit::getInstance()->keyboardEvent(m_browserWindowId,
    LLQtWebKit::KE_KEY_DOWN, key, str.c_str(), LLQtWebKit::KM_MODIFIER_NONE) &&
    LLQtWebKit::getInstance()->keyboardEvent(m_browserWindowId,
      LLQtWebKit::KE_KEY_UP, key, str.c_str(), LLQtWebKit::KM_MODIFIER_NONE))
    return true;
  return false;
}

/* Function to flag that an update is required - page grab happens in idle() so
 * we don't stall
 */
void CGUIEmbeddedBrowserWindowObserver::onPageChanged(const EventType &eventIn)
{
  m_needsUpdate = true;
}

void CGUIEmbeddedBrowserWindowObserver::onNavigateBegin(const EventType &eventIn)
{
  std::cout << "Event: begin navigation to " << eventIn.getEventUri() <<
    std::endl;
}

void CGUIEmbeddedBrowserWindowObserver::onNavigateComplete(
  const EventType &eventIn)
{
  std::cout << "Event: end navigation to " << eventIn.getEventUri() <<
    " with response status of " << eventIn.getIntValue() << std::endl;
  Render(m_xPos, m_yPos, m_appWindowWidth, m_appWindowHeight);
}

void CGUIEmbeddedBrowserWindowObserver::onUpdateProgress(
  const EventType &eventIn)
{
  std::cout << "Event: progress value updated to " << eventIn.getIntValue() <<
    std::endl;
}

void CGUIEmbeddedBrowserWindowObserver::onStatusTextChange(
  const EventType &eventIn)
{
  std::cout << "Event: status updated to " << eventIn.getStringValue() <<
    std::endl;
}

void CGUIEmbeddedBrowserWindowObserver::onTitleChange(const EventType &eventIn)
{
  std::cout << "Event: title changed to  " << eventIn.getStringValue() <<
    std::endl;
}

void CGUIEmbeddedBrowserWindowObserver::onLocationChange(
  const EventType &eventIn)
{
  std::cout << "Event: location changed to " << eventIn.getStringValue() <<
    std::endl;
}

void CGUIEmbeddedBrowserWindowObserver::onClickLinkHref(const EventType &eventIn)
{
  std::cout << "Event: clicked on link:" << std::endl;
  std::cout << "  URL:" << eventIn.getStringValue() << std::endl;

  if (LLQtWebKit::LTT_TARGET_NONE == eventIn.getLinkType())
    std::cout << "  No target attribute - opening in current window" <<
      std::endl;

  if (LLQtWebKit::LTT_TARGET_BLANK == eventIn.getLinkType())
    std::cout << "  Blank target attribute (" << eventIn.getStringValue2() <<
      ") - not navigating in this window" << std::endl;

  if (LLQtWebKit::LTT_TARGET_EXTERNAL == eventIn.getLinkType())
    std::cout << "  External target attribute (" << eventIn.getStringValue2() <<
      ") - not navigating in this window" << std::endl;

  if (LLQtWebKit::LTT_TARGET_OTHER == eventIn.getLinkType())
    std::cout << "  Other target attribute (" << eventIn.getStringValue2() <<
      ") - opening in current window" << std::endl;

  std::cout << std::endl;
}

void CGUIEmbeddedBrowserWindowObserver::SetTexture(unsigned int *appTexture)
{
  m_appTexture = *appTexture;
}

void CGUIEmbeddedBrowserWindowObserver::SetXPosition(float xPos)
{
  m_xPos = xPos;
}

void CGUIEmbeddedBrowserWindowObserver::SetYPosition(float yPos)
{
  m_yPos = yPos;
}

void CGUIEmbeddedBrowserWindowObserver::SetWidth(float width)
{
  m_appWindowWidth = m_browserWindowWidth = width;
}

void CGUIEmbeddedBrowserWindowObserver::SetHeight(float height)
{
  m_appWindowHeight = m_browserWindowHeight = height;
}

void CGUIEmbeddedBrowserWindowObserver::SetParams(float xPos, float yPos,
                                                  float width, float height)
{
  m_xPos = xPos;
  m_yPos = yPos;
  m_appWindowWidth = m_browserWindowWidth = width;
  m_appWindowHeight = m_browserWindowHeight = height;
}

float CGUIEmbeddedBrowserWindowObserver::getXPos()
{
  return m_xPos;
}

float CGUIEmbeddedBrowserWindowObserver::getYPos()
{
  return m_yPos;
}

float CGUIEmbeddedBrowserWindowObserver::getAppWindowWidth()
{
  return m_appWindowWidth;
}

float CGUIEmbeddedBrowserWindowObserver::getAppWindowHeight()
{
  return m_appWindowHeight;
}

CStdString CGUIEmbeddedBrowserWindowObserver::getAppWindowName()
{
  return m_appWindowName;
}

void* CGUIEmbeddedBrowserWindowObserver::getNativeWindowHandle()
{
  /* HACK: This implementation of the embedded browser needs a native window
   * handle.
   */
#ifdef _WINDOWS
  return FindWindow(NULL, (LPCWSTR)m_appWindowName.c_str());
#else
  return 0;
#endif
}

void CGUIEmbeddedBrowserWindowObserver::Back()
{
  if (!LLQtWebKit::getInstance()->userAction(m_browserWindowId,
    LLQtWebKit::UA_NAVIGATE_BACK))
  {
    CStdString message = "CGUIEmbeddedBrowserWindowObserver::Back: "
      "could not navigate back";
    CLog::Log(LOGERROR, message);
  }
}

void CGUIEmbeddedBrowserWindowObserver::Forward()
{
  if (!LLQtWebKit::getInstance()->userAction(m_browserWindowId,
    LLQtWebKit::UA_NAVIGATE_FORWARD))
  {
    CStdString message = "CGUIEmbeddedBrowserWindowObserver::Forward: "
      "could not navigate forward";
    CLog::Log(LOGERROR, message);
  }
}

void CGUIEmbeddedBrowserWindowObserver::Reload()
{
  if (!LLQtWebKit::getInstance()->userAction(m_browserWindowId,
    LLQtWebKit::UA_NAVIGATE_RELOAD))
  {
    CStdString message = "CGUIEmbeddedBrowserWindowObserver::Forward: "
      "could not reload page";
    CLog::Log(LOGERROR, message);
  }
}

void CGUIEmbeddedBrowserWindowObserver::Stop()
{
  if (!LLQtWebKit::getInstance()->userAction(m_browserWindowId,
    LLQtWebKit::UA_NAVIGATE_STOP))
  {
    CStdString message = "CGUIEmbeddedBrowserWindowObserver::Forward: "
      "could not navigate stop loading page";
    CLog::Log(LOGERROR, message);
  }
}

void CGUIEmbeddedBrowserWindowObserver::Home()
{
  if (!LLQtWebKit::getInstance()->navigateTo(m_browserWindowId, m_homeUrl))
  {
    CStdString message = "CGUIEmbeddedBrowserWindowObserver::Home: "
      "could not navigate to '";
    message.append(m_homeUrl);
    message.append("'");
    CLog::Log(LOGERROR, message);
  }
}

void CGUIEmbeddedBrowserWindowObserver::Go(const CStdString &url)
{
  if (!LLQtWebKit::getInstance()->navigateTo(m_browserWindowId, url))
  {
    CStdString message = "CGUIEmbeddedBrowserWindowObserver::Go: "
      "could not navigate to '";
    message.append(url);
    message.append("'");
    CLog::Log(LOGERROR, message);
  }
}

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

#include "GUIWebBrowserControl.h"
#include "Key.h"

// static bool needs_full_refresh = true;
// 
// class GlDelegate : public Berkelium::WindowDelegate
// {
// public:
//     GlDelegate(int width, int height, unsigned int texture) : m_width(width), m_height(height), m_texture(texture)
//     {}
// 
//     virtual void onPaint(Berkelium::Window *wini, const unsigned char *bitmap_in, const Berkelium::Rect &bitmap_rect,
//         int dx, int dy, const Berkelium::Rect &scroll_rect)
//     {
//         g_graphicsContext.CaptureStateBlock();
//         glBindTexture(GL_TEXTURE_2D, m_texture);
// 
//         // If we've reloaded the page and need a full update, ignore updates
//         // until a full one comes in.  This handles out of date updates due to
//         // delays in event processing.
//         if (needs_full_refresh) {
//             if (bitmap_rect.left() != 0 ||
//                 bitmap_rect.top() != 0 ||
//                 bitmap_rect.right() != m_width ||
//                 bitmap_rect.bottom() != m_height) {
//                 return;
//             }
// 
//             glTexImage2D(GL_TEXTURE_2D, 0, 3, m_width, m_height, 0,
//                 GL_BGRA, GL_UNSIGNED_BYTE, bitmap_in);
//             //glutPostRedisplay();
//             needs_full_refresh = false;
//             return;
//         }
// /*
//         list<string> history;
//         list<string> future;
// 
//         //note: back and forward buttons should be inactive while it's list is empty
// 
//         //onAddressBarChange
//         history.push_front(*url);
// 
//         //on back button
//         future.push_front(*url);
//         //set address bar to history.pop_front()
// 
//         //on forward button
//         history.push_front(*url)
//         //set address bar to future.pop_front()
// */
// 
// /*
//   // if user bookmarks this page
//   if (bookmark_indicator) {
// 
//     // initializations
//     list<string> bookmarks;
//     ifstream infilestream;
//     ofstream outfilestream;
//     string line;
// 
//     // open bookmarks file
//     infilestream.open("Bookmarks.txt");
// 
//     // save existing bookmarks
//     infilestream.getline(line,256);
//     while (!infilestream.eof()) {
//       bookmarks.push_back(line);
//       infilestream.getline(line,256);
//     }
// 
//     // reopen bookmarks file
//     infilestream.close();
//     outfilestream.open("Bookmarks.txt");
// 
//     // add new bookmark
//     bookmarks.push_back(*url);
// 
//     // write bookmarks to file
//     while(!bookmarks.empty()) {
//       outfilestream >> bookmarks.back();
//       bookmarks.pop_back();
//     }
// 
//     // close bookmarks file
//     outfilestream.close();
//   }
// */
// 
// #if 0
//         // Now, we first handle scrolling. We need to do this first since it
//         // requires shifting existing data, some of which will be overwritten by
//         // the regular dirty rect update.
//         if (dx != 0 || dy != 0) {
//             // scroll_rect contains the Rect we need to move
//             // First we figure out where the the data is moved to by translating it
//             Berkelium::Rect scrolled_rect = scroll_rect.translate(-dx, -dy);
//             // Next we figure out where they intersect, giving the scrolled
//             // region
//             Berkelium::Rect scrolled_shared_rect = scroll_rect.intersect(scrolled_rect);
//             // Only do scrolling if they have non-zero intersection
//             if (scrolled_shared_rect.width() > 0 && scrolled_shared_rect.height() > 0) {
//                 // And the scroll is performed by moving shared_rect by (dx,dy)
//                 Berkelium::Rect shared_rect = scrolled_shared_rect.translate(dx, dy);
// 
//                 // Copy the data out of the texture
//                 glGetTexImage(
//                     GL_TEXTURE_2D, 0,
//                     GL_BGRA, GL_UNSIGNED_BYTE,
//                     scroll_buffer
//                 );
// 
//                 // Annoyingly, OpenGL doesn't provide convenient primitives, so
//                 // we manually copy out the region to the beginning of the
//                 // buffer
//                 int wid = scrolled_shared_rect.width();
//                 int hig = scrolled_shared_rect.height();
//                 for(int jj = 0; jj < hig; jj++) {
//                     memcpy(
//                         scroll_buffer + (jj*wid * 4),
//                         scroll_buffer + ((scrolled_shared_rect.top()+jj)*WIDTH + scrolled_shared_rect.left()) * 4,
//                         wid*4
//                     );
//                 }
// 
//                 // And finally, we push it back into the texture in the right
//                 // location
//                 glTexSubImage2D(GL_TEXTURE_2D, 0,
//                     shared_rect.left(), shared_rect.top(),
//                     shared_rect.width(), shared_rect.height(),
//                     GL_BGRA, GL_UNSIGNED_BYTE, scroll_buffer
//                 );
//             }
//         }
// #endif
// 
//         // Finally, we perform the main update, just copying the rect that is
//         // marked as dirty but not from scrolled data.
//         glTexSubImage2D(GL_TEXTURE_2D, 0,
//             bitmap_rect.left(), bitmap_rect.top(),
//             bitmap_rect.width(), bitmap_rect.height(),
//             GL_BGRA, GL_UNSIGNED_BYTE, bitmap_in
//         );
// 
//         g_graphicsContext.ApplyStateBlock();
//     }
// 
//     void onAddressBarChanged(Berkelium::Window *win, const char* newURL, size_t newURLSize)
//     {
//       if (!strcmp(newURL, current_page.data()))
//         return;
// 
//       while (!forward_history.empty())
//         forward_history.pop();
//       back_history.push(current_page);
//       current_page = std::string(newURL);
//     }
// 
//     void navigateBack(Berkelium::Window *win)
//     {
//       forward_history.push(current_page);
//       current_page = back_history.top();
//       back_history.pop();
//       win->navigateTo(current_page.data(), current_page.length());
//     }
// 
//     void navigateForward(Berkelium::Window *win)
//     {
//       back_history.push(current_page);
//       current_page = forward_history.top();
//       forward_history.pop();
//       win->navigateTo(current_page.data(), current_page.length());
//     }
// 
// private:
//     int m_width;
//     int m_height;
//     unsigned int m_texture;
//     std::stack<std::string> back_history;
//     std::stack<std::string> forward_history;
//     std::string current_page;
// };

CGUIWebBrowserControl::CGUIWebBrowserControl(DWORD dwParentID,
      DWORD dwControlId, float posX, float posY, float width, float height)
  : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_texture = 0;
  m_observer = new CGUIEmbeddedBrowserWindowObserver(posX, posY, width, height);
  ControlType = GUICONTROL_WEB_BROWSER;
}

CGUIWebBrowserControl::~CGUIWebBrowserControl()
{}

void CGUIWebBrowserControl::AllocResources()
{
  try
  {
//     g_graphicsContext.CaptureStateBlock();
// 
//     // Create texture to hold rendered view
//     glGenTextures(1, &m_texture);
//     glBindTexture(GL_TEXTURE_2D, m_texture);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 
// //     // Initialize Berkelium and create a window
// //     m_window = Berkelium::Window::create();
// //     m_delegate = new GlDelegate((int)GetWidth(), (int)GetHeight(), m_texture);
// //     m_window->setDelegate(m_delegate);
// //     m_window->resize((int)GetWidth(), (int)GetHeight());
// 
//     unsigned char black = 0;
//         glTexImage2D(GL_TEXTURE_2D, 0, 3, 1, 1, 0,
//                     GL_LUMINANCE, GL_UNSIGNED_BYTE, &black);
// 
// //     std::string url = std::string("http://www.youtube.com");
// //     m_window->navigateTo(url.data(), url.length());
// 
//     g_graphicsContext.ApplyStateBlock();

    m_observer->init();
  }
  catch (...)
  {
    //CLog::Log(LOGERROR, "Exception in CGUIWebBrowserControl::Render()");
  }
}

void CGUIWebBrowserControl::FreeResources()
{
  if (m_texture)
  {
    glDeleteTextures(1, &m_texture);
    m_texture = 0;
  }
  if (m_observer)
  {
    delete m_observer;
    m_observer = NULL;
  }
}

void CGUIWebBrowserControl::Render()
{
//   int realWidth = (int)floor(g_graphicsContext.ScaleFinalXCoord(GetWidth(), GetHeight()));
//   int realHeight = (int)floor(g_graphicsContext.ScaleFinalYCoord(GetWidth(), GetHeight()));
//
//   try
//   {
//     g_graphicsContext.CaptureStateBlock();
//     glEnable( GL_TEXTURE_2D );
//     glBindTexture(GL_TEXTURE_2D, m_texture);
//     glBegin(GL_QUADS);
//     glTexCoord2f(0.f, 0.f); glVertex2f(0.f, 0.f);
//     glTexCoord2f(1.f, 0.f); glVertex2f((float)realWidth, 0.f);
//     glTexCoord2f(1.f, 1.f); glVertex2f((float)realWidth, (float)realHeight);
//     glTexCoord2f(0.f, 1.f); glVertex2f(0.f, (float)realHeight);
//     glEnd();
//     g_graphicsContext.ApplyStateBlock();
//   }
//   catch (...)
//   {
//     //CLog::Log(LOGERROR, "Exception in CGUIWebBrowserControl::Render()");
//   }
//   g_graphicsContext.RestoreViewPort();

  m_observer->Render(GetXPosition(), GetYPosition(), GetWidth(), GetHeight());
  CGUIControl::Render();
}

EVENT_RESULT CGUIWebBrowserControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
//     m_window->mouseButton(0, true);
//     m_window->mouseButton(0, false);
    return EVENT_RESULT_HANDLED;
  }
//   else
//   {
//     m_window->mouseMoved((int)point.x, (int)point.y);
//   }

  return CGUIControl::OnMouseEvent(point, event);
}

void CGUIWebBrowserControl::Back()
{
//   m_delegate->navigateBack(m_window);
}

void CGUIWebBrowserControl::Forward()
{
//   m_delegate->navigateForward(m_window);
}

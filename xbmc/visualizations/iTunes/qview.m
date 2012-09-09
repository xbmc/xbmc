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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#import <QuickTime/QuickTime.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

NSView* g_view   = NULL;
int     g_width  = 0;
int     g_height = 0;

void* get_view(int width, int height)
{
  NSWindow *window = [ [NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
                                        styleMask: NSBorderlessWindowMask
                                        backing: NSBackingStoreBuffered /* NSBackingStoreNonretained */
                                        defer: NO ];
  [ window display ];

  if ( g_view )
  {
    [ g_view release ];
    g_view = nil;
  }
  g_view = [ window contentView ];
  g_width  = width;
  g_height = height;
  return GetWindowPort( [ window windowRef ] );
}

int get_pixels(void* dest, long dest_size, bool opengl)
{
  int bpp = 0;
  if ( !g_view || !dest )
	return bpp;

  [ g_view lockFocus ];
  if ( opengl )
  {
    glReadBuffer( GL_FRONT );
    glReadPixels( 0, 0, g_width, g_height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, dest );
    glReadBuffer( GL_BACK );
    bpp = 4;
  }
  else
  {
    NSBitmapImageRep* bmp = [ [ NSBitmapImageRep alloc ] initWithFocusedViewRect:[ g_view bounds ] ];
    if (bmp)
    {
      // FIXME: much better error checking
      memcpy( dest, (void*)[ bmp bitmapData ], [ bmp bytesPerPlane ] );
      [ bmp release ];
      bpp = 3;
    }
  }
  [ g_view unlockFocus ];
  return bpp;
}

void release_view(void* view)
{
  if ( view && g_view )
  {
    [ g_view release ];
    g_view = nil;
    // FIXME: use 'view'
  }
}

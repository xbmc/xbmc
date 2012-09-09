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

/*********************************************************************/
/* OS X implementation of common platform specific functions         */
/*********************************************************************/

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

#include "common.h"

void     find_normal_plugins     ( NSArray* searchPaths, NSMutableDictionary* pluginMap );
void     find_composition_plugins( NSMutableDictionary* pluginMap );

NSArray* get_search_paths()
{
  return [ [ NSArray alloc ] initWithObjects:
                               @"/Library/iTunes/iTunes Plug-ins",
                               @"~/Library/iTunes/iTunes Plug-ins",
                               @"~/Library/Application Support/XBMC/visualisations/iTunes",
                               nil ];
}

/*********************************************************************/
/* _get_visualisations                                              */
/*********************************************************************/
int _get_visualisations(char*** names, char*** paths)
{
  /* search for iTunes visualisers in the following directories */
  NSArray *searchPaths = get_search_paths();

  NSMutableDictionary *pluginMap = [ [ NSMutableDictionary alloc ] init ];
  NSEnumerator        *pluginEnumerator;

  /* search the searchPaths for iTunes plugins */
  find_normal_plugins     ( searchPaths, pluginMap );
  //find_composition_plugins( pluginMap );

  /* allocate enough memory for the names and paths of the plugins */
  int pluginCount = [ [ pluginMap allKeys ] count ];
  *names = (char**)calloc( pluginCount, sizeof(char *) );
  *paths = (char**)calloc( pluginCount, sizeof(char *) );

  /* populate 'names' and 'paths' */
  int current = 0;
  if ( names && paths )
  {
    NSString *pluginName;

    pluginEnumerator = [ pluginMap keyEnumerator ];
    while ( ( pluginName = [ pluginEnumerator nextObject ] ) )
    {
      const char *cStringName = [ pluginName cStringUsingEncoding: NSASCIIStringEncoding ];
      const char *cStringPath = [ [ pluginMap objectForKey: pluginName ] cStringUsingEncoding: NSASCIIStringEncoding ];

      if ( cStringName && cStringPath )
      {
        char *name = *( *names + current ) = (char*)malloc( strlen(cStringName) * sizeof(char) + 1 );
        char *path = *( *paths + current ) = (char*)malloc( strlen(cStringPath) * sizeof(char) + 1 );

        if ( name && path )
        {
          strcpy( name, cStringName );
          strcpy( path, cStringPath );
          current++;
        }
      }
    }
  }

  /* free the plugin map */
  [ pluginMap removeAllObjects ];
  [ pluginMap release ];
  [ searchPaths release ];

  /* return the number of plugins that were loaded */
  return current;
}

/*********************************************************************/
/* _get_visualisation_path                                           */
/*********************************************************************/
char* _get_visualisation_path( const char* name )
{
  /* search for iTunes visualisers in the following directories */
  NSArray *searchPaths = get_search_paths();
  NSMutableDictionary *pluginMap = [ [ NSMutableDictionary alloc ] init ];
  NSString *pluginName = [ NSString stringWithCString: name
                                    encoding: NSASCIIStringEncoding ];

  /* search the searchPaths for iTunes plugins */
  find_normal_plugins     ( searchPaths, pluginMap );
  find_composition_plugins( pluginMap );

  NSString* pluginPath = [ pluginMap objectForKey: pluginName ];
  if ( pluginPath )
  {
    int strLength = [ pluginPath lengthOfBytesUsingEncoding: NSASCIIStringEncoding ] + 1;
    char *path = (char*)malloc( strLength );
    if ( path )
    {
      strncpy( path, [ pluginPath cStringUsingEncoding: NSASCIIStringEncoding ], strLength );
      return path;
    }
  }
  return NULL;
}

/*********************************************************************/
/* _get_executable_path                                              */
/*********************************************************************/
char* _get_executable_path( const char* plugin_path )
{
  char *exe_path = NULL;
  NSString *pluginPath = [ NSString stringWithCString: plugin_path encoding: NSASCIIStringEncoding ];
  NSBundle *pluginBundle = [ NSBundle bundleWithPath: pluginPath ];

  if ( pluginBundle )
  {
    NSString *ns_exe_path = [ pluginBundle executablePath ];
    if ( ns_exe_path )
    {
      int strLength = [ ns_exe_path lengthOfBytesUsingEncoding: NSASCIIStringEncoding ] + 1;
      exe_path = (char*)malloc( strLength );
      if ( exe_path )
      {
        strncpy( exe_path, [ ns_exe_path cStringUsingEncoding: NSASCIIStringEncoding ], strLength );
      }
    }
  }
  return exe_path;
}

/*********************************************************************/
/* _copy_to_pascal_string                                            */
/*********************************************************************/
void  _copy_to_pascal_string( unsigned char dest[], const char* src, int dest_length )
{
  CFStringGetPascalString( (CFStringRef)[NSString stringWithCString: src encoding: NSUTF8StringEncoding ],
                           (StringPtr)dest, dest_length, kCFStringEncodingASCII );
}

/*********************************************************************/
/* _copy_to_unicode_string                                           */
/*********************************************************************/
void  _copy_to_unicode_string( unsigned short dest[], const char* src, int dest_length )
{
  NSString *srcString = [NSString stringWithCString: src encoding: NSUTF8StringEncoding ];
  [ srcString getCString: (char*)dest maxLength: dest_length encoding: NSUnicodeStringEncoding ];
}

/*********************************************************************/
/* _get_album_art_from_file                                          */
/*********************************************************************/
void  _get_album_art_from_file( const char *filename, Handle* handle, OSType* format )
{
  NSString *album_art_file = [ NSString stringWithUTF8String: filename ];
  NSData *album_art_data   = [ [ NSData alloc ] initWithContentsOfFile: album_art_file ];
  if ( album_art_data != nil )
  {
    PtrToHand( [ album_art_data bytes ], handle, [ album_art_data length ] );
    NSString *type = NSHFSTypeOfFile( album_art_file );
    NSLog( @"Type: %@ (length=%d)", type, [type length] );

    /* since XBMC thumbs usually have .tbn extension, we'll likely not
       get a match. For now, default to OSType 'JPEG' until we have a
       proper image type detection mechanism. */
    if ( [ type isEqualToString:@"''" ] )
      *format = 'JPEG';
    else
      *format = NSHFSTypeCodeFromFileType(type);
  }
}

/*********************************************************************/
/* find_composition_plugins                                          */
/*********************************************************************/
void find_composition_plugins( NSMutableDictionary* pluginMap )
#if (MAC_OS_X_VERSION_MAX_ALLOWED <= 1040)
{
  /* QCComposition and QCCompositionRepository are 10.5 only */
  return;
}
#else
{
  QCCompositionRepository *repo = [ QCCompositionRepository sharedCompositionRepository ];
  if ( repo )
  {
    NSArray *visualisers = [ repo compositionsWithProtocols:
                    [ NSArray arrayWithObjects:
                        @"com.apple.QuartzComposer.protocol.visualizer-music",
                            nil ]
                  andAttributes: nil ];
    if ( visualisers )
    {
      QCComposition *vis;
      NSString      *pluginName;

      for ( vis in visualisers )
      {
        /* get plugin name */
        pluginName = [ [ vis attributes ] objectForKey: QCCompositionAttributeNameKey ];

        /* add plugin to plugin map using an empty string as path */
        [ pluginMap setObject: @"" forKey: [ pluginName retain ] ];
      }
    }
  }
}
#endif

/*********************************************************************/
/* find_normal_plugins                                               */
/*********************************************************************/
void find_normal_plugins( NSArray* searchPaths, NSMutableDictionary* pluginMap )
{
  NSString *path;
  NSEnumerator *pluginEnumerator;
  NSEnumerator *pathEnumerator = [ searchPaths objectEnumerator ];

  while ( ( path = [ pathEnumerator nextObject ] ) )
  {
    path = [ path stringByExpandingTildeInPath ];
    NSString  *pluginPath;
    NSArray   *plugins = [ NSBundle pathsForResourcesOfType:@"bundle" inDirectory:path ];

    pluginEnumerator = [ plugins objectEnumerator ];
    while( ( pluginPath = [ pluginEnumerator nextObject ] ) )
    {
      NSBundle *pluginBundle = nil;
      NSString *pluginName   = nil;

      pluginName   = [ pluginPath lastPathComponent ];
      pluginBundle = [ NSBundle bundleWithPath: pluginPath ];

      if ( pluginBundle && pluginName
           && ( pluginName = [ pluginName stringByDeletingPathExtension ] )
           && ( [ pluginName compare: @"Quartz Composer Visualizer" ] != 0 ) )
      {
        /* save the path and name to the plugin map */
        [ pluginMap setValue: pluginPath forKey: pluginName ];
      }
    }
  }
}

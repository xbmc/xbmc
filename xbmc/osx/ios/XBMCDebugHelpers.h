/*
 *  XBMCDebugHelpers.h
 *  xbmclauncher
 *
 *      Created by Stephan Diederich on 21.09.08.
 *      Copyright 2008 University Heidelberg. All rights reserved.
 *
 *      Copyright (C) 2008-2013 Team XBMC
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
 *  Stuff below was taken from LoggingUtils.h of ATVFiles
 *
 *  Just some utility macros for logging...
 *
 *  Created by Eric Steil III on 4/1/07.
 *  Copyright (C) 2007-2008 Eric Steil III
 *
 */
#define DEBUG
#ifdef DEBUG
#define LOG(s, ...)  NSLog(@"[DEBUG] " s, ##__VA_ARGS__)
#define ILOG(s, ...) NSLog(@"[INFO]  " s, ##__VA_ARGS__)
#define ELOG(s, ...) NSLog(@"[ERROR] " s, ##__VA_ARGS__)
#define DLOG(s, ...) LOG(s, ##__VA_ARGS__)
#else
#define LOG(s, ...) 
#define ILOG(s, ...) NSLog(@"[INFO]  " s, ##__VA_ARGS__)
#define ELOG(s, ...) NSLog(@"[ERROR] " s, ##__VA_ARGS__)
#define DLOG(s, ...) LOG(s, ##__VA_ARGS__)
#endif

#define PRINT_SIGNATURE() LOG(@"%s", __PRETTY_FUNCTION__)

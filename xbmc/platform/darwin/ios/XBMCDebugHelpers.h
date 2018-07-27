/*
 *  XBMCDebugHelpers.h
 *  xbmclauncher
 *
 *      Created by Stephan Diederich on 21.09.08.
 *  Copyright 2008 University Heidelberg. All rights reserved.
 *
 *  Copyright (C) 2008-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

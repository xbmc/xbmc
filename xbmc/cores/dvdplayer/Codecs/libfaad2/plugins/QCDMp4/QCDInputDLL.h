//-----------------------------------------------------------------------------
// 
// File:	QCDInputDLL.h
//
// About:	QCD Player Input module DLL interface.  For more documentation, see
//			QCDModInput.h.
//
// Authors:	Written by Paul Quinn and Richard Carlson.
//
//	QCD multimedia player application Software Development Kit Release 1.0.
//
//	Copyright (C) 1997-2002 Quinnware
//
//	This code is free.  If you redistribute it in any form, leave this notice 
//	here.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//-----------------------------------------------------------------------------

#ifndef QCDInputDLL_H
#define QCDInputDLL_H

#include "QCDModInput.h"

// Calls from the Player
int  GetMediaSupported(const char* medianame, MediaInfo *mediaInfo);
int  GetTrackExtents(const char* medianame, TrackExtents *ext, int flags);
int  GetCurrentPosition(const char* medianame, long *track, long *offset);

void SetEQ(EQInfo*);
void SetVolume(int levelleft, int levelright, int flags);

int  Play(const char* medianame, int framefrom, int frameto, int flags);
int  Pause(const char* medianame, int flags);
int  Stop(const char* medianame, int flags);
int  Eject(const char* medianame, int flags);

int  Initialize(QCDModInfo *ModInfo, int flags);
void ShutDown(int flags);
void Configure(int flags);
void About(int flags);

#endif //QCDInputDLL_H
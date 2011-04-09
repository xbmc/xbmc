/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Medium
// C++ header

#ifndef _MEDIA_HH
#define _MEDIA_HH

#ifndef _LIVEMEDIA_VERSION_HH
#include "liveMedia_version.hh"
#endif

#ifndef _BOOLEAN_HH
#include "Boolean.hh"
#endif

#ifndef _USAGE_ENVIRONMENT_HH
#include "UsageEnvironment.hh"
#endif

// Lots of files end up needing the following, so just #include them here:
#ifndef _NET_COMMON_H
#include "NetCommon.h"
#endif
#include <stdio.h>

// The following makes the Borland compiler happy:
#ifdef __BORLANDC__
#define _strnicmp strnicmp
#define fabsf(x) fabs(x)
#endif

#define mediumNameMaxLen 30

class Medium {
public:
  static Boolean lookupByName(UsageEnvironment& env,
			      char const* mediumName,
			      Medium*& resultMedium);
  static void close(UsageEnvironment& env, char const* mediumName);
  static void close(Medium* medium); // alternative close() method using ptrs
      // (has no effect if medium == NULL)

  UsageEnvironment& envir() const {return fEnviron;}

  char const* name() const {return fMediumName;}

  // Test for specific types of media:
  virtual Boolean isSource() const;
  virtual Boolean isSink() const;
  virtual Boolean isRTCPInstance() const;
  virtual Boolean isRTSPClient() const;
  virtual Boolean isRTSPServer() const;
  virtual Boolean isMediaSession() const;
  virtual Boolean isServerMediaSession() const;
  virtual Boolean isDarwinInjector() const;

protected:
  Medium(UsageEnvironment& env); // abstract base class
  virtual ~Medium(); // instances are deleted using close() only

  TaskToken& nextTask() {
	return fNextTask;
  }

private:
  friend class MediaLookupTable;
  UsageEnvironment& fEnviron;
  char fMediumName[mediumNameMaxLen];
  TaskToken fNextTask;
};

// The structure pointed to by the "liveMediaPriv" UsageEnvironment field:
class _Tables {
public:
  static _Tables* getOurTables(UsageEnvironment& env);
      // returns a pointer to an "ourTables" structure (creating it if necessary)
  void reclaimIfPossible();
      // used to delete ourselves when we're no longer used

  void* mediaTable;
  void* socketTable;

protected:
  _Tables(UsageEnvironment& env);
  virtual ~_Tables();

private:
  UsageEnvironment& fEnv;
};

#endif

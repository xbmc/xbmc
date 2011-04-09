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
// MP3 File Sources
// C++ header

#ifndef _MP3_FILE_SOURCE_HH
#define _MP3_FILE_SOURCE_HH

#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif

class MP3StreamState; // forward

class MP3FileSource: public FramedFileSource {
public:
  static MP3FileSource* createNew(UsageEnvironment& env, char const* fileName);

  float filePlayTime() const;
  void setPresentationTimeScale(unsigned scale);
  void seekWithinFile(double seekNPT);

protected:
  MP3FileSource(UsageEnvironment& env, FILE* fid);
	// called only by createNew()

  virtual ~MP3FileSource();

protected:
  void assignStream(FILE* fid, unsigned filesize);
  Boolean initializeStream();

  MP3StreamState* streamState() {return fStreamState;}

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual char const* MIMEtype() const;
  virtual void getAttributes() const;

private:
  virtual Boolean doGetNextFrame1();

private:
  MP3StreamState* fStreamState;
  Boolean fHaveJustInitialized;
  struct timeval fFirstFramePresentationTime; // set on stream init
};

#endif

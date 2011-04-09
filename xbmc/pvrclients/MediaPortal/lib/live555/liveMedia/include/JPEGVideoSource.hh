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
// JPEG video sources
// C++ header

#ifndef _JPEG_VIDEO_SOURCE_HH
#define _JPEG_VIDEO_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class JPEGVideoSource: public FramedSource {
public:
  virtual u_int8_t type() = 0;
  virtual u_int8_t qFactor() = 0;
  virtual u_int8_t width() = 0; // # pixels/8 (or 0 for 2048 pixels)
  virtual u_int8_t height() = 0; // # pixels/8 (or 0 for 2048 pixels)

  virtual u_int8_t const* quantizationTables(u_int8_t& precision,
					     u_int16_t& length);
    // If "qFactor()" returns a value >= 128, then this function is called
    // to tell us the quantization tables that are being used.
    // (The default implementation of this function just returns NULL.)
    // "precision" and "length" are as defined in RFC 2435, section 3.1.8.

protected:
  JPEGVideoSource(UsageEnvironment& env); // abstract base class
  virtual ~JPEGVideoSource();

private:
  // redefined virtual functions:
  virtual Boolean isJPEGVideoSource() const;
};

#endif

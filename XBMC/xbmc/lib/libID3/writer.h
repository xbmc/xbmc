// -*- C++ -*-
// $Id$

// id3lib: a software library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug

// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
// License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// The id3lib authors encourage improvements and optimisations to be sent to
// the id3lib coordinator.  Please see the README file for details on where to
// send such submissions.  See the AUTHORS file for a list of people who have
// contributed to id3lib.  See the ChangeLog file for a list of changes to
// id3lib.  These files are distributed with id3lib at
// http://download.sourceforge.net/id3lib/

#ifndef _ID3LIB_WRITER_H_
#define _ID3LIB_WRITER_H_

#include "globals.h" //has <stdlib.h> "sized_types.h"

class ID3_CPP_EXPORT ID3_Writer
{
 public:
  typedef uint32 size_type;
  typedef uint8  char_type;
  typedef uint32 pos_type;
  typedef  int32 off_type;
  typedef  int16 int_type;
  static const int_type END_OF_WRITER;
  
  /** Close the writer.  Any further actions on the writer should fail. **/
  virtual void close() = 0;

  /** Flush the writer. **/
  virtual void flush() = 0;

  /** Return the beginning position in the writer **/
  virtual pos_type getBeg() { return static_cast<pos_type>(0); }

  /** Return the first position that can't be written to.  A return value of
   ** -1 indicates no (reasonable) limit to the writer. 
   **/
  virtual pos_type getEnd() { return static_cast<pos_type>(-1); }

  /** Return the next position that will be written to */
  virtual pos_type getCur() = 0;

  /** Return the number of bytes written **/
  virtual size_type getSize() { return this->getCur() - this->getBeg(); }

  /** Return the maximum number of bytes that can be written **/
  virtual size_type getMaxSize() { return this->getEnd() - this->getBeg(); }

  /** Write a single character and advance the internal position.  Note that
   ** the interal position may advance more than one byte for a single
   ** character write.  Returns END_OF_WRITER if there isn't a character to
   ** write.
   **/
  virtual int_type writeChar(char_type ch) 
  {
    if (this->atEnd())
    { 
      return END_OF_WRITER; 
    }
    this->writeChars(&ch, 1);
    return ch;
  }

  /** Write up to \c len characters into buf and advance the internal position
   ** accordingly.  Returns the number of characters write into buf.  Note that
   ** the value returned may be less than the number of bytes that the internal
   ** position advances, due to multi-byte characters.
   **/
  virtual size_type writeChars(const char_type buf[], size_type len) = 0;
  virtual size_type writeChars(const char buf[], size_type len)
  {
    return this->writeChars(reinterpret_cast<const char_type *>(buf), len);
  }

  virtual bool atEnd()
  {
    return this->getCur() >= this->getEnd();
  }
};

#endif /* _ID3LIB_WRITER_H_ */


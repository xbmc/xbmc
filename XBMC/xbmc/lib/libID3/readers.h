// -*- C++ -*-
// $Id$

// id3lib: a software library for creating and manipulating id3v1/v2 tags
// Copyright 1999, 2000  Scott Thomas Haug
// Copyright 2002 Thijmen Klok (thijmen@id3lib.org)

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

#ifndef _ID3LIB_READERS_H_
#define _ID3LIB_READERS_H_

#if defined(__BORLANDC__)
// due to a bug in borland it sometimes still wants mfc compatibility even when you disable it
#  if defined(_MSC_VER)
#    undef _MSC_VER
#  endif
#  if defined(__MFC_COMPAT__)
#    undef __MFC_COMPAT__
#  endif
#endif

#include "id3lib_streams.h"
#include "reader.h"

class ID3_CPP_EXPORT ID3_IStreamReader : public ID3_Reader
{
  istream& _stream;
 protected:
  istream& getReader() const { return _stream; }
 public:
  ID3_IStreamReader(istream& reader) : _stream(reader) { ; }
  virtual ~ID3_IStreamReader() { ; }
  virtual void close() { ; }

  virtual int_type peekChar() { return _stream.peek(); }

  /** Read up to \c len chars into buf and advance the internal position
   ** accordingly.  Returns the number of characters read into buf.
   **/
  virtual size_type readChars(char buf[], size_type len)
  {
    return this->readChars(reinterpret_cast<uchar *>(buf), len);
  }
  virtual size_type readChars(char_type buf[], size_type len)
  {
    _stream.read((char *)buf, len);
    return _stream.gcount();
  }

  virtual pos_type getBeg() { return 0; }
  virtual pos_type getCur() { return _stream.tellg(); }
  virtual pos_type getEnd()
  {
    pos_type cur = this->getCur();
    _stream.seekg(0, ios::end);
    pos_type end = this->getCur();
    this->setCur(cur);
    return end;
  }

  /** Set the value of the internal position for reading.
   **/
  virtual pos_type setCur(pos_type pos) { _stream.seekg(pos); return pos; }
};

class ID3_CPP_EXPORT ID3_IFStreamReader : public ID3_IStreamReader
{
  ifstream& _file;
 public:
  ID3_IFStreamReader(ifstream& reader)
    : ID3_IStreamReader(reader), _file(reader) { ; }

  virtual void close()
  {
    _file.close();
  }
};

class ID3_CPP_EXPORT ID3_MemoryReader : public ID3_Reader
{
  const char_type* _beg;
  const char_type* _cur;
  const char_type* _end;
 protected:
  void setBuffer(const char_type* buf, size_type size)
  {
    _beg = buf;
    _cur = buf;
    _end = buf + size;
  };
 public:
  ID3_MemoryReader()
  {
    this->setBuffer(NULL, 0);
  }
  ID3_MemoryReader(const char_type* buf, size_type size)
  {
    this->setBuffer(buf, size);
  };
  ID3_MemoryReader(const char* buf, size_type size)
  {
    this->setBuffer(reinterpret_cast<const char_type*>(buf), size);
  };
  virtual ~ID3_MemoryReader() { ; }
  virtual void close() { ; }

  virtual int_type peekChar()
  {
    if (!this->atEnd())
    {
      return *_cur;
    }
    return END_OF_READER;
  }

  /** Read up to \c len chars into buf and advance the internal position
   ** accordingly.  Returns the number of characters read into buf.
   **/
  virtual size_type readChars(char buf[], size_type len)
  {
    return this->readChars(reinterpret_cast<char_type *>(buf), len);
  }
  virtual size_type readChars(char_type buf[], size_type len);

  virtual pos_type getCur()
  {
    return _cur - _beg;
  }

  virtual pos_type getBeg()
  {
    return _beg - _beg;
  }

  virtual pos_type getEnd()
  {
    return _end - _beg;
  }

  /** Set the value of the internal position for reading.
   **/
  virtual pos_type setCur(pos_type pos)
  {
    pos_type end = this->getEnd();
    size_type size = (pos < end) ? pos : end;
    _cur = _beg + size;
    return this->getCur();
  }
};

#endif /* _ID3LIB_READERS_H_ */


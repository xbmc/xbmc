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

#ifndef _ID3LIB_WRITERS_H_
#define _ID3LIB_WRITERS_H_

#if defined(__BORLANDC__)
// due to a bug in borland it sometimes still wants mfc compatibility even when you disable it
#  if defined(_MSC_VER)
#    undef _MSC_VER
#  endif
#  if defined(__MFC_COMPAT__)
#    undef __MFC_COMPAT__
#  endif
#endif

#include "id3/writer.h"
#include "id3/id3lib_streams.h"
//#include <string.h>

class ID3_CPP_EXPORT ID3_OStreamWriter : public ID3_Writer
{
  ostream& _stream;
  pos_type _beg;
 protected:
  ostream& getWriter() const { return _stream; }
 public:
  ID3_OStreamWriter(ostream& writer) : _stream(writer), _beg(_stream.tellp()) { ; }
  virtual ~ID3_OStreamWriter() { ; }

  virtual void close() { ; }
  virtual void flush() { _stream.flush(); }

  virtual int_type writeChar(char_type ch)
  {
    _stream.put(ch);
    return ch;
  }

  /** Write up to \c len chars into buf and advance the internal position
   ** accordingly.  Returns the number of characters write into buf.
   **/
  virtual size_type writeChars(const char buf[], size_type len)
  {
    _stream.write(buf, len);
    return len;
  }
  virtual size_type writeChars(const char_type buf[], size_type len)
  {
    _stream.write(reinterpret_cast<const char*>(buf), len);
    return len;
  }

  virtual pos_type getBeg() { return _beg; }
  virtual pos_type getCur() { return _stream.tellp(); }
};

class ID3_CPP_EXPORT ID3_OFStreamWriter : public ID3_OStreamWriter
{
  ofstream& _file;
 public:
  ID3_OFStreamWriter(ofstream& writer)
    : ID3_OStreamWriter(writer), _file(writer) { ; }

  virtual void close()
  {
    _file.close();
  }
};

class ID3_CPP_EXPORT ID3_IOStreamWriter : public ID3_Writer
{
  iostream& _stream;
  pos_type  _beg;
 protected:
  iostream& getWriter() const { return _stream; }
 public:
  ID3_IOStreamWriter(iostream& writer) : _stream(writer), _beg(_stream.tellp()) { ; }
  virtual ~ID3_IOStreamWriter() { ; }

  virtual void close() { ; }
  virtual void flush() { _stream.flush(); }

  virtual int_type writeChar(char_type ch)
  {
    _stream.put(ch);
    return ch;
  }

  /** Write up to \c len chars into buf and advance the internal position
   ** accordingly.  Returns the number of characters write into buf.
   **/
  virtual size_type writeChars(const char buf[], size_type len)
  {
    _stream.write(buf, len);
    return len;
  }
  virtual size_type writeChars(const char_type buf[], size_type len)
  {
    _stream.write(reinterpret_cast<const char*>(buf), len);
    return len;
  }

  virtual pos_type getBeg() { return _beg; }
  virtual pos_type getCur() { return _stream.tellp(); }
};

class ID3_CPP_EXPORT ID3_FStreamWriter : public ID3_IOStreamWriter
{
  fstream& _file;
 public:
  ID3_FStreamWriter(fstream& writer)
    : ID3_IOStreamWriter(writer), _file(writer) { ; }

  virtual void close()
  {
    _file.close();
  }
};

class ID3_CPP_EXPORT ID3_MemoryWriter : public ID3_Writer
{
  const char_type* _beg;
  /* */ char_type* _cur;
  const char_type* _end;
 protected:
  void setBuffer(char_type* buf, size_t size)
  {
    _beg = buf;
    _cur = buf;
    _end = buf + size;
  };
 public:
  ID3_MemoryWriter()
  {
    this->setBuffer(NULL, 0);
  }
  ID3_MemoryWriter(char_type buf[], size_t size)
  {
    this->setBuffer(buf, size);
  }
  virtual ~ID3_MemoryWriter() { ; }
  virtual void close() { ; }
  virtual void flush() { ; }

  /** Write up to \c len chars from buf and advance the internal position
   ** accordingly.  Returns the number of characters written from buf.
   **/
  virtual size_type writeChars(const char buf[], size_type len)
  {
    return this->writeChars(reinterpret_cast<const char_type *>(buf), len);
  }
  virtual size_type writeChars(const char_type buf[], size_type len)
  {
    size_type remaining = _end - _cur;
    size_type size = (remaining > len) ? len : remaining;
    ::memcpy(_cur, buf, size);
    _cur += size;
    return size;
  }

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
};

#endif /* _ID3LIB_WRITERS_H_ */


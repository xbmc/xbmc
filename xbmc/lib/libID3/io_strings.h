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

#ifndef _ID3LIB_IO_STRINGS_H_
#define _ID3LIB_IO_STRINGS_H_

#include "id3lib_strings.h"
#include "reader.h"
#include "writer.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

namespace dami
{
  namespace io
  {
    class ID3_CPP_EXPORT StringReader : public ID3_Reader
    {
      const String&  _string;
      pos_type _cur;
     public:
      StringReader(const String& string) : _string(string), _cur(0) { ; }
      virtual ~StringReader() { ; }

      virtual void close() { ; }
      virtual int_type peekChar() 
      { 
        if (!this->atEnd())
        {
          return _string[_cur];
        }
        return END_OF_READER;
      }
    
      /** Read up to \c len chars into buf and advance the internal position
       ** accordingly.  Returns the number of characters read into buf.
       **/
      size_type readChars(char buf[], size_type len)
      { 
        return this->readChars((char_type*) buf, len); 
      }
      virtual size_type readChars(char_type buf[], size_type len)
      {
        size_type size = min((unsigned int)len, (unsigned int)(_string.size() - _cur));
        _string.copy(reinterpret_cast<String::value_type *>(buf), size, _cur);
        _cur += size;
        return size;
      }
      
      virtual pos_type getCur() 
      { 
        return _cur;
      }
      
      virtual pos_type getBeg()
      {
        return 0;
      }
      
      virtual pos_type getEnd()
      {
        return _string.size();
      }
      
      /** Set the value of the internal position for reading.
       **/
      virtual pos_type setCur(pos_type pos)
      {
        pos_type end = this->getEnd();
        _cur = (pos < end) ? pos : end;
        return _cur;
      }

      virtual bool atEnd()
      {
        return _cur >= _string.size();
      }

      virtual size_type skipChars(size_type len)
      {
        size_type size = min((unsigned int)len, (unsigned int)(_string.size() - _cur));
        _cur += size;
        return size;
      }
    };

    class ID3_CPP_EXPORT BStringReader : public ID3_Reader
    {
      const BString&  _string;
      pos_type _cur;
     public:
      BStringReader(const BString& string) : _string(string), _cur(0) { ; }
      virtual ~BStringReader() { ; }

      virtual void close() { ; }
      virtual int_type peekChar() 
      { 
        if (!this->atEnd())
        {
          return _string[_cur];
        }
        return END_OF_READER;
      }
    
      /** Read up to \c len chars into buf and advance the internal position
       ** accordingly.  Returns the number of characters read into buf.
       **/
      size_type readChars(char buf[], size_type len)
      { 
        return this->readChars((char_type*) buf, len); 
      }
      virtual size_type readChars(char_type buf[], size_type len)
      {
        size_type size = min((unsigned int)len, (unsigned int)(_string.size() - _cur));
        _string.copy(reinterpret_cast<BString::value_type *>(buf), size, _cur);
        _cur += size;
        return size;
      }
      
      virtual pos_type getCur() 
      { 
        return _cur;
      }
      
      virtual pos_type getBeg()
      {
        return 0;
      }
      
      virtual pos_type getEnd()
      {
        return _string.size();
      }
      
      /** Set the value of the internal position for reading.
       **/
      virtual pos_type setCur(pos_type pos)
      {
        pos_type end = this->getEnd();
        _cur = (pos < end) ? pos : end;
        return _cur;
      }

      virtual bool atEnd()
      {
        return _cur >= _string.size();
      }

      virtual size_type skipChars(size_type len)
      {
        size_type size = min((unsigned int)len,(unsigned int)( _string.size() - _cur));
        _cur += size;
        return size;
      }
    };

    class ID3_CPP_EXPORT StringWriter : public ID3_Writer
    {
      String& _string;
     public:
      StringWriter(String& string) : _string(string) { ; }
      virtual ~StringWriter() { ; }

      void close() { ; }
      void flush() { ; }
      virtual size_type writeChars(const char buf[], size_type len)
      { 
        _string.append(reinterpret_cast<const String::value_type *>(buf), len);
        return len;
      }
      size_type writeChars(const char_type buf[], size_type len)
      {
        _string.append(reinterpret_cast<const String::value_type *>(buf), len);
        return len;
      }

      pos_type getCur()
      {
        return _string.size();
      }
    };

    class ID3_CPP_EXPORT BStringWriter : public ID3_Writer
    {
      BString& _string;
     public:
      BStringWriter(BString& string) : _string(string) { ; }
      virtual ~BStringWriter() { ; }

      void close() { ; }
      void flush() { ; }
      virtual size_type writeChars(const char buf[], size_type len)
      { 
        _string.append(reinterpret_cast<const BString::value_type *>(buf), len);
        return len;
      }
      size_type writeChars(const char_type buf[], size_type len)
      {
        _string.append(reinterpret_cast<const BString::value_type *>(buf), len);
        return len;
      }

      pos_type getCur()
      {
        return _string.size();
      }
    };
  };
};

#endif /* _ID3LIB_IO_STRINGS_H_ */


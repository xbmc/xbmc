/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project:Dataset C++ Dynamic Library
 * Module: FieldValue class and result sets classes header file
 * Author: Leo Seib      E-Mail: leoseib@web.de
 * Begin: 5/04/2002
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **********************************************************************/


#include <map>
#include <vector>
#include <iostream>
#include <string>

#ifndef _QRYDAT_H
#define _QRYDAT_H


using namespace std;

namespace dbiplus {

enum fType { 
	ft_String,
	ft_Boolean,
	ft_Char,
	ft_WChar,
	ft_WideString,
	ft_Short,
	ft_UShort,
	ft_Long,
	ft_ULong,
	ft_Float,
	ft_Double,
	ft_LongDouble,
	ft_Object
    };



class field_value {
private:
  fType field_type;
  string str_value;
  union {
    bool   bool_value;
    char   char_value;
    short  short_value;
    unsigned short ushort_value;
    long   long_value;
    unsigned long  ulong_value;
    float  float_value;
    double double_value;
    void   *object_value;
  } ;

  bool is_null;

public:
  field_value();
  field_value(const char *s);
  field_value(const bool b);
  field_value(const char c);
  field_value(const short s);
  field_value(const unsigned short us);
  field_value(const long l);
  field_value(const unsigned long ul);
  field_value(const int i);
  field_value(const float f);
  field_value(const double d);
  field_value(const field_value & fv);
  ~field_value();
  

  fType get_fType() const {return field_type;}
  bool get_isNull() const {return is_null;}
  string get_asString() const;
  bool get_asBool() const;
  char get_asChar() const;
  short get_asShort() const;
  unsigned short get_asUShort() const;
  long get_asLong() const;
  int get_asInteger() const;
  unsigned long get_asULong() const;
  float get_asFloat() const;
  double get_asDouble() const;

  field_value& operator= (const char *s)
    {set_asString(s); return *this;}
  field_value& operator= (const string &s)
    {set_asString(s); return *this;}
  field_value& operator= (const bool b)
    {set_asBool(b); return *this;}
  field_value& operator= (const short s)
    {set_asShort(s); return *this;}
  field_value& operator= (const unsigned short us)
    {set_asUShort(us); return *this;}
  field_value& operator= (const long l)
    {set_asLong(l); return *this;}
  field_value& operator= (const unsigned long l)
    {set_asULong(l); return *this;}
  field_value& operator= (const int i)
    {set_asLong(i); return *this;}
  field_value& operator= (const float f)
    {set_asFloat(f); return *this;}
  field_value& operator= (const double d)
    {set_asDouble(d); return *this;}
  field_value& operator= (const field_value & fv);
  
  //class ostream;
  friend ostream& operator<< (ostream& os, const field_value &fv)
  {switch (fv.get_fType()) {
    case ft_String: {
      return os << fv.get_asString();
      break;
    }
    case ft_Boolean:{
      return os << fv.get_asBool();
      break;     
    }
    case ft_Char: {
      return os << fv.get_asChar();
      break;
    }
    case ft_Short: {
      return os << fv.get_asShort();
      break;
    }
    case ft_UShort: {
      return os << fv.get_asUShort();
      break;
    }
    case ft_Long: {
      return os << fv.get_asLong();
      break;
    }
    case ft_ULong: {
      return os << fv.get_asULong();
      break;
    }
    case ft_Float: {
      return os << fv.get_asFloat();
      break;
    }
    case ft_Double: {
      return os << fv.get_asDouble();
      break;
    }
  }
  }

  void set_isNull(){is_null=true;}
  void set_asString(const char *s);
  void set_asString(const string & s);
  void set_asBool(const bool b);
  void set_asChar(const char c);
  void set_asShort(const short s);
  void set_asUShort(const unsigned short us);
  void set_asInteger(const int i);
  void set_asLong(const long l);
  void set_asULong(const unsigned long l);
  void set_asFloat(const float f);
  void set_asDouble(const double d);

  fType get_field_type();
  string gft();
};

struct field_prop {
  string name,display_name;
  fType type;
  string field_table; //?
  bool read_only;
  unsigned int field_len;
  unsigned int field_flags;
  int idx;
};

struct field {
  field_prop props;
  field_value val;
}; 


typedef map<int,field> Fields;
typedef map<int,field_value> sql_record;
typedef map<int,field_prop> record_prop;
typedef map<int,sql_record> query_data;
typedef field_value variant;

//typedef Fields::iterator fld_itor;
typedef sql_record::iterator rec_itor;
typedef record_prop::iterator recprop_itor;
typedef query_data::iterator qry_itor;

struct result_set {
  record_prop record_header;
  query_data records;
};

} // namespace

#endif

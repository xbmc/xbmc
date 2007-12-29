/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project: C++ Dynamic Library
 * Module: FieldValue class realisation file
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
/**********************************************************************
 * 2005-03-29 - Minor modifications to allow get_asBool to function on
 *              on string values that are 1 or 0
 **********************************************************************/

#include "qry_dat.h"

#pragma warning (disable:4800)
#pragma warning (disable:4715)

namespace dbiplus {

//Constructors 
field_value::field_value(){
  str_value = "";
  field_type = ft_String;
  is_null = false;
  };

field_value::field_value(const char *s) {
  str_value = s;
  field_type = ft_String;
  is_null = false;
}
  
field_value::field_value(const bool b) {
  bool_value = b; 
  field_type = ft_Boolean;
  is_null = false;
}

field_value::field_value(const char c) {
  char_value = c; 
  field_type = ft_Char;
  is_null = false;
}
  
field_value::field_value(const short s) {
  short_value = s; 
  field_type = ft_Short;
  is_null = false;
}
  
field_value::field_value(const unsigned short us) {
  ushort_value = us; 
  field_type = ft_UShort;
  is_null = false;
}
  
field_value::field_value(const long l) {
  long_value = l; 
  field_type = ft_Long;
  is_null = false;
}

field_value::field_value(const int i) {
  long_value = (long)i; 
  field_type = ft_Long;
  is_null = false;
}
  
field_value::field_value(const unsigned long ul) {
  ulong_value = ul; 
  field_type = ft_ULong;
  is_null = false;
}
  
field_value::field_value(const float f) {
  float_value = f; 
  field_type = ft_Float;
  is_null = false;
}
  
field_value::field_value(const double d) {
  double_value = d; 
  field_type = ft_Double;
  is_null = false;
}
  
field_value::field_value (const field_value & fv) {
  switch (fv.get_fType()) {
    case ft_String: {
      set_asString(fv.get_asString());
      break;
    }
    case ft_Boolean:{
      set_asBool(fv.get_asBool());
      break;     
    }
    case ft_Char: {
      set_asChar(fv.get_asChar());
      break;
    }
    case ft_Short: {
      set_asShort(fv.get_asShort());
      break;
    }
    case ft_UShort: {
      set_asUShort(fv.get_asUShort());
      break;
    }
    case ft_Long: {
      set_asLong(fv.get_asLong());
      break;
    }
    case ft_ULong: {
      set_asULong(fv.get_asULong());
      break;
    }
    case ft_Float: {
      set_asFloat(fv.get_asFloat());
      break;
    }
    case ft_Double: {
      set_asDouble(fv.get_asDouble());
      break;
    }
  }
  is_null = false;
};


//empty destructor
field_value::~field_value(){

  }

  
//Conversations functions
string field_value::get_asString() const {
    string tmp;
    switch (field_type) {
    case ft_String: {
      tmp = str_value;
      return tmp;
    }
    case ft_Boolean:{
      if (bool_value) 
	return tmp = "True";
      else
	return tmp = "False";
    }
    case ft_Char: {
      return tmp = char_value;
    }
    case ft_Short: {
      char t[10];
      sprintf(t,"%i",short_value);
      return tmp = t;
    }
    case ft_UShort: {
      char t[10];
      sprintf(t,"%i",ushort_value);
      return tmp = t;
    }
    case ft_Long: {
      char t[12];
      sprintf(t,"%i",long_value);
      return tmp = t;
    }
    case ft_ULong: {
      char t[12];
      sprintf(t,"%i",ulong_value);
      return tmp = t;
    }
    case ft_Float: {
      char t[16];
      sprintf(t,"%f",float_value);
      return tmp = t;
    }
    case ft_Double: {
      char t[32];
      sprintf(t,"%f",double_value);
      return tmp = t;
    }
    }
  };



bool field_value::get_asBool() const {
    switch (field_type) {
    case ft_String: {
      if (str_value == "True" || str_value == "true" || str_value == "1")
          return true;
      else
	return false;
    }
    case ft_Boolean:{
      return bool_value;
      }
    case ft_Char: {
      if (char_value == 'T' || char_value == 't')
	return true;
      else
	return false;
    }
    case ft_Short: {
      return (bool)short_value;
    }
    case ft_UShort: {
      return (bool)ushort_value;
    }
    case ft_Long: {
      return (bool)long_value;
    }
    case ft_ULong: {
      return (bool)ulong_value;
    }
    case ft_Float: {
      return (bool)float_value;
    }
    case ft_Double: {
      return (bool)double_value;
    }
    }
  };
  

char field_value::get_asChar() const {
  switch (field_type) {
    case ft_String: {
      return str_value[0];
    }
    case ft_Boolean:{
      char c;
      if (bool_value) 
	return c='T';
      else
	return c='F';
    }
    case ft_Char: {
      return  char_value;
    }
    case ft_Short: {
      char t[10];
      sprintf(t,"%i",short_value);
      return t[0];
    }
    case ft_UShort: {
      char t[10];
      sprintf(t,"%i",ushort_value);
      return t[0];
    }
    case ft_Long: {
      char t[12];
      sprintf(t,"%i",long_value);
      return t[0];
    }
    case ft_ULong: {
      char t[12];
      sprintf(t,"%i",ulong_value);
      return t[0];
    }
    case ft_Float: {
      char t[16];
      sprintf(t,"%f",float_value);
      return t[0];
    }
    case ft_Double: {
      char t[32];
      sprintf(t,"%f",double_value);
      return t[0];
    }
    }
  };


short field_value::get_asShort() const {
    switch (field_type) {
    case ft_String: {
      return (short)atoi(str_value.c_str());
    }
    case ft_Boolean:{
      return (short)bool_value;
    }
    case ft_Char: {
      return (short)char_value;
    }
    case ft_Short: {
       return short_value;
    }
    case ft_UShort: {
       return (short)ushort_value;
    }
    case ft_Long: {
      return (short)long_value;
    }
    case ft_ULong: {
      return (short)ulong_value;
    }
    case ft_Float: {
      return (short)float_value;
    }
    case ft_Double: {
      return (short)double_value;
    }
    }
  };


unsigned short field_value::get_asUShort() const {
    switch (field_type) {
    case ft_String: {
      return (unsigned short)atoi(str_value.c_str());
    }
    case ft_Boolean:{
      return (unsigned short)bool_value;
    }
    case ft_Char: {
      return (unsigned short)char_value;
    }
    case ft_Short: {
       return (unsigned short)short_value;
    }
    case ft_UShort: {
       return ushort_value;
    }
    case ft_Long: {
      return (unsigned short)long_value;
    }
    case ft_ULong: {
      return (unsigned short)ulong_value;
    }
    case ft_Float: {
      return (unsigned short)float_value;
    }
    case ft_Double: {
      return (unsigned short)double_value;
    }
    }
  };

long field_value::get_asLong() const {
    switch (field_type) {
    case ft_String: {
      return (long)atoi(str_value.c_str());
    }
    case ft_Boolean:{
      return (long)bool_value;
    }
    case ft_Char: {
      return (long)char_value;
    }
    case ft_Short: {
       return (long)short_value;
    }
    case ft_UShort: {
       return (long)ushort_value;
    }
    case ft_Long: {
      return long_value;
    }
    case ft_ULong: {
      return (long)ulong_value;
    }
    case ft_Float: {
      return (long)float_value;
    }
    case ft_Double: {
      return (long)double_value;
    }
    }
  };

int field_value::get_asInteger() const{
  return (int)get_asLong();
}

unsigned long field_value::get_asULong() const {
    switch (field_type) {
    case ft_String: {
      return (unsigned long)atoi(str_value.c_str());
    }
    case ft_Boolean:{
      return (unsigned long)bool_value;
    }
    case ft_Char: {
      return (unsigned long)char_value;
    }
    case ft_Short: {
       return (unsigned long)short_value;
    }
    case ft_UShort: {
       return (unsigned long)ushort_value;
    }
    case ft_Long: {
      return (unsigned long)long_value;
    }
    case ft_ULong: {
      return ulong_value;
    }
    case ft_Float: {
      return (unsigned long)float_value;
    }
    case ft_Double: {
      return (unsigned long)double_value;
    }
    }
  };

float field_value::get_asFloat() const {
    switch (field_type) {
    case ft_String: {
      return (float)atof(str_value.c_str());
    }
    case ft_Boolean:{
      return (float)bool_value;
    }
    case ft_Char: {
      return (float)char_value;
    }
    case ft_Short: {
       return (float)short_value;
    }
    case ft_UShort: {
       return (float)ushort_value;
    }
    case ft_Long: {
      return (float)long_value;
    }
    case ft_ULong: {
      return (float)ulong_value;
    }
    case ft_Float: {
      return float_value;
    }
    case ft_Double: {
      return (float)double_value;
    }
    }
  };

double field_value::get_asDouble() const {
    switch (field_type) {
    case ft_String: {
      return atof(str_value.c_str());
    }
    case ft_Boolean:{
      return (double)bool_value;
    }
    case ft_Char: {
      return (double)char_value;
    }
    case ft_Short: {
       return (double)short_value;
    }
    case ft_UShort: {
       return (double)ushort_value;
    }
    case ft_Long: {
      return (double)long_value;
    }
    case ft_ULong: {
      return (double)ulong_value;
    }
    case ft_Float: {
      return (double)float_value;
    }
    case ft_Double: {
      return (double)double_value;
    }
    }
  };



field_value& field_value::operator= (const field_value & fv) {
  if ( this == &fv ) return *this;
  
  switch (fv.get_fType()) {
    case ft_String: {
      set_asString(fv.get_asString());
      return *this;
      break;
    }
    case ft_Boolean:{
      set_asBool(fv.get_asBool());
      return *this;
      break;     
    }
    case ft_Char: {
      set_asChar(fv.get_asChar());
      return *this;
      break;
    }
    case ft_Short: {
      set_asShort(fv.get_asShort());
      return *this;
      break;
    }
    case ft_UShort: {
      set_asUShort(fv.get_asUShort());
      return *this;
      break;
    }
    case ft_Long: {
      set_asLong(fv.get_asLong());
      return *this;
      break;
    }
    case ft_ULong: {
      set_asULong(fv.get_asULong());
      return *this;
      break;
    }
    case ft_Float: {
      set_asFloat(fv.get_asFloat());
      return *this;
      break;
    }
    case ft_Double: {
      set_asDouble(fv.get_asDouble());
      return *this;
      break;
    }
    }
};



//Set functions
void field_value::set_asString(const char *s) {
  str_value = s;
  field_type = ft_String;};

void field_value::set_asString(const string & s) {
  str_value = s;
  field_type = ft_String;};
  
void field_value::set_asBool(const bool b) {
  bool_value = b; 
  field_type = ft_Boolean;};
  
void field_value::set_asChar(const char c) {
  char_value = c; 
  field_type = ft_Char;};
  
void field_value::set_asShort(const short s) {
  short_value = s; 
  field_type = ft_Short;};
  
void field_value::set_asUShort(const unsigned short us) {
  ushort_value = us; 
  field_type = ft_UShort;};
  
void field_value::set_asLong(const long l) {
  long_value = l; 
  field_type = ft_Long;};

void field_value::set_asInteger(const int i) {
  long_value = (long)i; 
  field_type = ft_Long;};
  
void field_value::set_asULong(const unsigned long ul) {
  long_value = ul; 
  field_type = ft_ULong;};
  
void field_value::set_asFloat(const float f) {
  float_value = f; 
  field_type = ft_Float;};
  
void field_value::set_asDouble(const double d) {
  double_value = d; 
  field_type = ft_Double;};

  
fType field_value::get_field_type() {
  return field_type;}

  
string field_value::gft() {
    string tmp;
    switch (field_type) {
    case ft_String: {
      tmp.assign("string");
      return tmp;
    }
    case ft_Boolean:{
      tmp.assign("bool");
      return tmp;
    }
    case ft_Char: {
      tmp.assign("char");
      return tmp;
    }
    case ft_Short: {
      tmp.assign("short");
      return tmp;
    }
    case ft_Long: {
      tmp.assign("long");
      return tmp;
    }
    case ft_Float: {
      tmp.assign("float");
      return tmp;
    }
    case ft_Double: {
      tmp.assign("double");
      return tmp;
    }
    }
  }

} //namespace 

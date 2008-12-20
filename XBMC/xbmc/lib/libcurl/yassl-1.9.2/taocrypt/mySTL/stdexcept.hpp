/* mySTL stdexcept.hpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL.
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


/* mySTL memory implements exception, runtime_error
 *
 */

#ifndef mySTL_STDEXCEPT_HPP
#define mySTL_STDEXCEPT_HPP


#include <string.h>  // strncpy
#include <assert.h>  // assert
#include <stdlib.h>  // size_t


namespace mySTL {


class exception {
public:
    exception() {}
    virtual ~exception() {}   // to shut up compiler warnings

    virtual const char* what() const { return ""; }

    // for compiler generated call, never used
    static void operator delete(void*) { assert(0); }
private:
    // don't allow dynamic creation of exceptions
    static void* operator new(size_t);
};


class named_exception : public exception {
public:
    enum { NAME_SIZE = 80 };

    explicit named_exception(const char* str) 
    {
        strncpy(name_, str, NAME_SIZE);
        name_[NAME_SIZE - 1] = 0;
    }

    virtual const char* what() const { return name_; }
private:
    char name_[NAME_SIZE];
};


class runtime_error : public named_exception {
public:
    explicit runtime_error(const char* str) : named_exception(str) {}
};




} // namespace mySTL

#endif // mySTL_STDEXCEPT_HPP

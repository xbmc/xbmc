/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

/**
 * SWIGHIDDENVIRTUAL allows the keyword 'virtual' to be there when the main
 *  Addon api is compiled, but be hidden from the SWIG code generator. 
 *
 * This is to provide finer grain control over which methods are callbackable 
 *  (is that a word? ...)
 *  into the scripting language, and which ones are not. True polymorphic
 *  behavior across the scripting language boundary will ONLY occur where
 *  the keyword 'virtual' is used. In other words, you can use the macro
 *  SWIGHIDDENVIRTUAL to 'hide' the polymorphic behavior from the scripting
 *  language using the macro instead.
 *
 * Note: You should not hide virtual destructors from the scripting langage.
 */
#ifdef SWIG
#define SWIGHIDDENVIRTUAL 
#else
#define SWIGHIDDENVIRTUAL virtual
#endif

/**
 * SWIG_CONSTANT_FROM_GETTER will define a constant in the scripting
 *  language from a getter in the Addon api and also provide the 
 *  Addon api declaration. E.g. If you use:
 *
 *  SWIG_CONSTANT_FROM_GETTER(int, MY_CONSTANT);
 *
 *  ... in an Addon api header file then you need to define a function:
 *
 *  int getMy_CONSTANT();
 *
 *  ... in a .cpp file. That call will be made to determine the value 
 *  of the constant in the scripting language.
 */
#ifdef SWIG
#define SWIG_CONSTANT_FROM_GETTER(type,varname) %constant type varname = get##varname ()
#else
#define SWIG_CONSTANT_FROM_GETTER(type,varname) type get##varname ()
#endif

/**
 * SWIG_CONSTANT defines a constant in SWIG from an already existing
 * definition in the Addon api. E.g. a #define from the core window
 * system like SORT_METHOD_PROGRAM_COUNT included in the api via
 * a #include can be exposed to the scripting language using
 * SWIG_CONSTANT(int,SORT_METHOD_PROGRAM_COUNT).
 *
 * This macro can be used when the constant name and the C++ reference
 *   look the same. When they look different see SWIG_CONSTANT2
 *
 * Note, this declaration is invisible to the API C++ code and can
 *  only be seen by the SWIG processor.
 */
#ifdef SWIG
#define SWIG_CONSTANT(type,var) %constant type var = var
#else
#define SWIG_CONSTANT(type,var)
#endif

/**
 * SWIG_CONSTANT2 defines a constant in SWIG from an already existing
 * definition in the Addon api. E.g. a #define from the core window
 * system like SORT_METHOD_VIDEO_YEAR included in the api via
 * a #include can be exposed to the scripting language using
 * SWIG_CONSTANT2(int,SORT_METHOD_VIDEO_YEAR,SORT_METHOD_YEAR).
 *
 * This macro can be used when the constant name and the C++ reference
 *   don't look the same. When they look the same see SWIG_CONSTANT
 *
 * Note, this declaration is invisible to the API C++ code and can
 *  only be seen by the SWIG processor.
 */
#ifdef SWIG
#define SWIG_CONSTANT2(type,var,val) %constant type var = val
#else
#define SWIG_CONSTANT2(type,var,val)
#endif


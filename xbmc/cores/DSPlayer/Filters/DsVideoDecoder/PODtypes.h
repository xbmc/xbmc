/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _PODTYPES_H_
#define _PODTYPES_H_

template<class T> struct isPOD {enum {is=false};};
template<> struct isPOD<bool> {enum {is=true};};

template<> struct isPOD<char> {enum {is=true};};

template<> struct isPOD<signed char> {enum {is=true};};
template<> struct isPOD<short int> {enum {is=true};};
template<> struct isPOD<int> {enum {is=true};};
template<> struct isPOD<long int> {enum {is=true};};
template<> struct isPOD<__int64> {enum {is=true};};

template<> struct isPOD<unsigned char> {enum {is=true};};
template<> struct isPOD<unsigned short int> {enum {is=true};};
template<> struct isPOD<unsigned int> {enum {is=true};};
template<> struct isPOD<unsigned long int> {enum {is=true};};
template<> struct isPOD<unsigned __int64> {enum {is=true};};

template<> struct isPOD<float> {enum {is=true};};
template<> struct isPOD<double> {enum {is=true};};
template<> struct isPOD<long double> {enum {is=true};};

#if defined(__INTEL_COMPILER) || defined(__GNUC__) || (_MSC_VER>=1300)
template<> struct isPOD<wchar_t> {enum {is=true};};
template<class Tp> struct isPOD<Tp*> {enum {is=true};};
#endif

template<class A> struct allocator_traits {enum {is_static=false};};

#endif

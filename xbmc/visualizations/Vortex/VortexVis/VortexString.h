/*
 *      Copyright (C) 2010 Team XBMC
 *      http://xbmc.org
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//
// asCScriptString
//
// This class is used to pass strings between the application and the script engine.
// It is basically a container for the normal std::string, with the addition of a 
// reference counter so that the script can use object handles to hold the type.
//
// Because the class is reference counted it cannot be stored locally in the 
// application functions, nor be received or returned by value. Instead it should
// be manipulated through pointers or references.
// 
// Note, because the internal buffer is placed at the beginning of the class 
// structure it is infact possible to receive this type as a reference or pointer
// to a normal std::string where the reference counter doesn't have to be manipulated.
//

#ifndef VORTEX_SCRIPTSTRING_H
#define VORTEX_SCRIPTSTRING_H

#include "..\angelscript\sdk\angelscript\include\angelscript.h"
//#include <string>

//BEGIN_AS_NAMESPACE

class ScriptString_c
{
public:
	ScriptString_c();
	//	asCScriptString(const asCScriptString &other);
	ScriptString_c(const char *s);
	//	asCScriptString(const std::string &s);

	void AddRef();
	void Release();

	//	asCScriptString &operator=(const asCScriptString &other);
	//	asCScriptString &operator+=(const asCScriptString &other);

	//	std::string buffer;
	char* myString;
	int stringLength;

protected:
	~ScriptString_c();
	int refCount;
};

// Call this function to register all the necessary 
// functions for the scripts to use this type
void RegisterVortexScriptString(asIScriptEngine *engine);

//END_AS_NAMESPACE

#endif

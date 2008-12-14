///
///	@file 	list.cpp
/// @brief 	List support for the List, Link and MprStringList clases
/// @overview This is the preferred list class for all of the MPR 
///		and supporting modules.
///	@remarks This module is not thread-safe. It is the callers responsibility
///	to perform all thread synchronization.
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr.h"

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// List, Link /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	All List and Link methods are declared inline in mpr.h
//

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprStringList ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprStringList::MprStringList()
{
}

////////////////////////////////////////////////////////////////////////////////

MprStringList::MprStringList(char *str)
{
	parse(str);
}

////////////////////////////////////////////////////////////////////////////////

MprStringList::~MprStringList()
{
	MprStringData	*sp, *next;

	sp = (MprStringData*) getFirst();
	while (sp) {
		next = (MprStringData*) getNext(sp);
		//
		//	This will delete the item and if it is a MprStringData object, 
		//	it will free any allocated string.
		//
		delete sp;
		sp = next;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MprStringList::insert(char *s)
{
	head->insert(new MprStringData(s));
}

////////////////////////////////////////////////////////////////////////////////
//
//	Parse a string and create a list of words 
//

void MprStringList::parse(char *str)
{
	char	*word, *tok;

	mprAssert(str);
	if (str == 0 || *str == '\0') {
		return;
	}

	str = mprStrdup(str);
	word = mprStrTok(str, " \t\r\n", &tok);
	while (word) {
		insert(word);
		word = mprStrTok(0, " \t\r\n", &tok);
	}
	mprFree(str);
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED_AS_YET
//
//	Convert the list to a string of words. 
//

void char *MprStringList::convertToString()
{
	int		size;
	char	*sp, *str;

	mprAssert(lp);
	mprAssert(str);

	if (next == head) {
		return 0;
	}

	//
	//	Determine size of string needed
	//
	size = 0;
	np = getFirst();
	while (np) {
		size += strlen(np->str);
		np = getNext(np);
		size++;
	}
	str = new char[size];
		
	//
	//	Copy strings
	//
	sp = str;
	np = getFirst();
	while (np) {
		strcpy(sp, np->str);
		np = getNext(np);
	}
	return str;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Convert a list to an Argv style array
//

char **MprStringList::convertToArgv()
{
	MprStringList	*np;
	int				count;

	count = getNumItems();
	argv = new char*[(count + 1)];
	np = getFirst();
	while (np) {
		argv[count] = np->str;
		np = getNext(np);
	}
}
	
#endif
////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MprStringData /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprStringData::MprStringData(char *s) 
{ 
	string = mprStrdup(s); 
}

////////////////////////////////////////////////////////////////////////////////

MprStringData::~MprStringData() 
{ 
	mprFree(string);
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//

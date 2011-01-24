//-----------------------------------------------------------------------------
//
// File:	QCDModTagEditor
//
// About:	Tag Editing plugin module interface.  This file is published with the 
//			QCD plugin SDK.
//
// Authors:	Written by Paul Quinn
//
// Copyright:
//
//	QCD multimedia player application Software Development Kit Release 1.0.
//
//	Copyright (C) 2002 Quinnware
//
//	This code is free.  If you redistribute it in any form, leave this notice 
//	here.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//-----------------------------------------------------------------------------

#ifndef QCDMODTAGEDITOR_H
#define QCDMODTAGEDITOR_H

#include "QCDModDefs.h"

// name of the DLL export for output plugins
#define TAGEDITORDLL_ENTRY_POINT	QTagEditorModule

// Tag field ids
typedef enum
{
	TAGFIELD_FIRSTFIELD = 0,

	TAGFIELD_TITLE = 0,
	TAGFIELD_ARTIST,
	TAGFIELD_ALBUM, 
	TAGFIELD_GENRE,
	TAGFIELD_YEAR,  
	TAGFIELD_TRACK, 
	TAGFIELD_COMMENT,

	TAGFIELD_COMPOSER,
	TAGFIELD_CONDUCTOR,
	TAGFIELD_ORCHESTRA,
	TAGFIELD_YEARCOMPOSED,

	TAGFIELD_ORIGARTIST,
	TAGFIELD_LABEL, 
	TAGFIELD_COPYRIGHT,
	TAGFIELD_ENCODER,
	TAGFIELD_CDDBTAGID,

	TAGFIELD_FIELDCOUNT
};

//-----------------------------------------------------------------------------

typedef struct 
{
	UINT	size;			// size of init structure
	UINT	version;		// plugin structure version (set to PLUGIN_API_VERSION)

	LPCSTR	description;
	LPCSTR	defaultexts;

	bool	(*Read)(LPCSTR filename, void* tagHandle);
	bool	(*Write)(LPCSTR filename, void* tagHandle);
	bool	(*Strip)(LPCSTR filename);

	void	(*ShutDown)(int flags);

	void	(*SetFieldA)(void* tagHandle, int fieldId, LPCSTR szNewText);
	void	(*SetFieldW)(void* tagHandle, int fieldId, LPCWSTR szNewText);

	LPCSTR	(*GetFieldA)(void* tagHandle, int fieldId);
	LPCWSTR	(*GetFieldW)(void* tagHandle, int fieldId);

} QCDModInitTag;

#endif //QCDMODTAGEDITOR_H
/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id$

  Portable file I/O routines

==============================================================================*/
#include "xbsection_start.h"


/*

	The way this module works:

	- _mm_fopen will call the errorhandler [see mmerror.c] in addition to
	  setting _mm_errno on exit.
	- _mm_iobase is for internal use.  It is used by Player_LoadFP to
	  ensure that it works properly with wad files.
	- _mm_read_I_* and _mm_read_M_* differ : the first is for reading data
	  written by a little endian (intel) machine, and the second is for reading
	  big endian (Mac, RISC, Alpha) machine data.
	- _mm_write functions work the same as the _mm_read functions.
	- _mm_read_string is for reading binary strings.  It is basically the same
	  as an fread of bytes.

*/

/* FIXME
	the _mm_iobase variable ought to be MREADER-specific. It will eventually
	become a private field of the MREADER structure, but this will require a
	soname version bump.

	In the meantime, the drawback is that if you use the xxx_LoadFP functions,
	you can't have several MREADER objects with different iobase values.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include "mikmod.h"
#include "mikmod_internals.h"

#ifdef SUNOS
extern int fclose(FILE *);
extern int fgetc(FILE *);
extern int fputc(int, FILE *);
extern size_t fread(void *, size_t, size_t, FILE *);
extern int fseek(FILE *, long, int);
extern size_t fwrite(const void *, size_t, size_t, FILE *);
#endif

#define COPY_BUFSIZE  1024

static long _mm_iobase=0,temp_iobase=0;

FILE* _mm_fopen(CHAR* fname,CHAR* attrib)
{
	FILE *fp;

	if(!(fp=fopen(fname,attrib))) {
		_mm_errno = MMERR_OPENING_FILE;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return fp;
}

BOOL _mm_FileExists(CHAR* fname)
{
	FILE *fp;

	if(!(fp=fopen(fname,"r"))) return 0;
	fclose(fp);

	return 1;
}

int _mm_fclose(FILE *fp)
{
	return fclose(fp);
}

/* Sets the current file-position as the new _mm_iobase */
void _mm_iobase_setcur(MREADER* reader)
{
	temp_iobase=_mm_iobase;  /* store old value in case of revert */
	_mm_iobase=reader->Tell(reader);
}

/* Reverts to the last known _mm_iobase value. */
void _mm_iobase_revert(void)
{
	_mm_iobase=temp_iobase;
}

/*========== File Reader */

typedef struct MFILEREADER {
	MREADER core;
	FILE*   file;
} MFILEREADER;

static BOOL _mm_FileReader_Eof(MREADER* reader)
{
	return feof(((MFILEREADER*)reader)->file);
}

static BOOL _mm_FileReader_Read(MREADER* reader,void* ptr,size_t size)
{
	return fread(ptr,size,1,((MFILEREADER*)reader)->file);
}

static int _mm_FileReader_Get(MREADER* reader)
{
	return fgetc(((MFILEREADER*)reader)->file);
}

static BOOL _mm_FileReader_Seek(MREADER* reader,long offset,int whence)
{
	return fseek(((MFILEREADER*)reader)->file,
				 (whence==SEEK_SET)?offset+_mm_iobase:offset,whence);
}

static long _mm_FileReader_Tell(MREADER* reader)
{
	return ftell(((MFILEREADER*)reader)->file)-_mm_iobase;
}

MREADER *_mm_new_file_reader(FILE* fp)
{
	MFILEREADER* reader=(MFILEREADER*)_mm_malloc(sizeof(MFILEREADER));
	if (reader) {
		reader->core.Eof =&_mm_FileReader_Eof;
		reader->core.Read=&_mm_FileReader_Read;
		reader->core.Get =&_mm_FileReader_Get;
		reader->core.Seek=&_mm_FileReader_Seek;
		reader->core.Tell=&_mm_FileReader_Tell;
		reader->file=fp;
	}
	return (MREADER*)reader;
}

void _mm_delete_file_reader (MREADER* reader)
{
	if(reader) free(reader);
}

/*========== File Writer */

typedef struct MFILEWRITER {
	MWRITER core;
	FILE*   file;
} MFILEWRITER;

static BOOL _mm_FileWriter_Seek(MWRITER* writer,long offset,int whence)
{
	return fseek(((MFILEWRITER*)writer)->file,offset,whence);
}

static long _mm_FileWriter_Tell(MWRITER* writer)
{
	return ftell(((MFILEWRITER*)writer)->file);
}

static BOOL _mm_FileWriter_Write(MWRITER* writer,void* ptr,size_t size)
{
	return (fwrite(ptr,size,1,((MFILEWRITER*)writer)->file)==size);
}

static BOOL _mm_FileWriter_Put(MWRITER* writer,int value)
{
	return fputc(value,((MFILEWRITER*)writer)->file);
}

MWRITER *_mm_new_file_writer(FILE* fp)
{
	MFILEWRITER* writer=(MFILEWRITER*)_mm_malloc(sizeof(MFILEWRITER));
	if (writer) {
		writer->core.Seek =&_mm_FileWriter_Seek;
		writer->core.Tell =&_mm_FileWriter_Tell;
		writer->core.Write=&_mm_FileWriter_Write;
		writer->core.Put  =&_mm_FileWriter_Put;
		writer->file=fp;
	}
	return (MWRITER*) writer;
}

void _mm_delete_file_writer (MWRITER* writer)
{
	if(writer) free (writer);
}

/*========== Write functions */

void _mm_write_string(CHAR* data,MWRITER* writer)
{
	if(data)
		_mm_write_UBYTES(data,strlen(data),writer);
}

void _mm_write_M_UWORD(UWORD data,MWRITER* writer)
{
	_mm_write_UBYTE(data>>8,writer);
	_mm_write_UBYTE(data&0xff,writer);
}

void _mm_write_I_UWORD(UWORD data,MWRITER* writer)
{
	_mm_write_UBYTE(data&0xff,writer);
	_mm_write_UBYTE(data>>8,writer);
}

void _mm_write_M_ULONG(ULONG data,MWRITER* writer)
{
 	_mm_write_M_UWORD(data>>16,writer);
	_mm_write_M_UWORD(data&0xffff,writer);
}

void _mm_write_I_ULONG(ULONG data,MWRITER* writer)
{
	_mm_write_I_UWORD(data&0xffff,writer);
	_mm_write_I_UWORD(data>>16,writer);
}

void _mm_write_M_SWORD(SWORD data,MWRITER* writer)
{
	_mm_write_M_UWORD((UWORD)data,writer);
}

void _mm_write_I_SWORD(SWORD data,MWRITER* writer)
{
	_mm_write_I_UWORD((UWORD)data,writer);
}

void _mm_write_M_SLONG(SLONG data,MWRITER* writer)
{
	_mm_write_M_ULONG((ULONG)data,writer);
}

void _mm_write_I_SLONG(SLONG data,MWRITER* writer)
{
	_mm_write_I_ULONG((ULONG)data,writer);
}

#if defined __STDC__ || defined _MSC_VER
#define DEFINE_MULTIPLE_WRITE_FUNCTION(type_name,type)						\
void _mm_write_##type_name##S (type *buffer,int number,MWRITER* writer)		\
{																			\
	while(number-->0)														\
		_mm_write_##type_name(*(buffer++),writer);							\
}
#else
#define DEFINE_MULTIPLE_WRITE_FUNCTION(type_name,type)						\
void _mm_write_/**/type_name/**/S (type *buffer,int number,MWRITER* writer)	\
{																			\
	while(number-->0)														\
		_mm_write_/**/type_name(*(buffer++),writer);						\
}
#endif

DEFINE_MULTIPLE_WRITE_FUNCTION(M_SWORD,SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION(M_UWORD,UWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_SWORD,SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_UWORD,UWORD)

DEFINE_MULTIPLE_WRITE_FUNCTION(M_SLONG,SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION(M_ULONG,ULONG)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_SLONG,SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION(I_ULONG,ULONG)

/*========== Read functions */

int _mm_read_string(CHAR* buffer,int number,MREADER* reader)
{
	return reader->Read(reader,buffer,number);
}

UWORD _mm_read_M_UWORD(MREADER* reader)
{
	UWORD result=((UWORD)_mm_read_UBYTE(reader))<<8;
	result|=_mm_read_UBYTE(reader);
	return result;
}

UWORD _mm_read_I_UWORD(MREADER* reader)
{
	UWORD result=_mm_read_UBYTE(reader);
	result|=((UWORD)_mm_read_UBYTE(reader))<<8;
	return result;
}

ULONG _mm_read_M_ULONG(MREADER* reader)
{
	ULONG result=((ULONG)_mm_read_M_UWORD(reader))<<16;
	result|=_mm_read_M_UWORD(reader);
	return result;
}

ULONG _mm_read_I_ULONG(MREADER* reader)
{
	ULONG result=_mm_read_I_UWORD(reader);
	result|=((ULONG)_mm_read_I_UWORD(reader))<<16;
	return result;
}

SWORD _mm_read_M_SWORD(MREADER* reader)
{
	return((SWORD)_mm_read_M_UWORD(reader));
}

SWORD _mm_read_I_SWORD(MREADER* reader)
{
	return((SWORD)_mm_read_I_UWORD(reader));
}

SLONG _mm_read_M_SLONG(MREADER* reader)
{
	return((SLONG)_mm_read_M_ULONG(reader));
}

SLONG _mm_read_I_SLONG(MREADER* reader)
{
	return((SLONG)_mm_read_I_ULONG(reader));
}

#if defined __STDC__ || defined _MSC_VER
#define DEFINE_MULTIPLE_READ_FUNCTION(type_name,type)						\
int _mm_read_##type_name##S (type *buffer,int number,MREADER* reader)		\
{																			\
	while(number-->0)														\
		*(buffer++)=_mm_read_##type_name(reader);							\
	return !reader->Eof(reader);											\
}
#else
#define DEFINE_MULTIPLE_READ_FUNCTION(type_name,type)						\
int _mm_read_/**/type_name/**/S (type *buffer,int number,MREADER* reader)	\
{																			\
	while(number-->0)														\
		*(buffer++)=_mm_read_/**/type_name(reader);							\
	return !reader->Eof(reader);											\
}
#endif

DEFINE_MULTIPLE_READ_FUNCTION(M_SWORD,SWORD)
DEFINE_MULTIPLE_READ_FUNCTION(M_UWORD,UWORD)
DEFINE_MULTIPLE_READ_FUNCTION(I_SWORD,SWORD)
DEFINE_MULTIPLE_READ_FUNCTION(I_UWORD,UWORD)

DEFINE_MULTIPLE_READ_FUNCTION(M_SLONG,SLONG)
DEFINE_MULTIPLE_READ_FUNCTION(M_ULONG,ULONG)
DEFINE_MULTIPLE_READ_FUNCTION(I_SLONG,SLONG)
DEFINE_MULTIPLE_READ_FUNCTION(I_ULONG,ULONG)

/* ex:set ts=4: */

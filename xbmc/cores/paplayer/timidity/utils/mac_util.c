/* 
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
	
    mac_util.c
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include 	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"mac_util.h"


OSErr GetFullPath( const FSSpec* spec, Str255 fullPath)
{
	CInfoPBRec		myPB;
	Str255		dirName;
	OSErr		myErr;
	 
	fullPath[0] = '\0';
	BlockMoveData(spec->name, fullPath, spec->name[spec->name[0]]+1 );
	
	myPB.dirInfo.ioNamePtr = dirName;
	myPB.dirInfo.ioVRefNum = spec->vRefNum;
	myPB.dirInfo.ioDrParID = spec->parID;
	myPB.dirInfo.ioFDirIndex = -1;
							
	do{
		myPB.dirInfo.ioDrDirID = myPB.dirInfo.ioDrParID;
		myErr = PBGetCatInfoSync(&myPB);
		
		dirName[0]++;
		dirName[ dirName[0] ]=':';
		if( dirName[0]+fullPath[0]>=254 ){ fullPath[0]=0; return 1; }
		BlockMoveData(fullPath+1, dirName+dirName[0]+1, fullPath[0]);
		dirName[0]+=fullPath[0];
		BlockMoveData(dirName, fullPath, dirName[0]+1);
	}while( !(myPB.dirInfo.ioDrDirID == fsRtDirID) );
	return noErr;
 }

void StopAlertMessage(Str255 s)
{
	ParamText(s,"\p","\p","\p");
	StopAlert(128,0);
}

void	LDeselectAll(ListHandle lHandle)
{
	Cell  cell={0,0};
	while(LGetSelect(true, &cell, lHandle)){
		LSetSelect(false, cell, lHandle);
	}
}

void	TEReadFile(char* filename, TEHandle te)
{
	FILE	*in;
	
		//clear TE
	TESetSelect(0, (**te).teLength, te);
	TEDelete(te);
	
		//read
	if( (in=fopen(filename, "rb"))==0 ){
		char *s="No document.";
		TEInsert(s, strlen(s), te);
	}else{
		char	buf[80];
		int		len;
		while(fgets( buf, 80, in)){
			len=strlen(buf);
			if( buf[len-1]==0xA ){
				buf[len-1]=0xD; //len--;
			} 
			TEInsert(buf, len, te);
		}
		fclose(in);
	}
}

/***********************/
#pragma	mark	-

void SetDialogItemValue(DialogPtr dialog, short item, short value)
{
	short			itemType;
	ControlHandle	itemHandle;
	Rect			itemRect;
	
	GetDialogItem(dialog, item, &itemType, (Handle*)&itemHandle, &itemRect);
	SetControlValue(itemHandle, value);
}

short GetDialogItemValue(DialogPtr dialog, short item )
{
	short			itemType;
	ControlHandle	itemHandle;
	Rect			itemRect;
	
	GetDialogItem(dialog, item, &itemType, (Handle*)&itemHandle, &itemRect);
	return GetControlValue(itemHandle);
}

void SetDialogTEValue(DialogRef dialog, short item, int value)
{
	Str255		s;
	
	NumToString(value, s);
	mySetDialogItemText(dialog, item, s);
}

int GetDialogTEValue(DialogRef dialog, short item )
{
	Str255		s;
	
	myGetDialogItemText(dialog, item, s);
	return atof(p2cstr(s));
}

short ToggleDialogItem(DialogPtr dialog, short item )
{
	short			itemType, value;
	ControlHandle	itemHandle;
	Rect			itemRect;
	
	GetDialogItem(dialog, item, &itemType, (Handle*)&itemHandle, &itemRect);
	value=GetControlValue(itemHandle);
	SetControlValue(itemHandle, !value);
	return !value;
}

void myGetDialogItemText(DialogPtr theDialog, short itemNo, Str255 s)
{
	short	itemType;
	Handle	item;
	Rect	box;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	GetDialogItemText(item, s);
}

void mySetDialogItemText(DialogRef theDialog, short itemNo, const Str255 text)
{
	short	itemType;
	Handle	item;
	Rect	box;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	SetDialogItemText(item, text);
}

void SetDialogControlTitle(DialogRef theDialog, short itemNo, const Str255 text)
{
	short	itemType;
	Handle	item;
	Rect	box;
	
	GetDialogItem(theDialog, itemNo, &itemType, &item, &box);
	SetControlTitle((ControlRef)item, text);
}

void SetDialogItemHilite(DialogRef dialog, short item, short value)
{
	short		itemType;
	ControlRef	itemHandle;
	Rect		itemRect;
	
	GetDialogItem(dialog, item, &itemType, (Handle *)&itemHandle, &itemRect);
	HiliteControl(itemHandle, value);
}

// ************************************************
void    mac_TransPathSeparater(const char str[], char out[])
{
	int i;
	for( i=0; str[i]!=0; i++ ){
		if( str[i]=='/' ){
			out[i]=':';
		} else {
			out[i]=str[i];
		}
	}
}
#if 0

char** sys_errlist_()
{
	static char	s[80];
	static char*	ps=s;
	static char**	ret=&ps;
	
	/* if( errno==ENOENT )  strcpy(s,"File not Found.");
	else */   /* ENOENT vanished at CWPro2 */
	if( errno==ENOSPC )
		strcpy(s,"Out of Space.");
	else
		snprintf(s, 80, "error no.%d", errno);
	
	return(ret-errno);
}
#endif /*0*/
// **************************************************
#pragma mark -

/* special fgets */
/* allow \r as return code */
char* mac_fgets( char buf[], int n, FILE* file)
{

	int c,i;
	
	if( n==0 )  return 0;
	for( i=0; i<n-1; ){
		buf[i++]=c=fgetc(file);
		if( c=='\n' || c=='\r' || c==EOF ) break;
	}
	
	buf[i]=0;
	if( i>0 && buf[i-1]==EOF ){ buf[i-1]=0; i--; }
	
	if( i==0 && c==EOF && feof(file) ) return 0;
	
	
	if( i==1 && buf[0]=='\n' ){ /* probably dos LF (Unix blank line?)*/
		return mac_fgets( buf, n, file);
	}
	else if( buf[i-1]=='\n' ){	/*unix*/
		return buf;
	}
	else if( buf[i-1]=='\r' ){ /*mac or dos*/
		buf[i-1]='\n';
		return buf;
	}
	return buf;  /* for safty*/
}

void *mac_memchr (const void *s, int c, size_t n)
{
	char* buf=(char*)s;
	int i;
	
	for( i=0; i<n; i++ ){
		if( buf[i]==c || (c=='\n' && buf[i]=='\r') ){
			return buf+i;
		}
	}
	return 0;
}

void p2cstrcpy(char* dst, ConstStr255Param src)
{
	int i;
	for( i=0; i<src[0]; i++){
		dst[i]=src[i+1];
	}
	dst[i]=0;
}

int mac_strcasecmp(const char *s1, const char *s2)
{
	int	c1,c2,i;
	
	for( i=0; ; i++){
		if( !s1[i] && !s2[i] ) return 0; //equal
		if( !s1[i] || !s2[i] ) return 1;
		c1= ( isupper(s1[i]) ? tolower(s1[i]) : s1[i] );
		c2= ( isupper(s2[i]) ? tolower(s2[i]) : s2[i] );
		if( c1 != c2 )	return 1;
	}
	return 0; //equal
}

#define CHAR_CASECMP(c1,c2) ((isupper(c1)?tolower(c1):(c1))!=(isupper(c2)?tolower(c2):(c2)))

int mac_strncasecmp(const char *s1, const char *s2, size_t n )
{
	char	c1,c2;
	int i;

	for( i=0; i<n; i++){
		if( !s1[i] && !s2[i] ) return 0; //equal
		if( !s1[i] || !s2[i] ) return 1;
		c1= ( isupper(s1[i]) ? tolower(s1[i]) : s1[i] );
		c2= ( isupper(s2[i]) ? tolower(s2[i]) : s2[i] );
		if( c1 != c2 )	return 1;
	}
	return 0; //equal
}

#define min(a,b)  (((a) < (b)) ? (a) : (b))


int strtailcasecmp(const char *s1, const char *s2)
{
	int	len1, len2, cmplen,
		i, i1,i2;
	
	len1=strlen(s1); len2=strlen(s2);
	cmplen= min(len1,len2);
	for( i=0, i1=len1-1, i2=len2-1; i<cmplen; i++, i1--, i2--){
		if( CHAR_CASECMP(s1[i1],s2[i2])!=0 ){
			return 1;  //differ
		}
	}
	return 0; //equal
}




#include "stdafx.h"
#include "Xenium.h"
#include "XeniumSPI.h"

#define SPI0 0
#define SPI1 1



void Xenium::OutputByte(unsigned char data)
{
	EnterCriticalSection(&CriticalSection);
	XeniumSPISendByte(data,SPI0);
	LeaveCriticalSection(&CriticalSection);
}

void Xenium::OutputString(const char *str, int maxLen)
{
	int i;
	EnterCriticalSection(&CriticalSection);
	for (i=0; (0 != str[i]) && (i<maxLen);i++)
	{
		XeniumSPISendByte( str[i],SPI0 );
	} 
	LeaveCriticalSection(&CriticalSection);
}

void Xenium::OutputCenteredString( const char *str, int row )
{
	int len;
	int max;
	int ellipses = 0;
	int i;
	
	len = strlen(str);
	if (len > 20)
	{
		ellipses = 1;
		len = 20;
		max = 17;
	}
	else
	{
		max = len;
	}
	EnterCriticalSection(&CriticalSection);
	SetCursorPosition((20 - len) / 2,row);
	for (i=0;i<max;i++)
	{
		XeniumSPISendByte( str[i],SPI0 );
	} 

	if (ellipses)
	{
		XeniumSPISendByte( '.',SPI0 );
		XeniumSPISendByte( '.',SPI0 );
		XeniumSPISendByte( '.',SPI0 );
	}
	LeaveCriticalSection(&CriticalSection);
}

	
void Xenium::HideDisplay(void)
{
	XeniumSPISendByte( 2,SPI0 );
}
	
void Xenium::ShowDisplay(void)
{
	XeniumSPISendByte( 3,SPI0 ); 
}

void Xenium::HideCursor(void)
{
	XeniumSPISendByte( 4 ,SPI0);
}
	
void Xenium::ShowUnderlineCursor(void)
{
	XeniumSPISendByte( 5 ,SPI0);
}

void Xenium::ShowBlockCursor(void)
{
	XeniumSPISendByte( 6,SPI0 ); 
}

void Xenium::ShowInvertedCursor(void)
{
	XeniumSPISendByte( 7,SPI0 ); 
}

void Xenium::Backspace(void)
{
	XeniumSPISendByte( 8,SPI0 ); 
}

void Xenium::LineFeed(void)
{
	XeniumSPISendByte( 10,SPI0 );
}
	
void Xenium::DeleteInPlace(void)
{
	XeniumSPISendByte( 11,SPI0 ); 
}
	
void Xenium::FormFeed(void)
{
	XeniumSPISendByte( 12,SPI0 ); 
}
	
void Xenium::CarriageReturn(void)
{
	XeniumSPISendByte( 13,SPI0 );
}

void Xenium::SetBacklight(unsigned int level)
{
	if (level > 25) level = 25;
	EnterCriticalSection(&CriticalSection); 
	XeniumSPISendByte( 14,SPI0 );
	XeniumSPISendByte(level*4,SPI0);
	LeaveCriticalSection(&CriticalSection); 
}
	
void Xenium::SetContrast(unsigned int level)
{
	if (level > 25) level = 25;
	EnterCriticalSection(&CriticalSection); 
	XeniumSPISendByte( 15,SPI0 );
	XeniumSPISendByte(level*4,SPI0);
	LeaveCriticalSection(&CriticalSection); 
}

void Xenium::SetCursorPosition(  unsigned int col, unsigned int row )
{
	if (row > 3) row = 3;
	if (col > 19) col = 19;
	EnterCriticalSection(&CriticalSection); 
	XeniumSPISendByte( 17,SPI0 );
	XeniumSPISendByte( col,SPI0 );
	XeniumSPISendByte( row,SPI0 );
	LeaveCriticalSection(&CriticalSection); 
}

void Xenium::HorizontalBarGraph( unsigned char graphIndex, unsigned char style, unsigned char startCol, unsigned char endCol, unsigned char length, unsigned char row)
{
	EnterCriticalSection(&CriticalSection); 
	XeniumSPISendByte( 18,SPI0 );
	XeniumSPISendByte( graphIndex,SPI0 );
	XeniumSPISendByte( style,SPI0 );
	XeniumSPISendByte( startCol,SPI0 );
	XeniumSPISendByte( endCol,SPI0 );
	XeniumSPISendByte( length,SPI0 );
	XeniumSPISendByte( row,SPI0 );
	LeaveCriticalSection(&CriticalSection); 
}

void Xenium::ScrollOn(void)
{
	XeniumSPISendByte( 19,SPI0 );
}

void Xenium::ScrollOff(void)
{
	XeniumSPISendByte( 20,SPI0 ); 
}

void Xenium::WrapOn(void)
{
	XeniumSPISendByte( 23,SPI0 ); 
}

void Xenium::WrapOff(void)
{
	XeniumSPISendByte( 24,SPI0 );
}

void Xenium::Reboot(void)
{
	EnterCriticalSection(&CriticalSection); 
	for (int i=0;i<9;i++)
	{
		XeniumSPISendByte(' ',SPI0);
	}
	XeniumSPISendByte( 26,SPI0 );
	XeniumSPISendByte( 26,SPI0 );
	LeaveCriticalSection(&CriticalSection); 
}

void Xenium::CursorUp(void)
{
	EnterCriticalSection(&CriticalSection); 
	XeniumSPISendByte( 27,SPI0 );
	XeniumSPISendByte( 91,SPI0 );
	XeniumSPISendByte( 65,SPI0 );
	LeaveCriticalSection(&CriticalSection); 
}

void Xenium::CursorDown(void)
{
	EnterCriticalSection(&CriticalSection); 
	XeniumSPISendByte( 27,SPI0 );
	XeniumSPISendByte( 91,SPI0 );
	XeniumSPISendByte( 66,SPI0 );
	LeaveCriticalSection(&CriticalSection);
}

void Xenium::CursorRight(void)
{
	EnterCriticalSection(&CriticalSection); 
	XeniumSPISendByte( 27,SPI0 );
	XeniumSPISendByte( 91,SPI0 );
	XeniumSPISendByte( 67,SPI0 );
	LeaveCriticalSection(&CriticalSection); 
}

void Xenium::CursorLeft(void)
{
	EnterCriticalSection(&CriticalSection);
	XeniumSPISendByte( 27,SPI0 );
	XeniumSPISendByte( 91,SPI0 );
	XeniumSPISendByte( 68,SPI0 );
	LeaveCriticalSection(&CriticalSection); 
}

void Xenium::LargeBlockNumber( unsigned char style, unsigned char column, unsigned char number)
{
	if (style > 1) style = 1;
	if (!style && column > 17) column = 17;
	if (style && column > 16) column = 16;
	if (number < '0') number = '0';
	if (number > '9') number = '9';
	EnterCriticalSection(&CriticalSection);
	XeniumSPISendByte( 28,SPI0 );
	XeniumSPISendByte( style,SPI0 );
	XeniumSPISendByte( column,SPI0 );
	XeniumSPISendByte( number,SPI0 );
	LeaveCriticalSection(&CriticalSection);
}

void Xenium::AcquireLCDLock(void)
{
	EnterCriticalSection(&CriticalSection); 
}

void Xenium::ReleaseLCDLock(void)
{
	LeaveCriticalSection(&CriticalSection);
}
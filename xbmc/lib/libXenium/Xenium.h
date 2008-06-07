//  Xenium LCD Driver
#ifndef XENIUM_LCD_H
#define XENIUM_LCD_H

#pragma once

class Xenium 
{
public:
	void OutputByte(unsigned char data);
	void OutputString(const char *str, int maxLen);
	void OutputCenteredString(const char *str, int row);
	
	void HideDisplay(void);
	void ShowDisplay(void);
	void HideCursor(void);
	void ShowUnderlineCursor(void);
	void ShowBlockCursor(void);
	void ShowInvertedCursor(void);
	void Backspace(void);
	void LineFeed(void);
	void DeleteInPlace(void);
	void FormFeed(void);
	void CarriageReturn(void);
	void SetBacklight(unsigned int level);
	void SetContrast(unsigned int level);
	void SetCursorPosition(unsigned int col, unsigned int row);
	void HorizontalBarGraph( unsigned char graphIndex, unsigned char style, unsigned char startCol, unsigned char endCol, unsigned char length, unsigned char row);
	void ScrollOn(void);
	void ScrollOff(void);
	void WrapOn(void);
	void WrapOff(void);
	void Reboot(void);
	void CursorUp(void);
	void CursorDown(void);
	void CursorRight(void);
	void CursorLeft(void);
	void LargeBlockNumber( unsigned char style, unsigned char column, unsigned char number);

	void AcquireLCDLock(void);
	void ReleaseLCDLock(void);

private:
	CRITICAL_SECTION CriticalSection;	// Critical section object

	char *m_ScrollText;			// Current scrolling text
	int m_ScrollRow;				// Current scrolling row
	bool m_Scrolling;			// Scroll thread sentinal

public:	
    Xenium()
	{
		InitializeCriticalSection(&CriticalSection);
    //Statics
    m_ScrollText = 0;
    m_ScrollRow = 0;
    m_Scrolling = false;
	}
	
	virtual ~Xenium()
	{
		DeleteCriticalSection(&CriticalSection);
	}

public:


};



#endif

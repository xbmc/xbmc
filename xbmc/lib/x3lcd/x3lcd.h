#pragma once
#include "utils/thread.h"
#include "utils/lcd.h"

#define MAX_ROWS 20

/*
HD44780 or equivalent
Character located  1   2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 
DDRAM address      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13
DDRAM address      40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53
DDRAM address      14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27
DDRAM address      54 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67
*/

#define SCROLL_SPEED_IN_MSEC 	250
#define DISP_O_DAT				0xF504
#define DISP_O_CMD				0xF505
#define DISP_O_DIR_DAT			0xF506
#define DISP_O_DIR_CMD			0xF507
#define DISP_O_LIGHT			0xF503
#define DISP_CTR_TIME		    2		    // Controll Timing for Display routine
#define DISPCON_RS				0x01
#define DISPCON_RW				0x02
#define DISPCON_E				0x04

#define INI			0x01
#define CMD			0x00
#define DAT			0x02

#define DISP_CLEAR		0x01			// cmd: clear display command
#define DISP_HOME		0x02			// cmd: return cursor to home
#define DISP_ENTRY_MODE_SET	0x04			// cmd: enable cursor moving direction and enable the shift of entire display
#define DISP_S_FLAG		0x01
#define DISP_ID_FLAG		0x02

#define DISP_CONTROL		0x08			// cmd: display ON/OFF
#define DISP_D_FLAG		0x04			// display on
#define DISP_C_FLAG		0x02			// cursor on
#define DISP_B_FLAG		0x01			// blinking on

#define DISP_SHIFT		0x10			// cmd: set cursor moving and display shift control bit, and the direction without changing ddram data
#define DISP_SC_FLAG		0x08
#define DISP_RL_FLAG		0x04

#define DISP_FUNCTION_SET	0x20			// cmd: set interface data length
#define DISP_DL_FLAG		0x10			// set interface data length: 8bit/4bit
#define DISP_N_FLAG		0x08			// number of display lines:2-line / 1-line
#define DISP_F_FLAG		0x04			// display font type 5x11 dots or 5x8 dots

#define DISP_RE_FLAG		0x04

#define DISP_CGRAM_SET		0x40			// cmd: set CGRAM address in address counter
#define DISP_SEGRAM_SET		0x40			// RE_FLAG = 1

#define DISP_DDRAM_SET		0x80			// cmd: set DDRAM address in address counter

class CX3LCD : public ILCD
{
public:
	CX3LCD();
	virtual ~CX3LCD(void);
	virtual void Initialize();
	virtual void Stop();
	virtual void SetBackLight(int iLight);
	virtual void SetContrast(int iContrast);
protected:
	virtual void		Process();
	virtual void SetLine(int iLine, const CStdString& strLine);
	void    DisplayInit();
	void    DisplaySetBacklight(unsigned char level) ;
    void    DisplaySetContrast(unsigned char level);
	void    DisplayProgressBar(unsigned char percent, unsigned char charcnt);
	void    DisplayClearChars(unsigned char startpos , unsigned char line, unsigned char lenght) ;
	void    DisplayWriteString(char *pointer) ;
	void    DisplayWriteFixtext(const char *textstring);
	void    DisplaySetPos(unsigned char pos, unsigned char line) ;
	void    DisplayBuildCustomChars() ;
	void    DisplayOut(unsigned char data, unsigned char command) ;
	void    wait_us(unsigned int value) ;
	unsigned int m_iColumns;			// display columns for each line
	unsigned int m_iRows;				// total number of rows
	unsigned int m_iRow1adr ;
	unsigned int m_iRow2adr ;
	unsigned int m_iRow3adr ;
	unsigned int m_iRow4adr ;
	unsigned int m_iActualpos;			// actual cursor possition
	int          m_iBackLight;
	bool         m_bUpdate[MAX_ROWS];
	CStdString   m_strLine[MAX_ROWS];
	int          m_iPos[MAX_ROWS];
	DWORD        m_dwSleep[MAX_ROWS];
	CEvent       m_event;

};

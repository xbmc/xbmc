
#include "../stdafx.h"
#include "smartxxlcd.h"
#include "../../settings.h"
#include "../../utils/log.h"
#include <xtl.h>
#include "conio.h"
#include "../../util.h"

/*
HD44780 or equivalent
Character located  1   2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 
DDRAM address      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13
DDRAM address      40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53
DDRAM address      14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27
DDRAM address      54 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67
*/

#define SCROLL_SPEED_IN_MSEC 250
#define DISP_O			        0xF700		// Display Port
#define DISP_O_LIGHT			  0xF701		// Display Port brightness control
#define DISP_CTR_TIME		    2		      // Controll Timing for Display routine

#define DISPCON_RS		      0x02		  // some Display definitions
#define DISPCON_E		        0x04

#define INI			            0x01
#define CMD			            0x00
#define DAT			            0x02

#define DISP_CLEAR		      0x01 // cmd: clear display command
#define DISP_HOME		        0x02 // cmd: return cursor to home
#define DISP_ENTRY_MODE_SET	0x04 // cmd: enable cursor moving direction and enable the shift of entire display
  #define DISP_S_FLAG		      0x01
  #define DISP_ID_FLAG		    0x02

#define DISP_CONTROL		    0x08	//cmd: display ON/OFF
  #define DISP_D_FLAG		      0x04	// display on
  #define DISP_C_FLAG		      0x02	// cursor on
  #define DISP_B_FLAG		      0x01	// blinking on

#define DISP_EXT_CONTROL	  0x08	//RE_FLAG = 1
#define DISP_FW_FLAG		    0x04	//RE_FLAG = 1
#define DISP_BW_FLAG		    0x02	//RE_FLAG = 1
#define DISP_NW_FLAG		    0x01	//RE_FLAG = 1

#define DISP_SHIFT		      0x10  //cmd: set cursor moving and display shift control bit, and the direction without changing of ddram data
  #define DISP_SC_FLAG		    0x08
  #define DISP_RL_FLAG		    0x04

#define DISP_FUNCTION_SET	  0x20 // cmd: set interface data length
  #define DISP_DL_FLAG		    0x10  // set interface data length: 8bit/4bit
  #define DISP_N_FLAG		      0x08  // number of display lines:2-line / 1-line
  #define DISP_F_FLAG		      0x04  // display font type 5x11 dots or 5x8 dots

  #define DISP_RE_FLAG		    0x04

#define DISP_CGRAM_SET		  0x40 // cmd: set CGRAM address in address counter
  #define DISP_SEGRAM_SET		  0x40	//RE_FLAG = 1

#define DISP_DDRAM_SET		  0x80 // cmd: set DDRAM address in address counter

char AnimIndex=0;

//*************************************************************************************************************
static void outb(unsigned short port, unsigned char data)
{
  __asm
  {
    nop
    mov dx, port
    nop
    mov al, data
    nop
    out dx,al
    nop
    nop
  }
}

//*************************************************************************************************************
static unsigned char inb(unsigned short port)
{
  unsigned char data;
  __asm
  {
    mov dx, port
    in al, dx
    mov data,al
  }
  return data;
}

//*************************************************************************************************************
CSmartXXLCD::CSmartXXLCD()
{
  m_iActualpos=0;
  m_iRows    = 4;
  m_iColumns = 20;				// display rows each line
  m_iRow1adr = 0x00;
  m_iRow2adr = 0x20;
  m_iRow3adr = 0x40;
  m_iRow4adr = 0x60;

  m_iRow1adr = 0x00;
  m_iRow2adr = 0x40;
  m_iRow3adr = 0x14;
  m_iRow4adr = 0x54;
  m_iBackLight=32;
}

//*************************************************************************************************************
CSmartXXLCD::~CSmartXXLCD()
{
}

//*************************************************************************************************************
void CSmartXXLCD::Initialize()
{
  StopThread();
  if (!g_stSettings.m_bLCDUsed) 
  {
    CLog::Log("lcd not used");
    return;
  }
  Create();
  
}
void CSmartXXLCD::SetBackLight(int iLight)
{
  m_iBackLight=iLight;
}

//*************************************************************************************************************
void CSmartXXLCD::Stop()
{
  if (!g_stSettings.m_bLCDUsed) return;
  StopThread();
}

//*************************************************************************************************************
void CSmartXXLCD::SetLine(int iLine, const CStdString& strLine)
{
  if (!g_stSettings.m_bLCDUsed) return;
  if (iLine < 0 || iLine >= (int)m_iRows) return;
  
  CStdString strLineLong=strLine;
  strLineLong.Trim();
	StringToLCDCharSet(strLineLong);

  while (strLineLong.size() < m_iColumns) strLineLong+=" ";
  if (strLineLong != m_strLine[iLine])
  {
//    CLog::Log("set line:%i [%s]", iLine,strLineLong.c_str());
    m_bUpdate[iLine]=true;
    m_strLine[iLine]=strLineLong;
    m_event.Set();
  }
}


//************************************************************************************************************************
// wait_us: delay routine
// Input: (wait in ~us)
//************************************************************************************************************************
void CSmartXXLCD::wait_us(unsigned int value) 
{
  // 1 us = 1000msec
  int iValue=value/30;
  if (iValue>10) iValue=10;
  if (iValue) Sleep(iValue);
}


//************************************************************************************************************************
// DisplayOut: writes command or datas to display
// Input: (Value to write, token as CMD = Command / DAT = DATAs / INI for switching to 4 bit mode)
//************************************************************************************************************************
void CSmartXXLCD::DisplayOut(unsigned char data, unsigned char command) 
{
	unsigned char odathigh;
	unsigned char odatlow;
  static DWORD dwTick=0;

  if ((GetTickCount()-dwTick) < 3)
  {
    Sleep(1);
  }
  dwTick=GetTickCount();

	// Data bit's mapping for higher nibble outbut
	odathigh  = (data >> 2) & 0x28;		// outD7 => PortD5 => DisplayD7  //outD5 => PortD3 => DisplayD5
	odathigh |= (data >> 0) & 0x50;		// outD6 => PortD6 => DisplayD6  //outD4 => PortD4 => DisplayD4		
	
  // Data bit's mapping for lower nibble outbut
	odatlow  = (data << 2) & 0x28;	 // outD3 => PortD5 => DisplayD7  //outD1 => PortD3 => DisplayD5
	odatlow |= (data << 4) & 0x50;	 // outD2 => PortD6 => DisplayD6  //outD0 => PortD4 => DisplayD4
	
	//outbut higher nibble
	outb(DISP_O, (command & DISPCON_RS) | odathigh);					    // set the RS if needed
	outb(DISP_O,((command & DISPCON_RS) | DISPCON_E | odathigh));	// set E
	outb(DISP_O,((command & DISPCON_RS) | odathigh));			        // reset E


	if ((command & INI) == 0) 
  {							
    // if it's not the init command, do second nibble
		//wait_us(DISP_CTR_TIME * 21);
		//outbut lower nibble
		outb(DISP_O,((command & DISPCON_RS) | odatlow));		          // set E
		outb(DISP_O,((command & DISPCON_RS) | DISPCON_E | odatlow));	// set Data
		outb(DISP_O,((command & DISPCON_RS) | odatlow));		          // reset E
		if ((data & 0xFC) == 0) 
    {						                          
      // if command was a Clear Display
      // or Cursor Home, wait at least 1.5 ms more
			m_iActualpos = 0;
			Sleep(2);							                                   
		}
		if ((command & DISPCON_RS) == 0x02) m_iActualpos++;
	}
	else 
  {										
		m_iActualpos = 0;
    
		//wait_us(2500);		// wait 2.5 msec	
	}		
 }

//************************************************************************************************************************
//DisplayBuildCustomChars: load customized characters to character ram of display, resets cursor to pos 0
//************************************************************************************************************************
void CSmartXXLCD::DisplayBuildCustomChars() 
{
	int I;
	
	static char Bar0[] ={0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
	static char Bar1[] ={0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
	static char Bar2[] ={0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c};
	static char REW[8][8]=
	{
		{0x00, 0x05, 0x0a, 0x14, 0x0a, 0x05, 0x00, 0x00},
		{0x00, 0x0a, 0x14, 0x08, 0x14, 0x0a, 0x00, 0x00},
		{0x00, 0x14, 0x08, 0x10, 0x08, 0x14, 0x00, 0x00},
		{0x00, 0x08, 0x10, 0x00, 0x10, 0x08, 0x00, 0x00},
		{0x00, 0x10, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00},
		{0x00, 0x00, 0x01, 0x02, 0x01, 0x00, 0x00, 0x00},
		{0x00, 0x01, 0x02, 0x05, 0x02, 0x01, 0x00, 0x00},
		{0x00, 0x02, 0x05, 0x0a, 0x05, 0x02, 0x00, 0x00}

	};
	static char FF[8][8]=
	{
		{0x00, 0x14, 0x0a, 0x05, 0x0a, 0x14, 0x00, 0x00},
		{0x00, 0x0a, 0x05, 0x02, 0x05, 0x0a, 0x00, 0x00},
		{0x00, 0x05, 0x02, 0x01, 0x02, 0x05, 0x00, 0x00},
		{0x00, 0x02, 0x01, 0x00, 0x01, 0x02, 0x00, 0x00},
		{0x00, 0x01, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00},
		{0x00, 0x00, 0x10, 0x08, 0x10, 0x00, 0x00, 0x00},
		{0x00, 0x10, 0x08, 0x14, 0x08, 0x10, 0x00, 0x00},
		{0x00, 0x08, 0x14, 0x0a, 0x14, 0x08, 0x00, 0x00}
	};
	static char Play[] ={0x00, 0x04, 0x06, 0x07, 0x07, 0x06, 0x04, 0x00};
	static char Stop[] ={0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x00};
	static char Pause[]={0x00, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x00};
	
	DisplayOut(DISP_CGRAM_SET, CMD);
	 for(I=0;I<8;I++) DisplayOut(Bar0[I], DAT);  // Bar0
	 for(I=0;I<8;I++) DisplayOut(Bar1[I], DAT);  // Bar1
	 for(I=0;I<8;I++) DisplayOut(Bar2[I], DAT);  // Bar2
	 for(I=0;I<8;I++) DisplayOut(REW[AnimIndex][I], DAT);   // REW
	 for(I=0;I<8;I++) DisplayOut(FF[AnimIndex][I], DAT);    // FF
	 for(I=0;I<8;I++) DisplayOut(Play[I], DAT);  // Play
	 for(I=0;I<8;I++) DisplayOut(Stop[I], DAT);  // Stop
	 for(I=0;I<8;I++) DisplayOut(Pause[I], DAT); // Pause
	DisplayOut(DISP_DDRAM_SET, CMD);
    AnimIndex=(AnimIndex+1) & 0x7;
}


//************************************************************************************************************************
// DisplaySetPos: sets cursor position
// Input: (row position, line number from 0 to 3)
//************************************************************************************************************************
void CSmartXXLCD::DisplaySetPos(unsigned char pos, unsigned char line) 
{
	
	unsigned int cursorpointer;
	
	cursorpointer = pos % m_iColumns;

	if (line == 0) {
		cursorpointer += m_iRow1adr;
	}
	if (line == 1) {
		cursorpointer += m_iRow2adr;
	}
	if (line == 2) {
		cursorpointer += m_iRow3adr;
	}
	if (line == 3) {
		cursorpointer += m_iRow4adr;
	}
	DisplayOut(DISP_DDRAM_SET | cursorpointer, CMD);
	m_iActualpos = cursorpointer;
}

//************************************************************************************************************************
// DisplayWriteFixText: write a fixed text to actual cursor position
// Input: ("fixed text like")
//************************************************************************************************************************
void CSmartXXLCD::DisplayWriteFixtext(const char *textstring)
{
	unsigned char  c;
	while (c = *textstring++) {
		DisplayOut(c,DAT);
	}
}


//************************************************************************************************************************
// DisplayWriteString: write a string to acutal cursor position 
// Input: (pointer to a 0x00 terminated string)
//************************************************************************************************************************

void CSmartXXLCD::DisplayWriteString(char *pointer) 
{
	/* display a normal 0x00 terminated string on the LCD display */
	unsigned char c;
	do {
		c = *pointer;
		if (c == 0x00) 
			break;
    
		DisplayOut(c, DAT);
		*pointer++;
  	} while(1);
 
}		


//************************************************************************************************************************
// DisplayClearChars:  clears a number of chars in a line and resets cursor position to it's startposition
// Input: (Startposition of clear in row, row number, number of chars to clear)
//************************************************************************************************************************
void CSmartXXLCD::DisplayClearChars(unsigned char startpos , unsigned char line, unsigned char lenght) 
{
	int i;
	
	DisplaySetPos(startpos,line);
	for (i=0;i<lenght; i++){
		DisplayOut(0x20,DAT);
	}
	DisplaySetPos(startpos,line);
  
}


//************************************************************************************************************************
// DisplayProgressBar: shows a grafic bar staring at actual cursor position
// Input: (percent of bar to display, lenght of whole bar in chars when 100 %)
//************************************************************************************************************************
void CSmartXXLCD::DisplayProgressBar(unsigned char percent, unsigned char charcnt) 
{
		
	unsigned int barcnt100;
	
	barcnt100 = charcnt * 5 * percent / 100;

	int i;	
	for (i=1; i<=charcnt; i++) {
		if (i<=(int)(barcnt100 / 5)){
			DisplayOut(4,DAT);
		}
		else 
		{
			if ( (i == (barcnt100 /5)+1) && (barcnt100 % 5 != 0) ){
	 			DisplayOut((barcnt100 % 5)-1,DAT);
			}
			else{
				DisplayOut(0x20,DAT);
			}
		}
	}
}
//************************************************************************************************************************
//Set brightness level 
//************************************************************************************************************************
void CSmartXXLCD::DisplaySetBacklight(unsigned char level) 
{
  if (g_stSettings.m_iLCDType==LCD_MODE_TYPE_LCD)
  {
    float fBackLight=((float)level)/100.0f;
    fBackLight*=63.0f;
    int iNewLevel=(int)fBackLight;
    if (iNewLevel==31) iNewLevel=32;
    outb(DISP_O_LIGHT, iNewLevel&63);
  }

  if (g_stSettings.m_iLCDType==LCD_MODE_TYPE_VFD)
  {
    //VFD:(value 0 to 3 = 100%, 75%, 50%, 25%)
    if (level<0) level=0;
    if (level>99) level=99;
	  level = (99-level);
    level/=25;
	  DisplayOut(DISP_FUNCTION_SET | DISP_N_FLAG | level,CMD);
  }
}
//************************************************************************************************************************
void CSmartXXLCD::DisplayInit()
{
	outb(DISP_O,0);
	DisplayOut(DISP_FUNCTION_SET | DISP_DL_FLAG, INI);	              // 8-Bit Datalenght if display is already initialized to 4 bit
	Sleep(5);
  DisplayOut(DISP_FUNCTION_SET | DISP_DL_FLAG, INI);	              // 8-Bit Datalenght if display is already initialized to 4 bit
	Sleep(5);
  DisplayOut(DISP_FUNCTION_SET | DISP_DL_FLAG, INI);	              // 8-Bit Datalenght if display is already initialized to 4 bit
	Sleep(5);
	DisplayOut(DISP_FUNCTION_SET,INI);			                          // 4-Bit Datalenght
	Sleep(5);
	DisplayOut(DISP_FUNCTION_SET | DISP_N_FLAG | DISP_RE_FLAG ,CMD);	// 4-Bit Datalenght, 2 Lines, Font 5x7, and set RE Flag
	Sleep(5);
	DisplayOut(DISP_SEGRAM_SET, CMD);			                            // set SEGRAM to 0x00 (RE = 1)
	Sleep(5);
	DisplayOut(DISP_EXT_CONTROL | DISP_NW_FLAG ,CMD);	                // set Display to 2 lines (RE = 1)
	Sleep(5);
	DisplayOut(DISP_FUNCTION_SET | DISP_N_FLAG ,CMD);	                // 4-Bit Datalenght, 2 Lines, Font 5x7, clear RE Flag
	Sleep(5);
	DisplayOut(DISP_CONTROL | DISP_D_FLAG ,CMD);		                  // Display on
	Sleep(5);
	DisplayOut(DISP_CLEAR,CMD);				                                // Display clear
	Sleep(5);
	DisplayOut(DISP_ENTRY_MODE_SET | DISP_ID_FLAG,CMD);	              // Cursor autoincrement
	Sleep(5);

  DisplayBuildCustomChars();				
  DisplaySetPos(0,0);
}

//************************************************************************************************************************
void CSmartXXLCD::Process()
{
  int iOldLight=-1;  

  
  m_iColumns = g_stSettings.m_iLCDColumns;
  m_iRows    = g_stSettings.m_iLCDRows;
  m_iRow1adr = g_stSettings.m_iLCDAdress[0];
  m_iRow2adr = g_stSettings.m_iLCDAdress[1];
  m_iRow3adr = g_stSettings.m_iLCDAdress[2];
  m_iRow4adr = g_stSettings.m_iLCDAdress[3];
  m_iBackLight= g_stSettings.m_iLCDBackLight;
  if (m_iRows >= MAX_ROWS) m_iRows=MAX_ROWS-1;

  DisplayInit();
  while (!m_bStop)
  {
    Sleep(SCROLL_SPEED_IN_MSEC);  
    if (m_iBackLight != iOldLight)
    {
      // backlight setting changed
      iOldLight=m_iBackLight;
      DisplaySetBacklight(m_iBackLight);
    }
	  DisplayBuildCustomChars();
	  for (int iLine=0; iLine < (int)m_iRows; ++iLine)
    {
	    if (m_bUpdate[iLine])
			{
        CStdString strTmp=m_strLine[iLine];
        if (strTmp.size() > m_iColumns)
        {
          strTmp=m_strLine[iLine].Left(m_iColumns);
        }
        m_iPos[iLine]=0;
        DisplaySetPos(0,iLine);
        DisplayWriteFixtext(strTmp.c_str());
        m_bUpdate[iLine]=false;
        m_dwSleep[iLine]=GetTickCount();
      }
      else if ( (GetTickCount()-m_dwSleep[iLine]) > 1000)
      {
        int iSize=m_strLine[iLine].size();
        if (iSize > (int)m_iColumns)
        {
          //scroll line
          CStdString strRow=m_strLine[iLine]+"   -   ";
          int iSize=strRow.size();
          m_iPos[iLine]++;
          if (m_iPos[iLine]>=iSize) m_iPos[iLine]=0;
          int iPos=m_iPos[iLine];
          CStdString strLine="";
          for (int iCol=0; iCol < (int)m_iColumns;++iCol)
          {
            strLine +=strRow.GetAt(iPos);
            iPos++;
            if (iPos >= iSize) iPos=0;
          }
          DisplaySetPos(0,iLine);
          DisplayWriteFixtext(strLine.c_str());
        }
      }
    }
  }
  for (int i=0; i < (int)m_iRows; i++)
  {
	  DisplayClearChars(0,i,m_iColumns);
  } 
  DisplayOut(DISP_CONTROL ,CMD);		                  // Display off
}
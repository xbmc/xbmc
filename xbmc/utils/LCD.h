#pragma once
#include "stdstring.h"
using namespace std;

class CLCD
{
public:
  CLCD();
  virtual ~CLCD(void);
  static void Initialize();
  static void SetLine(int iLine, const CStdString& strLine);
protected:
  void    DisplayProgressBar(unsigned char percent, unsigned char charcnt);
  void    DisplayClearChars(unsigned char startpos , unsigned char line, unsigned char lenght) ;
  void    DisplayWriteString(char *pointer) ;
  void    DisplayWriteFixtext(const char *textstring);
  void    DisplaySetPos(unsigned char pos, unsigned char line) ;
  void    DisplayBuildCustomChars() ;
  void    DisplayOut(unsigned char data, unsigned char command) ;
  void    wait_us(unsigned int value) ;
  unsigned int m_iColumns;				// display rows each line
  unsigned int m_iRow1adr ;
  unsigned int m_iRow2adr ;
  unsigned int m_iRow3adr ;
  unsigned int m_iRow4adr ;
  unsigned int m_iActualpos;				// actual cursor possition

};
extern CLCD g_lcd;
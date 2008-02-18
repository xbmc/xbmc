#pragma once
#include <SDL/SDL_thread.h>
#include "../utils/LCD.h"

#define MAX_ROWS 20

class XLCDproc : public ILCD
{
public:
  XLCDproc();
  virtual ~XLCDproc(void);
  virtual void Initialize();
  virtual void Stop();
  virtual void SetBackLight(int iLight);
  virtual void SetContrast(int iContrast);
protected:
  virtual void Process();
  virtual void SetLine(int iLine, const CStdString& strLine);
  unsigned int m_iColumns;				// display columns for each line
  unsigned int m_iRows;				// total number of rows
  unsigned int m_iRow1adr ;
  unsigned int m_iRow2adr ;
  unsigned int m_iRow3adr ;
  unsigned int m_iRow4adr ;
  unsigned int m_iActualpos;				// actual cursor possition
  int          m_iBackLight;
  int          m_iLCDContrast;
  bool         m_bUpdate[MAX_ROWS];
  CStdString   m_strLine[MAX_ROWS];
  int          m_iPos[MAX_ROWS];
  DWORD        m_dwSleep[MAX_ROWS];
  CEvent       m_event;
  int 	       sockfd;

};

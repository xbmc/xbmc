
#include "stdafx.h"
#include "LCDFactory.h"
#ifdef _XBOX
#include "../lib/smartxx/smartxxLCD.h"
#include "../lib/libXenium/XeniumLCD.h"
#include "../lib/x3lcd/x3lcd.h"
#else
#include "../linux/XLCDproc.h"
#endif

ILCD* g_lcd = NULL;
CLCDFactory::CLCDFactory(void)
{}

CLCDFactory::~CLCDFactory(void)
{}

ILCD* CLCDFactory::Create()
{
#ifdef _XBOX
  switch (g_guiSettings.GetInt("lcd.modchip"))
  {
  case MODCHIP_XENIUM:
    return new CXeniumLCD();
    break;

  case MODCHIP_SMARTXX:
    return new CSmartXXLCD();
    break;

  case MODCHIP_XECUTER3:
    return new CX3LCD();
    break;
  }
  return new CSmartXXLCD();
#endif
#ifdef _LINUX
  return new XLCDproc(); 
#endif
}

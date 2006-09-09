/*
XBOX Front LED control
OFF/Green/Red/Orange/Cycle
10.11.2004 GeminiServer
*/
#include "../stdafx.h"
#include "LED.h"
#include "SystemInfo.h"
#include "../xbox/XKUtils.h"
CSysInfo* t_sysinfo = new CSysInfo();

void ILED::CLEDControl(int ixLED)
{
  if (ixLED == LED_COLOUR_OFF)
  {
    XKUtils::SetXBOXLEDStatus(0);
	  //t_sysinfo.SmartXXLEDControll(SMARTXX_LED_OFF);
  }
  else if (ixLED == LED_COLOUR_GREEN)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_GREEN | XKUtils::LED_REGISTER_CYCLE3_GREEN);
	  //t_sysinfo.SmartXXLEDControll(SMARTXX_LED_BLUE);
  }
  else if (ixLED == LED_COLOUR_RED)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_RED | XKUtils::LED_REGISTER_CYCLE2_RED | XKUtils::LED_REGISTER_CYCLE1_RED | XKUtils::LED_REGISTER_CYCLE3_RED);
	  //t_sysinfo.SmartXXLEDControll(SMARTXX_LED_RED);
  }
  else if (ixLED == LED_COLOUR_ORANGE)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_ORANGE | XKUtils::LED_REGISTER_CYCLE2_ORANGE | XKUtils::LED_REGISTER_CYCLE1_ORANGE | XKUtils::LED_REGISTER_CYCLE3_ORANGE);
	  //t_sysinfo.SmartXXLEDControll(SMARTXX_LED_BLUE_RED);
  }
  else if (ixLED == LED_COLOUR_CYCLE)
  {
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_ORANGE | XKUtils::LED_REGISTER_CYCLE3_RED);
	  //t_sysinfo.SmartXXLEDControll(SMARTXX_LED_CYCLE);
  }
  else if (ixLED == LED_COLOUR_NO_CHANGE) //Default Bios Settings
  {
    //No need to Change! Leave the LED with the Default Bios Settings
    //Since we can't get the BIOS LED Color Setting: we will set it to Standart Green!
    XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_GREEN | XKUtils::LED_REGISTER_CYCLE3_GREEN);
  }
}


ILEDSmartxxRGB::ILEDSmartxxRGB()
{
  if(mapc_rgb.size()>0)
    mapc_rgb.clear();
	InitializeCriticalSection(&m_criticalSection);
	strCurrentStatus = "NULL";
	strLastStatus = "NULL";
  
  s_RGBs.strTransition = "NULL";
	s_CurRGB.red = 0;
	s_CurRGB.green = 0;
	s_CurRGB.blue = 0;
	
  dwLastTime = 0;
	iSleepTime = 0;
}

ILEDSmartxxRGB::~ILEDSmartxxRGB()
{
	mapc_rgb.clear();
	DeleteCriticalSection(&m_criticalSection);
}
void ILEDSmartxxRGB::OnStartup()
{
  if (t_sysinfo->SmartXXModCHIP().Equals("SmartXX V3") || t_sysinfo->SmartXXModCHIP().Equals("SmartXX OPX"))
  {
	  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
	  CLog::Log(LOGDEBUG,"Starting SmartXX RGB LED thread");
    outb(SMARTXX_PWM_STATUS, 0x0);  // Status LED OFF
	  SetRGBStatus("general");
  }
}

void ILEDSmartxxRGB::Process()
{
  bool bSettingsRGBTrue = true; // settings
	while(!m_bStop && bSettingsRGBTrue)
	{
		dwFrameTime = timeGetTime() - dwLastTime;

    if( (s_RGBs.strTransition.Equals("none") || s_RGBs.strTransition.IsEmpty()) && !strLastTransition.Equals("none"))
		{
			strLastTransition = "none";
			SetRGBLed(s_RGBs.red1,s_RGBs.green1,s_RGBs.blue1);
			iSleepTime = 200;
		}
    else if(s_RGBs.strTransition.Equals("switch") && !strLastTransition.Equals("switch"))
		{
			strLastTransition = "switch";
			if(dwFrameTime >= s_RGBs.iTime)
			{
				s_CurRGB.red = (s_CurRGB.red != s_RGBs.red1) ? s_RGBs.red1 : s_RGBs.red2;
				s_CurRGB.green = (s_CurRGB.green != s_RGBs.green1) ? s_RGBs.green1 : s_RGBs.green2;
        s_CurRGB.blue = (s_CurRGB.blue != s_RGBs.blue1) ? s_RGBs.blue1 : s_RGBs.blue2;	
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
			}
			else
				iSleepTime = s_RGBs.iTime - dwFrameTime;
		}
    else if(s_RGBs.strTransition.Equals("blink"))
		{
			strLastTransition = "blink";
			if(dwFrameTime >= s_RGBs.iTime )
			{
				s_CurRGB.red = (s_CurRGB.red != s_RGBs.red1) ? s_RGBs.red1 : s_RGBs.red2;
				s_CurRGB.green = (s_CurRGB.green != s_RGBs.green1) ? s_RGBs.green1 : s_RGBs.green2;
        s_CurRGB.blue = (s_CurRGB.blue != s_RGBs.blue1) ? s_RGBs.blue1 : s_RGBs.blue2;	
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
			}
			else
				iSleepTime = s_RGBs.iTime - dwFrameTime;

		}
    else if(s_RGBs.strTransition.Equals("fade"))
		{
      if(!strLastTransition.Equals("fade"))
			{
				s_CurRGB.red = s_RGBs.red1;
				s_CurRGB.green = s_RGBs.green1;
				s_CurRGB.blue = s_RGBs.blue1;
				strLastTransition = "fade";
			}

			if(dwFrameTime >= s_RGBs.iTime )
			{
				if(s_CurRGB.red > s_RGBs.red2 )
					s_CurRGB.red--;
				else if(s_CurRGB.red < s_RGBs.red2)
					s_CurRGB.red++;
        
				if(s_CurRGB.green > s_RGBs.green2)
					s_CurRGB.green--;
				else if(s_CurRGB.green < s_RGBs.green2)
					s_CurRGB.green++;
        
				if(s_CurRGB.blue > s_RGBs.blue2)
					s_CurRGB.blue--;
				else if(s_CurRGB.blue < s_RGBs.blue2)
					s_CurRGB.blue++;
        
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
			}
			else
				iSleepTime = s_RGBs.iTime - dwFrameTime;
			
		}
    else if(s_RGBs.strTransition.Equals("fadeloop"))
		{
      if(!strLastTransition.Equals("fadeloop"))
			{
				s_CurRGB.red = s_RGBs.red1;
				s_CurRGB.green = s_RGBs.green1;
				s_CurRGB.blue = s_RGBs.blue1;
				strLastTransition = "fadeloop";
			}

			if(dwFrameTime >= s_RGBs.iTime )
			{
				if(s_CurRGB.red > s_RGBs.red2 )
					s_CurRGB.red--;
				else if(s_CurRGB.red < s_RGBs.red2)
					s_CurRGB.red++;
        
				if(s_CurRGB.green > s_RGBs.green2)
					s_CurRGB.green--;
				else if(s_CurRGB.green < s_RGBs.green2)
					s_CurRGB.green++;
        
				if(s_CurRGB.blue > s_RGBs.blue2)
					s_CurRGB.blue--;
				else if(s_CurRGB.blue < s_RGBs.blue2)
					s_CurRGB.blue++;

        if (s_CurRGB.red == s_RGBs.red2 && s_CurRGB.green == s_RGBs.green2 && s_CurRGB.blue == s_RGBs.blue2)
        { //reset for loop
          strLastTransition.clear();
        }
        
				dwLastTime = timeGetTime();
				SetRGBLed(s_CurRGB.red,s_CurRGB.green,s_CurRGB.blue);
			}
			else
				iSleepTime = s_RGBs.iTime - dwFrameTime;
      
			
		}
		if(iSleepTime < 0)
			iSleepTime = 0;
		Sleep(iSleepTime);
	}
}

void ILEDSmartxxRGB::OnExit()
{
	SetRGBLed(0,0,0);
	CLog::Log(LOGDEBUG,"Stopping SmartXX RGB LED thread");
}

bool ILEDSmartxxRGB::Start()
{
  if (t_sysinfo->SmartXXModCHIP().Equals("SmartXX V3") || t_sysinfo->SmartXXModCHIP().Equals("SmartXX OPX"))
  {
    Create();
    return true;
  }
  else return false;
  
}
void ILEDSmartxxRGB::Stop()
{
  StopThread();
}
bool ILEDSmartxxRGB::IsRunning()
{
  return (m_ThreadHandle != NULL);
}
void ILEDSmartxxRGB::getRGBValues(CStdString strRGBa,CStdString strRGBb,RGBVALUES* s_rgb)
{
	DWORD red=0,green=0,blue=0;
	int ret = sscanf(strRGBa.c_str(),"#%2X%2X%2X",&red,&green,&blue); 
	if(ret == 3)
	{
		s_rgb->red1 = int(red/2);
		s_rgb->green1 = int(green/2);
		s_rgb->blue1 = int(blue/2);
	}
	else
	{
		s_rgb->red1 = 0;
		s_rgb->green1 = 0;
		s_rgb->blue1 = 0;
	}

	ret = sscanf(strRGBb.c_str(),"#%2X%2X%2X",&red,&green,&blue);
	if(ret == 3)
	{
		s_rgb->red2 = int(red/2);
		s_rgb->green2 = int(green/2);
		s_rgb->blue2 = int(blue/2);
	}
	else
	{
		s_rgb->red2 = 0;
		s_rgb->green2 = 0;
		s_rgb->blue2 = 0;
	}
}

void ILEDSmartxxRGB::SetRGBStatus(CStdString strStatus)
{
  map<CStdString,RGBVALUES>::iterator ikey;

	ikey = mapc_rgb.find(strStatus);
	if(ikey != mapc_rgb.end())
	{
		EnterCriticalSection(&m_criticalSection);
		strLastStatus = strCurrentStatus;
		strCurrentStatus = strStatus;
		s_RGBs = ikey->second;
		LeaveCriticalSection(&m_criticalSection);
	}
	return;
}

void ILEDSmartxxRGB::SetLastRGBStatus()
{
	SetRGBStatus(strLastStatus);
}


void ILEDSmartxxRGB::SetRGBLed(int red, int green, int blue)
{
	//CLog::Log(LOGDEBUG,"Setting SmartXX RGB LED to %s (%d,%d,%d) %d",strCurrentStatus.c_str(),red,green,blue,timeGetTime() );
  /*
  char buf[1024];
  sprintf(buf,"Setting SmartXX RGB LED to %s (%d,%d,%d) %d\n",strCurrentStatus.c_str(),red,green,blue,timeGetTime() );
  OutputDebugString(buf);
  */

	outb(SMARTXX_PWD_RED,red);
	outb(SMARTXX_PWD_GREEN,green); 
	outb(SMARTXX_PWD_BLUE,blue);
  
  // Front LED 
  //if(red  > 50 )XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_RED | XKUtils::LED_REGISTER_CYCLE2_RED | XKUtils::LED_REGISTER_CYCLE1_RED | XKUtils::LED_REGISTER_CYCLE3_RED);
  //if(green> 50 )XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_GREEN | XKUtils::LED_REGISTER_CYCLE2_GREEN | XKUtils::LED_REGISTER_CYCLE1_GREEN | XKUtils::LED_REGISTER_CYCLE3_GREEN);
  //if(blue > 50 )XKUtils::SetXBOXLEDStatus(XKUtils::LED_REGISTER_CYCLE0_ORANGE | XKUtils::LED_REGISTER_CYCLE2_ORANGE | XKUtils::LED_REGISTER_CYCLE1_ORANGE | XKUtils::LED_REGISTER_CYCLE3_ORANGE);
}


//strRGB1: from rgb state in form: #rrggbb
//strRGB2: to rgb state in form: #rrggbb
//strTransition: "none", "blink", "fade"
//iTranTime: int time between transitions e.g. 50
bool ILEDSmartxxRGB::SetRGBState(CStdString strRGB1, CStdString strRGB2, CStdString strTransition, int iTranTime)
{
  CStdString	strValue;
	RGBVALUES	s_rgb;
  getRGBValues(strRGB1,strRGB2,&s_rgb);
	
  if(!strTransition.Equals("none") || !strTransition.IsEmpty() )
    s_rgb.strTransition = strTransition.c_str();
	else
		s_rgb.strTransition = "none";

  if (iTranTime > 0)
    s_rgb.iTime = iTranTime;
	else
		s_rgb.iTime = 0;

	mapc_rgb.insert(std::pair<CStdString,RGBVALUES>(strTransition,s_rgb));
  SetRGBStatus(strTransition);

	return true;
}
/*
Module : SNTP.H
Purpose: Interface for a MFC class to encapsulate the SNTP protocol
Created: PJN / 05-08-1998


Copyright (c) 1998 - 2003 by PJ Naughter.  (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 

*/

#ifndef __SNTP_H__
#define __SNTP_H__

#include "Thread.h" 
///////////////////////////////// Classes //////////////////////////////

//Representation of an NTP timestamp
struct CNtpTimePacket
{
  DWORD m_dwInteger;
  DWORD m_dwFractional;
};


//Helper class to encapulate NTP time stamps
class CNtpTime
{
public:
  //Constructors / Destructors
  CNtpTime();
  CNtpTime(const CNtpTime& time);
  CNtpTime(CNtpTimePacket& packet);
  CNtpTime(const SYSTEMTIME& st);

  //General functions
  CNtpTime& operator=(const CNtpTime& time);
  double operator-(const CNtpTime& time) const;
  CNtpTime operator+(const double& timespan) const;
  operator SYSTEMTIME() const;
  operator CNtpTimePacket() const;
  operator unsigned __int64() const { return m_Time; };
  DWORD Seconds() const;
  DWORD Fraction() const;

  //Static functions
  static CNtpTime GetCurrentTime();
  static DWORD MsToNtpFraction(WORD wMilliSeconds);
  static WORD NtpFractionToMs(DWORD dwFraction);
  static double NtpFractionToSecond(DWORD dwFraction);

protected:
  //Internal static functions and data
  static long GetJulianDay(WORD Year, WORD Month, WORD Day);
  static void GetGregorianDate(long JD, WORD& Year, WORD& Month, WORD& Day);
  static DWORD m_MsToNTP[1000];

  //The actual data
  unsigned __int64 m_Time;
};


struct NtpServerResponse
{
  int m_nLeapIndicator; //0: no warning
  //1: last minute in day has 61 seconds
  //2: last minute has 59 seconds
  //3: clock not synchronized

  int m_nStratum; //0: unspecified or unavailable
  //1: primary reference (e.g., radio clock)
  //2-15: secondary reference (via NTP or SNTP)
  //16-255: reserved

  CNtpTime m_OriginateTime;    //Time when the request was sent from the client to the SNTP server
  CNtpTime m_ReceiveTime;      //Time when the request was received by the server
  CNtpTime m_TransmitTime;     //Time when the server sent the request back to the client
  CNtpTime m_DestinationTime;  //Time when the reply was received by the client
  double m_RoundTripDelay;   //Round trip time in seconds
  double m_LocalClockOffset; //Local clock offset relative to the server
};


//The actual SNTP class
class CSNTPClient : private CThread
{
public:
  //Constructors / Destructors
  CSNTPClient();
  virtual ~CSNTPClient();
  void Update();
  bool UpdateNeeded();
protected:
  virtual void Process();

  //General functions
  BOOL GetServerTime(LPCTSTR pszHostName, NtpServerResponse& response, int nPort = 123);
  void SetTimeout(DWORD dwTimeout) { m_dwTimeout = dwTimeout; };
  BOOL SetClientTime(const CNtpTime& NewTime);


  //AR 22-07-2000
#ifndef _WIN32_WCE
  BOOL EnableSetTimePriviledge();
  void RevertSetTimePriviledge();
#endif

  // next point in time we wish to update
  DWORD m_dwTimeout;

  //AR 22-07-2000
#ifndef _WIN32_WCE
  HANDLE m_hToken;
  TOKEN_PRIVILEGES m_TokenPriv;
  BOOL m_bTakenPriviledge;
#endif
};


#endif //__SNTP_H__

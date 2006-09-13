// -----------------------------------------------------
//  HDD S.M.A.R.T Values 
//  Requesting values from HDD using the Self-Monitoring, Analysis and Reporting Technology

//  GeminiServer
// -----------------------------------------------------
//
//
// Possible request SMART Attributes: system.hddsmart(ID)
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------/*
//                        Request:    ID: Offset:	Name of attribute:		        Description:						
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------/*
//	"No Attribute Here          ", //0		0		0		No Attribute Here					        No description here		
//	"Raw Read Error Rate        ", //1		1		1		Raw Read Error Rate				        Frequency of errors appearance while reading RAW data from a disk	
//	"Throughput Performance     ", //2		2		2		Throughput Performance		        The average efficiency of hard disk	
//	"Spin Up Time               ", //3		3		3		Spin Up Time						          Time needed by spindle to spin-up	
//	"Start/Stop Count           ", //4		4		4		Start/Stop Count					        Number of start/stop cycles of spindle	
//	"Reallocated Sector Count   ", //5		5		5		Reallocated Sector Count	        Quantity of remapped sectors	
//	"Read Channel Margin        ", //6		6		6		Read Channel Margin				        Reserve of channel while reading	
//	"Seek Error Rate            ", //7		7		7		Seek Error Rate						        Frequency of errors appearance while positioning	
//	"Seek Time Performance      ", //8		8		8		Seek Time Performance			        The average efficiency of operations while positioning	
//	"Power On Hours Count       ", //9		9		9		Power-On Hours Count			        Quantity of elapsed hours in the switched-on state	
//	"Spin Retry Count           ", //10		10		A		Spin-up Retry Count			        Number of attempts to start a spindle of a disk	
//	"Calibration Retry Count    ", //11		11		B		Calibration Retry Count	        Number of attempts to calibrate a drive	
//	"Power Cycle Count          ", //12		12		C		Power Cycle Count				        Number of complete start/stop cycles of hard disk	
//	"Soft Read Error Rate       ", //13		13		D		Soft Read Error Rate		        Frequency of "program" errors appearance while reading data from a disk	
//	"G-Sense Error Rate         ", //14		191		BF		G-Sense Error Rate		        Frequency of mistakes appearance as a result of impact loads	
//	"Power-Off Retract Cycle    ", //15		192		C0		Power-Off Retract Cycle       Number of the fixed 'turning off drive' cycles (Fujitsu: Emergency Retract Cycle Count)	
//	"Load/Unload Cycle Count    ", //16		193		C1		Load/Unload Cycle Count	      Number of cycles into Landing Zone position	
//	"HDD Temperature            ", //17		194		C2		HDA Temperature				        Temperature of a Hard Disk Assembly	
//	"Hardware ECC Recovered     ", //18		195		C3		Hardware ECC Recovered        Frequency of the on the fly errors (Fujitsu: ECC On The Fly Count)	
//	"Reallocated Event Count    ", //19		196		C4		Reallocated Event Count	      Quantity of remapping operations	
//	"CurrentPending SecrCnt     ", //20		197		C5		Current Pending Sector Count	Current quantity of unstable sectors (waiting for remapping)	
//	"Off-LineUncorrectableCnt   ", //21		198		C6		Off-line Scan Uncorrectable Count	Quantity of uncorrected errors	
//	"UltraDMA CRC Error Rate    ", //22		199		C7		UltraDMA CRC Error Rate				Total quantity of errors CRC during UltraDMA mode	
//	"Write Error Rate           ", //23		200		C8		Write Error Rate					    Frequency of errors appearance while recording data into disk (Western Digital: Multi Zone Error Rate)	
//	"Soft Read Error Rate       ", //24		201		C9		Soft Read Error Rate				  Frequency of the off track errors (Maxtor: Off Track Errors)	
//	"Data Address Mark Errors   ", //25		202		CA		Data Address Mark Errors			Frequency of the Data Address Mark errors	
//	"Run Out Cancel             ", //26		203		CB		Run Out Cancel						    Frequency of the ECC errors (Maxtor: ECC Errors)	
//	"Soft ECC Correction        ", //27		204		CC		Soft ECC Correction					  Quantity of errors corrected by software ECC	
//	"Thermal Asperity Rate      ", //28		205		CD		Thermal Asperity Rate				  Frequency of the thermal asperity errors	
//	"Flying Height              ", //29		206		CE		Flying Height						      The height of the disk heads above the disk surface	
//	"Spin High Current          ", //30		207		CF		Spin High Current					    Quantity of used high current to spin up drive	
//	"Spin Buzz                  ", //31		208		D0		Spin Buzz							        Quantity of used buzz routines to spin up drive	
//	"Offline Seek Performance   ", //32		209		D1		Offline Seek Performance			Drive's seek performance during offline operations	
//	"Disk Shift                 ", //34		220		DC		Disk Shift							      Shift of disk is possible as a result of strong shock loading in the store, as a result of it's falling or for other reasons (sometimes: Temperature)	
//	"G-Sense Error              ", //35		221		DD		G-Sense Error						      Rate	This attribute is an indication of shock-sensitive sensor - total quantity of errors appearance as a result of impact loads (dropping drive, for example)	
//	"Loaded Hours               ", //36		222		DE		Loaded Hours						      Loading on drive caused by the general operating time of hours it stores	
//	"Load/Unload Retry Count    ", //37		223		DF		Load/Unload Retry Count				Loading on drive caused by numerous recurrences of operations like: reading, recording, positioning of heads, etc.	
//	"Load Friction              ", //38		224		E0		Load Friction						      Loading on drive caused by friction in mechanical parts of the store	
//	"Load/Unload Cycle Count    ", //39		225		E1		Load/Unload Cycle Count				Total of cycles of loading on drive	
//	"Load-in Time               ", //40		226		E2		Load-in Time						      General time of loading for drive	
//	"Torque Amplification Cnt   ", //41		227		E3		Torque Amplification Count		Quantity efforts of the rotating moment of a drive	
//	"Power-Off Retract Count    ", //42		228		E4		Power-Off Retract Count				Quantity of the fixed turning off's a drive	
//	"GMR Head Amplitude         ", //43		230		E6		GMR Head Amplitude					  Amplitude of heads trembling (GMR-head) in running mode	
//	"Temperature                ", //44		231		E7		Temperature							      Temperature of a drive	
//	"Head Flying Hours          ", //45		240		F0		Head Flying Hours					    Time while head is positioning	
//	"Read Error Retry Rate      "  //46		250		FA		Read Error Retry Rate				  Frequency of errors appearance while reading data from a disk	
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------/*

#include "../stdafx.h"
#include <conio.h>
#include "HddSmart.h"
BYTE retVal = 0;

CHDDSmart g_hddsmart;

/* use one global section */
CCriticalSection m_section;

CHDDSmart::CHDDSmart()
{
  m_HddSmarValue= 0;
}
CHDDSmart::~CHDDSmart()
{
}

BOOL CHDDSmart::SendATACommand(WORD IDEPort, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite)
{
  CSingleLock lock(m_section);

  // Wait Timers! No wait time.. let's see if this will also work!
  const int waitSleepTime = 0; //default 5 
  const int writeSleepTime = 0; // default 10
  const int readSleepTime = 0; // default 300

	//XBOX Sending ATA Commands..

	BOOL retVal			= FALSE;
	UCHAR waitcount		= 15;
	WORD inVal			= 0;
	WORD SuccessRet		= 0x58;
	LPDWORD PIDEDATA	= (LPDWORD) &ATACommandObj->DATA_BUFFER ;

  //Write IDE Registers to IDE Port.. and in essence Execute the ATA Command..
	_outp(IDEPort + 1, ATACommandObj->IPReg.bFeaturesReg);		
      Sleep(writeSleepTime);
	_outp(IDEPort + 2, ATACommandObj->IPReg.bSectorCountReg); 	
      Sleep(writeSleepTime);
	_outp(IDEPort + 3, ATACommandObj->IPReg.bSectorNumberReg);	
      Sleep(writeSleepTime);
	_outp(IDEPort + 4, ATACommandObj->IPReg.bCylLowReg);		
      Sleep(writeSleepTime);
	_outp(IDEPort + 5, ATACommandObj->IPReg.bCylHighReg);		
      Sleep(writeSleepTime);
	_outp(IDEPort + 6, ATACommandObj->IPReg.bDriveHeadReg);		
      Sleep(writeSleepTime);
	_outp(IDEPort + 7, ATACommandObj->IPReg.bCommandReg);		
      Sleep(readSleepTime);

	//Command Executed, Check Status.. If not success, wait a while..
	inVal = _inp(IDEPort+7); 
	while (((inVal & SuccessRet) != SuccessRet) && (waitcount > 0))
	{
		inVal = _inp(IDEPort+7); //Check Status..
		Sleep(readSleepTime);
		waitcount--;
	}

	if ((waitcount > 0) && (ReadWrite == 0x00))
	{
		//Read the command return output Registers
		ATACommandObj->OPReg.bErrorReg =		_inp(IDEPort + 1);
		ATACommandObj->OPReg.bSectorCountReg =	_inp(IDEPort + 2);
		ATACommandObj->OPReg.bSectorNumberReg =	_inp(IDEPort + 3);
		ATACommandObj->OPReg.bCylLowReg =		_inp(IDEPort + 4);
		ATACommandObj->OPReg.bCylHighReg =		_inp(IDEPort + 5);
		ATACommandObj->OPReg.bDriveHeadReg =	_inp(IDEPort + 6);
		ATACommandObj->OPReg.bStatusReg =		_inp(IDEPort + 7);

		ATACommandObj->DATA_BUFFSIZE = 512;
		Sleep(writeSleepTime);

		//Now read a sector (512 Bytes) from the IDE Port
		ZeroMemory(ATACommandObj->DATA_BUFFER, 512);
		for (int i = 0; i < 128; i++)
		{
			PIDEDATA[i] = _inpd(IDEPort);
			Sleep(waitSleepTime);
		}
		retVal = TRUE;
	}
  
	return retVal;
}
BYTE CHDDSmart::GetSmartValue(int SmartREQ)
{
  CSingleLock lock(m_section);

  ATA_COMMAND_OBJ hddcommand;
	ZeroMemory(&hddcommand, sizeof(ATA_COMMAND_OBJ));
	
	hddcommand.DATA_BUFFSIZE = 0;
	hddcommand.IPReg.bFeaturesReg		= 0xd0;	        // SEND READ SMART VALUES
	hddcommand.IPReg.bSectorCountReg	= 1;					
	hddcommand.IPReg.bSectorNumberReg	= 1;
	hddcommand.IPReg.bCylLowReg			= 0x4f;				  //SET SMART CYL LOW
	hddcommand.IPReg.bCylHighReg		= 0xc2;					//SET SMART CYL HI
	hddcommand.IPReg.bDriveHeadReg		= 0x00a0;			//SET Device Where HDD is
	hddcommand.IPReg.bCommandReg		= 0xb0;	        //SET ON IDE SEND SMART MODE;
	
  bool waitcon		= true;
  bool bSuccessRet = false;
  while (!bSuccessRet && waitcon)
	{
    waitcon = false; // run just for one time!

    if (SendATACommand(0x01f0, &hddcommand, 0x00))
    {	
	    int	i;
	    BYTE Attr;
	    PDRIVEATTRIBUTE	pDA;
	    PATTRTHRESHOLD	pAT;
	    pDA = (PDRIVEATTRIBUTE)&hddcommand.DATA_BUFFER[2];
	    pAT = (PATTRTHRESHOLD)&hddcommand.DATA_BUFFER[2];
      for (i = 0; i < 46; i++)
	    {	
        Attr = pDA->bAttrID;
        if (Attr == 191) Attr =14;	
        if (Attr == 192) Attr =15;	
        if (Attr == 193) Attr =16;
        if (Attr == 194) Attr =17;
		    if (Attr == 195) Attr =18;	
        if (Attr == 196) Attr =19;

        if (Attr == 197) Attr =20;
		    if (Attr == 198) Attr =21;
        if (Attr == 199) Attr =22;
        if (Attr == 200) Attr =23;
        if (Attr == 201) Attr =24;
		    if (Attr == 202) Attr =25;
        if (Attr == 203) Attr =26;
        if (Attr == 204) Attr =27;
        if (Attr == 205) Attr =28;
		    if (Attr == 206) Attr =29;

        if (Attr == 207) Attr =30;
        if (Attr == 208) Attr =31;
        if (Attr == 209) Attr =32;
        if (Attr == 220) Attr =34;
        if (Attr == 221) Attr =35;
        if (Attr == 222) Attr =36;
		    if (Attr == 223) Attr =37;
        if (Attr == 224) Attr =38;
        if (Attr == 225) Attr =39;

        if (Attr == 226) Attr =40;
		    if (Attr == 227) Attr =41;
        if (Attr == 228) Attr =42;
        if (Attr == 230) Attr =43;
        if (Attr == 231) Attr =44;
        if (Attr == 240) Attr =45;
        if (Attr == 250) Attr =46;

        if( (Attr < 47) && (Attr == SmartREQ))
        {
          // This is only for HDD-Temperature! ID:17! Returned: bWorstValue! Seems to be a working value on all Drives!
          if (SmartREQ == 17)
          {
            retVal = (pDA->bWorstValue);	
            bSuccessRet = true;
            CLog::Log(LOGDEBUG, "HDD S.M.A.R.T Reported Temperature: %d °C",retVal);
          }
		      if (SmartREQ != 17)
		      {
			      retVal = (pDA->bAttrValue);
            bSuccessRet = true;
            CLog::Log(LOGDEBUG, "HDD S.M.A.R.T Request Result: %d",retVal);
          }
        }
        pDA++;
		    pAT++;
      }
    }
  }
  // If the returned value is 0, return the last known Value!
  if (retVal == 0)
    retVal = m_HddSmarValue;
  else
    m_HddSmarValue = retVal;

  return retVal;
}
bool CHDDSmart::DelayRequestSmartValue(int SmartREQ , int iDelayTime)
{
  CSingleLock lock(m_section);

  static DWORD pingTimer = 0;
  if (iDelayTime <0 ) iDelayTime = 0; //60 default
  bool bWait=false;
  if( timeGetTime() - pingTimer < (DWORD)iDelayTime * 1000)
    return false;    
  else { do
  {
    GetSmartValue(SmartREQ);
    if (retVal>0)
      bWait=true;
    }while(!bWait);
  }
  pingTimer = timeGetTime();
  
  return bWait;
}
/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "stdafx.h"
#include "SystemInfo.h"
#include <conio.h>
#include "../settings.h"
#include "../utils/log.h"
#include "../cores/dllloader/dllloader.h"
#include "../utils/http.h"
#ifdef HAS_XBOX_HARDWARE
#include "../xbox/undocumented.h"
#include "../xbox/xkutils.h"
#include "../xbox/xkhdd.h"
#include "../xbox/xkflash.h"
#include "../xbox/xkrc4.h"

extern "C" XPP_DEVICE_TYPE XDEVICE_TYPE_IR_REMOTE_TABLE;
#endif
CSysInfo g_sysinfo;

void CBackgroundSystemInfoLoader::GetInformation()
{
  CSysInfo *callback = (CSysInfo *)m_callback;
#ifdef HAS_XBOX_HARDWARE
  //Request only one time!
  if(!callback->m_bRequestDone)
  {
    // The X2 series of modchips cause an error on XBE launching if GetModChipInfo()
    if(!g_advancedSettings.m_DisableModChipDetection)
    {
      callback->m_XboxModChip     = callback->GetModChipInfo();
    }
    callback->m_XboxBios        = callback->GetBIOSInfo();
    callback->m_mplayerversion  = callback->GetMPlayerVersion();
    callback->m_kernelversion   = callback->GetKernelVersion();
    callback->m_cpufrequency    = callback->GetCPUFreqInfo();
    callback->m_xboxversion     = callback->GetXBVerInfo();
    callback->m_avpackinfo      = callback->GetAVPackInfo();
    callback->m_videoencoder    = callback->GetVideoEncoder();
    callback->m_xboxserial      = callback->GetXBOXSerial(true);
    callback->m_hddlockkey      = callback->GetHDDKey();
    callback->m_macadress       = callback->GetMACAddress();
    callback->m_videoxberegion  = callback->GetVideoXBERegion();
    callback->m_videodvdzone    = callback->GetDVDZone();
    callback->m_produceinfo     = callback->GetXBProduceInfo();
    g_sysinfo.GetRefurbInfo(callback->m_hddbootdate, callback->m_hddcyclecount);

    if (callback->m_bSmartSupported && !callback->m_bSmartEnabled)
    {
      CLog::Log(LOGNOTICE, "Enabling SMART...");
      XKHDD::EnableSMART();
    }

    callback->m_bRequestDone = true;
  }
#endif
  //Request always
  callback->m_systemuptime = callback->GetSystemUpTime(false);
  callback->m_systemtotaluptime = callback->GetSystemUpTime(true);
  callback->m_InternetState = callback->GetInternetState();
#ifdef HAS_XBOX_HARDWARE
  if (!callback->m_hddRequest)
    callback->GetHDDInfo(callback->m_HDDModel, 
                          callback->m_HDDSerial,
                          callback->m_HDDFirmware,
                          callback->m_HDDpw, 
                          callback->m_HDDLockState);
  // don't check the DVD-ROM if we have already successfully retrieved its info, or it is specified
  // as not present in advancedsettings
  if (!callback->m_dvdRequest && !g_advancedSettings.m_noDVDROM)
    callback->GetDVDInfo(callback->m_DVDModel, callback->m_DVDFirmware);

  if (callback->m_bSmartEnabled)
    callback->byHddTemp = XKHDD::GetHddSmartTemp();
  else
    callback->byHddTemp = 0;
#endif
}

const char *CSysInfo::TranslateInfo(DWORD dwInfo)
{
  switch(dwInfo)
  {
#ifdef HAS_XBOX_HARDWARE
  case SYSTEM_MPLAYER_VERSION:
    if (m_bRequestDone) return m_mplayerversion;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_KERNEL_VERSION:
    if (m_bRequestDone) return m_kernelversion;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_CPUFREQUENCY:
    if (m_bRequestDone) return m_cpufrequency;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_XBOX_VERSION:
    if (m_bRequestDone) return m_xboxversion;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_AV_PACK_INFO:
    if (m_bRequestDone) return m_avpackinfo;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_VIDEO_ENCODER_INFO:
    if (m_bRequestDone) return m_videoencoder;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_XBOX_SERIAL:
    if (m_bRequestDone) return m_xboxserial;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_HDD_LOCKKEY:
    if (m_bRequestDone) return m_hddlockkey;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_HDD_BOOTDATE:
    if (m_bRequestDone) return m_hddbootdate;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_HDD_CYCLECOUNT:
    if (m_bRequestDone) return m_hddcyclecount;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case NETWORK_MAC_ADDRESS:
    if (m_bRequestDone) return m_macadress;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_XBE_REGION:
    if (m_bRequestDone) return m_videoxberegion;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_DVD_ZONE:
    if (m_bRequestDone) return m_videodvdzone;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_XBOX_PRODUCE_INFO:
    if (m_bRequestDone) return m_produceinfo;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_XBOX_BIOS:
    if (m_bRequestDone) return m_XboxBios;
    else return CInfoLoader::BusyInfo(dwInfo);
    break;
  case SYSTEM_XBOX_MODCHIP:
    if (g_advancedSettings.m_DisableModChipDetection)
        return "Modchip lookup is disabled";
    else
    {
        if (m_bRequestDone) 
          return m_XboxModChip;
        else 
          return CInfoLoader::BusyInfo(dwInfo);
    }
    break;
  // HDD request
  case SYSTEM_HDD_MODEL:
    m_temp.Format("%s %s",g_localizeStrings.Get(13154), m_HDDModel);
    if (m_hddRequest) return m_temp;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_HDD_SERIAL:
    m_temp.Format("%s %s", g_localizeStrings.Get(13155), m_HDDSerial);
    if (m_hddRequest) return m_temp;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_HDD_FIRMWARE:
    m_temp.Format("%s %s", g_localizeStrings.Get(13156), m_HDDFirmware);
    if (m_hddRequest) return m_temp;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_HDD_PASSWORD:
    m_temp.Format("%s %s", g_localizeStrings.Get(13157), m_HDDpw);
    if (m_hddRequest) return m_temp;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_HDD_LOCKSTATE:
    m_temp.Format("%s %s", g_localizeStrings.Get(13158), m_HDDLockState);
    if (m_hddRequest) return m_temp;
    else return CInfoLoader::BusyInfo(dwInfo);
  // DVD request
  case SYSTEM_DVD_MODEL:
    m_temp.Format("%s %s", g_localizeStrings.Get(13152), m_DVDModel);
    if (m_dvdRequest) return m_temp;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_DVD_FIRMWARE:
    m_temp.Format("%s %s", g_localizeStrings.Get(13153), m_DVDFirmware);
    if (m_dvdRequest) return m_temp;
    else return CInfoLoader::BusyInfo(dwInfo);
  // All Time request
  case LCD_HDD_TEMPERATURE:
  case SYSTEM_HDD_TEMPERATURE:
  {
    CTemperature temp;
    temp.SetState(CTemperature::invalid);
    if(m_bSmartEnabled && byHddTemp != 0)
      temp = CTemperature::CreateFromCelsius((double)byHddTemp);
    switch(dwInfo)
    {
      case SYSTEM_HDD_TEMPERATURE:
        m_temp.Format("%s %s", g_localizeStrings.Get(13151), temp.ToString());
        break;
      case LCD_HDD_TEMPERATURE:
        m_temp.Format("%s",temp.ToString());
        break;
    }
    return m_temp;
  }
#endif
  case SYSTEM_UPTIME:
    if (!m_systemuptime.IsEmpty()) return m_systemuptime;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_TOTALUPTIME:
     if (!m_systemtotaluptime.IsEmpty()) return m_systemtotaluptime;
    else return CInfoLoader::BusyInfo(dwInfo);
  case SYSTEM_INTERNET_STATE:
    if (!m_InternetState.IsEmpty())return m_InternetState;
    else return g_localizeStrings.Get(503); //Busy text

  default:
    return g_localizeStrings.Get(503); //Busy text
  }
}
DWORD CSysInfo::TimeToNextRefreshInMs()
{ 
  // request every 15 seconds
  return 15000;
}
void CSysInfo::Reset()
{
#ifdef HAS_XBOX_HARDWARE
  m_XboxBios ="";
  m_XboxModChip ="";
  m_dvdRequest = false;
  m_hddRequest = false;

  m_HDDModel ="";
  m_HDDSerial="";
  m_HDDFirmware="";
  m_HDDpw ="";
  m_HDDLockState = "";
  m_DVDModel=""; 
  m_DVDFirmware="";
#endif
  m_bInternetState = false;
  m_InternetState = "";
}

CSysInfo::CSysInfo(void) : CInfoLoader("sysinfo")
{
#ifdef HAS_XBOX_HARDWARE
  m_bRequestDone = false;
  m_XKEEPROM = new XKEEPROM;
  m_XKEEPROM->ReadFromXBOX();
  // XKEEPROM functions will automatically decrypt if encrypted already, but we should
  // automatically decrypt before fetching version information to set the XBOXVersion first.
  m_XKEEPROM->Decrypt();
  m_XBOXVersion = m_XKEEPROM->GetXBOXVersion();
#endif
}

CSysInfo::~CSysInfo()
{
#ifdef HAS_XBOX_HARDWARE
   delete m_XKEEPROM;
#endif
}
#ifdef HAS_XBOX_HARDWARE
struct Bios * CSysInfo::LoadBiosSigns()
{
  FILE *infile;

  if ((infile = fopen(XBOX_BIOS_ID_INI_FILE,"r")) == NULL)
  {
    CLog::Log(LOGDEBUG, "ERROR LOADING BIOSES.INI!!");
    return NULL;
  }
  else
  {
    struct Bios * Listone = (struct Bios *)calloc(1000, sizeof(struct Bios));
    int cntBioses=0;
    char buffer[255];
    char stringone[255];
    do
    {
      fgets(stringone,255,infile);
      if  (stringone[0] != '#')
      {
        if (strstr(stringone,"=")!= NULL)
        {
          strcpy(Listone[cntBioses].Name,ReturnBiosName(buffer, stringone));
          strcpy(Listone[cntBioses].Signature,ReturnBiosSign(buffer, stringone));
          cntBioses++;
        }
      }
    } while( !feof( infile ) && cntBioses < 999 );
    fclose(infile);
    strcpy(Listone[cntBioses++].Name,"\0");
    strcpy(Listone[cntBioses++].Signature,"\0");
    return Listone;
  }
}
char* CSysInfo::MD5Buffer(char *buffer, long PosizioneInizio,int KBytes)
{
  MD5_CTX mdContext;
  unsigned char md5sum[16];
  char md5sumstring[33] = "";
  MD5Init (&mdContext);
  MD5Update(&mdContext, (unsigned char *)(buffer + PosizioneInizio), KBytes * 1024);
  MD5Final (md5sum, &mdContext);
  XKGeneral::BytesToHexStr(md5sum, 16, md5sumstring);
  strcpy(MD5_Sign, md5sumstring);
  return MD5_Sign;
}

char* CSysInfo::ReturnBiosName(char *buffer, char *str)
{
  int cnt1,cnt2,i;
  cnt1=cnt2=0;

  for (i=0;i<255;i++) buffer[i]='\0';
  if ( (strstr(str,"(1MB)")==0) || (strstr(str,"(512)")==0) || (strstr(str,"(256)")==0) )
    cnt2=5;

  while (str[cnt2] != '=')
  {
    buffer[cnt1]=str[cnt2];
    cnt1++;
    cnt2++;
  }
  buffer[cnt1++]='\0';
  return buffer;
}
char* CSysInfo::ReturnBiosSign(char *buffer, char *str)
{
  int cnt1,cnt2,i;
  cnt1=cnt2=0;
  for (i=0;i<255;i++) buffer[i]='\0';
  while (str[cnt2] != '=') cnt2++;
  cnt2++;
  while (str[cnt2] != NULL)
  {
    if ( str[cnt2] != ' ' )
    {
      buffer[cnt1]=toupper(str[cnt2]);
      cnt1++;
      cnt2++;
    }
    else cnt2++;
  }
  buffer[cnt1++]='\0';
  return buffer;
}
char* CSysInfo::CheckMD5 (struct Bios *Listone, char *Sign)
{
  int cntBioses;
  cntBioses=0;
  do
  {
    if  (strstr(Listone[cntBioses].Signature, Sign) != NULL)
    { return (Listone[cntBioses].Name);   }
    cntBioses++;
  }
  while( strcmp(Listone[cntBioses].Name,"\0") != 0);
  return ("Unknown");
}

void CSysInfo::WriteTXTInfoFile()
{
  LPCSTR strFilename = XBOX_XBMC_TXT_INFOFILE;
  BOOL retVal = FALSE;
  DWORD dwBytesWrote = 0;
  CHAR tmpData[SYSINFO_TMP_SIZE];
  CStdString tmpstring;
  LPSTR tmpFileStr = new CHAR[2048];
  ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
  ZeroMemory(tmpFileStr, 2048);

  HANDLE hf = CreateFile(strFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hf !=  INVALID_HANDLE_VALUE)
  {
    //Write File Header..
    strcat(tmpFileStr, "*******  XBOXMEDIACENTER [XBMC] INFORMATION FILE  *******\r\n");
    if (m_XBOXVersion== m_XKEEPROM->V1_0)
      strcat(tmpFileStr, "\r\nXBOX Version = \t\tV1.0");
    else if (m_XBOXVersion == m_XKEEPROM->V1_1)
      strcat(tmpFileStr, "\r\nXBOX Version = \t\tV1.1-V1.5");
    else if (m_XBOXVersion == m_XKEEPROM->V1_6)
      strcat(tmpFileStr,  "\r\nXBOX Version = \t\tV1.6");
    //Get Kernel Version
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    sprintf(tmpData, "\r\nKernel Version: \t%d.%d.%d.%d", XboxKrnlVersion->VersionMajor,XboxKrnlVersion->VersionMinor,XboxKrnlVersion->Build,XboxKrnlVersion->Qfe);
    strcat(tmpFileStr, tmpData);

    //Get Memory Status
    strcat(tmpFileStr, "\r\nXBOX RAM = \t\t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    MEMORYSTATUS stat;
    GlobalMemoryStatus( &stat );
    ltoa(stat.dwTotalPhys/1024/1024, tmpData, 10);
    strcat(tmpFileStr, tmpData);
    strcat(tmpFileStr, " MBytes");

    //Write Serial Number..
    strcat(tmpFileStr, "\r\n\r\nXBOX Serial Number = \t");
    CHAR serial[SERIALNUMBER_SIZE + 1] = "";
    m_XKEEPROM->GetSerialNumberString(serial);
    strcat(tmpFileStr, serial);

    //Write MAC Address..
    strcat(tmpFileStr, "\r\nXBOX MAC Address = \t");
    m_XKEEPROM->GetMACAddressString((LPSTR)&tmpstring, ':');
    strcat(tmpFileStr, tmpstring.c_str());

    //Write Online Key ..
    strcat(tmpFileStr, "\r\nXBOX Online Key = \t");
    char livekey[ONLINEKEY_SIZE * 2 + 1] = "";
    m_XKEEPROM->GetOnlineKeyString(livekey);
    strcat(tmpFileStr, livekey);

    //Write VideoMode ..
    strcat(tmpFileStr, "\r\nXBOX Video Mode = \t");
    VIDEO_STANDARD vdo = m_XKEEPROM->GetVideoStandardVal();
    if (vdo == XKEEPROM::VIDEO_STANDARD::PAL_I)
      strcat(tmpFileStr, "PAL");
    else
      strcat(tmpFileStr, "NTSC");

    //Write XBE Region..
    strcat(tmpFileStr, "\r\nXBOX XBE Region = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    m_XKEEPROM->GetXBERegionString(tmpData);
    strcat(tmpFileStr, tmpData);

    //Write HDDKey..
    strcat(tmpFileStr, "\r\nXBOX HDD Key = \t\t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    m_XKEEPROM->GetHDDKeyString(tmpData);
    strcat(tmpFileStr, tmpData);

    //Write Confounder..
    strcat(tmpFileStr, "\r\nXBOX Confounder = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    m_XKEEPROM->GetConfounderString(tmpData);
    strcat(tmpFileStr, tmpData);

    //GET HDD Info...
    //Query ATA IDENTIFY
    XKHDD::ATA_COMMAND_OBJ cmdObj;
    ZeroMemory(&cmdObj, sizeof(XKHDD::ATA_COMMAND_OBJ));
    cmdObj.DATA_BUFFSIZE = 0x200;
    cmdObj.IPReg.bCommandReg = ATA_IDENTIFY_DEVICE;
    XKHDD::SendATACommand(XBOX_DEVICE_HDD, &cmdObj, IDE_COMMAND_READ);

    //Write HDD Model
    strcat(tmpFileStr, "\r\n\r\nXBOX HDD Model = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    XKHDD::GetIDEModel(cmdObj.DATA_BUFFER, (LPSTR)tmpData);
    strcat(tmpFileStr, tmpData);

    //Write HDD Serial..
    strcat(tmpFileStr, "\r\nXBOX HDD Serial = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    XKHDD::GetIDESerial(cmdObj.DATA_BUFFER, (LPSTR)tmpData);
    strcat(tmpFileStr, tmpData);

    //Write HDD Password..
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    strcat(tmpFileStr, "\r\n\r\nXBOX HDD Password = \t");

    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    BYTE HDDpwd[20];
    ZeroMemory(HDDpwd, 20);
    XKHDD::GenerateHDDPwd((UCHAR *)XboxHDKey, cmdObj.DATA_BUFFER, (UCHAR*)&HDDpwd);
    XKGeneral::BytesToHexStr(HDDpwd, 20, tmpData);
    strcat(tmpFileStr, tmpData);

    //Query ATAPI IDENTIFY
    ZeroMemory(&cmdObj, sizeof(XKHDD::ATA_COMMAND_OBJ));
    cmdObj.DATA_BUFFSIZE = 0x200;
    cmdObj.IPReg.bCommandReg = ATA_IDENTIFY_PACKET_DEVICE;
    XKHDD::SendATACommand(XBOX_DEVICE_DVDROM, &cmdObj, IDE_COMMAND_READ);

    //Write DVD Model
    strcat(tmpFileStr, "\r\n\r\nXBOX DVD Model = \t");
    ZeroMemory(tmpData, SYSINFO_TMP_SIZE);
    XKHDD::GetIDEModel(cmdObj.DATA_BUFFER, (LPSTR)tmpData);
    strcat(tmpFileStr, tmpData);
    strupr(tmpFileStr);

    WriteFile(hf, tmpFileStr, (DWORD)strlen(tmpFileStr), &dwBytesWrote, NULL);
  }
  delete[] tmpFileStr;
  CloseHandle(hf);
}
bool CSysInfo::CreateBiosBackup()
{
  FILE *fp;
  DWORD addr        = FLASH_BASE_ADDRESS;
  DWORD addr_kernel = KERNEL_BASE_ADDRESS;
  char * flash_copy, data;
  CXBoxFlash mbFlash;

  flash_copy = (char *) malloc(0x100000);

  if((fp = fopen(XBOX_BIOS_BACKUP_FILE, "wb")) != NULL)
  {
    for(int loop=0;loop<0x100000;loop++)
    {
        data = mbFlash.Read(addr++);
        flash_copy[loop] = data;
    }
    fwrite(flash_copy, 0x100000, 1, fp);
    fclose(fp);
    free(flash_copy);
    return true;
  }
  else
  {
    CLog::Log(LOGINFO, "BIOS FILE CREATION ERROR!");
    return false;
  }
}

bool CSysInfo::CreateEEPROMBackup()
{
  m_XKEEPROM->WriteToBINFile(XBOX_EEPROM_BIN_BACKUP_FILE);
  m_XKEEPROM->WriteToCFGFile(XBOX_EEPROM_CFG_BACKUP_FILE);
  return true;
}
bool CSysInfo::CheckBios(CStdString& strDetBiosNa)
{
  BYTE data;
  char *BIOS_Name;
  int BiosTrovato,i;
  DWORD addr        = FLASH_BASE_ADDRESS;
  DWORD addr_kernel = KERNEL_BASE_ADDRESS;
  CXBoxFlash mbFlash;
  char * flash_copy;

  flash_copy = (char *) malloc(0x100000);

  BiosTrovato     = 0;
  BIOS_Name     = (char*) malloc(100);

  struct Bios *Listone = LoadBiosSigns();

  if( !Listone )
  {
    free(BIOS_Name);
    return false;
  }

  for(int loop=0;loop<0x100000;loop++)
  {
    data = mbFlash.Read(addr++);
    flash_copy[loop] = data;
  }

  // Detect a 1024 KB Bios MD5
  MD5Buffer (flash_copy,0,1024);
  strcpy(BIOS_Name,CheckMD5(Listone, MD5_Sign));
  if ( strcmp(BIOS_Name, "Unknown") == 0)
  {
    // Detect a 512 KB Bios MD5
    MD5Buffer (flash_copy,0,512);
    strcpy(BIOS_Name, CheckMD5(Listone, MD5_Sign));
    if ( strcmp(BIOS_Name,"Unknown") == 0)
    {
      // Detect a 256 KB Bios MD5
      MD5Buffer (flash_copy,0,256);
      strcpy(BIOS_Name,CheckMD5(Listone, MD5_Sign));
      if ( strcmp(BIOS_Name,"Unknown") != 0)
      {
        CLog::Log(LOGDEBUG, "- Detected BIOS [256 KB]: %hs",BIOS_Name);
        CLog::Log(LOGDEBUG, "- BIOS MD5 Hash: %hs",MD5_Sign);
        strDetBiosNa = BIOS_Name;
        free(flash_copy);
        free(Listone);
        free(BIOS_Name);
        return true;
      }
      else
      {
        CLog::Log(LOGINFO, "------------------- BIOS Detection Log ------------------");
        // 256k Bios MD5
        if ( (MD5BufferNew(flash_copy,0,256) == MD5BufferNew(flash_copy,262144,256)) && (MD5BufferNew(flash_copy,524288,256)== MD5BufferNew(flash_copy,786432,256)) )
        {
            for (i=0;i<16; i++) MD5_Sign[i]='\0';
            MD5Buffer(flash_copy,0,256);
            CLog::Log(LOGINFO, "256k BIOSES: Checksums are (256)");
            CLog::Log(LOGINFO, "  1.Bios > %hs",CheckMD5(Listone, MD5_Sign));
            CLog::Log(LOGINFO, "  Add this to BiosIDs.ini: (256)BiosNameHere = %hs",MD5_Sign);
            CLog::Log(LOGINFO, "---------------------------------------------------------");
            strDetBiosNa = g_localizeStrings.Get(20306);
            free(flash_copy);
            free(Listone);
            free(BIOS_Name);
            return true;
        }
        else
        { 
          CLog::Log(LOGINFO, "- BIOS: This is not a 256KB Bios!");
          // 512k Bios MD5
          if ((MD5BufferNew(flash_copy,0,512)) == (MD5BufferNew(flash_copy,524288,512)))
          {
            for (i=0;i<16; i++) MD5_Sign[i]='\0';
            MD5Buffer(flash_copy,0,512);
            CLog::Log(LOGINFO, "512k BIOSES: Checksums are (512)");
            CLog::Log(LOGINFO, "  1.Bios > %hs",CheckMD5(Listone,MD5_Sign));
            CLog::Log(LOGINFO, "  Add. this to BiosIDs.ini: (512)BiosNameHere = %hs",MD5_Sign);
            CLog::Log(LOGINFO, "---------------------------------------------------------");
            strDetBiosNa = g_localizeStrings.Get(20306);
            free(flash_copy);
            free(Listone);
            free(BIOS_Name);
            return true;
          }
          else
          {
            CLog::Log(LOGINFO, "- BIOS: This is not a 512KB Bios!");
            // 1024k Bios MD5
            for (i=0;i<16; i++) MD5_Sign[i]='\0';
            MD5Buffer(flash_copy,0,1024);
            CLog::Log(LOGINFO, "1024k BIOS: Checksums are (1MB)");
            CLog::Log(LOGINFO, "  1.Bios > %hs",CheckMD5(Listone, MD5_Sign));
            CLog::Log(LOGINFO, "  Add. this to BiosIDs.ini: (1MB)BiosNameHere = %hs",MD5_Sign);
            CLog::Log(LOGINFO, "---------------------------------------------------------");
            strDetBiosNa = g_localizeStrings.Get(20306);
            free(flash_copy);
            free(Listone);
            free(BIOS_Name);
            return true;
          }
        }
      }
    }
    else
    {
      CLog::Log(LOGINFO, "- Detected BIOS [512 KB]: %hs",BIOS_Name);
      CLog::Log(LOGINFO, "- BIOS MD5 Hash: %hs",MD5_Sign);
      strDetBiosNa = BIOS_Name;
      free(flash_copy);
      free(Listone);
      free(BIOS_Name);
      return true;

    }
  }
  else
  {
    CLog::Log(LOGINFO, "- Detected BIOS [1024 KB]: %hs",BIOS_Name);
    CLog::Log(LOGINFO, "- BIOS MD5 Hash: %hs",MD5_Sign);
    strDetBiosNa = BIOS_Name;
    free(flash_copy);
    free(Listone);
    free(BIOS_Name);
    return true;
  }
  free(flash_copy);
  free(Listone);
  free(BIOS_Name);
  return false;
}
bool CSysInfo::GetXBOXVersionDetected(CStdString& strXboxVer)
{
  unsigned int iTemp;
  char Ver[6];

  HalReadSMBusValue(0x20,0x01,0,(LPBYTE)&Ver[0]);
  HalReadSMBusValue(0x20,0x01,0,(LPBYTE)&Ver[1]);
  HalReadSMBusValue(0x20,0x01,0,(LPBYTE)&Ver[2]);
  Ver[3] = 0; Ver[4] = 0; Ver[5] = 0;

  if ( strcmp(Ver,("01D")) == NULL || strcmp(Ver,("D01")) == NULL || strcmp(Ver,("1D0")) == NULL || strcmp(Ver,("0D1")) == NULL)
  { strXboxVer = "DEVKIT";  return true;}
  else if ( strcmp(Ver,("DBG")) == NULL){ strXboxVer = "DEBUGKIT Green";  return true;}
  else if ( strcmp(Ver,("B11")) == NULL){ strXboxVer = "DEBUGKIT Green";  return true;}
  else if ( strcmp(Ver,("P01")) == NULL){ strXboxVer = "v1.0";  return true;}
  else if ( strcmp(Ver,("P05")) == NULL){ strXboxVer = "v1.1";  return true;}
  else if ( strcmp(Ver,("P11")) == NULL ||  strcmp(Ver,("1P1")) == NULL || strcmp(Ver,("11P")) == NULL )
  {
    if (HalReadSMBusValue(0xD4,0x00,0,(LPBYTE)&iTemp)==0){  strXboxVer = "v1.4";  return true; }
    else {  strXboxVer = "v1.2/v1.3";   return true;}
  }
  else if ( strcmp(Ver,("P2L")) == NULL){ strXboxVer = "v1.6";  return true;}
  else  { strXboxVer.Format("UNKNOWN: Please report this --> %s",Ver); return true;
  }
}

bool CSysInfo::GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware)
{
  XKHDD::ATA_COMMAND_OBJ hddcommand;
  DWORD slen = 0;

  ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));

  //Detect DVD Model...
  hddcommand.DATA_BUFFSIZE = 0x200;
  hddcommand.IPReg.bCommandReg = ATA_IDENTIFY_PACKET_DEVICE;

  if (XKHDD::SendATACommand(XBOX_DEVICE_DVDROM, &hddcommand, IDE_COMMAND_READ))
  {
    //Get DVD Model
    CHAR lpsDVDModel[100];
    ZeroMemory(&lpsDVDModel,100);
    XKHDD::GetIDEModel(hddcommand.DATA_BUFFER, lpsDVDModel);
    CLog::Log(LOGDEBUG, "DVD Model: %s",lpsDVDModel);
    strDVDModel.Format("%s",lpsDVDModel);

    //Get DVD FirmWare...
    CHAR lpsDVDFirmware[100];
    ZeroMemory(&lpsDVDFirmware,100);
    XKHDD::GetIDEFirmWare(hddcommand.DATA_BUFFER, lpsDVDFirmware);
    CLog::Log(LOGDEBUG, "DVD Firmware: %s",lpsDVDFirmware);
    strDVDFirmware.Format("%s",lpsDVDFirmware);
    m_dvdRequest= true;
  }
  //check if the requested values are empty to reset the request..
  if(m_dvdRequest && strDVDModel.IsEmpty() && strDVDFirmware.IsEmpty())
    m_dvdRequest=false;

  return m_dvdRequest;
}
bool CSysInfo::GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState)
{
  XKHDD::ATA_COMMAND_OBJ hddcommand;

  ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));

  hddcommand.DATA_BUFFSIZE = 0x200;
  hddcommand.IPReg.bCommandReg = ATA_IDENTIFY_DEVICE;

  if (XKHDD::SendATACommand(XBOX_DEVICE_HDD, &hddcommand, IDE_COMMAND_READ))
  {
    //Get Model Name
    CHAR lpsHDDModel[100] = "";
    XKHDD::GetIDEModel(hddcommand.DATA_BUFFER, lpsHDDModel);
    strHDDModel.Format("%s",lpsHDDModel);

    //Get Serial...
    CHAR lpsHDDSerial[100] = "";
    XKHDD::GetIDESerial(hddcommand.DATA_BUFFER, lpsHDDSerial);
    strHDDSerial.Format("%s", lpsHDDSerial);

    //Get HDD FirmWare...
    CHAR lpsHDDFirmware[100] = "";
    XKHDD::GetIDEFirmWare(hddcommand.DATA_BUFFER, lpsHDDFirmware);
    strHDDFirmware.Format("%s", lpsHDDFirmware);

    //Print HDD Password...
    BYTE pbHDDPassword[32] = "";
    CHAR lpsHDDPassword[65] = "";
    XKHDD::GenerateHDDPwd((UCHAR *)XboxHDKey, hddcommand.DATA_BUFFER, pbHDDPassword);
    XKGeneral::BytesToHexStr(pbHDDPassword, 20, lpsHDDPassword);
    strHDDpw.Format("%s", lpsHDDPassword);

    //Get ATA Locked State
    DWORD SecStatus = XKHDD::GetIDESecurityStatus(hddcommand.DATA_BUFFER);
    if (!(SecStatus & ATA_SECURITY_SUPPORTED))
      strHDDLockState = g_localizeStrings.Get(20164);
    else if ((SecStatus & ATA_SECURITY_SUPPORTED) && !(SecStatus & ATA_SECURITY_ENABLED))
      strHDDLockState = g_localizeStrings.Get(20165);
    else if ((SecStatus & ATA_SECURITY_SUPPORTED) && (SecStatus & ATA_SECURITY_ENABLED))
      strHDDLockState = g_localizeStrings.Get(20166);
    else if (SecStatus & ATA_SECURITY_FROZEN)
      strHDDLockState = g_localizeStrings.Get(20167);
    else if (SecStatus & ATA_SECURITY_COUNT_EXPIRED)
      strHDDLockState = g_localizeStrings.Get(20168);

    if (XKHDD::IsSmartSupported(hddcommand.DATA_BUFFER))
    {
      m_bSmartSupported = true;
      CLog::Log(LOGNOTICE, "HDD: SMART is supported.");
      if (XKHDD::IsSmartEnabled(hddcommand.DATA_BUFFER))
      {
        m_bSmartEnabled = true;
        CLog::Log(LOGNOTICE, "HDD: SMART is enabled.");
      }
    }

    //Command was succesful
    m_hddRequest = true;
  }
  //check if the requested values are empty to reset the request..
  if(m_hddRequest && strHDDModel.IsEmpty() && strHDDSerial.IsEmpty())
    m_hddRequest = false;

  return m_hddRequest;
}
bool CSysInfo::GetRefurbInfo(CStdString& rfi_FirstBootTime, CStdString& rfi_PowerCycleCount)
{
  XBOX_REFURB_INFO xri;
  SYSTEMTIME sys_time;
  if (ExReadWriteRefurbInfo(&xri, sizeof(XBOX_REFURB_INFO), FALSE) < 0)
    return false;

  FileTimeToSystemTime((FILETIME*)&xri.FirstBootTime, &sys_time);
  rfi_FirstBootTime.Format("%s %d-%d-%d %d:%02d", g_localizeStrings.Get(13173), 
    sys_time.wMonth, 
    sys_time.wDay, 
    sys_time.wYear,
    sys_time.wHour,
    sys_time.wMinute);

  rfi_PowerCycleCount.Format("%s %d", g_localizeStrings.Get(13174), xri.PowerCycleCount);
  return true;
}
#endif
bool CSysInfo::GetDiskSpace(const CStdString drive,int& iTotal, int& iTotalFree, int& iTotalUsed, int& iPercentFree, int& iPercentUsed)
{
  CStdString driveName = drive + ":\\";
  ULARGE_INTEGER total, totalFree, totalUsed;

  if (drive.IsEmpty() || drive.Equals("*")) //All Drives
  {
    ULARGE_INTEGER totalC, totalFreeC;
    ULARGE_INTEGER totalE, totalFreeE;
    ULARGE_INTEGER totalF, totalFreeF;
    ULARGE_INTEGER totalG, totalFreeG;
    ULARGE_INTEGER totalX, totalFreeX;
    ULARGE_INTEGER totalY, totalFreeY;
    ULARGE_INTEGER totalZ, totalFreeZ;
    
    BOOL bC = GetDiskFreeSpaceEx("C:\\", NULL, &totalC, &totalFreeC);
    BOOL bE = GetDiskFreeSpaceEx("E:\\", NULL, &totalE, &totalFreeE);
    BOOL bF = GetDiskFreeSpaceEx("F:\\", NULL, &totalF, &totalFreeF);
    BOOL bG = GetDiskFreeSpaceEx("G:\\", NULL, &totalG, &totalFreeG);
    BOOL bX = GetDiskFreeSpaceEx("X:\\", NULL, &totalX, &totalFreeX);
    BOOL bY = GetDiskFreeSpaceEx("Y:\\", NULL, &totalY, &totalFreeY);
    BOOL bZ = GetDiskFreeSpaceEx("Z:\\", NULL, &totalZ, &totalFreeZ);
    
    total.QuadPart = (bC?totalC.QuadPart:0)+
      (bE?totalE.QuadPart:0)+
      (bF?totalF.QuadPart:0)+
      (bG?totalG.QuadPart:0)+
      (bX?totalX.QuadPart:0)+
      (bY?totalY.QuadPart:0)+
      (bZ?totalZ.QuadPart:0);
    totalFree.QuadPart = (bC?totalFreeC.QuadPart:0)+
      (bE?totalFreeE.QuadPart:0)+
      (bF?totalFreeF.QuadPart:0)+
      (bG?totalFreeG.QuadPart:0)+
      (bX?totalFreeX.QuadPart:0)+
      (bY?totalFreeY.QuadPart:0)+
      (bZ?totalFreeZ.QuadPart:0);
    
    iTotal = (int)(total.QuadPart/MB);
    iTotalFree = (int)(totalFree.QuadPart/MB);
    iTotalUsed = (int)((total.QuadPart - totalFree.QuadPart)/MB);

    totalUsed.QuadPart = total.QuadPart - totalFree.QuadPart;
    iPercentUsed = (int)(100.0f * totalUsed.QuadPart/total.QuadPart + 0.5f);
    iPercentFree = 100 - iPercentUsed;
    return true;
  }
  else if ( GetDiskFreeSpaceEx(driveName.c_str(), NULL, &total, &totalFree))
  {
    iTotal = (int)(total.QuadPart/MB);
    iTotalFree = (int)(totalFree.QuadPart/MB);
    iTotalUsed = (int)((total.QuadPart - totalFree.QuadPart)/MB);

    totalUsed.QuadPart = total.QuadPart - totalFree.QuadPart;
    iPercentUsed = (int)(100.0f * totalUsed.QuadPart/total.QuadPart + 0.5f);
    iPercentFree = 100 - iPercentUsed;
    return true;
  }
  return false;
}
#ifdef HAS_XBOX_HARDWARE
double CSysInfo::GetCPUFrequency()
{
  DWORD Twin_fsb, Twin_result;
  double Tcpu_fsb, Tcpu_result, Fcpu, CPUSpeed;

  Tcpu_fsb = RDTSC();
  Twin_fsb = GetTickCount();

  Sleep(300);

  Tcpu_result = RDTSC();
  Twin_result = GetTickCount();

  Fcpu  = (Tcpu_result-Tcpu_fsb);
  Fcpu /= (Twin_result-Twin_fsb);

  CPUSpeed = Fcpu/1000;

  CLog::Log(LOGDEBUG, "- CPU Speed: %4.6fMHz",CPUSpeed);
  return CPUSpeed;
}

double CSysInfo::RDTSC(void)
{
  unsigned long a, b;
  double x;
  __asm
  {
    RDTSC
    mov [a],eax
    mov [b],edx
  }
  x=b;
  x*=0x100000000;
  x+=a;
  return x;
}
CStdString CSysInfo::GetModCHIPDetected()
{
  CXBoxFlash *mbFlash=new CXBoxFlash(); //Max description Leng= 40
  {
    // Unknown or TSOP
    mbFlash->AddFCI(0x09,0x00,"Unknown/Onboard TSOP (protected)",0x00000);

    // Known XBOX ModCHIP IDs&Names
    mbFlash->AddFCI(0x01,0xAD,"XECUTER 3",0x100000); // if Write Protection is ON!: this chip can not detected! X3 Bug or Feature! it will return Unknown/Onboard TSOP (protected)!
    mbFlash->AddFCI(0x01,0xD5,"XECUTER 2",0x100000);
    mbFlash->AddFCI(0x01,0xC4,"XENIUM",0x100000);
    mbFlash->AddFCI(0x01,0xC4,"XENIUM",0x000000);
    mbFlash->AddFCI(0x04,0xBA,"ALX2+ R3 FLASH",0x40000);
    // XBOX Possible Flash CHIPs
    mbFlash->AddFCI(0x01,0xb0,"AMD Am29F002BT/NBT",0x40000);
    mbFlash->AddFCI(0x01,0x34,"AMD Am29F002BB/NBB",0x40000);
    mbFlash->AddFCI(0x01,0x51,"AMD Am29F200BT",0x40000);
    mbFlash->AddFCI(0x01,0x57,"AMD Am29F200BB",0x40000);
    mbFlash->AddFCI(0x01,0x40,"AMD Am29LV002BT",0x40000);
    mbFlash->AddFCI(0x01,0xc2,"AMD Am29LV002BB",0x40000);
    mbFlash->AddFCI(0x01,0x3b,"AMD Am29LV200BT",0x40000);
    mbFlash->AddFCI(0x01,0xbf,"AMD Am29LV200BB",0x40000);
    mbFlash->AddFCI(0x01,0x0c,"AMD Am29DL400BT",0x80000);
    mbFlash->AddFCI(0x01,0x0f,"AMD Am29DL400BB",0x80000);
    mbFlash->AddFCI(0x01,0x77,"AMD Am29F004BT",0x80000);
    mbFlash->AddFCI(0x01,0x7b,"AMD Am29F004BB",0x80000);
    mbFlash->AddFCI(0x01,0xa4,"AMD Am29F040B",0x80000);
    mbFlash->AddFCI(0x01,0x23,"AMD Am29F400BT",0x80000);
    mbFlash->AddFCI(0x01,0xab,"AMD Am29F400BB",0x80000);
    mbFlash->AddFCI(0x01,0xb5,"AMD Am29LV004BT",0x80000);
    mbFlash->AddFCI(0x01,0xb6,"AMD Am29LV004BB",0x80000);
    mbFlash->AddFCI(0x01,0x4f,"AMD Am29LV040B",0x80000);
    mbFlash->AddFCI(0x01,0xb9,"AMD Am29LV400BT",0x80000);
    mbFlash->AddFCI(0x01,0xba,"AMD Am29LV400BB",0x80000);
    mbFlash->AddFCI(0x01,0x4a,"AMD Am29DL800BT",0x100000);
    mbFlash->AddFCI(0x01,0xcb,"AMD Am29DL800BB",0x100000);
    mbFlash->AddFCI(0x01,0xd5,"AMD Am29F080B",0x100000);
    mbFlash->AddFCI(0x01,0xd6,"AMD Am29F800BT",0x100000);
    mbFlash->AddFCI(0x01,0x58,"AMD Am29F800BB",0x100000);
    mbFlash->AddFCI(0x01,0x3e,"AMD Am29LV008BT",0x100000);
    mbFlash->AddFCI(0x01,0x37,"AMD Am29LV008BB",0x100000);
    mbFlash->AddFCI(0x01,0x38,"AMD Am29LV080B",0x100000);
    mbFlash->AddFCI(0x01,0xda,"AMD Am29LV800BT/DT",0x100000);
    mbFlash->AddFCI(0x01,0x5b,"AMD Am29LV800BB/DB",0x100000);

    mbFlash->AddFCI(0x37,0x8c,"AMIC A29002T/290021T",0x40000);
    mbFlash->AddFCI(0x37,0x0d,"AMIC A29002U/290021U",0x40000);
    mbFlash->AddFCI(0x37,0x86,"AMIC A29040A",0x80000);
    mbFlash->AddFCI(0x37,0xb0,"AMIC A29400T/294001T",0x80000);
    mbFlash->AddFCI(0x37,0x31,"AMIC A29400U/294001U",0x80000);
    mbFlash->AddFCI(0x37,0x34,"AMIC A29L004T/A29L400T",0x80000);
    mbFlash->AddFCI(0x37,0xb5,"AMIC A29L004U/A29L400U",0x80000);
    mbFlash->AddFCI(0x37,0x92,"AMIC A29L040",0x80000);
    mbFlash->AddFCI(0x37,0x0e,"AMIC A29800T",0x100000);
    mbFlash->AddFCI(0x37,0x8f,"AMIC A29800U",0x100000);
    mbFlash->AddFCI(0x37,0x1a,"AMIC A29L008T/A29L800T",0x100000);
    mbFlash->AddFCI(0x37,0x9b,"AMIC A29L008U/A29L800U",0x100000);

    mbFlash->AddFCI(0x1f,0x07,"Atmel AT49F002A",0x40000);
    mbFlash->AddFCI(0x1f,0x08,"Atmel AT49F002AT",0x40000);

    mbFlash->AddFCI(0x04,0xb0,"Fujitsu MBM29F002TC",0x40000);
    mbFlash->AddFCI(0x04,0x34,"Fujitsu MBM29F002BC",0x40000);
    mbFlash->AddFCI(0x04,0x51,"Fujitsu MBM29F200TC",0x40000);
    mbFlash->AddFCI(0x04,0x57,"Fujitsu MBM29F200BC",0x40000);
    mbFlash->AddFCI(0x04,0x40,"Fujitsu MBM29LV002TC",0x40000);
    mbFlash->AddFCI(0x04,0xc2,"Fujitsu MBM29LV002BC",0x40000);
    mbFlash->AddFCI(0x04,0x3b,"Fujitsu MBM29LV200TC",0x40000);
    mbFlash->AddFCI(0x04,0xbf,"Fujitsu MBM29LV200BC",0x40000);
    mbFlash->AddFCI(0x04,0x0c,"Fujitsu MBM29DL400TC",0x80000);
    mbFlash->AddFCI(0x04,0x0f,"Fujitsu MBM29DL400BC",0x80000);
    mbFlash->AddFCI(0x04,0x77,"Fujitsu MBM29F004TC",0x80000);
    mbFlash->AddFCI(0x04,0x7b,"Fujitsu MBM29F004BC",0x80000);
    mbFlash->AddFCI(0x04,0xa4,"Fujitsu MBM29F040C",0x80000);
    mbFlash->AddFCI(0x04,0x23,"Fujitsu MBM29F400TC",0x80000);
    mbFlash->AddFCI(0x04,0xab,"Fujitsu MBM29F400BC",0x80000);
    mbFlash->AddFCI(0x04,0xb5,"Fujitsu MBM29LV004TC",0x80000);
    mbFlash->AddFCI(0x04,0xb6,"Fujitsu MBM29LV004BC",0x80000);
    mbFlash->AddFCI(0x04,0xb9,"Fujitsu MBM29LV400TC",0x80000);
    mbFlash->AddFCI(0x04,0xba,"Fujitsu MBM29LV400BC",0x80000);
    mbFlash->AddFCI(0x04,0x4a,"Fujitsu MBM29DL800TA",0x100000);
    mbFlash->AddFCI(0x04,0xcb,"Fujitsu MBM29DL800BA",0x100000);
    mbFlash->AddFCI(0x04,0xd5,"Fujitsu MBM29F080A",0x100000);
    mbFlash->AddFCI(0x04,0xd6,"Fujitsu MBM29F800TA",0x100000);
    mbFlash->AddFCI(0x04,0x58,"Fujitsu MBM29F800BA",0x100000);
    mbFlash->AddFCI(0x04,0x3e,"Fujitsu MBM29LV008TA",0x100000);
    mbFlash->AddFCI(0x04,0x37,"Fujitsu MBM29LV008BA",0x100000);
    mbFlash->AddFCI(0x04,0x38,"Fujitsu MBM29LV080A",0x100000);
    mbFlash->AddFCI(0x04,0xda,"Fujitsu MBM29LV800TA/TE",0x100000);
    mbFlash->AddFCI(0x04,0x5b,"Fujitsu MBM29LV800BA/BE",0x100000);

    mbFlash->AddFCI(0xad,0xb0,"Hynix HY29F002",0x40000);
    mbFlash->AddFCI(0xad,0xa4,"Hynix HY29F040A",0x80000);
    mbFlash->AddFCI(0xad,0x23,"Hynix HY29F400T/AT",0x80000);
    mbFlash->AddFCI(0xad,0xab,"Hynix HY29F400B/AB",0x80000);
    mbFlash->AddFCI(0xad,0xb9,"Hynix HY29LV400T",0x80000);
    mbFlash->AddFCI(0xad,0xba,"Hynix HY29LV400B",0x80000);
    mbFlash->AddFCI(0xad,0xd5,"Hynix HY29F080",0x100000);
    mbFlash->AddFCI(0xad,0xd6,"Hynix HY29F800T/AT",0x100000);
    mbFlash->AddFCI(0xad,0x58,"Hynix HY29F800B/AB",0x100000);
    mbFlash->AddFCI(0xad,0xda,"Hynix HY29LV800T",0x100000);
    mbFlash->AddFCI(0xad,0x5b,"Hynix HY29LV800B",0x100000);

    mbFlash->AddFCI(0xc2,0xb0,"Macronix MX29F002T/NT",0x40000);
    mbFlash->AddFCI(0xc2,0x34,"Macronix MX29F002B/NB",0x40000);
    mbFlash->AddFCI(0xc2,0x36,"Macronix MX29F022T/NT",0x40000);
    mbFlash->AddFCI(0xc2,0x37,"Macronix MX29F022B/NB",0x40000);
    mbFlash->AddFCI(0xc2,0x51,"Macronix MX29F200T",0x40000);
    mbFlash->AddFCI(0xc2,0x57,"Macronix MX29F200B",0x40000);
    mbFlash->AddFCI(0xc2,0x45,"Macronix MX29F004T",0x80000);
    mbFlash->AddFCI(0xc2,0x46,"Macronix MX29F004B",0x80000);
    mbFlash->AddFCI(0xc2,0xa4,"Macronix MX29F040",0x80000);
    mbFlash->AddFCI(0xc2,0x23,"Macronix MX29F400T",0x80000);
    mbFlash->AddFCI(0xc2,0xab,"Macronix MX29F400B",0x80000);
    mbFlash->AddFCI(0xc2,0xb5,"Macronix MX29LV004T",0x80000);
    mbFlash->AddFCI(0xc2,0xb6,"Macronix MX29LV004B",0x80000);
    mbFlash->AddFCI(0xc2,0x4f,"Macronix MX29LV040",0x80000);
    mbFlash->AddFCI(0xc2,0xb9,"Macronix MX29LV400T",0x80000);
    mbFlash->AddFCI(0xc2,0xba,"Macronix MX29LV400B",0x80000);
    mbFlash->AddFCI(0xc2,0xd5,"Macronix MX29F080",0x100000);
    mbFlash->AddFCI(0xc2,0xd6,"Macronix MX29F800T",0x100000);
    mbFlash->AddFCI(0xc2,0x58,"Macronix MX29F800B",0x100000);
    mbFlash->AddFCI(0xc2,0x3e,"Macronix MX29LV008T",0x100000);
    mbFlash->AddFCI(0xc2,0x37,"Macronix MX29LV008B",0x100000);
    mbFlash->AddFCI(0xc2,0x38,"Macronix MX29LV081",0x100000);
    mbFlash->AddFCI(0xc2,0xda,"Macronix MX29LV800T",0x100000);
    mbFlash->AddFCI(0xc2,0x5b,"Macronix MX29LV800B",0x100000);

    mbFlash->AddFCI(0xb0,0xc9,"Sharp LHF00L02/L06/L07",0x100000);
    mbFlash->AddFCI(0xb0,0xcf,"Sharp LHF00L03/L04/L05",0x100000);
    mbFlash->AddFCI(0x89,0xa2,"Sharp LH28F008SA series",0x100000);
    mbFlash->AddFCI(0x89,0xa6,"Sharp LH28F008SC series",0x100000);
    mbFlash->AddFCI(0xb0,0xec,"Sharp LH28F008BJxx-PT series",0x100000);
    mbFlash->AddFCI(0xb0,0xed,"Sharp LH28F008BJxx-PB series",0x100000);
    mbFlash->AddFCI(0xb0,0x4b,"Sharp LH28F800BVxx-BTL series",0x100000);
    mbFlash->AddFCI(0xb0,0x4c,"Sharp LH28F800BVxx-TV series",0x100000);
    mbFlash->AddFCI(0xb0,0x4d,"Sharp LH28F800BVxx-BV series",0x100000);

    mbFlash->AddFCI(0xbf,0x10,"SST 29EE020",0x40000);
    mbFlash->AddFCI(0xbf,0x12,"SST 29LE020/29VE020",0x40000);
    mbFlash->AddFCI(0xbf,0xd6,"SST 39LF020/39VF020",0x40000);
    mbFlash->AddFCI(0xbf,0xb6,"SST 39SF020A",0x40000);
    mbFlash->AddFCI(0xbf,0x57,"SST 49LF002A",0x40000);
    mbFlash->AddFCI(0xbf,0x57,"SST 49LF002A",0x100000);
    mbFlash->AddFCI(0xbf,0x52,"SST 49LF020A",0x40000);
    mbFlash->AddFCI(0xbf,0x1b,"SST 49LF003A",0x60000);
    mbFlash->AddFCI(0xbf,0x1c,"SST 49LF030A",0x60000);
    mbFlash->AddFCI(0xbf,0x61,"SST 49LF020",0x40000);
    mbFlash->AddFCI(0xbf,0x13,"SST 29SF040",0x80000);
    mbFlash->AddFCI(0xbf,0x14,"SST 29VF040",0x80000);
    mbFlash->AddFCI(0xbf,0xd7,"SST 39LF040/39VF040",0x80000);
    mbFlash->AddFCI(0xbf,0xb7,"SST 39SF040",0x80000);
    mbFlash->AddFCI(0xbf,0x60,"SST 49LF004A/B",0x80000);
    mbFlash->AddFCI(0xbf,0x51,"SST 49LF040",0x80000);
    mbFlash->AddFCI(0xbf,0xd8,"SST 39LF080/39VF080/39VF088",0x100000);
    mbFlash->AddFCI(0xbf,0x5a,"SST 49LF008A",0x100000);
    mbFlash->AddFCI(0xbf,0x5b,"SST 49LF080A",0x100000);
    mbFlash->AddFCI(0x20,0xb0,"ST M29F002T/NT/BT/BNT",0x40000);
    mbFlash->AddFCI(0x20,0x34,"ST M29F002B/BB",0x40000);
    mbFlash->AddFCI(0x20,0xd3,"ST M29F200BT",0x40000);
    mbFlash->AddFCI(0x20,0xd4,"ST M29F200BB",0x40000);
    mbFlash->AddFCI(0x20,0xe2,"ST M29F040 series",0x80000);
    mbFlash->AddFCI(0x20,0xd5,"ST M29F400T/BT",0x80000);
    mbFlash->AddFCI(0x20,0xd6,"ST M29F400B/BB",0x80000);
    mbFlash->AddFCI(0x20,0xf1,"ST M29F080 series",0x100000);
    mbFlash->AddFCI(0x20,0xec,"ST M29F800DT",0x100000);
    mbFlash->AddFCI(0x20,0x58,"ST M29F800DB",0x100000);
    mbFlash->AddFCI(0xda,0x45,"Winbond W29C020",0x40000);
    mbFlash->AddFCI(0x09,0x00,"Winbond W49F020T",0x40000);
    mbFlash->AddFCI(0xda,0xb5,"Winbond W39L020",0x40000);
    mbFlash->AddFCI(0xda,0x0b,"Winbond W49F002U",0x40000);
    mbFlash->AddFCI(0xda,0x8c,"Winbond W49F020",0x40000);
    mbFlash->AddFCI(0xda,0xb0,"Winbond W49V002A",0x40000);
    mbFlash->AddFCI(0xda,0x46,"Winbond W29C040",0x40000);
    mbFlash->AddFCI(0xda,0xb6,"Winbond W39L040",0x80000);
    mbFlash->AddFCI(0xda,0x3d,"Winbond W39V040A",0x80000);
  }
  CStdString strTemp = "", strTemp1 = "", strTemp2 = "";
  if (mbFlash->CheckID()!=0 || mbFlash->CheckID2()!=0)
  {
    CLog::Log(LOGDEBUG, "- Detected TSOP/ModChip: %s",mbFlash->CheckID()->text);
    CLog::Log(LOGDEBUG, "- Detected TSOP/ModChip: %s",mbFlash->CheckID2()->text);
    strTemp1 = mbFlash->CheckID()->text;
    strTemp2 = mbFlash->CheckID2()->text;
  }
  else {  CLog::Log(LOGDEBUG, "- Detected TSOP/MOdCHIP: Unknown");  strTemp2 = "Unknown"; }

  if (strTemp1 != strTemp2)
  {
    CLog::Log(LOGDEBUG, "- Detected TSOP/MOdCHIP: Detection does not match! (%s != %s)",strTemp1.c_str(),strTemp2.c_str());
    CLog::Log(LOGDEBUG, "- Detected TSOP/ModChip: Using -> %s",strTemp1.c_str());
    strTemp.Format("%s",strTemp1.c_str());
  }
  else strTemp = strTemp2;

  delete mbFlash;

  return strTemp;
}

CStdString CSysInfo::MD5BufferNew(char *buffer,long PosizioneInizio,int KBytes)
{
  CStdString strReturn;
  MD5_CTX mdContext;
  unsigned char md5sum[16];
  char md5sumstring[33] = "";
  MD5Init (&mdContext);
  MD5Update(&mdContext, (unsigned char *)(buffer + PosizioneInizio), KBytes * 1024);
  MD5Final (md5sum, &mdContext);
  XKGeneral::BytesToHexStr(md5sum, 16, md5sumstring);
  strReturn.Format("%s", md5sumstring);
  return strReturn;
}
CStdString CSysInfo::GetAVPackInfo()
{  
  //AV-Pack Detection PICReg(0x04)
  int cAVPack;
  HalReadSMBusValue(0x20,XKUtils::PIC16L_CMD_AV_PACK,0,(LPBYTE)&cAVPack);

  if (cAVPack == XKUtils::AV_PACK_SCART) return g_localizeStrings.Get(13292)+" "+"SCART";
  else if (cAVPack == XKUtils::AV_PACK_HDTV) return g_localizeStrings.Get(13292)+" "+"HDTV";
  else if (cAVPack == XKUtils::AV_PACK_VGA) return g_localizeStrings.Get(13292)+" "+"VGA";
  else if (cAVPack == XKUtils::AV_PACK_RFU) return g_localizeStrings.Get(13292)+" "+"RFU";
  else if (cAVPack == XKUtils::AV_PACK_SVideo) return g_localizeStrings.Get(13292)+" "+"S-Video";
  else if (cAVPack == XKUtils::AV_PACK_Undefined) return g_localizeStrings.Get(13292)+" "+"Undefined";
  else if (cAVPack == XKUtils::AV_PACK_Standard) return g_localizeStrings.Get(13292)+" "+"Standard RGB";
  else if (cAVPack == XKUtils::AV_PACK_Missing) return g_localizeStrings.Get(13292)+" "+"Missing or Unknown";
  else return g_localizeStrings.Get(13292)+" "+"Unknown";
}
CStdString CSysInfo::GetVideoEncoder()
{
  int iTemp;
  if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_CONNEXANT,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0)
  { 
    CLog::Log(LOGDEBUG, "Video Encoder: CONNEXANT");  
    return g_localizeStrings.Get(13286)+" "+"CONNEXANT"; 
  }
  if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_FOCUS,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0)
  { 
    CLog::Log(LOGDEBUG, "Video Encoder: FOCUS");
    return g_localizeStrings.Get(13286)+" "+"FOCUS";   
  }
  if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_XCALIBUR,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0)
  { 
    CLog::Log(LOGDEBUG, "Video Encoder: XCALIBUR");   
    return g_localizeStrings.Get(13286)+" "+ "XCALIBUR";
  }
  else 
  {  
    CLog::Log(LOGDEBUG, "Video Encoder: UNKNOWN");  
    return g_localizeStrings.Get(13286)+" "+"UNKNOWN"; 
  }
}
CStdString CSysInfo::SmartXXModCHIP()
{
  // SmartXX ModChip Detection
  unsigned char uSmartXX_ID = ((_inp(0xf701)) & 0xf);

  if ( uSmartXX_ID == 1 )      // SmartXX V1+V2
    return "SmartXX V1/V2";
  else if ( uSmartXX_ID == 2 ) // SmartXX V1+V2
    return "SmartXX V1/V2";
  else if ( uSmartXX_ID == 5 ) // SmartXX OPX
    return "SmartXX OPX";
  else if ( uSmartXX_ID == 8 ) // SmartXX V3
    return "SmartXX V3";
  else 
    return "None";
}

CStdString CSysInfo::GetMPlayerVersion()
{
  CStdString strVersion="";
  DllLoader* mplayerDll;
  const char* (__cdecl* pMplayerGetVersion)();
  const char* (__cdecl* pMplayerGetCompileDate)();
  const char* (__cdecl* pMplayerGetCompileTime)();

  const char *version = NULL;
  const char *date = NULL;
  const char *btime = NULL;

  mplayerDll = new DllLoader("Q:\\system\\players\\mplayer\\mplayer.dll",true);

  if( mplayerDll->Load() )
  {
    if (mplayerDll->ResolveExport("mplayer_getversion", (void**)&pMplayerGetVersion))
      version = pMplayerGetVersion();
    if (mplayerDll->ResolveExport("mplayer_getcompiledate", (void**)&pMplayerGetCompileDate))
      date = pMplayerGetCompileDate();
    if (mplayerDll->ResolveExport("mplayer_getcompiletime", (void**)&pMplayerGetCompileTime))
      btime = pMplayerGetCompileTime();
    if (version && date && btime)
    {
      strVersion.Format("%s (%s - %s)",version, date, btime);
    }
    else if (version)
    {
      strVersion.Format("%s",version);
    }
  }
  delete mplayerDll;
  mplayerDll=NULL;
  return strVersion;
}
CStdString CSysInfo::GetKernelVersion()
{
  int ikrnl = XboxKrnlVersion->Qfe & 67;
  CLog::Log(LOGDEBUG, "- XBOX Kernel Qfe= %i", XboxKrnlVersion->Qfe);
  CLog::Log(LOGDEBUG, "- XBOX Kernel Drive FG result= %i", ikrnl);
  CStdString strKernel;
  strKernel.Format("%s %u.%u.%u.%u",g_localizeStrings.Get(13283),XboxKrnlVersion->VersionMajor,XboxKrnlVersion->VersionMinor,XboxKrnlVersion->Build,XboxKrnlVersion->Qfe);
  return strKernel;
}
CStdString CSysInfo::GetCPUFreqInfo()
{
  CStdString strCPUFreq;
  double CPUFreq = GetCPUFrequency();
  strCPUFreq.Format("%s %4.2fMHz", g_localizeStrings.Get(13284), CPUFreq);
  return strCPUFreq;
}
CStdString CSysInfo::GetXBVerInfo()
{
  CStdString strXBoxVer;
  CStdString strXBOXVersion;
  if (GetXBOXVersionDetected(strXBOXVersion))
    strXBoxVer.Format("%s %s", g_localizeStrings.Get(13288), strXBOXVersion);
  else 
    strXBoxVer.Format("%s %s", g_localizeStrings.Get(13288), g_localizeStrings.Get(13205)); // "Unknown"
  return strXBoxVer;
}
CStdString CSysInfo::GetUnits(int iFrontPort)
{
  // Get the Connected Units on the Front USB Ports!
  DWORD dwDeviceGamePad = XGetDevices(XDEVICE_TYPE_GAMEPAD);
  DWORD dwDeviceKeyboard = XGetDevices(XDEVICE_TYPE_DEBUG_KEYBOARD);
  DWORD dwDeviceMouse = XGetDevices(XDEVICE_TYPE_DEBUG_MOUSE);
  DWORD dwDeviceHeadPhone = XGetDevices(XDEVICE_TYPE_VOICE_HEADPHONE);
  DWORD dwDeviceMicroPhone = XGetDevices(XDEVICE_TYPE_VOICE_MICROPHONE);
  DWORD dwDeviceMemory = XGetDevices(XDEVICE_TYPE_MEMORY_UNIT);
  DWORD dwDeviceIRRemote = XGetDevices(XDEVICE_TYPE_IR_REMOTE);

  // Values 1 ->  on Port 1
  // Values 2 ->  on Port 2
  // Values 4 ->  on Port 3
  // Values 8 ->  on Port 4
  // Values 3 ->  on Port 1&2
  // Values 5 ->  on Port 1&3
  // Values 6 ->  on Port 2&3
  // Values 7 ->  on Port 1&2&3
  // Values 9  -> on Port 1&4
  // Values 10 -> on Port 2&4
  // Values 11 -> on Port 1&2&4
  // Values 12 -> on Port 3&4
  // Values 13 -> on Port 1&3&4
  // Values 14 -> on Port 2&3&4
  // Values 15 -> on Port 1&2&3&4

  bool bPad=false, bMem=false, bKeyb=false, bMouse=false, bHeadSet=false, bMic=false, bIR=false;
  if (iFrontPort == 1)
  {
    bPad = dwDeviceGamePad > 0 && dwDeviceGamePad == 1 || dwDeviceGamePad == 3 || dwDeviceGamePad == 5 || dwDeviceGamePad == 7 || dwDeviceGamePad == 9 || dwDeviceGamePad == 11 || dwDeviceGamePad == 13 || dwDeviceGamePad == 15;
    bMem = dwDeviceMemory > 0 && dwDeviceMemory == 1 || dwDeviceMemory == 3 || dwDeviceMemory == 5 || dwDeviceMemory == 7 || dwDeviceMemory == 9 || dwDeviceMemory == 11 || dwDeviceMemory == 13 || dwDeviceMemory == 15;
  }
  else if (iFrontPort == 2)
  {
    bPad = dwDeviceGamePad > 0 && dwDeviceGamePad == 2 || dwDeviceGamePad == 3 || dwDeviceGamePad == 6 || dwDeviceGamePad == 7 || dwDeviceGamePad == 10 || dwDeviceGamePad == 11 || dwDeviceGamePad == 14 || dwDeviceGamePad == 15;
    bMem = dwDeviceMemory > 0 && dwDeviceMemory == 2 || dwDeviceMemory == 3 || dwDeviceMemory == 6 || dwDeviceMemory == 7 || dwDeviceMemory == 10 || dwDeviceMemory == 11 || dwDeviceMemory == 14 || dwDeviceMemory == 15;
  }
  else if (iFrontPort == 3)
  {
    bPad = dwDeviceGamePad > 0 && dwDeviceGamePad == 4 || dwDeviceGamePad == 5 || dwDeviceGamePad == 6 || dwDeviceGamePad == 7 || dwDeviceGamePad == 12 || dwDeviceGamePad == 13 || dwDeviceGamePad == 14 || dwDeviceGamePad == 15;
    bMem = dwDeviceMemory > 0 && dwDeviceMemory == 4 || dwDeviceMemory == 5 || dwDeviceMemory == 6 || dwDeviceMemory == 7 || dwDeviceMemory == 12 || dwDeviceMemory == 13 || dwDeviceMemory == 14 || dwDeviceMemory == 15;
    iFrontPort = 4;
  }
  else if (iFrontPort == 4)
  {
    bPad = dwDeviceGamePad > 0 && dwDeviceGamePad == 8 || dwDeviceGamePad == 9 || dwDeviceGamePad == 10 || dwDeviceGamePad == 11 || dwDeviceGamePad == 12 || dwDeviceGamePad == 13 || dwDeviceGamePad == 14 || dwDeviceGamePad == 15;
    bMem = dwDeviceMemory > 0 && dwDeviceMemory == 8 || dwDeviceMemory == 9 || dwDeviceMemory == 10 || dwDeviceMemory == 11 || dwDeviceMemory == 12 || dwDeviceMemory == 13 || dwDeviceMemory == 14 || dwDeviceMemory == 15;
    iFrontPort = 8;
  }
  bKeyb = dwDeviceKeyboard > 0 && dwDeviceKeyboard == iFrontPort;
  bMouse = dwDeviceMouse > 0 && dwDeviceMouse == iFrontPort;
  bHeadSet = dwDeviceHeadPhone > 0 && dwDeviceHeadPhone == iFrontPort;
  bMic = dwDeviceMicroPhone > 0 && dwDeviceMicroPhone == iFrontPort;
  bIR = dwDeviceIRRemote > 0 && dwDeviceIRRemote == iFrontPort;
  
  CStdString strReturn;
  if (iFrontPort==4) iFrontPort = 3;
  if (iFrontPort==8) iFrontPort = 4;
  strReturn.Format("%s %i: %s%s%s%s%s%s%s%s%s%s%s%s%s",g_localizeStrings.Get(13169),iFrontPort, 
    bPad ? g_localizeStrings.Get(13163):"", bPad && bKeyb ? ", ":"", bPad && bMem ? ", ":"", bPad && (bHeadSet || bMic) ? ", ":"",
    bKeyb ? g_localizeStrings.Get(13164):"", bKeyb && bMouse ? ", ":"",
    bMouse ? g_localizeStrings.Get(13165):"",bMouse && (bHeadSet || bMic) ? ", ":"",
    bHeadSet || bMic ? g_localizeStrings.Get(13166):"", (bHeadSet || bMic) && bMem ? ", ":"",
    bMem ? g_localizeStrings.Get(13167):"", bMem && bIR ? ", ":"",
    bIR ? g_localizeStrings.Get(13168):""
    );

  return strReturn;
}
CStdString CSysInfo::GetMACAddress()
{
  char macaddress[20] = "";

  m_XKEEPROM->GetMACAddressString((LPSTR)&macaddress, ':');

  CStdString strMacAddress;
  strMacAddress.Format("%s: %s", g_localizeStrings.Get(149), macaddress);
  return strMacAddress;
}
CStdString CSysInfo::GetXBOXSerial(bool bLabel)
{
  CHAR serial[SERIALNUMBER_SIZE + 1] = "";
  m_XKEEPROM->GetSerialNumberString(serial);

  CStdString strXBOXSerial;
  if (!bLabel)
    strXBOXSerial.Format("%s", serial);
  else 
    strXBOXSerial.Format("%s %s",g_localizeStrings.Get(13289), serial);
  return strXBOXSerial;
}
CStdString CSysInfo::GetXBProduceInfo()
{
  CStdString serial = GetXBOXSerial(false);
  // Print XBOX Production Place and Date
  char *info = (char *) serial.c_str();
  char *country;
  switch (atoi(&info[11]))
  {
  case 2:
    country = "Mexico";
    break;
  case 3:
    country = "Hungary";
    break;
  case 5:
    country = "China";
    break;
  case 6:
    country = "Taiwan";
    break;
  default:
    country = "Unknown";
    break;
  }
  
  CLog::Log(LOGDEBUG, "- XBOX production info: Country: %s, LineNumber: %c, Week %c%c, Year 200%c", country, info[0x00], info[0x08], info[0x09],info[0x07]);
  CStdString strXBProDate;
  strXBProDate.Format("%s %s, %s 200%c, %s: %c%c %s: %c",
    g_localizeStrings.Get(13290), 
    country, 
    g_localizeStrings.Get(201),
    info[0x07],
    g_localizeStrings.Get(20169),
    info[0x08],
    info[0x09],
    g_localizeStrings.Get(20170),
    info[0x00]);
  return strXBProDate;
}
CStdString CSysInfo::GetVideoXBERegion()
{
  //Print Video Standard & XBE Region...
  CStdString XBEString, VideoStdString;
  switch (m_XKEEPROM->GetVideoStandardVal())
  {
  case XKEEPROM::NTSC_J:
    VideoStdString = "NTSC J";
    break;
  case XKEEPROM::NTSC_M:
    VideoStdString = "NTSC M";
    break;
  case XKEEPROM::PAL_I:
    VideoStdString = "PAL I";
    break;
  case XKEEPROM::PAL_M:
    VideoStdString = "PAL M";
    break;
  default:
    VideoStdString = g_localizeStrings.Get(13205); // "Unknown"
  }

  switch(m_XKEEPROM->GetXBERegionVal())
  {
  case XKEEPROM::NORTH_AMERICA:
    XBEString = "North America";
    break;
  case XKEEPROM::JAPAN:
    XBEString = "Japan";
    break;
  case XKEEPROM::EURO_AUSTRALIA:
    XBEString = "Europe / Australia";
    break;
  default:
    XBEString = g_localizeStrings.Get(13205); // "Unknown"
  }

  CStdString strVideoXBERegion;
  strVideoXBERegion.Format("%s %s, %s", g_localizeStrings.Get(13293), VideoStdString, XBEString);
  return strVideoXBERegion;
}
CStdString CSysInfo::GetDVDZone()
{
  //Print DVD [Region] Zone ..
  DVD_ZONE dvdVal;
  dvdVal = m_XKEEPROM->GetDVDRegionVal();
  CStdString strdvdzone;
  strdvdzone.Format("%s %d",g_localizeStrings.Get(13294), dvdVal);
  return strdvdzone;
}

CStdString CSysInfo::GetXBLiveKey()
{
  //Print XBLIVE Online Key..
  char livekey[ONLINEKEY_SIZE * 2 + 1] = "";
  m_XKEEPROM->GetOnlineKeyString(livekey);

  CStdString strXBLiveKey;
  strXBLiveKey.Format("%s %s",g_localizeStrings.Get(13298), livekey);
  return strXBLiveKey;
}
CStdString CSysInfo::GetHDDKey()
{
  //Print HDD Key...
  char hdkey[HDDKEY_SIZE * 2 + 1];
  m_XKEEPROM->GetHDDKeyString((LPSTR)&hdkey);

  CStdString strhddlockey;
  strhddlockey.Format("%s %s",g_localizeStrings.Get(13150), hdkey);
  return strhddlockey;
}

CStdString CSysInfo::GetModChipInfo()
{
  CStdString strModChipInfo;
  // XBOX Modchip Type Detection
  CStdString ModChip = GetModCHIPDetected();
  CStdString SmartXX = SmartXXModCHIP();
  
  // Check if it is a SmartXX
  if (!SmartXX.Equals("None"))
  {
    strModChipInfo.Format("%s %s", g_localizeStrings.Get(13291), SmartXX);
    CLog::Log(LOGDEBUG, "- Detected ModChip: %s",SmartXX.c_str());
  }
  else
  {
    if ( !ModChip.Equals("Unknown/Onboard TSOP (protected)"))
    {
      strModChipInfo.Format("%s %s", g_localizeStrings.Get(13291), ModChip);
    }
    else
    {
      strModChipInfo.Format("%s %s", g_localizeStrings.Get(13291), g_localizeStrings.Get(20311));
    }
  }
  return strModChipInfo;
}

CStdString CSysInfo::GetBIOSInfo()
{
  //Format bios informations
  CStdString strBiosName;
  CStdString cBIOSName;
  if(CheckBios(cBIOSName))
    strBiosName.Format("%s %s", g_localizeStrings.Get(13285),cBIOSName);
  else
    strBiosName.Format("%s %s", g_localizeStrings.Get(13285),"File: BiosIDs.ini Not Found!");

  return strBiosName;
}
CStdString CSysInfo::GetTrayState()
{
  // Set DVD Drive State! [TrayOpen, NotReady....]
  CStdString trayState = "D: ";
  switch (CIoSupport::GetTrayState())
  {
  case TRAY_OPEN:
    trayState+=g_localizeStrings.Get(162);
    break;
  case DRIVE_NOT_READY:
    trayState+=g_localizeStrings.Get(163);
    break;
  case TRAY_CLOSED_NO_MEDIA:
    trayState+=g_localizeStrings.Get(164);
    break;
  case TRAY_CLOSED_MEDIA_PRESENT:
    trayState+=g_localizeStrings.Get(165);
    break;
  default:
    trayState+=g_localizeStrings.Get(503); //Busy
    break;
  }
  return trayState;
}
#endif
CStdString CSysInfo::GetHddSpaceInfo(int drive, bool shortText)
{
 int percent;
 return GetHddSpaceInfo( percent, drive, shortText);
}
CStdString CSysInfo::GetHddSpaceInfo(int& percent, int drive, bool shortText)
{
  int total, totalFree, totalUsed, percentFree, percentused;
  CStdString strDrive; 
  bool bRet=false;
  percent = 0;
  CStdString strRet;
  switch (drive)
  {
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
    case SYSTEM_TOTAL_SPACE:
    case SYSTEM_FREE_SPACE_PERCENT:
    case SYSTEM_USED_SPACE_PERCENT:
      strDrive = g_localizeStrings.Get(20161);
      bRet = g_sysinfo.GetDiskSpace("",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_C:
    case SYSTEM_FREE_SPACE_C:
    case SYSTEM_USED_SPACE_C:
    case SYSTEM_TOTAL_SPACE_C:
    case SYSTEM_FREE_SPACE_PERCENT_C:
    case SYSTEM_USED_SPACE_PERCENT_C:
      strDrive = "C";
      bRet = g_sysinfo.GetDiskSpace("C",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_E:
    case SYSTEM_FREE_SPACE_E:
    case SYSTEM_USED_SPACE_E:
    case SYSTEM_TOTAL_SPACE_E:
    case SYSTEM_FREE_SPACE_PERCENT_E:
    case SYSTEM_USED_SPACE_PERCENT_E:
      strDrive = "E";
      bRet = g_sysinfo.GetDiskSpace("E",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_F:
    case SYSTEM_FREE_SPACE_F:
    case SYSTEM_USED_SPACE_F:
    case SYSTEM_TOTAL_SPACE_F:
    case SYSTEM_FREE_SPACE_PERCENT_F:
    case SYSTEM_USED_SPACE_PERCENT_F:
      strDrive = "F";
      bRet = g_sysinfo.GetDiskSpace("F",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case LCD_FREE_SPACE_G:
    case SYSTEM_FREE_SPACE_G:
    case SYSTEM_USED_SPACE_G:
    case SYSTEM_TOTAL_SPACE_G:
    case SYSTEM_FREE_SPACE_PERCENT_G:
    case SYSTEM_USED_SPACE_PERCENT_G:
      strDrive = "G";
      bRet = g_sysinfo.GetDiskSpace("G",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case SYSTEM_USED_SPACE_X:
    case SYSTEM_FREE_SPACE_X:
    case SYSTEM_TOTAL_SPACE_X:
      strDrive = "X";
      bRet = g_sysinfo.GetDiskSpace("X",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case SYSTEM_USED_SPACE_Y:
    case SYSTEM_FREE_SPACE_Y:
    case SYSTEM_TOTAL_SPACE_Y:
      strDrive = "Y";
      bRet = g_sysinfo.GetDiskSpace("Y",total, totalFree, totalUsed, percentFree, percentused);
      break;
    case SYSTEM_USED_SPACE_Z:
    case SYSTEM_FREE_SPACE_Z:
    case SYSTEM_TOTAL_SPACE_Z:
      strDrive = "Z";
      bRet = g_sysinfo.GetDiskSpace("Z",total, totalFree, totalUsed, percentFree, percentused);
      break;
  }
  if (bRet)
  {
    if (shortText)
    {
      switch(drive)
      {
        case LCD_FREE_SPACE_C:
        case LCD_FREE_SPACE_E:
        case LCD_FREE_SPACE_F:
        case LCD_FREE_SPACE_G:
          strRet.Format("%iMB", totalFree);
          break;
        case SYSTEM_FREE_SPACE:
        case SYSTEM_FREE_SPACE_C:
        case SYSTEM_FREE_SPACE_E:
        case SYSTEM_FREE_SPACE_F:
        case SYSTEM_FREE_SPACE_G:
        case SYSTEM_FREE_SPACE_X:
        case SYSTEM_FREE_SPACE_Y:
        case SYSTEM_FREE_SPACE_Z:
          percent = percentFree;
          break;
        case SYSTEM_USED_SPACE:
        case SYSTEM_USED_SPACE_C:
        case SYSTEM_USED_SPACE_E:
        case SYSTEM_USED_SPACE_F:
        case SYSTEM_USED_SPACE_G:
        case SYSTEM_USED_SPACE_X:
        case SYSTEM_USED_SPACE_Y:
        case SYSTEM_USED_SPACE_Z:
          percent = percentused;
          break;
      }
    }
    else
    {
      switch(drive)
      {
      case SYSTEM_FREE_SPACE:
      case SYSTEM_FREE_SPACE_C:
      case SYSTEM_FREE_SPACE_E:
      case SYSTEM_FREE_SPACE_F:
      case SYSTEM_FREE_SPACE_G:
      case SYSTEM_FREE_SPACE_X:
      case SYSTEM_FREE_SPACE_Y:
      case SYSTEM_FREE_SPACE_Z:
        strRet.Format("%s: %i MB %s", strDrive, totalFree, g_localizeStrings.Get(160));
        break;
      case SYSTEM_USED_SPACE:
      case SYSTEM_USED_SPACE_C:
      case SYSTEM_USED_SPACE_E:
      case SYSTEM_USED_SPACE_F:
      case SYSTEM_USED_SPACE_G:
      case SYSTEM_USED_SPACE_X:
      case SYSTEM_USED_SPACE_Y:
      case SYSTEM_USED_SPACE_Z:
        strRet.Format("%s: %i MB %s", strDrive, totalUsed, g_localizeStrings.Get(20162));
        break;
      case SYSTEM_TOTAL_SPACE:
      case SYSTEM_TOTAL_SPACE_C:
      case SYSTEM_TOTAL_SPACE_E:
      case SYSTEM_TOTAL_SPACE_F:
      case SYSTEM_TOTAL_SPACE_G:
        strRet.Format("%s: %i MB %s", strDrive, total, g_localizeStrings.Get(20161));
        break;
      case SYSTEM_FREE_SPACE_PERCENT:
      case SYSTEM_FREE_SPACE_PERCENT_C:
      case SYSTEM_FREE_SPACE_PERCENT_E:
      case SYSTEM_FREE_SPACE_PERCENT_F:
      case SYSTEM_FREE_SPACE_PERCENT_G:
        strRet.Format("%s: %i%% %s", strDrive, percentFree, g_localizeStrings.Get(160));
        break;
      case SYSTEM_USED_SPACE_PERCENT:
      case SYSTEM_USED_SPACE_PERCENT_C:
      case SYSTEM_USED_SPACE_PERCENT_E:
      case SYSTEM_USED_SPACE_PERCENT_F:
      case SYSTEM_USED_SPACE_PERCENT_G:
        strRet.Format("%s: %i%% %s", strDrive, percentused, g_localizeStrings.Get(20162));
        break;
      }
    }
  }
  else
  {
    if (shortText)
      strRet = "N/A";
    else
      strRet.Format("%s: %s", strDrive, g_localizeStrings.Get(161));
  }
  return strRet;
}

bool CSysInfo::SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays)
{
  iMinutes=0;iHours=0;iDays=0;
  iMinutes = iInputMinutes;
  if (iMinutes >= 60) // Hour's
  {
    iHours = iMinutes / 60;
    iMinutes = iMinutes - (iHours *60);
  }
  if (iHours >= 24) // Days
  {
    iDays = iHours / 24;
    iHours = iHours - (iDays * 24);
  }
  return true;
}

CStdString CSysInfo::GetSystemUpTime(bool bTotalUptime)
{
  CStdString strSystemUptime;
  CStdString strLabel;
  int iInputMinutes, iMinutes,iHours,iDays;
  
  if(bTotalUptime)
  {
    //Total Uptime
    strLabel = g_localizeStrings.Get(12394);
    iInputMinutes = g_stSettings.m_iSystemTimeTotalUp + ((int)(timeGetTime() / 60000));
  }
  else
  {
    //Current UpTime
    strLabel = g_localizeStrings.Get(12390);
    iInputMinutes = (int)(timeGetTime() / 60000);
  }

  SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  if (iDays > 0) 
  {
    strSystemUptime.Format("%s: %i %s, %i %s, %i %s",
      strLabel,
      iDays,g_localizeStrings.Get(12393),
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours >= 1 )
  {
    strSystemUptime.Format("%s: %i %s, %i %s",
      strLabel, 
      iHours,g_localizeStrings.Get(12392),
      iMinutes, g_localizeStrings.Get(12391));
  }
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0)
  {
    strSystemUptime.Format("%s: %i %s",
      strLabel, 
      iMinutes, g_localizeStrings.Get(12391));
  }
  return strSystemUptime;
}
CStdString CSysInfo::GetInternetState()
{
  // Internet connection state!
  CHTTP http;
  CStdString strInetCon;
  m_bInternetState = http.IsInternet();
  if (m_bInternetState)
    strInetCon.Format("%s %s",g_localizeStrings.Get(13295), g_localizeStrings.Get(13296));
  else if (http.IsInternet(false))
    strInetCon.Format("%s %s",g_localizeStrings.Get(13295), g_localizeStrings.Get(13274));
  else // NOT Connected to the Internet!
    strInetCon.Format("%s %s",g_localizeStrings.Get(13295), g_localizeStrings.Get(13297));
  return strInetCon;
}

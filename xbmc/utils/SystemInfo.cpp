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
#include "../stdafx.h"
#include "SystemInfo.h"

#include "../xbox/XKUtils.h"
#include "../xbox/xkhdd.h"
#include "../xbox/XKflash.h"
#include "../xbox/XKRC4.h"
#include "../settings.h"
#include "../utils/log.h"
#include "../xbox/Undocumented.h"
#include "../xbox/xkeeprom.h"
#include "../cores/dllloader/dllloader.h"

#include <conio.h>

const char * CSysInfo::cTempBIOSFile = "Q:\\System\\SystemInfo\\BiosBackup.bin";
const char * CSysInfo::cBIOSmd5IDs   = "Q:\\System\\SystemInfo\\BiosIDs.ini";
char CSysInfo::MD5_Sign[16];
extern "C" XPP_DEVICE_TYPE XDEVICE_TYPE_IR_REMOTE_TABLE;
#define XDEVICE_TYPE_IR_REMOTE  (&XDEVICE_TYPE_IR_REMOTE_TABLE)
#define DEBUG_KEYBOARD
#define DEBUG_MOUSE

char* CSysInfo::MDPrint (MD5_CTX *mdContext)
{
  ZeroMemory(MD5_Sign, 16);
  int i;
  char carat[10],tmpcarat;
  for (i = 0; i < 16; i++)
  {
    ultoa(mdContext->digest[i],carat,16);
    if (strlen(carat)==1 )
    {
      tmpcarat = carat[0];
      carat[0] = '0';
      carat[1] = tmpcarat;
      carat[2] = '\0';
    }
    strcat(MD5_Sign,carat);
  }
  return (strupr(MD5_Sign));
}

char* CSysInfo::MD5Buffer(char *buffer, long PosizioneInizio,int KBytes)
{
  MD5_CTX mdContext;
  MD5Init (&mdContext);
  MD5Update(&mdContext, (unsigned char *)(buffer + PosizioneInizio), KBytes * 1024);
  MD5Final (&mdContext);
  strcpy(MD5_Sign, MDPrint(&mdContext));
  return MD5_Sign;
}

CStdString CSysInfo::MD5BufferNew(char *buffer,long PosizioneInizio,int KBytes)
{
  CStdString strReturn;
  MD5_CTX mdContext;
  MD5Init (&mdContext);
  MD5Update(&mdContext, (unsigned char *)(buffer + PosizioneInizio), KBytes * 1024);
  MD5Final (&mdContext);
  strReturn.Format("%s", MDPrint(&mdContext));
  return strReturn;
}
CStdString CSysInfo::GetAVPackInfo()
{  //AV-Pack Detection PICReg(0x04)
  int cAVPack;
  HalReadSMBusValue(0x20,XKUtils::PIC16L_CMD_AV_PACK,0,(LPBYTE)&cAVPack);

     if (cAVPack == XKUtils::AV_PACK_SCART)   return "SCART";
  else if (cAVPack == XKUtils::AV_PACK_HDTV)    return "HDTV";
  else if (cAVPack == XKUtils::AV_PACK_VGA)   return "VGA";
  else if (cAVPack == XKUtils::AV_PACK_RFU)   return "RFU";
  else if (cAVPack == XKUtils::AV_PACK_SVideo)  return "S-Video";
  else if (cAVPack == XKUtils::AV_PACK_Undefined) return "Undefined";
  else if (cAVPack == XKUtils::AV_PACK_Standard)  return "Standard RGB";
  else if (cAVPack == XKUtils::AV_PACK_Missing) return "Missing or Unknown";
  else return "Unknown";
}
CStdString CSysInfo::GetVideoEncoder()
{
  int iTemp;
  if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_CONNEXANT,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0)
  { CLog::Log(LOGDEBUG, "Video Encoder: CONNEXANT");  return "CONNEXANT"; }
  if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_FOCUS,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0)
  { CLog::Log(LOGDEBUG, "Video Encoder: FOCUS");    return "FOCUS";   }
  if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_XCALIBUR,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0)
  { CLog::Log(LOGDEBUG, "Video Encoder: XCALIBUR");   return "XCALIBUR";  }
  else {  CLog::Log(LOGDEBUG, "Video Encoder: UNKNOWN");  return "UNKNOWN"; }
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

bool CSysInfo::BackupBios()
{
  FILE *fp;
  DWORD addr        = FLASH_BASE_ADDRESS;
  DWORD addr_kernel = KERNEL_BASE_ADDRESS;
  char * flash_copy, data;
  CXBoxFlash mbFlash;

  flash_copy = (char *) malloc(0x100000);

  if((fp = fopen(cTempBIOSFile, "wb")) != NULL)
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
struct Bios * CSysInfo::LoadBiosSigns()
{
  FILE *infile;

  if ((infile = fopen(cBIOSmd5IDs,"r")) == NULL)
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
bool CSysInfo::GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware)
{
  XKHDD::ATA_COMMAND_OBJ hddcommand;
  DWORD slen = 0;
  ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));
  hddcommand.DATA_BUFFSIZE = 0;

  //Detect DVD Model...
  hddcommand.IPReg.bDriveHeadReg = IDE_DEVICE_SLAVE;
  hddcommand.IPReg.bCommandReg = IDE_ATAPI_IDENTIFY;
  if (XKHDD::SendATACommand(IDE_PRIMARY_PORT, &hddcommand, IDE_COMMAND_READ))
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
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "DVD Model Detection FAILED!");
    return false;
  }
}
bool CSysInfo::GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState)
{
  XKHDD::ATA_COMMAND_OBJ hddcommand;

  ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));
  hddcommand.IPReg.bDriveHeadReg = IDE_DEVICE_MASTER;
  hddcommand.IPReg.bCommandReg = IDE_ATA_IDENTIFY;

  if (XKHDD::SendATACommand(IDE_PRIMARY_PORT, &hddcommand, IDE_COMMAND_READ))
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
    if (!(SecStatus & IDE_SECURITY_SUPPORTED))
    {
      strHDDLockState = g_localizeStrings.Get(20164);
      return true;
    }
    if ((SecStatus & IDE_SECURITY_SUPPORTED) && !(SecStatus & IDE_SECURITY_ENABLED))
    {
      strHDDLockState = g_localizeStrings.Get(20165);
      return true;
    }
    if ((SecStatus & IDE_SECURITY_SUPPORTED) && (SecStatus & IDE_SECURITY_ENABLED))
    {
      strHDDLockState = g_localizeStrings.Get(20166);
      return true;
    }

    if (SecStatus & IDE_SECURITY_FROZEN)
    {
      strHDDLockState = g_localizeStrings.Get(20167);
      return true;
    }

    if (SecStatus & IDE_SECURITY_COUNT_EXPIRED)
    {
      strHDDLockState = g_localizeStrings.Get(20168);
      return true;
    }
  return true;
  }
  else return false;
}
double CSysInfo::GetCPUFrequency()
{
  unsigned __int64 Fwin;
  unsigned __int64 Twin_fsb, Twin_result;
  double Tcpu_fsb, Tcpu_result, Fcpu, CPUSpeed;


  if (!QueryPerformanceFrequency((LARGE_INTEGER*)&Fwin))
    return 0;
  Tcpu_fsb = RDTSC();

  if (!QueryPerformanceCounter((LARGE_INTEGER*)&Twin_fsb))
    return 0;
  Sleep(300);
  Tcpu_result = RDTSC();

  if (!QueryPerformanceCounter((LARGE_INTEGER*)&Twin_result))
    return 0;

  Fcpu  = (Tcpu_result-Tcpu_fsb);
  Fcpu *= Fwin;
  Fcpu /= (Twin_result-Twin_fsb);

  CPUSpeed = Fcpu/1000000;

  CLog::Log(LOGDEBUG, "- CPU Speed: %4.6f Mhz",CPUSpeed);
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

  if( mplayerDll->Parse() )
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
  CStdString strKernel;
  strKernel.Format("%s %d.%d.%d.%d",g_localizeStrings.Get(13283),XboxKrnlVersion->VersionMajor,XboxKrnlVersion->VersionMinor,XboxKrnlVersion->Build,XboxKrnlVersion->Qfe);
  return strKernel;
}
CStdString CSysInfo::GetSystemTotalUpTime()
{
  CStdString strSystemUptime;
  CStdString lbl1 = g_localizeStrings.Get(12394);
  CStdString lblMin = g_localizeStrings.Get(12391);
  CStdString lblHou = g_localizeStrings.Get(12392);
  CStdString lblDay = g_localizeStrings.Get(12393);

  int iInputMinutes, iMinutes,iHours,iDays;
  iInputMinutes = g_stSettings.m_iSystemTimeTotalUp + ((int)(timeGetTime() / 60000));
  SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  // Will Display Autodetected Values!
  if (iDays > 0) strSystemUptime.Format("%s: %i %s, %i %s, %i %s",lbl1, iDays,lblDay, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours >= 1 ) strSystemUptime.Format("%s: %i %s, %i %s",lbl1, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0) strSystemUptime.Format("%s: %i %s",lbl1, iMinutes, lblMin);

  return strSystemUptime;
}
CStdString CSysInfo::GetSystemUpTime()
{
  CStdString strSystemUptime;
  CStdString lbl1 = g_localizeStrings.Get(12390);
  CStdString lblMin = g_localizeStrings.Get(12391);
  CStdString lblHou = g_localizeStrings.Get(12392);
  CStdString lblDay = g_localizeStrings.Get(12393);

  int iInputMinutes, iMinutes,iHours,iDays;
  iInputMinutes = (int)(timeGetTime() / 60000);
  CSysInfo::SystemUpTime(iInputMinutes,iMinutes, iHours, iDays);
  // Will Display Autodetected Values!
  if (iDays > 0) strSystemUptime.Format("%s: %i %s, %i %s, %i %s",lbl1, iDays,lblDay, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours >= 1 ) strSystemUptime.Format("%s: %i %s, %i %s",lbl1, iHours,lblHou, iMinutes, lblMin);
  else if (iDays == 0 && iHours == 0 &&  iMinutes >= 0) strSystemUptime.Format("%s: %i %s",lbl1, iMinutes, lblMin);

  return strSystemUptime;
}
CStdString CSysInfo::GetCPUFreqInfo()
{
  CStdString strCPUFreq;
  double CPUFreq;
  CStdString lblCPUSpeed  = g_localizeStrings.Get(13284);
  CPUFreq                 = GetCPUFrequency();
  strCPUFreq.Format("%s %4.2f Mhz.", lblCPUSpeed, CPUFreq);
  return strCPUFreq;
}
CStdString CSysInfo::GetXBVerInfo()
{
  CStdString strXBoxVer;
  CStdString strXBOXVersion;
  CStdString lblXBver   =  g_localizeStrings.Get(13288);
  if (GetXBOXVersionDetected(strXBOXVersion))
  {
    strXBoxVer.Format("%s %s", lblXBver,strXBOXVersion);
    CLog::Log(LOGDEBUG,"XBOX Version: %s",strXBOXVersion.c_str());
  }
  else 
  {
    strXBoxVer.Format("%s %s", lblXBver,g_localizeStrings.Get(13205)); // "Unknown"
    CLog::Log(LOGDEBUG,"XBOX Version: %s",g_localizeStrings.Get(13205).c_str());
  }
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
    if( dwDeviceGamePad > 0 && dwDeviceGamePad == 1 || dwDeviceGamePad == 3 || dwDeviceGamePad == 5 || dwDeviceGamePad == 7 || dwDeviceGamePad == 9 || dwDeviceGamePad == 11 || dwDeviceGamePad == 13 || dwDeviceGamePad == 15 )
    {
      //OK hier hängt ein GamePad
      bPad=true;
    }
    if( dwDeviceMemory > 0 && dwDeviceMemory == 1 || dwDeviceMemory == 3 || dwDeviceMemory == 5 || dwDeviceMemory == 7 || dwDeviceMemory == 9 || dwDeviceMemory == 11 || dwDeviceMemory == 13 || dwDeviceMemory == 15 )
    {
      //OK hier hängt ein MemCard
      bMem=true;
    }
    if( dwDeviceKeyboard > 0 && dwDeviceKeyboard == 1 )
    {
      //OK hier hängt ein Keyboard
      bKeyb=true;
    }
    if( dwDeviceMouse > 0 && dwDeviceMouse == 1 )
    {
      //OK hier hängt eine Maus
      bMouse=true;
    }
    if( dwDeviceHeadPhone > 0 && dwDeviceHeadPhone == 1 )
    {
      //OK hier hängt eine HeadSet
      bHeadSet=true;
    }
    if( dwDeviceMicroPhone > 0 && dwDeviceMicroPhone == 1 )
    {
      //OK hier hängt eine Micro
      bMic=true;
    }
    if( dwDeviceIRRemote > 0 && dwDeviceIRRemote == 1 )
    {
      //OK hier hängt eine Remote
      bIR=true;
    }
  }
  else if (iFrontPort == 2)
  {
    if( dwDeviceGamePad > 0 && dwDeviceGamePad == 2 || dwDeviceGamePad == 3 || dwDeviceGamePad == 6 || dwDeviceGamePad == 7 || dwDeviceGamePad == 10 || dwDeviceGamePad == 11 || dwDeviceGamePad == 14 || dwDeviceGamePad == 15 )
    {
      //OK hier hängt ein GamePad
      bPad=true;
    }
    if( dwDeviceMemory > 0 && dwDeviceMemory == 2 || dwDeviceMemory == 3 || dwDeviceMemory == 6 || dwDeviceMemory == 7 || dwDeviceMemory == 10 || dwDeviceMemory == 11 || dwDeviceMemory == 14 || dwDeviceMemory == 15 )
    {
      //OK hier hängt ein MemCard
      bMem=true;
    }
    if( dwDeviceKeyboard > 0 && dwDeviceKeyboard == 2 )
    {
      //OK hier hängt ein Keyboard
      bKeyb=true;
    }
    if( dwDeviceMouse > 0 && dwDeviceMouse == 2 )
    {
      //OK hier hängt eine Maus
       bMouse=true;
    }
    if( dwDeviceHeadPhone > 0 && dwDeviceHeadPhone == 2 )
    {
      //OK hier hängt eine HeadSet
      bHeadSet=true;
    }
    if( dwDeviceMicroPhone > 0 && dwDeviceMicroPhone == 2 )
    {
      //OK hier hängt eine Micro
    }
    if( dwDeviceIRRemote > 0 && dwDeviceIRRemote == 2 )
    {
      //OK hier hängt eine Micro
       bMic=true;
    }
  }
  else if (iFrontPort == 3)
  {
    if( dwDeviceGamePad > 0 && dwDeviceGamePad == 4 || dwDeviceGamePad == 5 || dwDeviceGamePad == 6 || dwDeviceGamePad == 7 || dwDeviceGamePad == 12 || dwDeviceGamePad == 13 || dwDeviceGamePad == 14 || dwDeviceGamePad == 15 )
    {
      //OK hier hängt ein GamePad
      bPad=true;
    }
    if( dwDeviceMemory > 0 && dwDeviceMemory == 4 || dwDeviceMemory == 5 || dwDeviceMemory == 6 || dwDeviceMemory == 7 || dwDeviceMemory == 12 || dwDeviceMemory == 13 || dwDeviceMemory == 14 || dwDeviceMemory == 15 )
    {
      //OK hier hängt ein MemCard
      bMem=true;
    }
    if( dwDeviceKeyboard > 0 && dwDeviceKeyboard == 4 )
    {
      //OK hier hängt ein Keyboard
      bKeyb=true;
    }
    if( dwDeviceMouse > 0 && dwDeviceMouse == 4 )
    {
      //OK hier hängt eine Maus
       bMouse=true;
    }
    if( dwDeviceHeadPhone > 0 && dwDeviceHeadPhone == 4 )
    {
      //OK hier hängt eine HeadSet
      bHeadSet=true;
    }
    if( dwDeviceMicroPhone > 0 && dwDeviceMicroPhone == 4 )
    {
      //OK hier hängt eine Micro
      bMic=true;
    }
    if( dwDeviceIRRemote > 0 && dwDeviceIRRemote == 4 )
    {
      //OK hier hängt eine Micro
       bIR=true;
    }
  }
  else if (iFrontPort == 4)
  {
    if( dwDeviceGamePad > 0 && dwDeviceGamePad == 8 || dwDeviceGamePad == 9 || dwDeviceGamePad == 10 || dwDeviceGamePad == 11 || dwDeviceGamePad == 12 || dwDeviceGamePad == 13 || dwDeviceGamePad == 14 || dwDeviceGamePad == 15 )
    {
      //OK hier hängt ein GamePad
      bPad=true;
    }
    if( dwDeviceMemory > 0 && dwDeviceMemory == 8 || dwDeviceMemory == 9 || dwDeviceMemory == 10 || dwDeviceMemory == 11 || dwDeviceMemory == 12 || dwDeviceMemory == 13 || dwDeviceMemory == 14 || dwDeviceMemory == 15 )
    {
      //OK hier hängt ein MemCard
      bMem=true;
    }
    if( dwDeviceKeyboard > 0 && dwDeviceKeyboard == 8 )
    {
      //OK hier hängt ein Keyboard
      bKeyb=true;
    }
    if( dwDeviceMouse > 0 && dwDeviceMouse == 8 )
    {
      //OK hier hängt eine Maus
      bMouse=true;
    }
    if( dwDeviceHeadPhone > 0 && dwDeviceHeadPhone == 8 )
    {
      //OK hier hängt eine HeadSet
      bHeadSet=true;
    }
    if( dwDeviceMicroPhone > 0 && dwDeviceMicroPhone == 8 )
    {
      //OK hier hängt eine Micro
      bMic=true;
    }
    if( dwDeviceIRRemote > 0 && dwDeviceIRRemote == 8 )
    {
      //OK hier hängt eine Micro
      bIR=true;
    }
  }
  
  CStdString strReturn;
  strReturn.Format("%s %i: %s%s%s%s%s%s%s%s%s%s%s",g_localizeStrings.Get(13169),iFrontPort, 
    bPad ? g_localizeStrings.Get(13163):"", bPad && bKeyb ? ",":"", 
    bKeyb ? g_localizeStrings.Get(13164):"", bKeyb && bMouse ? ",":"",
    bMouse ? g_localizeStrings.Get(13165):"", bMouse && (bHeadSet || bMic) ? ",":"",
    bHeadSet || bMic ? g_localizeStrings.Get(13166):"", (bHeadSet || bMic) && bMem ? ",":"",
    bMem ? g_localizeStrings.Get(13167):"", bMem && bIR ? ",":"",
    bIR ? g_localizeStrings.Get(13168):""
    );
  
  return strReturn;
}

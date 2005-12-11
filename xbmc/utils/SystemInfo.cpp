//////////////////////////////////////////
//		XBOX Hardware Info				        //
//	   01.02.2005 GeminiServer			    //
//////////////////////////////////////////
#include "../stdafx.h"
#include "SystemInfo.h"
#include "../xbox/XKUtils.h"
#include "../xbox/xkhdd.h"
#include "../xbox/xkeeprom.h"
#include "../xbox/XKflash.h"
#include "../xbox/XKRC4.h"
#include "../utils/md5.h"
#include "../settings.h"
#include "../utils/log.h"
#include "../xbox/Undocumented.h"


// SMART Attributes				   //Req: ID:	Offset:		Name of attribute:					Description:						
PCHAR	pAttrNames[] = {		   //----------------------------------------------------------------------------------------------------------------------------	
	"No Attribute Here          ", //0		0		0		No Attribute Here					No description here		
	"Raw Read Error Rate        ", //1		1		1		Raw Read Error Rate					Frequency of errors appearance while reading RAW data from a disk	
	"Throughput Performance     ", //2		2		2		Throughput Performance				The average efficiency of hard disk	
	"Spin Up Time               ", //3		3		3		Spin Up Time						Time needed by spindle to spin-up	
	"Start/Stop Count           ", //4		4		4		Start/Stop Count					Number of start/stop cycles of spindle	
	"Reallocated Sector Count   ", //5		5		5		Reallocated Sector Count			Quantity of remapped sectors	
	"Read Channel Margin        ", //6		6		6		Read Channel Margin					Reserve of channel while reading	
	"Seek Error Rate            ", //7		7		7		Seek Error Rate						Frequency of errors appearance while positioning	
	"Seek Time Performance      ", //8		8		8		Seek Time Performance				The average efficiency of operations while positioning	
	"Power On Hours Count       ", //9		9		9		Power-On Hours Count				Quantity of elapsed hours in the switched-on state	
	"Spin Retry Count           ", //10		10		A		Spin-up Retry Count					Number of attempts to start a spindle of a disk	
	"Calibration Retry Count    ", //11		11		B		Calibration Retry Count				Number of attempts to calibrate a drive	
	"Power Cycle Count          ", //12		12		C		Power Cycle Count					Number of complete start/stop cycles of hard disk	
	"Soft Read Error Rate       ", //13		13		D		Soft Read Error Rate				Frequency of "program" errors appearance while reading data from a disk	
	"G-Sense Error Rate         ", //14		191		BF		G-Sense Error Rate					Frequency of mistakes appearance as a result of impact loads	
	"Power-Off Retract Cycle    ", //15		192		C0		Power-Off Retract Cycle				Number of the fixed 'turning off drive' cycles (Fujitsu: Emergency Retract Cycle Count)	
	"Load/Unload Cycle Count    ", //16		193		C1		Load/Unload Cycle Count				Number of cycles into Landing Zone position	
	"HDA Temperature            ", //17		194		C2		HDA Temperature						Temperature of a Hard Disk Assembly	
	"Hardware ECC Recovered     ", //18		195		C3		Hardware ECC Recovered				Frequency of the on the fly errors (Fujitsu: ECC On The Fly Count)	
	"Reallocated Event Count    ", //19		196		C4		Reallocated Event Count				Quantity of remapping operations	
	"CurrentPending SecrCnt     ", //20		197		C5		Current Pending Sector Count		Current quantity of unstable sectors (waiting for remapping)	
	"Off-LineUncorrectableCnt   ", //21		198		C6		Off-line Scan Uncorrectable Count	Quantity of uncorrected errors	
	"UltraDMA CRC Error Rate    ", //22		199		C7		UltraDMA CRC Error Rate				Total quantity of errors CRC during UltraDMA mode	
	"Write Error Rate           ", //23		200		C8		Write Error Rate					Frequency of errors appearance while recording data into disk (Western Digital: Multi Zone Error Rate)	
	"Soft Read Error Rate       ", //24		201		C9		Soft Read Error Rate				Frequency of the off track errors (Maxtor: Off Track Errors)	
	"Data Address Mark Errors   ", //25		202		CA		Data Address Mark Errors			Frequency of the Data Address Mark errors	
	"Run Out Cancel             ", //26		203		CB		Run Out Cancel						Frequency of the ECC errors (Maxtor: ECC Errors)	
	"Soft ECC Correction        ", //27		204		CC		Soft ECC Correction					Quantity of errors corrected by software ECC	
	"Thermal Asperity Rate      ", //28		205		CD		Thermal Asperity Rate				Frequency of the thermal asperity errors	
	"Flying Height              ", //29		206		CE		Flying Height						The height of the disk heads above the disk surface	
	"Spin High Current          ", //30		207		CF		Spin High Current					Quantity of used high current to spin up drive	
	"Spin Buzz                  ", //31		208		D0		Spin Buzz							Quantity of used buzz routines to spin up drive	
	"Offline Seek Performance   ", //32		209		D1		Offline Seek Performance			Drive's seek performance during offline operations	
	"Disk Shift                 ", //34		220		DC		Disk Shift							Shift of disk is possible as a result of strong shock loading in the store, as a result of it's falling or for other reasons (sometimes: Temperature)	
	"G-Sense Error              ", //35		221		DD		G-Sense Error						Rate	This attribute is an indication of shock-sensitive sensor - total quantity of errors appearance as a result of impact loads (dropping drive, for example)	
	"Loaded Hours               ", //36		222		DE		Loaded Hours						Loading on drive caused by the general operating time of hours it stores	
	"Load/Unload Retry Count    ", //37		223		DF		Load/Unload Retry Count				Loading on drive caused by numerous recurrences of operations like: reading, recording, positioning of heads, etc.	
	"Load Friction              ", //38		224		E0		Load Friction						Loading on drive caused by friction in mechanical parts of the store	
	"Load/Unload Cycle Count    ", //39		225		E1		Load/Unload Cycle Count				Total of cycles of loading on drive	
	"Load-in Time               ", //40		226		E2		Load-in Time						General time of loading for drive	
	"Torque Amplification Cnt   ", //41		227		E3		Torque Amplification Count			Quantity efforts of the rotating moment of a drive	
	"Power-Off Retract Count    ", //42		228		E4		Power-Off Retract Count				Quantity of the fixed turning off's a drive	
	"GMR Head Amplitude         ", //43		230		E6		GMR Head Amplitude					Amplitude of heads trembling (GMR-head) in running mode	
	"Temperature                ", //44		231		E7		Temperature							Temperature of a drive	
	"Head Flying Hours          ", //45		240		F0		Head Flying Hours					Time while head is positioning	
	"Read Error Retry Rate      "  //46		250		FA		Read Error Retry Rate				Frequency of errors appearance while reading data from a disk	
};		

SYSINFO*			SYSINFO::_Instance = NULL;
CXBoxFlash			*mbFlash;

typedef struct 
{
    short num_Cylinders;
    short num_Heads;
    short num_SectorsPerTrack;
    short num_BytesPerSector;
} ideDisk;
struct Bios
{
	char Name[200];
	char Signature[200];
} Listone[1000];

int ideDebug = 1;
int numDrives = 1;
ideDisk drives[2];
char RetStrNomeBios[255];
char RetStrSignBios[255];
char MD5_Sign[16];

// Folder where the Bios Detections Files Are!
char* cTempBIOSFile =	"Q:\\System\\SystemInfo\\BiosBackup.bin";
char* cBIOSmd5IDs	=	"Q:\\System\\SystemInfo\\BiosIDs.ini";

static char* MDPrint (MD5_CTX_N *mdContext);
static char* MD5File(char *filename,long PosizioneInizio,int KBytes);

SYSINFO* SYSINFO::Instance()
{
  if (_Instance == NULL)
  {
    _Instance = new SYSINFO();
  }
  return _Instance;
}
SYSINFO::SYSINFO()
{}
SYSINFO::~SYSINFO(void)
{
	_Instance = NULL;
}

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

char* MDPrint (MD5_CTX_N *mdContext)
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
char* MD5File(char *filename,long PosizioneInizio,int KBytes)
{
  FILE *inFile = fopen (filename, "rb");
  MD5_CTX_N mdContext;
  unsigned int bytes;
  unsigned char *data;
  if (inFile == NULL) 
  {
    printf ("%s can't be opened.\n", filename);
    return NULL;
  }
  data	= (unsigned char*) malloc (1024*KBytes);
  fseek(inFile,PosizioneInizio,SEEK_SET);
  
  MD5Init (&mdContext);
  bytes	= fread (data, 1, 1024*KBytes, inFile);
  
  MD5Update(&mdContext, data, bytes);
  MD5Final (&mdContext);
  strcpy(MD5_Sign,MDPrint (&mdContext));
  fclose (inFile);
  free (data);
  return (strupr(MD5_Sign));
}
CStdString SYSINFO::MD5FileNew(char *filename,long PosizioneInizio,int KBytes)
{
  CStdString strReturn;
  FILE *inFile = fopen (filename, "rb");
  MD5_CTX_N mdContext;  int bytes;  UCHAR *data;
  data	= (UCHAR*) malloc (1024*KBytes);
  fseek(inFile,PosizioneInizio,SEEK_SET);
  MD5Init(&mdContext);
  bytes	= fread(data, 1, 1024*KBytes, inFile);
  MD5Update(&mdContext, data, bytes);
  MD5Final (&mdContext);
  strReturn.Format("%s",MDPrint(&mdContext));
  fclose(inFile);
  free(data);
  return strReturn;
}
CStdString SYSINFO::GetAVPackInfo()
{	//AV-Pack Detection PICReg(0x04)
	int cAVPack; 
	HalReadSMBusValue(0x20,XKUtils::PIC16L_CMD_AV_PACK,0,(LPBYTE)&cAVPack);
		
		 if (cAVPack == XKUtils::AV_PACK_SCART)		return "SCART";				
	else if (cAVPack == XKUtils::AV_PACK_HDTV)		return "HDTV";				
	else if (cAVPack == XKUtils::AV_PACK_VGA)		return "VGA";				
	else if (cAVPack == XKUtils::AV_PACK_RFU)		return "RFU";				
	else if (cAVPack == XKUtils::AV_PACK_SVideo)	return "S-Video";			
	else if (cAVPack == XKUtils::AV_PACK_Undefined)	return "Undefined";			
	else if (cAVPack == XKUtils::AV_PACK_Standard)	return "Standard RGB";			
	else if (cAVPack == XKUtils::AV_PACK_Missing)	return "Missing or Unknown";
	else return "Unknown";
}
CStdString SYSINFO::GetVideoEncoder()
{	
	// XBOX Video Encoder Detection GeminiServer
	int iTemp;
	if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_CONNEXANT,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0)
	{	CLog::Log(LOGDEBUG, "Video Encoder: CONNEXANT");	return "CONNEXANT";	}
	if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_FOCUS,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0) 
	{	CLog::Log(LOGDEBUG, "Video Encoder: FOCUS");		return "FOCUS";		}
	if (HalReadSMBusValue(XKUtils::SMBDEV_VIDEO_ENCODER_XCALIBUR,XKUtils::VIDEO_ENCODER_CMD_DETECT,0,(LPBYTE)&iTemp)==0) 
	{	CLog::Log(LOGDEBUG, "Video Encoder: XCALIBUR");		return "XCALIBUR";	}
	else {	CLog::Log(LOGDEBUG, "Video Encoder: UNKNOWN");	return "UNKNOWN";	}
}

CStdString SYSINFO::GetModCHIPDetected()
{
	mbFlash=new(CXBoxFlash);
	{
		// Known XBOX ModCHIP IDs&Names
		mbFlash->AddFCI(0x01,0xAD,"XECUTER 3",0x100000);
		mbFlash->AddFCI(0x01,0xD5,"XECUTER 2",0x100000);
		mbFlash->AddFCI(0x01,0xC4,"XENIUM",0x100000);
		mbFlash->AddFCI(0x01,0xC4,"XENIUM",0x000000);
		mbFlash->AddFCI(0x04,0xBA,"ALX2+ R3 FLASH",0x40000);
		// XBOX Possible FLash CHIPs
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
  else {	CLog::Log(LOGDEBUG, "- Detected TSOP/MOdCHIP: Unknown");	strTemp2 = "Unknown"; }
  
  if (strTemp1 != strTemp2) strTemp.Format("%s %s",strTemp1.c_str(),strTemp2.c_str());
  else strTemp = strTemp2;
  
  return strTemp;
}

CStdString SYSINFO::SmartXXModCHIP()
{	
  // SmartXX ModChip Detection Routine! 13.04.2005 GeminiServer
  unsigned char uSmartXX_ID = ((inb(0xf701)) & 0xf);
  if ( uSmartXX_ID == 1 )      // SmartXX V1+V2
	{
		CLog::Log(LOGDEBUG, "- Detected ModChip: SmartXX V1/V2");
		return "SmartXX V1/V2";
	}
  else if ( uSmartXX_ID == 2 ) // SmartXX V1+V2
	{
		CLog::Log(LOGDEBUG, "- Detected ModChip: SmartXX V1/V2");
		return "SmartXX V1/V2";
	}
	else if ( uSmartXX_ID == 5 ) // SmartXX OPX
	{
		CLog::Log(LOGDEBUG, "- Detected ModChip: SmartXX OPX");
		return "SmartXX OPX";
	}
  else if ( uSmartXX_ID == 8 ) // SmartXX V3
	{
		CLog::Log(LOGDEBUG, "- Detected ModChip: SmartXX V3");
		return "SmartXX V3";
	}
	else 
	{
		//CLog::Log(LOGDEBUG, "Detected ModCHIP: No SmartXX Detected!");
		return "None";
	}
}
UCHAR SYSINFO::IdeRead(USHORT port) 
{
  UCHAR rval;
  _asm 
  { 
    mov dx,   port 
    in  al,   dx 
    mov rval, al
  }
  return rval;
}
BYTE SYSINFO::GetSmartValues(int SmartREQ)
{
	BYTE retVal = 0;
	XKHDD::ATA_COMMAND_OBJ hddcommand;
	ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));
	
	hddcommand.DATA_BUFFSIZE = 0;
	hddcommand.IPReg.bFeaturesReg		= SMART_READ_ATTRIBUTE_VALUES;	// SEND READ SMART VALUES
	hddcommand.IPReg.bSectorCountReg	= 1;					
	hddcommand.IPReg.bSectorNumberReg	= 1;
	hddcommand.IPReg.bCylLowReg			= SMART_CYL_LOW;				//SET SMART CYL LOW
	hddcommand.IPReg.bCylHighReg		= SMART_CYL_HI;					//SET SMART CYL HI
	hddcommand.IPReg.bDriveHeadReg		= IDE_DEVICE_MASTER;			//SET Device Where HDD is
	hddcommand.IPReg.bCommandReg		= IDE_EXECUTE_SMART_FUNCTION;	//SET ON IDE SEND SMART MODE;
	
	if (XKHDD::SendATACommand(IDE_PRIMARY_PORT, &hddcommand, IDE_COMMAND_READ))
	{	
		int	i;
		BYTE Attr;
		PDRIVEATTRIBUTE	pDA;
		PATTRTHRESHOLD	pAT;
		pDA = (PDRIVEATTRIBUTE)&hddcommand.DATA_BUFFER[2];
		pAT = (PATTRTHRESHOLD)&hddcommand.DATA_BUFFER[2];
		for (i = 0; i < MAX_KNOWN_ATTRIBUTES; i++)
		{	Attr = pDA->bAttrID;
			if (Attr == 191) Attr =14;	if (Attr == 192) Attr =15;	if (Attr == 192) Attr =15;	if (Attr == 193) Attr =16;
			if (Attr == 195) Attr =18;	if (Attr == 196) Attr =19;	if (Attr == 197) Attr =20;	if (Attr == 250) Attr =45;
			if (Attr == 198) Attr =21;	if (Attr == 199) Attr =22;	if (Attr == 200) Attr =23;	if (Attr == 201) Attr =24;
			if (Attr == 202) Attr =25;	if (Attr == 203) Attr =26;	if (Attr == 204) Attr =27;	if (Attr == 205) Attr =28;
			if (Attr == 206) Attr =29;	if (Attr == 207) Attr =30;	if (Attr == 208) Attr =31;	if (Attr == 209) Attr =32;
			if (Attr == 220) Attr =33;	if (Attr == 221) Attr =34;	if (Attr == 222) Attr =35;	if (Attr == 223) Attr =36;
			if (Attr == 224) Attr =37;	if (Attr == 225) Attr =38;	if (Attr == 226) Attr =39;	if (Attr == 227) Attr =40;
			if (Attr == 228) Attr =41;	if (Attr == 230) Attr =42;	if (Attr == 231) Attr =43;	if (Attr == 240) Attr =44;
			if (Attr == 194) Attr =17;	
			if (Attr)
			{
				if (Attr > MAX_KNOWN_ATTRIBUTES)	Attr = MAX_KNOWN_ATTRIBUTES+1;
				if (Attr == SmartREQ ) retVal = (pDA->bWorstValue);	// S.M.A.R.T Get HDD Temp ID: 194 Offset: C2
			}
			pDA++;
			pAT++;
		}
		return retVal;
		CLog::Log(LOGDEBUG, "HDD S.M.A.R.T Request: %d°C",retVal);
	}
	else return 0;
}
bool SYSINFO::CheckBios(CStdString& strDetBiosNa)
{	
	FILE *fp;
	BYTE data;
	char *BIOS_Name;
	double loop;
	int BiosTrovato,i;
	DWORD addr			=	FLASH_BASE_ADDRESS;
	DWORD addr_kernel	=	KERNEL_BASE_ADDRESS;
	mbFlash				=	new(CXBoxFlash);
	BiosTrovato			=	0;
	BIOS_Name			=	(char*) malloc(100);
	
	if((fp=fopen(cTempBIOSFile,"wb"))!=NULL)
		{
		for(loop=0;loop<0x100000;loop++)
			{
				data = mbFlash->Read(addr++);
				fwrite(&data,1,sizeof(BYTE),fp);
			}
		fclose(fp);
		if (!LoadBiosSigns()) return false;
		
		// Detect a 1024 KB Bios MD5
		MD5File (cTempBIOSFile,0,1024);
		strcpy(BIOS_Name,CheckMD5(MD5_Sign));
		if ( strcmp(BIOS_Name,"Unknown") == 0)
		{
			// Detect a 512 KB Bios MD5
			MD5File (cTempBIOSFile,0,512);
			strcpy(BIOS_Name,CheckMD5(MD5_Sign));
			if ( strcmp(BIOS_Name,"Unknown") == 0)
			{
				// Detect a 256 KB Bios MD5
				MD5File (cTempBIOSFile,0,256);
				strcpy(BIOS_Name,CheckMD5(MD5_Sign));
				if ( strcmp(BIOS_Name,"Unknown") != 0)
				{
					CLog::Log(LOGDEBUG, "- Detected BIOS [256 KB]: %hs",BIOS_Name);
					CLog::Log(LOGDEBUG, "- BIOS MD5 Hash: %hs",MD5_Sign);
					strDetBiosNa = BIOS_Name;
					free(BIOS_Name);
					return true;					
				}
				else
				{
					CLog::Log(LOGINFO, "------------------- BIOS Detection Log ------------------");
					// 256k Bios MD5
					if ( (MD5FileNew(cTempBIOSFile,0,256) == MD5FileNew(cTempBIOSFile,262144,256)) && (MD5FileNew(cTempBIOSFile,524288,256)== MD5FileNew(cTempBIOSFile,786432,256)) )
					{
							for (i=0;i<16; i++) MD5_Sign[i]='\0';
							MD5File(cTempBIOSFile,0,256);
							CLog::Log(LOGINFO, "256k BIOSES: Checksums are (256)");					
							CLog::Log(LOGINFO, "  1.Bios > %hs",CheckMD5(MD5_Sign));
							CLog::Log(LOGINFO, "  Add this to BiosIDs.ini: (256)BiosNameHere = %hs",MD5_Sign);
							CLog::Log(LOGINFO, "---------------------------------------------------------");
							strDetBiosNa = "Unknown! Add. MD5 from Log to BiosIDs.ini!";
							free(BIOS_Name);
							return true;
					}
					else
					{	CLog::Log(LOGINFO, "- BIOS: This is not a 256KB Bios!");
						// 512k Bios MD5
						if ((MD5FileNew(cTempBIOSFile,0,512)) == (MD5FileNew(cTempBIOSFile,524288,512)))
						{
							for (i=0;i<16; i++) MD5_Sign[i]='\0';
							MD5File(cTempBIOSFile,0,512);
							CLog::Log(LOGINFO, "512k BIOSES: Checksums are (512)");					
							CLog::Log(LOGINFO, "  1.Bios > %hs",CheckMD5(MD5_Sign));
							CLog::Log(LOGINFO, "  Add. this to BiosIDs.ini: (512)BiosNameHere = %hs",MD5_Sign);
							CLog::Log(LOGINFO, "---------------------------------------------------------");
							strDetBiosNa = "Unknown! Add. MD5 from Log to BiosIDs.ini!";
							free(BIOS_Name);
							return true;
						}
						else 
						{
							CLog::Log(LOGINFO, "- BIOS: This is not a 512KB Bios!");
							// 1024k Bios MD5
							for (i=0;i<16; i++) MD5_Sign[i]='\0';
							MD5File(cTempBIOSFile,0,1024);
							CLog::Log(LOGINFO, "1024k BIOS: Checksums are (1MB)");
							CLog::Log(LOGINFO, "  1.Bios > %hs",CheckMD5(MD5_Sign));
							CLog::Log(LOGINFO, "  Add. this to BiosIDs.ini: (1MB)BiosNameHere = %hs",MD5_Sign);
							CLog::Log(LOGINFO, "---------------------------------------------------------");
							strDetBiosNa = "Unknown! Add. MD5 from Log to BiosIDs.ini!";
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
				free(BIOS_Name);
				return true;
				
			} 
		}
		else
		{
			CLog::Log(LOGINFO, "- Detected BIOS [1024 KB]: %hs",BIOS_Name);
			CLog::Log(LOGINFO, "- BIOS MD5 Hash: %hs",MD5_Sign);
			strDetBiosNa = BIOS_Name;
			free(BIOS_Name);
			return true;
		}
	}
	else 
	{
		CLog::Log(LOGINFO, "BIOS FILE CREATION ERROR!");
		strDetBiosNa = "Detection Error!";
		free(BIOS_Name);
		return false;
	}
	free(BIOS_Name);
	return false;
}
bool SYSINFO::GetXBOXVersionDetected(CStdString& strXboxVer)
{
	unsigned int iTemp;
	char Ver[6];

	HalReadSMBusValue(0x20,0x01,0,(LPBYTE)&Ver[0]);
	HalReadSMBusValue(0x20,0x01,0,(LPBYTE)&Ver[1]);
	HalReadSMBusValue(0x20,0x01,0,(LPBYTE)&Ver[2]);
	Ver[3] = 0;	Ver[4] = 0;	Ver[5] = 0;

	if ( strcmp(Ver,("01D")) == NULL || strcmp(Ver,("D01")) == NULL || strcmp(Ver,("1D0")) == NULL || strcmp(Ver,("0D1")) == NULL)
	{ strXboxVer = "DEVKIT";  return true;}
	else if ( strcmp(Ver,("DBG")) == NULL){ strXboxVer = "DEBUGKIT Green";	return true;}
	else if ( strcmp(Ver,("P01")) == NULL){ strXboxVer = "v1.0";  return true;}
	else if ( strcmp(Ver,("P05")) == NULL){	strXboxVer = "v1.1";  return true;}
	else if ( strcmp(Ver,("P11")) == NULL ||  strcmp(Ver,("1P1")) == NULL || strcmp(Ver,("11P")) == NULL )
	{	
		if (HalReadSMBusValue(0xD4,0x00,0,(LPBYTE)&iTemp)==0){  strXboxVer = "v1.4";  return true; } 
		else {  strXboxVer = "v1.2/v1.3";		return true;}
	}
	else if ( strcmp(Ver,("???")) == NULL){	strXboxVer = "v1.5";  return true;} //Todo: Need XBOX v1.5 to Detect the ver.!!
	else if ( strcmp(Ver,("P2L")) == NULL){	strXboxVer = "v1.6";	return true;}
  else  {	strXboxVer.Format("UNKNOWN: Please report this --> %s",Ver); return true;
	}
}
bool SYSINFO::LoadBiosSigns()
{
	FILE *infile;
	int cntBioses;
	char stringone[255];

	if ((infile = fopen(cBIOSmd5IDs,"r")) == NULL)
	{ 
		CLog::Log(LOGDEBUG, "ERROR LOADING BIOSES.INI!!"); 
		return false;
	}
	else
	{
		cntBioses=0;
		do
		{
	fgets(stringone,255,infile);
			if  (stringone[0] != '#') 
			{
				if (strstr(stringone,"=")!= NULL)
				{
					strcpy(Listone[cntBioses].Name,SYSINFO::ReturnBiosName(stringone));
					strcpy(Listone[cntBioses].Signature,SYSINFO::ReturnBiosSign(stringone));
				cntBioses++;
				}
			}
		} while( !feof( infile ) );
		fclose(infile);
		strcpy(Listone[cntBioses++].Name,"\0");
		strcpy(Listone[cntBioses++].Signature,"\0");
		return true;
	}
}
bool SYSINFO::GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware)
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
		XKHDD::GetIDEModel(hddcommand.DATA_BUFFER, lpsDVDModel, &slen);
		CLog::Log(LOGDEBUG, "DVD Model: %s",lpsDVDModel);
		strDVDModel.Format("%s",lpsDVDModel);
		
		//Get DVD FirmWare...
		CHAR lpsDVDFirmware[100];
		ZeroMemory(&lpsDVDFirmware,100);
		XKHDD::GetIDEFirmWare(hddcommand.DATA_BUFFER, lpsDVDFirmware, &slen);
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
bool SYSINFO::GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState)
{
	XKHDD::ATA_COMMAND_OBJ hddcommand;
	DWORD slen = 0;
	ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));
	hddcommand.DATA_BUFFSIZE = 0;
	
	//Detect HDD Model...
	ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));
	hddcommand.IPReg.bDriveHeadReg = IDE_DEVICE_MASTER;
	hddcommand.IPReg.bCommandReg = IDE_ATA_IDENTIFY;
	if (XKHDD::SendATACommand(IDE_PRIMARY_PORT, &hddcommand, IDE_COMMAND_READ))
	{
		//Get Model Name
		CHAR lpsHDDModel[100];
		ZeroMemory(&lpsHDDModel,100);
		XKHDD::GetIDEModel(hddcommand.DATA_BUFFER, lpsHDDModel, &slen);
		strHDDModel.Format("%s",lpsHDDModel);

		//Get Serial...
		CHAR lpsHDDSerial[100];
		ZeroMemory(&lpsHDDSerial,100);
		XKHDD::GetIDESerial(hddcommand.DATA_BUFFER, lpsHDDSerial, &slen);
		strHDDSerial.Format("%s",lpsHDDSerial);

		//Get HDD FirmWare...
		CHAR lpsHDDFirmware[100];
		ZeroMemory(&lpsHDDFirmware,100);
		XKHDD::GetIDEFirmWare(hddcommand.DATA_BUFFER, lpsHDDFirmware, &slen);
		strHDDFirmware.Format("%s",lpsHDDFirmware);

		//Print HDD Password... [SomeThing Goes Wrong! Need analizing & Fixing!]
		CHAR lpsHDDPassword[100];
		UCHAR cHddPass[32];
		ZeroMemory(lpsHDDPassword, 100);
		ZeroMemory(cHddPass, 32);
		strupr(lpsHDDPassword);
		XKGeneral::BytesToHexStr(cHddPass, 20, lpsHDDPassword); 
		strHDDpw.Format("%s",lpsHDDPassword);
		
		//Get ATA Locked State
		DWORD SecStatus = XKHDD::GetIDESecurityStatus(hddcommand.DATA_BUFFER);
		if (!(SecStatus & IDE_SECURITY_SUPPORTED))
		{
			strHDDLockState.Format("%s","LOCKING NOT SUPPORTED");
			return true;
		}
		if ((SecStatus & IDE_SECURITY_SUPPORTED) && !(SecStatus & IDE_SECURITY_ENABLED))
		{
			strHDDLockState.Format("%s","NOT LOCKED");
			return true;
		}
		if ((SecStatus & IDE_SECURITY_SUPPORTED) && (SecStatus & IDE_SECURITY_ENABLED))
		{
			strHDDLockState.Format("%s","LOCKED");
			return true;
		}
		
		if (SecStatus & IDE_SECURITY_FROZEN)
		{
			strHDDLockState.Format("%s","FROZEN");
			return true;
		}
		
		if (SecStatus & IDE_SECURITY_COUNT_EXPIRED)
		{
			strHDDLockState.Format("%s","REQUIRES RESET");
			return true;
		}
	return true;
	}
	else return false;
}
bool SYSINFO::SmartXXLEDControll(int iSmartXXLED) 
{
	//SmartXX LED0/LED1 Controll Routine! 21.04.2005 GeminiServer
	//LED_R, LED_B, LED_RB, LED_RBCYCLE
	
	//We need to check b4 we use the SmartXX LED controll!!
	if ( (((inb(0xf701)) & 0xf) == 0x1) || (((inb(0xf701)) & 0xf) == 0x5) )
	{
		if (iSmartXXLED == SMARTXX_LED_OFF)
		{	
			outb(0xf702, 0xb);  //R:off	B:off
			CLog::Log(LOGDEBUG, "Setting SmartXX LED: BOTH OFF");
			return true;
		}
		else if (iSmartXXLED == SMARTXX_LED_RED)
		{	//Only Red:
			outb(0xf702, 0xb);  //R:off	B:off
			outb(0xf702, 0x1);  //R:on	B:off
			CLog::Log(LOGDEBUG, "Setting SmartXX LED: RED");
			return true;
		}
		else if (iSmartXXLED == SMARTXX_LED_BLUE)
		{	//Only Blue
			outb(0xf702, 0xb);  //R:off	B:off
			outb(0xf702, 0xa);  //R:off B:on
			CLog::Log(LOGDEBUG, "Setting SmartXX LED: BLUE");
			return true;
		}
		else if (iSmartXXLED == SMARTXX_LED_BLUE_RED)
		{	// RED and Blue
			outb(0xf702, 0xb);  //R:off	B:off
			outb(0xf702, 0x0);  //R:on	B:on 
			CLog::Log(LOGDEBUG, "Setting SmartXX LED: RED and BLUE");
			return true;
		}
		else if (iSmartXXLED == SMARTXX_LED_CYCLE)
		{
			CLog::Log(LOGDEBUG, "Setting SmartXX LED: 1x CYCLE");
			//while (iSmartXXLED == SMARTXX_LED_CYCLE)
			//{	// Blue and Red
				outb(0xf702, 0x0);  //R:on	B:on
				Sleep(100);
				outb(0xf702, 0x1);  //R:on	B:off
				Sleep(100);
				outb(0xf702, 0xa);  //R:off B:on
				Sleep(100);
				outb(0xf702, 0xb);  //R:off	B:off
				Sleep(100);
			//} 
			return true;
		}
		else
		{	
			outb(0xf702, 0xa);  //R:off B:on
			CLog::Log(LOGDEBUG, "Setting SmartXX LED: DEFAULT STATE --> RED");
			return true;	
		}
	}
	else return false;
}
void SYSINFO::Init_IDE()
{
    int ret;
    int errorCode;

    while (IdeRead(IDE_STATUS_REGISTER) != 0x50);

    if (ideDebug) CLog::Log(LOGDEBUG, "About to run drive Diagnosis\n");

   IdeWrite(IDE_COMMAND_REGISTER, IDE_COMMAND_DIAGNOSTIC);
    while (IdeRead(IDE_STATUS_REGISTER) & IDE_STATUS_DRIVE_BUSY);
    errorCode = IdeRead(IDE_ERROR_REGISTER);
    if (ideDebug > 1) CLog::Log(LOGDEBUG, "ide: ide error register = %x\n", errorCode);

    ret = readDriveConfig(0);
    if (ret < 0) {
        numDrives = 0;
	return;
    } else if (errorCode & 0x80) {
	if (ideDebug > 1) CLog::Log(LOGDEBUG, "ide: found one drive\n");
        numDrives = 1;
    } else {
	if (ideDebug > 1) CLog::Log(LOGDEBUG, "ide: found second drive\n");
        numDrives = 2;
	readDriveConfig(1);
    }
}

void SYSINFO::IdeWrite(USHORT port, UCHAR data) 
{
	_asm
	{
		mov dx, port
		mov al, data
		out dx, al
	}
}
double SYSINFO::GetCPUFrequence()
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
double SYSINFO::RDTSC(void)
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
	x*=4294967296;
	x+=a;
	return x;
}
int	SYSINFO::readDriveConfig(int drive)
{
    int i;
    int status;
    short info[256];
    long long int bytes;

    if (ideDebug > 1) CLog::Log(LOGDEBUG, "ide: about to read drive config for drive #%d\n",drive);

	IdeWrite(IDE_DRIVE_HEAD_REGISTER, (drive == 0) ? IDE_DRIVE_0 : IDE_DRIVE_1);
	IdeWrite(IDE_COMMAND_REGISTER, IDE_COMMAND_IDENTIFY_DRIVE);
    while ( IdeRead(IDE_STATUS_REGISTER)& IDE_STATUS_DRIVE_BUSY);
    
	status = IdeRead(IDE_STATUS_REGISTER);
    // simulate failure
    // status = 0x50;
    if ((status & IDE_STATUS_DRIVE_DATA_REQUEST)) 
	{
       CLog::Log(LOGDEBUG, "ide: probe found ATA drive\n");
       // drive responded to ATA probe
		for (i=0; i < 256; i++) 
		{
			info[i] = IdeRead(IDE_DATA_REGISTER); //0x01f0 0x01F0
		}
		drives[drive].num_Cylinders			= info[IDE_INDENTIFY_NUM_CYLINDERS];
		drives[drive].num_Heads				= info[IDE_INDENTIFY_NUM_HEADS];
		drives[drive].num_SectorsPerTrack	= info[IDE_INDENTIFY_NUM_SECTORS_TRACK];
		drives[drive].num_BytesPerSector	= info[IDE_INDENTIFY_NUM_BYTES_SECTOR];
    } 
	else 
	{
       // try for ATAPI
      IdeWrite(IDE_FEATURE_REG, 0);		// disable dma & overlap

      IdeWrite(IDE_DRIVE_HEAD_REGISTER, (drive == 0) ? IDE_DRIVE_0 : IDE_DRIVE_1);
      IdeWrite(IDE_COMMAND_REGISTER, IDE_COMMAND_ATAPI_IDENT_DRIVE);
       while (IdeRead(IDE_STATUS_REGISTER) & IDE_STATUS_DRIVE_BUSY);
       status = IdeRead(IDE_STATUS_REGISTER);
       CLog::Log(LOGDEBUG, "ide: found atapi drive\n");
       return -1;
    }

    CLog::Log(LOGDEBUG, "Found IDE: Drive %d\n", drive);
    CLog::Log(LOGDEBUG, "    %d cylinders, %d heads, %d sectors/tack, %d bytes/sector\n", 
        drives[drive].num_Cylinders, drives[drive].num_Heads,
        drives[drive].num_SectorsPerTrack, drives[drive].num_BytesPerSector);
    bytes = ((long long int )IDE_getNumBlocks(drive)) * 512;
    CLog::Log(LOGDEBUG, "    Disk has %d blocks (%l bytes)\n", IDE_getNumBlocks(drive), bytes);

    return 0;
}
int	SYSINFO::IDE_getNumBlocks(int driveNum)
{
    if (driveNum < 0 || driveNum > (numDrives-1)) {
        return IDE_ERROR_BAD_DRIVE;
    }

    return (drives[driveNum].num_Heads * 
            drives[driveNum].num_SectorsPerTrack *
	    drives[driveNum].num_Cylinders);
}

int	SYSINFO::IDE_Read(int driveNum, int blockNum, char *buffer)
{
    int i;
    int head;
    int sector;
    int cylinder;
    short *bufferW;
    int reEnable = 0;

    if (driveNum < 0 || driveNum > (numDrives-1)) {
	if (ideDebug) CLog::Log(LOGDEBUG, "ide: invalid drive %d\n", driveNum);
        return IDE_ERROR_BAD_DRIVE;
    }

    if (blockNum < 0 || blockNum >= IDE_getNumBlocks(driveNum)) {
	if (ideDebug) CLog::Log(LOGDEBUG, "ide: invalid block %d\n", blockNum);
        return IDE_ERROR_INVALID_BLOCK;
    }

    //if (Interrupts_Enabled()) {
	//Disable_Interrupts();
	//reEnable = 1;
    //}

    // now compute the head, cylinder, and sector
    sector = blockNum % drives[driveNum].num_SectorsPerTrack + 1;
    cylinder = blockNum / (drives[driveNum].num_Heads * 
     	drives[driveNum].num_SectorsPerTrack);
    head = (blockNum / drives[driveNum].num_SectorsPerTrack) % 
        drives[driveNum].num_Heads;

    if (ideDebug) {
	CLog::Log(LOGDEBUG, "request to read block %d\n", blockNum);
	CLog::Log(LOGDEBUG, "    head %d\n", head);
	CLog::Log(LOGDEBUG, "    cylinder %d\n", cylinder);
	CLog::Log(LOGDEBUG, "    sector %d\n", sector);
    }

   IdeWrite(IDE_SECTOR_COUNT_REGISTER, 1);
   IdeWrite(IDE_SECTOR_NUMBER_REGISTER, sector);
   IdeWrite(IDE_CYLINDER_LOW_REGISTER, LOW_BYTE(cylinder));
   IdeWrite(IDE_CYLINDER_HIGH_REGISTER, HIGH_BYTE(cylinder));
    if (driveNum == 0) {
	IdeWrite(IDE_DRIVE_HEAD_REGISTER, IDE_DRIVE_0 | head);
    } else if (driveNum == 1) {
	IdeWrite(IDE_DRIVE_HEAD_REGISTER, IDE_DRIVE_1 | head); 
    }

   IdeWrite(IDE_COMMAND_REGISTER, IDE_COMMAND_READ_SECTORS);

    if (ideDebug) CLog::Log(LOGDEBUG, "About to wait for Read \n");

    // wait for the drive
    while (IdeRead(IDE_STATUS_REGISTER) & IDE_STATUS_DRIVE_BUSY);

    if (IdeRead(IDE_STATUS_REGISTER) & IDE_STATUS_DRIVE_ERROR) {
	CLog::Log(LOGDEBUG, "ERROR: Got Read %d\n", IdeRead(IDE_STATUS_REGISTER));
	return IDE_ERROR_DRIVE_ERROR;
    }

    if (ideDebug) CLog::Log(LOGDEBUG, "got buffer \n");

    bufferW = (short *) buffer;
    for (i=0; i < 256; i++) {
        bufferW[i] = IdeRead(IDE_DATA_REGISTER);
    }

    //if (reEnable) Enable_Interrupts();

    return IDE_ERROR_NO_ERROR;
}
int	SYSINFO::IDE_Write(int driveNum, int blockNum, char *buffer)
{
    int i;
    int head;
    int sector;
    int cylinder;
    char *bufferW;
    int reEnable = 0;

    if (driveNum < 0 || driveNum > (numDrives-1)) 
	{
        return IDE_ERROR_BAD_DRIVE;
    }

    if (blockNum < 0 || blockNum >= IDE_getNumBlocks(driveNum)) 
	{
        return IDE_ERROR_INVALID_BLOCK;
    }

	//if (Interrupts_Enabled()) {
	//Disable_Interrupts();
	//reEnable = 1;
    //}
    // now compute the head, cylinder, and sector
    sector = blockNum % drives[driveNum].num_SectorsPerTrack + 1;
    cylinder = blockNum / (drives[driveNum].num_Heads * 
     	drives[driveNum].num_SectorsPerTrack);
    head = (blockNum / drives[driveNum].num_SectorsPerTrack) % 
        drives[driveNum].num_Heads;

    if (ideDebug) {
	CLog::Log(LOGDEBUG, "request to write block %d\n", blockNum);
	CLog::Log(LOGDEBUG, "    head %d\n", head);
	CLog::Log(LOGDEBUG, "    cylinder %d\n", cylinder);
	CLog::Log(LOGDEBUG, "    sector %d\n", sector);
    }

   IdeWrite(IDE_SECTOR_COUNT_REGISTER, 1);
   IdeWrite(IDE_SECTOR_NUMBER_REGISTER, sector);
   IdeWrite(IDE_CYLINDER_LOW_REGISTER, LOW_BYTE(cylinder));
   IdeWrite(IDE_CYLINDER_HIGH_REGISTER, HIGH_BYTE(cylinder));
    if (driveNum == 0) {
	IdeWrite(IDE_DRIVE_HEAD_REGISTER, IDE_DRIVE_0 | head);
    } else if (driveNum == 1) {
	IdeWrite(IDE_DRIVE_HEAD_REGISTER, IDE_DRIVE_1 | head);
    }
   IdeWrite(IDE_COMMAND_REGISTER, IDE_COMMAND_WRITE_SECTORS);
    
   // wait for the drive
    while (IdeRead(IDE_STATUS_REGISTER) & IDE_STATUS_DRIVE_BUSY);
    bufferW =  buffer;
    for (i=0; i < 256; i++) 
	{
        IdeWrite(IDE_DATA_REGISTER, bufferW[i]);
    }
    
	if (ideDebug) CLog::Log(LOGDEBUG, "About to wait for Write \n");

	// wait for the drive
    while (IdeRead(IDE_STATUS_REGISTER) & IDE_STATUS_DRIVE_BUSY);

    if (IdeRead(IDE_STATUS_REGISTER) & IDE_STATUS_DRIVE_ERROR) {
	CLog::Log(LOGDEBUG, "ERROR: Got Read %d\n", IdeRead(IDE_STATUS_REGISTER));
	return IDE_ERROR_DRIVE_ERROR;
    }
    //if (reEnable) Enable_Interrupts();
    return IDE_ERROR_NO_ERROR;
}
char* SYSINFO::ReturnBiosName(char *str)
{
	int cnt1,cnt2,i;
	cnt1=cnt2=0;

	for (i=0;i<255;i++) RetStrNomeBios[i]='\0';
	if ( (strstr(str,"(1MB)")==0) || (strstr(str,"(512)")==0) || (strstr(str,"(256)")==0) )
		cnt2=5;

	while (str[cnt2] != '=')
	{
		RetStrNomeBios[cnt1]=str[cnt2];
		cnt1++;
		cnt2++;
	}
	RetStrNomeBios[cnt1++]='\0';
	return (RetStrNomeBios);
}
char* SYSINFO::ReturnBiosSign(char *str)
{
	int cnt1,cnt2,i;
	cnt1=cnt2=0;
	for (i=0;i<255;i++) RetStrSignBios[i]='\0';
	while (str[cnt2] != '=') cnt2++;
	cnt2++;
	while (str[cnt2] != NULL)
	{
		if ( str[cnt2] != ' ' )
		{
			RetStrSignBios[cnt1]=str[cnt2];
			cnt1++;
			cnt2++;
		}
		else cnt2++;
	}
	RetStrSignBios[cnt1++]='\0';
	return (RetStrSignBios);
}
char* SYSINFO::CheckMD5 (char *Sign)
{
	int cntBioses;
	cntBioses=0;
	do 
	{	
		if  (strstr(Listone[cntBioses].Signature,Sign) != NULL)
		{	return (Listone[cntBioses].Name);		}
		cntBioses++;
	} 
	while( strcmp(Listone[cntBioses].Name,"\0") != 0);
	return ("Unknown");
}
//GeminiServer: System Up Time in Minutes 
// Will return the time since the system was started!
// Input Value int: Minutes  Return Values int: Minutes, Hours, Days
bool SYSINFO::SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays)
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
  /*
  if (iDays >= 7) // Weeks
  {
    iWeeks = iDays / 7;
    iDays = iDays - (iDays * 7);
  }
  if (iWeeks >= 4) // Months
  {
    iMonth = iWeeks / 4;
    iMonth = iMonth - (iMonth * 4);
  }
  if (iMonth >= 12) // Years
  {
    iMonth = iWeeks / 4;
    iMonth = iMonth - (iMonth * 4);
  }
  */

  return true;
}
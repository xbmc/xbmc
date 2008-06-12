/* 
   Unix SMB/Netbios implementation.

   Copyright (C) Andrew Tridgell              1992-2000,
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000,
   Copyright (C) Jean Francois Micouleau      1998-2000.
   Copyright (C) Gerald Carter                2001-2006.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _RPC_SPOOLSS_H		/* _RPC_SPOOLSS_H */
#define _RPC_SPOOLSS_H

/* spoolss pipe: this are the calls which are not implemented ...
#define SPOOLSS_GETPRINTERDRIVER			0x0b
#define SPOOLSS_READPRINTER				0x16
#define SPOOLSS_WAITFORPRINTERCHANGE			0x1c
#define SPOOLSS_ADDPORT					0x25
#define SPOOLSS_CONFIGUREPORT				0x26
#define SPOOLSS_DELETEPORT				0x27
#define SPOOLSS_CREATEPRINTERIC				0x28
#define SPOOLSS_PLAYGDISCRIPTONPRINTERIC		0x29
#define SPOOLSS_DELETEPRINTERIC				0x2a
#define SPOOLSS_ADDPRINTERCONNECTION			0x2b
#define SPOOLSS_DELETEPRINTERCONNECTION			0x2c
#define SPOOLSS_PRINTERMESSAGEBOX			0x2d
#define SPOOLSS_ADDMONITOR				0x2e
#define SPOOLSS_DELETEMONITOR				0x2f
#define SPOOLSS_DELETEPRINTPROCESSOR			0x30
#define SPOOLSS_ADDPRINTPROVIDOR			0x31
#define SPOOLSS_DELETEPRINTPROVIDOR			0x32
#define SPOOLSS_FINDFIRSTPRINTERCHANGENOTIFICATION	0x36
#define SPOOLSS_FINDNEXTPRINTERCHANGENOTIFICATION	0x37
#define SPOOLSS_ROUTERFINDFIRSTPRINTERNOTIFICATIONOLD	0x39
#define SPOOLSS_ADDPORTEX				0x3d
#define SPOOLSS_REMOTEFINDFIRSTPRINTERCHANGENOTIFICATION0x3e
#define SPOOLSS_SPOOLERINIT				0x3f
#define SPOOLSS_RESETPRINTEREX				0x40
*/

/* those are implemented */
#define SPOOLSS_ENUMPRINTERS				0x00
#define SPOOLSS_OPENPRINTER				0x01
#define SPOOLSS_SETJOB					0x02
#define SPOOLSS_GETJOB					0x03
#define SPOOLSS_ENUMJOBS				0x04
#define SPOOLSS_ADDPRINTER				0x05
#define SPOOLSS_DELETEPRINTER				0x06
#define SPOOLSS_SETPRINTER				0x07
#define SPOOLSS_GETPRINTER				0x08
#define SPOOLSS_ADDPRINTERDRIVER			0x09
#define SPOOLSS_ENUMPRINTERDRIVERS			0x0a
#define SPOOLSS_GETPRINTERDRIVERDIRECTORY		0x0c
#define SPOOLSS_DELETEPRINTERDRIVER			0x0d
#define SPOOLSS_ADDPRINTPROCESSOR			0x0e
#define SPOOLSS_ENUMPRINTPROCESSORS			0x0f
#define SPOOLSS_GETPRINTPROCESSORDIRECTORY		0x10
#define SPOOLSS_STARTDOCPRINTER				0x11
#define SPOOLSS_STARTPAGEPRINTER			0x12
#define SPOOLSS_WRITEPRINTER				0x13
#define SPOOLSS_ENDPAGEPRINTER				0x14
#define SPOOLSS_ABORTPRINTER				0x15
#define SPOOLSS_ENDDOCPRINTER				0x17
#define SPOOLSS_ADDJOB					0x18
#define SPOOLSS_SCHEDULEJOB				0x19
#define SPOOLSS_GETPRINTERDATA				0x1a
#define SPOOLSS_SETPRINTERDATA				0x1b
#define SPOOLSS_CLOSEPRINTER				0x1d
#define SPOOLSS_ADDFORM					0x1e
#define SPOOLSS_DELETEFORM				0x1f
#define SPOOLSS_GETFORM					0x20
#define SPOOLSS_SETFORM					0x21
#define SPOOLSS_ENUMFORMS				0x22
#define SPOOLSS_ENUMPORTS				0x23
#define SPOOLSS_ENUMMONITORS				0x24
#define SPOOLSS_ENUMPRINTPROCDATATYPES			0x33
#define SPOOLSS_RESETPRINTER				0x34
#define SPOOLSS_GETPRINTERDRIVER2			0x35
#define SPOOLSS_FCPN					0x38	/* FindClosePrinterNotify */
#define SPOOLSS_REPLYOPENPRINTER			0x3a
#define SPOOLSS_ROUTERREPLYPRINTER			0x3b
#define SPOOLSS_REPLYCLOSEPRINTER			0x3c
#define SPOOLSS_RFFPCNEX				0x41	/* RemoteFindFirstPrinterChangeNotifyEx */
#define SPOOLSS_RRPCN					0x42	/* RouteRefreshPrinterChangeNotification */
#define SPOOLSS_RFNPCNEX				0x43	/* RemoteFindNextPrinterChangeNotifyEx */
#define SPOOLSS_OPENPRINTEREX				0x45
#define SPOOLSS_ADDPRINTEREX				0x46
#define SPOOLSS_ENUMPRINTERDATA				0x48
#define SPOOLSS_DELETEPRINTERDATA			0x49
#define SPOOLSS_SETPRINTERDATAEX			0x4d
#define SPOOLSS_GETPRINTERDATAEX			0x4e
#define SPOOLSS_ENUMPRINTERDATAEX			0x4f
#define SPOOLSS_ENUMPRINTERKEY				0x50
#define SPOOLSS_DELETEPRINTERDATAEX			0x51
#define SPOOLSS_DELETEPRINTERKEY			0x52
#define SPOOLSS_DELETEPRINTERDRIVEREX			0x54
#define SPOOLSS_XCVDATAPORT				0x58
#define SPOOLSS_ADDPRINTERDRIVEREX			0x59

/* 
 * Special strings for the OpenPrinter() call.  See the MSDN DDK
 * docs on the XcvDataPort() for more details.
 */

#define SPL_LOCAL_PORT            "Local Port"
#define SPL_TCPIP_PORT            "Standard TCP/IP Port"
#define SPL_XCV_MONITOR_LOCALMON  ",XcvMonitor Local Port"
#define SPL_XCV_MONITOR_TCPMON    ",XcvMonitor Standard TCP/IP Port"


#define PRINTER_CONTROL_UNPAUSE		0x00000000
#define PRINTER_CONTROL_PAUSE		0x00000001
#define PRINTER_CONTROL_RESUME		0x00000002
#define PRINTER_CONTROL_PURGE		0x00000003
#define PRINTER_CONTROL_SET_STATUS	0x00000004

#define PRINTER_STATUS_OK               0x00000000
#define PRINTER_STATUS_PAUSED		0x00000001
#define PRINTER_STATUS_ERROR		0x00000002
#define PRINTER_STATUS_PENDING_DELETION	0x00000004
#define PRINTER_STATUS_PAPER_JAM	0x00000008

#define PRINTER_STATUS_PAPER_OUT	0x00000010
#define PRINTER_STATUS_MANUAL_FEED	0x00000020
#define PRINTER_STATUS_PAPER_PROBLEM	0x00000040
#define PRINTER_STATUS_OFFLINE		0x00000080

#define PRINTER_STATUS_IO_ACTIVE	0x00000100
#define PRINTER_STATUS_BUSY		0x00000200
#define PRINTER_STATUS_PRINTING		0x00000400
#define PRINTER_STATUS_OUTPUT_BIN_FULL	0x00000800

#define PRINTER_STATUS_NOT_AVAILABLE	0x00001000
#define PRINTER_STATUS_WAITING		0x00002000
#define PRINTER_STATUS_PROCESSING	0x00004000
#define PRINTER_STATUS_INITIALIZING	0x00008000

#define PRINTER_STATUS_WARMING_UP	0x00010000
#define PRINTER_STATUS_TONER_LOW	0x00020000
#define PRINTER_STATUS_NO_TONER		0x00040000
#define PRINTER_STATUS_PAGE_PUNT	0x00080000

#define PRINTER_STATUS_USER_INTERVENTION	0x00100000
#define PRINTER_STATUS_OUT_OF_MEMORY	0x00200000
#define PRINTER_STATUS_DOOR_OPEN	0x00400000
#define PRINTER_STATUS_SERVER_UNKNOWN	0x00800000

#define PRINTER_STATUS_POWER_SAVE	0x01000000

#define SERVER_ACCESS_ADMINISTER	0x00000001
#define SERVER_ACCESS_ENUMERATE		0x00000002
#define PRINTER_ACCESS_ADMINISTER	0x00000004
#define PRINTER_ACCESS_USE		0x00000008
#define JOB_ACCESS_ADMINISTER		0x00000010

/* JOB status codes. */

#define JOB_STATUS_QUEUED               0x0000
#define JOB_STATUS_PAUSED		0x0001
#define JOB_STATUS_ERROR		0x0002
#define JOB_STATUS_DELETING		0x0004
#define JOB_STATUS_SPOOLING		0x0008
#define JOB_STATUS_PRINTING		0x0010
#define JOB_STATUS_OFFLINE		0x0020
#define JOB_STATUS_PAPEROUT		0x0040
#define JOB_STATUS_PRINTED		0x0080
#define JOB_STATUS_DELETED		0x0100
#define JOB_STATUS_BLOCKED		0x0200
#define JOB_STATUS_USER_INTERVENTION	0x0400

/* Access rights for print servers */
#define SERVER_ALL_ACCESS	STANDARD_RIGHTS_REQUIRED_ACCESS|SERVER_ACCESS_ADMINISTER|SERVER_ACCESS_ENUMERATE
#define SERVER_READ		STANDARD_RIGHTS_READ_ACCESS|SERVER_ACCESS_ENUMERATE
#define SERVER_WRITE		STANDARD_RIGHTS_WRITE_ACCESS|SERVER_ACCESS_ADMINISTER|SERVER_ACCESS_ENUMERATE
#define SERVER_EXECUTE		STANDARD_RIGHTS_EXECUTE_ACCESS|SERVER_ACCESS_ENUMERATE

/* Access rights for printers */
#define PRINTER_ALL_ACCESS	STANDARD_RIGHTS_REQUIRED_ACCESS|PRINTER_ACCESS_ADMINISTER|PRINTER_ACCESS_USE
#define PRINTER_READ          STANDARD_RIGHTS_READ_ACCESS|PRINTER_ACCESS_USE
#define PRINTER_WRITE         STANDARD_RIGHTS_WRITE_ACCESS|PRINTER_ACCESS_USE
#define PRINTER_EXECUTE       STANDARD_RIGHTS_EXECUTE_ACCESS|PRINTER_ACCESS_USE

/* Access rights for jobs */
#define JOB_ALL_ACCESS	STANDARD_RIGHTS_REQUIRED_ACCESS|JOB_ACCESS_ADMINISTER
#define JOB_READ	STANDARD_RIGHTS_READ_ACCESS|JOB_ACCESS_ADMINISTER
#define JOB_WRITE	STANDARD_RIGHTS_WRITE_ACCESS|JOB_ACCESS_ADMINISTER
#define JOB_EXECUTE	STANDARD_RIGHTS_EXECUTE_ACCESS|JOB_ACCESS_ADMINISTER

/* ACE masks for the various print permissions */

#define PRINTER_ACE_FULL_CONTROL      (GENERIC_ALL_ACCESS|PRINTER_ALL_ACCESS)
#define PRINTER_ACE_MANAGE_DOCUMENTS  (GENERIC_ALL_ACCESS|READ_CONTROL_ACCESS)
#define PRINTER_ACE_PRINT             (GENERIC_EXECUTE_ACCESS|READ_CONTROL_ACCESS|PRINTER_ACCESS_USE)


/* Notify field types */

#define NOTIFY_ONE_VALUE 1		/* Notify data is stored in value1 */
#define NOTIFY_TWO_VALUE 2		/* Notify data is stored in value2 */
#define NOTIFY_POINTER   3		/* Data is a pointer to a buffer */
#define NOTIFY_STRING    4		/* Data is a pointer to a buffer w/length */
#define NOTIFY_SECDESC   5		/* Data is a security descriptor */

#define PRINTER_NOTIFY_TYPE 0x00
#define JOB_NOTIFY_TYPE     0x01
#define PRINT_TABLE_END     0xFF

#define MAX_PRINTER_NOTIFY 26
#define MAX_JOB_NOTIFY 24

#define MAX_NOTIFY_TYPE_FOR_NOW 26

#define PRINTER_NOTIFY_SERVER_NAME		0x00
#define PRINTER_NOTIFY_PRINTER_NAME		0x01
#define PRINTER_NOTIFY_SHARE_NAME		0x02
#define PRINTER_NOTIFY_PORT_NAME		0x03
#define PRINTER_NOTIFY_DRIVER_NAME		0x04
#define PRINTER_NOTIFY_COMMENT			0x05
#define PRINTER_NOTIFY_LOCATION			0x06
#define PRINTER_NOTIFY_DEVMODE			0x07
#define PRINTER_NOTIFY_SEPFILE			0x08
#define PRINTER_NOTIFY_PRINT_PROCESSOR		0x09
#define PRINTER_NOTIFY_PARAMETERS		0x0A
#define PRINTER_NOTIFY_DATATYPE			0x0B
#define PRINTER_NOTIFY_SECURITY_DESCRIPTOR	0x0C
#define PRINTER_NOTIFY_ATTRIBUTES		0x0D
#define PRINTER_NOTIFY_PRIORITY			0x0E
#define PRINTER_NOTIFY_DEFAULT_PRIORITY		0x0F
#define PRINTER_NOTIFY_START_TIME		0x10
#define PRINTER_NOTIFY_UNTIL_TIME		0x11
#define PRINTER_NOTIFY_STATUS			0x12
#define PRINTER_NOTIFY_STATUS_STRING		0x13
#define PRINTER_NOTIFY_CJOBS			0x14
#define PRINTER_NOTIFY_AVERAGE_PPM		0x15
#define PRINTER_NOTIFY_TOTAL_PAGES		0x16
#define PRINTER_NOTIFY_PAGES_PRINTED		0x17
#define PRINTER_NOTIFY_TOTAL_BYTES		0x18
#define PRINTER_NOTIFY_BYTES_PRINTED		0x19

#define JOB_NOTIFY_PRINTER_NAME			0x00
#define JOB_NOTIFY_MACHINE_NAME			0x01
#define JOB_NOTIFY_PORT_NAME			0x02
#define JOB_NOTIFY_USER_NAME			0x03
#define JOB_NOTIFY_NOTIFY_NAME			0x04
#define JOB_NOTIFY_DATATYPE			0x05
#define JOB_NOTIFY_PRINT_PROCESSOR		0x06
#define JOB_NOTIFY_PARAMETERS			0x07
#define JOB_NOTIFY_DRIVER_NAME			0x08
#define JOB_NOTIFY_DEVMODE			0x09
#define JOB_NOTIFY_STATUS			0x0A
#define JOB_NOTIFY_STATUS_STRING		0x0B
#define JOB_NOTIFY_SECURITY_DESCRIPTOR		0x0C
#define JOB_NOTIFY_DOCUMENT			0x0D
#define JOB_NOTIFY_PRIORITY			0x0E
#define JOB_NOTIFY_POSITION			0x0F
#define JOB_NOTIFY_SUBMITTED			0x10
#define JOB_NOTIFY_START_TIME			0x11
#define JOB_NOTIFY_UNTIL_TIME			0x12
#define JOB_NOTIFY_TIME				0x13
#define JOB_NOTIFY_TOTAL_PAGES			0x14
#define JOB_NOTIFY_PAGES_PRINTED		0x15
#define JOB_NOTIFY_TOTAL_BYTES			0x16
#define JOB_NOTIFY_BYTES_PRINTED		0x17

#define PRINTER_NOTIFY_OPTIONS_REFRESH  	0x01

#define PRINTER_CHANGE_ADD_PRINTER			0x00000001
#define PRINTER_CHANGE_SET_PRINTER			0x00000002
#define PRINTER_CHANGE_DELETE_PRINTER			0x00000004
#define PRINTER_CHANGE_FAILED_CONNECTION_PRINTER	0x00000008
#define PRINTER_CHANGE_PRINTER	(PRINTER_CHANGE_ADD_PRINTER | \
				 PRINTER_CHANGE_SET_PRINTER | \
				 PRINTER_CHANGE_DELETE_PRINTER | \
				 PRINTER_CHANGE_FAILED_CONNECTION_PRINTER )

#define PRINTER_CHANGE_ADD_JOB				0x00000100
#define PRINTER_CHANGE_SET_JOB				0x00000200
#define PRINTER_CHANGE_DELETE_JOB			0x00000400
#define PRINTER_CHANGE_WRITE_JOB			0x00000800
#define PRINTER_CHANGE_JOB	(PRINTER_CHANGE_ADD_JOB | \
				 PRINTER_CHANGE_SET_JOB | \
				 PRINTER_CHANGE_DELETE_JOB | \
				 PRINTER_CHANGE_WRITE_JOB )

#define PRINTER_CHANGE_ADD_FORM				0x00010000
#define PRINTER_CHANGE_SET_FORM				0x00020000
#define PRINTER_CHANGE_DELETE_FORM			0x00040000
#define PRINTER_CHANGE_FORM	(PRINTER_CHANGE_ADD_FORM | \
				 PRINTER_CHANGE_SET_FORM | \
				 PRINTER_CHANGE_DELETE_FORM )

#define PRINTER_CHANGE_ADD_PORT				0x00100000
#define PRINTER_CHANGE_CONFIGURE_PORT			0x00200000
#define PRINTER_CHANGE_DELETE_PORT			0x00400000
#define PRINTER_CHANGE_PORT	(PRINTER_CHANGE_ADD_PORT | \
				 PRINTER_CHANGE_CONFIGURE_PORT | \
				 PRINTER_CHANGE_DELETE_PORT )

#define PRINTER_CHANGE_ADD_PRINT_PROCESSOR		0x01000000
#define PRINTER_CHANGE_DELETE_PRINT_PROCESSOR		0x04000000
#define PRINTER_CHANGE_PRINT_PROCESSOR	(PRINTER_CHANGE_ADD_PRINT_PROCESSOR | \
					 PRINTER_CHANGE_DELETE_PRINT_PROCESSOR )

#define PRINTER_CHANGE_ADD_PRINTER_DRIVER		0x10000000
#define PRINTER_CHANGE_SET_PRINTER_DRIVER		0x20000000
#define PRINTER_CHANGE_DELETE_PRINTER_DRIVER		0x40000000
#define PRINTER_CHANGE_PRINTER_DRIVER	(PRINTER_CHANGE_ADD_PRINTER_DRIVER | \
					 PRINTER_CHANGE_SET_PRINTER_DRIVER | \
					 PRINTER_CHANGE_DELETE_PRINTER_DRIVER )

#define PRINTER_CHANGE_TIMEOUT				0x80000000
#define PRINTER_CHANGE_ALL	(PRINTER_CHANGE_JOB | \
				 PRINTER_CHANGE_FORM | \
				 PRINTER_CHANGE_PORT | \
				 PRINTER_CHANGE_PRINT_PROCESSOR | \
				 PRINTER_CHANGE_PRINTER_DRIVER )

#define PRINTER_NOTIFY_INFO_DISCARDED	0x1

/*
 * Set of macros for flagging what changed in the PRINTER_INFO_2 struct
 * when sending messages to other smbd's
 */
#define PRINTER_MESSAGE_NULL            0x00000000
#define PRINTER_MESSAGE_DRIVER		0x00000001
#define PRINTER_MESSAGE_COMMENT		0x00000002
#define PRINTER_MESSAGE_PRINTERNAME	0x00000004
#define PRINTER_MESSAGE_LOCATION	0x00000008
#define PRINTER_MESSAGE_DEVMODE		0x00000010	/* not curently supported */
#define PRINTER_MESSAGE_SEPFILE		0x00000020
#define PRINTER_MESSAGE_PRINTPROC	0x00000040
#define PRINTER_MESSAGE_PARAMS		0x00000080
#define PRINTER_MESSAGE_DATATYPE	0x00000100
#define PRINTER_MESSAGE_SECDESC		0x00000200
#define PRINTER_MESSAGE_CJOBS		0x00000400
#define PRINTER_MESSAGE_PORT		0x00000800
#define PRINTER_MESSAGE_SHARENAME	0x00001000
#define PRINTER_MESSAGE_ATTRIBUTES	0x00002000

typedef struct printer_message_info {
	uint32 low;		/* PRINTER_CHANGE_XXX */
	uint32 high;		/* PRINTER_CHANGE_XXX */
	fstring printer_name;
	uint32 flags;		/* PRINTER_MESSAGE_XXX */
}
PRINTER_MESSAGE_INFO;

/*
 * The printer attributes.
 * I #defined all of them (grabbed form MSDN)
 * I'm only using:
 * ( SHARED | NETWORK | RAW_ONLY )
 * RAW_ONLY _MUST_ be present otherwise NT will send an EMF file
 */

#define PRINTER_ATTRIBUTE_QUEUED		0x00000001
#define PRINTER_ATTRIBUTE_DIRECT		0x00000002
#define PRINTER_ATTRIBUTE_DEFAULT		0x00000004
#define PRINTER_ATTRIBUTE_SHARED		0x00000008

#define PRINTER_ATTRIBUTE_NETWORK		0x00000010
#define PRINTER_ATTRIBUTE_HIDDEN		0x00000020
#define PRINTER_ATTRIBUTE_LOCAL			0x00000040
#define PRINTER_ATTRIBUTE_ENABLE_DEVQ		0x00000080

#define PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS	0x00000100
#define PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST	0x00000200
#define PRINTER_ATTRIBUTE_WORK_OFFLINE		0x00000400
#define PRINTER_ATTRIBUTE_ENABLE_BIDI		0x00000800

#define PRINTER_ATTRIBUTE_RAW_ONLY		0x00001000
#define PRINTER_ATTRIBUTE_PUBLISHED		0x00002000

#define PRINTER_ATTRIBUTE_SAMBA			(PRINTER_ATTRIBUTE_RAW_ONLY|\
						 PRINTER_ATTRIBUTE_SHARED|\
						 PRINTER_ATTRIBUTE_LOCAL)
#define PRINTER_ATTRIBUTE_NOT_SAMBA		(PRINTER_ATTRIBUTE_NETWORK)

#define NO_PRIORITY	 0
#define MAX_PRIORITY	99
#define MIN_PRIORITY	 1
#define DEF_PRIORITY	 1

/* the flags of the query */
#define PRINTER_ENUM_DEFAULT		0x00000001
#define PRINTER_ENUM_LOCAL		0x00000002
#define PRINTER_ENUM_CONNECTIONS	0x00000004
#define PRINTER_ENUM_FAVORITE		0x00000004
#define PRINTER_ENUM_NAME		0x00000008
#define PRINTER_ENUM_REMOTE		0x00000010
#define PRINTER_ENUM_SHARED		0x00000020
#define PRINTER_ENUM_NETWORK		0x00000040

/* the flags of each printers */
#define PRINTER_ENUM_UNKNOWN_8		0x00000008
#define PRINTER_ENUM_EXPAND		0x00004000
#define PRINTER_ENUM_CONTAINER		0x00008000
#define PRINTER_ENUM_ICONMASK		0x00ff0000
#define PRINTER_ENUM_ICON1		0x00010000
#define PRINTER_ENUM_ICON2		0x00020000
#define PRINTER_ENUM_ICON3		0x00040000
#define PRINTER_ENUM_ICON4		0x00080000
#define PRINTER_ENUM_ICON5		0x00100000
#define PRINTER_ENUM_ICON6		0x00200000
#define PRINTER_ENUM_ICON7		0x00400000
#define PRINTER_ENUM_ICON8		0x00800000

/* FLAGS for SPOOLSS_DELETEPRINTERDRIVEREX */

#define DPD_DELETE_UNUSED_FILES		0x00000001
#define DPD_DELETE_SPECIFIC_VERSION	0x00000002
#define DPD_DELETE_ALL_FILES		0x00000004

#define DRIVER_ANY_VERSION		0xffffffff
#define DRIVER_MAX_VERSION		4

/* FLAGS for SPOOLSS_ADDPRINTERDRIVEREX */

#define APD_STRICT_UPGRADE		0x00000001
#define APD_STRICT_DOWNGRADE		0x00000002
#define APD_COPY_ALL_FILES		0x00000004
#define APD_COPY_NEW_FILES		0x00000008


/* this struct is undocumented */
/* thanks to the ddk ... */
typedef struct {
	uint32 size;		/* length of user_name & client_name + 2? */
	UNISTR2 *client_name;
	UNISTR2 *user_name;
	uint32 build;
	uint32 major;
	uint32 minor;
	uint32 processor;
} SPOOL_USER_1;

typedef struct {
	uint32 level;
	union {
		SPOOL_USER_1 *user1;
	} user;
} SPOOL_USER_CTR;

/*
 * various bits in the DEVICEMODE.fields member
 */

#define DEVMODE_ORIENTATION		0x00000001
#define DEVMODE_PAPERSIZE		0x00000002
#define DEVMODE_PAPERLENGTH		0x00000004
#define DEVMODE_PAPERWIDTH		0x00000008
#define DEVMODE_SCALE			0x00000010
#define DEVMODE_POSITION		0x00000020
#define DEVMODE_NUP			0x00000040
#define DEVMODE_COPIES			0x00000100
#define DEVMODE_DEFAULTSOURCE		0x00000200
#define DEVMODE_PRINTQUALITY		0x00000400
#define DEVMODE_COLOR			0x00000800
#define DEVMODE_DUPLEX			0x00001000
#define DEVMODE_YRESOLUTION		0x00002000
#define DEVMODE_TTOPTION		0x00004000
#define DEVMODE_COLLATE			0x00008000
#define DEVMODE_FORMNAME		0x00010000
#define DEVMODE_LOGPIXELS		0x00020000
#define DEVMODE_BITSPERPEL		0x00040000
#define DEVMODE_PELSWIDTH		0x00080000
#define DEVMODE_PELSHEIGHT		0x00100000
#define DEVMODE_DISPLAYFLAGS		0x00200000
#define DEVMODE_DISPLAYFREQUENCY	0x00400000
#define DEVMODE_ICMMETHOD		0x00800000
#define DEVMODE_ICMINTENT		0x01000000
#define DEVMODE_MEDIATYPE		0x02000000
#define DEVMODE_DITHERTYPE		0x04000000
#define DEVMODE_PANNINGWIDTH		0x08000000
#define DEVMODE_PANNINGHEIGHT		0x10000000


/* 
 * Devicemode structure
 */

typedef struct devicemode
{
	UNISTR devicename;
	uint16 specversion;
	uint16 driverversion;
	uint16 size;
	uint16 driverextra;
	uint32 fields;
	uint16 orientation;
	uint16 papersize;
	uint16 paperlength;
	uint16 paperwidth;
	uint16 scale;
	uint16 copies;
	uint16 defaultsource;
	uint16 printquality;
	uint16 color;
	uint16 duplex;
	uint16 yresolution;
	uint16 ttoption;
	uint16 collate;
	UNISTR formname;
	uint16 logpixels;
	uint32 bitsperpel;
	uint32 pelswidth;
	uint32 pelsheight;
	uint32 displayflags;
	uint32 displayfrequency;
	uint32 icmmethod;
	uint32 icmintent;
	uint32 mediatype;
	uint32 dithertype;
	uint32 reserved1;
	uint32 reserved2;
	uint32 panningwidth;
	uint32 panningheight;
	uint8 *dev_private;
}
DEVICEMODE;

typedef struct _devmode_cont
{
	uint32 size;
	uint32 devmode_ptr;
	DEVICEMODE *devmode;
}
DEVMODE_CTR;

typedef struct _printer_default
{
	uint32 datatype_ptr;
	UNISTR2 datatype;
	DEVMODE_CTR devmode_cont;
	uint32 access_required;
}
PRINTER_DEFAULT;

/********************************************/

typedef struct {
	UNISTR2 *printername;
	PRINTER_DEFAULT printer_default;
} SPOOL_Q_OPEN_PRINTER;

typedef struct {
	POLICY_HND handle;	/* handle used along all transactions (20*uint8) */
	WERROR status;
} SPOOL_R_OPEN_PRINTER;

/********************************************/

typedef struct {
	UNISTR2 *printername;
	PRINTER_DEFAULT printer_default;
	uint32 user_switch;
	SPOOL_USER_CTR user_ctr;
} SPOOL_Q_OPEN_PRINTER_EX;

typedef struct {
	POLICY_HND handle;	/* handle used along all transactions (20*uint8) */
	WERROR status;
} SPOOL_R_OPEN_PRINTER_EX;

/********************************************/

typedef struct spool_notify_option_type
{
	uint16 type;
	uint16 reserved0;
	uint32 reserved1;
	uint32 reserved2;
	uint32 count;
	uint32 fields_ptr;
	uint32 count2;
	uint16 fields[MAX_NOTIFY_TYPE_FOR_NOW];
}
SPOOL_NOTIFY_OPTION_TYPE;

typedef struct spool_notify_option_type_ctr
{
	uint32 count;
	SPOOL_NOTIFY_OPTION_TYPE *type;
}
SPOOL_NOTIFY_OPTION_TYPE_CTR;



typedef struct s_header_type
{
	uint32 type;
	union
	{
		uint32 value;
		UNISTR string;
	}
	data;
}
HEADER_TYPE;


typedef struct spool_q_getprinterdata
{
	POLICY_HND handle;
	UNISTR2 valuename;
	uint32 size;
}
SPOOL_Q_GETPRINTERDATA;

typedef struct spool_r_getprinterdata
{
	uint32 type;
	uint32 size;
	uint8 *data;
	uint32 needed;
	WERROR status;
}
SPOOL_R_GETPRINTERDATA;

typedef struct spool_q_deleteprinterdata
{
	POLICY_HND handle;
	UNISTR2 valuename;
}
SPOOL_Q_DELETEPRINTERDATA;

typedef struct spool_r_deleteprinterdata
{
	WERROR status;
}
SPOOL_R_DELETEPRINTERDATA;

typedef struct spool_q_closeprinter
{
	POLICY_HND handle;
}
SPOOL_Q_CLOSEPRINTER;

typedef struct spool_r_closeprinter
{
	POLICY_HND handle;
	WERROR status;
}
SPOOL_R_CLOSEPRINTER;

typedef struct spool_q_startpageprinter
{
	POLICY_HND handle;
}
SPOOL_Q_STARTPAGEPRINTER;

typedef struct spool_r_startpageprinter
{
	WERROR status;
}
SPOOL_R_STARTPAGEPRINTER;

typedef struct spool_q_endpageprinter
{
	POLICY_HND handle;
}
SPOOL_Q_ENDPAGEPRINTER;

typedef struct spool_r_endpageprinter
{
	WERROR status;
}
SPOOL_R_ENDPAGEPRINTER;


typedef struct spool_q_deleteprinterdriver
{
	uint32 server_ptr;
	UNISTR2 server;
	UNISTR2 arch;
	UNISTR2 driver;
}
SPOOL_Q_DELETEPRINTERDRIVER;

typedef struct spool_r_deleteprinterdriver
{
	WERROR status;
}
SPOOL_R_DELETEPRINTERDRIVER;

typedef struct spool_q_deleteprinterdriverex
{
	uint32 server_ptr;
	UNISTR2 server;
	UNISTR2 arch;
	UNISTR2 driver;
	uint32 delete_flags;
	uint32 version;
}
SPOOL_Q_DELETEPRINTERDRIVEREX;

typedef struct spool_r_deleteprinterdriverex
{
	WERROR status;
}
SPOOL_R_DELETEPRINTERDRIVEREX;


typedef struct spool_doc_info_1
{
	uint32 p_docname;
	uint32 p_outputfile;
	uint32 p_datatype;
	UNISTR2 docname;
	UNISTR2 outputfile;
	UNISTR2 datatype;
}
DOC_INFO_1;

typedef struct spool_doc_info
{
	uint32 switch_value;
	DOC_INFO_1 doc_info_1;
}
DOC_INFO;

typedef struct spool_doc_info_container
{
	uint32 level;
	DOC_INFO docinfo;
}
DOC_INFO_CONTAINER;

typedef struct spool_q_startdocprinter
{
	POLICY_HND handle;
	DOC_INFO_CONTAINER doc_info_container;
}
SPOOL_Q_STARTDOCPRINTER;

typedef struct spool_r_startdocprinter
{
	uint32 jobid;
	WERROR status;
}
SPOOL_R_STARTDOCPRINTER;

typedef struct spool_q_enddocprinter
{
	POLICY_HND handle;
}
SPOOL_Q_ENDDOCPRINTER;

typedef struct spool_r_enddocprinter
{
	WERROR status;
}
SPOOL_R_ENDDOCPRINTER;

typedef struct spool_q_writeprinter
{
	POLICY_HND handle;
	uint32 buffer_size;
	uint8 *buffer;
	uint32 buffer_size2;
}
SPOOL_Q_WRITEPRINTER;

typedef struct spool_r_writeprinter
{
	uint32 buffer_written;
	WERROR status;
}
SPOOL_R_WRITEPRINTER;

typedef struct spool_notify_option
{
	uint32 version;
	uint32 flags;
	uint32 count;
	uint32 option_type_ptr;
	SPOOL_NOTIFY_OPTION_TYPE_CTR ctr;
}
SPOOL_NOTIFY_OPTION;

typedef struct spool_notify_info_data
{
	uint16 type;
	uint16 field;
	uint32 reserved;
	uint32 id;
	union {
		uint32 value[2];
		struct {
			uint32 length;
			uint16 *string;
		} data;
		struct {
			uint32	size;
			SEC_DESC *desc;
		} sd;
	}
	notify_data;
	uint32 size;
	uint32 enc_type;
} SPOOL_NOTIFY_INFO_DATA;

typedef struct spool_notify_info
{
	uint32 version;
	uint32 flags;
	uint32 count;
	SPOOL_NOTIFY_INFO_DATA *data;
}
SPOOL_NOTIFY_INFO;

/* If the struct name looks obscure, yes it is ! */
/* RemoteFindFirstPrinterChangeNotificationEx query struct */
typedef struct spoolss_q_rffpcnex
{
	POLICY_HND handle;
	uint32 flags;
	uint32 options;
	uint32 localmachine_ptr;
	UNISTR2 localmachine;
	uint32 printerlocal;
	uint32 option_ptr;
	SPOOL_NOTIFY_OPTION *option;
}
SPOOL_Q_RFFPCNEX;

typedef struct spool_r_rffpcnex
{
	WERROR status;
}
SPOOL_R_RFFPCNEX;

/* Remote Find Next Printer Change Notify Ex */
typedef struct spool_q_rfnpcnex
{
	POLICY_HND handle;
	uint32 change;
	uint32 option_ptr;
	SPOOL_NOTIFY_OPTION *option;
}
SPOOL_Q_RFNPCNEX;

typedef struct spool_r_rfnpcnex
{
	uint32 info_ptr;
	SPOOL_NOTIFY_INFO info;
	WERROR status;
}
SPOOL_R_RFNPCNEX;

/* Find Close Printer Notify */
typedef struct spool_q_fcpn
{
	POLICY_HND handle;
}
SPOOL_Q_FCPN;

typedef struct spool_r_fcpn
{
	WERROR status;
}
SPOOL_R_FCPN;


typedef struct printer_info_0
{
	UNISTR printername;
	UNISTR servername;
	uint32 cjobs;
	uint32 total_jobs;
	uint32 total_bytes;
	
	uint16 year;
	uint16 month;
	uint16 dayofweek;
	uint16 day;
	uint16 hour;
	uint16 minute;
	uint16 second;
	uint16 milliseconds;

	uint32 global_counter;
	uint32 total_pages;

	uint16 major_version;
	uint16 build_version;

	uint32 unknown7;
	uint32 unknown8;
	uint32 unknown9;
	uint32 session_counter;
	uint32 unknown11;
	uint32 printer_errors;
	uint32 unknown13;
	uint32 unknown14;
	uint32 unknown15;
	uint32 unknown16;
	uint32 change_id;
	uint32 unknown18;
	uint32 status;
	uint32 unknown20;
	uint32 c_setprinter;

	uint16 unknown22;
	uint16 unknown23;
	uint16 unknown24;
	uint16 unknown25;
	uint16 unknown26;
	uint16 unknown27;
	uint16 unknown28;
	uint16 unknown29;
} PRINTER_INFO_0;

typedef struct printer_info_1
{
	uint32 flags;
	UNISTR description;
	UNISTR name;
	UNISTR comment;
}
PRINTER_INFO_1;

typedef struct printer_info_2
{
	UNISTR servername;
	UNISTR printername;
	UNISTR sharename;
	UNISTR portname;
	UNISTR drivername;
	UNISTR comment;
	UNISTR location;
	DEVICEMODE *devmode;
	UNISTR sepfile;
	UNISTR printprocessor;
	UNISTR datatype;
	UNISTR parameters;
	SEC_DESC *secdesc;
	uint32 attributes;
	uint32 priority;
	uint32 defaultpriority;
	uint32 starttime;
	uint32 untiltime;
	uint32 status;
	uint32 cjobs;
	uint32 averageppm;
}
PRINTER_INFO_2;

typedef struct printer_info_3
{
	uint32 flags;
	SEC_DESC *secdesc;
}
PRINTER_INFO_3;

typedef struct printer_info_4
{
	UNISTR printername;
	UNISTR servername;
	uint32 attributes;
}
PRINTER_INFO_4;

typedef struct printer_info_5
{
	UNISTR printername;
	UNISTR portname;
	uint32 attributes;
	uint32 device_not_selected_timeout;
	uint32 transmission_retry_timeout;
}
PRINTER_INFO_5;

#define SPOOL_DS_PUBLISH	1
#define SPOOL_DS_UPDATE		2
#define SPOOL_DS_UNPUBLISH	4
#define SPOOL_DS_PENDING        0x80000000

typedef struct printer_info_7
{
	UNISTR guid; /* text form of printer guid */
	uint32 action;
}
PRINTER_INFO_7;

typedef struct spool_q_enumprinters
{
	uint32 flags;
	uint32 servername_ptr;
	UNISTR2 servername;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMPRINTERS;

typedef struct printer_info_ctr_info
{
	PRINTER_INFO_0 *printers_0;
	PRINTER_INFO_1 *printers_1;
	PRINTER_INFO_2 *printers_2;
	PRINTER_INFO_3 *printers_3;
	PRINTER_INFO_4 *printers_4;
	PRINTER_INFO_5 *printers_5;
	PRINTER_INFO_7 *printers_7;
}
PRINTER_INFO_CTR;

typedef struct spool_r_enumprinters
{
	RPC_BUFFER *buffer;
	uint32 needed;		/* bytes needed */
	uint32 returned;	/* number of printers */
	WERROR status;
}
SPOOL_R_ENUMPRINTERS;


typedef struct spool_q_getprinter
{
	POLICY_HND handle;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_GETPRINTER;

typedef struct printer_info_info
{
	union
	{
		PRINTER_INFO_0 *info0;
		PRINTER_INFO_1 *info1;
		PRINTER_INFO_2 *info2;
		void *info;
	} printer;
} PRINTER_INFO;

typedef struct spool_r_getprinter
{
	RPC_BUFFER *buffer;
	uint32 needed;
	WERROR status;
} SPOOL_R_GETPRINTER;

typedef struct driver_info_1
{
	UNISTR name;
} DRIVER_INFO_1;

typedef struct driver_info_2
{
	uint32 version;
	UNISTR name;
	UNISTR architecture;
	UNISTR driverpath;
	UNISTR datafile;
	UNISTR configfile;
} DRIVER_INFO_2;

typedef struct driver_info_3
{
	uint32 version;
	UNISTR name;
	UNISTR architecture;
	UNISTR driverpath;
	UNISTR datafile;
	UNISTR configfile;
	UNISTR helpfile;
	uint16 *dependentfiles;
	UNISTR monitorname;
	UNISTR defaultdatatype;
}
DRIVER_INFO_3;

typedef struct driver_info_6
{
	uint32 version;
	UNISTR name;
	UNISTR architecture;
	UNISTR driverpath;
	UNISTR datafile;
	UNISTR configfile;
	UNISTR helpfile;
	uint16 *dependentfiles;
	UNISTR monitorname;
	UNISTR defaultdatatype;
	uint16* previousdrivernames;
	NTTIME driver_date;
	uint32 padding;
	uint32 driver_version_low;
	uint32 driver_version_high;
	UNISTR mfgname;
	UNISTR oem_url;
	UNISTR hardware_id;
	UNISTR provider;
}
DRIVER_INFO_6;

typedef struct driver_info_info
{
	DRIVER_INFO_1 *info1;
	DRIVER_INFO_2 *info2;
	DRIVER_INFO_3 *info3;
	DRIVER_INFO_6 *info6;
}
PRINTER_DRIVER_CTR;

typedef struct spool_q_getprinterdriver2
{
	POLICY_HND handle;
	uint32 architecture_ptr;
	UNISTR2 architecture;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
	uint32 clientmajorversion;
	uint32 clientminorversion;
}
SPOOL_Q_GETPRINTERDRIVER2;

typedef struct spool_r_getprinterdriver2
{
	RPC_BUFFER *buffer;
	uint32 needed;
	uint32 servermajorversion;
	uint32 serverminorversion;
	WERROR status;
}
SPOOL_R_GETPRINTERDRIVER2;


typedef struct add_jobinfo_1
{
	UNISTR path;
	uint32 job_number;
}
ADD_JOBINFO_1;


typedef struct spool_q_addjob
{
	POLICY_HND handle;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ADDJOB;

typedef struct spool_r_addjob
{
	RPC_BUFFER *buffer;
	uint32 needed;
	WERROR status;
}
SPOOL_R_ADDJOB;

/*
 * I'm really wondering how many different time formats
 * I will have to cope with
 *
 * JFM, 09/13/98 In a mad mood ;-(
*/
typedef struct systemtime
{
	uint16 year;
	uint16 month;
	uint16 dayofweek;
	uint16 day;
	uint16 hour;
	uint16 minute;
	uint16 second;
	uint16 milliseconds;
}
SYSTEMTIME;

typedef struct s_job_info_1
{
	uint32 jobid;
	UNISTR printername;
	UNISTR machinename;
	UNISTR username;
	UNISTR document;
	UNISTR datatype;
	UNISTR text_status;
	uint32 status;
	uint32 priority;
	uint32 position;
	uint32 totalpages;
	uint32 pagesprinted;
	SYSTEMTIME submitted;
}
JOB_INFO_1;

typedef struct s_job_info_2
{
	uint32 jobid;
	UNISTR printername;
	UNISTR machinename;
	UNISTR username;
	UNISTR document;
	UNISTR notifyname;
	UNISTR datatype;
	UNISTR printprocessor;
	UNISTR parameters;
	UNISTR drivername;
	DEVICEMODE *devmode;
	UNISTR text_status;
/*	SEC_DESC sec_desc;*/
	uint32 status;
	uint32 priority;
	uint32 position;
	uint32 starttime;
	uint32 untiltime;
	uint32 totalpages;
	uint32 size;
	SYSTEMTIME submitted;
	uint32 timeelapsed;
	uint32 pagesprinted;
}
JOB_INFO_2;

typedef struct spool_q_enumjobs
{
	POLICY_HND handle;
	uint32 firstjob;
	uint32 numofjobs;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMJOBS;

typedef struct job_info_ctr_info
{
	union
	{
		JOB_INFO_1 *job_info_1;
		JOB_INFO_2 *job_info_2;
		void *info;
	} job;

} JOB_INFO_CTR;

typedef struct spool_r_enumjobs
{
	RPC_BUFFER *buffer;
	uint32 needed;
	uint32 returned;
	WERROR status;
}
SPOOL_R_ENUMJOBS;

typedef struct spool_q_schedulejob
{
	POLICY_HND handle;
	uint32 jobid;
}
SPOOL_Q_SCHEDULEJOB;

typedef struct spool_r_schedulejob
{
	WERROR status;
}
SPOOL_R_SCHEDULEJOB;

typedef struct s_port_info_1
{
	UNISTR port_name;
}
PORT_INFO_1;

typedef struct s_port_info_2
{
	UNISTR port_name;
	UNISTR monitor_name;
	UNISTR description;
	uint32 port_type;
	uint32 reserved;
}
PORT_INFO_2;

/* Port Type bits */
#define PORT_TYPE_WRITE         0x0001
#define PORT_TYPE_READ          0x0002
#define PORT_TYPE_REDIRECTED    0x0004
#define PORT_TYPE_NET_ATTACHED  0x0008

typedef struct spool_q_enumports
{
	uint32 name_ptr;
	UNISTR2 name;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMPORTS;

typedef struct port_info_ctr_info
{
	union
	{
		PORT_INFO_1 *info_1;
		PORT_INFO_2 *info_2;
	}
	port;

}
PORT_INFO_CTR;

typedef struct spool_r_enumports
{
	RPC_BUFFER *buffer;
	uint32 needed;		/* bytes needed */
	uint32 returned;	/* number of printers */
	WERROR status;
}
SPOOL_R_ENUMPORTS;

#define JOB_CONTROL_PAUSE              1
#define JOB_CONTROL_RESUME             2
#define JOB_CONTROL_CANCEL             3
#define JOB_CONTROL_RESTART            4
#define JOB_CONTROL_DELETE             5

typedef struct job_info_info
{
	union
	{
		JOB_INFO_1 job_info_1;
		JOB_INFO_2 job_info_2;
	}
	job;

}
JOB_INFO;

typedef struct spool_q_setjob
{
	POLICY_HND handle;
	uint32 jobid;
	uint32 level;
	JOB_INFO ctr;
	uint32 command;

}
SPOOL_Q_SETJOB;

typedef struct spool_r_setjob
{
	WERROR status;

}
SPOOL_R_SETJOB;

typedef struct spool_q_enumprinterdrivers
{
	uint32 name_ptr;
	UNISTR2 name;
	uint32 environment_ptr;
	UNISTR2 environment;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMPRINTERDRIVERS;

typedef struct spool_r_enumprinterdrivers
{
	RPC_BUFFER *buffer;
	uint32 needed;
	uint32 returned;
	WERROR status;
}
SPOOL_R_ENUMPRINTERDRIVERS;

#define FORM_USER    0
#define FORM_BUILTIN 1
#define FORM_PRINTER 2

typedef struct spool_form_1
{
	uint32 flag;
	UNISTR name;
	uint32 width;
	uint32 length;
	uint32 left;
	uint32 top;
	uint32 right;
	uint32 bottom;
}
FORM_1;

typedef struct spool_q_enumforms
{
	POLICY_HND handle;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMFORMS;

typedef struct spool_r_enumforms
{
	RPC_BUFFER *buffer;
	uint32 needed;
	uint32 numofforms;
	WERROR status;
}
SPOOL_R_ENUMFORMS;

typedef struct spool_q_getform
{
	POLICY_HND handle;
	UNISTR2 formname;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_GETFORM;

typedef struct spool_r_getform
{
	RPC_BUFFER *buffer;
	uint32 needed;
	WERROR status;
}
SPOOL_R_GETFORM;

typedef struct spool_printer_info_level_1
{
	uint32 flags;
	uint32 description_ptr;
	uint32 name_ptr;
	uint32 comment_ptr;
	UNISTR2 description;
	UNISTR2 name;
	UNISTR2 comment;	
} SPOOL_PRINTER_INFO_LEVEL_1;

typedef struct spool_printer_info_level_2
{
	uint32 servername_ptr;
	uint32 printername_ptr;
	uint32 sharename_ptr;
	uint32 portname_ptr;
	uint32 drivername_ptr;
	uint32 comment_ptr;
	uint32 location_ptr;
	uint32 devmode_ptr;
	uint32 sepfile_ptr;
	uint32 printprocessor_ptr;
	uint32 datatype_ptr;
	uint32 parameters_ptr;
	uint32 secdesc_ptr;
	uint32 attributes;
	uint32 priority;
	uint32 default_priority;
	uint32 starttime;
	uint32 untiltime;
	uint32 status;
	uint32 cjobs;
	uint32 averageppm;
	UNISTR2 servername;
	UNISTR2 printername;
	UNISTR2 sharename;
	UNISTR2 portname;
	UNISTR2 drivername;
	UNISTR2 comment;
	UNISTR2 location;
	UNISTR2 sepfile;
	UNISTR2 printprocessor;
	UNISTR2 datatype;
	UNISTR2 parameters;
}
SPOOL_PRINTER_INFO_LEVEL_2;

typedef struct spool_printer_info_level_3
{
	uint32 secdesc_ptr;
}
SPOOL_PRINTER_INFO_LEVEL_3;

typedef struct spool_printer_info_level_7
{
	uint32 guid_ptr;
	uint32 action;
	UNISTR2 guid;
}
SPOOL_PRINTER_INFO_LEVEL_7;

typedef struct spool_printer_info_level
{
	uint32 level;
	uint32 info_ptr;
	SPOOL_PRINTER_INFO_LEVEL_1 *info_1;
	SPOOL_PRINTER_INFO_LEVEL_2 *info_2;
	SPOOL_PRINTER_INFO_LEVEL_3 *info_3;
	SPOOL_PRINTER_INFO_LEVEL_7 *info_7;
}
SPOOL_PRINTER_INFO_LEVEL;

typedef struct spool_printer_driver_info_level_3
{
	uint32 cversion;
	uint32 name_ptr;
	uint32 environment_ptr;
	uint32 driverpath_ptr;
	uint32 datafile_ptr;
	uint32 configfile_ptr;
	uint32 helpfile_ptr;
	uint32 monitorname_ptr;
	uint32 defaultdatatype_ptr;
	uint32 dependentfilessize;
	uint32 dependentfiles_ptr;

	UNISTR2 name;
	UNISTR2 environment;
	UNISTR2 driverpath;
	UNISTR2 datafile;
	UNISTR2 configfile;
	UNISTR2 helpfile;
	UNISTR2 monitorname;
	UNISTR2 defaultdatatype;
	BUFFER5 dependentfiles;

}
SPOOL_PRINTER_DRIVER_INFO_LEVEL_3;

/* SPOOL_PRINTER_DRIVER_INFO_LEVEL_6 structure */
typedef struct {
	uint32 version;
	uint32 name_ptr;
	uint32 environment_ptr;
	uint32 driverpath_ptr;
	uint32 datafile_ptr;
	uint32 configfile_ptr;
	uint32 helpfile_ptr;
	uint32 monitorname_ptr;
	uint32 defaultdatatype_ptr;
	uint32 dependentfiles_len;
	uint32 dependentfiles_ptr;
	uint32 previousnames_len;
	uint32 previousnames_ptr;
	NTTIME	driverdate;
	UINT64_S	driverversion;
	uint32	dummy4;
	uint32 mfgname_ptr;
	uint32 oemurl_ptr;
	uint32 hardwareid_ptr;
	uint32 provider_ptr;
	UNISTR2	name;
	UNISTR2	environment;
	UNISTR2	driverpath;
	UNISTR2	datafile;
	UNISTR2	configfile;
	UNISTR2	helpfile;
	UNISTR2	monitorname;
	UNISTR2	defaultdatatype;
	BUFFER5	dependentfiles;
	BUFFER5	previousnames;
	UNISTR2	mfgname;
	UNISTR2	oemurl;
	UNISTR2	hardwareid;
	UNISTR2	provider;
} SPOOL_PRINTER_DRIVER_INFO_LEVEL_6;


typedef struct spool_printer_driver_info_level
{
	uint32 level;
	uint32 ptr;
	SPOOL_PRINTER_DRIVER_INFO_LEVEL_3 *info_3;
	SPOOL_PRINTER_DRIVER_INFO_LEVEL_6 *info_6;
}
SPOOL_PRINTER_DRIVER_INFO_LEVEL;


typedef struct spool_q_setprinter
{
	POLICY_HND handle;
	uint32 level;
	SPOOL_PRINTER_INFO_LEVEL info;
	SEC_DESC_BUF *secdesc_ctr;
	DEVMODE_CTR devmode_ctr;

	uint32 command;

}
SPOOL_Q_SETPRINTER;

typedef struct spool_r_setprinter
{
	WERROR status;
}
SPOOL_R_SETPRINTER;

/********************************************/

typedef struct {
	POLICY_HND handle;
} SPOOL_Q_DELETEPRINTER;

typedef struct {
	POLICY_HND handle;
	WERROR status;
} SPOOL_R_DELETEPRINTER;

/********************************************/

typedef struct {
	POLICY_HND handle;
} SPOOL_Q_ABORTPRINTER;

typedef struct {
	WERROR status;
} SPOOL_R_ABORTPRINTER;


/********************************************/

typedef struct {
	UNISTR2 *server_name;
	uint32 level;
	SPOOL_PRINTER_INFO_LEVEL info;
	DEVMODE_CTR devmode_ctr;
	SEC_DESC_BUF *secdesc_ctr;
	uint32 user_switch;
	SPOOL_USER_CTR user_ctr;
} SPOOL_Q_ADDPRINTEREX;

typedef struct {
	POLICY_HND handle;
	WERROR status;
} SPOOL_R_ADDPRINTEREX;

/********************************************/

typedef struct spool_q_addprinterdriver
{
	uint32 server_name_ptr;
	UNISTR2 server_name;
	uint32 level;
	SPOOL_PRINTER_DRIVER_INFO_LEVEL info;
}
SPOOL_Q_ADDPRINTERDRIVER;

typedef struct spool_r_addprinterdriver
{
	WERROR status;
}
SPOOL_R_ADDPRINTERDRIVER;

typedef struct spool_q_addprinterdriverex
{
	uint32 server_name_ptr;
	UNISTR2 server_name;
	uint32 level;
	SPOOL_PRINTER_DRIVER_INFO_LEVEL info;
	uint32 copy_flags;
}
SPOOL_Q_ADDPRINTERDRIVEREX;

typedef struct spool_r_addprinterdriverex
{
	WERROR status;
}
SPOOL_R_ADDPRINTERDRIVEREX;


typedef struct driver_directory_1
{
	UNISTR name;
}
DRIVER_DIRECTORY_1;

typedef struct driver_info_ctr_info
{
	DRIVER_DIRECTORY_1 *info1;
}
DRIVER_DIRECTORY_CTR;

typedef struct spool_q_getprinterdriverdirectory
{
	uint32 name_ptr;
	UNISTR2 name;
	uint32 environment_ptr;
	UNISTR2 environment;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_GETPRINTERDRIVERDIR;

typedef struct spool_r_getprinterdriverdirectory
{
	RPC_BUFFER *buffer;
	uint32 needed;
	WERROR status;
}
SPOOL_R_GETPRINTERDRIVERDIR;

typedef struct spool_q_addprintprocessor
{
	uint32 server_ptr;
	UNISTR2 server;
	UNISTR2 environment;
	UNISTR2 path;
	UNISTR2 name;
}
SPOOL_Q_ADDPRINTPROCESSOR;

typedef struct spool_r_addprintprocessor
{
	WERROR status;
}
SPOOL_R_ADDPRINTPROCESSOR;


typedef struct spool_q_enumprintprocessors
{
	uint32 name_ptr;
	UNISTR2 name;
	uint32 environment_ptr;
	UNISTR2 environment;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMPRINTPROCESSORS;

typedef struct printprocessor_1
{
	UNISTR name;
}
PRINTPROCESSOR_1;

typedef struct spool_r_enumprintprocessors
{
	RPC_BUFFER *buffer;
	uint32 needed;
	uint32 returned;
	WERROR status;
}
SPOOL_R_ENUMPRINTPROCESSORS;

typedef struct spool_q_enumprintprocdatatypes
{
	uint32 name_ptr;
	UNISTR2 name;
	uint32 processor_ptr;
	UNISTR2 processor;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMPRINTPROCDATATYPES;

typedef struct ppdatatype_1
{
	UNISTR name;
}
PRINTPROCDATATYPE_1;

typedef struct spool_r_enumprintprocdatatypes
{
	RPC_BUFFER *buffer;
	uint32 needed;
	uint32 returned;
	WERROR status;
}
SPOOL_R_ENUMPRINTPROCDATATYPES;

typedef struct printmonitor_1
{
	UNISTR name;
}
PRINTMONITOR_1;

typedef struct printmonitor_2
{
	UNISTR name;
	UNISTR environment;
	UNISTR dll_name;
}
PRINTMONITOR_2;

typedef struct spool_q_enumprintmonitors
{
	uint32 name_ptr;
	UNISTR2 name;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_ENUMPRINTMONITORS;

typedef struct spool_r_enumprintmonitors
{
	RPC_BUFFER *buffer;
	uint32 needed;
	uint32 returned;
	WERROR status;
}
SPOOL_R_ENUMPRINTMONITORS;


typedef struct spool_q_enumprinterdata
{
	POLICY_HND handle;
	uint32 index;
	uint32 valuesize;
	uint32 datasize;
}
SPOOL_Q_ENUMPRINTERDATA;

typedef struct spool_r_enumprinterdata
{
	uint32 valuesize;
	uint16 *value;
	uint32 realvaluesize;
	uint32 type;
	uint32 datasize;
	uint8 *data;
	uint32 realdatasize;
	WERROR status;
}
SPOOL_R_ENUMPRINTERDATA;

typedef struct spool_q_setprinterdata
{
	POLICY_HND handle;
	UNISTR2 value;
	uint32 type;
	uint32 max_len;
	uint8 *data;
	uint32 real_len;
	uint32 numeric_data;
}
SPOOL_Q_SETPRINTERDATA;

typedef struct spool_r_setprinterdata
{
	WERROR status;
}
SPOOL_R_SETPRINTERDATA;

typedef struct spool_q_resetprinter
{
	POLICY_HND handle;
	uint32 datatype_ptr;
	UNISTR2 datatype;
	DEVMODE_CTR devmode_ctr;

} SPOOL_Q_RESETPRINTER;

typedef struct spool_r_resetprinter
{
	WERROR status;
} 
SPOOL_R_RESETPRINTER;



typedef struct _form
{
	uint32 flags;
	uint32 name_ptr;
	uint32 size_x;
	uint32 size_y;
	uint32 left;
	uint32 top;
	uint32 right;
	uint32 bottom;
	UNISTR2 name;
}
FORM;

typedef struct spool_q_addform
{
	POLICY_HND handle;
	uint32 level;
	uint32 level2;		/* This should really be part of the FORM structure */
	FORM form;
}
SPOOL_Q_ADDFORM;

typedef struct spool_r_addform
{
	WERROR status;
}
SPOOL_R_ADDFORM;

typedef struct spool_q_setform
{
	POLICY_HND handle;
	UNISTR2 name;
	uint32 level;
	uint32 level2;
	FORM form;
}
SPOOL_Q_SETFORM;

typedef struct spool_r_setform
{
	WERROR status;
}
SPOOL_R_SETFORM;

typedef struct spool_q_deleteform
{
	POLICY_HND handle;
	UNISTR2 name;
}
SPOOL_Q_DELETEFORM;

typedef struct spool_r_deleteform
{
	WERROR status;
}
SPOOL_R_DELETEFORM;

typedef struct spool_q_getjob
{
	POLICY_HND handle;
	uint32 jobid;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_GETJOB;

typedef struct pjob_info_info
{
	union
	{
		JOB_INFO_1 *job_info_1;
		JOB_INFO_2 *job_info_2;
		void *info;
	}
	job;

}
PJOB_INFO;

typedef struct spool_r_getjob
{
	RPC_BUFFER *buffer;
	uint32 needed;
	WERROR status;
}
SPOOL_R_GETJOB;

typedef struct spool_q_replyopenprinter
{
	UNISTR2 string;
	uint32 printer;
	uint32 type;
	uint32 unknown0;
	uint32 unknown1;
}
SPOOL_Q_REPLYOPENPRINTER;

typedef struct spool_r_replyopenprinter
{
	POLICY_HND handle;
	WERROR status;
}
SPOOL_R_REPLYOPENPRINTER;

typedef struct spool_q_routerreplyprinter
{
	POLICY_HND handle;
	uint32 condition;
	uint32 unknown1;	/* 0x00000001 */
	uint32 change_id;
	uint8  unknown2[5];	/* 0x0000000001 */
}
SPOOL_Q_ROUTERREPLYPRINTER;

typedef struct spool_r_routerreplyprinter
{
	WERROR status;
}
SPOOL_R_ROUTERREPLYPRINTER;

typedef struct spool_q_replycloseprinter
{
	POLICY_HND handle;
}
SPOOL_Q_REPLYCLOSEPRINTER;

typedef struct spool_r_replycloseprinter
{
	POLICY_HND handle;
	WERROR status;
}
SPOOL_R_REPLYCLOSEPRINTER;

typedef struct spool_q_rrpcn
{
	POLICY_HND handle;
	uint32 change_low;
	uint32 change_high;
	uint32 unknown0;
	uint32 unknown1;
	uint32 info_ptr;
	SPOOL_NOTIFY_INFO info;	
}
SPOOL_Q_REPLY_RRPCN;

typedef struct spool_r_rrpcn
{
	uint32 unknown0;
	WERROR status;
}
SPOOL_R_REPLY_RRPCN;

typedef struct spool_q_getprinterdataex
{
	POLICY_HND handle;
	UNISTR2 keyname;
        UNISTR2 valuename;
	uint32 size;
}
SPOOL_Q_GETPRINTERDATAEX;

typedef struct spool_r_getprinterdataex
{
	uint32 type;
	uint32 size;
	uint8 *data;
	uint32 needed;
	WERROR status;
}
SPOOL_R_GETPRINTERDATAEX;

typedef struct spool_q_setprinterdataex
{
	POLICY_HND handle;
	UNISTR2 key;
	UNISTR2 value;
	uint32 type;
	uint32 max_len;
	uint8 *data;
	uint32 real_len;
	uint32 numeric_data;
}
SPOOL_Q_SETPRINTERDATAEX;

typedef struct spool_r_setprinterdataex
{
	WERROR status;
}
SPOOL_R_SETPRINTERDATAEX;


typedef struct spool_q_deleteprinterdataex
{
	POLICY_HND handle;
	UNISTR2 keyname;
	UNISTR2 valuename;
}
SPOOL_Q_DELETEPRINTERDATAEX;

typedef struct spool_r_deleteprinterdataex
{
	WERROR status;
}
SPOOL_R_DELETEPRINTERDATAEX;


typedef struct spool_q_enumprinterkey
{
	POLICY_HND handle;
	UNISTR2 key;
	uint32 size;
}
SPOOL_Q_ENUMPRINTERKEY;

typedef struct spool_r_enumprinterkey
{
	BUFFER5 keys;
	uint32 needed;	/* in bytes */
	WERROR status;
}
SPOOL_R_ENUMPRINTERKEY;

typedef struct spool_q_deleteprinterkey
{
	POLICY_HND handle;
	UNISTR2 keyname;
}
SPOOL_Q_DELETEPRINTERKEY;

typedef struct spool_r_deleteprinterkey
{
	WERROR status;
}
SPOOL_R_DELETEPRINTERKEY;

typedef struct printer_enum_values
{
	UNISTR valuename;
	uint32 value_len;
	uint32 type;
	uint8  *data;
	uint32 data_len; 
	
}
PRINTER_ENUM_VALUES;

typedef struct printer_enum_values_ctr
{
	uint32 size;
	uint32 size_of_array;
	PRINTER_ENUM_VALUES *values;
}
PRINTER_ENUM_VALUES_CTR;

typedef struct spool_q_enumprinterdataex
{
	POLICY_HND handle;
	UNISTR2 key;
	uint32 size;
}
SPOOL_Q_ENUMPRINTERDATAEX;

typedef struct spool_r_enumprinterdataex
{
	PRINTER_ENUM_VALUES_CTR ctr;
	uint32 needed;
	uint32 returned;
	WERROR status;
}
SPOOL_R_ENUMPRINTERDATAEX;

typedef struct printprocessor_directory_1
{
	UNISTR name;
}
PRINTPROCESSOR_DIRECTORY_1;

typedef struct spool_q_getprintprocessordirectory
{
	UNISTR2 name;
	UNISTR2 environment;
	uint32 level;
	RPC_BUFFER *buffer;
	uint32 offered;
}
SPOOL_Q_GETPRINTPROCESSORDIRECTORY;

typedef struct spool_r_getprintprocessordirectory
{
	RPC_BUFFER *buffer;
	uint32 needed;
	WERROR status;
}
SPOOL_R_GETPRINTPROCESSORDIRECTORY;

/**************************************/

#define MAX_PORTNAME		64
#define MAX_NETWORK_NAME	49
#define MAX_SNMP_COMM_NAME	33
#define	MAX_QUEUE_NAME		33
#define MAX_IPADDR_STRING	17
		
typedef struct {
	uint16 portname[MAX_PORTNAME];
	uint32 version;
	uint32 protocol;
	uint32 size;
	uint32 reserved;
	uint16 hostaddress[MAX_NETWORK_NAME];
	uint16 snmpcommunity[MAX_SNMP_COMM_NAME];
	uint32 dblspool;
	uint16 queue[MAX_QUEUE_NAME];
	uint16 ipaddress[MAX_IPADDR_STRING];
	uint32 port;
	uint32 snmpenabled;
	uint32 snmpdevindex;
} SPOOL_PORT_DATA_1;

typedef struct {
	POLICY_HND handle;
	UNISTR2 dataname;
	RPC_BUFFER indata;
	uint32 indata_len;
	uint32 offered;
	uint32 unknown;
} SPOOL_Q_XCVDATAPORT;

typedef struct {
	RPC_BUFFER outdata;
	uint32 needed;
	uint32 unknown;
	WERROR status;
} SPOOL_R_XCVDATAPORT;

#define PRINTER_DRIVER_VERSION 2
#define PRINTER_DRIVER_ARCHITECTURE "Windows NT x86"

#endif /* _RPC_SPOOLSS_H */


/*
   Unix SMB/Netbios implementation.
   Version 1.9.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell              1992-2000,
   Copyright (C) Jean Francois Micouleau      1998-2000.

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

#ifndef NT_PRINTING_H_
#define NT_PRINTING_H_

#define ORIENTATION      0x00000001L
#define PAPERSIZE        0x00000002L
#define PAPERLENGTH      0x00000004L
#define PAPERWIDTH       0x00000008L
#define SCALE            0x00000010L
#define COPIES           0x00000100L
#define DEFAULTSOURCE    0x00000200L
#define PRINTQUALITY     0x00000400L
#define COLOR            0x00000800L
#define DUPLEX           0x00001000L
#define YRESOLUTION      0x00002000L
#define TTOPTION         0x00004000L
#define COLLATE          0x00008000L
#define FORMNAME         0x00010000L
#define LOGPIXELS        0x00020000L
#define BITSPERPEL       0x00040000L
#define PELSWIDTH        0x00080000L
#define PELSHEIGHT       0x00100000L
#define DISPLAYFLAGS     0x00200000L
#define DISPLAYFREQUENCY 0x00400000L
#define PANNINGWIDTH     0x00800000L
#define PANNINGHEIGHT    0x01000000L

#define ORIENT_PORTRAIT   1
#define ORIENT_LANDSCAPE  2

#define PAPER_FIRST                PAPER_LETTER
#define PAPER_LETTER               1  /* Letter 8 1/2 x 11 in               */
#define PAPER_LETTERSMALL          2  /* Letter Small 8 1/2 x 11 in         */
#define PAPER_TABLOID              3  /* Tabloid 11 x 17 in                 */
#define PAPER_LEDGER               4  /* Ledger 17 x 11 in                  */
#define PAPER_LEGAL                5  /* Legal 8 1/2 x 14 in                */
#define PAPER_STATEMENT            6  /* Statement 5 1/2 x 8 1/2 in         */
#define PAPER_EXECUTIVE            7  /* Executive 7 1/4 x 10 1/2 in        */
#define PAPER_A3                   8  /* A3 297 x 420 mm                    */
#define PAPER_A4                   9  /* A4 210 x 297 mm                    */
#define PAPER_A4SMALL             10  /* A4 Small 210 x 297 mm              */
#define PAPER_A5                  11  /* A5 148 x 210 mm                    */
#define PAPER_B4                  12  /* B4 (JIS) 250 x 354                 */
#define PAPER_B5                  13  /* B5 (JIS) 182 x 257 mm              */
#define PAPER_FOLIO               14  /* Folio 8 1/2 x 13 in                */
#define PAPER_QUARTO              15  /* Quarto 215 x 275 mm                */
#define PAPER_10X14               16  /* 10x14 in                           */
#define PAPER_11X17               17  /* 11x17 in                           */
#define PAPER_NOTE                18  /* Note 8 1/2 x 11 in                 */
#define PAPER_ENV_9               19  /* Envelope #9 3 7/8 x 8 7/8          */
#define PAPER_ENV_10              20  /* Envelope #10 4 1/8 x 9 1/2         */
#define PAPER_ENV_11              21  /* Envelope #11 4 1/2 x 10 3/8        */
#define PAPER_ENV_12              22  /* Envelope #12 4 \276 x 11           */
#define PAPER_ENV_14              23  /* Envelope #14 5 x 11 1/2            */
#define PAPER_CSHEET              24  /* C size sheet                       */
#define PAPER_DSHEET              25  /* D size sheet                       */
#define PAPER_ESHEET              26  /* E size sheet                       */
#define PAPER_ENV_DL              27  /* Envelope DL 110 x 220mm            */
#define PAPER_ENV_C5              28  /* Envelope C5 162 x 229 mm           */
#define PAPER_ENV_C3              29  /* Envelope C3  324 x 458 mm          */
#define PAPER_ENV_C4              30  /* Envelope C4  229 x 324 mm          */
#define PAPER_ENV_C6              31  /* Envelope C6  114 x 162 mm          */
#define PAPER_ENV_C65             32  /* Envelope C65 114 x 229 mm          */
#define PAPER_ENV_B4              33  /* Envelope B4  250 x 353 mm          */
#define PAPER_ENV_B5              34  /* Envelope B5  176 x 250 mm          */
#define PAPER_ENV_B6              35  /* Envelope B6  176 x 125 mm          */
#define PAPER_ENV_ITALY           36  /* Envelope 110 x 230 mm              */
#define PAPER_ENV_MONARCH         37  /* Envelope Monarch 3.875 x 7.5 in    */
#define PAPER_ENV_PERSONAL        38  /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
#define PAPER_FANFOLD_US          39  /* US Std Fanfold 14 7/8 x 11 in      */
#define PAPER_FANFOLD_STD_GERMAN  40  /* German Std Fanfold 8 1/2 x 12 in   */
#define PAPER_FANFOLD_LGL_GERMAN  41  /* German Legal Fanfold 8 1/2 x 13 in */

#define PAPER_LAST                PAPER_FANFOLD_LGL_GERMAN
#define PAPER_USER                256

#define BIN_FIRST         BIN_UPPER
#define BIN_UPPER         1
#define BIN_ONLYONE       1
#define BIN_LOWER         2
#define BIN_MIDDLE        3
#define BIN_MANUAL        4
#define BIN_ENVELOPE      5
#define BIN_ENVMANUAL     6
#define BIN_AUTO          7
#define BIN_TRACTOR       8
#define BIN_SMALLFMT      9
#define BIN_LARGEFMT      10
#define BIN_LARGECAPACITY 11
#define BIN_CASSETTE      14
#define BIN_FORMSOURCE    15
#define BIN_LAST          BIN_FORMSOURCE

#define BIN_USER          256     /* device specific bins start here */

#define RES_DRAFT         (-1)
#define RES_LOW           (-2)
#define RES_MEDIUM        (-3)
#define RES_HIGH          (-4)

#define COLOR_MONOCHROME  1
#define COLOR_COLOR       2

#define DUP_SIMPLEX    1
#define DUP_VERTICAL   2
#define DUP_HORIZONTAL 3

#define TT_BITMAP     1       /* print TT fonts as graphics */
#define TT_DOWNLOAD   2       /* download TT fonts as soft fonts */
#define TT_SUBDEV     3       /* substitute device fonts for TT fonts */

#define COLLATE_FALSE  0
#define COLLATE_TRUE   1

typedef struct nt_printer_driver_info_level_3
{
	uint32 cversion;

	fstring name;
	fstring environment;
	fstring driverpath;
	fstring datafile;
	fstring configfile;
	fstring helpfile;
	fstring monitorname;
	fstring defaultdatatype;
	fstring *dependentfiles;
} NT_PRINTER_DRIVER_INFO_LEVEL_3;

/* SPOOL_PRINTER_DRIVER_INFO_LEVEL_6 structure */
typedef struct {
	uint32	version;
	fstring	name;
	fstring	environment;
	fstring	driverpath;
	fstring	datafile;
	fstring	configfile;
	fstring	helpfile;
	fstring	monitorname;
	fstring	defaultdatatype;
	fstring	mfgname;
	fstring	oemurl;
	fstring	hardwareid;
	fstring	provider;
	fstring *dependentfiles;
	fstring *previousnames;
} NT_PRINTER_DRIVER_INFO_LEVEL_6;


typedef struct nt_printer_driver_info_level
{
	NT_PRINTER_DRIVER_INFO_LEVEL_3 *info_3;
	NT_PRINTER_DRIVER_INFO_LEVEL_6 *info_6;
} NT_PRINTER_DRIVER_INFO_LEVEL;

/* predefined registry key names for printer data */

#define SPOOL_PRINTERDATA_KEY		"PrinterDriverData"
#define SPOOL_DSSPOOLER_KEY		"DsSpooler"
#define SPOOL_DSDRIVER_KEY		"DsDriver"
#define SPOOL_DSUSER_KEY		"DsUser"
#define SPOOL_PNPDATA_KEY		"PnPData"
#define SPOOL_OID_KEY			"OID"

/* predefined value names for printer data */
#define SPOOL_REG_ASSETNUMBER		"assetNumber"
#define SPOOL_REG_BYTESPERMINUTE	"bytesPerMinute"
#define SPOOL_REG_DEFAULTPRIORITY	"defaultPriority"
#define SPOOL_REG_DESCRIPTION		"description"
#define SPOOL_REG_DRIVERNAME		"driverName"
#define SPOOL_REG_DRIVERVERSION		"driverVersion"
#define SPOOL_REG_FLAGS			"flags"
#define SPOOL_REG_LOCATION		"location"
#define SPOOL_REG_OPERATINGSYSTEM	"operatingSystem"
#define SPOOL_REG_OPERATINGSYSTEMHOTFIX	"operatingSystemHotfix"
#define SPOOL_REG_OPERATINGSYSTEMSERVICEPACK "operatingSystemServicePack"
#define SPOOL_REG_OPERATINGSYSTEMVERSION "operatingSystemVersion"
#define SPOOL_REG_PORTNAME		"portName"
#define SPOOL_REG_PRINTATTRIBUTES	"printAttributes"
#define SPOOL_REG_PRINTBINNAMES		"printBinNames"
#define SPOOL_REG_PRINTCOLLATE		"printCollate"
#define SPOOL_REG_PRINTCOLOR		"printColor"
#define SPOOL_REG_PRINTDUPLEXSUPPORTED	"printDuplexSupported"
#define SPOOL_REG_PRINTENDTIME		"printEndTime"
#define SPOOL_REG_PRINTERNAME		"printerName"
#define SPOOL_REG_PRINTFORMNAME		"printFormName"
#define SPOOL_REG_PRINTKEEPPRINTEDJOBS	"printKeepPrintedJobs"
#define SPOOL_REG_PRINTLANGUAGE		"printLanguage"
#define SPOOL_REG_PRINTMACADDRESS	"printMACAddress"
#define SPOOL_REG_PRINTMAXCOPIES	"printMaxCopies"
#define SPOOL_REG_PRINTMAXRESOLUTIONSUPPORTED "printMaxResolutionSupported"
#define SPOOL_REG_PRINTMAXXEXTENT	"printMaxXExtent"
#define SPOOL_REG_PRINTMAXYEXTENT	"printMaxYExtent"
#define SPOOL_REG_PRINTMEDIAREADY	"printMediaReady"
#define SPOOL_REG_PRINTMEDIASUPPORTED	"printMediaSupported"
#define SPOOL_REG_PRINTMEMORY		"printMemory"
#define SPOOL_REG_PRINTMINXEXTENT	"printMinXExtent"
#define SPOOL_REG_PRINTMINYEXTENT	"printMinYExtent"
#define SPOOL_REG_PRINTNETWORKADDRESS	"printNetworkAddress"
#define SPOOL_REG_PRINTNOTIFY		"printNotify"
#define SPOOL_REG_PRINTNUMBERUP		"printNumberUp"
#define SPOOL_REG_PRINTORIENTATIONSSUPPORTED "printOrientationsSupported"
#define SPOOL_REG_PRINTOWNER		"printOwner"
#define SPOOL_REG_PRINTPAGESPERMINUTE	"printPagesPerMinute"
#define SPOOL_REG_PRINTRATE		"printRate"
#define SPOOL_REG_PRINTRATEUNIT		"printRateUnit"
#define SPOOL_REG_PRINTSEPARATORFILE	"printSeparatorFile"
#define SPOOL_REG_PRINTSHARENAME	"printShareName"
#define SPOOL_REG_PRINTSPOOLING		"printSpooling"
#define SPOOL_REGVAL_PRINTWHILESPOOLING	"PrintWhileSpooling"
#define SPOOL_REGVAL_PRINTAFTERSPOOLED	"PrintAfterSpooled"
#define SPOOL_REGVAL_PRINTDIRECT	"PrintDirect"
#define SPOOL_REG_PRINTSTAPLINGSUPPORTED "printStaplingSupported"
#define SPOOL_REG_PRINTSTARTTIME	"printStartTime"
#define SPOOL_REG_PRINTSTATUS		"printStatus"
#define SPOOL_REG_PRIORITY		"priority"
#define SPOOL_REG_SERVERNAME		"serverName"
#define SPOOL_REG_SHORTSERVERNAME	"shortServerName"
#define SPOOL_REG_UNCNAME		"uNCName"
#define SPOOL_REG_URL			"url"
#define SPOOL_REG_VERSIONNUMBER		"versionNumber"

/* container for a single registry key */

typedef struct {
	char		*name;
	REGVAL_CTR 	*values;
} NT_PRINTER_KEY;

/* container for all printer data */

typedef struct {
	int		num_keys;
	NT_PRINTER_KEY	*keys;
} NT_PRINTER_DATA;

#define MAXDEVICENAME	32

typedef struct ntdevicemode
{
	fstring	devicename;
	fstring	formname;

	uint16	specversion;
	uint16	driverversion;
	uint16	size;
	uint16	driverextra;
	uint16	orientation;
	uint16	papersize;
	uint16	paperlength;
	uint16	paperwidth;
	uint16	scale;
	uint16	copies;
	uint16	defaultsource;
	uint16	printquality;
	uint16	color;
	uint16	duplex;
	uint16	yresolution;
	uint16	ttoption;
	uint16	collate;
	uint16	logpixels;

	uint32	fields;
	uint32	bitsperpel;
	uint32	pelswidth;
	uint32	pelsheight;
	uint32	displayflags;
	uint32	displayfrequency;
	uint32	icmmethod;
	uint32	icmintent;
	uint32	mediatype;
	uint32	dithertype;
	uint32	reserved1;
	uint32	reserved2;
	uint32	panningwidth;
	uint32	panningheight;
	uint8 	*nt_dev_private;
} NT_DEVICEMODE;

typedef struct nt_printer_info_level_2
{
	uint32 attributes;
	uint32 priority;
	uint32 default_priority;
	uint32 starttime;
	uint32 untiltime;
	uint32 status;
	uint32 cjobs;
	uint32 averageppm;
	fstring servername;
	fstring printername;
	fstring sharename;
	fstring portname;
	fstring drivername;
	pstring comment;
	fstring location;
	NT_DEVICEMODE *devmode;
	fstring sepfile;
	fstring printprocessor;
	fstring datatype;
	fstring parameters;
	NT_PRINTER_DATA *data;
	SEC_DESC_BUF *secdesc_buf;
	uint32 changeid;
	uint32 c_setprinter;
	uint32 setuptime;	
} NT_PRINTER_INFO_LEVEL_2;

typedef struct nt_printer_info_level
{
	NT_PRINTER_INFO_LEVEL_2 *info_2;
} NT_PRINTER_INFO_LEVEL;

typedef struct
{
	fstring name;
	uint32 flag;
	uint32 width;
	uint32 length;
	uint32 left;
	uint32 top;
	uint32 right;
	uint32 bottom;
} nt_forms_struct;

#ifndef SAMBA_PRINTER_PORT_NAME
#define SAMBA_PRINTER_PORT_NAME "Samba Printer Port"
#endif


/*
 * Structures for the XcvDataPort() calls
 */

#define PORT_PROTOCOL_DIRECT	1
#define PORT_PROTOCOL_LPR	2

typedef struct {
	fstring name;
	uint32 version;
	uint32 protocol;
	fstring hostaddr;
	fstring snmpcommunity;
	fstring queue;
	uint32 dblspool;
	fstring ipaddr;
	uint32 port;
	BOOL enable_snmp;
	uint32 snmp_index;
} NT_PORT_DATA_1;

/* DOS header format */
#define DOS_HEADER_SIZE                 64
#define DOS_HEADER_MAGIC_OFFSET         0
#define DOS_HEADER_MAGIC                0x5A4D
#define DOS_HEADER_LFANEW_OFFSET        60

/* New Executable format (Win or OS/2 1.x segmented) */
#define NE_HEADER_SIZE                  64
#define NE_HEADER_SIGNATURE_OFFSET      0
#define NE_HEADER_SIGNATURE             0x454E
#define NE_HEADER_TARGET_OS_OFFSET      54
#define NE_HEADER_TARGOS_WIN            0x02
#define NE_HEADER_MINOR_VER_OFFSET      62
#define NE_HEADER_MAJOR_VER_OFFSET      63

/* Portable Executable format */
#define PE_HEADER_SIZE                  248
#define PE_HEADER_SIGNATURE_OFFSET      0
#define PE_HEADER_SIGNATURE             0x00004550
#define PE_HEADER_MACHINE_OFFSET        4
#define PE_HEADER_MACHINE_I386          0x14c
#define PE_HEADER_NUMBER_OF_SECTIONS    6
#define PE_HEADER_MAJOR_OS_VER_OFFSET   64
#define PE_HEADER_MINOR_OS_VER_OFFSET   66
#define PE_HEADER_MAJOR_IMG_VER_OFFSET  68
#define PE_HEADER_MINOR_IMG_VER_OFFSET  70
#define PE_HEADER_MAJOR_SS_VER_OFFSET   72
#define PE_HEADER_MINOR_SS_VER_OFFSET   74
#define PE_HEADER_SECT_HEADER_SIZE      40
#define PE_HEADER_SECT_NAME_OFFSET      0
#define PE_HEADER_SECT_SIZE_DATA_OFFSET 16
#define PE_HEADER_SECT_PTR_DATA_OFFSET  20

/* Microsoft file version format */
#define VS_SIGNATURE                    "VS_VERSION_INFO"
#define VS_MAGIC_VALUE                  0xfeef04bd
#define VS_MAJOR_OFFSET					8
#define VS_MINOR_OFFSET					12
#define VS_VERSION_INFO_UNICODE_SIZE    (sizeof(VS_SIGNATURE)*2+4+VS_MINOR_OFFSET+4) /* not true size! */
#define VS_VERSION_INFO_SIZE            (sizeof(VS_SIGNATURE)+4+VS_MINOR_OFFSET+4)   /* not true size! */
#define VS_NE_BUF_SIZE                  4096  /* Must be > 2*VS_VERSION_INFO_SIZE */

/* Notify spoolss clients that something has changed.  The
   notification data is either stored in two uint32 values or a
   variable length array. */

#define SPOOLSS_NOTIFY_MSG_UNIX_JOBID 0x0001    /* Job id is unix  */

typedef struct spoolss_notify_msg {
	fstring printer;	/* Name of printer notified */
	uint32 type;		/* Printer or job notify */
	uint32 field;		/* Notify field changed */
	uint32 id;		/* Job id */
	uint32 len;		/* Length of data, 0 for two uint32 value */
	uint32 flags;
	union {
		uint32 value[2];
		char *data;
	} notify;
} SPOOLSS_NOTIFY_MSG;

typedef struct {
	fstring 		printername;
	uint32			num_msgs;
	SPOOLSS_NOTIFY_MSG	*msgs;
} SPOOLSS_NOTIFY_MSG_GROUP;

typedef struct {
	TALLOC_CTX 			*ctx;
	uint32				num_groups;
	SPOOLSS_NOTIFY_MSG_GROUP	*msg_groups;
} SPOOLSS_NOTIFY_MSG_CTR;

#define SPLHND_PRINTER		1
#define SPLHND_SERVER	 	2
#define SPLHND_PORTMON_TCP	3
#define SPLHND_PORTMON_LOCAL	4

/* structure to store the printer handles */
/* and a reference to what it's pointing to */
/* and the notify info asked about */
/* that's the central struct */
typedef struct _Printer{
	struct _Printer *prev, *next;
	BOOL document_started;
	BOOL page_started;
	uint32 jobid; /* jobid in printing backend */
	BOOL printer_type;
	TALLOC_CTX *ctx;
	fstring servername;
	fstring sharename;
	uint32 type;
	uint32 access_granted;
	struct {
		uint32 flags;
		uint32 options;
		fstring localmachine;
		uint32 printerlocal;
		SPOOL_NOTIFY_OPTION *option;
		POLICY_HND client_hnd;
		BOOL client_connected;
		uint32 change;
		/* are we in a FindNextPrinterChangeNotify() call? */
		BOOL fnpcn;
	} notify;
	struct {
		fstring machine;
		fstring user;
	} client;
	
	/* devmode sent in the OpenPrinter() call */
	NT_DEVICEMODE	*nt_devmode;
	
	/* cache the printer info */
	NT_PRINTER_INFO_LEVEL *printer_info;
	
} Printer_entry;

#endif /* NT_PRINTING_H_ */

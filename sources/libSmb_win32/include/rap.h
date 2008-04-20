/* 
   Samba Unix/Linux SMB client library 
   RAP (SMB Remote Procedure Calls) defines and structures
   Copyright (C) Steve French 2001  (sfrench@us.ibm.com)
   Copyright (C) Jim McDonough 2001 (jmcd@us.ibm.com)
   
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

#ifndef _RAP_H_
#define _RAP_H_

/*****************************************************/
/*                                                   */
/*   Additional RAP functionality                    */
/*                                                   */
/*   RAP is the original SMB RPC, documented         */
/*   by Microsoft and X/Open in the 1990s and        */
/*   supported by most SMB/CIFS servers although     */
/*   it is unlikely that any one implementation      */
/*   supports all RAP command codes since some       */
/*   are quite obsolete and a few are specific       */
/*   to a particular network operating system        */
/*                                                   */ 
/*   Although it has largely been replaced           */ 
/*   for complex remote admistration and management  */
/*   (of servers) by the relatively newer            */
/*   DCE/RPC based remote API (which better handles  */
/*   large >64K data structures), there are many     */
/*   important administrative and resource location  */
/*   tasks and user tasks (e.g. password change)     */
/*   that are performed via RAP.                     */
/*                                                   */
/*   Although a few of the RAP calls are implemented */
/*   in the Samba client library already (clirap.c)  */
/*   the new ones are in clirap2.c for easy patching */
/*   and integration and a corresponding header      */
/*   file, rap.h, has been created.                  */
/*                                                   */
/*   This is based on data from the CIFS spec        */
/*   and the LAN Server and LAN Manager              */
/*   Programming Reference books and published       */
/*   RAP document and CIFS forum postings and        */
/*   lots of trial and error.  Additional            */
/*   background information is available from the    */
/*   X/Open reference book in their PC Interworking  */
/*   series "IPC for SMB" and also from the          */
/*   interoperability documentation in               */
/*   ftp://ftp.microsoft.com/developr/drg/cifs       */
/*                                                   */
/*   Function names changed from API_ (as they are   */
/*   in the CIFS specification to RAP_ in order      */
/*   to avoid confusion with other API calls         */
/*   sent via DCE RPC                                */
/*                                                   */
/*****************************************************/

/*****************************************************/
/*                                                   */
/* Although without pound defines (of this header)   */
/* cifsrap.c already includes support for:           */
/*                                                   */
/* WshareEnum (API number 0, level 1)                */
/* NetServerEnum2 (API num 104, level 1)             */
/* WWkstaUserLogon (132)                             */
/* SamOEMchgPasswordUser2_P (214)                    */
/*                                                   */
/* and cifsprint.c already includes support for:     */
/*                                                   */
/* WPrintJobEnum (API num 76, level 2)               */
/* WPrintJobDel  (API num 81)                        */
/*                                                   */
/*****************************************************/ 

#define RAP_WshareEnum			        0
#define RAP_WshareGetInfo			1
#define RAP_WshareSetInfo			2
#define RAP_WshareAdd				3
#define RAP_WshareDel				4
#define RAP_NetShareCheck			5
#define RAP_WsessionEnum			6
#define RAP_WsessionGetInfo			7
#define RAP_WsessionDel		        	8
#define RAP_WconnectionEnum			9
#define RAP_WfileEnum				10
#define RAP_WfileGetInfo			11
#define RAP_WfileClose				12
#define RAP_WserverGetInfo			13
#define RAP_WserverSetInfo			14
#define RAP_WserverDiskEnum			15
#define RAP_WserverAdminCommand	        	16
#define RAP_NetAuditOpen			17
#define RAP_WauditClear			        18
#define RAP_NetErrorLogOpen			19
#define RAP_WerrorLogClear			20
#define RAP_NetCharDevEnum			21
#define RAP_NetCharDevGetInfo			22
#define RAP_WCharDevControl			23
#define RAP_NetCharDevQEnum			24
#define RAP_NetCharDevQGetInfo	        	25
#define RAP_WCharDevQSetInfo			26
#define RAP_WCharDevQPurge			27
#define RAP_WCharDevQPurgeSelf		        28
#define RAP_WMessageNameEnum		        29
#define RAP_WMessageNameGetInfo  		30
#define RAP_WMessageNameAdd			31
#define RAP_WMessageNameDel			32
#define RAP_WMessageNameFwd			33
#define RAP_WMessageNameUnFwd	        	34
#define RAP_WMessageBufferSend	        	35
#define RAP_WMessageFileSend			36
#define RAP_WMessageLogFileSet	         	37
#define RAP_WMessageLogFileGet		        38
#define RAP_WServiceEnum			39
#define RAP_WServiceInstall			40
#define RAP_WServiceControl			41
#define RAP_WAccessEnum	         		42
#define RAP_WAccessGetInfo			43
#define RAP_WAccessSetInfo			44
#define RAP_WAccessAdd		        	45
#define RAP_WAccessDel		        	46
#define RAP_WGroupEnum			        47
#define RAP_WGroupAdd		        	48
#define RAP_WGroupDel				49
#define RAP_WGroupAddUser			50
#define RAP_WGroupDelUser			51
#define RAP_WGroupGetUsers			52
#define RAP_WUserEnum		         	53
#define RAP_WUserAdd				54
#define RAP_WUserDel				55
#define RAP_WUserGetInfo			56
#define RAP_WUserSetInfo			57
#define RAP_WUserPasswordSet			58
#define RAP_WUserGetGroups			59
#define RAP_WWkstaSetUID			62
#define RAP_WWkstaGetInfo			63
#define RAP_WWkstaSetInfo			64
#define RAP_WUseEnum				65
#define RAP_WUseAdd				66
#define RAP_WUseDel				67
#define RAP_WUseGetInfo		        	68
#define RAP_WPrintQEnum		        	69
#define RAP_WPrintQGetInfo			70
#define RAP_WPrintQSetInfo			71
#define RAP_WPrintQAdd		        	72
#define RAP_WPrintQDel				73
#define RAP_WPrintQPause			74
#define RAP_WPrintQContinue			75
#define RAP_WPrintJobEnum			76
#define RAP_WPrintJobGetInfo			77
#define RAP_WPrintJobSetInfo_OLD		78
#define RAP_WPrintJobDel			81
#define RAP_WPrintJobPause			82
#define RAP_WPrintJobContinue			83
#define RAP_WPrintDestEnum			84
#define RAP_WPrintDestGetInfo			85
#define RAP_WPrintDestControl			86
#define RAP_WProfileSave			87
#define RAP_WProfileLoad			88
#define RAP_WStatisticsGet			89
#define RAP_WStatisticsClear			90
#define RAP_NetRemoteTOD			91
#define RAP_WNetBiosEnum			92
#define RAP_WNetBiosGetInfo			93
#define RAP_NetServerEnum			94
#define RAP_I_NetServerEnum			95
#define RAP_WServiceGetInfo			96
#define RAP_WPrintQPurge			103
#define RAP_NetServerEnum2			104
#define RAP_WAccessGetUserPerms		        105
#define RAP_WGroupGetInfo			106
#define RAP_WGroupSetInfo			107
#define RAP_WGroupSetUsers			108
#define RAP_WUserSetGroups			109
#define RAP_WUserModalsGet			110
#define RAP_WUserModalsSet			111
#define RAP_WFileEnum2		        	112
#define RAP_WUserAdd2				113
#define RAP_WUserSetInfo2			114
#define RAP_WUserPasswordSet2			115
#define RAP_I_NetServerEnum2			116
#define RAP_WConfigGet2			        117
#define RAP_WConfigGetAll2			118
#define RAP_WGetDCName		        	119
#define RAP_NetHandleGetInfo			120
#define RAP_NetHandleSetInfo			121
#define RAP_WStatisticsGet2			122
#define RAP_WBuildGetInfo			123
#define RAP_WFileGetInfo2			124
#define RAP_WFileClose2			        125
#define RAP_WNetServerReqChallenge		126
#define RAP_WNetServerAuthenticate		127
#define RAP_WNetServerPasswordSet		128
#define RAP_WNetAccountDeltas			129
#define RAP_WNetAccountSync			130
#define RAP_WUserEnum2	        		131
#define RAP_WWkstaUserLogon			132
#define RAP_WWkstaUserLogoff			133
#define RAP_WLogonEnum	         		134
#define RAP_WErrorLogRead			135
#define RAP_NetPathType		        	136
#define RAP_NetPathCanonicalize		        137
#define RAP_NetPathCompare			138
#define RAP_NetNameValidate		        139
#define RAP_NetNameCanonicalize		        140
#define RAP_NetNameCompare		        141
#define RAP_WAuditRead		        	142
#define RAP_WPrintDestAdd			143
#define RAP_WPrintDestSetInfo			144
#define RAP_WPrintDestDel			145
#define RAP_WUserValidate2			146
#define RAP_WPrintJobSetInfo			147
#define RAP_TI_NetServerDiskEnum		148
#define RAP_TI_NetServerDiskGetInfo		149
#define RAP_TI_FTVerifyMirror			150
#define RAP_TI_FTAbortVerify			151
#define RAP_TI_FTGetInfo			152
#define RAP_TI_FTSetInfo			153
#define RAP_TI_FTLockDisk			154
#define RAP_TI_FTFixError			155
#define RAP_TI_FTAbortFix			156
#define RAP_TI_FTDiagnoseError			157
#define RAP_TI_FTGetDriveStats			158
#define RAP_TI_FTErrorGetInfo			160
#define RAP_NetAccessCheck			163
#define RAP_NetAlertRaise			164
#define RAP_NetAlertStart			165
#define RAP_NetAlertStop			166
#define RAP_NetAuditWrite			167
#define RAP_NetIRemoteAPI			168
#define RAP_NetServiceStatus			169
#define RAP_NetServerRegister			170
#define RAP_NetServerDeregister		        171
#define RAP_NetSessionEntryMake	        	172
#define RAP_NetSessionEntryClear		173
#define RAP_NetSessionEntryGetInfo		174
#define RAP_NetSessionEntrySetInfo		175
#define RAP_NetConnectionEntryMake		176
#define RAP_NetConnectionEntryClear		177
#define RAP_NetConnectionEntrySetInfo		178
#define RAP_NetConnectionEntryGetInfo		179
#define RAP_NetFileEntryMake			180
#define RAP_NetFileEntryClear			181
#define RAP_NetFileEntrySetInfo	        	182
#define RAP_NetFileEntryGetInfo		        183
#define RAP_AltSrvMessageBufferSend		184
#define RAP_AltSrvMessageFileSend		185
#define RAP_wI_NetRplWkstaEnum		        186
#define RAP_wI_NetRplWkstaGetInfo		187
#define RAP_wI_NetRplWkstaSetInfo		188
#define RAP_wI_NetRplWkstaAdd		        189
#define RAP_wI_NetRplWkstaDel			190
#define RAP_wI_NetRplProfileEnum		191
#define RAP_wI_NetRplProfileGetInfo		192
#define RAP_wI_NetRplProfileSetInfo		193
#define RAP_wI_NetRplProfileAdd	        	194
#define RAP_wI_NetRplProfileDel			195
#define RAP_wI_NetRplProfileClone		196
#define RAP_wI_NetRplBaseProfileEnum		197
#define RAP_WIServerSetInfo			201
#define RAP_WPrintDriverEnum			205
#define RAP_WPrintQProcessorEnum		206
#define RAP_WPrintPortEnum			207
#define RAP_WNetWriteUpdateLog	        	208
#define RAP_WNetAccountUpdate			209
#define RAP_WNetAccountConfirmUpdate		210
#define RAP_WConfigSet				211
#define RAP_WAccountsReplicate			212                      
#define RAP_SamOEMChgPasswordUser2_P	        214
#define RAP_NetServerEnum3			215
#define RAP_WprintDriverGetInfo			250
#define RAP_WprintDriverSetInfo			251
#define RAP_WaliasAdd				252
#define RAP_WaliasDel				253
#define RAP_WaliasGetInfo			254
#define RAP_WaliasSetInfo			255
#define RAP_WaliasEnum	        		256
#define RAP_WuserGetLogonAsn			257
#define RAP_WuserSetLogonAsn			258
#define RAP_WuserGetAppSel			259
#define RAP_WuserSetAppSel			260
#define RAP_WappAdd				261
#define RAP_WappDel				262
#define RAP_WappGetInfo		        	263
#define RAP_WappSetInfo			        264
#define RAP_WappEnum				265
#define RAP_WUserDCDBInit			266
#define RAP_WDASDAdd		        	267
#define RAP_WDASDDel		        	268
#define RAP_WDASDGetInfo			269
#define RAP_WDASDSetInfo			270
#define RAP_WDASDEnum			        271
#define RAP_WDASDCheck	         		272
#define RAP_WDASDCtl				273
#define RAP_WuserRemoteLogonCheck		274
#define RAP_WUserPasswordSet3			275
#define RAP_WCreateRIPLMachine   		276
#define RAP_WDeleteRIPLMachine		        277
#define RAP_WGetRIPLMachineInfo	         	278
#define RAP_WSetRIPLMachineInfo	         	279
#define RAP_WEnumRIPLMachine	        	280
#define RAP_I_ShareAdd		        	281
#define RAP_AliasEnum		         	282
#define RAP_WaccessApply			283
#define RAP_WPrt16Query			        284
#define RAP_WPrt16Set				285
#define RAP_WUserDel100		        	286
#define RAP_WUserRemoteLogonCheck2		287
#define RAP_WRemoteTODSet			294
#define RAP_WprintJobMoveAll			295
#define RAP_W16AppParmAdd			296
#define RAP_W16AppParmDel			297
#define RAP_W16AppParmGet			298
#define RAP_W16AppParmSet			299
#define RAP_W16RIPLMachineCreate		300
#define RAP_W16RIPLMachineGetInfo		301
#define RAP_W16RIPLMachineSetInfo		302
#define RAP_W16RIPLMachineEnum		        303
#define RAP_W16RIPLMachineListParmEnum	        304
#define RAP_W16RIPLMachClassGetInfo		305
#define RAP_W16RIPLMachClassEnum		306
#define RAP_W16RIPLMachClassCreate		307
#define RAP_W16RIPLMachClassSetInfo		308
#define RAP_W16RIPLMachClassDelete		309
#define RAP_W16RIPLMachClassLPEnum		310
#define RAP_W16RIPLMachineDelete		311
#define RAP_W16WSLevelGetInfo	         	312
#define RAP_WserverNameAdd			313
#define RAP_WserverNameDel			314
#define RAP_WserverNameEnum			315
#define RAP_I_WDASDEnum	         		316
#define RAP_WDASDEnumTerminate	         	317
#define RAP_WDASDSetInfo2			318
#define MAX_API					318


/* Parameter description strings for RAP calls   */
/* Names are defined name for RAP call with _REQ */
/* appended to end.                              */

#define RAP_WFileEnum2_REQ	"zzWrLehb8g8"
#define RAP_WFileGetInfo2_REQ	"DWrLh"
#define RAP_WFileClose2_REQ     "D"  	

#define RAP_NetGroupEnum_REQ    "WrLeh"
#define RAP_NetGroupAdd_REQ     "WsT"
#define RAP_NetGroupDel_REQ     "z"
#define RAP_NetGroupAddUser_REQ "zz"
#define RAP_NetGroupDelUser_REQ "zz"
#define RAP_NetGroupGetUsers_REQ "zWrLeh"
#define RAP_NetGroupSetUsers_REQ "zWsTW"

#define RAP_NetUserAdd2_REQ       "WsTWW"
#define RAP_NetUserEnum_REQ       "WrLeh"
#define RAP_NetUserEnum2_REQ      "WrLDieh"
#define RAP_NetUserGetGroups_REQ  "zWrLeh"
#define RAP_NetUserSetGroups_REQ  "zWsTW"
#define RAP_NetUserPasswordSet_REQ "zb16b16w"
#define RAP_NetUserPasswordSet2_REQ "zb16b16WW"
#define RAP_SAMOEMChgPasswordUser2_REQ "B516B16"
#define RAP_NetUserValidate2_REQ    "Wb62WWrLhWW"

#define RAP_NetServerEnum2_REQ  "WrLehDz"
#define RAP_WserverGetInfo_REQ  "WrLh"
#define RAP_NetWkstatGetInfo    "WrLh"

#define RAP_WShareAdd_REQ       "WsT"
#define RAP_WShareEnum_REQ      "WrLeh"
#define RAP_WShareDel_REQ       "zW"
#define RAP_WWkstaGetInfo_REQ   "WrLh"

#define RAP_NetPrintQEnum_REQ   "WrLeh"
#define RAP_NetPrintQGetInfo_REQ "zWrLh"

#define RAP_NetServerAdminCommand_REQ "zhrLeh"
#define RAP_NetServiceEnum_REQ  "WrLeh"
#define RAP_NetServiceControl_REQ "zWWrL"
#define RAP_NetServiceInstall_REQ "zF88sg88T"
#define RAP_NetServiceGetInfo_REQ "zWrLh"
#define RAP_NetSessionEnum_REQ  "WrLeh"
#define RAP_NetSessionGetInfo_REQ "zWrLh"
#define RAP_NetSessionDel_REQ   "zW"

#define RAP_NetConnectionEnum_REQ "zWrLeh"

#define RAP_NetWkstaUserLogoff_REQ "zzWb38WrLh"

/* Description strings for returned data in RAP calls */
/* I use all caps here in part to avoid accidental    */
/* name collisions */

#define RAP_FILE_INFO_L2        "D"
#define RAP_FILE_INFO_L3        "DWWzz"

#define RAP_GROUP_INFO_L0       "B21"
#define RAP_GROUP_INFO_L1       "B21Bz"
#define RAP_GROUP_USERS_INFO_0  "B21"
#define RAP_GROUP_USERS_INFO_1  "B21BN"

#define RAP_USER_INFO_L0        "B21"
#define RAP_USER_INFO_L1        "B21BB16DWzzWz"

#define RAP_SERVER_INFO_L0      "B16"
#define RAP_SERVER_INFO_L1      "B16BBDz"
#define RAP_SERVER_INFO_L2 "B16BBDzDDDWWzWWWWWWWB21BzWWWWWWWWWWWWWWWWWWWWWWz"
#define RAP_SERVER_INFO_L3 "B16BBDzDDDWWzWWWWWWWB21BzWWWWWWWWWWWWWWWWWWWWWWzDWz"
#define RAP_SERVICE_INFO_L0     "B16"
#define RAP_SERVICE_INFO_L2     "B16WDWB64"
#define RAP_SHARE_INFO_L0       "B13"
#define RAP_SHARE_INFO_L1	"B13BWz"
#define RAP_SHARE_INFO_L2	"B13BWzWWWzB9B"

#define RAP_PRINTQ_INFO_L2      "B13BWWWzzzzzWN"
#define RAP_SMB_PRINT_JOB_L1    "WB21BB16B10zWWzDDz"

#define RAP_SESSION_INFO_L2      "zzWWWDDDz"
#define RAP_CONNECTION_INFO_L1   "WWWWDzz"

#define RAP_USER_LOGOFF_INFO_L1     "WDW"

#define RAP_WKSTA_INFO_L1       "WDzzzzBBDWDWWWWWWWWWWWWWWWWWWWzzWzzW"
#define RAP_WKSTA_INFO_L10      "zzzBBzz"

/* BB explicit packing would help in structs below */

/* sizes of fixed-length fields, including null terminator */
#define RAP_GROUPNAME_LEN 21
#define RAP_USERNAME_LEN 21
#define RAP_SHARENAME_LEN 13
#define RAP_UPASSWD_LEN 16 /* user password */
#define RAP_SPASSWD_LEN 9 /* share password */
#define RAP_MACHNAME_LEN 16
#define RAP_SRVCNAME_LEN 16
#define RAP_SRVCCMNT_LEN 64
#define RAP_DATATYPE_LEN 10


typedef struct rap_group_info_1
{
    char   group_name[RAP_GROUPNAME_LEN];
    char   reserved1;
    char * comment;
} RAP_GROUP_INFO_1;

typedef struct rap_user_info_1
{
    char   user_name[RAP_USERNAME_LEN];
    char   reserved1;
    char   passwrd[RAP_UPASSWD_LEN];
    uint32 pwage;
    uint16 priv;
    char * home_dir;
    char * comment;
    uint16 userflags;
    char * logon_script;
} RAP_USER_INFO_1;

typedef struct rap_service_info_2
{
    char   service_name[RAP_SRVCNAME_LEN];
    uint16 status;
    uint32 installcode;
    uint16 process_num;
    char * comment;
} RAP_SERVICE_INFO_2;


typedef struct rap_share_info_0
{
    char   share_name[RAP_SHARENAME_LEN];
} RAP_SHARE_INFO_0;

typedef struct rap_share_info_1
{
    char   share_name[RAP_SHARENAME_LEN];
    char   reserved1;
    uint16 share_type;
    char * comment;
} RAP_SHARE_INFO_1;

typedef struct rap_share_info_2
{
    char   share_name[RAP_SHARENAME_LEN];
    char   reserved1;
    uint16 share_type;
    char * comment;
    uint16 perms;
    uint16 maximum_users;
    uint16 active_users;
    char * path;
    char   password[RAP_SPASSWD_LEN];
    char   reserved2;
} RAP_SHARE_INFO_2;

#endif /* _RAP_H_ */

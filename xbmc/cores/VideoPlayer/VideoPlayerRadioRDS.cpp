/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

//#define RDS_IMPROVE_CHECK         1

/*
 * The RDS decoder bases partly on the source of the VDR radio plugin.
 * http://www.egal-vdr.de/plugins/
 * and reworked a bit with references from SPB 490, IEC62106
 * and several other documents.
 *
 * A lot more information is sendet which is currently unused and partly
 * not required.
 */

#include "VideoPlayerRadioRDS.h"

#include "DVDStreamInfo.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "Interface/DemuxPacket.h"
#include "ServiceBroker.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "music/tags/MusicInfoTag.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <mutex>

using namespace XFILE;
using namespace PVR;
using namespace KODI::MESSAGING;

using namespace std::chrono_literals;

/**
 * Universal Encoder Communication Protocol (UECP)
 * List of defined commands
 * iaw.: SPB 490
 */

/// UECP Message element pointers (different on several commands)
#define UECP_ME_MEC                     0       // Message Element Code
#define UECP_ME_DSN                     1       // Data Set Number
#define UECP_ME_PSN                     2       // Program Service Number
#define UECP_ME_MEL                     3       // Message Element data Length
#define UECP_ME_DATA                    4       //

/// RDS message commands
#define UECP_RDS_PI                     0x01    // Program Identification
#define UECP_RDS_PS                     0x02    // Program Service name
#define UECP_RDS_PIN                    0x06    // Program Item Number
#define UECP_RDS_DI                     0x04    // Decoder Identification and dynamic PTY indicator
#define UECP_RDS_TA_TP                  0x03    // Traffic Announcement identification / Traffic Program identification
#define UECP_RDS_MS                     0x05    // Music/Speech switch
#define UECP_RDS_PTY                    0x07    // Program TYpe
#define UECP_RDS_PTYN                   0x3A    // Program TYpe Name
#define UECP_RDS_RT                     0x0A    // RadioText
#define UECP_RDS_AF                     0x13    // Alternative Frequencies list
#define UECP_RDS_EON_AF                 0x14    // Enhanced Other Networks information
#define UECP_SLOW_LABEL_CODES           0x1A    // Slow Labeling codes
#define UECP_LINKAGE_INFO               0x2E    // Linkage information

/// Open Data Application commands
#define UECP_ODA_CONF_SHORT_MSG_CMD     0x40    // ODA configuration and short message command
#define UECP_ODA_IDENT_GROUP_USAGE_SEQ  0x41    // ODA identification group usage sequence
#define UECP_ODA_FREE_FORMAT_GROUP      0x42    // ODA free-format group
#define UECP_ODA_REL_PRIOR_GROUP_SEQ    0x43    // ODA relative priority group sequence
#define UECP_ODA_BURST_MODE_CONTROL     0x44    // ODA “Burst mode” control
#define UECP_ODA_SPINN_WHEEL_TIMING_CTL 0x45    // ODA “Spinning Wheel” timing control
#define UECP_ODA_DATA                   0x46    // ODA Data
#define UECP_ODA_DATA_CMD_ACCESS_RIGHT  0x47    // ODA data command access right

/// DAB
#define UECP_DAB_DYN_LABEL_CMD          0x48    // DAB: Dynamic Label command
#define UECP_DAB_DYN_LABEL_MSG          0xAA    // DAB: Dynamic Label message (DL)

/// Transparent data commands
#define UECP_TDC_TDC                    0x26    // TDC
#define UECP_TDC_EWS                    0x2B    // EWS
#define UECP_TDC_IH                     0x25    // IH
#define UECP_TDC_TMC                    0x30    // TMC
#define UECP_TDC_FREE_FMT_GROUP         0x24    // Free-format group

/// Paging commands
#define UECP_PAGING_CALL_WITHOUT_MESSAGE                                        0x0C
#define UECP_PAGING_CALL_NUMERIC_MESSAGE_10DIGITS                               0x08
#define UECP_PAGING_CALL_NUMERIC_MESSAGE_18DIGITS                               0x20
#define UECP_PAGING_CALL_ALPHANUMERIC_MESSAGE_80CHARACTERS                      0x1B
#define UECP_INTERNATIONAL_PAGING_NUMERIC_MESSAGE_15DIGITS                      0x11
#define UECP_INTERNATIONAL_PAGING_FUNCTIONS_MESSAGE                             0x10
#define UECP_TRANSMITTER_NETWORK_GROUP_DESIGNATION                              0x12
#define UECP_EPP_TM_INFO                                                        0x31
#define UECP_EPP_CALL_WITHOUT_ADDITIONAL_MESSAGE                                0x32
#define UECP_EPP_NATIONAL_INTERNATIONAL_CALL_ALPHANUMERIC_MESSAGE               0x33
#define UECP_EPP_NATIONAL_INTERNATIONAL_CALL_VARIABLE_LENGTH_NUMERIC_MESSAGE    0x34
#define UECP_EPP_NATIONAL_INTERNATIONAL_CALL_VARIABLE_LENGTH_FUNCTIONS_MESSAGE  0x35

/// Clock setting and control
#define UECP_CLOCK_RTC                0x0D    // Real time clock
#define UECP_CLOCK_RTC_CORR           0x09    // Real time clock correction
#define UECP_CLOCK_CT_ON_OFF          0x19    // CT On/Off

/// RDS adjustment and control
#define RDS_ON_OFF                    0x1E
#define RDS_PHASE                     0x22
#define RDS_LEVEL                     0x0E

/// ARI adjustment and control
#define UECP_ARI_ARI_ON_OFF           0x21
#define UECP_ARI_ARI_AREA (BK)        0x0F
#define UECP_ARI_ARI_LEVEL            0x1F

/// Control and set up commands
#define UECP_CTR_SITE_ADDRESS         0x23
#define UECP_CTR_ENCODER_ADDRESS      0x27
#define UECP_CTR_MAKE_PSN_LIST        0x28
#define UECP_CTR_PSN_ENABLE_DISABLE   0x0B
#define UECP_CTR_COMMUNICATION_MODE   0x2C
#define UECP_CTR_TA_CONTROL           0x2A
#define UECP_CTR_EON_TA_CONTROL       0x15
#define UECP_CTR_REFERENCE_INPUT_SEL  0x1D
#define UECP_CTR_DATA_SET_SELECT      0x1C    // Data set select
#define UECP_CTR_GROUP_SEQUENCE       0x16
#define UECP_CTR_GROUP_VAR_CODE_SEQ   0x29
#define UECP_CTR_EXTENDED_GROUP_SEQ   0x38
#define UECP_CTR_PS_CHAR_CODE_TBL_SEL 0x2F
#define UECP_CTR_ENCODER_ACCESS_RIGHT 0x3A
#define UECP_CTR_COM_PORT_CONF_MODE   0x3B
#define UECP_CTR_COM_PORT_CONF_SPEED  0x3C
#define UECP_CTR_COM_PORT_CONF_TMEOUT 0x3D

/// Other commands
#define UECP_OTHER_RASS               0xda

/// Bi-directional commands (Remote and configuration commands)
#define BIDIR_MESSAGE_ACKNOWLEDGMENT  0x18
#define BIDIR_REQUEST_MESSAGE         0x17

/// Specific message commands
#define SPEC_MFG_SPECIFIC_CMD         0x2D

/**
 * RDS and RBDS relevant
 */

/// RDS Program type id's
enum {
  RDS_PTY_NONE = 0,
  RDS_PTY_NEWS,
  RDS_PTY_CURRENT_AFFAIRS,
  RDS_PTY_INFORMATION,
  RDS_PTY_SPORT,
  RDS_PTY_EDUCATION,
  RDS_PTY_DRAMA,
  RDS_PTY_CULTURE,
  RDS_PTY_SCIENCE,
  RDS_PTY_VARIED,
  RDS_PTY_POP_MUSIC,
  RDS_PTY_ROCK_MUSIC,
  RDS_PTY_MOR_MUSIC,
  RDS_PTY_LIGHT_CLASSICAL,
  RDS_PTY_SERIOUS_CLASSICAL,
  RDS_PTY_OTHER_MUSIC,
  RDS_PTY_WEATHER,
  RDS_PTY_FINANCE,
  RDS_PTY_CHILDRENS_PROGRAMMES,
  RDS_PTY_SOCIAL_AFFAIRS,
  RDS_PTY_RELIGION,
  RDS_PTY_PHONE_IN,
  RDS_PTY_TRAVEL,
  RDS_PTY_LEISURE,
  RDS_PTY_JAZZ_MUSIC,
  RDS_PTY_COUNTRY_MUSIC,
  RDS_PTY_NATIONAL_MUSIC,
  RDS_PTY_OLDIES_MUSIC,
  RDS_PTY_FOLK_MUSIC,
  RDS_PTY_DOCUMENTARY,
  RDS_PTY_ALARM_TEST,
  RDS_PTY_ALARM
};

/// RBDS Program type id's
enum {
  RBDS_PTY_NONE = 0,
  RBDS_PTY_NEWS,
  RBDS_PTY_INFORMATION,
  RBDS_PTY_SPORT,
  RBDS_PTY_TALK,
  RBDS_PTY_ROCK_MUSIC,
  RBDS_PTY_CLASSIC_ROCK_MUSIC,
  RBDS_PTY_ADULT_HITS,
  RBDS_PTY_SOFT_ROCK,
  RBDS_PTY_TOP_40,
  RBDS_PTY_COUNTRY,
  RBDS_PTY_OLDIES,
  RBDS_PTY_SOFT,
  RBDS_PTY_NOSTALGIA,
  RBDS_PTY_JAZZ,
  RBDS_PTY_CLASSICAL,
  RBDS_PTY_R__B,
  RBDS_PTY_SOFT_R__B,
  RBDS_PTY_LANGUAGE,
  RBDS_PTY_RELIGIOUS_MUSIC,
  RBDS_PTY_RELIGIOUS_TALK,
  RBDS_PTY_PERSONALITY,
  RBDS_PTY_PUBLIC,
  RBDS_PTY_COLLEGE,
  RBDS_PTY_WEATHER = 29,
  RBDS_PTY_EMERGENCY_TEST,
  RBDS_PTY_EMERGENCY
};

/// RadioText+ message type id's
enum {
  RTPLUS_DUMMY_CLASS        = 0,

  RTPLUS_ITEM_TITLE         = 1,
  RTPLUS_ITEM_ALBUM         = 2,
  RTPLUS_ITEM_TRACKNUMBER   = 3,
  RTPLUS_ITEM_ARTIST        = 4,
  RTPLUS_ITEM_COMPOSITION   = 5,
  RTPLUS_ITEM_MOVEMENT      = 6,
  RTPLUS_ITEM_CONDUCTOR     = 7,
  RTPLUS_ITEM_COMPOSER      = 8,
  RTPLUS_ITEM_BAND          = 9,
  RTPLUS_ITEM_COMMENT       = 10,
  RTPLUS_ITEM_GENRE         = 11,

  RTPLUS_INFO_NEWS          = 12,
  RTPLUS_INFO_NEWS_LOCAL    = 13,
  RTPLUS_INFO_STOCKMARKET   = 14,
  RTPLUS_INFO_SPORT         = 15,
  RTPLUS_INFO_LOTTERY       = 16,
  RTPLUS_INFO_HOROSCOPE     = 17,
  RTPLUS_INFO_DAILY_DIVERSION = 18,
  RTPLUS_INFO_HEALTH        = 19,
  RTPLUS_INFO_EVENT         = 20,
  RTPLUS_INFO_SZENE         = 21,
  RTPLUS_INFO_CINEMA        = 22,
  RTPLUS_INFO_STUPIDITY_MACHINE = 23,
  RTPLUS_INFO_DATE_TIME     = 24,
  RTPLUS_INFO_WEATHER       = 25,
  RTPLUS_INFO_TRAFFIC       = 26,
  RTPLUS_INFO_ALARM         = 27,
  RTPLUS_INFO_ADVERTISEMENT = 28,
  RTPLUS_INFO_URL           = 29,
  RTPLUS_INFO_OTHER         = 30,

  RTPLUS_STATIONNAME_SHORT  = 31,
  RTPLUS_STATIONNAME_LONG   = 32,

  RTPLUS_PROGRAMME_NOW      = 33,
  RTPLUS_PROGRAMME_NEXT     = 34,
  RTPLUS_PROGRAMME_PART     = 35,
  RTPLUS_PROGRAMME_HOST     = 36,
  RTPLUS_PROGRAMME_EDITORIAL_STAFF = 37,
  RTPLUS_PROGRAMME_FREQUENCY= 38,
  RTPLUS_PROGRAMME_HOMEPAGE = 39,
  RTPLUS_PROGRAMME_SUBCHANNEL = 40,

  RTPLUS_PHONE_HOTLINE      = 41,
  RTPLUS_PHONE_STUDIO       = 42,
  RTPLUS_PHONE_OTHER        = 43,

  RTPLUS_SMS_STUDIO         = 44,
  RTPLUS_SMS_OTHER          = 45,

  RTPLUS_EMAIL_HOTLINE      = 46,
  RTPLUS_EMAIL_STUDIO       = 47,
  RTPLUS_EMAIL_OTHER        = 48,

  RTPLUS_MMS_OTHER          = 49,

  RTPLUS_CHAT               = 50,
  RTPLUS_CHAT_CENTER        = 51,

  RTPLUS_VOTE_QUESTION      = 52,
  RTPLUS_VOTE_CENTER        = 53,

  RTPLUS_PLACE              = 59,
  RTPLUS_APPOINTMENT        = 60,
  RTPLUS_IDENTIFIER         = 61,
  RTPLUS_PURCHASE           = 62,
  RTPLUS_GET_DATA           = 63
};

/* page 71, Annex D, table D.1 in the standard and Annex N */
static const char *piCountryCodes_A[15][7]=
{
  // 0   1    2    3    4    5    6
  {"US","__","AI","BO","GT","__","__"}, // 1
  {"US","__","AG","CO","HN","__","__"}, // 2
  {"US","__","EC","JM","AW","__","__"}, // 3
  {"US","__","FK","MQ","__","__","__"}, // 4
  {"US","__","BB","GF","MS","__","__"}, // 5
  {"US","__","BZ","PY","TT","__","__"}, // 6
  {"US","__","KY","NI","PE","__","__"}, // 7
  {"US","__","CR","__","SR","__","__"}, // 8
  {"US","__","CU","PA","UY","__","__"}, // 9
  {"US","__","AR","DM","KN","__","__"}, // A
  {"US","CA","BR","DO","LC","MX","__"}, // B
  {"__","CA","BM","CL","SV","VC","__"}, // C
  {"US","CA","AN","GD","HT","MX","__"}, // D
  {"US","CA","GP","TC","VE","MX","__"}, // E
  {"__","GL","BS","GY","__","VG","PM"}  // F
};

static const char *piCountryCodes_D[15][7]=
{
  // 0   1    2    3    4    5    6
  {"CM","NA","SL","__","__","__","__"}, // 1
  {"CF","LR","ZW","__","__","__","__"}, // 2
  {"DJ","GH","MZ","EH","__","__","__"}, // 3
  {"MG","MR","UG","xx","__","__","__"}, // 4
  {"ML","ST","SZ","RW","__","__","__"}, // 5
  {"AO","CV","KE","LS","__","__","__"}, // 6
  {"GQ","SN","SO","__","__","__","__"}, // 7
  {"GA","GM","NE","SC","__","__","__"}, // 8
  {"GN","BI","TD","__","__","__","__"}, // 9
  {"ZA","AC","GW","MU","__","__","__"}, // A
  {"BF","BW","ZR","__","__","__","__"}, // B
  {"CG","KM","CI","SD","__","__","__"}, // C
  {"TG","TZ","Zanzibar","__","__","__","__"}, // D
  {"BJ","ET","ZM","__","__","__","__"}, // E
  {"MW","NG","__","__","__","__","__"}  // F
};

static const char *piCountryCodes_E[15][7]=
{
  // 0   1    2    3    4    5    6
  {"DE","GR","MA","__","MD","__","__"},
  {"DZ","CY","CZ","IE","EE","__","__"},
  {"AD","SM","PL","TR","KG","__","__"},
  {"IL","CH","VA","MK","__","__","__"},
  {"IT","JO","SK","TJ","__","__","__"},
  {"BE","FI","SY","__","UA","__","__"},
  {"RU","LU","TN","__","__","__","__"},
  {"PS","BG","__","NL","PT","__","__"},
  {"AL","DK","LI","LV","SI","__","__"}, // 9
  {"AT","GI","IS","LB","AM","__","__"}, // A
  {"HU","IQ","MC","AZ","UZ","__","__"}, // B
  {"MT","GB","LT","HR","GE","__","__"}, // C
  {"DE","LY","YU","KZ","__","__","__"}, // D
  {"__","RO","ES","SE","TM","__","__"}, // E
  {"EG","FR","NO","BY","BA","__","__"}  // F
};

static const char *piCountryCodes_F[15][7]=
{
  // 0   1    2    3    4    5    6
  {"AU","KI","KW","LA","__","__","__"}, // 1
  {"AU","BT","QA","TH","__","__","__"}, // 2
  {"AU","BD","KH","TO","__","__","__"}, // 3
  {"AU","PK","WS","__","__","__","__"}, // 4
  {"AU","FJ","IN","__","__","__","__"}, // 5
  {"AU","OM","MO","__","__","__","__"}, // 6
  {"AU","NR","VN","__","__","__","__"}, // 7
  {"AU","IR","PH","__","__","__","__"}, // 8
  {"SA","NZ","JP","PG","__","__","__"}, // 9
  {"AF","SB","SG","__","__","__","__"}, // A
  {"MM","BN","MV","YE","__","__","__"}, // B
  {"CN","LK","ID","__","__","__","__"}, // C
  {"KP","TW","AE","__","__","__","__"}, // D
  {"BH","KR","NP","FM","__","__","__"}, // E
  {"MY","HK","VU","MN","__","__","__"}  // F
};

/* see page 84, Annex J in the standard */
static const std::string piRDSLanguageCodes[128]=
{
  // 0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F
  "___", "alb", "bre", "cat", "hrv", "wel", "cze", "dan", "ger", "eng", "spa", "epo", "est", "baq", "fae", "fre", // 0
  "fry", "gle", "gla", "glg", "ice", "ita", "smi", "lat", "lav", "ltz", "lit", "hun", "mlt", "dut", "nor", "oci", // 1
  "pol", "por", "rum", "rom", "srp", "slo", "slv", "fin", "swe", "tur", "nld", "wln", "___", "___", "___", "___", // 2
  "___", "___", "___", "___", "___", "___", "___", "___", "___", "___", "___", "___", "___", "___", "___", "___", // 3
  "___", "___", "___", "___", "___", "zul", "vie", "uzb", "urd", "ukr", "tha", "tel", "tat", "tam", "tgk", "swa", // 4
  "srn", "som", "sin", "sna", "scc", "rue", "rus", "que", "pus", "pan", "per", "pap", "ori", "nep", "nde", "mar", // 5
  "mol", "mys", "mlg", "mkd", "_?_", "kor", "khm", "kaz", "kan", "jpn", "ind", "hin", "heb", "hau", "grn", "guj", // 6
  "gre", "geo", "ful", "prs", "chv", "chi", "bur", "bul", "ben", "bel", "bam", "aze", "asm", "arm", "ara", "amh"  // 7
};

/* ----------------------------------------------------------------------------------------------------------- */

#define EntityChars 56
static const char *entitystr[EntityChars]  = { "&apos;",   "&amp;",    "&quot;",  "&gt",      "&lt",      "&copy;",   "&times;", "&nbsp;",
                                               "&Auml;",   "&auml;",   "&Ouml;",  "&ouml;",   "&Uuml;",   "&uuml;",   "&szlig;", "&deg;",
                                               "&Agrave;", "&Aacute;", "&Acirc;", "&Atilde;", "&agrave;", "&aacute;", "&acirc;", "&atilde;",
                                               "&Egrave;", "&Eacute;", "&Ecirc;", "&Euml;",   "&egrave;", "&eacute;", "&ecirc;", "&euml;",
                                               "&Igrave;", "&Iacute;", "&Icirc;", "&Iuml;",   "&igrave;", "&iacute;", "&icirc;", "&iuml;",
                                               "&Ograve;", "&Oacute;", "&Ocirc;", "&Otilde;", "&ograve;", "&oacute;", "&ocirc;", "&otilde;",
                                               "&Ugrave;", "&Uacute;", "&Ucirc;", "&Ntilde;", "&ugrave;", "&uacute;", "&ucirc;", "&ntilde;" };
static const char *entitychar[EntityChars] = { "'",        "&",        "\"",      ">",        "<",         "c",        "*",      " ",
                                               "Ä",        "ä",        "Ö",       "ö",        "Ü",         "ü",        "ß",      "°",
                                               "À",        "Á",        "Â",       "Ã",        "à",         "á",        "â",      "ã",
                                               "È",        "É",        "Ê",       "Ë",        "è",         "é",        "ê",      "ë",
                                               "Ì",        "Í",        "Î",       "Ï",        "ì",         "í",        "î",      "ï",
                                               "Ò",        "Ó",        "Ô",       "Õ",        "ò",         "ó",        "ô",      "õ",
                                               "Ù",        "Ú",        "Û",       "Ñ",        "ù",         "ú",        "û",      "ñ" };

// RDS-Chartranslation: 0x80..0xff
static unsigned char sRDSAddChar[128] =
{
  0xe1, 0xe0, 0xe9, 0xe8, 0xed, 0xec, 0xf3, 0xf2,
  0xfa, 0xf9, 0xd1, 0xc7, 0x8c, 0xdf, 0x8e, 0x8f,
  0xe2, 0xe4, 0xea, 0xeb, 0xee, 0xef, 0xf4, 0xf6,
  0xfb, 0xfc, 0xf1, 0xe7, 0x9c, 0x9d, 0x9e, 0x9f,
  0xaa, 0xa1, 0xa9, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xa3, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xba, 0xb9, 0xb2, 0xb3, 0xb1, 0xa1, 0xb6, 0xb7,
  0xb5, 0xbf, 0xf7, 0xb0, 0xbc, 0xbd, 0xbe, 0xa7,
  0xc1, 0xc0, 0xc9, 0xc8, 0xcd, 0xcc, 0xd3, 0xd2,
  0xda, 0xd9, 0xca, 0xcb, 0xcc, 0xcd, 0xd0, 0xcf,
  0xc2, 0xc4, 0xca, 0xcb, 0xce, 0xcf, 0xd4, 0xd6,
  0xdb, 0xdc, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xc3, 0xc5, 0xc6, 0xe3, 0xe4, 0xdd, 0xd5, 0xd8,
  0xde, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xf0,
  0xe3, 0xe5, 0xe6, 0xf3, 0xf4, 0xfd, 0xf5, 0xf8,
  0xfe, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static char *rds_entitychar(char *text)
{
  int i = 0, l, lof, lre, space;
  char *temp;

  while (i < EntityChars)
  {
    if ((temp = strstr(text, entitystr[i])) != NULL)
    {
      l = strlen(entitystr[i]);
      lof = (temp-text);
      if (strlen(text) < RT_MEL)
      {
        lre = strlen(text) - lof - l;
        space = 1;
      }
      else
      {
        lre =  RT_MEL - 1 - lof - l;
        space = 0;
      }
      memmove(text+lof, entitychar[i], 1);
      memmove(text+lof+1, temp+l, lre);
      if (space != 0)
        memmove(text+lof+1+lre, "       ", l-1);
    }
    else
      ++i;
  }

  return text;
}

static unsigned short crc16_ccitt(const unsigned char *data, int len, bool skipfirst)
{
  // CRC16-CCITT: x^16 + x^12 + x^5 + 1
  // with start 0xffff and result inverse
  unsigned short crc = 0xffff;

  if (skipfirst)
    ++data;

  while (len--)
  {
    crc = (crc >> 8) | (crc << 8);
    crc ^= *data++;
    crc ^= (crc & 0xff) >> 4;
    crc ^= (crc << 8) << 4;
    crc ^= ((crc & 0xff) << 4) << 1;
  }

  return ~(crc);
}


/// --- CDVDRadioRDSData ------------------------------------------------------------

CDVDRadioRDSData::CDVDRadioRDSData(CProcessInfo &processInfo)
  : CThread("DVDRDSData")
  , IDVDStreamPlayer(processInfo)
  , m_speed(DVD_PLAYSPEED_NORMAL)
  , m_messageQueue("rds")
{
  CLog::Log(LOGDEBUG, "Radio UECP (RDS) Processor - new {}", __FUNCTION__);

  m_messageQueue.SetMaxDataSize(40 * 256 * 1024);
}

CDVDRadioRDSData::~CDVDRadioRDSData()
{
  CLog::Log(LOGDEBUG, "Radio UECP (RDS) Processor - delete {}", __FUNCTION__);
  StopThread();
}

bool CDVDRadioRDSData::CheckStream(const CDVDStreamInfo& hints)
{
  if (hints.type == STREAM_RADIO_RDS)
    return true;

  return false;
}

bool CDVDRadioRDSData::OpenStream(CDVDStreamInfo hints)
{
  CloseStream(true);

  m_messageQueue.Init();
  if (hints.type == STREAM_RADIO_RDS)
  {
    Flush();
    CLog::Log(LOGINFO, "Creating UECP (RDS) data thread");
    Create();
    return true;
  }
  return false;
}

void CDVDRadioRDSData::CloseStream(bool bWaitForBuffers)
{
  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGINFO, "Radio UECP (RDS) Processor - waiting for data thread to exit");

  StopThread(); // will set this->m_bStop to true

  m_messageQueue.End();
  m_currentInfoTag.reset();
  if (m_currentChannel)
    m_currentChannel->SetRadioRDSInfoTag(m_currentInfoTag);
  m_currentChannel.reset();
}

void CDVDRadioRDSData::ResetRDSCache()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  m_currentFileUpdate = false;

  m_UECPDataStart = false;
  m_UECPDatabStuff = false;
  m_UECPDataIndex = 0;

  m_RDS_IsRBDS = false;
  m_RDS_SlowLabelingCodesPresent = false;

  m_PI_Current = 0;
  m_PI_CountryCode = 0;
  m_PI_ProgramType = 0;
  m_PI_ProgramReferenceNumber = 0;

  m_EPP_TM_INFO_ExtendedCountryCode = 0;

  m_DI_IsStereo = true;
  m_DI_ArtificialHead = false;
  m_DI_Compressed = false;
  m_DI_DynamicPTY = false;

  m_TA_TP_TrafficAdvisory = false;
  m_TA_TP_TrafficVolume = 0.0;

  m_MS_SpeechActive = false;

  m_PTY = 0;
  memset(m_PTYN, 0x20, 8);
  m_PTYN[8] = 0;
  m_PTYN_Present = false;

  m_RT_NewItem = false;

  m_RTPlus_TToggle = false;
  m_RTPlus_Show = false;
  m_RTPlus_iToggle = 0;
  m_RTPlus_ItemToggle = 1;
  m_RTPlus_Title[0] = 0;
  m_RTPlus_Artist[0] = 0;
  m_RTPlus_Starttime = time(NULL);
  m_RTPlus_GenrePresent = false;

  m_currentInfoTag = std::make_shared<CPVRRadioRDSInfoTag>();
  m_currentChannel = g_application.CurrentFileItem().GetPVRChannelInfoTag();
  if (m_currentChannel)
    m_currentChannel->SetRadioRDSInfoTag(m_currentInfoTag);

  // send a message to all windows to tell them to update the radiotext
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_RADIOTEXT);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CDVDRadioRDSData::Process()
{
  CLog::Log(LOGINFO, "Radio UECP (RDS) Processor - running thread");

  while (!m_bStop)
  {
    std::shared_ptr<CDVDMsg> pMsg;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE) ? 1 : 0;
    MsgQueueReturnCode ret = m_messageQueue.Get(pMsg, 2s, iPriority);

    if (ret == MSGQ_TIMEOUT)
    {
      /* Timeout for RDS is not a bad thing, so we continue without error */
      continue;
    }

    if (MSGQ_IS_ERROR(ret))
    {
      if (!m_messageQueue.ReceivedAbortRequest())
        CLog::Log(LOGERROR, "MSGQ_IS_ERROR returned true ({})", ret);

      break;
    }

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      std::unique_lock<CCriticalSection> lock(m_critSection);

      DemuxPacket* pPacket = std::static_pointer_cast<CDVDMsgDemuxerPacket>(pMsg)->GetPacket();

      ProcessUECP(pPacket->pData, pPacket->iSize);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = std::static_pointer_cast<CDVDMsgInt>(pMsg)->m_value;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)
          || pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      ResetRDSCache();
    }
  }
}

void CDVDRadioRDSData::Flush()
{
  if(!m_messageQueue.IsInited())
    return;
  /* flush using message as this get's called from VideoPlayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(std::make_shared<CDVDMsg>(CDVDMsg::GENERAL_FLUSH));
}

void CDVDRadioRDSData::OnExit()
{
  CLog::Log(LOGINFO, "Radio UECP (RDS) Processor - thread end");
}

void CDVDRadioRDSData::SetRadioStyle(const std::string& genre)
{
  g_application.CurrentFileItem().GetMusicInfoTag()->SetGenre(genre);
  m_currentInfoTag->SetProgStyle(genre);
  m_currentFileUpdate = true;

  CLog::Log(LOGDEBUG, "Radio UECP (RDS) Processor - {} - Stream genre set to {}", __FUNCTION__,
            genre);
}

void CDVDRadioRDSData::ProcessUECP(const unsigned char *data, unsigned int len)
{
  for (unsigned int i = 0; i < len; ++i)
  {
    if (data[i] == UECP_DATA_START)                               //!< Start
    {
      m_UECPDataIndex  = -1;
      m_UECPDataStart  = true;
      m_UECPDatabStuff = false;
    }

    if (m_UECPDataStart)
    {
      //! byte-stuffing reverse: 0xfd00->0xfd, 0xfd01->0xfe, 0xfd02->0xff
      if (m_UECPDatabStuff == true)
      {
        switch (data[i])
        {
          case 0x00: m_UECPData[m_UECPDataIndex]   = 0xfd; break;
          case 0x01: m_UECPData[m_UECPDataIndex]   = 0xfe; break;
          case 0x02: m_UECPData[m_UECPDataIndex]   = 0xff; break;
          default:   m_UECPData[++m_UECPDataIndex] = data[i];      // should never be
        }
        m_UECPDatabStuff = false;
      }
      else
      {
        m_UECPData[++m_UECPDataIndex] = data[i];
      }

      if (data[i] == 0xfd && m_UECPDataIndex > 0)                 //!< stuffing found
        m_UECPDatabStuff = true;

      if (m_UECPDataIndex >= UECP_SIZE_MAX)                       //!< max. UECP data length, garbage ?
      {
        CLog::Log(LOGERROR, "Radio UECP (RDS) Processor - Error(TS): too long, garbage ?");
        m_UECPDataStart = false;
      }
    }

    if (m_UECPDataStart == true && data[i] == UECP_DATA_STOP && m_currentInfoTag)     //!< End
    {
      m_UECPDataStart = false;

      if (m_UECPDataIndex < 9)
      {
        CLog::Log(LOGERROR, "Radio UECP (RDS) Processor - Error(TS): too short -> garbage ?");
      }
      else
      {
        //! crc16-check
        unsigned short crc16 = crc16_ccitt(m_UECPData, m_UECPDataIndex-3, true);
        if (crc16 != (m_UECPData[m_UECPDataIndex-2]<<8) + m_UECPData[m_UECPDataIndex-1])
        {
          CLog::Log(LOGERROR,
                    "Radio UECP (RDS) Processor - Error(TS): wrong CRC # calc = {:04x} <> transmit "
                    "= {:02x}{:02x}",
                    crc16, m_UECPData[m_UECPDataIndex - 2], m_UECPData[m_UECPDataIndex - 1]);
        }
        else
        {
          m_UECPDataDeadBreak = false;

          unsigned int ret = 0;
          unsigned int ptr = 5;
          unsigned int len = m_UECPDataIndex-7;
          do
          {
            uint8_t *msg = m_UECPData+ptr;               //!< Current selected UECP message element (increased if more as one element is in frame)
            switch (msg[UECP_ME_MEC])
            {
              case UECP_RDS_PI:                 ret = DecodePI(msg);                  break;  //!< Program Identification
              case UECP_RDS_PS:                 ret = DecodePS(msg);                  break;  //!< Program Service name (PS)
              case UECP_RDS_DI:                 ret = DecodeDI(msg);                  break;  //!< Decoder Identification and dynamic PTY indicator
              case UECP_RDS_TA_TP:              ret = DecodeTA_TP(msg);               break;  //!< Traffic Announcement and Traffic Programme bits.
              case UECP_RDS_MS:                 ret = DecodeMS(msg);                  break;  //!< Music/Speech switch
              case UECP_RDS_PTY:                ret = DecodePTY(msg);                 break;  //!< Program Type
              case UECP_RDS_PTYN:               ret = DecodePTYN(msg);                break;  //!< Program Type Name
              case UECP_RDS_RT:                 ret = DecodeRT(msg, len);             break;  //!< RadioText
              case UECP_ODA_DATA:               ret = DecodeODA(msg, len);            break;  //!< Open Data Application
              case UECP_OTHER_RASS:             m_UECPDataDeadBreak = true;           break;  //!< Radio screen show (RaSS) (not present, before on SWR radio)
              case UECP_CLOCK_RTC:              ret = DecodeRTC(msg);                 break;  //!< Real time clock
              case UECP_TDC_TMC:                ret = DecodeTMC(msg, len);            break;  //!< Traffic message channel
              case UECP_EPP_TM_INFO:            ret = DecodeEPPTransmitterInfo(msg);  break;  //!< EPP transmitter information
              case UECP_SLOW_LABEL_CODES:       ret = DecodeSlowLabelingCodes(msg);   break;  //!< Slow Labeling codes
              case UECP_DAB_DYN_LABEL_CMD:      ret = DecodeDABDynLabelCmd(msg, len); break;  //!< DAB: Dynamic Label command
              case UECP_DAB_DYN_LABEL_MSG:      ret = DecodeDABDynLabelMsg(msg, len); break;  //!< DAB: Dynamic Label message (DL)
              case UECP_RDS_AF:                 ret = DecodeAF(msg, len);             break;  //!< Alternative Frequencies list
              case UECP_RDS_EON_AF:             ret = DecodeEonAF(msg, len);          break;  //!< EON Alternative Frequencies list
              case UECP_TDC_TDC:                ret = DecodeTDC(msg, len);            break;  //!< Transparent Data Channel
              case UECP_LINKAGE_INFO:           ret = 5;                              break;  //!< Linkage information
              case UECP_TDC_EWS:                ret = 6;                              break;  //!< Emergency warning system
              case UECP_RDS_PIN:                ret = 5;                              break;  //!< Program Item Number
              case UECP_TDC_IH:                 ret = 7;                              break;  //!< In-house applications (Should be ignored)
              case UECP_TDC_FREE_FMT_GROUP:     ret = 7;                              break;  //!< Free-format group (unused)
              case UECP_ODA_CONF_SHORT_MSG_CMD: ret = 8;                              break;  //!< ODA Configuration and Short Message Command (unused)
              case UECP_CLOCK_RTC_CORR:         ret = 3;                              break;  //!< Real time clock correction (unused)
              case UECP_CLOCK_CT_ON_OFF:        ret = 2;                              break;  //!< Real time clock on/off (unused)
              default:
#ifdef RDS_IMPROVE_CHECK
                printf("Unknown UECP data packet = 0x%02X\n", msg[UECP_ME_MEC]);
#endif
                m_UECPDataDeadBreak = true;
                break;
            }
            ptr += ret;
            len -= ret;
          }
          while (ptr < m_UECPDataIndex-5 && !m_UECPDataDeadBreak && !m_bStop);

          if (m_currentFileUpdate && !m_bStop)
          {
            CServiceBroker::GetGUI()->GetInfoManager().SetCurrentItem(g_application.CurrentFileItem());
            m_currentFileUpdate = false;
          }
        }
      }
    }
  }
}

unsigned int CDVDRadioRDSData::DecodePI(const uint8_t* msgElement)
{
  uint16_t PICode = (msgElement[3] << 8) | msgElement[4];
  if (m_PI_Current != PICode)
  {
    m_PI_Current = PICode;

    m_PI_CountryCode             = (m_PI_Current>>12) & 0x0F;
    m_PI_ProgramType             = (m_PI_Current>>8) & 0x0F;
    m_PI_ProgramReferenceNumber  = m_PI_Current & 0xFF;

    CLog::Log(LOGINFO,
              "Radio UECP (RDS) Processor - PI code changed to Country {:X}, Type {:X} and "
              "reference no. {}",
              m_PI_CountryCode, m_PI_ProgramType, m_PI_ProgramReferenceNumber);
  }

  return 5;
}

unsigned int CDVDRadioRDSData::DecodePS(uint8_t *msgElement)
{
  uint8_t *text = msgElement+3;

  char decodedText[9] = {};
  for (int i = 0; i < 8; ++i)
  {
    if (text[i] <= 0xfe)
      decodedText[i] = (text[i] >= 0x80)
                           ? sRDSAddChar[text[i] - 0x80]
                           : text[i]; //!< additional rds-character, see RBDS-Standard, Annex E
  }

  m_currentInfoTag->SetProgramServiceText(decodedText);

  return 11;
}

unsigned int CDVDRadioRDSData::DecodeDI(const uint8_t* msgElement)
{
  bool value;

  value = (msgElement[3] & 1) != 0;
  if (m_DI_IsStereo != value)
  {
    CLog::Log(LOGDEBUG, "Radio UECP (RDS) Processor - {} - Stream changed over to {}", __FUNCTION__,
              value ? "Stereo" : "Mono");
    m_DI_IsStereo = value;
  }

  value = (msgElement[3] & 2) != 0;
  if (m_DI_ArtificialHead != value)
  {
    CLog::Log(LOGDEBUG,
              "Radio UECP (RDS) Processor - {} - Stream changed over to {}Artificial Head",
              __FUNCTION__, value ? "" : "Not ");
    m_DI_ArtificialHead = value;
  }

  value = (msgElement[3] & 4) != 0;
  if (m_DI_ArtificialHead != value)
  {
    CLog::Log(LOGDEBUG,
              "Radio UECP (RDS) Processor - {} - Stream changed over to {}Compressed Head",
              __FUNCTION__, value ? "" : "Not ");
    m_DI_ArtificialHead = value;
  }

  value = (msgElement[3] & 8) != 0;
  if (m_DI_DynamicPTY != value)
  {
    CLog::Log(LOGDEBUG, "Radio UECP (RDS) Processor - {} - Stream changed over to {} PTY",
              __FUNCTION__, value ? "dynamic" : "static");
    m_DI_DynamicPTY = value;
  }

  return 4;
}

unsigned int CDVDRadioRDSData::DecodeTA_TP(const uint8_t* msgElement)
{
  uint8_t dsn = msgElement[1];
  bool traffic_announcement = (msgElement[3] & 1) != 0;
  bool traffic_programme    = (msgElement[3] & 2) != 0;

  if (traffic_announcement && !m_TA_TP_TrafficAdvisory && traffic_programme && dsn == 0 && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("pvrplayback.trafficadvisory"))
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(19021), g_localizeStrings.Get(29930));
    m_TA_TP_TrafficAdvisory = true;
    auto& components = CServiceBroker::GetAppComponents();
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    m_TA_TP_TrafficVolume = appVolume->GetVolumePercent();
    float trafAdvVol = (float)CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt("pvrplayback.trafficadvisoryvolume");
    if (trafAdvVol)
      appVolume->SetVolume(m_TA_TP_TrafficVolume + trafAdvVol);

    CVariant data(CVariant::VariantTypeObject);
    data["on"] = true;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::PVR, "RDSRadioTA", data);
  }

  if (!traffic_announcement && m_TA_TP_TrafficAdvisory && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool("pvrplayback.trafficadvisory"))
  {
    m_TA_TP_TrafficAdvisory = false;
    auto& components = CServiceBroker::GetAppComponents();
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    appVolume->SetVolume(m_TA_TP_TrafficVolume);

    CVariant data(CVariant::VariantTypeObject);
    data["on"] = false;
    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::PVR, "RDSRadioTA", data);
  }

  return 4;
}

unsigned int CDVDRadioRDSData::DecodeMS(const uint8_t* msgElement)
{
  bool speechActive = msgElement[3] == 0;
  if (m_MS_SpeechActive != speechActive)
  {
    m_currentInfoTag->SetSpeechActive(m_MS_SpeechActive);
    CLog::Log(LOGDEBUG, "Radio UECP (RDS) Processor - {} - Stream changed over to {}", __FUNCTION__,
              speechActive ? "Speech" : "Music");
    m_MS_SpeechActive = speechActive;
  }

  return 4;
}

/*!
 * EBU - SPB 490 - 3.3.7 and 62106IEC:1999 - 3.2.1.2, Message Name: Programme Type
 * Message Element Code: 07
 */
//! @todo Improve and test alarm message
typedef struct { const char *style_name; int name; } pty_skin_info;
pty_skin_info pty_skin_info_table[32][2] =
{
  { { "none",           29940 }, { "none",            29940 } },
  { { "news",           29941 }, { "news",            29941 } },
  { { "currentaffairs", 29942 }, { "information",     29943 } },
  { { "information",    29943 }, { "sport",           29944 } },
  { { "sport",          29944 }, { "talk",            29939 } },
  { { "education",      29945 }, { "rockmusic",       29951 } },
  { { "drama",          29946 }, { "classicrockmusic",29977 } },
  { { "cultures",       29947 }, { "adulthits",       29937 } },
  { { "science",        29948 }, { "softrock",        29938 } },
  { { "variedspeech",   29949 }, { "top40",           29972 } },
  { { "popmusic",       29950 }, { "countrymusic",    29965 } },
  { { "rockmusic",      29951 }, { "oldiesmusic",     29967 } },
  { { "easylistening",  29952 }, { "softmusic",       29936 } },
  { { "lightclassics",  29953 }, { "nostalgia",       29979 } },
  { { "seriousclassics",29954 }, { "jazzmusic",       29964 } },
  { { "othermusic",     29955 }, { "classical",       29978 } },
  { { "weather",        29956 }, { "randb",           29975 } },
  { { "finance",        29957 }, { "softrandb",       29976 } },
  { { "childrensprogs", 29958 }, { "language",        29932 } },
  { { "socialaffairs",  29959 }, { "religiousmusic",  29973 } },
  { { "religion",       29960 }, { "religioustalk",   29974 } },
  { { "phonein",        29961 }, { "personality",     29934 } },
  { { "travelandtouring",29962 },{ "public",          29935 } },
  { { "leisureandhobby",29963 }, { "college",         29933 } },
  { { "jazzmusic",      29964 }, { "spanishtalk",     29927 } },
  { { "countrymusic",   29965 }, { "spanishmusic",    29928 } },
  { { "nationalmusic",  29966 }, { "hiphop",          29929 } },
  { { "oldiesmusic",    29967 }, { "",                -1    } },
  { { "folkmusic",      29968 }, { "",                -1    } },
  { { "documentary",    29969 }, { "weather",         29956 } },
  { { "alarmtest",      29970 }, { "alarmtest",       29970 } },
  { { "alarm-alarm",    29971 }, { "alarm-alarm",     29971 } }
};

unsigned int CDVDRadioRDSData::DecodePTY(const uint8_t* msgElement)
{
  int pty = msgElement[3];
  if (pty >= 0 && pty < 32 && m_PTY != pty)
  {
    m_PTY = pty;

    // save info
    m_currentInfoTag->SetRadioStyle(pty_skin_info_table[m_PTY][m_RDS_IsRBDS].style_name);
    if (!m_RTPlus_GenrePresent && !m_PTYN_Present)
      SetRadioStyle(g_localizeStrings.Get(pty_skin_info_table[m_PTY][m_RDS_IsRBDS].name));

    if (m_PTY == RDS_PTY_ALARM_TEST)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(29931), g_localizeStrings.Get(29970), TOAST_DISPLAY_TIME, false);

    if (m_PTY == RDS_PTY_ALARM)
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, g_localizeStrings.Get(29931), g_localizeStrings.Get(29971), TOAST_DISPLAY_TIME*2, true);
    }
  }

  return 4;
}

unsigned int CDVDRadioRDSData::DecodePTYN(uint8_t *msgElement)
{
  // decode Text
  uint8_t *text = msgElement+3;

  for (int i = 0; i < 8; ++i)
  {
    if (text[i] <= 0xfe)
      m_PTYN[i] = (text[i] >= 0x80) ? sRDSAddChar[text[i]-0x80] : text[i];
  }

  m_PTYN_Present = true;

  if (!m_RTPlus_GenrePresent)
  {
    std::string progTypeName = StringUtils::Format(
        "{}: {}", g_localizeStrings.Get(pty_skin_info_table[m_PTY][m_RDS_IsRBDS].name), m_PTYN);
    SetRadioStyle(progTypeName);
  }

  return 11;
}

inline void rtrim_str(std::string &text)
{
  for (int i = text.length()-1; i >= 0; --i)
  {
    if (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r')
      text[i] = 0;
    else
      break;
  }
}

unsigned int CDVDRadioRDSData::DecodeRT(uint8_t *msgElement, unsigned int len)
{
  m_currentInfoTag->SetPlayingRadioText(true);

  int bufConf = (msgElement[UECP_ME_DATA] >> 5) & 0x03;
  unsigned int msgLength = msgElement[UECP_ME_MEL];
  if (msgLength > len-2)
  {
    CLog::Log(LOGERROR,
              "Radio UECP (RDS) - {} - RT-Error: Length=0 or not correct (MFL= {}, MEL= {})",
              __FUNCTION__, len, msgLength);
    m_UECPDataDeadBreak = true;
    return 0;
  }
  else if (msgLength == 0 || (msgLength == 1 && bufConf == 0))
  {
    return msgLength + 4;
  }
  else
  {
  //  bool flagToggle = msgElement[UECP_ME_DATA] & 0x01 ? true : false;
  //  int txQty = (msgElement[UECP_ME_DATA] >> 1) & 0x0F;
  //  int bufConf = (msgElement[UECP_ME_DATA] >> 5) & 0x03;

    //! byte 4 = RT-Status bitcodet (0=AB-flagcontrol, 1-4=Transmission-Number, 5+6=Buffer-Config, ignored, always 0x01 ?)
    char temptext[RT_MEL];
    memset(temptext, 0x0, RT_MEL);
    for (unsigned int i = 1, ii = 0; i < msgLength; ++i)
    {
      if (msgElement[UECP_ME_DATA+i] <= 0xfe) // additional rds-character, see RBDS-Standard, Annex E
        temptext[ii++] = (msgElement[UECP_ME_DATA+i] >= 0x80) ? sRDSAddChar[msgElement[UECP_ME_DATA+i]-0x80] : msgElement[UECP_ME_DATA+i];
    }

    memcpy(m_RTPlus_WorkText, temptext, RT_MEL);
    rds_entitychar(temptext);

    m_currentInfoTag->SetRadioText(temptext);

    m_RTPlus_iToggle = 0x03;     // Bit 0/1 = Title/Artist
  }
  return msgLength+4;
}

#define UECP_CLOCK_YEAR        1
#define UECP_CLOCK_MONTH       2
#define UECP_CLOCK_DAY         3
#define UECP_CLOCK_HOURS       4
#define UECP_CLOCK_MINUTES     5
#define UECP_CLOCK_SECONDS     6
#define UECP_CLOCK_CENTSEC     7
#define UECP_CLOCK_LOCALOFFSET 8
unsigned int CDVDRadioRDSData::DecodeRTC(uint8_t *msgElement)
{
  uint8_t hours   = msgElement[UECP_CLOCK_HOURS];
  uint8_t minutes = msgElement[UECP_CLOCK_MINUTES];
  bool    minus   = (msgElement[UECP_CLOCK_LOCALOFFSET] & 0x20) != 0;
  if (minus)
  {
    if (msgElement[UECP_CLOCK_LOCALOFFSET] >> 1)
      hours -= msgElement[UECP_CLOCK_LOCALOFFSET] >> 1;
    if (msgElement[UECP_CLOCK_LOCALOFFSET] & 1)
      minutes -= 30;
  }
  else
  {
    if (msgElement[UECP_CLOCK_LOCALOFFSET] >> 1)
      hours += msgElement[UECP_CLOCK_LOCALOFFSET] >> 1;
    if (msgElement[UECP_CLOCK_LOCALOFFSET] & 1)
      minutes += 30;
  }
  m_RTC_DateTime.SetDateTime(msgElement[UECP_CLOCK_YEAR], msgElement[UECP_CLOCK_MONTH], msgElement[UECP_CLOCK_DAY],
                            hours, minutes, msgElement[UECP_CLOCK_SECONDS]);

  CLog::Log(LOGDEBUG,
            "Radio UECP (RDS) - {} - Current RDS Data Time: {:02}.{:02}.{:02} - UTC: "
            "{:02}:{:02}:{:02},0.{}s - Local: {}{} min",
            __FUNCTION__, msgElement[UECP_CLOCK_DAY], msgElement[UECP_CLOCK_MONTH],
            msgElement[UECP_CLOCK_YEAR], msgElement[UECP_CLOCK_HOURS],
            msgElement[UECP_CLOCK_MINUTES], msgElement[UECP_CLOCK_SECONDS],
            msgElement[UECP_CLOCK_CENTSEC], minus ? '-' : '+',
            msgElement[UECP_CLOCK_LOCALOFFSET] * 30);

  CVariant data(CVariant::VariantTypeObject);
  data["dateTime"] = (m_RTC_DateTime.IsValid()) ? m_RTC_DateTime.GetAsRFC1123DateTime() : "";
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::PVR, "RDSRadioRTC", data);

  return 8;
}

unsigned int CDVDRadioRDSData::DecodeODA(uint8_t *msgElement, unsigned int len)
{
  unsigned int procData = msgElement[1];
  if (procData == 0 || procData > len-2)
  {
    CLog::Log(LOGERROR, "Radio UECP (RDS) - Invalid ODA data size");
    m_UECPDataDeadBreak = true;
    return 0;
  }

  switch ((msgElement[2]<<8)+msgElement[3])      // ODA-ID
  {
    case 0x4bd7: //!< RT+
      procData = DecodeRTPlus(msgElement, len);
      break;
    case 0x0d45: //!< TMC Alert-C
    case 0xcd46:
      SendTMCSignal(msgElement[4], msgElement+5);
      break;
    default:
      m_UECPDataDeadBreak = true;
#ifdef RDS_IMPROVE_CHECK
      printf("[RDS-ODA AID '%02x%02x' not used -> End]\n", msgElement[2], msgElement[3]);
#endif // RDS_IMPROVE_CHECK
      break;
  }
  return procData;
}

unsigned int CDVDRadioRDSData::DecodeRTPlus(uint8_t *msgElement, unsigned int len)
{
  if (m_RTPlus_iToggle == 0)    // RTplus tags V2.1, only if RT
    return 10;

  m_currentInfoTag->SetPlayingRadioTextPlus(true);

  if (msgElement[1] > len-2 || msgElement[1] != 8)  // byte 6 = MEL, only 8 byte for 2 tags
  {
    CLog::Log(LOGERROR, "Radio UECP (RDS) - {} - RTp-Error: Length not correct (MEL= {})",
              __FUNCTION__, msgElement[1]);
    m_UECPDataDeadBreak = true;
    return 0;
  }
  unsigned int rtp_typ[2], rtp_start[2], rtp_len[2];
  // byte 2+3 = ApplicationID, always 0x4bd7
  // byte 4   = Applicationgroup Typecode / PTY ?
  // bit 10#4 = Item Togglebit
  // bit 10#3 = Item Runningbit321
  // Tag1: bit 10#2..11#5 = Contenttype, 11#4..12#7 = Startmarker, 12#6..12#1 = Length
  rtp_typ[0]   = (0x38 & msgElement[5]<<3) | msgElement[6]>>5;
  rtp_start[0] = (0x3e & msgElement[6]<<1) | msgElement[7]>>7;
  rtp_len[0]   = 0x3f & msgElement[7]>>1;
  // Tag2: bit 12#0..13#3 = Contenttype, 13#2..14#5 = Startmarker, 14#4..14#0 = Length(5bit)
  rtp_typ[1]   = (0x20 & msgElement[7]<<5) | msgElement[8]>>3;
  rtp_start[1] = (0x38 & msgElement[8]<<3) | msgElement[9]>>5;
  rtp_len[1]   = 0x1f & msgElement[9];

  /// Hack for error on BR Classic
  if ((msgElement[5]&0x10) && (msgElement[5]&0x08) && rtp_typ[0] == RTPLUS_INFO_URL && rtp_typ[1] == RTPLUS_ITEM_ARTIST)
    return 10;

  // save info
  MUSIC_INFO::CMusicInfoTag *currentMusic = g_application.CurrentFileItem().GetMusicInfoTag();

  for (int i = 0; i < 2; ++i)
  {
    if (rtp_start[i]+rtp_len[i]+1 >= RT_MEL)  // length-error
    {
      CLog::Log(
          LOGERROR,
          "Radio UECP (RDS) - {} - (tag#{} = Typ/Start/Len): {}/{}/{} (Start+Length > 'RT-MEL' !)",
          __FUNCTION__, i + 1, rtp_typ[i], rtp_start[i], rtp_len[i]);
    }
    else
    {
      // +Memory
      memset(m_RTPlus_Temptext, 0x20, RT_MEL);
      memcpy(m_RTPlus_Temptext, m_RTPlus_WorkText+rtp_start[i], rtp_len[i]+1);
      m_RTPlus_Temptext[rtp_len[i]+1] = 0;
      rds_entitychar(m_RTPlus_Temptext);
      switch (rtp_typ[i])
      {
        case RTPLUS_DUMMY_CLASS:
          break;
        case RTPLUS_ITEM_TITLE:     // Item-Title...
          if ((msgElement[5] & 0x08) > 0 && (m_RTPlus_iToggle & 0x01) == 0x01)
          {
            m_RTPlus_iToggle -= 0x01;
            if (memcmp(m_RTPlus_Title, m_RTPlus_Temptext, RT_MEL) != 0 || (msgElement[5] & 0x10) != m_RTPlus_ItemToggle)
            {
              memcpy(m_RTPlus_Title, m_RTPlus_Temptext, RT_MEL);
              if (m_RTPlus_Show && m_RTPlus_iTime.GetElapsedSeconds() > 1)
                m_RTPlus_iDiffs = (int) m_RTPlus_iTime.GetElapsedSeconds();
              if (!m_RT_NewItem)
              {
                m_RTPlus_Starttime = time(NULL);
                m_RTPlus_iTime.StartZero();
                m_RTPlus_Artist[0] = 0;
              }
              m_RT_NewItem = (!m_RT_NewItem) ? true : false;
              m_RTPlus_Show = m_RTPlus_TToggle = true;
            }
          }
          break;
        case RTPLUS_ITEM_ALBUM:
          m_currentInfoTag->SetAlbum(m_RTPlus_Temptext);
          currentMusic->SetAlbum(m_RTPlus_Temptext);
          break;
        case RTPLUS_ITEM_TRACKNUMBER:
          m_currentInfoTag->SetAlbumTrackNumber(atoi(m_RTPlus_Temptext));
          currentMusic->SetAlbumId(atoi(m_RTPlus_Temptext));
          break;
        case RTPLUS_ITEM_ARTIST:     // Item-Artist..
          if ((msgElement[5] & 0x08) > 0 && (m_RTPlus_iToggle & 0x02) == 0x02)
          {
            m_RTPlus_iToggle -= 0x02;
            if (memcmp(m_RTPlus_Artist, m_RTPlus_Temptext, RT_MEL) != 0 || (msgElement[5] & 0x10) != m_RTPlus_ItemToggle)
            {
              memcpy(m_RTPlus_Artist, m_RTPlus_Temptext, RT_MEL);
              if (m_RTPlus_Show && m_RTPlus_iTime.GetElapsedSeconds() > 1)
                m_RTPlus_iDiffs = (int) m_RTPlus_iTime.GetElapsedSeconds();
              if (!m_RT_NewItem)
              {
                m_RTPlus_Starttime = time(NULL);
                m_RTPlus_iTime.StartZero();
                m_RTPlus_Title[0] = 0;
              }
              m_RT_NewItem = (!m_RT_NewItem) ? true : false;
              m_RTPlus_Show = m_RTPlus_TToggle = true;
            }
          }
          break;
        case RTPLUS_ITEM_CONDUCTOR:
          m_currentInfoTag->SetConductor(m_RTPlus_Temptext);
          break;
        case RTPLUS_ITEM_COMPOSER:
        case RTPLUS_ITEM_COMPOSITION:
          m_currentInfoTag->SetComposer(m_RTPlus_Temptext);
          if (m_currentInfoTag->GetRadioStyle() == "unknown")
            m_currentInfoTag->SetRadioStyle("classical");
          break;
        case RTPLUS_ITEM_BAND:
          m_currentInfoTag->SetBand(m_RTPlus_Temptext);
          break;
        case RTPLUS_ITEM_COMMENT:
          m_currentInfoTag->SetComment(m_RTPlus_Temptext);
          break;
        case RTPLUS_ITEM_GENRE:
          {
            std::string str = m_RTPlus_Temptext;
            g_charsetConverter.unknownToUTF8(str);
            m_RTPlus_GenrePresent = true;
            m_currentInfoTag->SetProgStyle(str);
          }
          break;
        case RTPLUS_INFO_NEWS:    // Info_News
          m_currentInfoTag->SetInfoNews(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_NEWS_LOCAL:    // Info_NewsLocal
          m_currentInfoTag->SetInfoNewsLocal(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_STOCKMARKET:    // Info_Stockmarket
          m_currentInfoTag->SetInfoStock(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_SPORT:    // Info_Sport
          m_currentInfoTag->SetInfoSport(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_LOTTERY:    // Info_Lottery
          m_currentInfoTag->SetInfoLottery(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_HOROSCOPE:
          m_currentInfoTag->SetInfoHoroscope(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_CINEMA:
          m_currentInfoTag->SetInfoCinema(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_WEATHER:    // Info_Weather/
          m_currentInfoTag->SetInfoWeather(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_URL:    // Info_Url
          if (m_currentInfoTag->GetProgWebsite().empty())
            m_currentInfoTag->SetProgWebsite(m_RTPlus_Temptext);
          break;
        case RTPLUS_INFO_OTHER:    // Info_Other
          m_currentInfoTag->SetInfoOther(m_RTPlus_Temptext);
          break;
        case RTPLUS_STATIONNAME_LONG:    // Programme_Stationname.Long
          m_currentInfoTag->SetProgStation(m_RTPlus_Temptext);
          break;
        case RTPLUS_PROGRAMME_NOW:    // Programme_Now
          m_currentInfoTag->SetProgNow(m_RTPlus_Temptext);
          break;
        case RTPLUS_PROGRAMME_NEXT:    // Programme_Next
          m_currentInfoTag->SetProgNext(m_RTPlus_Temptext);
          break;
        case RTPLUS_PROGRAMME_HOST:    // Programme_Host
          m_currentInfoTag->SetProgHost(m_RTPlus_Temptext);
          break;
        case RTPLUS_PROGRAMME_EDITORIAL_STAFF:    // Programme_EditorialStaff
          m_currentInfoTag->SetEditorialStaff(m_RTPlus_Temptext);
          break;
        case RTPLUS_PROGRAMME_HOMEPAGE:    // Programme_Homepage
          m_currentInfoTag->SetProgWebsite(m_RTPlus_Temptext);
          break;
        case RTPLUS_PHONE_HOTLINE:    // Phone_Hotline
          m_currentInfoTag->SetPhoneHotline(m_RTPlus_Temptext);
          break;
        case RTPLUS_PHONE_STUDIO:    // Phone_Studio
          m_currentInfoTag->SetPhoneStudio(m_RTPlus_Temptext);
          break;
        case RTPLUS_SMS_STUDIO:    // SMS_Studio
          m_currentInfoTag->SetSMSStudio(m_RTPlus_Temptext);
          break;
        case RTPLUS_EMAIL_HOTLINE:    // Email_Hotline
          m_currentInfoTag->SetEMailHotline(m_RTPlus_Temptext);
          break;
        case RTPLUS_EMAIL_STUDIO:    // Email_Studio
          m_currentInfoTag->SetEMailStudio(m_RTPlus_Temptext);
          break;
        /**
         * Currently unused radiotext plus messages
         * Must be check where present and if it is usable
         */
        case RTPLUS_ITEM_MOVEMENT:
        case RTPLUS_INFO_DAILY_DIVERSION:
        case RTPLUS_INFO_HEALTH:
        case RTPLUS_INFO_EVENT:
        case RTPLUS_INFO_SZENE:
        case RTPLUS_INFO_STUPIDITY_MACHINE:
        case RTPLUS_INFO_TRAFFIC:
        case RTPLUS_INFO_ALARM:
        case RTPLUS_INFO_ADVERTISEMENT:
        case RTPLUS_PROGRAMME_PART:
        case RTPLUS_PROGRAMME_FREQUENCY:
        case RTPLUS_PROGRAMME_SUBCHANNEL:
        case RTPLUS_PHONE_OTHER:
        case RTPLUS_SMS_OTHER:
        case RTPLUS_EMAIL_OTHER:
        case RTPLUS_MMS_OTHER:
        case RTPLUS_CHAT:
        case RTPLUS_CHAT_CENTER:
        case RTPLUS_VOTE_QUESTION:
        case RTPLUS_VOTE_CENTER:
        case RTPLUS_PLACE:
        case RTPLUS_APPOINTMENT:
        case RTPLUS_IDENTIFIER:
        case RTPLUS_PURCHASE:
        case RTPLUS_GET_DATA:
#ifdef RDS_IMPROVE_CHECK
          printf("  RTp-Unkn. : %02i - %s\n", rtp_typ[i], m_RTPlus_Temptext);
          break;
#endif // RDS_IMPROVE_CHECK
        /// Unused and not needed data information
        case RTPLUS_STATIONNAME_SHORT:  //!< Must be rechecked under DAB
        case RTPLUS_INFO_DATE_TIME:
          break;
        default:
          break;
      }
    }
  }

  // Title-end @ no Item-Running'
  if ((msgElement[5] & 0x08) == 0)
  {
    m_RTPlus_Title[0]    = 0;
    m_RTPlus_Artist[0]   = 0;
    m_currentInfoTag->ResetSongInformation();
    currentMusic->SetAlbum("");
    if (m_RTPlus_GenrePresent)
    {
      m_currentInfoTag->SetProgStyle("");
      m_RTPlus_GenrePresent = false;
    }

    if (m_RTPlus_Show)
    {
      m_RTPlus_Show = false;
      m_RTPlus_TToggle = true;
      m_RTPlus_iDiffs = (int) m_RTPlus_iTime.GetElapsedSeconds();
      m_RTPlus_Starttime = time(NULL);
    }
    m_RT_NewItem = false;
  }

  if (m_RTPlus_TToggle)
  {
#ifdef RDS_IMPROVE_CHECK
    {
      struct tm tm_store;
      struct tm *ts = localtime_r(&m_RTPlus_Starttime, &tm_store);
      if (m_RTPlus_iDiffs > 0)
        printf("  StartTime : %02d:%02d:%02d  (last Title elapsed = %d s)\n", ts->tm_hour, ts->tm_min, ts->tm_sec, m_RTPlus_iDiffs);
      else
        printf("  StartTime : %02d:%02d:%02d\n", ts->tm_hour, ts->tm_min, ts->tm_sec);
      printf("  RTp-Title : %s\n  RTp-Artist: %s\n", m_RTPlus_Title, m_RTPlus_Artist);
    }
#endif // RDS_IMPROVE_CHECK
    m_RTPlus_ItemToggle = msgElement[5] & 0x10;
    m_RTPlus_TToggle = false;
    m_RTPlus_iDiffs = 0;

    std::string str;

    str = m_RTPlus_Artist;
    m_currentInfoTag->SetArtist(str);
    if (str.empty() && !m_currentInfoTag->GetComposer().empty())
      str = m_currentInfoTag->GetComposer();
    else if (str.empty() && !m_currentInfoTag->GetConductor().empty())
      str = m_currentInfoTag->GetConductor();
    else if (str.empty() && !m_currentInfoTag->GetBand().empty())
      str = m_currentInfoTag->GetBand();

    if (!str.empty())
      g_charsetConverter.unknownToUTF8(str);
    currentMusic->SetArtist(str);

    str = m_RTPlus_Title;
    g_charsetConverter.unknownToUTF8(str);
    currentMusic->SetTitle(str);
    m_currentInfoTag->SetTitle(str);
    m_currentFileUpdate = true;
  }
  m_RTPlus_iToggle = 0;

  return 10;
}

unsigned int CDVDRadioRDSData::DecodeTMC(uint8_t *msgElement, unsigned int len)
{
  unsigned int msgElementLength = msgElement[1];
  if (msgElementLength == 0)
    msgElementLength = 6;
  if (msgElementLength + 2 > len)
  {
    m_UECPDataDeadBreak = true;
    return 0;
  }

  for (unsigned int i = 0; i < msgElementLength; i += 5)
    SendTMCSignal(msgElement[2], msgElement+3+i);

  return msgElementLength + 2;
}

unsigned int CDVDRadioRDSData::DecodeEPPTransmitterInfo(const uint8_t* msgElement)
{
  if (!m_RDS_SlowLabelingCodesPresent && m_PI_CountryCode != 0)
  {
    int codeHigh = msgElement[2]&0xF0;
    int codeLow  = msgElement[2]&0x0F;
    if (codeLow > 7)
    {
      CLog::Log(LOGERROR, "Radio RDS - {} - invalid country code {:#02X}{:02X}", __FUNCTION__,
                codeHigh, codeLow);
      return 7;
    }

    std::string countryName;
    switch (codeHigh)
    {
      case 0xA0:
        countryName = piCountryCodes_A[m_PI_CountryCode-1][codeLow];
        break;
      case 0xD0:
        countryName = piCountryCodes_D[m_PI_CountryCode-1][codeLow];
        break;
      case 0xE0:
        countryName = piCountryCodes_E[m_PI_CountryCode-1][codeLow];
        break;
      case 0xF0:
        countryName = piCountryCodes_F[m_PI_CountryCode-1][codeLow];
        break;
      default:
        CLog::Log(LOGERROR, "Radio RDS - {} - invalid extended country region code:{:02X}{:02X}",
                  __FUNCTION__, codeHigh, codeLow);
        return 7;
    }

    // The United States, Canada, and Mexico use the RBDS standard
    m_RDS_IsRBDS = (countryName == "US" || countryName == "CA" || countryName == "MX");

    m_currentInfoTag->SetCountry(countryName);
  }

  return 7;
}

/* SLOW LABELLING: see page 23 in the standard
 * for paging see page 90, Annex M in the standard (NOT IMPLEMENTED)
 * for extended country codes see page 69, Annex D.2 in the standard
 * for language codes see page 84, Annex J in the standard
 * for emergency warning systems (EWS) see page 53 in the standard */
#define VARCODE_PAGING_EXTCOUNTRYCODE   0
#define VARCODE_TMC_IDENT               1
#define VARCODE_PAGING_IDENT            2
#define VARCODE_LANGUAGE_CODES          3
#define VARCODE_OWN_BROADCASTER         6
#define VARCODE_EWS_CHANNEL_IDENT       7
unsigned int CDVDRadioRDSData::DecodeSlowLabelingCodes(const uint8_t* msgElement)
{
  uint16_t slowLabellingCode = (msgElement[2]<<8 | msgElement[3]) & 0xfff;
  int      VariantCode       = (msgElement[2]>>4) & 0x7;

  switch (VariantCode)
  {
    case VARCODE_PAGING_EXTCOUNTRYCODE:      // paging + ecc
    {
      // int paging = (slowLabellingCode>>8)&0x0f; unused

      if (m_PI_CountryCode != 0)
      {
        int codeHigh    = slowLabellingCode&0xF0;
        int codeLow     = slowLabellingCode&0x0F;
        if (codeLow > 5)
        {
          CLog::Log(LOGERROR, "Radio RDS - {} - invalid country code {:#02X}{:02X}", __FUNCTION__,
                    codeHigh, codeLow);
          return 4;
        }

        std::string countryName;
        switch (codeHigh)
        {
          case 0xA0:
            countryName = piCountryCodes_A[m_PI_CountryCode-1][codeLow];
            break;
          case 0xD0:
            countryName = piCountryCodes_D[m_PI_CountryCode-1][codeLow];
            break;
          case 0xE0:
            countryName = piCountryCodes_E[m_PI_CountryCode-1][codeLow];
            break;
          case 0xF0:
            countryName = piCountryCodes_F[m_PI_CountryCode-1][codeLow];
            break;
          default:
            CLog::Log(LOGERROR,
                      "Radio RDS - {} - invalid extended country region code:{:02X}{:02X}",
                      __FUNCTION__, codeHigh, codeLow);
            return 4;
        }

        m_currentInfoTag->SetCountry(countryName);
      }
      break;
    }
    case VARCODE_LANGUAGE_CODES:      // language codes
      if (slowLabellingCode > 1 && slowLabellingCode < 0x80)
        m_currentInfoTag->SetLanguage(piRDSLanguageCodes[slowLabellingCode]);
      else
        CLog::Log(LOGERROR, "Radio RDS - {} - invalid language code {}", __FUNCTION__,
                  slowLabellingCode);
      break;

    case VARCODE_TMC_IDENT:           // TMC identification
    case VARCODE_PAGING_IDENT:        // Paging identification
    case VARCODE_OWN_BROADCASTER:
    case VARCODE_EWS_CHANNEL_IDENT:
    default:
      break;
  }

  m_RDS_SlowLabelingCodesPresent = true;
  return 4;
}

/*!
 * currently unused need to be checked on DAB, processed here to have length of it
 */
unsigned int CDVDRadioRDSData::DecodeDABDynLabelCmd(const uint8_t* msgElement, unsigned int len)
{
  unsigned int msgElementLength = msgElement[1];
  if (msgElementLength < 1 || msgElementLength + 2 > len)
  {
    m_UECPDataDeadBreak = true;
    return 0;
  }

  return msgElementLength+2;
}

/*!
 * currently unused need to be checked on DAB, processed here to have length of it
 */
unsigned int CDVDRadioRDSData::DecodeDABDynLabelMsg(const uint8_t* msgElement, unsigned int len)
{
  unsigned int msgElementLength = msgElement[1];
  if (msgElementLength < 2 || msgElementLength + 2 > len)
  {
    m_UECPDataDeadBreak = true;
    return 0;
  }

  return msgElementLength+2;
}

/*!
 *  unused processed here to have length of it
 */
unsigned int CDVDRadioRDSData::DecodeAF(uint8_t *msgElement, unsigned int len)
{
  unsigned int msgElementLength = msgElement[3];
  if (msgElementLength < 3 || msgElementLength + 4 > len)
  {
    m_UECPDataDeadBreak = true;
    return 0;
  }

  return msgElementLength+4;
}

/*!
 *  unused processed here to have length of it
 */
unsigned int CDVDRadioRDSData::DecodeEonAF(uint8_t *msgElement, unsigned int len)
{
  unsigned int msgElementLength = msgElement[3];
  if (msgElementLength < 4 || msgElementLength + 4 > len)
  {
    m_UECPDataDeadBreak = true;
    return 0;
  }

  return msgElementLength+4;
}

/*!
 *  unused processed here to have length of it
 */
unsigned int CDVDRadioRDSData::DecodeTDC(uint8_t *msgElement, unsigned int len)
{
  unsigned int msgElementLength = msgElement[1];
  if (msgElementLength < 2 || msgElementLength+2 > len)
  {
    m_UECPDataDeadBreak = true;
    return 0;
  }

  return msgElementLength+2;
}

void CDVDRadioRDSData::SendTMCSignal(unsigned int flags, uint8_t *data)
{
  if (!(flags & 0x80) && (memcmp(data, m_TMC_LastData, 5) == 0))
    return;

  memcpy(m_TMC_LastData, data, 5);

  if (m_currentChannel)
  {
    CVariant msg(CVariant::VariantTypeObject);
    msg["channel"] = m_currentChannel->ChannelName();
    msg["ident"]   = m_PI_Current;
    msg["flags"]   = flags;
    msg["x"]       = m_TMC_LastData[0];
    msg["y"]       = (unsigned int)(m_TMC_LastData[1]<<8 | m_TMC_LastData[2]);
    msg["z"]       = (unsigned int)(m_TMC_LastData[3]<<8 | m_TMC_LastData[4]);

    CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::PVR, "RDSRadioTMC", msg);
  }
}

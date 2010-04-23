/*
 * wirbelscan.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __WIRBELSCAN_SERVICE_H
#define __WIRBELSCAN_SERVICE_H

typedef enum scantype
{
  DVB_TERR    = 0,
  DVB_CABLE   = 1,
  DVB_SAT     = 2,
  PVRINPUT    = 3,
  PVRINPUT_FM = 4,
  DVB_ATSC    = 5,
} scantype_t;

typedef void (*WirbelScanService_GetCountries_v1_0)(int index, const char *isoName, const char *longName);
typedef void (*WirbelScanService_GetSatellites_v1_0)(int index, const char *shortName, const char *longName);

struct WirbelScanService_DoScan_v1_0
{
  scantype_t  type;

  bool        scan_tv;
  bool        scan_radio;
  bool        scan_fta;
  bool        scan_scrambled;
  bool        scan_hd;

  int         CountryIndex;

  int         DVBC_Inversion;
  int         DVBC_Symbolrate;
  int         DVBC_QAM;

  int         DVBT_Inversion;

  int         SatIndex;

  int         ATSC_Type;

  void (*SetPercentage)(int percent);
  void (*SetSignalStrength)(int strenght, bool locked);
  void (*SetDeviceInfo)(const char *Info);
  void (*SetTransponder)(const char *Info);
  void (*NewChannel)(const char *Name, bool isRadio, bool isEncrypted, bool isHD);
  void (*IsFinished)();
  void (*SetStatus)(int status);
};

#endif //__WIRBELSCAN_SERVICE_H


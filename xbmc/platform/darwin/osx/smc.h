/*
 *  Apple System Management Control (SMC) Tool
 *  Copyright (C) 2006 devnull
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <libkern/OSTypes.h>

#define SMC_VERSION               "0.01"

#define OP_NONE               0
#define OP_LIST               1
#define OP_READ               2
#define OP_READ_FAN           3
#define OP_WRITE              4

#define KERNEL_INDEX_SMC      2

#define SMC_CMD_READ_BYTES    5
#define SMC_CMD_WRITE_BYTES   6
#define SMC_CMD_READ_INDEX    8
#define SMC_CMD_READ_KEYINFO  9
#define SMC_CMD_READ_PLIMIT   11
#define SMC_CMD_READ_VERS     12

#define DATATYPE_FPE2         "fpe2"
#define DATATYPE_UINT8        "ui8 "
#define DATATYPE_UINT16       "ui16"
#define DATATYPE_UINT32       "ui32"
#define DATATYPE_SP78         "sp78"

// key values
#define SMC_KEY_CPU_TEMP      "TC0D"
#define SMC_KEY_GPU_TEMP      "TG0D"
#define SMC_KEY_FAN0_RPM_MIN  "F0Mn"
#define SMC_KEY_FAN1_RPM_MIN  "F1Mn"
#define SMC_KEY_FAN0_RPM_CUR  "F0Ac"
#define SMC_KEY_FAN1_RPM_CUR  "F1Ac"


typedef struct {
  char                  major;
  char                  minor;
  char                  build;
  char                  reserved[1];
  UInt16                release;
} SMCKeyData_vers_t;

typedef struct {
  UInt16                version;
  UInt16                length;
  UInt32                cpuPLimit;
  UInt32                gpuPLimit;
  UInt32                memPLimit;
} SMCKeyData_pLimitData_t;

typedef struct {
  UInt32                dataSize;
  UInt32                dataType;
  char                  dataAttributes;
} SMCKeyData_keyInfo_t;

typedef char              SMCBytes_t[32];

typedef struct {
  UInt32                  key;
  SMCKeyData_vers_t       vers;
  SMCKeyData_pLimitData_t pLimitData;
  SMCKeyData_keyInfo_t    keyInfo;
  char                    result;
  char                    status;
  char                    data8;
  UInt32                  data32;
  SMCBytes_t              bytes;
} SMCKeyData_t;

typedef const char        UInt32ConstChar_t[5];
typedef char              UInt32Char_t[5];

typedef struct {
  UInt32Char_t            key;
  UInt32                  dataSize;
  UInt32Char_t            dataType;
  SMCBytes_t              bytes;
} SMCVal_t;

#ifdef __cplusplus
extern "C"
{
#endif

// prototypes
double SMCGetTemperature(const char *key);

#ifdef __cplusplus
}
#endif

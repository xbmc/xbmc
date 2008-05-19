/* 7zCrc.h */

#ifndef __7Z_CRC_H
#define __7Z_CRC_H

#include <stddef.h>

#include "Types.h"

extern UInt32 g_CrcTable[];

void MY_FAST_CALL CrcGenerateTable(void);

#define CRC_INIT_VAL 0xFFFFFFFF
#define CRC_GET_DIGEST(crc) ((crc) ^ 0xFFFFFFFF)
#define CRC_UPDATE_BYTE(crc, b) (g_CrcTable[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))

UInt32 MY_FAST_CALL CrcUpdate(UInt32 crc, const void *data, size_t size);
UInt32 MY_FAST_CALL CrcCalc(const void *data, size_t size);

#endif

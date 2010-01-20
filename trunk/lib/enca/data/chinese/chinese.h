/* This header file is in the public domain. */
#ifndef CHINESE_H
#define CHINESE_H
#include <string.h>

struct zh_weight
{
  unsigned char name[2];
  double freq;
};

typedef const struct zh_weight *RateFunc (const unsigned char *str);
typedef int ValidityFunc (const unsigned char *str);

#include "zh_weight_gbk.h"
#include "zh_weight_big5.h"
#endif

/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <unicode/icundk.h>
#include <android/log.h>


#include <unicode/icudataver.h>
#include <unicode/putil.h>
#include <unicode/ubidi.h>
#include <unicode/ubrk.h>
#include <unicode/ucal.h>
#include <unicode/ucasemap.h>
#include <unicode/ucat.h>
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>
#include <unicode/ucnv_err.h>
#include <unicode/ucnvsel.h>
#include <unicode/ucol.h>
#include <unicode/ucoleitr.h>
#include <unicode/ucsdet.h>
#include <unicode/ucurr.h>
#include <unicode/udat.h>
#include <unicode/udata.h>
#include <unicode/udateintervalformat.h>
#include <unicode/udatpg.h>
#include <unicode/uenum.h>
#include <unicode/ufieldpositer.h>
#include <unicode/uformattable.h>
#include <unicode/ugender.h>
#include <unicode/uidna.h>
#include <unicode/uiter.h>
#include <unicode/uldnames.h>
#include <unicode/ulistformatter.h>
#include <unicode/uloc.h>
#include <unicode/ulocdata.h>
#include <unicode/umsg.h>
#include <unicode/unorm2.h>
#include <unicode/unum.h>
#include <unicode/unumsys.h>
#include <unicode/upluralrules.h>
#include <unicode/uregex.h>
#include <unicode/uregion.h>
#include <unicode/ures.h>
#include <unicode/uscript.h>
#include <unicode/usearch.h>
#include <unicode/uset.h>
#include <unicode/ushape.h>
#include <unicode/uspoof.h>
#include <unicode/usprep.h>
#include <unicode/ustring.h>
#include <unicode/utext.h>
#include <unicode/utf8.h>
#include <unicode/utmscale.h>
#include <unicode/utrace.h>
#include <unicode/utrans.h>
#include <unicode/utypes.h>
#include <unicode/uversion.h>

/* Allowed version number ranges between [44, 999].
 * 44 is the minimum supported ICU version that was shipped in
 * Gingerbread (2.3.3) devices.
 */
#define ICUDATA_VERSION_MIN_LENGTH 2
#define ICUDATA_VERSION_MAX_LENGTH 3
#define ICUDATA_VERSION_MIN 44

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static char g_icudata_version[ICUDATA_VERSION_MAX_LENGTH + 1];

static const char kLogTag[] = "NDKICU";

static void* handle_common = nullptr;
static void* handle_i18n = nullptr;

static int __icu_dat_file_filter(const dirent* dirp) {
  const char* name = dirp->d_name;

  // Is the name the right length to match 'icudt(\d\d\d)l.dat'?
  const size_t len = strlen(name);
  if (len < 10 + ICUDATA_VERSION_MIN_LENGTH ||
      len > 10 + ICUDATA_VERSION_MAX_LENGTH) {
    return 0;
  }

  // Check that the version is a valid decimal number.
  for (int i = 5; i < len - 5; i++) {
    if (!isdigit(name[i])) {
      return 0;
    }
  }

  return !strncmp(name, "icudt", 5) && !strncmp(&name[len - 5], "l.dat", 5);
}

static void init_icudata_version() {
  dirent** namelist = nullptr;
  int n =
      scandir("/system/usr/icu", &namelist, &__icu_dat_file_filter, alphasort);
  int max_version = -1;
  while (n--) {
    // We prefer the latest version available.
    int version = atoi(&namelist[n]->d_name[strlen("icudt")]);
    if (version != 0 && version > max_version) {
      max_version = version;
    }
    free(namelist[n]);
  }
  free(namelist);

  if (max_version < ICUDATA_VERSION_MIN) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        "Cannot locate ICU data file at /system/usr/icu.");
    abort();
  }

  snprintf(g_icudata_version, sizeof(g_icudata_version), "_%d", max_version);

  handle_i18n = dlopen("libicui18n.so", RTLD_LOCAL);
  if (handle_i18n == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        "Could not open libicui18n: %s", dlerror());
    abort();
  }

  handle_common = dlopen("libicuuc.so", RTLD_LOCAL);
  if (handle_common == nullptr) {
    __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                        "Could not open libicuuc: %s", dlerror());
    abort();
  }
}

bool ndk_is_icu_function_available(const char* name) {
  pthread_once(&once_control, &init_icudata_version);

  char versioned_symbol_name[strlen(name) +
                             sizeof(g_icudata_version)];
  snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s", name,
           g_icudata_version);

  if (dlsym(handle_common, versioned_symbol_name) != nullptr) {
    return true;
  }

  if (dlsym(handle_i18n, versioned_symbol_name) != nullptr) {
    return true;
  }

  return false;
}

void* do_dlsym(void** handle, const char* name) {
  pthread_once(&once_control, &init_icudata_version);

  char versioned_symbol_name[strlen(name) + sizeof(g_icudata_version)];
  snprintf(versioned_symbol_name, sizeof(versioned_symbol_name), "%s%s", name,
           g_icudata_version);

  return dlsym(*handle, versioned_symbol_name);
}

[[noreturn]] void do_fail(const char* name) {
  __android_log_print(ANDROID_LOG_FATAL, kLogTag,
                      "Attempted to call unavailable ICU function %s: %s",
                      name, dlerror());
  abort();
}

void uenum_close(UEnumeration * en) {
  typedef decltype(&uenum_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uenum_close"));

  if (ptr == nullptr) {
    do_fail("uenum_close");
  }

  ptr(en);
}
int32_t uenum_count(UEnumeration * en, UErrorCode * status) {
  typedef decltype(&uenum_count) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uenum_count"));

  if (ptr == nullptr) {
    do_fail("uenum_count");
  }

  return ptr(en, status);
}
const UChar * uenum_unext(UEnumeration * en, int32_t * resultLength, UErrorCode * status) {
  typedef decltype(&uenum_unext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uenum_unext"));

  if (ptr == nullptr) {
    do_fail("uenum_unext");
  }

  return ptr(en, resultLength, status);
}
const char * uenum_next(UEnumeration * en, int32_t * resultLength, UErrorCode * status) {
  typedef decltype(&uenum_next) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uenum_next"));

  if (ptr == nullptr) {
    do_fail("uenum_next");
  }

  return ptr(en, resultLength, status);
}
void uenum_reset(UEnumeration * en, UErrorCode * status) {
  typedef decltype(&uenum_reset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uenum_reset"));

  if (ptr == nullptr) {
    do_fail("uenum_reset");
  }

  ptr(en, status);
}
UEnumeration * uenum_openUCharStringsEnumeration(const UChar *const  strings[], int32_t count, UErrorCode * ec) {
  typedef decltype(&uenum_openUCharStringsEnumeration) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uenum_openUCharStringsEnumeration"));

  if (ptr == nullptr) {
    do_fail("uenum_openUCharStringsEnumeration");
  }

  return ptr(strings, count, ec);
}
UEnumeration * uenum_openCharStringsEnumeration(const char *const  strings[], int32_t count, UErrorCode * ec) {
  typedef decltype(&uenum_openCharStringsEnumeration) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uenum_openCharStringsEnumeration"));

  if (ptr == nullptr) {
    do_fail("uenum_openCharStringsEnumeration");
  }

  return ptr(strings, count, ec);
}
int ucnv_compareNames(const char * name1, const char * name2) {
  typedef decltype(&ucnv_compareNames) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_compareNames"));

  if (ptr == nullptr) {
    do_fail("ucnv_compareNames");
  }

  return ptr(name1, name2);
}
UConverter * ucnv_open(const char * converterName, UErrorCode * err) {
  typedef decltype(&ucnv_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_open"));

  if (ptr == nullptr) {
    do_fail("ucnv_open");
  }

  return ptr(converterName, err);
}
UConverter * ucnv_openU(const UChar * name, UErrorCode * err) {
  typedef decltype(&ucnv_openU) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_openU"));

  if (ptr == nullptr) {
    do_fail("ucnv_openU");
  }

  return ptr(name, err);
}
UConverter * ucnv_openCCSID(int32_t codepage, UConverterPlatform platform, UErrorCode * err) {
  typedef decltype(&ucnv_openCCSID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_openCCSID"));

  if (ptr == nullptr) {
    do_fail("ucnv_openCCSID");
  }

  return ptr(codepage, platform, err);
}
UConverter * ucnv_openPackage(const char * packageName, const char * converterName, UErrorCode * err) {
  typedef decltype(&ucnv_openPackage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_openPackage"));

  if (ptr == nullptr) {
    do_fail("ucnv_openPackage");
  }

  return ptr(packageName, converterName, err);
}
UConverter * ucnv_safeClone(const UConverter * cnv, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status) {
  typedef decltype(&ucnv_safeClone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_safeClone"));

  if (ptr == nullptr) {
    do_fail("ucnv_safeClone");
  }

  return ptr(cnv, stackBuffer, pBufferSize, status);
}
void ucnv_close(UConverter * converter) {
  typedef decltype(&ucnv_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_close"));

  if (ptr == nullptr) {
    do_fail("ucnv_close");
  }

  ptr(converter);
}
void ucnv_getSubstChars(const UConverter * converter, char * subChars, int8_t * len, UErrorCode * err) {
  typedef decltype(&ucnv_getSubstChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getSubstChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_getSubstChars");
  }

  ptr(converter, subChars, len, err);
}
void ucnv_setSubstChars(UConverter * converter, const char * subChars, int8_t len, UErrorCode * err) {
  typedef decltype(&ucnv_setSubstChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_setSubstChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_setSubstChars");
  }

  ptr(converter, subChars, len, err);
}
void ucnv_setSubstString(UConverter * cnv, const UChar * s, int32_t length, UErrorCode * err) {
  typedef decltype(&ucnv_setSubstString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_setSubstString"));

  if (ptr == nullptr) {
    do_fail("ucnv_setSubstString");
  }

  ptr(cnv, s, length, err);
}
void ucnv_getInvalidChars(const UConverter * converter, char * errBytes, int8_t * len, UErrorCode * err) {
  typedef decltype(&ucnv_getInvalidChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getInvalidChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_getInvalidChars");
  }

  ptr(converter, errBytes, len, err);
}
void ucnv_getInvalidUChars(const UConverter * converter, UChar * errUChars, int8_t * len, UErrorCode * err) {
  typedef decltype(&ucnv_getInvalidUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getInvalidUChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_getInvalidUChars");
  }

  ptr(converter, errUChars, len, err);
}
void ucnv_reset(UConverter * converter) {
  typedef decltype(&ucnv_reset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_reset"));

  if (ptr == nullptr) {
    do_fail("ucnv_reset");
  }

  ptr(converter);
}
void ucnv_resetToUnicode(UConverter * converter) {
  typedef decltype(&ucnv_resetToUnicode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_resetToUnicode"));

  if (ptr == nullptr) {
    do_fail("ucnv_resetToUnicode");
  }

  ptr(converter);
}
void ucnv_resetFromUnicode(UConverter * converter) {
  typedef decltype(&ucnv_resetFromUnicode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_resetFromUnicode"));

  if (ptr == nullptr) {
    do_fail("ucnv_resetFromUnicode");
  }

  ptr(converter);
}
int8_t ucnv_getMaxCharSize(const UConverter * converter) {
  typedef decltype(&ucnv_getMaxCharSize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getMaxCharSize"));

  if (ptr == nullptr) {
    do_fail("ucnv_getMaxCharSize");
  }

  return ptr(converter);
}
int8_t ucnv_getMinCharSize(const UConverter * converter) {
  typedef decltype(&ucnv_getMinCharSize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getMinCharSize"));

  if (ptr == nullptr) {
    do_fail("ucnv_getMinCharSize");
  }

  return ptr(converter);
}
int32_t ucnv_getDisplayName(const UConverter * converter, const char * displayLocale, UChar * displayName, int32_t displayNameCapacity, UErrorCode * err) {
  typedef decltype(&ucnv_getDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getDisplayName"));

  if (ptr == nullptr) {
    do_fail("ucnv_getDisplayName");
  }

  return ptr(converter, displayLocale, displayName, displayNameCapacity, err);
}
const char * ucnv_getName(const UConverter * converter, UErrorCode * err) {
  typedef decltype(&ucnv_getName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getName"));

  if (ptr == nullptr) {
    do_fail("ucnv_getName");
  }

  return ptr(converter, err);
}
int32_t ucnv_getCCSID(const UConverter * converter, UErrorCode * err) {
  typedef decltype(&ucnv_getCCSID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getCCSID"));

  if (ptr == nullptr) {
    do_fail("ucnv_getCCSID");
  }

  return ptr(converter, err);
}
UConverterPlatform ucnv_getPlatform(const UConverter * converter, UErrorCode * err) {
  typedef decltype(&ucnv_getPlatform) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getPlatform"));

  if (ptr == nullptr) {
    do_fail("ucnv_getPlatform");
  }

  return ptr(converter, err);
}
UConverterType ucnv_getType(const UConverter * converter) {
  typedef decltype(&ucnv_getType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getType"));

  if (ptr == nullptr) {
    do_fail("ucnv_getType");
  }

  return ptr(converter);
}
void ucnv_getStarters(const UConverter * converter, UBool  starters[256], UErrorCode * err) {
  typedef decltype(&ucnv_getStarters) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getStarters"));

  if (ptr == nullptr) {
    do_fail("ucnv_getStarters");
  }

  ptr(converter, starters, err);
}
void ucnv_getUnicodeSet(const UConverter * cnv, USet * setFillIn, UConverterUnicodeSet whichSet, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_getUnicodeSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getUnicodeSet"));

  if (ptr == nullptr) {
    do_fail("ucnv_getUnicodeSet");
  }

  ptr(cnv, setFillIn, whichSet, pErrorCode);
}
void ucnv_getToUCallBack(const UConverter * converter, UConverterToUCallback * action, const void ** context) {
  typedef decltype(&ucnv_getToUCallBack) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getToUCallBack"));

  if (ptr == nullptr) {
    do_fail("ucnv_getToUCallBack");
  }

  ptr(converter, action, context);
}
void ucnv_getFromUCallBack(const UConverter * converter, UConverterFromUCallback * action, const void ** context) {
  typedef decltype(&ucnv_getFromUCallBack) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getFromUCallBack"));

  if (ptr == nullptr) {
    do_fail("ucnv_getFromUCallBack");
  }

  ptr(converter, action, context);
}
void ucnv_setToUCallBack(UConverter * converter, UConverterToUCallback newAction, const void * newContext, UConverterToUCallback * oldAction, const void ** oldContext, UErrorCode * err) {
  typedef decltype(&ucnv_setToUCallBack) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_setToUCallBack"));

  if (ptr == nullptr) {
    do_fail("ucnv_setToUCallBack");
  }

  ptr(converter, newAction, newContext, oldAction, oldContext, err);
}
void ucnv_setFromUCallBack(UConverter * converter, UConverterFromUCallback newAction, const void * newContext, UConverterFromUCallback * oldAction, const void ** oldContext, UErrorCode * err) {
  typedef decltype(&ucnv_setFromUCallBack) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_setFromUCallBack"));

  if (ptr == nullptr) {
    do_fail("ucnv_setFromUCallBack");
  }

  ptr(converter, newAction, newContext, oldAction, oldContext, err);
}
void ucnv_fromUnicode(UConverter * converter, char ** target, const char * targetLimit, const UChar ** source, const UChar * sourceLimit, int32_t * offsets, UBool flush, UErrorCode * err) {
  typedef decltype(&ucnv_fromUnicode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_fromUnicode"));

  if (ptr == nullptr) {
    do_fail("ucnv_fromUnicode");
  }

  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
}
void ucnv_toUnicode(UConverter * converter, UChar ** target, const UChar * targetLimit, const char ** source, const char * sourceLimit, int32_t * offsets, UBool flush, UErrorCode * err) {
  typedef decltype(&ucnv_toUnicode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_toUnicode"));

  if (ptr == nullptr) {
    do_fail("ucnv_toUnicode");
  }

  ptr(converter, target, targetLimit, source, sourceLimit, offsets, flush, err);
}
int32_t ucnv_fromUChars(UConverter * cnv, char * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_fromUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_fromUChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_fromUChars");
  }

  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucnv_toUChars(UConverter * cnv, UChar * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_toUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_toUChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_toUChars");
  }

  return ptr(cnv, dest, destCapacity, src, srcLength, pErrorCode);
}
UChar32 ucnv_getNextUChar(UConverter * converter, const char ** source, const char * sourceLimit, UErrorCode * err) {
  typedef decltype(&ucnv_getNextUChar) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getNextUChar"));

  if (ptr == nullptr) {
    do_fail("ucnv_getNextUChar");
  }

  return ptr(converter, source, sourceLimit, err);
}
void ucnv_convertEx(UConverter * targetCnv, UConverter * sourceCnv, char ** target, const char * targetLimit, const char ** source, const char * sourceLimit, UChar * pivotStart, UChar ** pivotSource, UChar ** pivotTarget, const UChar * pivotLimit, UBool reset, UBool flush, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_convertEx) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_convertEx"));

  if (ptr == nullptr) {
    do_fail("ucnv_convertEx");
  }

  ptr(targetCnv, sourceCnv, target, targetLimit, source, sourceLimit, pivotStart, pivotSource, pivotTarget, pivotLimit, reset, flush, pErrorCode);
}
int32_t ucnv_convert(const char * toConverterName, const char * fromConverterName, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_convert) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_convert"));

  if (ptr == nullptr) {
    do_fail("ucnv_convert");
  }

  return ptr(toConverterName, fromConverterName, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_toAlgorithmic(UConverterType algorithmicType, UConverter * cnv, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_toAlgorithmic) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_toAlgorithmic"));

  if (ptr == nullptr) {
    do_fail("ucnv_toAlgorithmic");
  }

  return ptr(algorithmicType, cnv, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_fromAlgorithmic(UConverter * cnv, UConverterType algorithmicType, char * target, int32_t targetCapacity, const char * source, int32_t sourceLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_fromAlgorithmic) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_fromAlgorithmic"));

  if (ptr == nullptr) {
    do_fail("ucnv_fromAlgorithmic");
  }

  return ptr(cnv, algorithmicType, target, targetCapacity, source, sourceLength, pErrorCode);
}
int32_t ucnv_flushCache() {
  typedef decltype(&ucnv_flushCache) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_flushCache"));

  if (ptr == nullptr) {
    do_fail("ucnv_flushCache");
  }

  return ptr();
}
int32_t ucnv_countAvailable() {
  typedef decltype(&ucnv_countAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_countAvailable"));

  if (ptr == nullptr) {
    do_fail("ucnv_countAvailable");
  }

  return ptr();
}
const char * ucnv_getAvailableName(int32_t n) {
  typedef decltype(&ucnv_getAvailableName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getAvailableName"));

  if (ptr == nullptr) {
    do_fail("ucnv_getAvailableName");
  }

  return ptr(n);
}
UEnumeration * ucnv_openAllNames(UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_openAllNames) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_openAllNames"));

  if (ptr == nullptr) {
    do_fail("ucnv_openAllNames");
  }

  return ptr(pErrorCode);
}
uint16_t ucnv_countAliases(const char * alias, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_countAliases) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_countAliases"));

  if (ptr == nullptr) {
    do_fail("ucnv_countAliases");
  }

  return ptr(alias, pErrorCode);
}
const char * ucnv_getAlias(const char * alias, uint16_t n, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_getAlias) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getAlias"));

  if (ptr == nullptr) {
    do_fail("ucnv_getAlias");
  }

  return ptr(alias, n, pErrorCode);
}
void ucnv_getAliases(const char * alias, const char ** aliases, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_getAliases) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getAliases"));

  if (ptr == nullptr) {
    do_fail("ucnv_getAliases");
  }

  ptr(alias, aliases, pErrorCode);
}
UEnumeration * ucnv_openStandardNames(const char * convName, const char * standard, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_openStandardNames) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_openStandardNames"));

  if (ptr == nullptr) {
    do_fail("ucnv_openStandardNames");
  }

  return ptr(convName, standard, pErrorCode);
}
uint16_t ucnv_countStandards() {
  typedef decltype(&ucnv_countStandards) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_countStandards"));

  if (ptr == nullptr) {
    do_fail("ucnv_countStandards");
  }

  return ptr();
}
const char * ucnv_getStandard(uint16_t n, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_getStandard) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getStandard"));

  if (ptr == nullptr) {
    do_fail("ucnv_getStandard");
  }

  return ptr(n, pErrorCode);
}
const char * ucnv_getStandardName(const char * name, const char * standard, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_getStandardName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getStandardName"));

  if (ptr == nullptr) {
    do_fail("ucnv_getStandardName");
  }

  return ptr(name, standard, pErrorCode);
}
const char * ucnv_getCanonicalName(const char * alias, const char * standard, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_getCanonicalName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getCanonicalName"));

  if (ptr == nullptr) {
    do_fail("ucnv_getCanonicalName");
  }

  return ptr(alias, standard, pErrorCode);
}
const char * ucnv_getDefaultName() {
  typedef decltype(&ucnv_getDefaultName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_getDefaultName"));

  if (ptr == nullptr) {
    do_fail("ucnv_getDefaultName");
  }

  return ptr();
}
void ucnv_setDefaultName(const char * name) {
  typedef decltype(&ucnv_setDefaultName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_setDefaultName"));

  if (ptr == nullptr) {
    do_fail("ucnv_setDefaultName");
  }

  ptr(name);
}
void ucnv_fixFileSeparator(const UConverter * cnv, UChar * source, int32_t sourceLen) {
  typedef decltype(&ucnv_fixFileSeparator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_fixFileSeparator"));

  if (ptr == nullptr) {
    do_fail("ucnv_fixFileSeparator");
  }

  ptr(cnv, source, sourceLen);
}
UBool ucnv_isAmbiguous(const UConverter * cnv) {
  typedef decltype(&ucnv_isAmbiguous) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_isAmbiguous"));

  if (ptr == nullptr) {
    do_fail("ucnv_isAmbiguous");
  }

  return ptr(cnv);
}
void ucnv_setFallback(UConverter * cnv, UBool usesFallback) {
  typedef decltype(&ucnv_setFallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_setFallback"));

  if (ptr == nullptr) {
    do_fail("ucnv_setFallback");
  }

  ptr(cnv, usesFallback);
}
UBool ucnv_usesFallback(const UConverter * cnv) {
  typedef decltype(&ucnv_usesFallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_usesFallback"));

  if (ptr == nullptr) {
    do_fail("ucnv_usesFallback");
  }

  return ptr(cnv);
}
const char * ucnv_detectUnicodeSignature(const char * source, int32_t sourceLength, int32_t * signatureLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucnv_detectUnicodeSignature) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_detectUnicodeSignature"));

  if (ptr == nullptr) {
    do_fail("ucnv_detectUnicodeSignature");
  }

  return ptr(source, sourceLength, signatureLength, pErrorCode);
}
int32_t ucnv_fromUCountPending(const UConverter * cnv, UErrorCode * status) {
  typedef decltype(&ucnv_fromUCountPending) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_fromUCountPending"));

  if (ptr == nullptr) {
    do_fail("ucnv_fromUCountPending");
  }

  return ptr(cnv, status);
}
int32_t ucnv_toUCountPending(const UConverter * cnv, UErrorCode * status) {
  typedef decltype(&ucnv_toUCountPending) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_toUCountPending"));

  if (ptr == nullptr) {
    do_fail("ucnv_toUCountPending");
  }

  return ptr(cnv, status);
}
UBool ucnv_isFixedWidth(UConverter * cnv, UErrorCode * status) {
  typedef decltype(&ucnv_isFixedWidth) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_isFixedWidth"));

  if (ptr == nullptr) {
    do_fail("ucnv_isFixedWidth");
  }

  return ptr(cnv, status);
}
UChar32 uiter_current32(UCharIterator * iter) {
  typedef decltype(&uiter_current32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_current32"));

  if (ptr == nullptr) {
    do_fail("uiter_current32");
  }

  return ptr(iter);
}
UChar32 uiter_next32(UCharIterator * iter) {
  typedef decltype(&uiter_next32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_next32"));

  if (ptr == nullptr) {
    do_fail("uiter_next32");
  }

  return ptr(iter);
}
UChar32 uiter_previous32(UCharIterator * iter) {
  typedef decltype(&uiter_previous32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_previous32"));

  if (ptr == nullptr) {
    do_fail("uiter_previous32");
  }

  return ptr(iter);
}
uint32_t uiter_getState(const UCharIterator * iter) {
  typedef decltype(&uiter_getState) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_getState"));

  if (ptr == nullptr) {
    do_fail("uiter_getState");
  }

  return ptr(iter);
}
void uiter_setState(UCharIterator * iter, uint32_t state, UErrorCode * pErrorCode) {
  typedef decltype(&uiter_setState) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_setState"));

  if (ptr == nullptr) {
    do_fail("uiter_setState");
  }

  ptr(iter, state, pErrorCode);
}
void uiter_setString(UCharIterator * iter, const UChar * s, int32_t length) {
  typedef decltype(&uiter_setString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_setString"));

  if (ptr == nullptr) {
    do_fail("uiter_setString");
  }

  ptr(iter, s, length);
}
void uiter_setUTF16BE(UCharIterator * iter, const char * s, int32_t length) {
  typedef decltype(&uiter_setUTF16BE) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_setUTF16BE"));

  if (ptr == nullptr) {
    do_fail("uiter_setUTF16BE");
  }

  ptr(iter, s, length);
}
void uiter_setUTF8(UCharIterator * iter, const char * s, int32_t length) {
  typedef decltype(&uiter_setUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uiter_setUTF8"));

  if (ptr == nullptr) {
    do_fail("uiter_setUTF8");
  }

  ptr(iter, s, length);
}
UDataMemory * udata_open(const char * path, const char * type, const char * name, UErrorCode * pErrorCode) {
  typedef decltype(&udata_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_open"));

  if (ptr == nullptr) {
    do_fail("udata_open");
  }

  return ptr(path, type, name, pErrorCode);
}
UDataMemory * udata_openChoice(const char * path, const char * type, const char * name, UDataMemoryIsAcceptable * isAcceptable, void * context, UErrorCode * pErrorCode) {
  typedef decltype(&udata_openChoice) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_openChoice"));

  if (ptr == nullptr) {
    do_fail("udata_openChoice");
  }

  return ptr(path, type, name, isAcceptable, context, pErrorCode);
}
void udata_close(UDataMemory * pData) {
  typedef decltype(&udata_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_close"));

  if (ptr == nullptr) {
    do_fail("udata_close");
  }

  ptr(pData);
}
const void * udata_getMemory(UDataMemory * pData) {
  typedef decltype(&udata_getMemory) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_getMemory"));

  if (ptr == nullptr) {
    do_fail("udata_getMemory");
  }

  return ptr(pData);
}
void udata_getInfo(UDataMemory * pData, UDataInfo * pInfo) {
  typedef decltype(&udata_getInfo) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_getInfo"));

  if (ptr == nullptr) {
    do_fail("udata_getInfo");
  }

  ptr(pData, pInfo);
}
void udata_setCommonData(const void * data, UErrorCode * err) {
  typedef decltype(&udata_setCommonData) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_setCommonData"));

  if (ptr == nullptr) {
    do_fail("udata_setCommonData");
  }

  ptr(data, err);
}
void udata_setAppData(const char * packageName, const void * data, UErrorCode * err) {
  typedef decltype(&udata_setAppData) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_setAppData"));

  if (ptr == nullptr) {
    do_fail("udata_setAppData");
  }

  ptr(packageName, data, err);
}
void udata_setFileAccess(UDataFileAccess access, UErrorCode * status) {
  typedef decltype(&udata_setFileAccess) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "udata_setFileAccess"));

  if (ptr == nullptr) {
    do_fail("udata_setFileAccess");
  }

  ptr(access, status);
}
UListFormatter * ulistfmt_open(const char * locale, UErrorCode * status) {
  typedef decltype(&ulistfmt_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ulistfmt_open"));

  if (ptr == nullptr) {
    do_fail("ulistfmt_open");
  }

  return ptr(locale, status);
}
void ulistfmt_close(UListFormatter * listfmt) {
  typedef decltype(&ulistfmt_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ulistfmt_close"));

  if (ptr == nullptr) {
    do_fail("ulistfmt_close");
  }

  ptr(listfmt);
}
int32_t ulistfmt_format(const UListFormatter * listfmt, const UChar *const  strings[], const int32_t * stringLengths, int32_t stringCount, UChar * result, int32_t resultCapacity, UErrorCode * status) {
  typedef decltype(&ulistfmt_format) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ulistfmt_format"));

  if (ptr == nullptr) {
    do_fail("ulistfmt_format");
  }

  return ptr(listfmt, strings, stringLengths, stringCount, result, resultCapacity, status);
}
void u_getDataVersion(UVersionInfo dataVersionFillin, UErrorCode * status) {
  typedef decltype(&u_getDataVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getDataVersion"));

  if (ptr == nullptr) {
    do_fail("u_getDataVersion");
  }

  ptr(dataVersionFillin, status);
}
UBool u_hasBinaryProperty(UChar32 c, UProperty which) {
  typedef decltype(&u_hasBinaryProperty) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_hasBinaryProperty"));

  if (ptr == nullptr) {
    do_fail("u_hasBinaryProperty");
  }

  return ptr(c, which);
}
UBool u_isUAlphabetic(UChar32 c) {
  typedef decltype(&u_isUAlphabetic) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isUAlphabetic"));

  if (ptr == nullptr) {
    do_fail("u_isUAlphabetic");
  }

  return ptr(c);
}
UBool u_isULowercase(UChar32 c) {
  typedef decltype(&u_isULowercase) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isULowercase"));

  if (ptr == nullptr) {
    do_fail("u_isULowercase");
  }

  return ptr(c);
}
UBool u_isUUppercase(UChar32 c) {
  typedef decltype(&u_isUUppercase) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isUUppercase"));

  if (ptr == nullptr) {
    do_fail("u_isUUppercase");
  }

  return ptr(c);
}
UBool u_isUWhiteSpace(UChar32 c) {
  typedef decltype(&u_isUWhiteSpace) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isUWhiteSpace"));

  if (ptr == nullptr) {
    do_fail("u_isUWhiteSpace");
  }

  return ptr(c);
}
int32_t u_getIntPropertyValue(UChar32 c, UProperty which) {
  typedef decltype(&u_getIntPropertyValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getIntPropertyValue"));

  if (ptr == nullptr) {
    do_fail("u_getIntPropertyValue");
  }

  return ptr(c, which);
}
int32_t u_getIntPropertyMinValue(UProperty which) {
  typedef decltype(&u_getIntPropertyMinValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getIntPropertyMinValue"));

  if (ptr == nullptr) {
    do_fail("u_getIntPropertyMinValue");
  }

  return ptr(which);
}
int32_t u_getIntPropertyMaxValue(UProperty which) {
  typedef decltype(&u_getIntPropertyMaxValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getIntPropertyMaxValue"));

  if (ptr == nullptr) {
    do_fail("u_getIntPropertyMaxValue");
  }

  return ptr(which);
}
double u_getNumericValue(UChar32 c) {
  typedef decltype(&u_getNumericValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getNumericValue"));

  if (ptr == nullptr) {
    do_fail("u_getNumericValue");
  }

  return ptr(c);
}
UBool u_islower(UChar32 c) {
  typedef decltype(&u_islower) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_islower"));

  if (ptr == nullptr) {
    do_fail("u_islower");
  }

  return ptr(c);
}
UBool u_isupper(UChar32 c) {
  typedef decltype(&u_isupper) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isupper"));

  if (ptr == nullptr) {
    do_fail("u_isupper");
  }

  return ptr(c);
}
UBool u_istitle(UChar32 c) {
  typedef decltype(&u_istitle) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_istitle"));

  if (ptr == nullptr) {
    do_fail("u_istitle");
  }

  return ptr(c);
}
UBool u_isdigit(UChar32 c) {
  typedef decltype(&u_isdigit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isdigit"));

  if (ptr == nullptr) {
    do_fail("u_isdigit");
  }

  return ptr(c);
}
UBool u_isalpha(UChar32 c) {
  typedef decltype(&u_isalpha) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isalpha"));

  if (ptr == nullptr) {
    do_fail("u_isalpha");
  }

  return ptr(c);
}
UBool u_isalnum(UChar32 c) {
  typedef decltype(&u_isalnum) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isalnum"));

  if (ptr == nullptr) {
    do_fail("u_isalnum");
  }

  return ptr(c);
}
UBool u_isxdigit(UChar32 c) {
  typedef decltype(&u_isxdigit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isxdigit"));

  if (ptr == nullptr) {
    do_fail("u_isxdigit");
  }

  return ptr(c);
}
UBool u_ispunct(UChar32 c) {
  typedef decltype(&u_ispunct) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_ispunct"));

  if (ptr == nullptr) {
    do_fail("u_ispunct");
  }

  return ptr(c);
}
UBool u_isgraph(UChar32 c) {
  typedef decltype(&u_isgraph) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isgraph"));

  if (ptr == nullptr) {
    do_fail("u_isgraph");
  }

  return ptr(c);
}
UBool u_isblank(UChar32 c) {
  typedef decltype(&u_isblank) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isblank"));

  if (ptr == nullptr) {
    do_fail("u_isblank");
  }

  return ptr(c);
}
UBool u_isdefined(UChar32 c) {
  typedef decltype(&u_isdefined) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isdefined"));

  if (ptr == nullptr) {
    do_fail("u_isdefined");
  }

  return ptr(c);
}
UBool u_isspace(UChar32 c) {
  typedef decltype(&u_isspace) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isspace"));

  if (ptr == nullptr) {
    do_fail("u_isspace");
  }

  return ptr(c);
}
UBool u_isJavaSpaceChar(UChar32 c) {
  typedef decltype(&u_isJavaSpaceChar) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isJavaSpaceChar"));

  if (ptr == nullptr) {
    do_fail("u_isJavaSpaceChar");
  }

  return ptr(c);
}
UBool u_isWhitespace(UChar32 c) {
  typedef decltype(&u_isWhitespace) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isWhitespace"));

  if (ptr == nullptr) {
    do_fail("u_isWhitespace");
  }

  return ptr(c);
}
UBool u_iscntrl(UChar32 c) {
  typedef decltype(&u_iscntrl) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_iscntrl"));

  if (ptr == nullptr) {
    do_fail("u_iscntrl");
  }

  return ptr(c);
}
UBool u_isISOControl(UChar32 c) {
  typedef decltype(&u_isISOControl) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isISOControl"));

  if (ptr == nullptr) {
    do_fail("u_isISOControl");
  }

  return ptr(c);
}
UBool u_isprint(UChar32 c) {
  typedef decltype(&u_isprint) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isprint"));

  if (ptr == nullptr) {
    do_fail("u_isprint");
  }

  return ptr(c);
}
UBool u_isbase(UChar32 c) {
  typedef decltype(&u_isbase) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isbase"));

  if (ptr == nullptr) {
    do_fail("u_isbase");
  }

  return ptr(c);
}
UCharDirection u_charDirection(UChar32 c) {
  typedef decltype(&u_charDirection) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charDirection"));

  if (ptr == nullptr) {
    do_fail("u_charDirection");
  }

  return ptr(c);
}
UBool u_isMirrored(UChar32 c) {
  typedef decltype(&u_isMirrored) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isMirrored"));

  if (ptr == nullptr) {
    do_fail("u_isMirrored");
  }

  return ptr(c);
}
UChar32 u_charMirror(UChar32 c) {
  typedef decltype(&u_charMirror) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charMirror"));

  if (ptr == nullptr) {
    do_fail("u_charMirror");
  }

  return ptr(c);
}
UChar32 u_getBidiPairedBracket(UChar32 c) {
  typedef decltype(&u_getBidiPairedBracket) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getBidiPairedBracket"));

  if (ptr == nullptr) {
    do_fail("u_getBidiPairedBracket");
  }

  return ptr(c);
}
int8_t u_charType(UChar32 c) {
  typedef decltype(&u_charType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charType"));

  if (ptr == nullptr) {
    do_fail("u_charType");
  }

  return ptr(c);
}
void u_enumCharTypes(UCharEnumTypeRange * enumRange, const void * context) {
  typedef decltype(&u_enumCharTypes) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_enumCharTypes"));

  if (ptr == nullptr) {
    do_fail("u_enumCharTypes");
  }

  ptr(enumRange, context);
}
uint8_t u_getCombiningClass(UChar32 c) {
  typedef decltype(&u_getCombiningClass) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getCombiningClass"));

  if (ptr == nullptr) {
    do_fail("u_getCombiningClass");
  }

  return ptr(c);
}
int32_t u_charDigitValue(UChar32 c) {
  typedef decltype(&u_charDigitValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charDigitValue"));

  if (ptr == nullptr) {
    do_fail("u_charDigitValue");
  }

  return ptr(c);
}
UBlockCode ublock_getCode(UChar32 c) {
  typedef decltype(&ublock_getCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ublock_getCode"));

  if (ptr == nullptr) {
    do_fail("ublock_getCode");
  }

  return ptr(c);
}
int32_t u_charName(UChar32 code, UCharNameChoice nameChoice, char * buffer, int32_t bufferLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_charName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charName"));

  if (ptr == nullptr) {
    do_fail("u_charName");
  }

  return ptr(code, nameChoice, buffer, bufferLength, pErrorCode);
}
UChar32 u_charFromName(UCharNameChoice nameChoice, const char * name, UErrorCode * pErrorCode) {
  typedef decltype(&u_charFromName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charFromName"));

  if (ptr == nullptr) {
    do_fail("u_charFromName");
  }

  return ptr(nameChoice, name, pErrorCode);
}
void u_enumCharNames(UChar32 start, UChar32 limit, UEnumCharNamesFn * fn, void * context, UCharNameChoice nameChoice, UErrorCode * pErrorCode) {
  typedef decltype(&u_enumCharNames) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_enumCharNames"));

  if (ptr == nullptr) {
    do_fail("u_enumCharNames");
  }

  ptr(start, limit, fn, context, nameChoice, pErrorCode);
}
const char * u_getPropertyName(UProperty property, UPropertyNameChoice nameChoice) {
  typedef decltype(&u_getPropertyName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getPropertyName"));

  if (ptr == nullptr) {
    do_fail("u_getPropertyName");
  }

  return ptr(property, nameChoice);
}
UProperty u_getPropertyEnum(const char * alias) {
  typedef decltype(&u_getPropertyEnum) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getPropertyEnum"));

  if (ptr == nullptr) {
    do_fail("u_getPropertyEnum");
  }

  return ptr(alias);
}
const char * u_getPropertyValueName(UProperty property, int32_t value, UPropertyNameChoice nameChoice) {
  typedef decltype(&u_getPropertyValueName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getPropertyValueName"));

  if (ptr == nullptr) {
    do_fail("u_getPropertyValueName");
  }

  return ptr(property, value, nameChoice);
}
int32_t u_getPropertyValueEnum(UProperty property, const char * alias) {
  typedef decltype(&u_getPropertyValueEnum) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getPropertyValueEnum"));

  if (ptr == nullptr) {
    do_fail("u_getPropertyValueEnum");
  }

  return ptr(property, alias);
}
UBool u_isIDStart(UChar32 c) {
  typedef decltype(&u_isIDStart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isIDStart"));

  if (ptr == nullptr) {
    do_fail("u_isIDStart");
  }

  return ptr(c);
}
UBool u_isIDPart(UChar32 c) {
  typedef decltype(&u_isIDPart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isIDPart"));

  if (ptr == nullptr) {
    do_fail("u_isIDPart");
  }

  return ptr(c);
}
UBool u_isIDIgnorable(UChar32 c) {
  typedef decltype(&u_isIDIgnorable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isIDIgnorable"));

  if (ptr == nullptr) {
    do_fail("u_isIDIgnorable");
  }

  return ptr(c);
}
UBool u_isJavaIDStart(UChar32 c) {
  typedef decltype(&u_isJavaIDStart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isJavaIDStart"));

  if (ptr == nullptr) {
    do_fail("u_isJavaIDStart");
  }

  return ptr(c);
}
UBool u_isJavaIDPart(UChar32 c) {
  typedef decltype(&u_isJavaIDPart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_isJavaIDPart"));

  if (ptr == nullptr) {
    do_fail("u_isJavaIDPart");
  }

  return ptr(c);
}
UChar32 u_tolower(UChar32 c) {
  typedef decltype(&u_tolower) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_tolower"));

  if (ptr == nullptr) {
    do_fail("u_tolower");
  }

  return ptr(c);
}
UChar32 u_toupper(UChar32 c) {
  typedef decltype(&u_toupper) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_toupper"));

  if (ptr == nullptr) {
    do_fail("u_toupper");
  }

  return ptr(c);
}
UChar32 u_totitle(UChar32 c) {
  typedef decltype(&u_totitle) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_totitle"));

  if (ptr == nullptr) {
    do_fail("u_totitle");
  }

  return ptr(c);
}
UChar32 u_foldCase(UChar32 c, uint32_t options) {
  typedef decltype(&u_foldCase) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_foldCase"));

  if (ptr == nullptr) {
    do_fail("u_foldCase");
  }

  return ptr(c, options);
}
int32_t u_digit(UChar32 ch, int8_t radix) {
  typedef decltype(&u_digit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_digit"));

  if (ptr == nullptr) {
    do_fail("u_digit");
  }

  return ptr(ch, radix);
}
UChar32 u_forDigit(int32_t digit, int8_t radix) {
  typedef decltype(&u_forDigit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_forDigit"));

  if (ptr == nullptr) {
    do_fail("u_forDigit");
  }

  return ptr(digit, radix);
}
void u_charAge(UChar32 c, UVersionInfo versionArray) {
  typedef decltype(&u_charAge) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charAge"));

  if (ptr == nullptr) {
    do_fail("u_charAge");
  }

  ptr(c, versionArray);
}
void u_getUnicodeVersion(UVersionInfo versionArray) {
  typedef decltype(&u_getUnicodeVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getUnicodeVersion"));

  if (ptr == nullptr) {
    do_fail("u_getUnicodeVersion");
  }

  ptr(versionArray);
}
int32_t u_getFC_NFKC_Closure(UChar32 c, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  typedef decltype(&u_getFC_NFKC_Closure) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getFC_NFKC_Closure"));

  if (ptr == nullptr) {
    do_fail("u_getFC_NFKC_Closure");
  }

  return ptr(c, dest, destCapacity, pErrorCode);
}
const char * u_errorName(UErrorCode code) {
  typedef decltype(&u_errorName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_errorName"));

  if (ptr == nullptr) {
    do_fail("u_errorName");
  }

  return ptr(code);
}
const char * uloc_getDefault() {
  typedef decltype(&uloc_getDefault) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDefault"));

  if (ptr == nullptr) {
    do_fail("uloc_getDefault");
  }

  return ptr();
}
void uloc_setDefault(const char * localeID, UErrorCode * status) {
  typedef decltype(&uloc_setDefault) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_setDefault"));

  if (ptr == nullptr) {
    do_fail("uloc_setDefault");
  }

  ptr(localeID, status);
}
int32_t uloc_getLanguage(const char * localeID, char * language, int32_t languageCapacity, UErrorCode * err) {
  typedef decltype(&uloc_getLanguage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getLanguage"));

  if (ptr == nullptr) {
    do_fail("uloc_getLanguage");
  }

  return ptr(localeID, language, languageCapacity, err);
}
int32_t uloc_getScript(const char * localeID, char * script, int32_t scriptCapacity, UErrorCode * err) {
  typedef decltype(&uloc_getScript) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getScript"));

  if (ptr == nullptr) {
    do_fail("uloc_getScript");
  }

  return ptr(localeID, script, scriptCapacity, err);
}
int32_t uloc_getCountry(const char * localeID, char * country, int32_t countryCapacity, UErrorCode * err) {
  typedef decltype(&uloc_getCountry) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getCountry"));

  if (ptr == nullptr) {
    do_fail("uloc_getCountry");
  }

  return ptr(localeID, country, countryCapacity, err);
}
int32_t uloc_getVariant(const char * localeID, char * variant, int32_t variantCapacity, UErrorCode * err) {
  typedef decltype(&uloc_getVariant) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getVariant"));

  if (ptr == nullptr) {
    do_fail("uloc_getVariant");
  }

  return ptr(localeID, variant, variantCapacity, err);
}
int32_t uloc_getName(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err) {
  typedef decltype(&uloc_getName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getName"));

  if (ptr == nullptr) {
    do_fail("uloc_getName");
  }

  return ptr(localeID, name, nameCapacity, err);
}
int32_t uloc_canonicalize(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err) {
  typedef decltype(&uloc_canonicalize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_canonicalize"));

  if (ptr == nullptr) {
    do_fail("uloc_canonicalize");
  }

  return ptr(localeID, name, nameCapacity, err);
}
const char * uloc_getISO3Language(const char * localeID) {
  typedef decltype(&uloc_getISO3Language) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getISO3Language"));

  if (ptr == nullptr) {
    do_fail("uloc_getISO3Language");
  }

  return ptr(localeID);
}
const char * uloc_getISO3Country(const char * localeID) {
  typedef decltype(&uloc_getISO3Country) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getISO3Country"));

  if (ptr == nullptr) {
    do_fail("uloc_getISO3Country");
  }

  return ptr(localeID);
}
uint32_t uloc_getLCID(const char * localeID) {
  typedef decltype(&uloc_getLCID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getLCID"));

  if (ptr == nullptr) {
    do_fail("uloc_getLCID");
  }

  return ptr(localeID);
}
int32_t uloc_getDisplayLanguage(const char * locale, const char * displayLocale, UChar * language, int32_t languageCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getDisplayLanguage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDisplayLanguage"));

  if (ptr == nullptr) {
    do_fail("uloc_getDisplayLanguage");
  }

  return ptr(locale, displayLocale, language, languageCapacity, status);
}
int32_t uloc_getDisplayScript(const char * locale, const char * displayLocale, UChar * script, int32_t scriptCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getDisplayScript) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDisplayScript"));

  if (ptr == nullptr) {
    do_fail("uloc_getDisplayScript");
  }

  return ptr(locale, displayLocale, script, scriptCapacity, status);
}
int32_t uloc_getDisplayCountry(const char * locale, const char * displayLocale, UChar * country, int32_t countryCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getDisplayCountry) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDisplayCountry"));

  if (ptr == nullptr) {
    do_fail("uloc_getDisplayCountry");
  }

  return ptr(locale, displayLocale, country, countryCapacity, status);
}
int32_t uloc_getDisplayVariant(const char * locale, const char * displayLocale, UChar * variant, int32_t variantCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getDisplayVariant) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDisplayVariant"));

  if (ptr == nullptr) {
    do_fail("uloc_getDisplayVariant");
  }

  return ptr(locale, displayLocale, variant, variantCapacity, status);
}
int32_t uloc_getDisplayKeyword(const char * keyword, const char * displayLocale, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getDisplayKeyword) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDisplayKeyword"));

  if (ptr == nullptr) {
    do_fail("uloc_getDisplayKeyword");
  }

  return ptr(keyword, displayLocale, dest, destCapacity, status);
}
int32_t uloc_getDisplayKeywordValue(const char * locale, const char * keyword, const char * displayLocale, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getDisplayKeywordValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDisplayKeywordValue"));

  if (ptr == nullptr) {
    do_fail("uloc_getDisplayKeywordValue");
  }

  return ptr(locale, keyword, displayLocale, dest, destCapacity, status);
}
int32_t uloc_getDisplayName(const char * localeID, const char * inLocaleID, UChar * result, int32_t maxResultSize, UErrorCode * err) {
  typedef decltype(&uloc_getDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getDisplayName"));

  if (ptr == nullptr) {
    do_fail("uloc_getDisplayName");
  }

  return ptr(localeID, inLocaleID, result, maxResultSize, err);
}
const char * uloc_getAvailable(int32_t n) {
  typedef decltype(&uloc_getAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getAvailable"));

  if (ptr == nullptr) {
    do_fail("uloc_getAvailable");
  }

  return ptr(n);
}
int32_t uloc_countAvailable() {
  typedef decltype(&uloc_countAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_countAvailable"));

  if (ptr == nullptr) {
    do_fail("uloc_countAvailable");
  }

  return ptr();
}
const char *const * uloc_getISOLanguages() {
  typedef decltype(&uloc_getISOLanguages) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getISOLanguages"));

  if (ptr == nullptr) {
    do_fail("uloc_getISOLanguages");
  }

  return ptr();
}
const char *const * uloc_getISOCountries() {
  typedef decltype(&uloc_getISOCountries) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getISOCountries"));

  if (ptr == nullptr) {
    do_fail("uloc_getISOCountries");
  }

  return ptr();
}
int32_t uloc_getParent(const char * localeID, char * parent, int32_t parentCapacity, UErrorCode * err) {
  typedef decltype(&uloc_getParent) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getParent"));

  if (ptr == nullptr) {
    do_fail("uloc_getParent");
  }

  return ptr(localeID, parent, parentCapacity, err);
}
int32_t uloc_getBaseName(const char * localeID, char * name, int32_t nameCapacity, UErrorCode * err) {
  typedef decltype(&uloc_getBaseName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getBaseName"));

  if (ptr == nullptr) {
    do_fail("uloc_getBaseName");
  }

  return ptr(localeID, name, nameCapacity, err);
}
UEnumeration * uloc_openKeywords(const char * localeID, UErrorCode * status) {
  typedef decltype(&uloc_openKeywords) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_openKeywords"));

  if (ptr == nullptr) {
    do_fail("uloc_openKeywords");
  }

  return ptr(localeID, status);
}
int32_t uloc_getKeywordValue(const char * localeID, const char * keywordName, char * buffer, int32_t bufferCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getKeywordValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getKeywordValue"));

  if (ptr == nullptr) {
    do_fail("uloc_getKeywordValue");
  }

  return ptr(localeID, keywordName, buffer, bufferCapacity, status);
}
int32_t uloc_setKeywordValue(const char * keywordName, const char * keywordValue, char * buffer, int32_t bufferCapacity, UErrorCode * status) {
  typedef decltype(&uloc_setKeywordValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_setKeywordValue"));

  if (ptr == nullptr) {
    do_fail("uloc_setKeywordValue");
  }

  return ptr(keywordName, keywordValue, buffer, bufferCapacity, status);
}
UBool uloc_isRightToLeft(const char * locale) {
  typedef decltype(&uloc_isRightToLeft) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_isRightToLeft"));

  if (ptr == nullptr) {
    do_fail("uloc_isRightToLeft");
  }

  return ptr(locale);
}
ULayoutType uloc_getCharacterOrientation(const char * localeId, UErrorCode * status) {
  typedef decltype(&uloc_getCharacterOrientation) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getCharacterOrientation"));

  if (ptr == nullptr) {
    do_fail("uloc_getCharacterOrientation");
  }

  return ptr(localeId, status);
}
ULayoutType uloc_getLineOrientation(const char * localeId, UErrorCode * status) {
  typedef decltype(&uloc_getLineOrientation) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getLineOrientation"));

  if (ptr == nullptr) {
    do_fail("uloc_getLineOrientation");
  }

  return ptr(localeId, status);
}
int32_t uloc_acceptLanguageFromHTTP(char * result, int32_t resultAvailable, UAcceptResult * outResult, const char * httpAcceptLanguage, UEnumeration * availableLocales, UErrorCode * status) {
  typedef decltype(&uloc_acceptLanguageFromHTTP) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_acceptLanguageFromHTTP"));

  if (ptr == nullptr) {
    do_fail("uloc_acceptLanguageFromHTTP");
  }

  return ptr(result, resultAvailable, outResult, httpAcceptLanguage, availableLocales, status);
}
int32_t uloc_acceptLanguage(char * result, int32_t resultAvailable, UAcceptResult * outResult, const char ** acceptList, int32_t acceptListCount, UEnumeration * availableLocales, UErrorCode * status) {
  typedef decltype(&uloc_acceptLanguage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_acceptLanguage"));

  if (ptr == nullptr) {
    do_fail("uloc_acceptLanguage");
  }

  return ptr(result, resultAvailable, outResult, acceptList, acceptListCount, availableLocales, status);
}
int32_t uloc_getLocaleForLCID(uint32_t hostID, char * locale, int32_t localeCapacity, UErrorCode * status) {
  typedef decltype(&uloc_getLocaleForLCID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_getLocaleForLCID"));

  if (ptr == nullptr) {
    do_fail("uloc_getLocaleForLCID");
  }

  return ptr(hostID, locale, localeCapacity, status);
}
int32_t uloc_addLikelySubtags(const char * localeID, char * maximizedLocaleID, int32_t maximizedLocaleIDCapacity, UErrorCode * err) {
  typedef decltype(&uloc_addLikelySubtags) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_addLikelySubtags"));

  if (ptr == nullptr) {
    do_fail("uloc_addLikelySubtags");
  }

  return ptr(localeID, maximizedLocaleID, maximizedLocaleIDCapacity, err);
}
int32_t uloc_minimizeSubtags(const char * localeID, char * minimizedLocaleID, int32_t minimizedLocaleIDCapacity, UErrorCode * err) {
  typedef decltype(&uloc_minimizeSubtags) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_minimizeSubtags"));

  if (ptr == nullptr) {
    do_fail("uloc_minimizeSubtags");
  }

  return ptr(localeID, minimizedLocaleID, minimizedLocaleIDCapacity, err);
}
int32_t uloc_forLanguageTag(const char * langtag, char * localeID, int32_t localeIDCapacity, int32_t * parsedLength, UErrorCode * err) {
  typedef decltype(&uloc_forLanguageTag) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_forLanguageTag"));

  if (ptr == nullptr) {
    do_fail("uloc_forLanguageTag");
  }

  return ptr(langtag, localeID, localeIDCapacity, parsedLength, err);
}
int32_t uloc_toLanguageTag(const char * localeID, char * langtag, int32_t langtagCapacity, UBool strict, UErrorCode * err) {
  typedef decltype(&uloc_toLanguageTag) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_toLanguageTag"));

  if (ptr == nullptr) {
    do_fail("uloc_toLanguageTag");
  }

  return ptr(localeID, langtag, langtagCapacity, strict, err);
}
const char * uloc_toUnicodeLocaleKey(const char * keyword) {
  typedef decltype(&uloc_toUnicodeLocaleKey) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_toUnicodeLocaleKey"));

  if (ptr == nullptr) {
    do_fail("uloc_toUnicodeLocaleKey");
  }

  return ptr(keyword);
}
const char * uloc_toUnicodeLocaleType(const char * keyword, const char * value) {
  typedef decltype(&uloc_toUnicodeLocaleType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_toUnicodeLocaleType"));

  if (ptr == nullptr) {
    do_fail("uloc_toUnicodeLocaleType");
  }

  return ptr(keyword, value);
}
const char * uloc_toLegacyKey(const char * keyword) {
  typedef decltype(&uloc_toLegacyKey) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_toLegacyKey"));

  if (ptr == nullptr) {
    do_fail("uloc_toLegacyKey");
  }

  return ptr(keyword);
}
const char * uloc_toLegacyType(const char * keyword, const char * value) {
  typedef decltype(&uloc_toLegacyType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uloc_toLegacyType"));

  if (ptr == nullptr) {
    do_fail("uloc_toLegacyType");
  }

  return ptr(keyword, value);
}
UCaseMap * ucasemap_open(const char * locale, uint32_t options, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_open"));

  if (ptr == nullptr) {
    do_fail("ucasemap_open");
  }

  return ptr(locale, options, pErrorCode);
}
void ucasemap_close(UCaseMap * csm) {
  typedef decltype(&ucasemap_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_close"));

  if (ptr == nullptr) {
    do_fail("ucasemap_close");
  }

  ptr(csm);
}
const char * ucasemap_getLocale(const UCaseMap * csm) {
  typedef decltype(&ucasemap_getLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_getLocale"));

  if (ptr == nullptr) {
    do_fail("ucasemap_getLocale");
  }

  return ptr(csm);
}
uint32_t ucasemap_getOptions(const UCaseMap * csm) {
  typedef decltype(&ucasemap_getOptions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_getOptions"));

  if (ptr == nullptr) {
    do_fail("ucasemap_getOptions");
  }

  return ptr(csm);
}
void ucasemap_setLocale(UCaseMap * csm, const char * locale, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_setLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_setLocale"));

  if (ptr == nullptr) {
    do_fail("ucasemap_setLocale");
  }

  ptr(csm, locale, pErrorCode);
}
void ucasemap_setOptions(UCaseMap * csm, uint32_t options, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_setOptions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_setOptions"));

  if (ptr == nullptr) {
    do_fail("ucasemap_setOptions");
  }

  ptr(csm, options, pErrorCode);
}
const UBreakIterator * ucasemap_getBreakIterator(const UCaseMap * csm) {
  typedef decltype(&ucasemap_getBreakIterator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_getBreakIterator"));

  if (ptr == nullptr) {
    do_fail("ucasemap_getBreakIterator");
  }

  return ptr(csm);
}
void ucasemap_setBreakIterator(UCaseMap * csm, UBreakIterator * iterToAdopt, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_setBreakIterator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_setBreakIterator"));

  if (ptr == nullptr) {
    do_fail("ucasemap_setBreakIterator");
  }

  ptr(csm, iterToAdopt, pErrorCode);
}
int32_t ucasemap_toTitle(UCaseMap * csm, UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_toTitle) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_toTitle"));

  if (ptr == nullptr) {
    do_fail("ucasemap_toTitle");
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToLower(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_utf8ToLower) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_utf8ToLower"));

  if (ptr == nullptr) {
    do_fail("ucasemap_utf8ToLower");
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToUpper(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_utf8ToUpper) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_utf8ToUpper"));

  if (ptr == nullptr) {
    do_fail("ucasemap_utf8ToUpper");
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8ToTitle(UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_utf8ToTitle) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_utf8ToTitle"));

  if (ptr == nullptr) {
    do_fail("ucasemap_utf8ToTitle");
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
int32_t ucasemap_utf8FoldCase(const UCaseMap * csm, char * dest, int32_t destCapacity, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucasemap_utf8FoldCase) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucasemap_utf8FoldCase"));

  if (ptr == nullptr) {
    do_fail("ucasemap_utf8FoldCase");
  }

  return ptr(csm, dest, destCapacity, src, srcLength, pErrorCode);
}
const char * u_getDataDirectory() {
  typedef decltype(&u_getDataDirectory) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getDataDirectory"));

  if (ptr == nullptr) {
    do_fail("u_getDataDirectory");
  }

  return ptr();
}
void u_setDataDirectory(const char * directory) {
  typedef decltype(&u_setDataDirectory) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_setDataDirectory"));

  if (ptr == nullptr) {
    do_fail("u_setDataDirectory");
  }

  ptr(directory);
}
void u_charsToUChars(const char * cs, UChar * us, int32_t length) {
  typedef decltype(&u_charsToUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_charsToUChars"));

  if (ptr == nullptr) {
    do_fail("u_charsToUChars");
  }

  ptr(cs, us, length);
}
void u_UCharsToChars(const UChar * us, char * cs, int32_t length) {
  typedef decltype(&u_UCharsToChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_UCharsToChars"));

  if (ptr == nullptr) {
    do_fail("u_UCharsToChars");
  }

  ptr(us, cs, length);
}
int32_t uscript_getCode(const char * nameOrAbbrOrLocale, UScriptCode * fillIn, int32_t capacity, UErrorCode * err) {
  typedef decltype(&uscript_getCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_getCode"));

  if (ptr == nullptr) {
    do_fail("uscript_getCode");
  }

  return ptr(nameOrAbbrOrLocale, fillIn, capacity, err);
}
const char * uscript_getName(UScriptCode scriptCode) {
  typedef decltype(&uscript_getName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_getName"));

  if (ptr == nullptr) {
    do_fail("uscript_getName");
  }

  return ptr(scriptCode);
}
const char * uscript_getShortName(UScriptCode scriptCode) {
  typedef decltype(&uscript_getShortName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_getShortName"));

  if (ptr == nullptr) {
    do_fail("uscript_getShortName");
  }

  return ptr(scriptCode);
}
UScriptCode uscript_getScript(UChar32 codepoint, UErrorCode * err) {
  typedef decltype(&uscript_getScript) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_getScript"));

  if (ptr == nullptr) {
    do_fail("uscript_getScript");
  }

  return ptr(codepoint, err);
}
UBool uscript_hasScript(UChar32 c, UScriptCode sc) {
  typedef decltype(&uscript_hasScript) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_hasScript"));

  if (ptr == nullptr) {
    do_fail("uscript_hasScript");
  }

  return ptr(c, sc);
}
int32_t uscript_getScriptExtensions(UChar32 c, UScriptCode * scripts, int32_t capacity, UErrorCode * errorCode) {
  typedef decltype(&uscript_getScriptExtensions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_getScriptExtensions"));

  if (ptr == nullptr) {
    do_fail("uscript_getScriptExtensions");
  }

  return ptr(c, scripts, capacity, errorCode);
}
int32_t uscript_getSampleString(UScriptCode script, UChar * dest, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&uscript_getSampleString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_getSampleString"));

  if (ptr == nullptr) {
    do_fail("uscript_getSampleString");
  }

  return ptr(script, dest, capacity, pErrorCode);
}
UScriptUsage uscript_getUsage(UScriptCode script) {
  typedef decltype(&uscript_getUsage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_getUsage"));

  if (ptr == nullptr) {
    do_fail("uscript_getUsage");
  }

  return ptr(script);
}
UBool uscript_isRightToLeft(UScriptCode script) {
  typedef decltype(&uscript_isRightToLeft) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_isRightToLeft"));

  if (ptr == nullptr) {
    do_fail("uscript_isRightToLeft");
  }

  return ptr(script);
}
UBool uscript_breaksBetweenLetters(UScriptCode script) {
  typedef decltype(&uscript_breaksBetweenLetters) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_breaksBetweenLetters"));

  if (ptr == nullptr) {
    do_fail("uscript_breaksBetweenLetters");
  }

  return ptr(script);
}
UBool uscript_isCased(UScriptCode script) {
  typedef decltype(&uscript_isCased) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uscript_isCased"));

  if (ptr == nullptr) {
    do_fail("uscript_isCased");
  }

  return ptr(script);
}
ULocaleDisplayNames * uldn_open(const char * locale, UDialectHandling dialectHandling, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_open"));

  if (ptr == nullptr) {
    do_fail("uldn_open");
  }

  return ptr(locale, dialectHandling, pErrorCode);
}
void uldn_close(ULocaleDisplayNames * ldn) {
  typedef decltype(&uldn_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_close"));

  if (ptr == nullptr) {
    do_fail("uldn_close");
  }

  ptr(ldn);
}
const char * uldn_getLocale(const ULocaleDisplayNames * ldn) {
  typedef decltype(&uldn_getLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_getLocale"));

  if (ptr == nullptr) {
    do_fail("uldn_getLocale");
  }

  return ptr(ldn);
}
UDialectHandling uldn_getDialectHandling(const ULocaleDisplayNames * ldn) {
  typedef decltype(&uldn_getDialectHandling) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_getDialectHandling"));

  if (ptr == nullptr) {
    do_fail("uldn_getDialectHandling");
  }

  return ptr(ldn);
}
int32_t uldn_localeDisplayName(const ULocaleDisplayNames * ldn, const char * locale, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_localeDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_localeDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_localeDisplayName");
  }

  return ptr(ldn, locale, result, maxResultSize, pErrorCode);
}
int32_t uldn_languageDisplayName(const ULocaleDisplayNames * ldn, const char * lang, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_languageDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_languageDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_languageDisplayName");
  }

  return ptr(ldn, lang, result, maxResultSize, pErrorCode);
}
int32_t uldn_scriptDisplayName(const ULocaleDisplayNames * ldn, const char * script, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_scriptDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_scriptDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_scriptDisplayName");
  }

  return ptr(ldn, script, result, maxResultSize, pErrorCode);
}
int32_t uldn_scriptCodeDisplayName(const ULocaleDisplayNames * ldn, UScriptCode scriptCode, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_scriptCodeDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_scriptCodeDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_scriptCodeDisplayName");
  }

  return ptr(ldn, scriptCode, result, maxResultSize, pErrorCode);
}
int32_t uldn_regionDisplayName(const ULocaleDisplayNames * ldn, const char * region, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_regionDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_regionDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_regionDisplayName");
  }

  return ptr(ldn, region, result, maxResultSize, pErrorCode);
}
int32_t uldn_variantDisplayName(const ULocaleDisplayNames * ldn, const char * variant, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_variantDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_variantDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_variantDisplayName");
  }

  return ptr(ldn, variant, result, maxResultSize, pErrorCode);
}
int32_t uldn_keyDisplayName(const ULocaleDisplayNames * ldn, const char * key, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_keyDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_keyDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_keyDisplayName");
  }

  return ptr(ldn, key, result, maxResultSize, pErrorCode);
}
int32_t uldn_keyValueDisplayName(const ULocaleDisplayNames * ldn, const char * key, const char * value, UChar * result, int32_t maxResultSize, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_keyValueDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_keyValueDisplayName"));

  if (ptr == nullptr) {
    do_fail("uldn_keyValueDisplayName");
  }

  return ptr(ldn, key, value, result, maxResultSize, pErrorCode);
}
ULocaleDisplayNames * uldn_openForContext(const char * locale, UDisplayContext * contexts, int32_t length, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_openForContext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_openForContext"));

  if (ptr == nullptr) {
    do_fail("uldn_openForContext");
  }

  return ptr(locale, contexts, length, pErrorCode);
}
UDisplayContext uldn_getContext(const ULocaleDisplayNames * ldn, UDisplayContextType type, UErrorCode * pErrorCode) {
  typedef decltype(&uldn_getContext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uldn_getContext"));

  if (ptr == nullptr) {
    do_fail("uldn_getContext");
  }

  return ptr(ldn, type, pErrorCode);
}
u_nl_catd u_catopen(const char * name, const char * locale, UErrorCode * ec) {
  typedef decltype(&u_catopen) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_catopen"));

  if (ptr == nullptr) {
    do_fail("u_catopen");
  }

  return ptr(name, locale, ec);
}
void u_catclose(u_nl_catd catd) {
  typedef decltype(&u_catclose) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_catclose"));

  if (ptr == nullptr) {
    do_fail("u_catclose");
  }

  ptr(catd);
}
const UChar * u_catgets(u_nl_catd catd, int32_t set_num, int32_t msg_num, const UChar * s, int32_t * len, UErrorCode * ec) {
  typedef decltype(&u_catgets) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_catgets"));

  if (ptr == nullptr) {
    do_fail("u_catgets");
  }

  return ptr(catd, set_num, msg_num, s, len, ec);
}
void ucnv_cbFromUWriteBytes(UConverterFromUnicodeArgs * args, const char * source, int32_t length, int32_t offsetIndex, UErrorCode * err) {
  typedef decltype(&ucnv_cbFromUWriteBytes) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_cbFromUWriteBytes"));

  if (ptr == nullptr) {
    do_fail("ucnv_cbFromUWriteBytes");
  }

  ptr(args, source, length, offsetIndex, err);
}
void ucnv_cbFromUWriteSub(UConverterFromUnicodeArgs * args, int32_t offsetIndex, UErrorCode * err) {
  typedef decltype(&ucnv_cbFromUWriteSub) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_cbFromUWriteSub"));

  if (ptr == nullptr) {
    do_fail("ucnv_cbFromUWriteSub");
  }

  ptr(args, offsetIndex, err);
}
void ucnv_cbFromUWriteUChars(UConverterFromUnicodeArgs * args, const UChar ** source, const UChar * sourceLimit, int32_t offsetIndex, UErrorCode * err) {
  typedef decltype(&ucnv_cbFromUWriteUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_cbFromUWriteUChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_cbFromUWriteUChars");
  }

  ptr(args, source, sourceLimit, offsetIndex, err);
}
void ucnv_cbToUWriteUChars(UConverterToUnicodeArgs * args, const UChar * source, int32_t length, int32_t offsetIndex, UErrorCode * err) {
  typedef decltype(&ucnv_cbToUWriteUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_cbToUWriteUChars"));

  if (ptr == nullptr) {
    do_fail("ucnv_cbToUWriteUChars");
  }

  ptr(args, source, length, offsetIndex, err);
}
void ucnv_cbToUWriteSub(UConverterToUnicodeArgs * args, int32_t offsetIndex, UErrorCode * err) {
  typedef decltype(&ucnv_cbToUWriteSub) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnv_cbToUWriteSub"));

  if (ptr == nullptr) {
    do_fail("ucnv_cbToUWriteSub");
  }

  ptr(args, offsetIndex, err);
}
void UCNV_FROM_U_CALLBACK_STOP(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_STOP) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_FROM_U_CALLBACK_STOP"));

  if (ptr == nullptr) {
    do_fail("UCNV_FROM_U_CALLBACK_STOP");
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_TO_U_CALLBACK_STOP(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_STOP) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_TO_U_CALLBACK_STOP"));

  if (ptr == nullptr) {
    do_fail("UCNV_TO_U_CALLBACK_STOP");
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_FROM_U_CALLBACK_SKIP(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_SKIP) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_FROM_U_CALLBACK_SKIP"));

  if (ptr == nullptr) {
    do_fail("UCNV_FROM_U_CALLBACK_SKIP");
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_FROM_U_CALLBACK_SUBSTITUTE(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_SUBSTITUTE) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_FROM_U_CALLBACK_SUBSTITUTE"));

  if (ptr == nullptr) {
    do_fail("UCNV_FROM_U_CALLBACK_SUBSTITUTE");
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_FROM_U_CALLBACK_ESCAPE(const void * context, UConverterFromUnicodeArgs * fromUArgs, const UChar * codeUnits, int32_t length, UChar32 codePoint, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_FROM_U_CALLBACK_ESCAPE) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_FROM_U_CALLBACK_ESCAPE"));

  if (ptr == nullptr) {
    do_fail("UCNV_FROM_U_CALLBACK_ESCAPE");
  }

  ptr(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}
void UCNV_TO_U_CALLBACK_SKIP(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_SKIP) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_TO_U_CALLBACK_SKIP"));

  if (ptr == nullptr) {
    do_fail("UCNV_TO_U_CALLBACK_SKIP");
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_TO_U_CALLBACK_SUBSTITUTE(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_SUBSTITUTE) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_TO_U_CALLBACK_SUBSTITUTE"));

  if (ptr == nullptr) {
    do_fail("UCNV_TO_U_CALLBACK_SUBSTITUTE");
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
void UCNV_TO_U_CALLBACK_ESCAPE(const void * context, UConverterToUnicodeArgs * toUArgs, const char * codeUnits, int32_t length, UConverterCallbackReason reason, UErrorCode * err) {
  typedef decltype(&UCNV_TO_U_CALLBACK_ESCAPE) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "UCNV_TO_U_CALLBACK_ESCAPE"));

  if (ptr == nullptr) {
    do_fail("UCNV_TO_U_CALLBACK_ESCAPE");
  }

  ptr(context, toUArgs, codeUnits, length, reason, err);
}
int32_t u_shapeArabic(const UChar * source, int32_t sourceLength, UChar * dest, int32_t destSize, uint32_t options, UErrorCode * pErrorCode) {
  typedef decltype(&u_shapeArabic) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_shapeArabic"));

  if (ptr == nullptr) {
    do_fail("u_shapeArabic");
  }

  return ptr(source, sourceLength, dest, destSize, options, pErrorCode);
}
UConverterSelector * ucnvsel_open(const char *const * converterList, int32_t converterListSize, const USet * excludedCodePoints, const UConverterUnicodeSet whichSet, UErrorCode * status) {
  typedef decltype(&ucnvsel_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnvsel_open"));

  if (ptr == nullptr) {
    do_fail("ucnvsel_open");
  }

  return ptr(converterList, converterListSize, excludedCodePoints, whichSet, status);
}
void ucnvsel_close(UConverterSelector * sel) {
  typedef decltype(&ucnvsel_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnvsel_close"));

  if (ptr == nullptr) {
    do_fail("ucnvsel_close");
  }

  ptr(sel);
}
UConverterSelector * ucnvsel_openFromSerialized(const void * buffer, int32_t length, UErrorCode * status) {
  typedef decltype(&ucnvsel_openFromSerialized) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnvsel_openFromSerialized"));

  if (ptr == nullptr) {
    do_fail("ucnvsel_openFromSerialized");
  }

  return ptr(buffer, length, status);
}
int32_t ucnvsel_serialize(const UConverterSelector * sel, void * buffer, int32_t bufferCapacity, UErrorCode * status) {
  typedef decltype(&ucnvsel_serialize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnvsel_serialize"));

  if (ptr == nullptr) {
    do_fail("ucnvsel_serialize");
  }

  return ptr(sel, buffer, bufferCapacity, status);
}
UEnumeration * ucnvsel_selectForString(const UConverterSelector * sel, const UChar * s, int32_t length, UErrorCode * status) {
  typedef decltype(&ucnvsel_selectForString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnvsel_selectForString"));

  if (ptr == nullptr) {
    do_fail("ucnvsel_selectForString");
  }

  return ptr(sel, s, length, status);
}
UEnumeration * ucnvsel_selectForUTF8(const UConverterSelector * sel, const char * s, int32_t length, UErrorCode * status) {
  typedef decltype(&ucnvsel_selectForUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucnvsel_selectForUTF8"));

  if (ptr == nullptr) {
    do_fail("ucnvsel_selectForUTF8");
  }

  return ptr(sel, s, length, status);
}
void u_versionFromString(UVersionInfo versionArray, const char * versionString) {
  typedef decltype(&u_versionFromString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_versionFromString"));

  if (ptr == nullptr) {
    do_fail("u_versionFromString");
  }

  ptr(versionArray, versionString);
}
void u_versionFromUString(UVersionInfo versionArray, const UChar * versionString) {
  typedef decltype(&u_versionFromUString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_versionFromUString"));

  if (ptr == nullptr) {
    do_fail("u_versionFromUString");
  }

  ptr(versionArray, versionString);
}
void u_versionToString(const UVersionInfo versionArray, char * versionString) {
  typedef decltype(&u_versionToString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_versionToString"));

  if (ptr == nullptr) {
    do_fail("u_versionToString");
  }

  ptr(versionArray, versionString);
}
void u_getVersion(UVersionInfo versionArray) {
  typedef decltype(&u_getVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_getVersion"));

  if (ptr == nullptr) {
    do_fail("u_getVersion");
  }

  ptr(versionArray);
}
void utrace_setLevel(int32_t traceLevel) {
  typedef decltype(&utrace_setLevel) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utrace_setLevel"));

  if (ptr == nullptr) {
    do_fail("utrace_setLevel");
  }

  ptr(traceLevel);
}
int32_t utrace_getLevel() {
  typedef decltype(&utrace_getLevel) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utrace_getLevel"));

  if (ptr == nullptr) {
    do_fail("utrace_getLevel");
  }

  return ptr();
}
void utrace_setFunctions(const void * context, UTraceEntry * e, UTraceExit * x, UTraceData * d) {
  typedef decltype(&utrace_setFunctions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utrace_setFunctions"));

  if (ptr == nullptr) {
    do_fail("utrace_setFunctions");
  }

  ptr(context, e, x, d);
}
void utrace_getFunctions(const void ** context, UTraceEntry ** e, UTraceExit ** x, UTraceData ** d) {
  typedef decltype(&utrace_getFunctions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utrace_getFunctions"));

  if (ptr == nullptr) {
    do_fail("utrace_getFunctions");
  }

  ptr(context, e, x, d);
}
int32_t utrace_vformat(char * outBuf, int32_t capacity, int32_t indent, const char * fmt, va_list args) {
  typedef decltype(&utrace_vformat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utrace_vformat"));

  if (ptr == nullptr) {
    do_fail("utrace_vformat");
  }

  return ptr(outBuf, capacity, indent, fmt, args);
}
int32_t utrace_format(char * outBuf, int32_t capacity, int32_t indent, const char * fmt, ...) {
  typedef decltype(&utrace_vformat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utrace_vformat"));

  if (ptr == nullptr) {
    do_fail("utrace_format");
  }

  va_list args;
  va_start(args, fmt);
  int32_t ret = ptr(outBuf, capacity, indent, fmt, args);
  va_end(args);
  return ret;
}
const char * utrace_functionName(int32_t fnNumber) {
  typedef decltype(&utrace_functionName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utrace_functionName"));

  if (ptr == nullptr) {
    do_fail("utrace_functionName");
  }

  return ptr(fnNumber);
}
UResourceBundle * ures_open(const char * packageName, const char * locale, UErrorCode * status) {
  typedef decltype(&ures_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_open"));

  if (ptr == nullptr) {
    do_fail("ures_open");
  }

  return ptr(packageName, locale, status);
}
UResourceBundle * ures_openDirect(const char * packageName, const char * locale, UErrorCode * status) {
  typedef decltype(&ures_openDirect) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_openDirect"));

  if (ptr == nullptr) {
    do_fail("ures_openDirect");
  }

  return ptr(packageName, locale, status);
}
UResourceBundle * ures_openU(const UChar * packageName, const char * locale, UErrorCode * status) {
  typedef decltype(&ures_openU) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_openU"));

  if (ptr == nullptr) {
    do_fail("ures_openU");
  }

  return ptr(packageName, locale, status);
}
void ures_close(UResourceBundle * resourceBundle) {
  typedef decltype(&ures_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_close"));

  if (ptr == nullptr) {
    do_fail("ures_close");
  }

  ptr(resourceBundle);
}
void ures_getVersion(const UResourceBundle * resB, UVersionInfo versionInfo) {
  typedef decltype(&ures_getVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getVersion"));

  if (ptr == nullptr) {
    do_fail("ures_getVersion");
  }

  ptr(resB, versionInfo);
}
const char * ures_getLocaleByType(const UResourceBundle * resourceBundle, ULocDataLocaleType type, UErrorCode * status) {
  typedef decltype(&ures_getLocaleByType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getLocaleByType"));

  if (ptr == nullptr) {
    do_fail("ures_getLocaleByType");
  }

  return ptr(resourceBundle, type, status);
}
const UChar * ures_getString(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status) {
  typedef decltype(&ures_getString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getString"));

  if (ptr == nullptr) {
    do_fail("ures_getString");
  }

  return ptr(resourceBundle, len, status);
}
const char * ures_getUTF8String(const UResourceBundle * resB, char * dest, int32_t * length, UBool forceCopy, UErrorCode * status) {
  typedef decltype(&ures_getUTF8String) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getUTF8String"));

  if (ptr == nullptr) {
    do_fail("ures_getUTF8String");
  }

  return ptr(resB, dest, length, forceCopy, status);
}
const uint8_t * ures_getBinary(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status) {
  typedef decltype(&ures_getBinary) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getBinary"));

  if (ptr == nullptr) {
    do_fail("ures_getBinary");
  }

  return ptr(resourceBundle, len, status);
}
const int32_t * ures_getIntVector(const UResourceBundle * resourceBundle, int32_t * len, UErrorCode * status) {
  typedef decltype(&ures_getIntVector) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getIntVector"));

  if (ptr == nullptr) {
    do_fail("ures_getIntVector");
  }

  return ptr(resourceBundle, len, status);
}
uint32_t ures_getUInt(const UResourceBundle * resourceBundle, UErrorCode * status) {
  typedef decltype(&ures_getUInt) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getUInt"));

  if (ptr == nullptr) {
    do_fail("ures_getUInt");
  }

  return ptr(resourceBundle, status);
}
int32_t ures_getInt(const UResourceBundle * resourceBundle, UErrorCode * status) {
  typedef decltype(&ures_getInt) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getInt"));

  if (ptr == nullptr) {
    do_fail("ures_getInt");
  }

  return ptr(resourceBundle, status);
}
int32_t ures_getSize(const UResourceBundle * resourceBundle) {
  typedef decltype(&ures_getSize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getSize"));

  if (ptr == nullptr) {
    do_fail("ures_getSize");
  }

  return ptr(resourceBundle);
}
UResType ures_getType(const UResourceBundle * resourceBundle) {
  typedef decltype(&ures_getType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getType"));

  if (ptr == nullptr) {
    do_fail("ures_getType");
  }

  return ptr(resourceBundle);
}
const char * ures_getKey(const UResourceBundle * resourceBundle) {
  typedef decltype(&ures_getKey) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getKey"));

  if (ptr == nullptr) {
    do_fail("ures_getKey");
  }

  return ptr(resourceBundle);
}
void ures_resetIterator(UResourceBundle * resourceBundle) {
  typedef decltype(&ures_resetIterator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_resetIterator"));

  if (ptr == nullptr) {
    do_fail("ures_resetIterator");
  }

  ptr(resourceBundle);
}
UBool ures_hasNext(const UResourceBundle * resourceBundle) {
  typedef decltype(&ures_hasNext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_hasNext"));

  if (ptr == nullptr) {
    do_fail("ures_hasNext");
  }

  return ptr(resourceBundle);
}
UResourceBundle * ures_getNextResource(UResourceBundle * resourceBundle, UResourceBundle * fillIn, UErrorCode * status) {
  typedef decltype(&ures_getNextResource) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getNextResource"));

  if (ptr == nullptr) {
    do_fail("ures_getNextResource");
  }

  return ptr(resourceBundle, fillIn, status);
}
const UChar * ures_getNextString(UResourceBundle * resourceBundle, int32_t * len, const char ** key, UErrorCode * status) {
  typedef decltype(&ures_getNextString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getNextString"));

  if (ptr == nullptr) {
    do_fail("ures_getNextString");
  }

  return ptr(resourceBundle, len, key, status);
}
UResourceBundle * ures_getByIndex(const UResourceBundle * resourceBundle, int32_t indexR, UResourceBundle * fillIn, UErrorCode * status) {
  typedef decltype(&ures_getByIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getByIndex"));

  if (ptr == nullptr) {
    do_fail("ures_getByIndex");
  }

  return ptr(resourceBundle, indexR, fillIn, status);
}
const UChar * ures_getStringByIndex(const UResourceBundle * resourceBundle, int32_t indexS, int32_t * len, UErrorCode * status) {
  typedef decltype(&ures_getStringByIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getStringByIndex"));

  if (ptr == nullptr) {
    do_fail("ures_getStringByIndex");
  }

  return ptr(resourceBundle, indexS, len, status);
}
const char * ures_getUTF8StringByIndex(const UResourceBundle * resB, int32_t stringIndex, char * dest, int32_t * pLength, UBool forceCopy, UErrorCode * status) {
  typedef decltype(&ures_getUTF8StringByIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getUTF8StringByIndex"));

  if (ptr == nullptr) {
    do_fail("ures_getUTF8StringByIndex");
  }

  return ptr(resB, stringIndex, dest, pLength, forceCopy, status);
}
UResourceBundle * ures_getByKey(const UResourceBundle * resourceBundle, const char * key, UResourceBundle * fillIn, UErrorCode * status) {
  typedef decltype(&ures_getByKey) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getByKey"));

  if (ptr == nullptr) {
    do_fail("ures_getByKey");
  }

  return ptr(resourceBundle, key, fillIn, status);
}
const UChar * ures_getStringByKey(const UResourceBundle * resB, const char * key, int32_t * len, UErrorCode * status) {
  typedef decltype(&ures_getStringByKey) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getStringByKey"));

  if (ptr == nullptr) {
    do_fail("ures_getStringByKey");
  }

  return ptr(resB, key, len, status);
}
const char * ures_getUTF8StringByKey(const UResourceBundle * resB, const char * key, char * dest, int32_t * pLength, UBool forceCopy, UErrorCode * status) {
  typedef decltype(&ures_getUTF8StringByKey) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_getUTF8StringByKey"));

  if (ptr == nullptr) {
    do_fail("ures_getUTF8StringByKey");
  }

  return ptr(resB, key, dest, pLength, forceCopy, status);
}
UEnumeration * ures_openAvailableLocales(const char * packageName, UErrorCode * status) {
  typedef decltype(&ures_openAvailableLocales) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ures_openAvailableLocales"));

  if (ptr == nullptr) {
    do_fail("ures_openAvailableLocales");
  }

  return ptr(packageName, status);
}
USet * uset_openEmpty() {
  typedef decltype(&uset_openEmpty) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_openEmpty"));

  if (ptr == nullptr) {
    do_fail("uset_openEmpty");
  }

  return ptr();
}
USet * uset_open(UChar32 start, UChar32 end) {
  typedef decltype(&uset_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_open"));

  if (ptr == nullptr) {
    do_fail("uset_open");
  }

  return ptr(start, end);
}
USet * uset_openPattern(const UChar * pattern, int32_t patternLength, UErrorCode * ec) {
  typedef decltype(&uset_openPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_openPattern"));

  if (ptr == nullptr) {
    do_fail("uset_openPattern");
  }

  return ptr(pattern, patternLength, ec);
}
USet * uset_openPatternOptions(const UChar * pattern, int32_t patternLength, uint32_t options, UErrorCode * ec) {
  typedef decltype(&uset_openPatternOptions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_openPatternOptions"));

  if (ptr == nullptr) {
    do_fail("uset_openPatternOptions");
  }

  return ptr(pattern, patternLength, options, ec);
}
void uset_close(USet * set) {
  typedef decltype(&uset_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_close"));

  if (ptr == nullptr) {
    do_fail("uset_close");
  }

  ptr(set);
}
USet * uset_clone(const USet * set) {
  typedef decltype(&uset_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_clone"));

  if (ptr == nullptr) {
    do_fail("uset_clone");
  }

  return ptr(set);
}
UBool uset_isFrozen(const USet * set) {
  typedef decltype(&uset_isFrozen) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_isFrozen"));

  if (ptr == nullptr) {
    do_fail("uset_isFrozen");
  }

  return ptr(set);
}
void uset_freeze(USet * set) {
  typedef decltype(&uset_freeze) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_freeze"));

  if (ptr == nullptr) {
    do_fail("uset_freeze");
  }

  ptr(set);
}
USet * uset_cloneAsThawed(const USet * set) {
  typedef decltype(&uset_cloneAsThawed) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_cloneAsThawed"));

  if (ptr == nullptr) {
    do_fail("uset_cloneAsThawed");
  }

  return ptr(set);
}
void uset_set(USet * set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_set) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_set"));

  if (ptr == nullptr) {
    do_fail("uset_set");
  }

  ptr(set, start, end);
}
int32_t uset_applyPattern(USet * set, const UChar * pattern, int32_t patternLength, uint32_t options, UErrorCode * status) {
  typedef decltype(&uset_applyPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_applyPattern"));

  if (ptr == nullptr) {
    do_fail("uset_applyPattern");
  }

  return ptr(set, pattern, patternLength, options, status);
}
void uset_applyIntPropertyValue(USet * set, UProperty prop, int32_t value, UErrorCode * ec) {
  typedef decltype(&uset_applyIntPropertyValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_applyIntPropertyValue"));

  if (ptr == nullptr) {
    do_fail("uset_applyIntPropertyValue");
  }

  ptr(set, prop, value, ec);
}
void uset_applyPropertyAlias(USet * set, const UChar * prop, int32_t propLength, const UChar * value, int32_t valueLength, UErrorCode * ec) {
  typedef decltype(&uset_applyPropertyAlias) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_applyPropertyAlias"));

  if (ptr == nullptr) {
    do_fail("uset_applyPropertyAlias");
  }

  ptr(set, prop, propLength, value, valueLength, ec);
}
UBool uset_resemblesPattern(const UChar * pattern, int32_t patternLength, int32_t pos) {
  typedef decltype(&uset_resemblesPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_resemblesPattern"));

  if (ptr == nullptr) {
    do_fail("uset_resemblesPattern");
  }

  return ptr(pattern, patternLength, pos);
}
int32_t uset_toPattern(const USet * set, UChar * result, int32_t resultCapacity, UBool escapeUnprintable, UErrorCode * ec) {
  typedef decltype(&uset_toPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_toPattern"));

  if (ptr == nullptr) {
    do_fail("uset_toPattern");
  }

  return ptr(set, result, resultCapacity, escapeUnprintable, ec);
}
void uset_add(USet * set, UChar32 c) {
  typedef decltype(&uset_add) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_add"));

  if (ptr == nullptr) {
    do_fail("uset_add");
  }

  ptr(set, c);
}
void uset_addAll(USet * set, const USet * additionalSet) {
  typedef decltype(&uset_addAll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_addAll"));

  if (ptr == nullptr) {
    do_fail("uset_addAll");
  }

  ptr(set, additionalSet);
}
void uset_addRange(USet * set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_addRange) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_addRange"));

  if (ptr == nullptr) {
    do_fail("uset_addRange");
  }

  ptr(set, start, end);
}
void uset_addString(USet * set, const UChar * str, int32_t strLen) {
  typedef decltype(&uset_addString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_addString"));

  if (ptr == nullptr) {
    do_fail("uset_addString");
  }

  ptr(set, str, strLen);
}
void uset_addAllCodePoints(USet * set, const UChar * str, int32_t strLen) {
  typedef decltype(&uset_addAllCodePoints) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_addAllCodePoints"));

  if (ptr == nullptr) {
    do_fail("uset_addAllCodePoints");
  }

  ptr(set, str, strLen);
}
void uset_remove(USet * set, UChar32 c) {
  typedef decltype(&uset_remove) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_remove"));

  if (ptr == nullptr) {
    do_fail("uset_remove");
  }

  ptr(set, c);
}
void uset_removeRange(USet * set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_removeRange) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_removeRange"));

  if (ptr == nullptr) {
    do_fail("uset_removeRange");
  }

  ptr(set, start, end);
}
void uset_removeString(USet * set, const UChar * str, int32_t strLen) {
  typedef decltype(&uset_removeString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_removeString"));

  if (ptr == nullptr) {
    do_fail("uset_removeString");
  }

  ptr(set, str, strLen);
}
void uset_removeAll(USet * set, const USet * removeSet) {
  typedef decltype(&uset_removeAll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_removeAll"));

  if (ptr == nullptr) {
    do_fail("uset_removeAll");
  }

  ptr(set, removeSet);
}
void uset_retain(USet * set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_retain) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_retain"));

  if (ptr == nullptr) {
    do_fail("uset_retain");
  }

  ptr(set, start, end);
}
void uset_retainAll(USet * set, const USet * retain) {
  typedef decltype(&uset_retainAll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_retainAll"));

  if (ptr == nullptr) {
    do_fail("uset_retainAll");
  }

  ptr(set, retain);
}
void uset_compact(USet * set) {
  typedef decltype(&uset_compact) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_compact"));

  if (ptr == nullptr) {
    do_fail("uset_compact");
  }

  ptr(set);
}
void uset_complement(USet * set) {
  typedef decltype(&uset_complement) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_complement"));

  if (ptr == nullptr) {
    do_fail("uset_complement");
  }

  ptr(set);
}
void uset_complementAll(USet * set, const USet * complement) {
  typedef decltype(&uset_complementAll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_complementAll"));

  if (ptr == nullptr) {
    do_fail("uset_complementAll");
  }

  ptr(set, complement);
}
void uset_clear(USet * set) {
  typedef decltype(&uset_clear) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_clear"));

  if (ptr == nullptr) {
    do_fail("uset_clear");
  }

  ptr(set);
}
void uset_closeOver(USet * set, int32_t attributes) {
  typedef decltype(&uset_closeOver) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_closeOver"));

  if (ptr == nullptr) {
    do_fail("uset_closeOver");
  }

  ptr(set, attributes);
}
void uset_removeAllStrings(USet * set) {
  typedef decltype(&uset_removeAllStrings) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_removeAllStrings"));

  if (ptr == nullptr) {
    do_fail("uset_removeAllStrings");
  }

  ptr(set);
}
UBool uset_isEmpty(const USet * set) {
  typedef decltype(&uset_isEmpty) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_isEmpty"));

  if (ptr == nullptr) {
    do_fail("uset_isEmpty");
  }

  return ptr(set);
}
UBool uset_contains(const USet * set, UChar32 c) {
  typedef decltype(&uset_contains) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_contains"));

  if (ptr == nullptr) {
    do_fail("uset_contains");
  }

  return ptr(set, c);
}
UBool uset_containsRange(const USet * set, UChar32 start, UChar32 end) {
  typedef decltype(&uset_containsRange) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_containsRange"));

  if (ptr == nullptr) {
    do_fail("uset_containsRange");
  }

  return ptr(set, start, end);
}
UBool uset_containsString(const USet * set, const UChar * str, int32_t strLen) {
  typedef decltype(&uset_containsString) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_containsString"));

  if (ptr == nullptr) {
    do_fail("uset_containsString");
  }

  return ptr(set, str, strLen);
}
int32_t uset_indexOf(const USet * set, UChar32 c) {
  typedef decltype(&uset_indexOf) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_indexOf"));

  if (ptr == nullptr) {
    do_fail("uset_indexOf");
  }

  return ptr(set, c);
}
UChar32 uset_charAt(const USet * set, int32_t charIndex) {
  typedef decltype(&uset_charAt) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_charAt"));

  if (ptr == nullptr) {
    do_fail("uset_charAt");
  }

  return ptr(set, charIndex);
}
int32_t uset_size(const USet * set) {
  typedef decltype(&uset_size) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_size"));

  if (ptr == nullptr) {
    do_fail("uset_size");
  }

  return ptr(set);
}
int32_t uset_getItemCount(const USet * set) {
  typedef decltype(&uset_getItemCount) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_getItemCount"));

  if (ptr == nullptr) {
    do_fail("uset_getItemCount");
  }

  return ptr(set);
}
int32_t uset_getItem(const USet * set, int32_t itemIndex, UChar32 * start, UChar32 * end, UChar * str, int32_t strCapacity, UErrorCode * ec) {
  typedef decltype(&uset_getItem) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_getItem"));

  if (ptr == nullptr) {
    do_fail("uset_getItem");
  }

  return ptr(set, itemIndex, start, end, str, strCapacity, ec);
}
UBool uset_containsAll(const USet * set1, const USet * set2) {
  typedef decltype(&uset_containsAll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_containsAll"));

  if (ptr == nullptr) {
    do_fail("uset_containsAll");
  }

  return ptr(set1, set2);
}
UBool uset_containsAllCodePoints(const USet * set, const UChar * str, int32_t strLen) {
  typedef decltype(&uset_containsAllCodePoints) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_containsAllCodePoints"));

  if (ptr == nullptr) {
    do_fail("uset_containsAllCodePoints");
  }

  return ptr(set, str, strLen);
}
UBool uset_containsNone(const USet * set1, const USet * set2) {
  typedef decltype(&uset_containsNone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_containsNone"));

  if (ptr == nullptr) {
    do_fail("uset_containsNone");
  }

  return ptr(set1, set2);
}
UBool uset_containsSome(const USet * set1, const USet * set2) {
  typedef decltype(&uset_containsSome) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_containsSome"));

  if (ptr == nullptr) {
    do_fail("uset_containsSome");
  }

  return ptr(set1, set2);
}
int32_t uset_span(const USet * set, const UChar * s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_span) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_span"));

  if (ptr == nullptr) {
    do_fail("uset_span");
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanBack(const USet * set, const UChar * s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_spanBack) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_spanBack"));

  if (ptr == nullptr) {
    do_fail("uset_spanBack");
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanUTF8(const USet * set, const char * s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_spanUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_spanUTF8"));

  if (ptr == nullptr) {
    do_fail("uset_spanUTF8");
  }

  return ptr(set, s, length, spanCondition);
}
int32_t uset_spanBackUTF8(const USet * set, const char * s, int32_t length, USetSpanCondition spanCondition) {
  typedef decltype(&uset_spanBackUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_spanBackUTF8"));

  if (ptr == nullptr) {
    do_fail("uset_spanBackUTF8");
  }

  return ptr(set, s, length, spanCondition);
}
UBool uset_equals(const USet * set1, const USet * set2) {
  typedef decltype(&uset_equals) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_equals"));

  if (ptr == nullptr) {
    do_fail("uset_equals");
  }

  return ptr(set1, set2);
}
int32_t uset_serialize(const USet * set, uint16_t * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  typedef decltype(&uset_serialize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_serialize"));

  if (ptr == nullptr) {
    do_fail("uset_serialize");
  }

  return ptr(set, dest, destCapacity, pErrorCode);
}
UBool uset_getSerializedSet(USerializedSet * fillSet, const uint16_t * src, int32_t srcLength) {
  typedef decltype(&uset_getSerializedSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_getSerializedSet"));

  if (ptr == nullptr) {
    do_fail("uset_getSerializedSet");
  }

  return ptr(fillSet, src, srcLength);
}
void uset_setSerializedToOne(USerializedSet * fillSet, UChar32 c) {
  typedef decltype(&uset_setSerializedToOne) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_setSerializedToOne"));

  if (ptr == nullptr) {
    do_fail("uset_setSerializedToOne");
  }

  ptr(fillSet, c);
}
UBool uset_serializedContains(const USerializedSet * set, UChar32 c) {
  typedef decltype(&uset_serializedContains) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_serializedContains"));

  if (ptr == nullptr) {
    do_fail("uset_serializedContains");
  }

  return ptr(set, c);
}
int32_t uset_getSerializedRangeCount(const USerializedSet * set) {
  typedef decltype(&uset_getSerializedRangeCount) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_getSerializedRangeCount"));

  if (ptr == nullptr) {
    do_fail("uset_getSerializedRangeCount");
  }

  return ptr(set);
}
UBool uset_getSerializedRange(const USerializedSet * set, int32_t rangeIndex, UChar32 * pStart, UChar32 * pEnd) {
  typedef decltype(&uset_getSerializedRange) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uset_getSerializedRange"));

  if (ptr == nullptr) {
    do_fail("uset_getSerializedRange");
  }

  return ptr(set, rangeIndex, pStart, pEnd);
}
int32_t u_strlen(const UChar * s) {
  typedef decltype(&u_strlen) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strlen"));

  if (ptr == nullptr) {
    do_fail("u_strlen");
  }

  return ptr(s);
}
int32_t u_countChar32(const UChar * s, int32_t length) {
  typedef decltype(&u_countChar32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_countChar32"));

  if (ptr == nullptr) {
    do_fail("u_countChar32");
  }

  return ptr(s, length);
}
UBool u_strHasMoreChar32Than(const UChar * s, int32_t length, int32_t number) {
  typedef decltype(&u_strHasMoreChar32Than) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strHasMoreChar32Than"));

  if (ptr == nullptr) {
    do_fail("u_strHasMoreChar32Than");
  }

  return ptr(s, length, number);
}
UChar * u_strcat(UChar * dst, const UChar * src) {
  typedef decltype(&u_strcat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strcat"));

  if (ptr == nullptr) {
    do_fail("u_strcat");
  }

  return ptr(dst, src);
}
UChar * u_strncat(UChar * dst, const UChar * src, int32_t n) {
  typedef decltype(&u_strncat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strncat"));

  if (ptr == nullptr) {
    do_fail("u_strncat");
  }

  return ptr(dst, src, n);
}
UChar * u_strstr(const UChar * s, const UChar * substring) {
  typedef decltype(&u_strstr) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strstr"));

  if (ptr == nullptr) {
    do_fail("u_strstr");
  }

  return ptr(s, substring);
}
UChar * u_strFindFirst(const UChar * s, int32_t length, const UChar * substring, int32_t subLength) {
  typedef decltype(&u_strFindFirst) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFindFirst"));

  if (ptr == nullptr) {
    do_fail("u_strFindFirst");
  }

  return ptr(s, length, substring, subLength);
}
UChar * u_strchr(const UChar * s, UChar c) {
  typedef decltype(&u_strchr) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strchr"));

  if (ptr == nullptr) {
    do_fail("u_strchr");
  }

  return ptr(s, c);
}
UChar * u_strchr32(const UChar * s, UChar32 c) {
  typedef decltype(&u_strchr32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strchr32"));

  if (ptr == nullptr) {
    do_fail("u_strchr32");
  }

  return ptr(s, c);
}
UChar * u_strrstr(const UChar * s, const UChar * substring) {
  typedef decltype(&u_strrstr) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strrstr"));

  if (ptr == nullptr) {
    do_fail("u_strrstr");
  }

  return ptr(s, substring);
}
UChar * u_strFindLast(const UChar * s, int32_t length, const UChar * substring, int32_t subLength) {
  typedef decltype(&u_strFindLast) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFindLast"));

  if (ptr == nullptr) {
    do_fail("u_strFindLast");
  }

  return ptr(s, length, substring, subLength);
}
UChar * u_strrchr(const UChar * s, UChar c) {
  typedef decltype(&u_strrchr) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strrchr"));

  if (ptr == nullptr) {
    do_fail("u_strrchr");
  }

  return ptr(s, c);
}
UChar * u_strrchr32(const UChar * s, UChar32 c) {
  typedef decltype(&u_strrchr32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strrchr32"));

  if (ptr == nullptr) {
    do_fail("u_strrchr32");
  }

  return ptr(s, c);
}
UChar * u_strpbrk(const UChar * string, const UChar * matchSet) {
  typedef decltype(&u_strpbrk) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strpbrk"));

  if (ptr == nullptr) {
    do_fail("u_strpbrk");
  }

  return ptr(string, matchSet);
}
int32_t u_strcspn(const UChar * string, const UChar * matchSet) {
  typedef decltype(&u_strcspn) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strcspn"));

  if (ptr == nullptr) {
    do_fail("u_strcspn");
  }

  return ptr(string, matchSet);
}
int32_t u_strspn(const UChar * string, const UChar * matchSet) {
  typedef decltype(&u_strspn) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strspn"));

  if (ptr == nullptr) {
    do_fail("u_strspn");
  }

  return ptr(string, matchSet);
}
UChar * u_strtok_r(UChar * src, const UChar * delim, UChar ** saveState) {
  typedef decltype(&u_strtok_r) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strtok_r"));

  if (ptr == nullptr) {
    do_fail("u_strtok_r");
  }

  return ptr(src, delim, saveState);
}
int32_t u_strcmp(const UChar * s1, const UChar * s2) {
  typedef decltype(&u_strcmp) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strcmp"));

  if (ptr == nullptr) {
    do_fail("u_strcmp");
  }

  return ptr(s1, s2);
}
int32_t u_strcmpCodePointOrder(const UChar * s1, const UChar * s2) {
  typedef decltype(&u_strcmpCodePointOrder) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strcmpCodePointOrder"));

  if (ptr == nullptr) {
    do_fail("u_strcmpCodePointOrder");
  }

  return ptr(s1, s2);
}
int32_t u_strCompare(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, UBool codePointOrder) {
  typedef decltype(&u_strCompare) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strCompare"));

  if (ptr == nullptr) {
    do_fail("u_strCompare");
  }

  return ptr(s1, length1, s2, length2, codePointOrder);
}
int32_t u_strCompareIter(UCharIterator * iter1, UCharIterator * iter2, UBool codePointOrder) {
  typedef decltype(&u_strCompareIter) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strCompareIter"));

  if (ptr == nullptr) {
    do_fail("u_strCompareIter");
  }

  return ptr(iter1, iter2, codePointOrder);
}
int32_t u_strCaseCompare(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, uint32_t options, UErrorCode * pErrorCode) {
  typedef decltype(&u_strCaseCompare) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strCaseCompare"));

  if (ptr == nullptr) {
    do_fail("u_strCaseCompare");
  }

  return ptr(s1, length1, s2, length2, options, pErrorCode);
}
int32_t u_strncmp(const UChar * ucs1, const UChar * ucs2, int32_t n) {
  typedef decltype(&u_strncmp) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strncmp"));

  if (ptr == nullptr) {
    do_fail("u_strncmp");
  }

  return ptr(ucs1, ucs2, n);
}
int32_t u_strncmpCodePointOrder(const UChar * s1, const UChar * s2, int32_t n) {
  typedef decltype(&u_strncmpCodePointOrder) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strncmpCodePointOrder"));

  if (ptr == nullptr) {
    do_fail("u_strncmpCodePointOrder");
  }

  return ptr(s1, s2, n);
}
int32_t u_strcasecmp(const UChar * s1, const UChar * s2, uint32_t options) {
  typedef decltype(&u_strcasecmp) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strcasecmp"));

  if (ptr == nullptr) {
    do_fail("u_strcasecmp");
  }

  return ptr(s1, s2, options);
}
int32_t u_strncasecmp(const UChar * s1, const UChar * s2, int32_t n, uint32_t options) {
  typedef decltype(&u_strncasecmp) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strncasecmp"));

  if (ptr == nullptr) {
    do_fail("u_strncasecmp");
  }

  return ptr(s1, s2, n, options);
}
int32_t u_memcasecmp(const UChar * s1, const UChar * s2, int32_t length, uint32_t options) {
  typedef decltype(&u_memcasecmp) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memcasecmp"));

  if (ptr == nullptr) {
    do_fail("u_memcasecmp");
  }

  return ptr(s1, s2, length, options);
}
UChar * u_strcpy(UChar * dst, const UChar * src) {
  typedef decltype(&u_strcpy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strcpy"));

  if (ptr == nullptr) {
    do_fail("u_strcpy");
  }

  return ptr(dst, src);
}
UChar * u_strncpy(UChar * dst, const UChar * src, int32_t n) {
  typedef decltype(&u_strncpy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strncpy"));

  if (ptr == nullptr) {
    do_fail("u_strncpy");
  }

  return ptr(dst, src, n);
}
UChar * u_uastrcpy(UChar * dst, const char * src) {
  typedef decltype(&u_uastrcpy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_uastrcpy"));

  if (ptr == nullptr) {
    do_fail("u_uastrcpy");
  }

  return ptr(dst, src);
}
UChar * u_uastrncpy(UChar * dst, const char * src, int32_t n) {
  typedef decltype(&u_uastrncpy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_uastrncpy"));

  if (ptr == nullptr) {
    do_fail("u_uastrncpy");
  }

  return ptr(dst, src, n);
}
char * u_austrcpy(char * dst, const UChar * src) {
  typedef decltype(&u_austrcpy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_austrcpy"));

  if (ptr == nullptr) {
    do_fail("u_austrcpy");
  }

  return ptr(dst, src);
}
char * u_austrncpy(char * dst, const UChar * src, int32_t n) {
  typedef decltype(&u_austrncpy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_austrncpy"));

  if (ptr == nullptr) {
    do_fail("u_austrncpy");
  }

  return ptr(dst, src, n);
}
UChar * u_memcpy(UChar * dest, const UChar * src, int32_t count) {
  typedef decltype(&u_memcpy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memcpy"));

  if (ptr == nullptr) {
    do_fail("u_memcpy");
  }

  return ptr(dest, src, count);
}
UChar * u_memmove(UChar * dest, const UChar * src, int32_t count) {
  typedef decltype(&u_memmove) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memmove"));

  if (ptr == nullptr) {
    do_fail("u_memmove");
  }

  return ptr(dest, src, count);
}
UChar * u_memset(UChar * dest, UChar c, int32_t count) {
  typedef decltype(&u_memset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memset"));

  if (ptr == nullptr) {
    do_fail("u_memset");
  }

  return ptr(dest, c, count);
}
int32_t u_memcmp(const UChar * buf1, const UChar * buf2, int32_t count) {
  typedef decltype(&u_memcmp) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memcmp"));

  if (ptr == nullptr) {
    do_fail("u_memcmp");
  }

  return ptr(buf1, buf2, count);
}
int32_t u_memcmpCodePointOrder(const UChar * s1, const UChar * s2, int32_t count) {
  typedef decltype(&u_memcmpCodePointOrder) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memcmpCodePointOrder"));

  if (ptr == nullptr) {
    do_fail("u_memcmpCodePointOrder");
  }

  return ptr(s1, s2, count);
}
UChar * u_memchr(const UChar * s, UChar c, int32_t count) {
  typedef decltype(&u_memchr) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memchr"));

  if (ptr == nullptr) {
    do_fail("u_memchr");
  }

  return ptr(s, c, count);
}
UChar * u_memchr32(const UChar * s, UChar32 c, int32_t count) {
  typedef decltype(&u_memchr32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memchr32"));

  if (ptr == nullptr) {
    do_fail("u_memchr32");
  }

  return ptr(s, c, count);
}
UChar * u_memrchr(const UChar * s, UChar c, int32_t count) {
  typedef decltype(&u_memrchr) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memrchr"));

  if (ptr == nullptr) {
    do_fail("u_memrchr");
  }

  return ptr(s, c, count);
}
UChar * u_memrchr32(const UChar * s, UChar32 c, int32_t count) {
  typedef decltype(&u_memrchr32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_memrchr32"));

  if (ptr == nullptr) {
    do_fail("u_memrchr32");
  }

  return ptr(s, c, count);
}
int32_t u_unescape(const char * src, UChar * dest, int32_t destCapacity) {
  typedef decltype(&u_unescape) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_unescape"));

  if (ptr == nullptr) {
    do_fail("u_unescape");
  }

  return ptr(src, dest, destCapacity);
}
UChar32 u_unescapeAt(UNESCAPE_CHAR_AT charAt, int32_t * offset, int32_t length, void * context) {
  typedef decltype(&u_unescapeAt) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_unescapeAt"));

  if (ptr == nullptr) {
    do_fail("u_unescapeAt");
  }

  return ptr(charAt, offset, length, context);
}
int32_t u_strToUpper(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToUpper) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToUpper"));

  if (ptr == nullptr) {
    do_fail("u_strToUpper");
  }

  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}
int32_t u_strToLower(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToLower) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToLower"));

  if (ptr == nullptr) {
    do_fail("u_strToLower");
  }

  return ptr(dest, destCapacity, src, srcLength, locale, pErrorCode);
}
int32_t u_strToTitle(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, UBreakIterator * titleIter, const char * locale, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToTitle) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToTitle"));

  if (ptr == nullptr) {
    do_fail("u_strToTitle");
  }

  return ptr(dest, destCapacity, src, srcLength, titleIter, locale, pErrorCode);
}
int32_t u_strFoldCase(UChar * dest, int32_t destCapacity, const UChar * src, int32_t srcLength, uint32_t options, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFoldCase) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFoldCase"));

  if (ptr == nullptr) {
    do_fail("u_strFoldCase");
  }

  return ptr(dest, destCapacity, src, srcLength, options, pErrorCode);
}
wchar_t * u_strToWCS(wchar_t * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToWCS) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToWCS"));

  if (ptr == nullptr) {
    do_fail("u_strToWCS");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromWCS(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const wchar_t * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFromWCS) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFromWCS"));

  if (ptr == nullptr) {
    do_fail("u_strFromWCS");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
char * u_strToUTF8(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToUTF8"));

  if (ptr == nullptr) {
    do_fail("u_strToUTF8");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromUTF8(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFromUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFromUTF8"));

  if (ptr == nullptr) {
    do_fail("u_strFromUTF8");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
char * u_strToUTF8WithSub(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToUTF8WithSub) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToUTF8WithSub"));

  if (ptr == nullptr) {
    do_fail("u_strToUTF8WithSub");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar * u_strFromUTF8WithSub(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFromUTF8WithSub) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFromUTF8WithSub"));

  if (ptr == nullptr) {
    do_fail("u_strFromUTF8WithSub");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar * u_strFromUTF8Lenient(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFromUTF8Lenient) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFromUTF8Lenient"));

  if (ptr == nullptr) {
    do_fail("u_strFromUTF8Lenient");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar32 * u_strToUTF32(UChar32 * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToUTF32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToUTF32"));

  if (ptr == nullptr) {
    do_fail("u_strToUTF32");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromUTF32(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const UChar32 * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFromUTF32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFromUTF32"));

  if (ptr == nullptr) {
    do_fail("u_strFromUTF32");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar32 * u_strToUTF32WithSub(UChar32 * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToUTF32WithSub) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToUTF32WithSub"));

  if (ptr == nullptr) {
    do_fail("u_strToUTF32WithSub");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar * u_strFromUTF32WithSub(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const UChar32 * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFromUTF32WithSub) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFromUTF32WithSub"));

  if (ptr == nullptr) {
    do_fail("u_strFromUTF32WithSub");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
char * u_strToJavaModifiedUTF8(char * dest, int32_t destCapacity, int32_t * pDestLength, const UChar * src, int32_t srcLength, UErrorCode * pErrorCode) {
  typedef decltype(&u_strToJavaModifiedUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strToJavaModifiedUTF8"));

  if (ptr == nullptr) {
    do_fail("u_strToJavaModifiedUTF8");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, pErrorCode);
}
UChar * u_strFromJavaModifiedUTF8WithSub(UChar * dest, int32_t destCapacity, int32_t * pDestLength, const char * src, int32_t srcLength, UChar32 subchar, int32_t * pNumSubstitutions, UErrorCode * pErrorCode) {
  typedef decltype(&u_strFromJavaModifiedUTF8WithSub) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_strFromJavaModifiedUTF8WithSub"));

  if (ptr == nullptr) {
    do_fail("u_strFromJavaModifiedUTF8WithSub");
  }

  return ptr(dest, destCapacity, pDestLength, src, srcLength, subchar, pNumSubstitutions, pErrorCode);
}
UChar32 utf8_nextCharSafeBody(const uint8_t * s, int32_t * pi, int32_t length, UChar32 c, UBool strict) {
  typedef decltype(&utf8_nextCharSafeBody) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utf8_nextCharSafeBody"));

  if (ptr == nullptr) {
    do_fail("utf8_nextCharSafeBody");
  }

  return ptr(s, pi, length, c, strict);
}
int32_t utf8_appendCharSafeBody(uint8_t * s, int32_t i, int32_t length, UChar32 c, UBool * pIsError) {
  typedef decltype(&utf8_appendCharSafeBody) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utf8_appendCharSafeBody"));

  if (ptr == nullptr) {
    do_fail("utf8_appendCharSafeBody");
  }

  return ptr(s, i, length, c, pIsError);
}
UChar32 utf8_prevCharSafeBody(const uint8_t * s, int32_t start, int32_t * pi, UChar32 c, UBool strict) {
  typedef decltype(&utf8_prevCharSafeBody) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utf8_prevCharSafeBody"));

  if (ptr == nullptr) {
    do_fail("utf8_prevCharSafeBody");
  }

  return ptr(s, start, pi, c, strict);
}
int32_t utf8_back1SafeBody(const uint8_t * s, int32_t start, int32_t i) {
  typedef decltype(&utf8_back1SafeBody) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utf8_back1SafeBody"));

  if (ptr == nullptr) {
    do_fail("utf8_back1SafeBody");
  }

  return ptr(s, start, i);
}
void u_init(UErrorCode * status) {
  typedef decltype(&u_init) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_init"));

  if (ptr == nullptr) {
    do_fail("u_init");
  }

  ptr(status);
}
void u_cleanup() {
  typedef decltype(&u_cleanup) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_cleanup"));

  if (ptr == nullptr) {
    do_fail("u_cleanup");
  }

  ptr();
}
void u_setMemoryFunctions(const void * context, UMemAllocFn * a, UMemReallocFn * r, UMemFreeFn * f, UErrorCode * status) {
  typedef decltype(&u_setMemoryFunctions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "u_setMemoryFunctions"));

  if (ptr == nullptr) {
    do_fail("u_setMemoryFunctions");
  }

  ptr(context, a, r, f, status);
}
UBreakIterator * ubrk_open(UBreakIteratorType type, const char * locale, const UChar * text, int32_t textLength, UErrorCode * status) {
  typedef decltype(&ubrk_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_open"));

  if (ptr == nullptr) {
    do_fail("ubrk_open");
  }

  return ptr(type, locale, text, textLength, status);
}
UBreakIterator * ubrk_openRules(const UChar * rules, int32_t rulesLength, const UChar * text, int32_t textLength, UParseError * parseErr, UErrorCode * status) {
  typedef decltype(&ubrk_openRules) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_openRules"));

  if (ptr == nullptr) {
    do_fail("ubrk_openRules");
  }

  return ptr(rules, rulesLength, text, textLength, parseErr, status);
}
UBreakIterator * ubrk_safeClone(const UBreakIterator * bi, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status) {
  typedef decltype(&ubrk_safeClone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_safeClone"));

  if (ptr == nullptr) {
    do_fail("ubrk_safeClone");
  }

  return ptr(bi, stackBuffer, pBufferSize, status);
}
void ubrk_close(UBreakIterator * bi) {
  typedef decltype(&ubrk_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_close"));

  if (ptr == nullptr) {
    do_fail("ubrk_close");
  }

  ptr(bi);
}
void ubrk_setText(UBreakIterator * bi, const UChar * text, int32_t textLength, UErrorCode * status) {
  typedef decltype(&ubrk_setText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_setText"));

  if (ptr == nullptr) {
    do_fail("ubrk_setText");
  }

  ptr(bi, text, textLength, status);
}
void ubrk_setUText(UBreakIterator * bi, UText * text, UErrorCode * status) {
  typedef decltype(&ubrk_setUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_setUText"));

  if (ptr == nullptr) {
    do_fail("ubrk_setUText");
  }

  ptr(bi, text, status);
}
int32_t ubrk_current(const UBreakIterator * bi) {
  typedef decltype(&ubrk_current) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_current"));

  if (ptr == nullptr) {
    do_fail("ubrk_current");
  }

  return ptr(bi);
}
int32_t ubrk_next(UBreakIterator * bi) {
  typedef decltype(&ubrk_next) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_next"));

  if (ptr == nullptr) {
    do_fail("ubrk_next");
  }

  return ptr(bi);
}
int32_t ubrk_previous(UBreakIterator * bi) {
  typedef decltype(&ubrk_previous) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_previous"));

  if (ptr == nullptr) {
    do_fail("ubrk_previous");
  }

  return ptr(bi);
}
int32_t ubrk_first(UBreakIterator * bi) {
  typedef decltype(&ubrk_first) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_first"));

  if (ptr == nullptr) {
    do_fail("ubrk_first");
  }

  return ptr(bi);
}
int32_t ubrk_last(UBreakIterator * bi) {
  typedef decltype(&ubrk_last) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_last"));

  if (ptr == nullptr) {
    do_fail("ubrk_last");
  }

  return ptr(bi);
}
int32_t ubrk_preceding(UBreakIterator * bi, int32_t offset) {
  typedef decltype(&ubrk_preceding) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_preceding"));

  if (ptr == nullptr) {
    do_fail("ubrk_preceding");
  }

  return ptr(bi, offset);
}
int32_t ubrk_following(UBreakIterator * bi, int32_t offset) {
  typedef decltype(&ubrk_following) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_following"));

  if (ptr == nullptr) {
    do_fail("ubrk_following");
  }

  return ptr(bi, offset);
}
const char * ubrk_getAvailable(int32_t index) {
  typedef decltype(&ubrk_getAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_getAvailable"));

  if (ptr == nullptr) {
    do_fail("ubrk_getAvailable");
  }

  return ptr(index);
}
int32_t ubrk_countAvailable() {
  typedef decltype(&ubrk_countAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_countAvailable"));

  if (ptr == nullptr) {
    do_fail("ubrk_countAvailable");
  }

  return ptr();
}
UBool ubrk_isBoundary(UBreakIterator * bi, int32_t offset) {
  typedef decltype(&ubrk_isBoundary) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_isBoundary"));

  if (ptr == nullptr) {
    do_fail("ubrk_isBoundary");
  }

  return ptr(bi, offset);
}
int32_t ubrk_getRuleStatus(UBreakIterator * bi) {
  typedef decltype(&ubrk_getRuleStatus) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_getRuleStatus"));

  if (ptr == nullptr) {
    do_fail("ubrk_getRuleStatus");
  }

  return ptr(bi);
}
int32_t ubrk_getRuleStatusVec(UBreakIterator * bi, int32_t * fillInVec, int32_t capacity, UErrorCode * status) {
  typedef decltype(&ubrk_getRuleStatusVec) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_getRuleStatusVec"));

  if (ptr == nullptr) {
    do_fail("ubrk_getRuleStatusVec");
  }

  return ptr(bi, fillInVec, capacity, status);
}
const char * ubrk_getLocaleByType(const UBreakIterator * bi, ULocDataLocaleType type, UErrorCode * status) {
  typedef decltype(&ubrk_getLocaleByType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_getLocaleByType"));

  if (ptr == nullptr) {
    do_fail("ubrk_getLocaleByType");
  }

  return ptr(bi, type, status);
}
void ubrk_refreshUText(UBreakIterator * bi, UText * text, UErrorCode * status) {
  typedef decltype(&ubrk_refreshUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubrk_refreshUText"));

  if (ptr == nullptr) {
    do_fail("ubrk_refreshUText");
  }

  ptr(bi, text, status);
}
UIDNA * uidna_openUTS46(uint32_t options, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_openUTS46) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_openUTS46"));

  if (ptr == nullptr) {
    do_fail("uidna_openUTS46");
  }

  return ptr(options, pErrorCode);
}
void uidna_close(UIDNA * idna) {
  typedef decltype(&uidna_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_close"));

  if (ptr == nullptr) {
    do_fail("uidna_close");
  }

  ptr(idna);
}
int32_t uidna_labelToASCII(const UIDNA * idna, const UChar * label, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_labelToASCII) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_labelToASCII"));

  if (ptr == nullptr) {
    do_fail("uidna_labelToASCII");
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToUnicode(const UIDNA * idna, const UChar * label, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_labelToUnicode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_labelToUnicode"));

  if (ptr == nullptr) {
    do_fail("uidna_labelToUnicode");
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToASCII(const UIDNA * idna, const UChar * name, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_nameToASCII) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_nameToASCII"));

  if (ptr == nullptr) {
    do_fail("uidna_nameToASCII");
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToUnicode(const UIDNA * idna, const UChar * name, int32_t length, UChar * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_nameToUnicode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_nameToUnicode"));

  if (ptr == nullptr) {
    do_fail("uidna_nameToUnicode");
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToASCII_UTF8(const UIDNA * idna, const char * label, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_labelToASCII_UTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_labelToASCII_UTF8"));

  if (ptr == nullptr) {
    do_fail("uidna_labelToASCII_UTF8");
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_labelToUnicodeUTF8(const UIDNA * idna, const char * label, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_labelToUnicodeUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_labelToUnicodeUTF8"));

  if (ptr == nullptr) {
    do_fail("uidna_labelToUnicodeUTF8");
  }

  return ptr(idna, label, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToASCII_UTF8(const UIDNA * idna, const char * name, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_nameToASCII_UTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_nameToASCII_UTF8"));

  if (ptr == nullptr) {
    do_fail("uidna_nameToASCII_UTF8");
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t uidna_nameToUnicodeUTF8(const UIDNA * idna, const char * name, int32_t length, char * dest, int32_t capacity, UIDNAInfo * pInfo, UErrorCode * pErrorCode) {
  typedef decltype(&uidna_nameToUnicodeUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "uidna_nameToUnicodeUTF8"));

  if (ptr == nullptr) {
    do_fail("uidna_nameToUnicodeUTF8");
  }

  return ptr(idna, name, length, dest, capacity, pInfo, pErrorCode);
}
int32_t ucurr_forLocale(const char * locale, UChar * buff, int32_t buffCapacity, UErrorCode * ec) {
  typedef decltype(&ucurr_forLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_forLocale"));

  if (ptr == nullptr) {
    do_fail("ucurr_forLocale");
  }

  return ptr(locale, buff, buffCapacity, ec);
}
UCurrRegistryKey ucurr_register(const UChar * isoCode, const char * locale, UErrorCode * status) {
  typedef decltype(&ucurr_register) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_register"));

  if (ptr == nullptr) {
    do_fail("ucurr_register");
  }

  return ptr(isoCode, locale, status);
}
UBool ucurr_unregister(UCurrRegistryKey key, UErrorCode * status) {
  typedef decltype(&ucurr_unregister) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_unregister"));

  if (ptr == nullptr) {
    do_fail("ucurr_unregister");
  }

  return ptr(key, status);
}
const UChar * ucurr_getName(const UChar * currency, const char * locale, UCurrNameStyle nameStyle, UBool * isChoiceFormat, int32_t * len, UErrorCode * ec) {
  typedef decltype(&ucurr_getName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getName"));

  if (ptr == nullptr) {
    do_fail("ucurr_getName");
  }

  return ptr(currency, locale, nameStyle, isChoiceFormat, len, ec);
}
const UChar * ucurr_getPluralName(const UChar * currency, const char * locale, UBool * isChoiceFormat, const char * pluralCount, int32_t * len, UErrorCode * ec) {
  typedef decltype(&ucurr_getPluralName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getPluralName"));

  if (ptr == nullptr) {
    do_fail("ucurr_getPluralName");
  }

  return ptr(currency, locale, isChoiceFormat, pluralCount, len, ec);
}
int32_t ucurr_getDefaultFractionDigits(const UChar * currency, UErrorCode * ec) {
  typedef decltype(&ucurr_getDefaultFractionDigits) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getDefaultFractionDigits"));

  if (ptr == nullptr) {
    do_fail("ucurr_getDefaultFractionDigits");
  }

  return ptr(currency, ec);
}
int32_t ucurr_getDefaultFractionDigitsForUsage(const UChar * currency, const UCurrencyUsage usage, UErrorCode * ec) {
  typedef decltype(&ucurr_getDefaultFractionDigitsForUsage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getDefaultFractionDigitsForUsage"));

  if (ptr == nullptr) {
    do_fail("ucurr_getDefaultFractionDigitsForUsage");
  }

  return ptr(currency, usage, ec);
}
double ucurr_getRoundingIncrement(const UChar * currency, UErrorCode * ec) {
  typedef decltype(&ucurr_getRoundingIncrement) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getRoundingIncrement"));

  if (ptr == nullptr) {
    do_fail("ucurr_getRoundingIncrement");
  }

  return ptr(currency, ec);
}
double ucurr_getRoundingIncrementForUsage(const UChar * currency, const UCurrencyUsage usage, UErrorCode * ec) {
  typedef decltype(&ucurr_getRoundingIncrementForUsage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getRoundingIncrementForUsage"));

  if (ptr == nullptr) {
    do_fail("ucurr_getRoundingIncrementForUsage");
  }

  return ptr(currency, usage, ec);
}
UEnumeration * ucurr_openISOCurrencies(uint32_t currType, UErrorCode * pErrorCode) {
  typedef decltype(&ucurr_openISOCurrencies) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_openISOCurrencies"));

  if (ptr == nullptr) {
    do_fail("ucurr_openISOCurrencies");
  }

  return ptr(currType, pErrorCode);
}
UBool ucurr_isAvailable(const UChar * isoCode, UDate from, UDate to, UErrorCode * errorCode) {
  typedef decltype(&ucurr_isAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_isAvailable"));

  if (ptr == nullptr) {
    do_fail("ucurr_isAvailable");
  }

  return ptr(isoCode, from, to, errorCode);
}
int32_t ucurr_countCurrencies(const char * locale, UDate date, UErrorCode * ec) {
  typedef decltype(&ucurr_countCurrencies) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_countCurrencies"));

  if (ptr == nullptr) {
    do_fail("ucurr_countCurrencies");
  }

  return ptr(locale, date, ec);
}
int32_t ucurr_forLocaleAndDate(const char * locale, UDate date, int32_t index, UChar * buff, int32_t buffCapacity, UErrorCode * ec) {
  typedef decltype(&ucurr_forLocaleAndDate) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_forLocaleAndDate"));

  if (ptr == nullptr) {
    do_fail("ucurr_forLocaleAndDate");
  }

  return ptr(locale, date, index, buff, buffCapacity, ec);
}
UEnumeration * ucurr_getKeywordValuesForLocale(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status) {
  typedef decltype(&ucurr_getKeywordValuesForLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getKeywordValuesForLocale"));

  if (ptr == nullptr) {
    do_fail("ucurr_getKeywordValuesForLocale");
  }

  return ptr(key, locale, commonlyUsed, status);
}
int32_t ucurr_getNumericCode(const UChar * currency) {
  typedef decltype(&ucurr_getNumericCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ucurr_getNumericCode"));

  if (ptr == nullptr) {
    do_fail("ucurr_getNumericCode");
  }

  return ptr(currency);
}
UText * utext_close(UText * ut) {
  typedef decltype(&utext_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_close"));

  if (ptr == nullptr) {
    do_fail("utext_close");
  }

  return ptr(ut);
}
UText * utext_openUTF8(UText * ut, const char * s, int64_t length, UErrorCode * status) {
  typedef decltype(&utext_openUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_openUTF8"));

  if (ptr == nullptr) {
    do_fail("utext_openUTF8");
  }

  return ptr(ut, s, length, status);
}
UText * utext_openUChars(UText * ut, const UChar * s, int64_t length, UErrorCode * status) {
  typedef decltype(&utext_openUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_openUChars"));

  if (ptr == nullptr) {
    do_fail("utext_openUChars");
  }

  return ptr(ut, s, length, status);
}
UText * utext_clone(UText * dest, const UText * src, UBool deep, UBool readOnly, UErrorCode * status) {
  typedef decltype(&utext_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_clone"));

  if (ptr == nullptr) {
    do_fail("utext_clone");
  }

  return ptr(dest, src, deep, readOnly, status);
}
UBool utext_equals(const UText * a, const UText * b) {
  typedef decltype(&utext_equals) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_equals"));

  if (ptr == nullptr) {
    do_fail("utext_equals");
  }

  return ptr(a, b);
}
int64_t utext_nativeLength(UText * ut) {
  typedef decltype(&utext_nativeLength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_nativeLength"));

  if (ptr == nullptr) {
    do_fail("utext_nativeLength");
  }

  return ptr(ut);
}
UBool utext_isLengthExpensive(const UText * ut) {
  typedef decltype(&utext_isLengthExpensive) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_isLengthExpensive"));

  if (ptr == nullptr) {
    do_fail("utext_isLengthExpensive");
  }

  return ptr(ut);
}
UChar32 utext_char32At(UText * ut, int64_t nativeIndex) {
  typedef decltype(&utext_char32At) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_char32At"));

  if (ptr == nullptr) {
    do_fail("utext_char32At");
  }

  return ptr(ut, nativeIndex);
}
UChar32 utext_current32(UText * ut) {
  typedef decltype(&utext_current32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_current32"));

  if (ptr == nullptr) {
    do_fail("utext_current32");
  }

  return ptr(ut);
}
UChar32 utext_next32(UText * ut) {
  typedef decltype(&utext_next32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_next32"));

  if (ptr == nullptr) {
    do_fail("utext_next32");
  }

  return ptr(ut);
}
UChar32 utext_previous32(UText * ut) {
  typedef decltype(&utext_previous32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_previous32"));

  if (ptr == nullptr) {
    do_fail("utext_previous32");
  }

  return ptr(ut);
}
UChar32 utext_next32From(UText * ut, int64_t nativeIndex) {
  typedef decltype(&utext_next32From) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_next32From"));

  if (ptr == nullptr) {
    do_fail("utext_next32From");
  }

  return ptr(ut, nativeIndex);
}
UChar32 utext_previous32From(UText * ut, int64_t nativeIndex) {
  typedef decltype(&utext_previous32From) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_previous32From"));

  if (ptr == nullptr) {
    do_fail("utext_previous32From");
  }

  return ptr(ut, nativeIndex);
}
int64_t utext_getNativeIndex(const UText * ut) {
  typedef decltype(&utext_getNativeIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_getNativeIndex"));

  if (ptr == nullptr) {
    do_fail("utext_getNativeIndex");
  }

  return ptr(ut);
}
void utext_setNativeIndex(UText * ut, int64_t nativeIndex) {
  typedef decltype(&utext_setNativeIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_setNativeIndex"));

  if (ptr == nullptr) {
    do_fail("utext_setNativeIndex");
  }

  ptr(ut, nativeIndex);
}
UBool utext_moveIndex32(UText * ut, int32_t delta) {
  typedef decltype(&utext_moveIndex32) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_moveIndex32"));

  if (ptr == nullptr) {
    do_fail("utext_moveIndex32");
  }

  return ptr(ut, delta);
}
int64_t utext_getPreviousNativeIndex(UText * ut) {
  typedef decltype(&utext_getPreviousNativeIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_getPreviousNativeIndex"));

  if (ptr == nullptr) {
    do_fail("utext_getPreviousNativeIndex");
  }

  return ptr(ut);
}
int32_t utext_extract(UText * ut, int64_t nativeStart, int64_t nativeLimit, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&utext_extract) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_extract"));

  if (ptr == nullptr) {
    do_fail("utext_extract");
  }

  return ptr(ut, nativeStart, nativeLimit, dest, destCapacity, status);
}
UBool utext_isWritable(const UText * ut) {
  typedef decltype(&utext_isWritable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_isWritable"));

  if (ptr == nullptr) {
    do_fail("utext_isWritable");
  }

  return ptr(ut);
}
UBool utext_hasMetaData(const UText * ut) {
  typedef decltype(&utext_hasMetaData) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_hasMetaData"));

  if (ptr == nullptr) {
    do_fail("utext_hasMetaData");
  }

  return ptr(ut);
}
int32_t utext_replace(UText * ut, int64_t nativeStart, int64_t nativeLimit, const UChar * replacementText, int32_t replacementLength, UErrorCode * status) {
  typedef decltype(&utext_replace) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_replace"));

  if (ptr == nullptr) {
    do_fail("utext_replace");
  }

  return ptr(ut, nativeStart, nativeLimit, replacementText, replacementLength, status);
}
void utext_copy(UText * ut, int64_t nativeStart, int64_t nativeLimit, int64_t destIndex, UBool move, UErrorCode * status) {
  typedef decltype(&utext_copy) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_copy"));

  if (ptr == nullptr) {
    do_fail("utext_copy");
  }

  ptr(ut, nativeStart, nativeLimit, destIndex, move, status);
}
void utext_freeze(UText * ut) {
  typedef decltype(&utext_freeze) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_freeze"));

  if (ptr == nullptr) {
    do_fail("utext_freeze");
  }

  ptr(ut);
}
UText * utext_setup(UText * ut, int32_t extraSpace, UErrorCode * status) {
  typedef decltype(&utext_setup) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "utext_setup"));

  if (ptr == nullptr) {
    do_fail("utext_setup");
  }

  return ptr(ut, extraSpace, status);
}
const UNormalizer2 * unorm2_getNFCInstance(UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getNFCInstance) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getNFCInstance"));

  if (ptr == nullptr) {
    do_fail("unorm2_getNFCInstance");
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFDInstance(UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getNFDInstance) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getNFDInstance"));

  if (ptr == nullptr) {
    do_fail("unorm2_getNFDInstance");
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFKCInstance(UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getNFKCInstance) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getNFKCInstance"));

  if (ptr == nullptr) {
    do_fail("unorm2_getNFKCInstance");
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFKDInstance(UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getNFKDInstance) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getNFKDInstance"));

  if (ptr == nullptr) {
    do_fail("unorm2_getNFKDInstance");
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getNFKCCasefoldInstance(UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getNFKCCasefoldInstance) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getNFKCCasefoldInstance"));

  if (ptr == nullptr) {
    do_fail("unorm2_getNFKCCasefoldInstance");
  }

  return ptr(pErrorCode);
}
const UNormalizer2 * unorm2_getInstance(const char * packageName, const char * name, UNormalization2Mode mode, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getInstance) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getInstance"));

  if (ptr == nullptr) {
    do_fail("unorm2_getInstance");
  }

  return ptr(packageName, name, mode, pErrorCode);
}
UNormalizer2 * unorm2_openFiltered(const UNormalizer2 * norm2, const USet * filterSet, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_openFiltered) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_openFiltered"));

  if (ptr == nullptr) {
    do_fail("unorm2_openFiltered");
  }

  return ptr(norm2, filterSet, pErrorCode);
}
void unorm2_close(UNormalizer2 * norm2) {
  typedef decltype(&unorm2_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_close"));

  if (ptr == nullptr) {
    do_fail("unorm2_close");
  }

  ptr(norm2);
}
int32_t unorm2_normalize(const UNormalizer2 * norm2, const UChar * src, int32_t length, UChar * dest, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_normalize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_normalize"));

  if (ptr == nullptr) {
    do_fail("unorm2_normalize");
  }

  return ptr(norm2, src, length, dest, capacity, pErrorCode);
}
int32_t unorm2_normalizeSecondAndAppend(const UNormalizer2 * norm2, UChar * first, int32_t firstLength, int32_t firstCapacity, const UChar * second, int32_t secondLength, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_normalizeSecondAndAppend) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_normalizeSecondAndAppend"));

  if (ptr == nullptr) {
    do_fail("unorm2_normalizeSecondAndAppend");
  }

  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}
int32_t unorm2_append(const UNormalizer2 * norm2, UChar * first, int32_t firstLength, int32_t firstCapacity, const UChar * second, int32_t secondLength, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_append) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_append"));

  if (ptr == nullptr) {
    do_fail("unorm2_append");
  }

  return ptr(norm2, first, firstLength, firstCapacity, second, secondLength, pErrorCode);
}
int32_t unorm2_getDecomposition(const UNormalizer2 * norm2, UChar32 c, UChar * decomposition, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getDecomposition) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getDecomposition"));

  if (ptr == nullptr) {
    do_fail("unorm2_getDecomposition");
  }

  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}
int32_t unorm2_getRawDecomposition(const UNormalizer2 * norm2, UChar32 c, UChar * decomposition, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_getRawDecomposition) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getRawDecomposition"));

  if (ptr == nullptr) {
    do_fail("unorm2_getRawDecomposition");
  }

  return ptr(norm2, c, decomposition, capacity, pErrorCode);
}
UChar32 unorm2_composePair(const UNormalizer2 * norm2, UChar32 a, UChar32 b) {
  typedef decltype(&unorm2_composePair) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_composePair"));

  if (ptr == nullptr) {
    do_fail("unorm2_composePair");
  }

  return ptr(norm2, a, b);
}
uint8_t unorm2_getCombiningClass(const UNormalizer2 * norm2, UChar32 c) {
  typedef decltype(&unorm2_getCombiningClass) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_getCombiningClass"));

  if (ptr == nullptr) {
    do_fail("unorm2_getCombiningClass");
  }

  return ptr(norm2, c);
}
UBool unorm2_isNormalized(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_isNormalized) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_isNormalized"));

  if (ptr == nullptr) {
    do_fail("unorm2_isNormalized");
  }

  return ptr(norm2, s, length, pErrorCode);
}
UNormalizationCheckResult unorm2_quickCheck(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_quickCheck) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_quickCheck"));

  if (ptr == nullptr) {
    do_fail("unorm2_quickCheck");
  }

  return ptr(norm2, s, length, pErrorCode);
}
int32_t unorm2_spanQuickCheckYes(const UNormalizer2 * norm2, const UChar * s, int32_t length, UErrorCode * pErrorCode) {
  typedef decltype(&unorm2_spanQuickCheckYes) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_spanQuickCheckYes"));

  if (ptr == nullptr) {
    do_fail("unorm2_spanQuickCheckYes");
  }

  return ptr(norm2, s, length, pErrorCode);
}
UBool unorm2_hasBoundaryBefore(const UNormalizer2 * norm2, UChar32 c) {
  typedef decltype(&unorm2_hasBoundaryBefore) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_hasBoundaryBefore"));

  if (ptr == nullptr) {
    do_fail("unorm2_hasBoundaryBefore");
  }

  return ptr(norm2, c);
}
UBool unorm2_hasBoundaryAfter(const UNormalizer2 * norm2, UChar32 c) {
  typedef decltype(&unorm2_hasBoundaryAfter) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_hasBoundaryAfter"));

  if (ptr == nullptr) {
    do_fail("unorm2_hasBoundaryAfter");
  }

  return ptr(norm2, c);
}
UBool unorm2_isInert(const UNormalizer2 * norm2, UChar32 c) {
  typedef decltype(&unorm2_isInert) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm2_isInert"));

  if (ptr == nullptr) {
    do_fail("unorm2_isInert");
  }

  return ptr(norm2, c);
}
int32_t unorm_compare(const UChar * s1, int32_t length1, const UChar * s2, int32_t length2, uint32_t options, UErrorCode * pErrorCode) {
  typedef decltype(&unorm_compare) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "unorm_compare"));

  if (ptr == nullptr) {
    do_fail("unorm_compare");
  }

  return ptr(s1, length1, s2, length2, options, pErrorCode);
}
UBiDi * ubidi_open() {
  typedef decltype(&ubidi_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_open"));

  if (ptr == nullptr) {
    do_fail("ubidi_open");
  }

  return ptr();
}
UBiDi * ubidi_openSized(int32_t maxLength, int32_t maxRunCount, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_openSized) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_openSized"));

  if (ptr == nullptr) {
    do_fail("ubidi_openSized");
  }

  return ptr(maxLength, maxRunCount, pErrorCode);
}
void ubidi_close(UBiDi * pBiDi) {
  typedef decltype(&ubidi_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_close"));

  if (ptr == nullptr) {
    do_fail("ubidi_close");
  }

  ptr(pBiDi);
}
void ubidi_setInverse(UBiDi * pBiDi, UBool isInverse) {
  typedef decltype(&ubidi_setInverse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_setInverse"));

  if (ptr == nullptr) {
    do_fail("ubidi_setInverse");
  }

  ptr(pBiDi, isInverse);
}
UBool ubidi_isInverse(UBiDi * pBiDi) {
  typedef decltype(&ubidi_isInverse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_isInverse"));

  if (ptr == nullptr) {
    do_fail("ubidi_isInverse");
  }

  return ptr(pBiDi);
}
void ubidi_orderParagraphsLTR(UBiDi * pBiDi, UBool orderParagraphsLTR) {
  typedef decltype(&ubidi_orderParagraphsLTR) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_orderParagraphsLTR"));

  if (ptr == nullptr) {
    do_fail("ubidi_orderParagraphsLTR");
  }

  ptr(pBiDi, orderParagraphsLTR);
}
UBool ubidi_isOrderParagraphsLTR(UBiDi * pBiDi) {
  typedef decltype(&ubidi_isOrderParagraphsLTR) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_isOrderParagraphsLTR"));

  if (ptr == nullptr) {
    do_fail("ubidi_isOrderParagraphsLTR");
  }

  return ptr(pBiDi);
}
void ubidi_setReorderingMode(UBiDi * pBiDi, UBiDiReorderingMode reorderingMode) {
  typedef decltype(&ubidi_setReorderingMode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_setReorderingMode"));

  if (ptr == nullptr) {
    do_fail("ubidi_setReorderingMode");
  }

  ptr(pBiDi, reorderingMode);
}
UBiDiReorderingMode ubidi_getReorderingMode(UBiDi * pBiDi) {
  typedef decltype(&ubidi_getReorderingMode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getReorderingMode"));

  if (ptr == nullptr) {
    do_fail("ubidi_getReorderingMode");
  }

  return ptr(pBiDi);
}
void ubidi_setReorderingOptions(UBiDi * pBiDi, uint32_t reorderingOptions) {
  typedef decltype(&ubidi_setReorderingOptions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_setReorderingOptions"));

  if (ptr == nullptr) {
    do_fail("ubidi_setReorderingOptions");
  }

  ptr(pBiDi, reorderingOptions);
}
uint32_t ubidi_getReorderingOptions(UBiDi * pBiDi) {
  typedef decltype(&ubidi_getReorderingOptions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getReorderingOptions"));

  if (ptr == nullptr) {
    do_fail("ubidi_getReorderingOptions");
  }

  return ptr(pBiDi);
}
void ubidi_setContext(UBiDi * pBiDi, const UChar * prologue, int32_t proLength, const UChar * epilogue, int32_t epiLength, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_setContext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_setContext"));

  if (ptr == nullptr) {
    do_fail("ubidi_setContext");
  }

  ptr(pBiDi, prologue, proLength, epilogue, epiLength, pErrorCode);
}
void ubidi_setPara(UBiDi * pBiDi, const UChar * text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel * embeddingLevels, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_setPara) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_setPara"));

  if (ptr == nullptr) {
    do_fail("ubidi_setPara");
  }

  ptr(pBiDi, text, length, paraLevel, embeddingLevels, pErrorCode);
}
void ubidi_setLine(const UBiDi * pParaBiDi, int32_t start, int32_t limit, UBiDi * pLineBiDi, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_setLine) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_setLine"));

  if (ptr == nullptr) {
    do_fail("ubidi_setLine");
  }

  ptr(pParaBiDi, start, limit, pLineBiDi, pErrorCode);
}
UBiDiDirection ubidi_getDirection(const UBiDi * pBiDi) {
  typedef decltype(&ubidi_getDirection) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getDirection"));

  if (ptr == nullptr) {
    do_fail("ubidi_getDirection");
  }

  return ptr(pBiDi);
}
UBiDiDirection ubidi_getBaseDirection(const UChar * text, int32_t length) {
  typedef decltype(&ubidi_getBaseDirection) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getBaseDirection"));

  if (ptr == nullptr) {
    do_fail("ubidi_getBaseDirection");
  }

  return ptr(text, length);
}
const UChar * ubidi_getText(const UBiDi * pBiDi) {
  typedef decltype(&ubidi_getText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getText"));

  if (ptr == nullptr) {
    do_fail("ubidi_getText");
  }

  return ptr(pBiDi);
}
int32_t ubidi_getLength(const UBiDi * pBiDi) {
  typedef decltype(&ubidi_getLength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getLength"));

  if (ptr == nullptr) {
    do_fail("ubidi_getLength");
  }

  return ptr(pBiDi);
}
UBiDiLevel ubidi_getParaLevel(const UBiDi * pBiDi) {
  typedef decltype(&ubidi_getParaLevel) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getParaLevel"));

  if (ptr == nullptr) {
    do_fail("ubidi_getParaLevel");
  }

  return ptr(pBiDi);
}
int32_t ubidi_countParagraphs(UBiDi * pBiDi) {
  typedef decltype(&ubidi_countParagraphs) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_countParagraphs"));

  if (ptr == nullptr) {
    do_fail("ubidi_countParagraphs");
  }

  return ptr(pBiDi);
}
int32_t ubidi_getParagraph(const UBiDi * pBiDi, int32_t charIndex, int32_t * pParaStart, int32_t * pParaLimit, UBiDiLevel * pParaLevel, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_getParagraph) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getParagraph"));

  if (ptr == nullptr) {
    do_fail("ubidi_getParagraph");
  }

  return ptr(pBiDi, charIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}
void ubidi_getParagraphByIndex(const UBiDi * pBiDi, int32_t paraIndex, int32_t * pParaStart, int32_t * pParaLimit, UBiDiLevel * pParaLevel, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_getParagraphByIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getParagraphByIndex"));

  if (ptr == nullptr) {
    do_fail("ubidi_getParagraphByIndex");
  }

  ptr(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
}
UBiDiLevel ubidi_getLevelAt(const UBiDi * pBiDi, int32_t charIndex) {
  typedef decltype(&ubidi_getLevelAt) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getLevelAt"));

  if (ptr == nullptr) {
    do_fail("ubidi_getLevelAt");
  }

  return ptr(pBiDi, charIndex);
}
const UBiDiLevel * ubidi_getLevels(UBiDi * pBiDi, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_getLevels) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getLevels"));

  if (ptr == nullptr) {
    do_fail("ubidi_getLevels");
  }

  return ptr(pBiDi, pErrorCode);
}
void ubidi_getLogicalRun(const UBiDi * pBiDi, int32_t logicalPosition, int32_t * pLogicalLimit, UBiDiLevel * pLevel) {
  typedef decltype(&ubidi_getLogicalRun) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getLogicalRun"));

  if (ptr == nullptr) {
    do_fail("ubidi_getLogicalRun");
  }

  ptr(pBiDi, logicalPosition, pLogicalLimit, pLevel);
}
int32_t ubidi_countRuns(UBiDi * pBiDi, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_countRuns) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_countRuns"));

  if (ptr == nullptr) {
    do_fail("ubidi_countRuns");
  }

  return ptr(pBiDi, pErrorCode);
}
UBiDiDirection ubidi_getVisualRun(UBiDi * pBiDi, int32_t runIndex, int32_t * pLogicalStart, int32_t * pLength) {
  typedef decltype(&ubidi_getVisualRun) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getVisualRun"));

  if (ptr == nullptr) {
    do_fail("ubidi_getVisualRun");
  }

  return ptr(pBiDi, runIndex, pLogicalStart, pLength);
}
int32_t ubidi_getVisualIndex(UBiDi * pBiDi, int32_t logicalIndex, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_getVisualIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getVisualIndex"));

  if (ptr == nullptr) {
    do_fail("ubidi_getVisualIndex");
  }

  return ptr(pBiDi, logicalIndex, pErrorCode);
}
int32_t ubidi_getLogicalIndex(UBiDi * pBiDi, int32_t visualIndex, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_getLogicalIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getLogicalIndex"));

  if (ptr == nullptr) {
    do_fail("ubidi_getLogicalIndex");
  }

  return ptr(pBiDi, visualIndex, pErrorCode);
}
void ubidi_getLogicalMap(UBiDi * pBiDi, int32_t * indexMap, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_getLogicalMap) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getLogicalMap"));

  if (ptr == nullptr) {
    do_fail("ubidi_getLogicalMap");
  }

  ptr(pBiDi, indexMap, pErrorCode);
}
void ubidi_getVisualMap(UBiDi * pBiDi, int32_t * indexMap, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_getVisualMap) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getVisualMap"));

  if (ptr == nullptr) {
    do_fail("ubidi_getVisualMap");
  }

  ptr(pBiDi, indexMap, pErrorCode);
}
void ubidi_reorderLogical(const UBiDiLevel * levels, int32_t length, int32_t * indexMap) {
  typedef decltype(&ubidi_reorderLogical) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_reorderLogical"));

  if (ptr == nullptr) {
    do_fail("ubidi_reorderLogical");
  }

  ptr(levels, length, indexMap);
}
void ubidi_reorderVisual(const UBiDiLevel * levels, int32_t length, int32_t * indexMap) {
  typedef decltype(&ubidi_reorderVisual) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_reorderVisual"));

  if (ptr == nullptr) {
    do_fail("ubidi_reorderVisual");
  }

  ptr(levels, length, indexMap);
}
void ubidi_invertMap(const int32_t * srcMap, int32_t * destMap, int32_t length) {
  typedef decltype(&ubidi_invertMap) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_invertMap"));

  if (ptr == nullptr) {
    do_fail("ubidi_invertMap");
  }

  ptr(srcMap, destMap, length);
}
int32_t ubidi_getProcessedLength(const UBiDi * pBiDi) {
  typedef decltype(&ubidi_getProcessedLength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getProcessedLength"));

  if (ptr == nullptr) {
    do_fail("ubidi_getProcessedLength");
  }

  return ptr(pBiDi);
}
int32_t ubidi_getResultLength(const UBiDi * pBiDi) {
  typedef decltype(&ubidi_getResultLength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getResultLength"));

  if (ptr == nullptr) {
    do_fail("ubidi_getResultLength");
  }

  return ptr(pBiDi);
}
UCharDirection ubidi_getCustomizedClass(UBiDi * pBiDi, UChar32 c) {
  typedef decltype(&ubidi_getCustomizedClass) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getCustomizedClass"));

  if (ptr == nullptr) {
    do_fail("ubidi_getCustomizedClass");
  }

  return ptr(pBiDi, c);
}
void ubidi_setClassCallback(UBiDi * pBiDi, UBiDiClassCallback * newFn, const void * newContext, UBiDiClassCallback ** oldFn, const void ** oldContext, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_setClassCallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_setClassCallback"));

  if (ptr == nullptr) {
    do_fail("ubidi_setClassCallback");
  }

  ptr(pBiDi, newFn, newContext, oldFn, oldContext, pErrorCode);
}
void ubidi_getClassCallback(UBiDi * pBiDi, UBiDiClassCallback ** fn, const void ** context) {
  typedef decltype(&ubidi_getClassCallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_getClassCallback"));

  if (ptr == nullptr) {
    do_fail("ubidi_getClassCallback");
  }

  ptr(pBiDi, fn, context);
}
int32_t ubidi_writeReordered(UBiDi * pBiDi, UChar * dest, int32_t destSize, uint16_t options, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_writeReordered) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_writeReordered"));

  if (ptr == nullptr) {
    do_fail("ubidi_writeReordered");
  }

  return ptr(pBiDi, dest, destSize, options, pErrorCode);
}
int32_t ubidi_writeReverse(const UChar * src, int32_t srcLength, UChar * dest, int32_t destSize, uint16_t options, UErrorCode * pErrorCode) {
  typedef decltype(&ubidi_writeReverse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "ubidi_writeReverse"));

  if (ptr == nullptr) {
    do_fail("ubidi_writeReverse");
  }

  return ptr(src, srcLength, dest, destSize, options, pErrorCode);
}
UStringPrepProfile * usprep_open(const char * path, const char * fileName, UErrorCode * status) {
  typedef decltype(&usprep_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "usprep_open"));

  if (ptr == nullptr) {
    do_fail("usprep_open");
  }

  return ptr(path, fileName, status);
}
UStringPrepProfile * usprep_openByType(UStringPrepProfileType type, UErrorCode * status) {
  typedef decltype(&usprep_openByType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "usprep_openByType"));

  if (ptr == nullptr) {
    do_fail("usprep_openByType");
  }

  return ptr(type, status);
}
void usprep_close(UStringPrepProfile * profile) {
  typedef decltype(&usprep_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "usprep_close"));

  if (ptr == nullptr) {
    do_fail("usprep_close");
  }

  ptr(profile);
}
int32_t usprep_prepare(const UStringPrepProfile * prep, const UChar * src, int32_t srcLength, UChar * dest, int32_t destCapacity, int32_t options, UParseError * parseError, UErrorCode * status) {
  typedef decltype(&usprep_prepare) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_common, "usprep_prepare"));

  if (ptr == nullptr) {
    do_fail("usprep_prepare");
  }

  return ptr(prep, src, srcLength, dest, destCapacity, options, parseError, status);
}
UCollator * ucol_open(const char * loc, UErrorCode * status) {
  typedef decltype(&ucol_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_open"));

  if (ptr == nullptr) {
    do_fail("ucol_open");
  }

  return ptr(loc, status);
}
UCollator * ucol_openRules(const UChar * rules, int32_t rulesLength, UColAttributeValue normalizationMode, UCollationStrength strength, UParseError * parseError, UErrorCode * status) {
  typedef decltype(&ucol_openRules) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_openRules"));

  if (ptr == nullptr) {
    do_fail("ucol_openRules");
  }

  return ptr(rules, rulesLength, normalizationMode, strength, parseError, status);
}
void ucol_getContractionsAndExpansions(const UCollator * coll, USet * contractions, USet * expansions, UBool addPrefixes, UErrorCode * status) {
  typedef decltype(&ucol_getContractionsAndExpansions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getContractionsAndExpansions"));

  if (ptr == nullptr) {
    do_fail("ucol_getContractionsAndExpansions");
  }

  ptr(coll, contractions, expansions, addPrefixes, status);
}
void ucol_close(UCollator * coll) {
  typedef decltype(&ucol_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_close"));

  if (ptr == nullptr) {
    do_fail("ucol_close");
  }

  ptr(coll);
}
UCollationResult ucol_strcoll(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  typedef decltype(&ucol_strcoll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_strcoll"));

  if (ptr == nullptr) {
    do_fail("ucol_strcoll");
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UCollationResult ucol_strcollUTF8(const UCollator * coll, const char * source, int32_t sourceLength, const char * target, int32_t targetLength, UErrorCode * status) {
  typedef decltype(&ucol_strcollUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_strcollUTF8"));

  if (ptr == nullptr) {
    do_fail("ucol_strcollUTF8");
  }

  return ptr(coll, source, sourceLength, target, targetLength, status);
}
UBool ucol_greater(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  typedef decltype(&ucol_greater) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_greater"));

  if (ptr == nullptr) {
    do_fail("ucol_greater");
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UBool ucol_greaterOrEqual(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  typedef decltype(&ucol_greaterOrEqual) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_greaterOrEqual"));

  if (ptr == nullptr) {
    do_fail("ucol_greaterOrEqual");
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UBool ucol_equal(const UCollator * coll, const UChar * source, int32_t sourceLength, const UChar * target, int32_t targetLength) {
  typedef decltype(&ucol_equal) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_equal"));

  if (ptr == nullptr) {
    do_fail("ucol_equal");
  }

  return ptr(coll, source, sourceLength, target, targetLength);
}
UCollationResult ucol_strcollIter(const UCollator * coll, UCharIterator * sIter, UCharIterator * tIter, UErrorCode * status) {
  typedef decltype(&ucol_strcollIter) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_strcollIter"));

  if (ptr == nullptr) {
    do_fail("ucol_strcollIter");
  }

  return ptr(coll, sIter, tIter, status);
}
UCollationStrength ucol_getStrength(const UCollator * coll) {
  typedef decltype(&ucol_getStrength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getStrength"));

  if (ptr == nullptr) {
    do_fail("ucol_getStrength");
  }

  return ptr(coll);
}
void ucol_setStrength(UCollator * coll, UCollationStrength strength) {
  typedef decltype(&ucol_setStrength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_setStrength"));

  if (ptr == nullptr) {
    do_fail("ucol_setStrength");
  }

  ptr(coll, strength);
}
int32_t ucol_getReorderCodes(const UCollator * coll, int32_t * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  typedef decltype(&ucol_getReorderCodes) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getReorderCodes"));

  if (ptr == nullptr) {
    do_fail("ucol_getReorderCodes");
  }

  return ptr(coll, dest, destCapacity, pErrorCode);
}
void ucol_setReorderCodes(UCollator * coll, const int32_t * reorderCodes, int32_t reorderCodesLength, UErrorCode * pErrorCode) {
  typedef decltype(&ucol_setReorderCodes) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_setReorderCodes"));

  if (ptr == nullptr) {
    do_fail("ucol_setReorderCodes");
  }

  ptr(coll, reorderCodes, reorderCodesLength, pErrorCode);
}
int32_t ucol_getEquivalentReorderCodes(int32_t reorderCode, int32_t * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  typedef decltype(&ucol_getEquivalentReorderCodes) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getEquivalentReorderCodes"));

  if (ptr == nullptr) {
    do_fail("ucol_getEquivalentReorderCodes");
  }

  return ptr(reorderCode, dest, destCapacity, pErrorCode);
}
int32_t ucol_getDisplayName(const char * objLoc, const char * dispLoc, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&ucol_getDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getDisplayName"));

  if (ptr == nullptr) {
    do_fail("ucol_getDisplayName");
  }

  return ptr(objLoc, dispLoc, result, resultLength, status);
}
const char * ucol_getAvailable(int32_t localeIndex) {
  typedef decltype(&ucol_getAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getAvailable"));

  if (ptr == nullptr) {
    do_fail("ucol_getAvailable");
  }

  return ptr(localeIndex);
}
int32_t ucol_countAvailable() {
  typedef decltype(&ucol_countAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_countAvailable"));

  if (ptr == nullptr) {
    do_fail("ucol_countAvailable");
  }

  return ptr();
}
UEnumeration * ucol_openAvailableLocales(UErrorCode * status) {
  typedef decltype(&ucol_openAvailableLocales) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_openAvailableLocales"));

  if (ptr == nullptr) {
    do_fail("ucol_openAvailableLocales");
  }

  return ptr(status);
}
UEnumeration * ucol_getKeywords(UErrorCode * status) {
  typedef decltype(&ucol_getKeywords) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getKeywords"));

  if (ptr == nullptr) {
    do_fail("ucol_getKeywords");
  }

  return ptr(status);
}
UEnumeration * ucol_getKeywordValues(const char * keyword, UErrorCode * status) {
  typedef decltype(&ucol_getKeywordValues) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getKeywordValues"));

  if (ptr == nullptr) {
    do_fail("ucol_getKeywordValues");
  }

  return ptr(keyword, status);
}
UEnumeration * ucol_getKeywordValuesForLocale(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status) {
  typedef decltype(&ucol_getKeywordValuesForLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getKeywordValuesForLocale"));

  if (ptr == nullptr) {
    do_fail("ucol_getKeywordValuesForLocale");
  }

  return ptr(key, locale, commonlyUsed, status);
}
int32_t ucol_getFunctionalEquivalent(char * result, int32_t resultCapacity, const char * keyword, const char * locale, UBool * isAvailable, UErrorCode * status) {
  typedef decltype(&ucol_getFunctionalEquivalent) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getFunctionalEquivalent"));

  if (ptr == nullptr) {
    do_fail("ucol_getFunctionalEquivalent");
  }

  return ptr(result, resultCapacity, keyword, locale, isAvailable, status);
}
const UChar * ucol_getRules(const UCollator * coll, int32_t * length) {
  typedef decltype(&ucol_getRules) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getRules"));

  if (ptr == nullptr) {
    do_fail("ucol_getRules");
  }

  return ptr(coll, length);
}
int32_t ucol_getSortKey(const UCollator * coll, const UChar * source, int32_t sourceLength, uint8_t * result, int32_t resultLength) {
  typedef decltype(&ucol_getSortKey) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getSortKey"));

  if (ptr == nullptr) {
    do_fail("ucol_getSortKey");
  }

  return ptr(coll, source, sourceLength, result, resultLength);
}
int32_t ucol_nextSortKeyPart(const UCollator * coll, UCharIterator * iter, uint32_t  state[2], uint8_t * dest, int32_t count, UErrorCode * status) {
  typedef decltype(&ucol_nextSortKeyPart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_nextSortKeyPart"));

  if (ptr == nullptr) {
    do_fail("ucol_nextSortKeyPart");
  }

  return ptr(coll, iter, state, dest, count, status);
}
int32_t ucol_getBound(const uint8_t * source, int32_t sourceLength, UColBoundMode boundType, uint32_t noOfLevels, uint8_t * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&ucol_getBound) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getBound"));

  if (ptr == nullptr) {
    do_fail("ucol_getBound");
  }

  return ptr(source, sourceLength, boundType, noOfLevels, result, resultLength, status);
}
void ucol_getVersion(const UCollator * coll, UVersionInfo info) {
  typedef decltype(&ucol_getVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getVersion"));

  if (ptr == nullptr) {
    do_fail("ucol_getVersion");
  }

  ptr(coll, info);
}
void ucol_getUCAVersion(const UCollator * coll, UVersionInfo info) {
  typedef decltype(&ucol_getUCAVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getUCAVersion"));

  if (ptr == nullptr) {
    do_fail("ucol_getUCAVersion");
  }

  ptr(coll, info);
}
int32_t ucol_mergeSortkeys(const uint8_t * src1, int32_t src1Length, const uint8_t * src2, int32_t src2Length, uint8_t * dest, int32_t destCapacity) {
  typedef decltype(&ucol_mergeSortkeys) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_mergeSortkeys"));

  if (ptr == nullptr) {
    do_fail("ucol_mergeSortkeys");
  }

  return ptr(src1, src1Length, src2, src2Length, dest, destCapacity);
}
void ucol_setAttribute(UCollator * coll, UColAttribute attr, UColAttributeValue value, UErrorCode * status) {
  typedef decltype(&ucol_setAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_setAttribute"));

  if (ptr == nullptr) {
    do_fail("ucol_setAttribute");
  }

  ptr(coll, attr, value, status);
}
UColAttributeValue ucol_getAttribute(const UCollator * coll, UColAttribute attr, UErrorCode * status) {
  typedef decltype(&ucol_getAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getAttribute"));

  if (ptr == nullptr) {
    do_fail("ucol_getAttribute");
  }

  return ptr(coll, attr, status);
}
void ucol_setMaxVariable(UCollator * coll, UColReorderCode group, UErrorCode * pErrorCode) {
  typedef decltype(&ucol_setMaxVariable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_setMaxVariable"));

  if (ptr == nullptr) {
    do_fail("ucol_setMaxVariable");
  }

  ptr(coll, group, pErrorCode);
}
UColReorderCode ucol_getMaxVariable(const UCollator * coll) {
  typedef decltype(&ucol_getMaxVariable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getMaxVariable"));

  if (ptr == nullptr) {
    do_fail("ucol_getMaxVariable");
  }

  return ptr(coll);
}
uint32_t ucol_getVariableTop(const UCollator * coll, UErrorCode * status) {
  typedef decltype(&ucol_getVariableTop) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getVariableTop"));

  if (ptr == nullptr) {
    do_fail("ucol_getVariableTop");
  }

  return ptr(coll, status);
}
UCollator * ucol_safeClone(const UCollator * coll, void * stackBuffer, int32_t * pBufferSize, UErrorCode * status) {
  typedef decltype(&ucol_safeClone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_safeClone"));

  if (ptr == nullptr) {
    do_fail("ucol_safeClone");
  }

  return ptr(coll, stackBuffer, pBufferSize, status);
}
int32_t ucol_getRulesEx(const UCollator * coll, UColRuleOption delta, UChar * buffer, int32_t bufferLen) {
  typedef decltype(&ucol_getRulesEx) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getRulesEx"));

  if (ptr == nullptr) {
    do_fail("ucol_getRulesEx");
  }

  return ptr(coll, delta, buffer, bufferLen);
}
const char * ucol_getLocaleByType(const UCollator * coll, ULocDataLocaleType type, UErrorCode * status) {
  typedef decltype(&ucol_getLocaleByType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getLocaleByType"));

  if (ptr == nullptr) {
    do_fail("ucol_getLocaleByType");
  }

  return ptr(coll, type, status);
}
USet * ucol_getTailoredSet(const UCollator * coll, UErrorCode * status) {
  typedef decltype(&ucol_getTailoredSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getTailoredSet"));

  if (ptr == nullptr) {
    do_fail("ucol_getTailoredSet");
  }

  return ptr(coll, status);
}
int32_t ucol_cloneBinary(const UCollator * coll, uint8_t * buffer, int32_t capacity, UErrorCode * status) {
  typedef decltype(&ucol_cloneBinary) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_cloneBinary"));

  if (ptr == nullptr) {
    do_fail("ucol_cloneBinary");
  }

  return ptr(coll, buffer, capacity, status);
}
UCollator * ucol_openBinary(const uint8_t * bin, int32_t length, const UCollator * base, UErrorCode * status) {
  typedef decltype(&ucol_openBinary) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_openBinary"));

  if (ptr == nullptr) {
    do_fail("ucol_openBinary");
  }

  return ptr(bin, length, base, status);
}
UFormattable * ufmt_open(UErrorCode * status) {
  typedef decltype(&ufmt_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_open"));

  if (ptr == nullptr) {
    do_fail("ufmt_open");
  }

  return ptr(status);
}
void ufmt_close(UFormattable * fmt) {
  typedef decltype(&ufmt_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_close"));

  if (ptr == nullptr) {
    do_fail("ufmt_close");
  }

  ptr(fmt);
}
UFormattableType ufmt_getType(const UFormattable * fmt, UErrorCode * status) {
  typedef decltype(&ufmt_getType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getType"));

  if (ptr == nullptr) {
    do_fail("ufmt_getType");
  }

  return ptr(fmt, status);
}
UBool ufmt_isNumeric(const UFormattable * fmt) {
  typedef decltype(&ufmt_isNumeric) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_isNumeric"));

  if (ptr == nullptr) {
    do_fail("ufmt_isNumeric");
  }

  return ptr(fmt);
}
UDate ufmt_getDate(const UFormattable * fmt, UErrorCode * status) {
  typedef decltype(&ufmt_getDate) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getDate"));

  if (ptr == nullptr) {
    do_fail("ufmt_getDate");
  }

  return ptr(fmt, status);
}
double ufmt_getDouble(UFormattable * fmt, UErrorCode * status) {
  typedef decltype(&ufmt_getDouble) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getDouble"));

  if (ptr == nullptr) {
    do_fail("ufmt_getDouble");
  }

  return ptr(fmt, status);
}
int32_t ufmt_getLong(UFormattable * fmt, UErrorCode * status) {
  typedef decltype(&ufmt_getLong) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getLong"));

  if (ptr == nullptr) {
    do_fail("ufmt_getLong");
  }

  return ptr(fmt, status);
}
int64_t ufmt_getInt64(UFormattable * fmt, UErrorCode * status) {
  typedef decltype(&ufmt_getInt64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getInt64"));

  if (ptr == nullptr) {
    do_fail("ufmt_getInt64");
  }

  return ptr(fmt, status);
}
const void * ufmt_getObject(const UFormattable * fmt, UErrorCode * status) {
  typedef decltype(&ufmt_getObject) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getObject"));

  if (ptr == nullptr) {
    do_fail("ufmt_getObject");
  }

  return ptr(fmt, status);
}
const UChar * ufmt_getUChars(UFormattable * fmt, int32_t * len, UErrorCode * status) {
  typedef decltype(&ufmt_getUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getUChars"));

  if (ptr == nullptr) {
    do_fail("ufmt_getUChars");
  }

  return ptr(fmt, len, status);
}
int32_t ufmt_getArrayLength(const UFormattable * fmt, UErrorCode * status) {
  typedef decltype(&ufmt_getArrayLength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getArrayLength"));

  if (ptr == nullptr) {
    do_fail("ufmt_getArrayLength");
  }

  return ptr(fmt, status);
}
UFormattable * ufmt_getArrayItemByIndex(UFormattable * fmt, int32_t n, UErrorCode * status) {
  typedef decltype(&ufmt_getArrayItemByIndex) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getArrayItemByIndex"));

  if (ptr == nullptr) {
    do_fail("ufmt_getArrayItemByIndex");
  }

  return ptr(fmt, n, status);
}
const char * ufmt_getDecNumChars(UFormattable * fmt, int32_t * len, UErrorCode * status) {
  typedef decltype(&ufmt_getDecNumChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufmt_getDecNumChars"));

  if (ptr == nullptr) {
    do_fail("ufmt_getDecNumChars");
  }

  return ptr(fmt, len, status);
}
URegularExpression * uregex_open(const UChar * pattern, int32_t patternLength, uint32_t flags, UParseError * pe, UErrorCode * status) {
  typedef decltype(&uregex_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_open"));

  if (ptr == nullptr) {
    do_fail("uregex_open");
  }

  return ptr(pattern, patternLength, flags, pe, status);
}
URegularExpression * uregex_openUText(UText * pattern, uint32_t flags, UParseError * pe, UErrorCode * status) {
  typedef decltype(&uregex_openUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_openUText"));

  if (ptr == nullptr) {
    do_fail("uregex_openUText");
  }

  return ptr(pattern, flags, pe, status);
}
URegularExpression * uregex_openC(const char * pattern, uint32_t flags, UParseError * pe, UErrorCode * status) {
  typedef decltype(&uregex_openC) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_openC"));

  if (ptr == nullptr) {
    do_fail("uregex_openC");
  }

  return ptr(pattern, flags, pe, status);
}
void uregex_close(URegularExpression * regexp) {
  typedef decltype(&uregex_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_close"));

  if (ptr == nullptr) {
    do_fail("uregex_close");
  }

  ptr(regexp);
}
URegularExpression * uregex_clone(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_clone"));

  if (ptr == nullptr) {
    do_fail("uregex_clone");
  }

  return ptr(regexp, status);
}
const UChar * uregex_pattern(const URegularExpression * regexp, int32_t * patLength, UErrorCode * status) {
  typedef decltype(&uregex_pattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_pattern"));

  if (ptr == nullptr) {
    do_fail("uregex_pattern");
  }

  return ptr(regexp, patLength, status);
}
UText * uregex_patternUText(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_patternUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_patternUText"));

  if (ptr == nullptr) {
    do_fail("uregex_patternUText");
  }

  return ptr(regexp, status);
}
int32_t uregex_flags(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_flags) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_flags"));

  if (ptr == nullptr) {
    do_fail("uregex_flags");
  }

  return ptr(regexp, status);
}
void uregex_setText(URegularExpression * regexp, const UChar * text, int32_t textLength, UErrorCode * status) {
  typedef decltype(&uregex_setText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setText"));

  if (ptr == nullptr) {
    do_fail("uregex_setText");
  }

  ptr(regexp, text, textLength, status);
}
void uregex_setUText(URegularExpression * regexp, UText * text, UErrorCode * status) {
  typedef decltype(&uregex_setUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setUText"));

  if (ptr == nullptr) {
    do_fail("uregex_setUText");
  }

  ptr(regexp, text, status);
}
const UChar * uregex_getText(URegularExpression * regexp, int32_t * textLength, UErrorCode * status) {
  typedef decltype(&uregex_getText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_getText"));

  if (ptr == nullptr) {
    do_fail("uregex_getText");
  }

  return ptr(regexp, textLength, status);
}
UText * uregex_getUText(URegularExpression * regexp, UText * dest, UErrorCode * status) {
  typedef decltype(&uregex_getUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_getUText"));

  if (ptr == nullptr) {
    do_fail("uregex_getUText");
  }

  return ptr(regexp, dest, status);
}
void uregex_refreshUText(URegularExpression * regexp, UText * text, UErrorCode * status) {
  typedef decltype(&uregex_refreshUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_refreshUText"));

  if (ptr == nullptr) {
    do_fail("uregex_refreshUText");
  }

  ptr(regexp, text, status);
}
UBool uregex_matches(URegularExpression * regexp, int32_t startIndex, UErrorCode * status) {
  typedef decltype(&uregex_matches) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_matches"));

  if (ptr == nullptr) {
    do_fail("uregex_matches");
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_matches64(URegularExpression * regexp, int64_t startIndex, UErrorCode * status) {
  typedef decltype(&uregex_matches64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_matches64"));

  if (ptr == nullptr) {
    do_fail("uregex_matches64");
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_lookingAt(URegularExpression * regexp, int32_t startIndex, UErrorCode * status) {
  typedef decltype(&uregex_lookingAt) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_lookingAt"));

  if (ptr == nullptr) {
    do_fail("uregex_lookingAt");
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_lookingAt64(URegularExpression * regexp, int64_t startIndex, UErrorCode * status) {
  typedef decltype(&uregex_lookingAt64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_lookingAt64"));

  if (ptr == nullptr) {
    do_fail("uregex_lookingAt64");
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_find(URegularExpression * regexp, int32_t startIndex, UErrorCode * status) {
  typedef decltype(&uregex_find) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_find"));

  if (ptr == nullptr) {
    do_fail("uregex_find");
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_find64(URegularExpression * regexp, int64_t startIndex, UErrorCode * status) {
  typedef decltype(&uregex_find64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_find64"));

  if (ptr == nullptr) {
    do_fail("uregex_find64");
  }

  return ptr(regexp, startIndex, status);
}
UBool uregex_findNext(URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_findNext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_findNext"));

  if (ptr == nullptr) {
    do_fail("uregex_findNext");
  }

  return ptr(regexp, status);
}
int32_t uregex_groupCount(URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_groupCount) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_groupCount"));

  if (ptr == nullptr) {
    do_fail("uregex_groupCount");
  }

  return ptr(regexp, status);
}
int32_t uregex_groupNumberFromName(URegularExpression * regexp, const UChar * groupName, int32_t nameLength, UErrorCode * status) {
  typedef decltype(&uregex_groupNumberFromName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_groupNumberFromName"));

  if (ptr == nullptr) {
    do_fail("uregex_groupNumberFromName");
  }

  return ptr(regexp, groupName, nameLength, status);
}
int32_t uregex_groupNumberFromCName(URegularExpression * regexp, const char * groupName, int32_t nameLength, UErrorCode * status) {
  typedef decltype(&uregex_groupNumberFromCName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_groupNumberFromCName"));

  if (ptr == nullptr) {
    do_fail("uregex_groupNumberFromCName");
  }

  return ptr(regexp, groupName, nameLength, status);
}
int32_t uregex_group(URegularExpression * regexp, int32_t groupNum, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&uregex_group) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_group"));

  if (ptr == nullptr) {
    do_fail("uregex_group");
  }

  return ptr(regexp, groupNum, dest, destCapacity, status);
}
UText * uregex_groupUText(URegularExpression * regexp, int32_t groupNum, UText * dest, int64_t * groupLength, UErrorCode * status) {
  typedef decltype(&uregex_groupUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_groupUText"));

  if (ptr == nullptr) {
    do_fail("uregex_groupUText");
  }

  return ptr(regexp, groupNum, dest, groupLength, status);
}
int32_t uregex_start(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  typedef decltype(&uregex_start) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_start"));

  if (ptr == nullptr) {
    do_fail("uregex_start");
  }

  return ptr(regexp, groupNum, status);
}
int64_t uregex_start64(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  typedef decltype(&uregex_start64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_start64"));

  if (ptr == nullptr) {
    do_fail("uregex_start64");
  }

  return ptr(regexp, groupNum, status);
}
int32_t uregex_end(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  typedef decltype(&uregex_end) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_end"));

  if (ptr == nullptr) {
    do_fail("uregex_end");
  }

  return ptr(regexp, groupNum, status);
}
int64_t uregex_end64(URegularExpression * regexp, int32_t groupNum, UErrorCode * status) {
  typedef decltype(&uregex_end64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_end64"));

  if (ptr == nullptr) {
    do_fail("uregex_end64");
  }

  return ptr(regexp, groupNum, status);
}
void uregex_reset(URegularExpression * regexp, int32_t index, UErrorCode * status) {
  typedef decltype(&uregex_reset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_reset"));

  if (ptr == nullptr) {
    do_fail("uregex_reset");
  }

  ptr(regexp, index, status);
}
void uregex_reset64(URegularExpression * regexp, int64_t index, UErrorCode * status) {
  typedef decltype(&uregex_reset64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_reset64"));

  if (ptr == nullptr) {
    do_fail("uregex_reset64");
  }

  ptr(regexp, index, status);
}
void uregex_setRegion(URegularExpression * regexp, int32_t regionStart, int32_t regionLimit, UErrorCode * status) {
  typedef decltype(&uregex_setRegion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setRegion"));

  if (ptr == nullptr) {
    do_fail("uregex_setRegion");
  }

  ptr(regexp, regionStart, regionLimit, status);
}
void uregex_setRegion64(URegularExpression * regexp, int64_t regionStart, int64_t regionLimit, UErrorCode * status) {
  typedef decltype(&uregex_setRegion64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setRegion64"));

  if (ptr == nullptr) {
    do_fail("uregex_setRegion64");
  }

  ptr(regexp, regionStart, regionLimit, status);
}
void uregex_setRegionAndStart(URegularExpression * regexp, int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode * status) {
  typedef decltype(&uregex_setRegionAndStart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setRegionAndStart"));

  if (ptr == nullptr) {
    do_fail("uregex_setRegionAndStart");
  }

  ptr(regexp, regionStart, regionLimit, startIndex, status);
}
int32_t uregex_regionStart(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_regionStart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_regionStart"));

  if (ptr == nullptr) {
    do_fail("uregex_regionStart");
  }

  return ptr(regexp, status);
}
int64_t uregex_regionStart64(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_regionStart64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_regionStart64"));

  if (ptr == nullptr) {
    do_fail("uregex_regionStart64");
  }

  return ptr(regexp, status);
}
int32_t uregex_regionEnd(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_regionEnd) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_regionEnd"));

  if (ptr == nullptr) {
    do_fail("uregex_regionEnd");
  }

  return ptr(regexp, status);
}
int64_t uregex_regionEnd64(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_regionEnd64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_regionEnd64"));

  if (ptr == nullptr) {
    do_fail("uregex_regionEnd64");
  }

  return ptr(regexp, status);
}
UBool uregex_hasTransparentBounds(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_hasTransparentBounds) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_hasTransparentBounds"));

  if (ptr == nullptr) {
    do_fail("uregex_hasTransparentBounds");
  }

  return ptr(regexp, status);
}
void uregex_useTransparentBounds(URegularExpression * regexp, UBool b, UErrorCode * status) {
  typedef decltype(&uregex_useTransparentBounds) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_useTransparentBounds"));

  if (ptr == nullptr) {
    do_fail("uregex_useTransparentBounds");
  }

  ptr(regexp, b, status);
}
UBool uregex_hasAnchoringBounds(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_hasAnchoringBounds) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_hasAnchoringBounds"));

  if (ptr == nullptr) {
    do_fail("uregex_hasAnchoringBounds");
  }

  return ptr(regexp, status);
}
void uregex_useAnchoringBounds(URegularExpression * regexp, UBool b, UErrorCode * status) {
  typedef decltype(&uregex_useAnchoringBounds) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_useAnchoringBounds"));

  if (ptr == nullptr) {
    do_fail("uregex_useAnchoringBounds");
  }

  ptr(regexp, b, status);
}
UBool uregex_hitEnd(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_hitEnd) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_hitEnd"));

  if (ptr == nullptr) {
    do_fail("uregex_hitEnd");
  }

  return ptr(regexp, status);
}
UBool uregex_requireEnd(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_requireEnd) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_requireEnd"));

  if (ptr == nullptr) {
    do_fail("uregex_requireEnd");
  }

  return ptr(regexp, status);
}
int32_t uregex_replaceAll(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar * destBuf, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&uregex_replaceAll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_replaceAll"));

  if (ptr == nullptr) {
    do_fail("uregex_replaceAll");
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
UText * uregex_replaceAllUText(URegularExpression * regexp, UText * replacement, UText * dest, UErrorCode * status) {
  typedef decltype(&uregex_replaceAllUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_replaceAllUText"));

  if (ptr == nullptr) {
    do_fail("uregex_replaceAllUText");
  }

  return ptr(regexp, replacement, dest, status);
}
int32_t uregex_replaceFirst(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar * destBuf, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&uregex_replaceFirst) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_replaceFirst"));

  if (ptr == nullptr) {
    do_fail("uregex_replaceFirst");
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
UText * uregex_replaceFirstUText(URegularExpression * regexp, UText * replacement, UText * dest, UErrorCode * status) {
  typedef decltype(&uregex_replaceFirstUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_replaceFirstUText"));

  if (ptr == nullptr) {
    do_fail("uregex_replaceFirstUText");
  }

  return ptr(regexp, replacement, dest, status);
}
int32_t uregex_appendReplacement(URegularExpression * regexp, const UChar * replacementText, int32_t replacementLength, UChar ** destBuf, int32_t * destCapacity, UErrorCode * status) {
  typedef decltype(&uregex_appendReplacement) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_appendReplacement"));

  if (ptr == nullptr) {
    do_fail("uregex_appendReplacement");
  }

  return ptr(regexp, replacementText, replacementLength, destBuf, destCapacity, status);
}
void uregex_appendReplacementUText(URegularExpression * regexp, UText * replacementText, UText * dest, UErrorCode * status) {
  typedef decltype(&uregex_appendReplacementUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_appendReplacementUText"));

  if (ptr == nullptr) {
    do_fail("uregex_appendReplacementUText");
  }

  ptr(regexp, replacementText, dest, status);
}
int32_t uregex_appendTail(URegularExpression * regexp, UChar ** destBuf, int32_t * destCapacity, UErrorCode * status) {
  typedef decltype(&uregex_appendTail) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_appendTail"));

  if (ptr == nullptr) {
    do_fail("uregex_appendTail");
  }

  return ptr(regexp, destBuf, destCapacity, status);
}
UText * uregex_appendTailUText(URegularExpression * regexp, UText * dest, UErrorCode * status) {
  typedef decltype(&uregex_appendTailUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_appendTailUText"));

  if (ptr == nullptr) {
    do_fail("uregex_appendTailUText");
  }

  return ptr(regexp, dest, status);
}
int32_t uregex_split(URegularExpression * regexp, UChar * destBuf, int32_t destCapacity, int32_t * requiredCapacity, UChar * destFields[], int32_t destFieldsCapacity, UErrorCode * status) {
  typedef decltype(&uregex_split) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_split"));

  if (ptr == nullptr) {
    do_fail("uregex_split");
  }

  return ptr(regexp, destBuf, destCapacity, requiredCapacity, destFields, destFieldsCapacity, status);
}
int32_t uregex_splitUText(URegularExpression * regexp, UText * destFields[], int32_t destFieldsCapacity, UErrorCode * status) {
  typedef decltype(&uregex_splitUText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_splitUText"));

  if (ptr == nullptr) {
    do_fail("uregex_splitUText");
  }

  return ptr(regexp, destFields, destFieldsCapacity, status);
}
void uregex_setTimeLimit(URegularExpression * regexp, int32_t limit, UErrorCode * status) {
  typedef decltype(&uregex_setTimeLimit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setTimeLimit"));

  if (ptr == nullptr) {
    do_fail("uregex_setTimeLimit");
  }

  ptr(regexp, limit, status);
}
int32_t uregex_getTimeLimit(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_getTimeLimit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_getTimeLimit"));

  if (ptr == nullptr) {
    do_fail("uregex_getTimeLimit");
  }

  return ptr(regexp, status);
}
void uregex_setStackLimit(URegularExpression * regexp, int32_t limit, UErrorCode * status) {
  typedef decltype(&uregex_setStackLimit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setStackLimit"));

  if (ptr == nullptr) {
    do_fail("uregex_setStackLimit");
  }

  ptr(regexp, limit, status);
}
int32_t uregex_getStackLimit(const URegularExpression * regexp, UErrorCode * status) {
  typedef decltype(&uregex_getStackLimit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_getStackLimit"));

  if (ptr == nullptr) {
    do_fail("uregex_getStackLimit");
  }

  return ptr(regexp, status);
}
void uregex_setMatchCallback(URegularExpression * regexp, URegexMatchCallback * callback, const void * context, UErrorCode * status) {
  typedef decltype(&uregex_setMatchCallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setMatchCallback"));

  if (ptr == nullptr) {
    do_fail("uregex_setMatchCallback");
  }

  ptr(regexp, callback, context, status);
}
void uregex_getMatchCallback(const URegularExpression * regexp, URegexMatchCallback ** callback, const void ** context, UErrorCode * status) {
  typedef decltype(&uregex_getMatchCallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_getMatchCallback"));

  if (ptr == nullptr) {
    do_fail("uregex_getMatchCallback");
  }

  ptr(regexp, callback, context, status);
}
void uregex_setFindProgressCallback(URegularExpression * regexp, URegexFindProgressCallback * callback, const void * context, UErrorCode * status) {
  typedef decltype(&uregex_setFindProgressCallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_setFindProgressCallback"));

  if (ptr == nullptr) {
    do_fail("uregex_setFindProgressCallback");
  }

  ptr(regexp, callback, context, status);
}
void uregex_getFindProgressCallback(const URegularExpression * regexp, URegexFindProgressCallback ** callback, const void ** context, UErrorCode * status) {
  typedef decltype(&uregex_getFindProgressCallback) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregex_getFindProgressCallback"));

  if (ptr == nullptr) {
    do_fail("uregex_getFindProgressCallback");
  }

  ptr(regexp, callback, context, status);
}
UNumberingSystem * unumsys_open(const char * locale, UErrorCode * status) {
  typedef decltype(&unumsys_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_open"));

  if (ptr == nullptr) {
    do_fail("unumsys_open");
  }

  return ptr(locale, status);
}
UNumberingSystem * unumsys_openByName(const char * name, UErrorCode * status) {
  typedef decltype(&unumsys_openByName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_openByName"));

  if (ptr == nullptr) {
    do_fail("unumsys_openByName");
  }

  return ptr(name, status);
}
void unumsys_close(UNumberingSystem * unumsys) {
  typedef decltype(&unumsys_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_close"));

  if (ptr == nullptr) {
    do_fail("unumsys_close");
  }

  ptr(unumsys);
}
UEnumeration * unumsys_openAvailableNames(UErrorCode * status) {
  typedef decltype(&unumsys_openAvailableNames) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_openAvailableNames"));

  if (ptr == nullptr) {
    do_fail("unumsys_openAvailableNames");
  }

  return ptr(status);
}
const char * unumsys_getName(const UNumberingSystem * unumsys) {
  typedef decltype(&unumsys_getName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_getName"));

  if (ptr == nullptr) {
    do_fail("unumsys_getName");
  }

  return ptr(unumsys);
}
UBool unumsys_isAlgorithmic(const UNumberingSystem * unumsys) {
  typedef decltype(&unumsys_isAlgorithmic) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_isAlgorithmic"));

  if (ptr == nullptr) {
    do_fail("unumsys_isAlgorithmic");
  }

  return ptr(unumsys);
}
int32_t unumsys_getRadix(const UNumberingSystem * unumsys) {
  typedef decltype(&unumsys_getRadix) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_getRadix"));

  if (ptr == nullptr) {
    do_fail("unumsys_getRadix");
  }

  return ptr(unumsys);
}
int32_t unumsys_getDescription(const UNumberingSystem * unumsys, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&unumsys_getDescription) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unumsys_getDescription"));

  if (ptr == nullptr) {
    do_fail("unumsys_getDescription");
  }

  return ptr(unumsys, result, resultLength, status);
}
UEnumeration * ucal_openTimeZoneIDEnumeration(USystemTimeZoneType zoneType, const char * region, const int32_t * rawOffset, UErrorCode * ec) {
  typedef decltype(&ucal_openTimeZoneIDEnumeration) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_openTimeZoneIDEnumeration"));

  if (ptr == nullptr) {
    do_fail("ucal_openTimeZoneIDEnumeration");
  }

  return ptr(zoneType, region, rawOffset, ec);
}
UEnumeration * ucal_openTimeZones(UErrorCode * ec) {
  typedef decltype(&ucal_openTimeZones) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_openTimeZones"));

  if (ptr == nullptr) {
    do_fail("ucal_openTimeZones");
  }

  return ptr(ec);
}
UEnumeration * ucal_openCountryTimeZones(const char * country, UErrorCode * ec) {
  typedef decltype(&ucal_openCountryTimeZones) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_openCountryTimeZones"));

  if (ptr == nullptr) {
    do_fail("ucal_openCountryTimeZones");
  }

  return ptr(country, ec);
}
int32_t ucal_getDefaultTimeZone(UChar * result, int32_t resultCapacity, UErrorCode * ec) {
  typedef decltype(&ucal_getDefaultTimeZone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getDefaultTimeZone"));

  if (ptr == nullptr) {
    do_fail("ucal_getDefaultTimeZone");
  }

  return ptr(result, resultCapacity, ec);
}
void ucal_setDefaultTimeZone(const UChar * zoneID, UErrorCode * ec) {
  typedef decltype(&ucal_setDefaultTimeZone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_setDefaultTimeZone"));

  if (ptr == nullptr) {
    do_fail("ucal_setDefaultTimeZone");
  }

  ptr(zoneID, ec);
}
int32_t ucal_getDSTSavings(const UChar * zoneID, UErrorCode * ec) {
  typedef decltype(&ucal_getDSTSavings) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getDSTSavings"));

  if (ptr == nullptr) {
    do_fail("ucal_getDSTSavings");
  }

  return ptr(zoneID, ec);
}
UDate ucal_getNow() {
  typedef decltype(&ucal_getNow) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getNow"));

  if (ptr == nullptr) {
    do_fail("ucal_getNow");
  }

  return ptr();
}
UCalendar * ucal_open(const UChar * zoneID, int32_t len, const char * locale, UCalendarType type, UErrorCode * status) {
  typedef decltype(&ucal_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_open"));

  if (ptr == nullptr) {
    do_fail("ucal_open");
  }

  return ptr(zoneID, len, locale, type, status);
}
void ucal_close(UCalendar * cal) {
  typedef decltype(&ucal_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_close"));

  if (ptr == nullptr) {
    do_fail("ucal_close");
  }

  ptr(cal);
}
UCalendar * ucal_clone(const UCalendar * cal, UErrorCode * status) {
  typedef decltype(&ucal_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_clone"));

  if (ptr == nullptr) {
    do_fail("ucal_clone");
  }

  return ptr(cal, status);
}
void ucal_setTimeZone(UCalendar * cal, const UChar * zoneID, int32_t len, UErrorCode * status) {
  typedef decltype(&ucal_setTimeZone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_setTimeZone"));

  if (ptr == nullptr) {
    do_fail("ucal_setTimeZone");
  }

  ptr(cal, zoneID, len, status);
}
int32_t ucal_getTimeZoneID(const UCalendar * cal, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&ucal_getTimeZoneID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getTimeZoneID"));

  if (ptr == nullptr) {
    do_fail("ucal_getTimeZoneID");
  }

  return ptr(cal, result, resultLength, status);
}
int32_t ucal_getTimeZoneDisplayName(const UCalendar * cal, UCalendarDisplayNameType type, const char * locale, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&ucal_getTimeZoneDisplayName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getTimeZoneDisplayName"));

  if (ptr == nullptr) {
    do_fail("ucal_getTimeZoneDisplayName");
  }

  return ptr(cal, type, locale, result, resultLength, status);
}
UBool ucal_inDaylightTime(const UCalendar * cal, UErrorCode * status) {
  typedef decltype(&ucal_inDaylightTime) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_inDaylightTime"));

  if (ptr == nullptr) {
    do_fail("ucal_inDaylightTime");
  }

  return ptr(cal, status);
}
void ucal_setGregorianChange(UCalendar * cal, UDate date, UErrorCode * pErrorCode) {
  typedef decltype(&ucal_setGregorianChange) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_setGregorianChange"));

  if (ptr == nullptr) {
    do_fail("ucal_setGregorianChange");
  }

  ptr(cal, date, pErrorCode);
}
UDate ucal_getGregorianChange(const UCalendar * cal, UErrorCode * pErrorCode) {
  typedef decltype(&ucal_getGregorianChange) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getGregorianChange"));

  if (ptr == nullptr) {
    do_fail("ucal_getGregorianChange");
  }

  return ptr(cal, pErrorCode);
}
int32_t ucal_getAttribute(const UCalendar * cal, UCalendarAttribute attr) {
  typedef decltype(&ucal_getAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getAttribute"));

  if (ptr == nullptr) {
    do_fail("ucal_getAttribute");
  }

  return ptr(cal, attr);
}
void ucal_setAttribute(UCalendar * cal, UCalendarAttribute attr, int32_t newValue) {
  typedef decltype(&ucal_setAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_setAttribute"));

  if (ptr == nullptr) {
    do_fail("ucal_setAttribute");
  }

  ptr(cal, attr, newValue);
}
const char * ucal_getAvailable(int32_t localeIndex) {
  typedef decltype(&ucal_getAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getAvailable"));

  if (ptr == nullptr) {
    do_fail("ucal_getAvailable");
  }

  return ptr(localeIndex);
}
int32_t ucal_countAvailable() {
  typedef decltype(&ucal_countAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_countAvailable"));

  if (ptr == nullptr) {
    do_fail("ucal_countAvailable");
  }

  return ptr();
}
UDate ucal_getMillis(const UCalendar * cal, UErrorCode * status) {
  typedef decltype(&ucal_getMillis) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getMillis"));

  if (ptr == nullptr) {
    do_fail("ucal_getMillis");
  }

  return ptr(cal, status);
}
void ucal_setMillis(UCalendar * cal, UDate dateTime, UErrorCode * status) {
  typedef decltype(&ucal_setMillis) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_setMillis"));

  if (ptr == nullptr) {
    do_fail("ucal_setMillis");
  }

  ptr(cal, dateTime, status);
}
void ucal_setDate(UCalendar * cal, int32_t year, int32_t month, int32_t date, UErrorCode * status) {
  typedef decltype(&ucal_setDate) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_setDate"));

  if (ptr == nullptr) {
    do_fail("ucal_setDate");
  }

  ptr(cal, year, month, date, status);
}
void ucal_setDateTime(UCalendar * cal, int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode * status) {
  typedef decltype(&ucal_setDateTime) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_setDateTime"));

  if (ptr == nullptr) {
    do_fail("ucal_setDateTime");
  }

  ptr(cal, year, month, date, hour, minute, second, status);
}
UBool ucal_equivalentTo(const UCalendar * cal1, const UCalendar * cal2) {
  typedef decltype(&ucal_equivalentTo) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_equivalentTo"));

  if (ptr == nullptr) {
    do_fail("ucal_equivalentTo");
  }

  return ptr(cal1, cal2);
}
void ucal_add(UCalendar * cal, UCalendarDateFields field, int32_t amount, UErrorCode * status) {
  typedef decltype(&ucal_add) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_add"));

  if (ptr == nullptr) {
    do_fail("ucal_add");
  }

  ptr(cal, field, amount, status);
}
void ucal_roll(UCalendar * cal, UCalendarDateFields field, int32_t amount, UErrorCode * status) {
  typedef decltype(&ucal_roll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_roll"));

  if (ptr == nullptr) {
    do_fail("ucal_roll");
  }

  ptr(cal, field, amount, status);
}
int32_t ucal_get(const UCalendar * cal, UCalendarDateFields field, UErrorCode * status) {
  typedef decltype(&ucal_get) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_get"));

  if (ptr == nullptr) {
    do_fail("ucal_get");
  }

  return ptr(cal, field, status);
}
void ucal_set(UCalendar * cal, UCalendarDateFields field, int32_t value) {
  typedef decltype(&ucal_set) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_set"));

  if (ptr == nullptr) {
    do_fail("ucal_set");
  }

  ptr(cal, field, value);
}
UBool ucal_isSet(const UCalendar * cal, UCalendarDateFields field) {
  typedef decltype(&ucal_isSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_isSet"));

  if (ptr == nullptr) {
    do_fail("ucal_isSet");
  }

  return ptr(cal, field);
}
void ucal_clearField(UCalendar * cal, UCalendarDateFields field) {
  typedef decltype(&ucal_clearField) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_clearField"));

  if (ptr == nullptr) {
    do_fail("ucal_clearField");
  }

  ptr(cal, field);
}
void ucal_clear(UCalendar * calendar) {
  typedef decltype(&ucal_clear) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_clear"));

  if (ptr == nullptr) {
    do_fail("ucal_clear");
  }

  ptr(calendar);
}
int32_t ucal_getLimit(const UCalendar * cal, UCalendarDateFields field, UCalendarLimitType type, UErrorCode * status) {
  typedef decltype(&ucal_getLimit) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getLimit"));

  if (ptr == nullptr) {
    do_fail("ucal_getLimit");
  }

  return ptr(cal, field, type, status);
}
const char * ucal_getLocaleByType(const UCalendar * cal, ULocDataLocaleType type, UErrorCode * status) {
  typedef decltype(&ucal_getLocaleByType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getLocaleByType"));

  if (ptr == nullptr) {
    do_fail("ucal_getLocaleByType");
  }

  return ptr(cal, type, status);
}
const char * ucal_getTZDataVersion(UErrorCode * status) {
  typedef decltype(&ucal_getTZDataVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getTZDataVersion"));

  if (ptr == nullptr) {
    do_fail("ucal_getTZDataVersion");
  }

  return ptr(status);
}
int32_t ucal_getCanonicalTimeZoneID(const UChar * id, int32_t len, UChar * result, int32_t resultCapacity, UBool * isSystemID, UErrorCode * status) {
  typedef decltype(&ucal_getCanonicalTimeZoneID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getCanonicalTimeZoneID"));

  if (ptr == nullptr) {
    do_fail("ucal_getCanonicalTimeZoneID");
  }

  return ptr(id, len, result, resultCapacity, isSystemID, status);
}
const char * ucal_getType(const UCalendar * cal, UErrorCode * status) {
  typedef decltype(&ucal_getType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getType"));

  if (ptr == nullptr) {
    do_fail("ucal_getType");
  }

  return ptr(cal, status);
}
UEnumeration * ucal_getKeywordValuesForLocale(const char * key, const char * locale, UBool commonlyUsed, UErrorCode * status) {
  typedef decltype(&ucal_getKeywordValuesForLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getKeywordValuesForLocale"));

  if (ptr == nullptr) {
    do_fail("ucal_getKeywordValuesForLocale");
  }

  return ptr(key, locale, commonlyUsed, status);
}
UCalendarWeekdayType ucal_getDayOfWeekType(const UCalendar * cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode * status) {
  typedef decltype(&ucal_getDayOfWeekType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getDayOfWeekType"));

  if (ptr == nullptr) {
    do_fail("ucal_getDayOfWeekType");
  }

  return ptr(cal, dayOfWeek, status);
}
int32_t ucal_getWeekendTransition(const UCalendar * cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode * status) {
  typedef decltype(&ucal_getWeekendTransition) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getWeekendTransition"));

  if (ptr == nullptr) {
    do_fail("ucal_getWeekendTransition");
  }

  return ptr(cal, dayOfWeek, status);
}
UBool ucal_isWeekend(const UCalendar * cal, UDate date, UErrorCode * status) {
  typedef decltype(&ucal_isWeekend) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_isWeekend"));

  if (ptr == nullptr) {
    do_fail("ucal_isWeekend");
  }

  return ptr(cal, date, status);
}
int32_t ucal_getFieldDifference(UCalendar * cal, UDate target, UCalendarDateFields field, UErrorCode * status) {
  typedef decltype(&ucal_getFieldDifference) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getFieldDifference"));

  if (ptr == nullptr) {
    do_fail("ucal_getFieldDifference");
  }

  return ptr(cal, target, field, status);
}
UBool ucal_getTimeZoneTransitionDate(const UCalendar * cal, UTimeZoneTransitionType type, UDate * transition, UErrorCode * status) {
  typedef decltype(&ucal_getTimeZoneTransitionDate) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getTimeZoneTransitionDate"));

  if (ptr == nullptr) {
    do_fail("ucal_getTimeZoneTransitionDate");
  }

  return ptr(cal, type, transition, status);
}
int32_t ucal_getWindowsTimeZoneID(const UChar * id, int32_t len, UChar * winid, int32_t winidCapacity, UErrorCode * status) {
  typedef decltype(&ucal_getWindowsTimeZoneID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getWindowsTimeZoneID"));

  if (ptr == nullptr) {
    do_fail("ucal_getWindowsTimeZoneID");
  }

  return ptr(id, len, winid, winidCapacity, status);
}
int32_t ucal_getTimeZoneIDForWindowsID(const UChar * winid, int32_t len, const char * region, UChar * id, int32_t idCapacity, UErrorCode * status) {
  typedef decltype(&ucal_getTimeZoneIDForWindowsID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucal_getTimeZoneIDForWindowsID"));

  if (ptr == nullptr) {
    do_fail("ucal_getTimeZoneIDForWindowsID");
  }

  return ptr(winid, len, region, id, idCapacity, status);
}
UDateTimePatternGenerator * udatpg_open(const char * locale, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_open"));

  if (ptr == nullptr) {
    do_fail("udatpg_open");
  }

  return ptr(locale, pErrorCode);
}
UDateTimePatternGenerator * udatpg_openEmpty(UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_openEmpty) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_openEmpty"));

  if (ptr == nullptr) {
    do_fail("udatpg_openEmpty");
  }

  return ptr(pErrorCode);
}
void udatpg_close(UDateTimePatternGenerator * dtpg) {
  typedef decltype(&udatpg_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_close"));

  if (ptr == nullptr) {
    do_fail("udatpg_close");
  }

  ptr(dtpg);
}
UDateTimePatternGenerator * udatpg_clone(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_clone"));

  if (ptr == nullptr) {
    do_fail("udatpg_clone");
  }

  return ptr(dtpg, pErrorCode);
}
int32_t udatpg_getBestPattern(UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t length, UChar * bestPattern, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_getBestPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getBestPattern"));

  if (ptr == nullptr) {
    do_fail("udatpg_getBestPattern");
  }

  return ptr(dtpg, skeleton, length, bestPattern, capacity, pErrorCode);
}
int32_t udatpg_getBestPatternWithOptions(UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t length, UDateTimePatternMatchOptions options, UChar * bestPattern, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_getBestPatternWithOptions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getBestPatternWithOptions"));

  if (ptr == nullptr) {
    do_fail("udatpg_getBestPatternWithOptions");
  }

  return ptr(dtpg, skeleton, length, options, bestPattern, capacity, pErrorCode);
}
int32_t udatpg_getSkeleton(UDateTimePatternGenerator * unusedDtpg, const UChar * pattern, int32_t length, UChar * skeleton, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_getSkeleton) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getSkeleton"));

  if (ptr == nullptr) {
    do_fail("udatpg_getSkeleton");
  }

  return ptr(unusedDtpg, pattern, length, skeleton, capacity, pErrorCode);
}
int32_t udatpg_getBaseSkeleton(UDateTimePatternGenerator * unusedDtpg, const UChar * pattern, int32_t length, UChar * baseSkeleton, int32_t capacity, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_getBaseSkeleton) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getBaseSkeleton"));

  if (ptr == nullptr) {
    do_fail("udatpg_getBaseSkeleton");
  }

  return ptr(unusedDtpg, pattern, length, baseSkeleton, capacity, pErrorCode);
}
UDateTimePatternConflict udatpg_addPattern(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, UBool override, UChar * conflictingPattern, int32_t capacity, int32_t * pLength, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_addPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_addPattern"));

  if (ptr == nullptr) {
    do_fail("udatpg_addPattern");
  }

  return ptr(dtpg, pattern, patternLength, override, conflictingPattern, capacity, pLength, pErrorCode);
}
void udatpg_setAppendItemFormat(UDateTimePatternGenerator * dtpg, UDateTimePatternField field, const UChar * value, int32_t length) {
  typedef decltype(&udatpg_setAppendItemFormat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_setAppendItemFormat"));

  if (ptr == nullptr) {
    do_fail("udatpg_setAppendItemFormat");
  }

  ptr(dtpg, field, value, length);
}
const UChar * udatpg_getAppendItemFormat(const UDateTimePatternGenerator * dtpg, UDateTimePatternField field, int32_t * pLength) {
  typedef decltype(&udatpg_getAppendItemFormat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getAppendItemFormat"));

  if (ptr == nullptr) {
    do_fail("udatpg_getAppendItemFormat");
  }

  return ptr(dtpg, field, pLength);
}
void udatpg_setAppendItemName(UDateTimePatternGenerator * dtpg, UDateTimePatternField field, const UChar * value, int32_t length) {
  typedef decltype(&udatpg_setAppendItemName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_setAppendItemName"));

  if (ptr == nullptr) {
    do_fail("udatpg_setAppendItemName");
  }

  ptr(dtpg, field, value, length);
}
const UChar * udatpg_getAppendItemName(const UDateTimePatternGenerator * dtpg, UDateTimePatternField field, int32_t * pLength) {
  typedef decltype(&udatpg_getAppendItemName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getAppendItemName"));

  if (ptr == nullptr) {
    do_fail("udatpg_getAppendItemName");
  }

  return ptr(dtpg, field, pLength);
}
void udatpg_setDateTimeFormat(const UDateTimePatternGenerator * dtpg, const UChar * dtFormat, int32_t length) {
  typedef decltype(&udatpg_setDateTimeFormat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_setDateTimeFormat"));

  if (ptr == nullptr) {
    do_fail("udatpg_setDateTimeFormat");
  }

  ptr(dtpg, dtFormat, length);
}
const UChar * udatpg_getDateTimeFormat(const UDateTimePatternGenerator * dtpg, int32_t * pLength) {
  typedef decltype(&udatpg_getDateTimeFormat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getDateTimeFormat"));

  if (ptr == nullptr) {
    do_fail("udatpg_getDateTimeFormat");
  }

  return ptr(dtpg, pLength);
}
void udatpg_setDecimal(UDateTimePatternGenerator * dtpg, const UChar * decimal, int32_t length) {
  typedef decltype(&udatpg_setDecimal) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_setDecimal"));

  if (ptr == nullptr) {
    do_fail("udatpg_setDecimal");
  }

  ptr(dtpg, decimal, length);
}
const UChar * udatpg_getDecimal(const UDateTimePatternGenerator * dtpg, int32_t * pLength) {
  typedef decltype(&udatpg_getDecimal) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getDecimal"));

  if (ptr == nullptr) {
    do_fail("udatpg_getDecimal");
  }

  return ptr(dtpg, pLength);
}
int32_t udatpg_replaceFieldTypes(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, const UChar * skeleton, int32_t skeletonLength, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_replaceFieldTypes) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_replaceFieldTypes"));

  if (ptr == nullptr) {
    do_fail("udatpg_replaceFieldTypes");
  }

  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, dest, destCapacity, pErrorCode);
}
int32_t udatpg_replaceFieldTypesWithOptions(UDateTimePatternGenerator * dtpg, const UChar * pattern, int32_t patternLength, const UChar * skeleton, int32_t skeletonLength, UDateTimePatternMatchOptions options, UChar * dest, int32_t destCapacity, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_replaceFieldTypesWithOptions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_replaceFieldTypesWithOptions"));

  if (ptr == nullptr) {
    do_fail("udatpg_replaceFieldTypesWithOptions");
  }

  return ptr(dtpg, pattern, patternLength, skeleton, skeletonLength, options, dest, destCapacity, pErrorCode);
}
UEnumeration * udatpg_openSkeletons(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_openSkeletons) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_openSkeletons"));

  if (ptr == nullptr) {
    do_fail("udatpg_openSkeletons");
  }

  return ptr(dtpg, pErrorCode);
}
UEnumeration * udatpg_openBaseSkeletons(const UDateTimePatternGenerator * dtpg, UErrorCode * pErrorCode) {
  typedef decltype(&udatpg_openBaseSkeletons) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_openBaseSkeletons"));

  if (ptr == nullptr) {
    do_fail("udatpg_openBaseSkeletons");
  }

  return ptr(dtpg, pErrorCode);
}
const UChar * udatpg_getPatternForSkeleton(const UDateTimePatternGenerator * dtpg, const UChar * skeleton, int32_t skeletonLength, int32_t * pLength) {
  typedef decltype(&udatpg_getPatternForSkeleton) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udatpg_getPatternForSkeleton"));

  if (ptr == nullptr) {
    do_fail("udatpg_getPatternForSkeleton");
  }

  return ptr(dtpg, skeleton, skeletonLength, pLength);
}
int64_t utmscale_getTimeScaleValue(UDateTimeScale timeScale, UTimeScaleValue value, UErrorCode * status) {
  typedef decltype(&utmscale_getTimeScaleValue) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utmscale_getTimeScaleValue"));

  if (ptr == nullptr) {
    do_fail("utmscale_getTimeScaleValue");
  }

  return ptr(timeScale, value, status);
}
int64_t utmscale_fromInt64(int64_t otherTime, UDateTimeScale timeScale, UErrorCode * status) {
  typedef decltype(&utmscale_fromInt64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utmscale_fromInt64"));

  if (ptr == nullptr) {
    do_fail("utmscale_fromInt64");
  }

  return ptr(otherTime, timeScale, status);
}
int64_t utmscale_toInt64(int64_t universalTime, UDateTimeScale timeScale, UErrorCode * status) {
  typedef decltype(&utmscale_toInt64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utmscale_toInt64"));

  if (ptr == nullptr) {
    do_fail("utmscale_toInt64");
  }

  return ptr(universalTime, timeScale, status);
}
UFieldPositionIterator * ufieldpositer_open(UErrorCode * status) {
  typedef decltype(&ufieldpositer_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufieldpositer_open"));

  if (ptr == nullptr) {
    do_fail("ufieldpositer_open");
  }

  return ptr(status);
}
void ufieldpositer_close(UFieldPositionIterator * fpositer) {
  typedef decltype(&ufieldpositer_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufieldpositer_close"));

  if (ptr == nullptr) {
    do_fail("ufieldpositer_close");
  }

  ptr(fpositer);
}
int32_t ufieldpositer_next(UFieldPositionIterator * fpositer, int32_t * beginIndex, int32_t * endIndex) {
  typedef decltype(&ufieldpositer_next) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ufieldpositer_next"));

  if (ptr == nullptr) {
    do_fail("ufieldpositer_next");
  }

  return ptr(fpositer, beginIndex, endIndex);
}
UCharsetDetector * ucsdet_open(UErrorCode * status) {
  typedef decltype(&ucsdet_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_open"));

  if (ptr == nullptr) {
    do_fail("ucsdet_open");
  }

  return ptr(status);
}
void ucsdet_close(UCharsetDetector * ucsd) {
  typedef decltype(&ucsdet_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_close"));

  if (ptr == nullptr) {
    do_fail("ucsdet_close");
  }

  ptr(ucsd);
}
void ucsdet_setText(UCharsetDetector * ucsd, const char * textIn, int32_t len, UErrorCode * status) {
  typedef decltype(&ucsdet_setText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_setText"));

  if (ptr == nullptr) {
    do_fail("ucsdet_setText");
  }

  ptr(ucsd, textIn, len, status);
}
void ucsdet_setDeclaredEncoding(UCharsetDetector * ucsd, const char * encoding, int32_t length, UErrorCode * status) {
  typedef decltype(&ucsdet_setDeclaredEncoding) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_setDeclaredEncoding"));

  if (ptr == nullptr) {
    do_fail("ucsdet_setDeclaredEncoding");
  }

  ptr(ucsd, encoding, length, status);
}
const UCharsetMatch * ucsdet_detect(UCharsetDetector * ucsd, UErrorCode * status) {
  typedef decltype(&ucsdet_detect) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_detect"));

  if (ptr == nullptr) {
    do_fail("ucsdet_detect");
  }

  return ptr(ucsd, status);
}
const UCharsetMatch ** ucsdet_detectAll(UCharsetDetector * ucsd, int32_t * matchesFound, UErrorCode * status) {
  typedef decltype(&ucsdet_detectAll) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_detectAll"));

  if (ptr == nullptr) {
    do_fail("ucsdet_detectAll");
  }

  return ptr(ucsd, matchesFound, status);
}
const char * ucsdet_getName(const UCharsetMatch * ucsm, UErrorCode * status) {
  typedef decltype(&ucsdet_getName) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_getName"));

  if (ptr == nullptr) {
    do_fail("ucsdet_getName");
  }

  return ptr(ucsm, status);
}
int32_t ucsdet_getConfidence(const UCharsetMatch * ucsm, UErrorCode * status) {
  typedef decltype(&ucsdet_getConfidence) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_getConfidence"));

  if (ptr == nullptr) {
    do_fail("ucsdet_getConfidence");
  }

  return ptr(ucsm, status);
}
const char * ucsdet_getLanguage(const UCharsetMatch * ucsm, UErrorCode * status) {
  typedef decltype(&ucsdet_getLanguage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_getLanguage"));

  if (ptr == nullptr) {
    do_fail("ucsdet_getLanguage");
  }

  return ptr(ucsm, status);
}
int32_t ucsdet_getUChars(const UCharsetMatch * ucsm, UChar * buf, int32_t cap, UErrorCode * status) {
  typedef decltype(&ucsdet_getUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_getUChars"));

  if (ptr == nullptr) {
    do_fail("ucsdet_getUChars");
  }

  return ptr(ucsm, buf, cap, status);
}
UEnumeration * ucsdet_getAllDetectableCharsets(const UCharsetDetector * ucsd, UErrorCode * status) {
  typedef decltype(&ucsdet_getAllDetectableCharsets) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_getAllDetectableCharsets"));

  if (ptr == nullptr) {
    do_fail("ucsdet_getAllDetectableCharsets");
  }

  return ptr(ucsd, status);
}
UBool ucsdet_isInputFilterEnabled(const UCharsetDetector * ucsd) {
  typedef decltype(&ucsdet_isInputFilterEnabled) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_isInputFilterEnabled"));

  if (ptr == nullptr) {
    do_fail("ucsdet_isInputFilterEnabled");
  }

  return ptr(ucsd);
}
UBool ucsdet_enableInputFilter(UCharsetDetector * ucsd, UBool filter) {
  typedef decltype(&ucsdet_enableInputFilter) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucsdet_enableInputFilter"));

  if (ptr == nullptr) {
    do_fail("ucsdet_enableInputFilter");
  }

  return ptr(ucsd, filter);
}
UCalendarDateFields udat_toCalendarDateField(UDateFormatField field) {
  typedef decltype(&udat_toCalendarDateField) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_toCalendarDateField"));

  if (ptr == nullptr) {
    do_fail("udat_toCalendarDateField");
  }

  return ptr(field);
}
UDateFormat * udat_open(UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const char * locale, const UChar * tzID, int32_t tzIDLength, const UChar * pattern, int32_t patternLength, UErrorCode * status) {
  typedef decltype(&udat_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_open"));

  if (ptr == nullptr) {
    do_fail("udat_open");
  }

  return ptr(timeStyle, dateStyle, locale, tzID, tzIDLength, pattern, patternLength, status);
}
void udat_close(UDateFormat * format) {
  typedef decltype(&udat_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_close"));

  if (ptr == nullptr) {
    do_fail("udat_close");
  }

  ptr(format);
}
UBool udat_getBooleanAttribute(const UDateFormat * fmt, UDateFormatBooleanAttribute attr, UErrorCode * status) {
  typedef decltype(&udat_getBooleanAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getBooleanAttribute"));

  if (ptr == nullptr) {
    do_fail("udat_getBooleanAttribute");
  }

  return ptr(fmt, attr, status);
}
void udat_setBooleanAttribute(UDateFormat * fmt, UDateFormatBooleanAttribute attr, UBool newValue, UErrorCode * status) {
  typedef decltype(&udat_setBooleanAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_setBooleanAttribute"));

  if (ptr == nullptr) {
    do_fail("udat_setBooleanAttribute");
  }

  ptr(fmt, attr, newValue, status);
}
UDateFormat * udat_clone(const UDateFormat * fmt, UErrorCode * status) {
  typedef decltype(&udat_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_clone"));

  if (ptr == nullptr) {
    do_fail("udat_clone");
  }

  return ptr(fmt, status);
}
int32_t udat_format(const UDateFormat * format, UDate dateToFormat, UChar * result, int32_t resultLength, UFieldPosition * position, UErrorCode * status) {
  typedef decltype(&udat_format) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_format"));

  if (ptr == nullptr) {
    do_fail("udat_format");
  }

  return ptr(format, dateToFormat, result, resultLength, position, status);
}
int32_t udat_formatCalendar(const UDateFormat * format, UCalendar * calendar, UChar * result, int32_t capacity, UFieldPosition * position, UErrorCode * status) {
  typedef decltype(&udat_formatCalendar) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_formatCalendar"));

  if (ptr == nullptr) {
    do_fail("udat_formatCalendar");
  }

  return ptr(format, calendar, result, capacity, position, status);
}
int32_t udat_formatForFields(const UDateFormat * format, UDate dateToFormat, UChar * result, int32_t resultLength, UFieldPositionIterator * fpositer, UErrorCode * status) {
  typedef decltype(&udat_formatForFields) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_formatForFields"));

  if (ptr == nullptr) {
    do_fail("udat_formatForFields");
  }

  return ptr(format, dateToFormat, result, resultLength, fpositer, status);
}
int32_t udat_formatCalendarForFields(const UDateFormat * format, UCalendar * calendar, UChar * result, int32_t capacity, UFieldPositionIterator * fpositer, UErrorCode * status) {
  typedef decltype(&udat_formatCalendarForFields) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_formatCalendarForFields"));

  if (ptr == nullptr) {
    do_fail("udat_formatCalendarForFields");
  }

  return ptr(format, calendar, result, capacity, fpositer, status);
}
UDate udat_parse(const UDateFormat * format, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  typedef decltype(&udat_parse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_parse"));

  if (ptr == nullptr) {
    do_fail("udat_parse");
  }

  return ptr(format, text, textLength, parsePos, status);
}
void udat_parseCalendar(const UDateFormat * format, UCalendar * calendar, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  typedef decltype(&udat_parseCalendar) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_parseCalendar"));

  if (ptr == nullptr) {
    do_fail("udat_parseCalendar");
  }

  ptr(format, calendar, text, textLength, parsePos, status);
}
UBool udat_isLenient(const UDateFormat * fmt) {
  typedef decltype(&udat_isLenient) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_isLenient"));

  if (ptr == nullptr) {
    do_fail("udat_isLenient");
  }

  return ptr(fmt);
}
void udat_setLenient(UDateFormat * fmt, UBool isLenient) {
  typedef decltype(&udat_setLenient) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_setLenient"));

  if (ptr == nullptr) {
    do_fail("udat_setLenient");
  }

  ptr(fmt, isLenient);
}
const UCalendar * udat_getCalendar(const UDateFormat * fmt) {
  typedef decltype(&udat_getCalendar) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getCalendar"));

  if (ptr == nullptr) {
    do_fail("udat_getCalendar");
  }

  return ptr(fmt);
}
void udat_setCalendar(UDateFormat * fmt, const UCalendar * calendarToSet) {
  typedef decltype(&udat_setCalendar) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_setCalendar"));

  if (ptr == nullptr) {
    do_fail("udat_setCalendar");
  }

  ptr(fmt, calendarToSet);
}
const UNumberFormat * udat_getNumberFormat(const UDateFormat * fmt) {
  typedef decltype(&udat_getNumberFormat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getNumberFormat"));

  if (ptr == nullptr) {
    do_fail("udat_getNumberFormat");
  }

  return ptr(fmt);
}
const UNumberFormat * udat_getNumberFormatForField(const UDateFormat * fmt, UChar field) {
  typedef decltype(&udat_getNumberFormatForField) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getNumberFormatForField"));

  if (ptr == nullptr) {
    do_fail("udat_getNumberFormatForField");
  }

  return ptr(fmt, field);
}
void udat_adoptNumberFormatForFields(UDateFormat * fmt, const UChar * fields, UNumberFormat * numberFormatToSet, UErrorCode * status) {
  typedef decltype(&udat_adoptNumberFormatForFields) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_adoptNumberFormatForFields"));

  if (ptr == nullptr) {
    do_fail("udat_adoptNumberFormatForFields");
  }

  ptr(fmt, fields, numberFormatToSet, status);
}
void udat_setNumberFormat(UDateFormat * fmt, const UNumberFormat * numberFormatToSet) {
  typedef decltype(&udat_setNumberFormat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_setNumberFormat"));

  if (ptr == nullptr) {
    do_fail("udat_setNumberFormat");
  }

  ptr(fmt, numberFormatToSet);
}
void udat_adoptNumberFormat(UDateFormat * fmt, UNumberFormat * numberFormatToAdopt) {
  typedef decltype(&udat_adoptNumberFormat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_adoptNumberFormat"));

  if (ptr == nullptr) {
    do_fail("udat_adoptNumberFormat");
  }

  ptr(fmt, numberFormatToAdopt);
}
const char * udat_getAvailable(int32_t localeIndex) {
  typedef decltype(&udat_getAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getAvailable"));

  if (ptr == nullptr) {
    do_fail("udat_getAvailable");
  }

  return ptr(localeIndex);
}
int32_t udat_countAvailable() {
  typedef decltype(&udat_countAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_countAvailable"));

  if (ptr == nullptr) {
    do_fail("udat_countAvailable");
  }

  return ptr();
}
UDate udat_get2DigitYearStart(const UDateFormat * fmt, UErrorCode * status) {
  typedef decltype(&udat_get2DigitYearStart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_get2DigitYearStart"));

  if (ptr == nullptr) {
    do_fail("udat_get2DigitYearStart");
  }

  return ptr(fmt, status);
}
void udat_set2DigitYearStart(UDateFormat * fmt, UDate d, UErrorCode * status) {
  typedef decltype(&udat_set2DigitYearStart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_set2DigitYearStart"));

  if (ptr == nullptr) {
    do_fail("udat_set2DigitYearStart");
  }

  ptr(fmt, d, status);
}
int32_t udat_toPattern(const UDateFormat * fmt, UBool localized, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&udat_toPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_toPattern"));

  if (ptr == nullptr) {
    do_fail("udat_toPattern");
  }

  return ptr(fmt, localized, result, resultLength, status);
}
void udat_applyPattern(UDateFormat * format, UBool localized, const UChar * pattern, int32_t patternLength) {
  typedef decltype(&udat_applyPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_applyPattern"));

  if (ptr == nullptr) {
    do_fail("udat_applyPattern");
  }

  ptr(format, localized, pattern, patternLength);
}
int32_t udat_getSymbols(const UDateFormat * fmt, UDateFormatSymbolType type, int32_t symbolIndex, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&udat_getSymbols) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getSymbols"));

  if (ptr == nullptr) {
    do_fail("udat_getSymbols");
  }

  return ptr(fmt, type, symbolIndex, result, resultLength, status);
}
int32_t udat_countSymbols(const UDateFormat * fmt, UDateFormatSymbolType type) {
  typedef decltype(&udat_countSymbols) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_countSymbols"));

  if (ptr == nullptr) {
    do_fail("udat_countSymbols");
  }

  return ptr(fmt, type);
}
void udat_setSymbols(UDateFormat * format, UDateFormatSymbolType type, int32_t symbolIndex, UChar * value, int32_t valueLength, UErrorCode * status) {
  typedef decltype(&udat_setSymbols) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_setSymbols"));

  if (ptr == nullptr) {
    do_fail("udat_setSymbols");
  }

  ptr(format, type, symbolIndex, value, valueLength, status);
}
const char * udat_getLocaleByType(const UDateFormat * fmt, ULocDataLocaleType type, UErrorCode * status) {
  typedef decltype(&udat_getLocaleByType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getLocaleByType"));

  if (ptr == nullptr) {
    do_fail("udat_getLocaleByType");
  }

  return ptr(fmt, type, status);
}
void udat_setContext(UDateFormat * fmt, UDisplayContext value, UErrorCode * status) {
  typedef decltype(&udat_setContext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_setContext"));

  if (ptr == nullptr) {
    do_fail("udat_setContext");
  }

  ptr(fmt, value, status);
}
UDisplayContext udat_getContext(const UDateFormat * fmt, UDisplayContextType type, UErrorCode * status) {
  typedef decltype(&udat_getContext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udat_getContext"));

  if (ptr == nullptr) {
    do_fail("udat_getContext");
  }

  return ptr(fmt, type, status);
}
ULocaleData * ulocdata_open(const char * localeID, UErrorCode * status) {
  typedef decltype(&ulocdata_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_open"));

  if (ptr == nullptr) {
    do_fail("ulocdata_open");
  }

  return ptr(localeID, status);
}
void ulocdata_close(ULocaleData * uld) {
  typedef decltype(&ulocdata_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_close"));

  if (ptr == nullptr) {
    do_fail("ulocdata_close");
  }

  ptr(uld);
}
void ulocdata_setNoSubstitute(ULocaleData * uld, UBool setting) {
  typedef decltype(&ulocdata_setNoSubstitute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_setNoSubstitute"));

  if (ptr == nullptr) {
    do_fail("ulocdata_setNoSubstitute");
  }

  ptr(uld, setting);
}
UBool ulocdata_getNoSubstitute(ULocaleData * uld) {
  typedef decltype(&ulocdata_getNoSubstitute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getNoSubstitute"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getNoSubstitute");
  }

  return ptr(uld);
}
USet * ulocdata_getExemplarSet(ULocaleData * uld, USet * fillIn, uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode * status) {
  typedef decltype(&ulocdata_getExemplarSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getExemplarSet"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getExemplarSet");
  }

  return ptr(uld, fillIn, options, extype, status);
}
int32_t ulocdata_getDelimiter(ULocaleData * uld, ULocaleDataDelimiterType type, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&ulocdata_getDelimiter) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getDelimiter"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getDelimiter");
  }

  return ptr(uld, type, result, resultLength, status);
}
UMeasurementSystem ulocdata_getMeasurementSystem(const char * localeID, UErrorCode * status) {
  typedef decltype(&ulocdata_getMeasurementSystem) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getMeasurementSystem"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getMeasurementSystem");
  }

  return ptr(localeID, status);
}
void ulocdata_getPaperSize(const char * localeID, int32_t * height, int32_t * width, UErrorCode * status) {
  typedef decltype(&ulocdata_getPaperSize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getPaperSize"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getPaperSize");
  }

  ptr(localeID, height, width, status);
}
void ulocdata_getCLDRVersion(UVersionInfo versionArray, UErrorCode * status) {
  typedef decltype(&ulocdata_getCLDRVersion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getCLDRVersion"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getCLDRVersion");
  }

  ptr(versionArray, status);
}
int32_t ulocdata_getLocaleDisplayPattern(ULocaleData * uld, UChar * pattern, int32_t patternCapacity, UErrorCode * status) {
  typedef decltype(&ulocdata_getLocaleDisplayPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getLocaleDisplayPattern"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getLocaleDisplayPattern");
  }

  return ptr(uld, pattern, patternCapacity, status);
}
int32_t ulocdata_getLocaleSeparator(ULocaleData * uld, UChar * separator, int32_t separatorCapacity, UErrorCode * status) {
  typedef decltype(&ulocdata_getLocaleSeparator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ulocdata_getLocaleSeparator"));

  if (ptr == nullptr) {
    do_fail("ulocdata_getLocaleSeparator");
  }

  return ptr(uld, separator, separatorCapacity, status);
}
UDateIntervalFormat * udtitvfmt_open(const char * locale, const UChar * skeleton, int32_t skeletonLength, const UChar * tzID, int32_t tzIDLength, UErrorCode * status) {
  typedef decltype(&udtitvfmt_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udtitvfmt_open"));

  if (ptr == nullptr) {
    do_fail("udtitvfmt_open");
  }

  return ptr(locale, skeleton, skeletonLength, tzID, tzIDLength, status);
}
void udtitvfmt_close(UDateIntervalFormat * formatter) {
  typedef decltype(&udtitvfmt_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udtitvfmt_close"));

  if (ptr == nullptr) {
    do_fail("udtitvfmt_close");
  }

  ptr(formatter);
}
int32_t udtitvfmt_format(const UDateIntervalFormat * formatter, UDate fromDate, UDate toDate, UChar * result, int32_t resultCapacity, UFieldPosition * position, UErrorCode * status) {
  typedef decltype(&udtitvfmt_format) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "udtitvfmt_format"));

  if (ptr == nullptr) {
    do_fail("udtitvfmt_format");
  }

  return ptr(formatter, fromDate, toDate, result, resultCapacity, position, status);
}
UCollationElements * ucol_openElements(const UCollator * coll, const UChar * text, int32_t textLength, UErrorCode * status) {
  typedef decltype(&ucol_openElements) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_openElements"));

  if (ptr == nullptr) {
    do_fail("ucol_openElements");
  }

  return ptr(coll, text, textLength, status);
}
int32_t ucol_keyHashCode(const uint8_t * key, int32_t length) {
  typedef decltype(&ucol_keyHashCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_keyHashCode"));

  if (ptr == nullptr) {
    do_fail("ucol_keyHashCode");
  }

  return ptr(key, length);
}
void ucol_closeElements(UCollationElements * elems) {
  typedef decltype(&ucol_closeElements) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_closeElements"));

  if (ptr == nullptr) {
    do_fail("ucol_closeElements");
  }

  ptr(elems);
}
void ucol_reset(UCollationElements * elems) {
  typedef decltype(&ucol_reset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_reset"));

  if (ptr == nullptr) {
    do_fail("ucol_reset");
  }

  ptr(elems);
}
int32_t ucol_next(UCollationElements * elems, UErrorCode * status) {
  typedef decltype(&ucol_next) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_next"));

  if (ptr == nullptr) {
    do_fail("ucol_next");
  }

  return ptr(elems, status);
}
int32_t ucol_previous(UCollationElements * elems, UErrorCode * status) {
  typedef decltype(&ucol_previous) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_previous"));

  if (ptr == nullptr) {
    do_fail("ucol_previous");
  }

  return ptr(elems, status);
}
int32_t ucol_getMaxExpansion(const UCollationElements * elems, int32_t order) {
  typedef decltype(&ucol_getMaxExpansion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getMaxExpansion"));

  if (ptr == nullptr) {
    do_fail("ucol_getMaxExpansion");
  }

  return ptr(elems, order);
}
void ucol_setText(UCollationElements * elems, const UChar * text, int32_t textLength, UErrorCode * status) {
  typedef decltype(&ucol_setText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_setText"));

  if (ptr == nullptr) {
    do_fail("ucol_setText");
  }

  ptr(elems, text, textLength, status);
}
int32_t ucol_getOffset(const UCollationElements * elems) {
  typedef decltype(&ucol_getOffset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_getOffset"));

  if (ptr == nullptr) {
    do_fail("ucol_getOffset");
  }

  return ptr(elems);
}
void ucol_setOffset(UCollationElements * elems, int32_t offset, UErrorCode * status) {
  typedef decltype(&ucol_setOffset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_setOffset"));

  if (ptr == nullptr) {
    do_fail("ucol_setOffset");
  }

  ptr(elems, offset, status);
}
int32_t ucol_primaryOrder(int32_t order) {
  typedef decltype(&ucol_primaryOrder) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_primaryOrder"));

  if (ptr == nullptr) {
    do_fail("ucol_primaryOrder");
  }

  return ptr(order);
}
int32_t ucol_secondaryOrder(int32_t order) {
  typedef decltype(&ucol_secondaryOrder) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_secondaryOrder"));

  if (ptr == nullptr) {
    do_fail("ucol_secondaryOrder");
  }

  return ptr(order);
}
int32_t ucol_tertiaryOrder(int32_t order) {
  typedef decltype(&ucol_tertiaryOrder) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ucol_tertiaryOrder"));

  if (ptr == nullptr) {
    do_fail("ucol_tertiaryOrder");
  }

  return ptr(order);
}
const UGenderInfo * ugender_getInstance(const char * locale, UErrorCode * status) {
  typedef decltype(&ugender_getInstance) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ugender_getInstance"));

  if (ptr == nullptr) {
    do_fail("ugender_getInstance");
  }

  return ptr(locale, status);
}
UGender ugender_getListGender(const UGenderInfo * genderinfo, const UGender * genders, int32_t size, UErrorCode * status) {
  typedef decltype(&ugender_getListGender) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "ugender_getListGender"));

  if (ptr == nullptr) {
    do_fail("ugender_getListGender");
  }

  return ptr(genderinfo, genders, size, status);
}
UTransliterator * utrans_openU(const UChar * id, int32_t idLength, UTransDirection dir, const UChar * rules, int32_t rulesLength, UParseError * parseError, UErrorCode * pErrorCode) {
  typedef decltype(&utrans_openU) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_openU"));

  if (ptr == nullptr) {
    do_fail("utrans_openU");
  }

  return ptr(id, idLength, dir, rules, rulesLength, parseError, pErrorCode);
}
UTransliterator * utrans_openInverse(const UTransliterator * trans, UErrorCode * status) {
  typedef decltype(&utrans_openInverse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_openInverse"));

  if (ptr == nullptr) {
    do_fail("utrans_openInverse");
  }

  return ptr(trans, status);
}
UTransliterator * utrans_clone(const UTransliterator * trans, UErrorCode * status) {
  typedef decltype(&utrans_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_clone"));

  if (ptr == nullptr) {
    do_fail("utrans_clone");
  }

  return ptr(trans, status);
}
void utrans_close(UTransliterator * trans) {
  typedef decltype(&utrans_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_close"));

  if (ptr == nullptr) {
    do_fail("utrans_close");
  }

  ptr(trans);
}
const UChar * utrans_getUnicodeID(const UTransliterator * trans, int32_t * resultLength) {
  typedef decltype(&utrans_getUnicodeID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_getUnicodeID"));

  if (ptr == nullptr) {
    do_fail("utrans_getUnicodeID");
  }

  return ptr(trans, resultLength);
}
void utrans_register(UTransliterator * adoptedTrans, UErrorCode * status) {
  typedef decltype(&utrans_register) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_register"));

  if (ptr == nullptr) {
    do_fail("utrans_register");
  }

  ptr(adoptedTrans, status);
}
void utrans_unregisterID(const UChar * id, int32_t idLength) {
  typedef decltype(&utrans_unregisterID) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_unregisterID"));

  if (ptr == nullptr) {
    do_fail("utrans_unregisterID");
  }

  ptr(id, idLength);
}
void utrans_setFilter(UTransliterator * trans, const UChar * filterPattern, int32_t filterPatternLen, UErrorCode * status) {
  typedef decltype(&utrans_setFilter) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_setFilter"));

  if (ptr == nullptr) {
    do_fail("utrans_setFilter");
  }

  ptr(trans, filterPattern, filterPatternLen, status);
}
int32_t utrans_countAvailableIDs() {
  typedef decltype(&utrans_countAvailableIDs) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_countAvailableIDs"));

  if (ptr == nullptr) {
    do_fail("utrans_countAvailableIDs");
  }

  return ptr();
}
UEnumeration * utrans_openIDs(UErrorCode * pErrorCode) {
  typedef decltype(&utrans_openIDs) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_openIDs"));

  if (ptr == nullptr) {
    do_fail("utrans_openIDs");
  }

  return ptr(pErrorCode);
}
void utrans_trans(const UTransliterator * trans, UReplaceable * rep, UReplaceableCallbacks * repFunc, int32_t start, int32_t * limit, UErrorCode * status) {
  typedef decltype(&utrans_trans) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_trans"));

  if (ptr == nullptr) {
    do_fail("utrans_trans");
  }

  ptr(trans, rep, repFunc, start, limit, status);
}
void utrans_transIncremental(const UTransliterator * trans, UReplaceable * rep, UReplaceableCallbacks * repFunc, UTransPosition * pos, UErrorCode * status) {
  typedef decltype(&utrans_transIncremental) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_transIncremental"));

  if (ptr == nullptr) {
    do_fail("utrans_transIncremental");
  }

  ptr(trans, rep, repFunc, pos, status);
}
void utrans_transUChars(const UTransliterator * trans, UChar * text, int32_t * textLength, int32_t textCapacity, int32_t start, int32_t * limit, UErrorCode * status) {
  typedef decltype(&utrans_transUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_transUChars"));

  if (ptr == nullptr) {
    do_fail("utrans_transUChars");
  }

  ptr(trans, text, textLength, textCapacity, start, limit, status);
}
void utrans_transIncrementalUChars(const UTransliterator * trans, UChar * text, int32_t * textLength, int32_t textCapacity, UTransPosition * pos, UErrorCode * status) {
  typedef decltype(&utrans_transIncrementalUChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_transIncrementalUChars"));

  if (ptr == nullptr) {
    do_fail("utrans_transIncrementalUChars");
  }

  ptr(trans, text, textLength, textCapacity, pos, status);
}
int32_t utrans_toRules(const UTransliterator * trans, UBool escapeUnprintable, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&utrans_toRules) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_toRules"));

  if (ptr == nullptr) {
    do_fail("utrans_toRules");
  }

  return ptr(trans, escapeUnprintable, result, resultLength, status);
}
USet * utrans_getSourceSet(const UTransliterator * trans, UBool ignoreFilter, USet * fillIn, UErrorCode * status) {
  typedef decltype(&utrans_getSourceSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "utrans_getSourceSet"));

  if (ptr == nullptr) {
    do_fail("utrans_getSourceSet");
  }

  return ptr(trans, ignoreFilter, fillIn, status);
}
UPluralRules * uplrules_open(const char * locale, UErrorCode * status) {
  typedef decltype(&uplrules_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uplrules_open"));

  if (ptr == nullptr) {
    do_fail("uplrules_open");
  }

  return ptr(locale, status);
}
UPluralRules * uplrules_openForType(const char * locale, UPluralType type, UErrorCode * status) {
  typedef decltype(&uplrules_openForType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uplrules_openForType"));

  if (ptr == nullptr) {
    do_fail("uplrules_openForType");
  }

  return ptr(locale, type, status);
}
void uplrules_close(UPluralRules * uplrules) {
  typedef decltype(&uplrules_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uplrules_close"));

  if (ptr == nullptr) {
    do_fail("uplrules_close");
  }

  ptr(uplrules);
}
int32_t uplrules_select(const UPluralRules * uplrules, double number, UChar * keyword, int32_t capacity, UErrorCode * status) {
  typedef decltype(&uplrules_select) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uplrules_select"));

  if (ptr == nullptr) {
    do_fail("uplrules_select");
  }

  return ptr(uplrules, number, keyword, capacity, status);
}
int32_t u_formatMessage(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UErrorCode * status, ...) {
  typedef decltype(&u_vformatMessage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vformatMessage"));

  if (ptr == nullptr) {
    do_fail("u_formatMessage");
  }

  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, args, status);
  va_end(args);
  return ret;
}
int32_t u_vformatMessage(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, va_list ap, UErrorCode * status) {
  typedef decltype(&u_vformatMessage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vformatMessage"));

  if (ptr == nullptr) {
    do_fail("u_vformatMessage");
  }

  return ptr(locale, pattern, patternLength, result, resultLength, ap, status);
}
void u_parseMessage(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, UErrorCode * status, ...) {
  typedef decltype(&u_vparseMessage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vparseMessage"));

  if (ptr == nullptr) {
    do_fail("u_parseMessage");
  }

  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, status);
  va_end(args);
  return;
}
void u_vparseMessage(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, va_list ap, UErrorCode * status) {
  typedef decltype(&u_vparseMessage) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vparseMessage"));

  if (ptr == nullptr) {
    do_fail("u_vparseMessage");
  }

  ptr(locale, pattern, patternLength, source, sourceLength, ap, status);
}
int32_t u_formatMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UParseError * parseError, UErrorCode * status, ...) {
  typedef decltype(&u_vformatMessageWithError) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vformatMessageWithError"));

  if (ptr == nullptr) {
    do_fail("u_formatMessageWithError");
  }

  va_list args;
  va_start(args, status);
  int32_t ret = ptr(locale, pattern, patternLength, result, resultLength, parseError, args, status);
  va_end(args);
  return ret;
}
int32_t u_vformatMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, UChar * result, int32_t resultLength, UParseError * parseError, va_list ap, UErrorCode * status) {
  typedef decltype(&u_vformatMessageWithError) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vformatMessageWithError"));

  if (ptr == nullptr) {
    do_fail("u_vformatMessageWithError");
  }

  return ptr(locale, pattern, patternLength, result, resultLength, parseError, ap, status);
}
void u_parseMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, UParseError * parseError, UErrorCode * status, ...) {
  typedef decltype(&u_vparseMessageWithError) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vparseMessageWithError"));

  if (ptr == nullptr) {
    do_fail("u_parseMessageWithError");
  }

  va_list args;
  va_start(args, status);
  ptr(locale, pattern, patternLength, source, sourceLength, args, parseError, status);
  va_end(args);
  return;
}
void u_vparseMessageWithError(const char * locale, const UChar * pattern, int32_t patternLength, const UChar * source, int32_t sourceLength, va_list ap, UParseError * parseError, UErrorCode * status) {
  typedef decltype(&u_vparseMessageWithError) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "u_vparseMessageWithError"));

  if (ptr == nullptr) {
    do_fail("u_vparseMessageWithError");
  }

  ptr(locale, pattern, patternLength, source, sourceLength, ap, parseError, status);
}
UMessageFormat * umsg_open(const UChar * pattern, int32_t patternLength, const char * locale, UParseError * parseError, UErrorCode * status) {
  typedef decltype(&umsg_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_open"));

  if (ptr == nullptr) {
    do_fail("umsg_open");
  }

  return ptr(pattern, patternLength, locale, parseError, status);
}
void umsg_close(UMessageFormat * format) {
  typedef decltype(&umsg_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_close"));

  if (ptr == nullptr) {
    do_fail("umsg_close");
  }

  ptr(format);
}
UMessageFormat umsg_clone(const UMessageFormat * fmt, UErrorCode * status) {
  typedef decltype(&umsg_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_clone"));

  if (ptr == nullptr) {
    do_fail("umsg_clone");
  }

  return ptr(fmt, status);
}
void umsg_setLocale(UMessageFormat * fmt, const char * locale) {
  typedef decltype(&umsg_setLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_setLocale"));

  if (ptr == nullptr) {
    do_fail("umsg_setLocale");
  }

  ptr(fmt, locale);
}
const char * umsg_getLocale(const UMessageFormat * fmt) {
  typedef decltype(&umsg_getLocale) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_getLocale"));

  if (ptr == nullptr) {
    do_fail("umsg_getLocale");
  }

  return ptr(fmt);
}
void umsg_applyPattern(UMessageFormat * fmt, const UChar * pattern, int32_t patternLength, UParseError * parseError, UErrorCode * status) {
  typedef decltype(&umsg_applyPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_applyPattern"));

  if (ptr == nullptr) {
    do_fail("umsg_applyPattern");
  }

  ptr(fmt, pattern, patternLength, parseError, status);
}
int32_t umsg_toPattern(const UMessageFormat * fmt, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&umsg_toPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_toPattern"));

  if (ptr == nullptr) {
    do_fail("umsg_toPattern");
  }

  return ptr(fmt, result, resultLength, status);
}
int32_t umsg_format(const UMessageFormat * fmt, UChar * result, int32_t resultLength, UErrorCode * status, ...) {
  typedef decltype(&umsg_vformat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_vformat"));

  if (ptr == nullptr) {
    do_fail("umsg_format");
  }

  va_list args;
  va_start(args, status);
  int32_t ret = ptr(fmt, result, resultLength, args, status);
  va_end(args);
  return ret;
}
int32_t umsg_vformat(const UMessageFormat * fmt, UChar * result, int32_t resultLength, va_list ap, UErrorCode * status) {
  typedef decltype(&umsg_vformat) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_vformat"));

  if (ptr == nullptr) {
    do_fail("umsg_vformat");
  }

  return ptr(fmt, result, resultLength, ap, status);
}
void umsg_parse(const UMessageFormat * fmt, const UChar * source, int32_t sourceLength, int32_t * count, UErrorCode * status, ...) {
  typedef decltype(&umsg_vparse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_vparse"));

  if (ptr == nullptr) {
    do_fail("umsg_parse");
  }

  va_list args;
  va_start(args, status);
  ptr(fmt, source, sourceLength, count, args, status);
  va_end(args);
  return;
}
void umsg_vparse(const UMessageFormat * fmt, const UChar * source, int32_t sourceLength, int32_t * count, va_list ap, UErrorCode * status) {
  typedef decltype(&umsg_vparse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_vparse"));

  if (ptr == nullptr) {
    do_fail("umsg_vparse");
  }

  ptr(fmt, source, sourceLength, count, ap, status);
}
int32_t umsg_autoQuoteApostrophe(const UChar * pattern, int32_t patternLength, UChar * dest, int32_t destCapacity, UErrorCode * ec) {
  typedef decltype(&umsg_autoQuoteApostrophe) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "umsg_autoQuoteApostrophe"));

  if (ptr == nullptr) {
    do_fail("umsg_autoQuoteApostrophe");
  }

  return ptr(pattern, patternLength, dest, destCapacity, ec);
}
const URegion * uregion_getRegionFromCode(const char * regionCode, UErrorCode * status) {
  typedef decltype(&uregion_getRegionFromCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getRegionFromCode"));

  if (ptr == nullptr) {
    do_fail("uregion_getRegionFromCode");
  }

  return ptr(regionCode, status);
}
const URegion * uregion_getRegionFromNumericCode(int32_t code, UErrorCode * status) {
  typedef decltype(&uregion_getRegionFromNumericCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getRegionFromNumericCode"));

  if (ptr == nullptr) {
    do_fail("uregion_getRegionFromNumericCode");
  }

  return ptr(code, status);
}
UEnumeration * uregion_getAvailable(URegionType type, UErrorCode * status) {
  typedef decltype(&uregion_getAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getAvailable"));

  if (ptr == nullptr) {
    do_fail("uregion_getAvailable");
  }

  return ptr(type, status);
}
UBool uregion_areEqual(const URegion * uregion, const URegion * otherRegion) {
  typedef decltype(&uregion_areEqual) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_areEqual"));

  if (ptr == nullptr) {
    do_fail("uregion_areEqual");
  }

  return ptr(uregion, otherRegion);
}
const URegion * uregion_getContainingRegion(const URegion * uregion) {
  typedef decltype(&uregion_getContainingRegion) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getContainingRegion"));

  if (ptr == nullptr) {
    do_fail("uregion_getContainingRegion");
  }

  return ptr(uregion);
}
const URegion * uregion_getContainingRegionOfType(const URegion * uregion, URegionType type) {
  typedef decltype(&uregion_getContainingRegionOfType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getContainingRegionOfType"));

  if (ptr == nullptr) {
    do_fail("uregion_getContainingRegionOfType");
  }

  return ptr(uregion, type);
}
UEnumeration * uregion_getContainedRegions(const URegion * uregion, UErrorCode * status) {
  typedef decltype(&uregion_getContainedRegions) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getContainedRegions"));

  if (ptr == nullptr) {
    do_fail("uregion_getContainedRegions");
  }

  return ptr(uregion, status);
}
UEnumeration * uregion_getContainedRegionsOfType(const URegion * uregion, URegionType type, UErrorCode * status) {
  typedef decltype(&uregion_getContainedRegionsOfType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getContainedRegionsOfType"));

  if (ptr == nullptr) {
    do_fail("uregion_getContainedRegionsOfType");
  }

  return ptr(uregion, type, status);
}
UBool uregion_contains(const URegion * uregion, const URegion * otherRegion) {
  typedef decltype(&uregion_contains) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_contains"));

  if (ptr == nullptr) {
    do_fail("uregion_contains");
  }

  return ptr(uregion, otherRegion);
}
UEnumeration * uregion_getPreferredValues(const URegion * uregion, UErrorCode * status) {
  typedef decltype(&uregion_getPreferredValues) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getPreferredValues"));

  if (ptr == nullptr) {
    do_fail("uregion_getPreferredValues");
  }

  return ptr(uregion, status);
}
const char * uregion_getRegionCode(const URegion * uregion) {
  typedef decltype(&uregion_getRegionCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getRegionCode"));

  if (ptr == nullptr) {
    do_fail("uregion_getRegionCode");
  }

  return ptr(uregion);
}
int32_t uregion_getNumericCode(const URegion * uregion) {
  typedef decltype(&uregion_getNumericCode) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getNumericCode"));

  if (ptr == nullptr) {
    do_fail("uregion_getNumericCode");
  }

  return ptr(uregion);
}
URegionType uregion_getType(const URegion * uregion) {
  typedef decltype(&uregion_getType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uregion_getType"));

  if (ptr == nullptr) {
    do_fail("uregion_getType");
  }

  return ptr(uregion);
}
UNumberFormat * unum_open(UNumberFormatStyle style, const UChar * pattern, int32_t patternLength, const char * locale, UParseError * parseErr, UErrorCode * status) {
  typedef decltype(&unum_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_open"));

  if (ptr == nullptr) {
    do_fail("unum_open");
  }

  return ptr(style, pattern, patternLength, locale, parseErr, status);
}
void unum_close(UNumberFormat * fmt) {
  typedef decltype(&unum_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_close"));

  if (ptr == nullptr) {
    do_fail("unum_close");
  }

  ptr(fmt);
}
UNumberFormat * unum_clone(const UNumberFormat * fmt, UErrorCode * status) {
  typedef decltype(&unum_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_clone"));

  if (ptr == nullptr) {
    do_fail("unum_clone");
  }

  return ptr(fmt, status);
}
int32_t unum_format(const UNumberFormat * fmt, int32_t number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  typedef decltype(&unum_format) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_format"));

  if (ptr == nullptr) {
    do_fail("unum_format");
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatInt64(const UNumberFormat * fmt, int64_t number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  typedef decltype(&unum_formatInt64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_formatInt64"));

  if (ptr == nullptr) {
    do_fail("unum_formatInt64");
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatDouble(const UNumberFormat * fmt, double number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  typedef decltype(&unum_formatDouble) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_formatDouble"));

  if (ptr == nullptr) {
    do_fail("unum_formatDouble");
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_formatDecimal(const UNumberFormat * fmt, const char * number, int32_t length, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  typedef decltype(&unum_formatDecimal) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_formatDecimal"));

  if (ptr == nullptr) {
    do_fail("unum_formatDecimal");
  }

  return ptr(fmt, number, length, result, resultLength, pos, status);
}
int32_t unum_formatDoubleCurrency(const UNumberFormat * fmt, double number, UChar * currency, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  typedef decltype(&unum_formatDoubleCurrency) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_formatDoubleCurrency"));

  if (ptr == nullptr) {
    do_fail("unum_formatDoubleCurrency");
  }

  return ptr(fmt, number, currency, result, resultLength, pos, status);
}
int32_t unum_formatUFormattable(const UNumberFormat * fmt, const UFormattable * number, UChar * result, int32_t resultLength, UFieldPosition * pos, UErrorCode * status) {
  typedef decltype(&unum_formatUFormattable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_formatUFormattable"));

  if (ptr == nullptr) {
    do_fail("unum_formatUFormattable");
  }

  return ptr(fmt, number, result, resultLength, pos, status);
}
int32_t unum_parse(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  typedef decltype(&unum_parse) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_parse"));

  if (ptr == nullptr) {
    do_fail("unum_parse");
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
int64_t unum_parseInt64(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  typedef decltype(&unum_parseInt64) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_parseInt64"));

  if (ptr == nullptr) {
    do_fail("unum_parseInt64");
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
double unum_parseDouble(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  typedef decltype(&unum_parseDouble) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_parseDouble"));

  if (ptr == nullptr) {
    do_fail("unum_parseDouble");
  }

  return ptr(fmt, text, textLength, parsePos, status);
}
int32_t unum_parseDecimal(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, char * outBuf, int32_t outBufLength, UErrorCode * status) {
  typedef decltype(&unum_parseDecimal) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_parseDecimal"));

  if (ptr == nullptr) {
    do_fail("unum_parseDecimal");
  }

  return ptr(fmt, text, textLength, parsePos, outBuf, outBufLength, status);
}
double unum_parseDoubleCurrency(const UNumberFormat * fmt, const UChar * text, int32_t textLength, int32_t * parsePos, UChar * currency, UErrorCode * status) {
  typedef decltype(&unum_parseDoubleCurrency) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_parseDoubleCurrency"));

  if (ptr == nullptr) {
    do_fail("unum_parseDoubleCurrency");
  }

  return ptr(fmt, text, textLength, parsePos, currency, status);
}
UFormattable * unum_parseToUFormattable(const UNumberFormat * fmt, UFormattable * result, const UChar * text, int32_t textLength, int32_t * parsePos, UErrorCode * status) {
  typedef decltype(&unum_parseToUFormattable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_parseToUFormattable"));

  if (ptr == nullptr) {
    do_fail("unum_parseToUFormattable");
  }

  return ptr(fmt, result, text, textLength, parsePos, status);
}
void unum_applyPattern(UNumberFormat * format, UBool localized, const UChar * pattern, int32_t patternLength, UParseError * parseError, UErrorCode * status) {
  typedef decltype(&unum_applyPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_applyPattern"));

  if (ptr == nullptr) {
    do_fail("unum_applyPattern");
  }

  ptr(format, localized, pattern, patternLength, parseError, status);
}
const char * unum_getAvailable(int32_t localeIndex) {
  typedef decltype(&unum_getAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_getAvailable"));

  if (ptr == nullptr) {
    do_fail("unum_getAvailable");
  }

  return ptr(localeIndex);
}
int32_t unum_countAvailable() {
  typedef decltype(&unum_countAvailable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_countAvailable"));

  if (ptr == nullptr) {
    do_fail("unum_countAvailable");
  }

  return ptr();
}
int32_t unum_getAttribute(const UNumberFormat * fmt, UNumberFormatAttribute attr) {
  typedef decltype(&unum_getAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_getAttribute"));

  if (ptr == nullptr) {
    do_fail("unum_getAttribute");
  }

  return ptr(fmt, attr);
}
void unum_setAttribute(UNumberFormat * fmt, UNumberFormatAttribute attr, int32_t newValue) {
  typedef decltype(&unum_setAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_setAttribute"));

  if (ptr == nullptr) {
    do_fail("unum_setAttribute");
  }

  ptr(fmt, attr, newValue);
}
double unum_getDoubleAttribute(const UNumberFormat * fmt, UNumberFormatAttribute attr) {
  typedef decltype(&unum_getDoubleAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_getDoubleAttribute"));

  if (ptr == nullptr) {
    do_fail("unum_getDoubleAttribute");
  }

  return ptr(fmt, attr);
}
void unum_setDoubleAttribute(UNumberFormat * fmt, UNumberFormatAttribute attr, double newValue) {
  typedef decltype(&unum_setDoubleAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_setDoubleAttribute"));

  if (ptr == nullptr) {
    do_fail("unum_setDoubleAttribute");
  }

  ptr(fmt, attr, newValue);
}
int32_t unum_getTextAttribute(const UNumberFormat * fmt, UNumberFormatTextAttribute tag, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&unum_getTextAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_getTextAttribute"));

  if (ptr == nullptr) {
    do_fail("unum_getTextAttribute");
  }

  return ptr(fmt, tag, result, resultLength, status);
}
void unum_setTextAttribute(UNumberFormat * fmt, UNumberFormatTextAttribute tag, const UChar * newValue, int32_t newValueLength, UErrorCode * status) {
  typedef decltype(&unum_setTextAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_setTextAttribute"));

  if (ptr == nullptr) {
    do_fail("unum_setTextAttribute");
  }

  ptr(fmt, tag, newValue, newValueLength, status);
}
int32_t unum_toPattern(const UNumberFormat * fmt, UBool isPatternLocalized, UChar * result, int32_t resultLength, UErrorCode * status) {
  typedef decltype(&unum_toPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_toPattern"));

  if (ptr == nullptr) {
    do_fail("unum_toPattern");
  }

  return ptr(fmt, isPatternLocalized, result, resultLength, status);
}
int32_t unum_getSymbol(const UNumberFormat * fmt, UNumberFormatSymbol symbol, UChar * buffer, int32_t size, UErrorCode * status) {
  typedef decltype(&unum_getSymbol) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_getSymbol"));

  if (ptr == nullptr) {
    do_fail("unum_getSymbol");
  }

  return ptr(fmt, symbol, buffer, size, status);
}
void unum_setSymbol(UNumberFormat * fmt, UNumberFormatSymbol symbol, const UChar * value, int32_t length, UErrorCode * status) {
  typedef decltype(&unum_setSymbol) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_setSymbol"));

  if (ptr == nullptr) {
    do_fail("unum_setSymbol");
  }

  ptr(fmt, symbol, value, length, status);
}
const char * unum_getLocaleByType(const UNumberFormat * fmt, ULocDataLocaleType type, UErrorCode * status) {
  typedef decltype(&unum_getLocaleByType) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_getLocaleByType"));

  if (ptr == nullptr) {
    do_fail("unum_getLocaleByType");
  }

  return ptr(fmt, type, status);
}
void unum_setContext(UNumberFormat * fmt, UDisplayContext value, UErrorCode * status) {
  typedef decltype(&unum_setContext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_setContext"));

  if (ptr == nullptr) {
    do_fail("unum_setContext");
  }

  ptr(fmt, value, status);
}
UDisplayContext unum_getContext(const UNumberFormat * fmt, UDisplayContextType type, UErrorCode * status) {
  typedef decltype(&unum_getContext) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "unum_getContext"));

  if (ptr == nullptr) {
    do_fail("unum_getContext");
  }

  return ptr(fmt, type, status);
}
USpoofChecker * uspoof_open(UErrorCode * status) {
  typedef decltype(&uspoof_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_open"));

  if (ptr == nullptr) {
    do_fail("uspoof_open");
  }

  return ptr(status);
}
USpoofChecker * uspoof_openFromSerialized(const void * data, int32_t length, int32_t * pActualLength, UErrorCode * pErrorCode) {
  typedef decltype(&uspoof_openFromSerialized) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_openFromSerialized"));

  if (ptr == nullptr) {
    do_fail("uspoof_openFromSerialized");
  }

  return ptr(data, length, pActualLength, pErrorCode);
}
USpoofChecker * uspoof_openFromSource(const char * confusables, int32_t confusablesLen, const char * confusablesWholeScript, int32_t confusablesWholeScriptLen, int32_t * errType, UParseError * pe, UErrorCode * status) {
  typedef decltype(&uspoof_openFromSource) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_openFromSource"));

  if (ptr == nullptr) {
    do_fail("uspoof_openFromSource");
  }

  return ptr(confusables, confusablesLen, confusablesWholeScript, confusablesWholeScriptLen, errType, pe, status);
}
void uspoof_close(USpoofChecker * sc) {
  typedef decltype(&uspoof_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_close"));

  if (ptr == nullptr) {
    do_fail("uspoof_close");
  }

  ptr(sc);
}
USpoofChecker * uspoof_clone(const USpoofChecker * sc, UErrorCode * status) {
  typedef decltype(&uspoof_clone) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_clone"));

  if (ptr == nullptr) {
    do_fail("uspoof_clone");
  }

  return ptr(sc, status);
}
void uspoof_setChecks(USpoofChecker * sc, int32_t checks, UErrorCode * status) {
  typedef decltype(&uspoof_setChecks) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_setChecks"));

  if (ptr == nullptr) {
    do_fail("uspoof_setChecks");
  }

  ptr(sc, checks, status);
}
int32_t uspoof_getChecks(const USpoofChecker * sc, UErrorCode * status) {
  typedef decltype(&uspoof_getChecks) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getChecks"));

  if (ptr == nullptr) {
    do_fail("uspoof_getChecks");
  }

  return ptr(sc, status);
}
void uspoof_setRestrictionLevel(USpoofChecker * sc, URestrictionLevel restrictionLevel) {
  typedef decltype(&uspoof_setRestrictionLevel) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_setRestrictionLevel"));

  if (ptr == nullptr) {
    do_fail("uspoof_setRestrictionLevel");
  }

  ptr(sc, restrictionLevel);
}
URestrictionLevel uspoof_getRestrictionLevel(const USpoofChecker * sc) {
  typedef decltype(&uspoof_getRestrictionLevel) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getRestrictionLevel"));

  if (ptr == nullptr) {
    do_fail("uspoof_getRestrictionLevel");
  }

  return ptr(sc);
}
void uspoof_setAllowedLocales(USpoofChecker * sc, const char * localesList, UErrorCode * status) {
  typedef decltype(&uspoof_setAllowedLocales) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_setAllowedLocales"));

  if (ptr == nullptr) {
    do_fail("uspoof_setAllowedLocales");
  }

  ptr(sc, localesList, status);
}
const char * uspoof_getAllowedLocales(USpoofChecker * sc, UErrorCode * status) {
  typedef decltype(&uspoof_getAllowedLocales) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getAllowedLocales"));

  if (ptr == nullptr) {
    do_fail("uspoof_getAllowedLocales");
  }

  return ptr(sc, status);
}
void uspoof_setAllowedChars(USpoofChecker * sc, const USet * chars, UErrorCode * status) {
  typedef decltype(&uspoof_setAllowedChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_setAllowedChars"));

  if (ptr == nullptr) {
    do_fail("uspoof_setAllowedChars");
  }

  ptr(sc, chars, status);
}
const USet * uspoof_getAllowedChars(const USpoofChecker * sc, UErrorCode * status) {
  typedef decltype(&uspoof_getAllowedChars) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getAllowedChars"));

  if (ptr == nullptr) {
    do_fail("uspoof_getAllowedChars");
  }

  return ptr(sc, status);
}
int32_t uspoof_check(const USpoofChecker * sc, const UChar * id, int32_t length, int32_t * position, UErrorCode * status) {
  typedef decltype(&uspoof_check) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_check"));

  if (ptr == nullptr) {
    do_fail("uspoof_check");
  }

  return ptr(sc, id, length, position, status);
}
int32_t uspoof_checkUTF8(const USpoofChecker * sc, const char * id, int32_t length, int32_t * position, UErrorCode * status) {
  typedef decltype(&uspoof_checkUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_checkUTF8"));

  if (ptr == nullptr) {
    do_fail("uspoof_checkUTF8");
  }

  return ptr(sc, id, length, position, status);
}
int32_t uspoof_areConfusable(const USpoofChecker * sc, const UChar * id1, int32_t length1, const UChar * id2, int32_t length2, UErrorCode * status) {
  typedef decltype(&uspoof_areConfusable) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_areConfusable"));

  if (ptr == nullptr) {
    do_fail("uspoof_areConfusable");
  }

  return ptr(sc, id1, length1, id2, length2, status);
}
int32_t uspoof_areConfusableUTF8(const USpoofChecker * sc, const char * id1, int32_t length1, const char * id2, int32_t length2, UErrorCode * status) {
  typedef decltype(&uspoof_areConfusableUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_areConfusableUTF8"));

  if (ptr == nullptr) {
    do_fail("uspoof_areConfusableUTF8");
  }

  return ptr(sc, id1, length1, id2, length2, status);
}
int32_t uspoof_getSkeleton(const USpoofChecker * sc, uint32_t type, const UChar * id, int32_t length, UChar * dest, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&uspoof_getSkeleton) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getSkeleton"));

  if (ptr == nullptr) {
    do_fail("uspoof_getSkeleton");
  }

  return ptr(sc, type, id, length, dest, destCapacity, status);
}
int32_t uspoof_getSkeletonUTF8(const USpoofChecker * sc, uint32_t type, const char * id, int32_t length, char * dest, int32_t destCapacity, UErrorCode * status) {
  typedef decltype(&uspoof_getSkeletonUTF8) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getSkeletonUTF8"));

  if (ptr == nullptr) {
    do_fail("uspoof_getSkeletonUTF8");
  }

  return ptr(sc, type, id, length, dest, destCapacity, status);
}
const USet * uspoof_getInclusionSet(UErrorCode * status) {
  typedef decltype(&uspoof_getInclusionSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getInclusionSet"));

  if (ptr == nullptr) {
    do_fail("uspoof_getInclusionSet");
  }

  return ptr(status);
}
const USet * uspoof_getRecommendedSet(UErrorCode * status) {
  typedef decltype(&uspoof_getRecommendedSet) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_getRecommendedSet"));

  if (ptr == nullptr) {
    do_fail("uspoof_getRecommendedSet");
  }

  return ptr(status);
}
int32_t uspoof_serialize(USpoofChecker * sc, void * data, int32_t capacity, UErrorCode * status) {
  typedef decltype(&uspoof_serialize) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "uspoof_serialize"));

  if (ptr == nullptr) {
    do_fail("uspoof_serialize");
  }

  return ptr(sc, data, capacity, status);
}
UStringSearch * usearch_open(const UChar * pattern, int32_t patternlength, const UChar * text, int32_t textlength, const char * locale, UBreakIterator * breakiter, UErrorCode * status) {
  typedef decltype(&usearch_open) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_open"));

  if (ptr == nullptr) {
    do_fail("usearch_open");
  }

  return ptr(pattern, patternlength, text, textlength, locale, breakiter, status);
}
UStringSearch * usearch_openFromCollator(const UChar * pattern, int32_t patternlength, const UChar * text, int32_t textlength, const UCollator * collator, UBreakIterator * breakiter, UErrorCode * status) {
  typedef decltype(&usearch_openFromCollator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_openFromCollator"));

  if (ptr == nullptr) {
    do_fail("usearch_openFromCollator");
  }

  return ptr(pattern, patternlength, text, textlength, collator, breakiter, status);
}
void usearch_close(UStringSearch * searchiter) {
  typedef decltype(&usearch_close) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_close"));

  if (ptr == nullptr) {
    do_fail("usearch_close");
  }

  ptr(searchiter);
}
void usearch_setOffset(UStringSearch * strsrch, int32_t position, UErrorCode * status) {
  typedef decltype(&usearch_setOffset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_setOffset"));

  if (ptr == nullptr) {
    do_fail("usearch_setOffset");
  }

  ptr(strsrch, position, status);
}
int32_t usearch_getOffset(const UStringSearch * strsrch) {
  typedef decltype(&usearch_getOffset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getOffset"));

  if (ptr == nullptr) {
    do_fail("usearch_getOffset");
  }

  return ptr(strsrch);
}
void usearch_setAttribute(UStringSearch * strsrch, USearchAttribute attribute, USearchAttributeValue value, UErrorCode * status) {
  typedef decltype(&usearch_setAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_setAttribute"));

  if (ptr == nullptr) {
    do_fail("usearch_setAttribute");
  }

  ptr(strsrch, attribute, value, status);
}
USearchAttributeValue usearch_getAttribute(const UStringSearch * strsrch, USearchAttribute attribute) {
  typedef decltype(&usearch_getAttribute) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getAttribute"));

  if (ptr == nullptr) {
    do_fail("usearch_getAttribute");
  }

  return ptr(strsrch, attribute);
}
int32_t usearch_getMatchedStart(const UStringSearch * strsrch) {
  typedef decltype(&usearch_getMatchedStart) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getMatchedStart"));

  if (ptr == nullptr) {
    do_fail("usearch_getMatchedStart");
  }

  return ptr(strsrch);
}
int32_t usearch_getMatchedLength(const UStringSearch * strsrch) {
  typedef decltype(&usearch_getMatchedLength) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getMatchedLength"));

  if (ptr == nullptr) {
    do_fail("usearch_getMatchedLength");
  }

  return ptr(strsrch);
}
int32_t usearch_getMatchedText(const UStringSearch * strsrch, UChar * result, int32_t resultCapacity, UErrorCode * status) {
  typedef decltype(&usearch_getMatchedText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getMatchedText"));

  if (ptr == nullptr) {
    do_fail("usearch_getMatchedText");
  }

  return ptr(strsrch, result, resultCapacity, status);
}
void usearch_setBreakIterator(UStringSearch * strsrch, UBreakIterator * breakiter, UErrorCode * status) {
  typedef decltype(&usearch_setBreakIterator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_setBreakIterator"));

  if (ptr == nullptr) {
    do_fail("usearch_setBreakIterator");
  }

  ptr(strsrch, breakiter, status);
}
const UBreakIterator * usearch_getBreakIterator(const UStringSearch * strsrch) {
  typedef decltype(&usearch_getBreakIterator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getBreakIterator"));

  if (ptr == nullptr) {
    do_fail("usearch_getBreakIterator");
  }

  return ptr(strsrch);
}
void usearch_setText(UStringSearch * strsrch, const UChar * text, int32_t textlength, UErrorCode * status) {
  typedef decltype(&usearch_setText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_setText"));

  if (ptr == nullptr) {
    do_fail("usearch_setText");
  }

  ptr(strsrch, text, textlength, status);
}
const UChar * usearch_getText(const UStringSearch * strsrch, int32_t * length) {
  typedef decltype(&usearch_getText) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getText"));

  if (ptr == nullptr) {
    do_fail("usearch_getText");
  }

  return ptr(strsrch, length);
}
UCollator * usearch_getCollator(const UStringSearch * strsrch) {
  typedef decltype(&usearch_getCollator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getCollator"));

  if (ptr == nullptr) {
    do_fail("usearch_getCollator");
  }

  return ptr(strsrch);
}
void usearch_setCollator(UStringSearch * strsrch, const UCollator * collator, UErrorCode * status) {
  typedef decltype(&usearch_setCollator) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_setCollator"));

  if (ptr == nullptr) {
    do_fail("usearch_setCollator");
  }

  ptr(strsrch, collator, status);
}
void usearch_setPattern(UStringSearch * strsrch, const UChar * pattern, int32_t patternlength, UErrorCode * status) {
  typedef decltype(&usearch_setPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_setPattern"));

  if (ptr == nullptr) {
    do_fail("usearch_setPattern");
  }

  ptr(strsrch, pattern, patternlength, status);
}
const UChar * usearch_getPattern(const UStringSearch * strsrch, int32_t * length) {
  typedef decltype(&usearch_getPattern) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_getPattern"));

  if (ptr == nullptr) {
    do_fail("usearch_getPattern");
  }

  return ptr(strsrch, length);
}
int32_t usearch_first(UStringSearch * strsrch, UErrorCode * status) {
  typedef decltype(&usearch_first) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_first"));

  if (ptr == nullptr) {
    do_fail("usearch_first");
  }

  return ptr(strsrch, status);
}
int32_t usearch_following(UStringSearch * strsrch, int32_t position, UErrorCode * status) {
  typedef decltype(&usearch_following) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_following"));

  if (ptr == nullptr) {
    do_fail("usearch_following");
  }

  return ptr(strsrch, position, status);
}
int32_t usearch_last(UStringSearch * strsrch, UErrorCode * status) {
  typedef decltype(&usearch_last) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_last"));

  if (ptr == nullptr) {
    do_fail("usearch_last");
  }

  return ptr(strsrch, status);
}
int32_t usearch_preceding(UStringSearch * strsrch, int32_t position, UErrorCode * status) {
  typedef decltype(&usearch_preceding) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_preceding"));

  if (ptr == nullptr) {
    do_fail("usearch_preceding");
  }

  return ptr(strsrch, position, status);
}
int32_t usearch_next(UStringSearch * strsrch, UErrorCode * status) {
  typedef decltype(&usearch_next) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_next"));

  if (ptr == nullptr) {
    do_fail("usearch_next");
  }

  return ptr(strsrch, status);
}
int32_t usearch_previous(UStringSearch * strsrch, UErrorCode * status) {
  typedef decltype(&usearch_previous) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_previous"));

  if (ptr == nullptr) {
    do_fail("usearch_previous");
  }

  return ptr(strsrch, status);
}
void usearch_reset(UStringSearch * strsrch) {
  typedef decltype(&usearch_reset) FuncPtr;
  static FuncPtr ptr = reinterpret_cast<FuncPtr>(
      do_dlsym(&handle_i18n, "usearch_reset"));

  if (ptr == nullptr) {
    do_fail("usearch_reset");
  }

  ptr(strsrch);
}

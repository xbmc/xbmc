#pragma once
#include <cstdint>
#include <cstddef>

// VstInt32 / VstIntPtr — platform-size integer types for VST2 ABI
typedef int32_t   VstInt32;
typedef intptr_t  VstIntPtr;

// ---------------------------------------------------------------------------
// Process level constants (returned by audioMasterGetCurrentProcessLevel)
// ---------------------------------------------------------------------------

enum VstProcessLevel {
    kVstProcessLevelUnknown  = 0,
    kVstProcessLevelUser     = 1,  // GUI / main thread
    kVstProcessLevelRealtime = 2,  // Audio thread — return this during processReplacing
    kVstProcessLevelPrefetch = 3,
    kVstProcessLevelOffline  = 4,
};

// ---------------------------------------------------------------------------
// Language constants (returned by audioMasterGetLanguage)
// ---------------------------------------------------------------------------

enum VstHostLanguage {
    kVstLangEnglish  = 1,
    kVstLangGerman   = 2,
    kVstLangFrench   = 3,
    kVstLangItalian  = 4,
    kVstLangSpanish  = 5,
    kVstLangJapanese = 6,
};

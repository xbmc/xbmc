/*
 * addon_main.cpp — Kodi ADSP addon entry point implementations
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 *
 * Each audio stream gets its own DSPProcessor, allocated in StreamCreate and
 * stored in ADDON_HANDLE::dataAddress (void*).  GetProc() casts it back.
 * Stream-level cleanup happens in StreamDestroy; ADDON_Destroy does nothing.
 */

#include "addon_main.h"
#include "dsp/DSPProcessor.h"
#include "bridge/EditorBridge.h"
#include <string>
#include <cstring>

// ---------------------------------------------------------------------------
// Global state — set once in ADDON_Create, read by every StreamCreate.
// ---------------------------------------------------------------------------

static std::string g_addonDataPath;   // set in ADDON_Create, used for chain.json base path
static EditorBridge g_editorBridge;   // manages named pipe server + VST editor windows
static DSPProcessor* g_lastProcessor = nullptr;  // most recent processor for editor bridge

// ---------------------------------------------------------------------------
// Helper — retrieve the per-stream processor from the opaque handle.
// handle->dataAddress is void*; we store DSPProcessor* there.
// ---------------------------------------------------------------------------

static DSPProcessor* GetProc(const ADDON_HANDLE handle)
{
    return static_cast<DSPProcessor*>(handle->dataAddress);
}

// ---------------------------------------------------------------------------
// ADDON lifecycle
// ---------------------------------------------------------------------------

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
    // props is AE_DSP_PROPERTIES (defined in kodi_adsp_types.h as AE_DSP_PROPERTIES)
    const AE_DSP_PROPERTIES* dspProps = static_cast<const AE_DSP_PROPERTIES*>(props);
    if (dspProps && dspProps->strUserPath)
        g_addonDataPath = dspProps->strUserPath;

    return ADDON_STATUS_OK;
}

void ADDON_Destroy()
{
    // Stop the editor bridge before streams are torn down.
    g_editorBridge.stop();
    g_lastProcessor = nullptr;
}

ADDON_STATUS ADDON_GetStatus()
{
    return ADDON_STATUS_OK;
}

bool ADDON_HasSettings()
{
    return false;   // settings handled externally
}

unsigned int ADDON_GetSettings(ADDON_StructSetting*** sSet)
{
    (void)sSet;
    return 0;
}

ADDON_STATUS ADDON_SetSetting(const char* settingName, const void* settingValue)
{
    (void)settingName;
    (void)settingValue;
    return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
    // Nothing to do.
}

void ADDON_FreeSettings()
{
    // Nothing to do.
}

void ADDON_Announce(const char* flag, const char* sender, const char* message, const void* data)
{
    (void)flag;
    (void)sender;
    (void)message;
    (void)data;
}

// ---------------------------------------------------------------------------
// API version queries
// ---------------------------------------------------------------------------

const char* GetAudioDSPAPIVersion()
{
    return KODI_AE_DSP_API_VERSION;
}

const char* GetMinimumAudioDSPAPIVersion()
{
    return KODI_AE_DSP_MIN_API_VERSION;
}

const char* GetGUIAPIVersion()
{
    return "";
}

const char* GetMinimumGUIAPIVersion()
{
    return "";
}

// ---------------------------------------------------------------------------
// Addon capabilities
// ---------------------------------------------------------------------------

AE_DSP_ERROR GetAddonCapabilities(AE_DSP_ADDON_CAPABILITIES* pCapabilities)
{
    memset(pCapabilities, 0, sizeof(*pCapabilities));
    pCapabilities->bSupportsInputProcess    = false;
    pCapabilities->bSupportsPreProcess      = false;
    pCapabilities->bSupportsMasterProcess   = true;
    pCapabilities->bSupportsPostProcess     = false;
    pCapabilities->bSupportsInputResample   = false;
    pCapabilities->bSupportsOutputResample  = false;
    return AE_DSP_ERROR_NO_ERROR;
}

const char* GetDSPName()
{
    return "VST Host";
}

const char* GetDSPVersion()
{
    return "1.0.0";
}

// ---------------------------------------------------------------------------
// Menu hook
// NOTE: get_addon() wires pDSP->MenuHook = CallMenuHook, so the function
// that implements the AE_DSP_MENUHOOK callback MUST be named CallMenuHook.
// ---------------------------------------------------------------------------

AE_DSP_ERROR CallMenuHook(const AE_DSP_MENUHOOK& menuhook, const AE_DSP_MENUHOOK_DATA& item)
{
    (void)menuhook;
    (void)item;
    return AE_DSP_ERROR_NO_ERROR;
}

// ---------------------------------------------------------------------------
// Stream lifecycle
// ---------------------------------------------------------------------------

AE_DSP_ERROR StreamCreate(const AE_DSP_SETTINGS* addonSettings,
                          const AE_DSP_STREAM_PROPERTIES* pProperties,
                          ADDON_HANDLE handle)
{
    (void)pProperties;

    auto* proc = new DSPProcessor();

    // Pass the data directory to the processor; loadConfig()/saveConfig()
    // append "/chain.json" internally — do NOT append the filename here.
    proc->setConfigPath(g_addonDataPath);

    if (!proc->streamCreate(addonSettings))
    {
        delete proc;
        return AE_DSP_ERROR_FAILED;
    }

    // Store per-stream processor in the opaque handle.
    // dataAddress is void* — correct for a pointer value.
    // dataIdentifier (int) is NOT used for this purpose.
    handle->dataAddress = proc;

    // Start the editor bridge (if not already running) with this processor's chain.
    // The bridge allows the Python VST Manager addon to open native VST editor windows.
    g_lastProcessor = proc;
    if (!g_editorBridge.isRunning())
        g_editorBridge.start(&proc->getChain());

    return AE_DSP_ERROR_NO_ERROR;
}

AE_DSP_ERROR StreamDestroy(const ADDON_HANDLE handle)
{
    DSPProcessor* proc = GetProc(handle);
    if (!proc)
        return AE_DSP_ERROR_INVALID_PARAMETERS;

    // Stop the editor bridge if it is pointing at the processor being destroyed
    if (g_editorBridge.isRunning() && &proc->getChain() == g_editorBridge.getChain())
        g_editorBridge.stop();

    proc->streamDestroy();
    delete proc;
    handle->dataAddress = nullptr;
    return AE_DSP_ERROR_NO_ERROR;
}

AE_DSP_ERROR StreamIsModeSupported(const ADDON_HANDLE handle,
                                   AE_DSP_MODE_TYPE type,
                                   unsigned int mode_id,
                                   int unique_db_mode_id)
{
    (void)handle;
    (void)mode_id;
    (void)unique_db_mode_id;

    if (type == AE_DSP_MODE_TYPE_MASTER_PROCESS)
        return AE_DSP_ERROR_NO_ERROR;

    return AE_DSP_ERROR_IGNORE_ME;
}

AE_DSP_ERROR StreamInitialize(const ADDON_HANDLE handle, const AE_DSP_SETTINGS* settings)
{
    (void)handle;
    (void)settings;
    return AE_DSP_ERROR_NO_ERROR;
}

// ---------------------------------------------------------------------------
// Input processing (not supported — stub)
// ---------------------------------------------------------------------------

bool InputProcess(const ADDON_HANDLE handle, const float** array_in, unsigned int samples)
{
    (void)handle;
    (void)array_in;
    (void)samples;
    return false;
}

// ---------------------------------------------------------------------------
// Input resampling (not supported — stubs)
// ---------------------------------------------------------------------------

unsigned int InputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
    (void)handle;
    return 0;
}

unsigned int InputResampleProcess(const ADDON_HANDLE handle,
                                  float** array_in,
                                  float** array_out,
                                  unsigned int samples)
{
    (void)handle;
    (void)array_in;
    (void)array_out;
    return samples;
}

int InputResampleSampleRate(const ADDON_HANDLE handle)
{
    (void)handle;
    return 0;
}

float InputResampleGetDelay(const ADDON_HANDLE handle)
{
    (void)handle;
    return 0.0f;
}

// ---------------------------------------------------------------------------
// Pre-processing (not supported — stubs)
// ---------------------------------------------------------------------------

unsigned int PreProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
    (void)handle;
    (void)mode_id;
    return 0;
}

float PreProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
    (void)handle;
    (void)mode_id;
    return 0.0f;
}

unsigned int PreProcess(const ADDON_HANDLE handle,
                        unsigned int mode_id,
                        float** array_in,
                        float** array_out,
                        unsigned int samples)
{
    (void)handle;
    (void)mode_id;
    (void)array_in;
    (void)array_out;
    return samples;
}

// ---------------------------------------------------------------------------
// Master processing — the active stage for this addon
// ---------------------------------------------------------------------------

AE_DSP_ERROR MasterProcessSetMode(const ADDON_HANDLE handle,
                                  AE_DSP_STREAMTYPE type,
                                  unsigned int mode_id,
                                  int unique_db_mode_id)
{
    (void)handle;
    (void)type;
    (void)mode_id;
    (void)unique_db_mode_id;
    return AE_DSP_ERROR_NO_ERROR;
}

unsigned int MasterProcessNeededSamplesize(const ADDON_HANDLE handle)
{
    (void)handle;
    return 0;
}

float MasterProcessGetDelay(const ADDON_HANDLE handle)
{
    const DSPProcessor* proc = GetProc(handle);
    if (!proc)
        return 0.0f;
    const int sampleRate = proc->getSampleRate();
    if (sampleRate <= 0)
        return 0.0f;
    return static_cast<float>(proc->masterProcessGetDelay())
           / static_cast<float>(sampleRate);
}

int MasterProcessGetOutChannels(const ADDON_HANDLE handle,
                                unsigned long& out_channel_present_flags)
{
    out_channel_present_flags = GetProc(handle)->masterProcessGetOutChannels();
    return GetProc(handle)->getNumChannels();
}

unsigned int MasterProcess(const ADDON_HANDLE handle,
                           float** array_in,
                           float** array_out,
                           unsigned int samples)
{
    return static_cast<unsigned int>(
        GetProc(handle)->masterProcess(array_in, array_out, samples));
}

const char* MasterProcessGetStreamInfoString(const ADDON_HANDLE handle)
{
    return GetProc(handle)->masterProcessGetName();
}

// ---------------------------------------------------------------------------
// Post-processing (not supported — stubs)
// ---------------------------------------------------------------------------

unsigned int PostProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
    (void)handle;
    (void)mode_id;
    return 0;
}

float PostProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
    (void)handle;
    (void)mode_id;
    return 0.0f;
}

unsigned int PostProcess(const ADDON_HANDLE handle,
                         unsigned int mode_id,
                         float** array_in,
                         float** array_out,
                         unsigned int samples)
{
    (void)handle;
    (void)mode_id;
    (void)array_in;
    (void)array_out;
    return samples;
}

// ---------------------------------------------------------------------------
// Output resampling (not supported — stubs)
// ---------------------------------------------------------------------------

unsigned int OutputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
    (void)handle;
    return 0;
}

unsigned int OutputResampleProcess(const ADDON_HANDLE handle,
                                   float** array_in,
                                   float** array_out,
                                   unsigned int samples)
{
    (void)handle;
    (void)array_in;
    (void)array_out;
    return samples;
}

int OutputResampleSampleRate(const ADDON_HANDLE handle)
{
    (void)handle;
    return 0;
}

float OutputResampleGetDelay(const ADDON_HANDLE handle)
{
    (void)handle;
    return 0.0f;
}

#pragma once
/*
 * DSPProcessor.h — Bridge between the Kodi ADSP callback API and DSPChain
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "DSPChain.h"
#include <string>
#include <cstdint>

// Forward declaration — avoid pulling in the full Kodi header here.
struct AE_DSP_SETTINGS;

/// Adapts the Kodi ADSP master-process callback interface to DSPChain.
///
/// One DSPProcessor is created per Kodi audio stream
/// (see StreamCreate / StreamDestroy callbacks in kodi_audiodsp_dll.h).
///
/// Thread safety:
///   - streamCreate / streamDestroy — stream/setup thread
///   - masterProcess — audio render thread ONLY
///   - getChain / loadConfig / saveConfig / setConfigPath — settings thread
class DSPProcessor {
public:
    DSPProcessor();
    ~DSPProcessor();

    // -------------------------------------------------------------------------
    // Kodi ADSP callbacks
    // -------------------------------------------------------------------------

    /// Called by Kodi when a new audio stream is started.
    /// Initializes the DSPChain and loads the persisted config (best-effort).
    /// @return true on success; false causes Kodi to skip this add-on.
    bool streamCreate(const AE_DSP_SETTINGS* settings);

    /// Called by Kodi when the audio stream ends.
    void streamDestroy();

    /// Called by Kodi to process one block of audio through the master stage.
    /// @param in      Planar input  (one float* per channel).
    /// @param out     Planar output (one float* per channel).
    /// @param samples Number of samples in this block.
    /// @return Number of samples processed.
    int masterProcess(float** in, float** out, unsigned int samples);

    /// Returns the output channel presence bitmask (AE_DSP_PRSNT_CH_* flags).
    unsigned long masterProcessGetOutChannels() const;

    /// Returns the total processing latency in samples.
    int masterProcessGetDelay() const;

    /// Returns the human-readable name of this master mode.
    const char* masterProcessGetName() const;

    // -------------------------------------------------------------------------
    // Chain access for the settings UI
    // -------------------------------------------------------------------------

    DSPChain&       getChain()       { return m_chain; }
    const DSPChain& getChain() const { return m_chain; }

    // -------------------------------------------------------------------------
    // Config file management
    // -------------------------------------------------------------------------

    /// Set the directory where chain.json is read and written.
    void setConfigPath(const std::string& path) { m_configPath = path; }

    /// Load chain.json from m_configPath.  Failure is non-fatal.
    bool loadConfig();

    /// Save chain.json to m_configPath.
    bool saveConfig();

    // -------------------------------------------------------------------------
    // Stream info (populated during streamCreate)
    // -------------------------------------------------------------------------

    int getSampleRate()  const { return m_sampleRate;  }
    int getNumChannels() const { return m_numChannels; }
    int getStreamID()    const { return m_streamID;    }

private:
    DSPChain    m_chain;
    std::string m_configPath;

    int  m_sampleRate  = 44100;
    int  m_numChannels = 2;
    int  m_streamID    = 0;
    bool m_active      = false;

    static const char* s_processorName;
};

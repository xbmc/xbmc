#pragma once
/*
 * IVSTPlugin.h — Polymorphic interface for VST2 and VST3 plugin hosting
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include <string>
#include <vector>
#include <cstdint>

class IVSTPlugin
{
public:
    virtual ~IVSTPlugin() = default;

    // -------------------------------------------------------------------------
    // Lifecycle — called from settings/stream thread (never from audio thread)
    // -------------------------------------------------------------------------

    /// Load and initialize the plugin.
    /// @param sampleRate    Audio sample rate in Hz
    /// @param maxBlockSize  Maximum samples per process() call
    /// @param numChannels   Channel count (host-side)
    /// @return true on success
    virtual bool load(double sampleRate, int maxBlockSize, int numChannels) = 0;

    /// Release all plugin resources. Safe to call if already unloaded.
    virtual void unload() = 0;

    /// Reinitialize after format change (default: unload + load).
    virtual bool reinitialize(double sampleRate, int maxBlockSize, int numChannels)
    {
        unload();
        return load(sampleRate, maxBlockSize, numChannels);
    }

    // -------------------------------------------------------------------------
    // Audio processing — called ONLY from the audio render thread
    // No allocations, no locks, no I/O allowed inside process().
    // -------------------------------------------------------------------------

    /// Process one block. in/out are float** planar (one ptr per channel).
    /// @return Number of samples processed (should equal samples parameter)
    virtual int process(float** in, float** out, int samples) = 0;

    // -------------------------------------------------------------------------
    // Parameters — thread-safe (implementations must queue internally)
    // -------------------------------------------------------------------------

    /// Set parameter value. Thread-safe: queued for audio thread delivery.
    /// @param index  0-based parameter index
    /// @param value  Normalized value [0.0, 1.0]
    virtual void setParameter(int index, float value) = 0;

    /// Get current parameter value. Approximately thread-safe (display only).
    virtual float getParameter(int index) const = 0;

    /// Total parameter count.
    virtual int getParameterCount() const = 0;

    /// Human-readable parameter name (may be truncated to 7 chars for VST2).
    virtual std::string getParameterName(int index) const = 0;

    // -------------------------------------------------------------------------
    // Query
    // -------------------------------------------------------------------------

    /// Plugin processing latency in samples (used for A/V sync compensation).
    virtual int getLatencySamples() const = 0;

    /// Human-readable plugin name.
    virtual std::string getName() const = 0;

    /// Human-readable vendor/manufacturer name.
    virtual std::string getVendorName() const = 0;

    /// Absolute path to the plugin file on disk.
    virtual std::string getPath() const = 0;

    /// Whether the plugin is loaded and ready to process.
    virtual bool isLoaded() const = 0;

    // -------------------------------------------------------------------------
    // State persistence — called from settings thread, never audio thread
    // -------------------------------------------------------------------------

    /// Serialize plugin state to bytes.
    /// VST2: first byte is 'C' (chunk) or 'P' (params), then data.
    /// VST3: IBStream bytes.
    virtual std::vector<uint8_t> saveState() const = 0;

    /// Restore plugin state from previously saved bytes.
    virtual bool loadState(const std::vector<uint8_t>& data) = 0;

    // -------------------------------------------------------------------------
    // Format discriminator
    // -------------------------------------------------------------------------

    enum class PluginFormat { VST2, VST3 };
    virtual PluginFormat getFormat() const = 0;

    // -------------------------------------------------------------------------
    // Bypass — concrete in base (chain's decision, not the plugin's)
    // -------------------------------------------------------------------------

    bool isBypassed() const { return m_bypassed; }
    void setBypassed(bool v) { m_bypassed = v; }

    // -------------------------------------------------------------------------
    // Editor — native VST editor window support
    // -------------------------------------------------------------------------

    /// Whether the plugin provides a graphical editor window.
    virtual bool hasEditor() const = 0;

    /// Open the plugin's native editor inside the given parent window.
    /// @param parentWindow  Platform window handle (HWND on Windows).
    /// @return true if the editor was opened successfully.
    virtual bool openEditor(void* parentWindow) = 0;

    /// Close the plugin's native editor.
    virtual void closeEditor() = 0;

    /// Query the preferred editor window size in pixels.
    /// @param[out] width   Editor width in pixels.
    /// @param[out] height  Editor height in pixels.
    /// @return true if the size was retrieved successfully.
    virtual bool getEditorSize(int& width, int& height) const = 0;

    /// Pump the editor's idle loop (VST2 only; VST3 is self-pumping).
    virtual void idleEditor() = 0;

protected:
    bool m_bypassed = false;
};

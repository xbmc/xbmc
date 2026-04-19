#pragma once
/*
 * EditorBridge.h — Named pipe server + Win32 VST editor window manager
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 *
 * Provides IPC between the Python VST Manager addon and the C++ ADSP addon.
 * The Python side sends JSON commands over a named pipe to open/close native
 * VST editor windows.  This class manages the pipe server, the editor
 * windows, and the Win32 message pump.
 *
 * Threading:
 *   - The pipe server runs on a dedicated thread (m_pipeThread).
 *   - Editor windows are created on a dedicated UI thread (m_uiThread) which
 *     runs the Win32 message pump.  All VST editor calls (open/close/idle)
 *     happen on this thread.
 *   - Communication between the pipe thread and UI thread uses PostMessage.
 */

#include "../dsp/DSPChain.h"
#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_map>

class EditorBridge
{
public:
    EditorBridge();
    ~EditorBridge();

    /// Start the pipe server and UI thread.
    /// @param chain  Pointer to the active DSPChain — used to look up plugins.
    void start(DSPChain* chain);

    /// Stop the pipe server and close all editor windows.
    void stop();

    bool isRunning() const { return m_running.load(); }

private:
    // -------------------------------------------------------------------------
    // Pipe server
    // -------------------------------------------------------------------------

    /// Pipe server loop — runs on m_pipeThread.
    void pipeServerLoop();

    /// Handle a single client connection (blocking read/write).
    void handleClient(HANDLE pipe);

    /// Process a JSON command string, return the JSON response.
    std::string processCommand(const std::string& json);

    // -------------------------------------------------------------------------
    // UI thread — Win32 message pump and editor window management
    // -------------------------------------------------------------------------

    /// UI thread entry — registers window class, runs message loop.
    void uiThreadLoop();

    /// Open an editor window for the plugin at the given chain index.
    /// Called on the UI thread via PostMessage.
    void doOpenEditor(const std::string& pluginPath);

    /// Close an editor window for the given plugin path.
    /// Called on the UI thread via PostMessage.
    void doCloseEditor(const std::string& pluginPath);

    /// Close all open editor windows.
    void doCloseAll();

    /// Static window procedure.
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// Instance window procedure.
    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    static constexpr const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\kodi_vsthost_editor";
    static constexpr UINT WM_VSTBRIDGE_OPEN   = WM_USER + 100;
    static constexpr UINT WM_VSTBRIDGE_CLOSE  = WM_USER + 101;
    static constexpr UINT WM_VSTBRIDGE_QUIT   = WM_USER + 102;
    static constexpr UINT EDITOR_IDLE_TIMER   = 1;
    static constexpr UINT EDITOR_IDLE_MS      = 30;

    DSPChain*           m_chain = nullptr;
    std::atomic<bool>   m_running{false};
    std::thread         m_pipeThread;
    std::thread         m_uiThread;
    DWORD               m_uiThreadID = 0;

    /// Tracks open editor windows: plugin path → HWND.
    std::unordered_map<std::string, HWND> m_openEditors;
    std::mutex                            m_editorMutex;

    /// Map from HWND → plugin path (reverse lookup for WndProc).
    std::unordered_map<HWND, std::string> m_hwndToPath;

    /// Pipe server handle — stored so stop() can cancel blocking ConnectNamedPipe.
    HANDLE m_pipeHandle = INVALID_HANDLE_VALUE;

    /// Event signaled when the UI thread is ready (m_uiThreadID is set).
    HANDLE m_uiReadyEvent = nullptr;

    ATOM m_windowClass = 0;
};

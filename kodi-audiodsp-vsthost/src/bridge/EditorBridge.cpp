/*
 * EditorBridge.cpp — Named pipe server + Win32 VST editor window manager
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */

#include "EditorBridge.h"
#include "../util/JsonUtil.h"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <algorithm>

// ============================================================================
// Constructor / Destructor
// ============================================================================

EditorBridge::EditorBridge() = default;

EditorBridge::~EditorBridge()
{
    stop();
}

// ============================================================================
// start / stop
// ============================================================================

void EditorBridge::start(DSPChain* chain)
{
    if (m_running.load())
        return;

    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        m_chain = chain;
    }
    m_running = true;

    // Create an event to wait for the UI thread to become ready
    m_uiReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    // Start UI thread first so it can register the window class
    m_uiThread = std::thread([this]() { uiThreadLoop(); });

    // Wait for the UI thread to signal readiness (up to 5 seconds)
    if (m_uiReadyEvent)
    {
        WaitForSingleObject(m_uiReadyEvent, 5000);
        CloseHandle(m_uiReadyEvent);
        m_uiReadyEvent = nullptr;
    }

    // Start pipe server thread
    m_pipeThread = std::thread([this]() { pipeServerLoop(); });
}

void EditorBridge::stop()
{
    if (!m_running.load())
        return;

    m_running = false;

    // Cancel any blocking ConnectNamedPipe by closing the handle
    if (m_pipeHandle != INVALID_HANDLE_VALUE)
    {
        CancelIoEx(m_pipeHandle, nullptr);
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }

    // Tell the UI thread to quit
    if (m_uiThreadID != 0)
        PostThreadMessageW(m_uiThreadID, WM_QUIT, 0, 0);

    // Null out m_chain under the lock so concurrent readers see nullptr atomically.
    // The joins must happen OUTSIDE the lock: the UI thread's WM_CLOSE handler
    // calls DestroyWindow(), which synchronously delivers WM_DESTROY, which
    // acquires m_editorMutex — holding the lock here would therefore deadlock.
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        m_chain = nullptr;
    }

    if (m_pipeThread.joinable())
        m_pipeThread.join();
    if (m_uiThread.joinable())
        m_uiThread.join();

    m_uiThreadID = 0;
}

// ============================================================================
// Pipe server
// ============================================================================

void EditorBridge::pipeServerLoop()
{
    while (m_running.load())
    {
        m_pipeHandle = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,          // max instances
            4096,       // out buffer
            4096,       // in buffer
            0,          // default timeout
            nullptr);   // default security

        if (m_pipeHandle == INVALID_HANDLE_VALUE)
        {
            std::fprintf(stderr, "[EditorBridge] CreateNamedPipe failed: %lu\n",
                         GetLastError());
            Sleep(1000);
            continue;
        }

        // Block until a client connects
        BOOL connected = ConnectNamedPipe(m_pipeHandle, nullptr);
        DWORD err = GetLastError();

        if (!connected && err != ERROR_PIPE_CONNECTED)
        {
            // ConnectNamedPipe was cancelled (stop() called) or failed
            CloseHandle(m_pipeHandle);
            m_pipeHandle = INVALID_HANDLE_VALUE;
            continue;
        }

        // Handle client
        handleClient(m_pipeHandle);

        FlushFileBuffers(m_pipeHandle);
        DisconnectNamedPipe(m_pipeHandle);
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
}

void EditorBridge::handleClient(HANDLE pipe)
{
    // Read the full command (newline-delimited JSON)
    char buf[4096] = {};
    DWORD bytesRead = 0;

    if (!ReadFile(pipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) || bytesRead == 0)
        return;

    buf[bytesRead] = '\0';
    std::string command(buf);

    // Strip trailing newline
    while (!command.empty() && (command.back() == '\n' || command.back() == '\r'))
        command.pop_back();

    // Process and respond
    std::string response = processCommand(command);
    response += "\n";

    DWORD bytesWritten = 0;
    WriteFile(pipe, response.c_str(),
              static_cast<DWORD>(response.size()),
              &bytesWritten, nullptr);
}

std::string EditorBridge::processCommand(const std::string& json)
{
    std::string cmd  = JsonUtil::extractString(json, "cmd");
    std::string path = JsonUtil::extractString(json, "path");

    if (cmd == "ping")
    {
        return "{\"status\":\"ok\",\"cmd\":\"ping\"}";
    }

    if (cmd == "open")
    {
        if (path.empty())
            return "{\"status\":\"error\",\"cmd\":\"open\",\"error\":\"Missing path\"}";

        DSPChain* chain = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_editorMutex);
            chain = m_chain;
        }

        if (!chain)
            return "{\"status\":\"error\",\"cmd\":\"open\",\"path\":\""
                   + JsonUtil::escape(path)
                   + "\",\"error\":\"No active audio chain\"}";

        // Find the plugin in the chain
        IVSTPlugin* plugin = nullptr;
        for (int i = 0; i < chain->getPluginCount(); ++i)
        {
            auto* p = chain->getPlugin(i);
            if (p && p->getPath() == path) {
                plugin = p;
                break;
            }
        }

        if (!plugin)
            return "{\"status\":\"error\",\"cmd\":\"open\",\"path\":\""
                   + JsonUtil::escape(path)
                   + "\",\"error\":\"Plugin not in chain\"}";

        if (!plugin->hasEditor())
            return "{\"status\":\"ok\",\"cmd\":\"open\",\"path\":\""
                   + JsonUtil::escape(path)
                   + "\",\"hasEditor\":false}";

        // Post a message to the UI thread to open the editor
        // We allocate a string on the heap and pass its pointer via LPARAM
        auto* pathCopy = new std::string(path);
        if (!PostThreadMessageW(m_uiThreadID, WM_VSTBRIDGE_OPEN, 0,
                                reinterpret_cast<LPARAM>(pathCopy)))
        {
            delete pathCopy;
            return "{\"status\":\"error\",\"cmd\":\"open\",\"path\":\""
                   + JsonUtil::escape(path)
                   + "\",\"error\":\"Failed to post to UI thread\"}";
        }

        return "{\"status\":\"ok\",\"cmd\":\"open\",\"path\":\""
               + JsonUtil::escape(path)
               + "\",\"hasEditor\":true}";
    }

    if (cmd == "close")
    {
        if (path.empty())
            return "{\"status\":\"error\",\"cmd\":\"close\",\"error\":\"Missing path\"}";

        auto* pathCopy = new std::string(path);
        if (!PostThreadMessageW(m_uiThreadID, WM_VSTBRIDGE_CLOSE, 0,
                                reinterpret_cast<LPARAM>(pathCopy)))
        {
            delete pathCopy;
            return "{\"status\":\"error\",\"cmd\":\"close\",\"path\":\""
                   + JsonUtil::escape(path)
                   + "\",\"error\":\"Failed to post to UI thread\"}";
        }

        return "{\"status\":\"ok\",\"cmd\":\"close\",\"path\":\""
               + JsonUtil::escape(path) + "\"}";
    }

    if (cmd == "close_all")
    {
        PostThreadMessageW(m_uiThreadID, WM_VSTBRIDGE_CLOSE, 0, 0);
        return "{\"status\":\"ok\",\"cmd\":\"close_all\"}";
    }

    return "{\"status\":\"error\",\"error\":\"Unknown command\"}";
}

// ============================================================================
// UI thread
// ============================================================================

void EditorBridge::uiThreadLoop()
{
    // Register the editor host window class
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = GetModuleHandleW(nullptr);
    wc.lpszClassName  = L"KodiVSTEditor";
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    m_windowClass = RegisterClassExW(&wc);

    // Create a message-only window to receive thread messages
    m_uiThreadID = GetCurrentThreadId();

    // Signal that the UI thread is ready
    if (m_uiReadyEvent)
        SetEvent(m_uiReadyEvent);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        // Handle our custom messages
        if (msg.hwnd == nullptr)
        {
            switch (msg.message)
            {
            case WM_VSTBRIDGE_OPEN:
            {
                auto* pathPtr = reinterpret_cast<std::string*>(msg.lParam);
                if (pathPtr)
                {
                    doOpenEditor(*pathPtr);
                    delete pathPtr;
                }
                break;
            }
            case WM_VSTBRIDGE_CLOSE:
            {
                auto* pathPtr = reinterpret_cast<std::string*>(msg.lParam);
                if (pathPtr)
                {
                    doCloseEditor(*pathPtr);
                    delete pathPtr;
                }
                else
                {
                    doCloseAll();
                }
                break;
            }
            default:
                break;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup: close all editors
    doCloseAll();

    if (m_windowClass)
    {
        UnregisterClassW(L"KodiVSTEditor", GetModuleHandleW(nullptr));
        m_windowClass = 0;
    }
}

// ============================================================================
// Editor window management
// ============================================================================

void EditorBridge::doOpenEditor(const std::string& pluginPath)
{
    // Check if already open
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        if (m_openEditors.count(pluginPath))
        {
            // Bring existing window to front
            HWND existing = m_openEditors[pluginPath];
            SetForegroundWindow(existing);
            return;
        }
    }

    DSPChain* chain = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        chain = m_chain;
    }

    if (!chain)
        return;

    // Find the plugin
    IVSTPlugin* plugin = nullptr;
    for (int i = 0; i < chain->getPluginCount(); ++i)
    {
        auto* p = chain->getPlugin(i);
        if (p && p->getPath() == pluginPath) {
            plugin = p;
            break;
        }
    }

    if (!plugin || !plugin->hasEditor())
        return;

    // Get editor size
    int editorW = 640, editorH = 480;
    plugin->getEditorSize(editorW, editorH);

    // Compute window rect (add frame to client size)
    RECT wr = {0, 0, static_cast<LONG>(editorW), static_cast<LONG>(editorH)};
    AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int windowW = wr.right  - wr.left;
    int windowH = wr.bottom - wr.top;

    // Build window title
    std::string title = "VST: " + plugin->getName();
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    std::wstring wtitle(titleLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wtitle.data(), titleLen);

    // Create the editor host window
    HWND hwnd = CreateWindowExW(
        0,
        L"KodiVSTEditor",
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowW, windowH,
        nullptr,        // no parent
        nullptr,        // no menu
        GetModuleHandleW(nullptr),
        this);          // pass this pointer for WndProc

    if (!hwnd)
    {
        std::fprintf(stderr, "[EditorBridge] CreateWindowExW failed: %lu\n",
                     GetLastError());
        return;
    }

    // Open the VST editor inside our window (also attaches the VST3 view)
    if (!plugin->openEditor(static_cast<void*>(hwnd)))
    {
        std::fprintf(stderr, "[EditorBridge] plugin->openEditor() failed for '%s'\n",
                     pluginPath.c_str());
        DestroyWindow(hwnd);
        return;
    }

    // Resize window to match the attached view's reported size.
    // For VST3 this is the first accurate query (m_plugView is now set).
    {
        int viewW = 0, viewH = 0;
        if (plugin->getEditorSize(viewW, viewH) && viewW > 0 && viewH > 0
            && (viewW != editorW || viewH != editorH))
        {
            RECT newClientRect = {0, 0, static_cast<LONG>(viewW), static_cast<LONG>(viewH)};
            AdjustWindowRectEx(&newClientRect, WS_OVERLAPPEDWINDOW, FALSE, 0);
            SetWindowPos(hwnd, nullptr, 0, 0,
                         newClientRect.right - newClientRect.left,
                         newClientRect.bottom - newClientRect.top,
                         SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    // Track the editor
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        m_openEditors[pluginPath] = hwnd;
        m_hwndToPath[hwnd]        = pluginPath;
    }

    // Start idle timer for VST2 editors
    SetTimer(hwnd, EDITOR_IDLE_TIMER, EDITOR_IDLE_MS, nullptr);

    // Show the window
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

void EditorBridge::doCloseEditor(const std::string& pluginPath)
{
    HWND hwnd = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        auto it = m_openEditors.find(pluginPath);
        if (it == m_openEditors.end())
            return;
        hwnd = it->second;
    }

    if (hwnd)
        DestroyWindow(hwnd);
}

void EditorBridge::doCloseAll()
{
    // Collect HWNDs to destroy, then release the lock before calling
    // DestroyWindow (which triggers WM_DESTROY and modifies the maps).
    std::vector<HWND> hwnds;
    {
        std::lock_guard<std::mutex> lock(m_editorMutex);
        hwnds.reserve(m_openEditors.size());
        for (auto& [path, hwnd] : m_openEditors)
        {
            if (hwnd)
                hwnds.push_back(hwnd);
        }
    }
    for (HWND hwnd : hwnds)
        DestroyWindow(hwnd);
}

// ============================================================================
// Window procedure
// ============================================================================

LRESULT CALLBACK EditorBridge::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    EditorBridge* self = nullptr;

    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<EditorBridge*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<EditorBridge*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self)
        return self->handleMessage(hwnd, msg, wParam, lParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT EditorBridge::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
        if (wParam == EDITOR_IDLE_TIMER)
        {
            // Idle the VST editor — needed for VST2 plugins
            std::string pluginPath;
            DSPChain* chain = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_editorMutex);
                auto it = m_hwndToPath.find(hwnd);
                if (it != m_hwndToPath.end())
                    pluginPath = it->second;
                chain = m_chain;
            }

            if (!pluginPath.empty() && chain)
            {
                for (int i = 0; i < chain->getPluginCount(); ++i)
                {
                    auto* p = chain->getPlugin(i);
                    if (p && p->getPath() == pluginPath)
                    {
                        p->idleEditor();
                        break;
                    }
                }
            }
            return 0;
        }
        break;

    case WM_CLOSE:
    {
        // User clicked the X button — close the editor
        std::string pluginPath;
        DSPChain* chain = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_editorMutex);
            auto it = m_hwndToPath.find(hwnd);
            if (it != m_hwndToPath.end())
                pluginPath = it->second;
            chain = m_chain;
        }

        // Close the VST editor before destroying the window.
        // The lock is already released here, so WM_DESTROY can acquire it.
        if (!pluginPath.empty() && chain)
        {
            for (int i = 0; i < chain->getPluginCount(); ++i)
            {
                auto* p = chain->getPlugin(i);
                if (p && p->getPath() == pluginPath)
                {
                    p->closeEditor();
                    break;
                }
            }
        }

        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd, EDITOR_IDLE_TIMER);

        std::lock_guard<std::mutex> lock(m_editorMutex);
        auto pathIt = m_hwndToPath.find(hwnd);
        if (pathIt != m_hwndToPath.end())
        {
            m_openEditors.erase(pathIt->second);
            m_hwndToPath.erase(pathIt);
        }
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

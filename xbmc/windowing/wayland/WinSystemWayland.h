/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Connection.h"
#include "Output.h"
#include "Seat.h"
#include "SeatInputProcessing.h"
#include "ShellSurface.h"
#include "Signals.h"
#include "WindowDecorationHandler.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/ActorProtocol.h"
#include "windowing/WinSystem.h"

#include <atomic>
#include <chrono>
#include <ctime>
#include <list>
#include <map>
#include <set>
#include <time.h>

#include <wayland-client.hpp>
#include <wayland-cursor.hpp>
#include <wayland-extra-protocols.hpp>

class IDispResource;

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CRegistry;
class CWindowDecorator;

class CWinSystemWayland : public CWinSystemBase,
                          public IInputHandler,
                          public IWindowDecorationHandler,
                          public IShellSurfaceHandler
{
public:
  CWinSystemWayland();
  ~CWinSystemWayland() noexcept override;

  const std::string GetName() override { return "wayland"; }

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool DestroyWindow() override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void FinishModeChange(RESOLUTION res) override;
  void FinishWindowResize(int newWidth, int newHeight) override;

  bool UseLimitedColor() override;

  void UpdateResolutions() override;

  bool CanDoWindowed() override;
  bool Minimize() override;

  bool HasCursor() override;
  void ShowOSMouse(bool show) override;

  std::string GetClipboardText() override;

  float GetSyncOutputRefreshRate();
  float GetDisplayLatency() override;
  float GetFrameLatencyAdjustment() override;
  std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) override;

  void Register(IDispResource* resource) override;
  void Unregister(IDispResource* resource) override;

  using PresentationFeedbackHandler = std::function<void(timespec /* tv */, std::uint32_t /* refresh */, std::uint32_t /* sync output id */, float /* sync output fps */, std::uint64_t /* msc */)>;
  CSignalRegistration RegisterOnPresentationFeedback(const PresentationFeedbackHandler& handler);

  std::vector<std::string> GetConnectedOutputs() override;

  // winevents override
  bool MessagePump() override;

protected:
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;
  CSizeInt GetBufferSize() const
  {
    return m_bufferSize;
  }
  std::unique_ptr<CConnection> const& GetConnection()
  {
    return m_connection;
  }
  wayland::surface_t GetMainSurface()
  {
    return m_surface;
  }
  IShellSurface* GetShellSurface() { return m_shellSurface.get(); }

  void PrepareFramePresentation();
  void FinishFramePresentation();
  virtual void SetContextSize(CSizeInt size) = 0;
  virtual IShellSurface* CreateShellSurface(const std::string& name);

  // IShellSurfaceHandler
  void OnConfigure(std::uint32_t serial, CSizeInt size, IShellSurface::StateBitset state) override;
  void OnClose() override;

  virtual std::unique_ptr<CSeat> CreateSeat(std::uint32_t name, wayland::seat_t& seat);

private:
  // IInputHandler
  void OnEnter(InputType type) override;
  void OnLeave(InputType type) override;
  void OnEvent(InputType type, XBMC_Event& event) override;
  void OnSetCursor(std::uint32_t seatGlobalName, std::uint32_t serial) override;

  // IWindowDecorationHandler
  void OnWindowMove(const wayland::seat_t& seat, std::uint32_t serial) override;
  void OnWindowResize(const wayland::seat_t& seat, std::uint32_t serial, wayland::shell_surface_resize edge) override;
  void OnWindowShowContextMenu(const wayland::seat_t& seat, std::uint32_t serial, CPointInt position) override;
  void OnWindowClose() override;
  void OnWindowMaximize() override;
  void OnWindowMinimize() override;

  // Registry handlers
  void OnSeatAdded(std::uint32_t name, wayland::proxy_t&& seat);
  void OnSeatRemoved(std::uint32_t name);
  void OnOutputAdded(std::uint32_t name, wayland::proxy_t&& output);
  void OnOutputRemoved(std::uint32_t name);

  void LoadDefaultCursor();
  void SendFocusChange(bool focus);
  bool SetResolutionExternal(bool fullScreen, RESOLUTION_INFO const& res);
  void SetResolutionInternal(CSizeInt size, int scale, IShellSurface::StateBitset state, bool sizeIncludesDecoration, bool mustAck = false, std::uint32_t configureSerial = 0u);
  struct Sizes
  {
    CSizeInt surfaceSize;
    CSizeInt bufferSize;
    CSizeInt configuredSize;
  };
  Sizes CalculateSizes(CSizeInt size, int scale, IShellSurface::StateBitset state, bool sizeIncludesDecoration);
  struct SizeUpdateInformation
  {
    bool surfaceSizeChanged : 1;
    bool bufferSizeChanged : 1;
    bool configuredSizeChanged : 1;
    bool bufferScaleChanged : 1;
  };
  SizeUpdateInformation UpdateSizeVariables(CSizeInt size, int scale, IShellSurface::StateBitset state, bool sizeIncludesDecoration);
  void ApplySizeUpdate(SizeUpdateInformation update);
  void ApplyNextState();

  std::string UserFriendlyOutputName(std::shared_ptr<COutput> const& output);
  std::shared_ptr<COutput> FindOutputByUserFriendlyName(std::string const& name);
  std::shared_ptr<COutput> FindOutputByWaylandOutput(wayland::output_t const& output);

  // Called when wl_output::done is received for an output, i.e. associated
  // information like modes is available
  void OnOutputDone(std::uint32_t name);
  void UpdateBufferScale();
  void ApplyBufferScale();
  void ApplyOpaqueRegion();
  void ApplyWindowGeometry();
  void UpdateTouchDpi();
  void ApplyShellSurfaceState(IShellSurface::StateBitset state);

  void ProcessMessages();
  void AckConfigure(std::uint32_t serial);

  timespec GetPresentationClockTime();

  // Globals
  // -------
  std::unique_ptr<CConnection> m_connection;
  std::unique_ptr<CRegistry> m_registry;
  /**
   * Registry used exclusively for wayland::seat_t
   *
   * Use extra registry because seats can only be registered after the surface
   * has been created
   */
  std::unique_ptr<CRegistry> m_seatRegistry;
  wayland::compositor_t m_compositor;
  wayland::shm_t m_shm;
  wayland::presentation_t m_presentation;

  std::unique_ptr<IShellSurface> m_shellSurface;

  // Frame callback handling
  // -----------------------
  wayland::callback_t m_frameCallback;
  CEvent m_frameCallbackEvent;

  // Seat handling
  // -------------
  std::map<std::uint32_t, std::unique_ptr<CSeat>> m_seats;
  CCriticalSection m_seatsMutex;
  std::unique_ptr<CSeatInputProcessing> m_seatInputProcessing;
  std::map<std::uint32_t, std::shared_ptr<COutput>> m_outputs;
  /// outputs that did not receive their done event yet
  std::map<std::uint32_t, std::shared_ptr<COutput>> m_outputsInPreparation;
  CCriticalSection m_outputsMutex;

  // Windowed mode
  // -------------
  std::unique_ptr<CWindowDecorator> m_windowDecorator;

  // Cursor
  // ------
  bool m_osCursorVisible{true};
  wayland::cursor_theme_t m_cursorTheme;
  wayland::buffer_t m_cursorBuffer;
  wayland::cursor_image_t m_cursorImage;
  wayland::surface_t m_cursorSurface;

  // Presentation feedback
  // ---------------------
  clockid_t m_presentationClock;
  struct SurfaceSubmission
  {
    timespec submissionTime;
    float latency;
    wayland::presentation_feedback_t feedback;
    SurfaceSubmission(timespec const& submissionTime, wayland::presentation_feedback_t const& feedback);
  };
  std::list<SurfaceSubmission> m_surfaceSubmissions;
  CCriticalSection m_surfaceSubmissionsMutex;
  /// Protocol object ID of the sync output returned by wp_presentation
  std::uint32_t m_syncOutputID;
  /// Refresh rate of sync output returned by wp_presentation
  std::atomic<float> m_syncOutputRefreshRate{0.0f};
  static constexpr int LATENCY_MOVING_AVERAGE_SIZE{30};
  std::atomic<float> m_latencyMovingAverage;
  CSignalHandlerList<PresentationFeedbackHandler> m_presentationFeedbackHandlers;

  std::chrono::steady_clock::time_point m_frameStartTime{};

  // IDispResource
  // -------------
  std::set<IDispResource*> m_dispResources;
  CCriticalSection m_dispResourcesMutex;

  // Surface state
  // -------------
  wayland::surface_t m_surface;
  wayland::output_t m_lastSetOutput;
  /// Set of outputs that show some part of our main surface as indicated by
  /// compositor
  std::set<std::shared_ptr<COutput>> m_surfaceOutputs;
  CCriticalSection m_surfaceOutputsMutex;
  /// Size of our surface in "surface coordinates" (i.e. without scaling applied)
  /// and without window decorations
  CSizeInt m_surfaceSize;
  /// Size of the buffer that should back the surface (i.e. with scaling applied)
  CSizeInt m_bufferSize;
  /// Size of the whole window including window decorations as given by configure
  CSizeInt m_configuredSize;
  /// Scale in use for main surface buffer
  int m_scale{1};
  /// Shell surface state last acked
  IShellSurface::StateBitset m_shellSurfaceState;
  /// Whether the shell surface is waiting for initial configure
  bool m_shellSurfaceInitializing{false};
  struct
  {
    bool mustBeAcked{false};
    std::uint32_t configureSerial{};
    CSizeInt configuredSize;
    int scale{1};
    IShellSurface::StateBitset shellSurfaceState;
  } m_next;
  bool m_waitingForApply{false};

  // Internal communication
  // ----------------------
  /// Protocol for communicating events to the main thread
  Actor::Protocol m_protocol;

  // Configure state
  // ---------------
  bool m_firstSerialAcked{false};
  std::uint32_t m_lastAckedSerial{0u};
  /// Whether this is the first call to SetFullScreen
  bool m_isInitialSetFullScreen{true};
};


}
}
}

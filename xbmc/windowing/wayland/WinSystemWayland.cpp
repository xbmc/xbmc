/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "WinSystemWayland.h"

#include <algorithm>
#include <limits>
#include <numeric>

#if defined(HAVE_LIBVA)
#include <va/va_wayland.h>
#endif

#include "Application.h"
#include "Connection.h"
#include "cores/VideoPlayer/Process/wayland/ProcessInfoWayland.h"
#include "guilib/DispResource.h"
#include "guilib/LocalizeStrings.h"
#include "input/InputManager.h"
#include "input/touch/generic/GenericTouchActionHandler.h"
#include "input/touch/generic/GenericTouchInputHandler.h"
#include "linux/PlatformConstants.h"
#include "linux/TimeUtils.h"
#include "messaging/ApplicationMessenger.h"
#include "OSScreenSaverIdleInhibitUnstableV1.h"
#include "Registry.h"
#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "ShellSurfaceWlShell.h"
#include "ShellSurfaceXdgShellUnstableV6.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "VideoSyncWpPresentation.h"
#include "WindowDecorator.h"
#include "WinEventsWayland.h"
#include "windowing/linux/OSScreenSaverFreedesktop.h"
#include "utils/ActorProtocol.h"
#include "utils/TimeUtils.h"

#if defined(HAVE_DBUS)
# include "windowing/linux/OSScreenSaverFreedesktop.h"
#endif

using namespace KODI::WINDOWING;
using namespace KODI::WINDOWING::WAYLAND;
using namespace std::placeholders;

namespace
{

RESOLUTION FindMatchingCustomResolution(CSizeInt size, float refreshRate)
{
  for (size_t res{RES_DESKTOP}; res < CDisplaySettings::GetInstance().ResolutionInfoSize(); ++res)
  {
    auto const& resInfo = CDisplaySettings::GetInstance().GetResolutionInfo(res);
    if (resInfo.iWidth == size.Width() && resInfo.iHeight == size.Height() && MathUtils::FloatEquals(resInfo.fRefreshRate, refreshRate, 0.0005f))
    {
      return static_cast<RESOLUTION> (res);
    }
  }
  return RES_INVALID;
}

struct OutputScaleComparer
{
  bool operator()(std::shared_ptr<COutput> const& output1, std::shared_ptr<COutput> const& output2)
  {
    return output1->GetScale() < output2->GetScale();
  }
};

struct OutputCurrentRefreshRateComparer
{
  bool operator()(std::shared_ptr<COutput> const& output1, std::shared_ptr<COutput> const& output2)
  {
    return output1->GetCurrentMode().refreshMilliHz < output2->GetCurrentMode().refreshMilliHz;
  }
};

/// Scope guard for Actor::Message
class MessageHandle : public KODI::UTILS::CScopeGuard<Actor::Message*, nullptr, void(Actor::Message*)>
{
public:
  MessageHandle() : CScopeGuard{std::bind(&Actor::Message::Release, std::placeholders::_1), nullptr} {}
  explicit MessageHandle(Actor::Message* message) : CScopeGuard{std::bind(&Actor::Message::Release, std::placeholders::_1), message} {}
  Actor::Message* Get() { return static_cast<Actor::Message*> (*this); }
};

/**
 * Protocol for communication between Wayland event thread and main thread
 *
 * Many messages received from the Wayland compositor must be processed at a
 * defined time between frame rendering, such as resolution switches. Thus
 * they are pushed to the main thread for processing.
 *
 * The protocol is strictly uni-directional from event to main thread at the moment,
 * so \ref Actor::Protocol is mainly used as an event queue.
 */
namespace WinSystemWaylandProtocol
{

enum OutMessage
{
  CONFIGURE,
  OUTPUT_HOTPLUG,
  BUFFER_SCALE
};

struct MsgConfigure
{
  std::uint32_t serial;
  CSizeInt surfaceSize;
  IShellSurface::StateBitset state;
};

struct MsgBufferScale
{
  int scale;
};

};

}

CWinSystemWayland::CWinSystemWayland()
: CWinSystemBase{}, m_protocol{"WinSystemWaylandInternal"}
{
  m_eWindowSystem = WINDOW_SYSTEM_WAYLAND;
}

CWinSystemWayland::~CWinSystemWayland() noexcept
{
  DestroyWindowSystem();
}

bool CWinSystemWayland::InitWindowSystem()
{
  wayland::set_log_handler([](std::string message)
  {
    CLog::Log(LOGWARNING, "wayland-client log message: %s", message.c_str());
  });

  VIDEOPLAYER::CProcessInfoWayland::Register();

  CLog::LogF(LOGINFO, "Connecting to Wayland server");
  m_connection.reset(new CConnection);
  m_registry.reset(new CRegistry{*m_connection});

  m_registry->RequestSingleton(m_compositor, 1, 4);
  m_registry->RequestSingleton(m_shm, 1, 1);
  m_registry->RequestSingleton(m_presentation, 1, 1, false);
  // version 2 adds done() -> required
  // version 3 adds destructor -> optional
  m_registry->Request<wayland::output_t>(2, 3, std::bind(&CWinSystemWayland::OnOutputAdded, this, _1, _2), std::bind(&CWinSystemWayland::OnOutputRemoved, this, _1));

  m_registry->Bind();

  if (m_presentation)
  {
    m_presentation.on_clock_id() = [this](std::uint32_t clockId)
    {
      CLog::Log(LOGINFO, "Wayland presentation clock: %" PRIu32, clockId);
      m_presentationClock = static_cast<clockid_t> (clockId);
    };
  }

  // Do another roundtrip to get initial wl_output information
  m_connection->GetDisplay().roundtrip();
  if (m_outputs.empty())
  {
    throw std::runtime_error("No outputs received from compositor");
  }

  // Event loop is started in CreateWindow

  // pointer is by default not on this window, will be immediately rectified
  // by the enter() events if it is
  CServiceBroker::GetInputManager().SetMouseActive(false);
  // Always use the generic touch action handler
  CGenericTouchInputHandler::GetInstance().RegisterHandler(&CGenericTouchActionHandler::GetInstance());

  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemWayland::DestroyWindowSystem()
{
  DestroyWindow();
  // wl_display_disconnect frees all proxy objects, so we have to make sure
  // all stuff is gone on the C++ side before that
  m_cursorSurface = wayland::surface_t{};
  m_cursorBuffer = wayland::buffer_t{};
  m_cursorImage = wayland::cursor_image_t{};
  m_cursorTheme = wayland::cursor_theme_t{};
  m_outputsInPreparation.clear();
  m_outputs.clear();

  if (m_registry)
  {
    m_registry->UnbindSingletons();
  }
  m_registry.reset();
  m_connection.reset();

  CGenericTouchInputHandler::GetInstance().UnregisterHandler();

  return CWinSystemBase::DestroyWindowSystem();
}

bool CWinSystemWayland::CreateNewWindow(const std::string& name,
                                        bool fullScreen,
                                        RESOLUTION_INFO& res)
{
  CLog::LogF(LOGINFO, "Starting %s size %dx%d", fullScreen ? "full screen" : "windowed", res.iWidth, res.iHeight);

  m_surface = m_compositor.create_surface();
  m_surface.on_enter() = [this](wayland::output_t wloutput)
  {
    if (auto output = FindOutputByWaylandOutput(wloutput))
    {
      CLog::Log(LOGDEBUG, "Entering output \"%s\" with scale %d and %.3f dpi", UserFriendlyOutputName(output).c_str(), output->GetScale(), output->GetCurrentDpi());
      CSingleLock lock(m_surfaceOutputsMutex);
      m_surfaceOutputs.emplace(output);
      lock.Leave();
      UpdateBufferScale();
      UpdateTouchDpi();
    }
    else
    {
      CLog::Log(LOGWARNING, "Entering output that was not configured yet, ignoring");
    }
  };
  m_surface.on_leave() = [this](wayland::output_t wloutput)
  {
    if (auto output = FindOutputByWaylandOutput(wloutput))
    {
      CLog::Log(LOGDEBUG, "Leaving output \"%s\" with scale %d", UserFriendlyOutputName(output).c_str(), output->GetScale());
      CSingleLock lock(m_surfaceOutputsMutex);
      m_surfaceOutputs.erase(output);
      lock.Leave();
      UpdateBufferScale();
      UpdateTouchDpi();
    }
    else
    {
      CLog::Log(LOGWARNING, "Leaving output that was not configured yet, ignoring");
    }
  };

  m_windowDecorator.reset(new CWindowDecorator(*this, *m_connection, m_surface));

  if (fullScreen)
  {
    m_shellSurfaceState.set(IShellSurface::STATE_FULLSCREEN);
  }
  // Assume we're active on startup until someone tells us otherwise
  m_shellSurfaceState.set(IShellSurface::STATE_ACTIVATED);
  // Try with this resolution if compositor does not say otherwise
  UpdateSizeVariables({res.iWidth, res.iHeight}, m_scale, m_shellSurfaceState, false);

  m_shellSurface.reset(CShellSurfaceXdgShellUnstableV6::TryCreate(*this, *m_connection, m_surface, name, KODI::LINUX::DESKTOP_FILE_NAME));
  if (!m_shellSurface)
  {
    CLog::LogF(LOGWARNING, "Compositor does not support xdg_shell unstable v6 protocol - falling back to wl_shell, not all features might work");
    m_shellSurface.reset(new CShellSurfaceWlShell(*this, *m_connection, m_surface, name, KODI::LINUX::DESKTOP_FILE_NAME));
  }

  if (fullScreen)
  {
    // Try to start on correct monitor and with correct buffer scale
    auto output = FindOutputByUserFriendlyName(CServiceBroker::GetSettings().GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR));
    auto wlOutput = output ? output->GetWaylandOutput() : wayland::output_t{};
    m_lastSetOutput = wlOutput;
    m_shellSurface->SetFullScreen(wlOutput, res.fRefreshRate);
    if (output && m_surface.can_set_buffer_scale())
    {
      m_scale = output->GetScale();
      ApplyBufferScale();
    }
  }

  // Just remember initial width/height for context creation in OnConfigure
  // This is used for sizing the EGLSurface
  m_shellSurfaceInitializing = true;
  m_shellSurface->Initialize();
  m_shellSurfaceInitializing = false;

  // Apply window decorations if necessary
  m_windowDecorator->SetState(m_configuredSize, m_scale, m_shellSurfaceState);

  // Update resolution with real size as it could have changed due to configure()
  UpdateDesktopResolution(res, 0, m_bufferSize.Width(), m_bufferSize.Height(), res.fRefreshRate);
  res.bFullScreen = fullScreen;

  m_seatRegistry.reset(new CRegistry{*m_connection});
  // version 2 adds name event -> optional
  // version 4 adds wl_keyboard repeat_info -> optional
  // version 5 adds discrete axis events in wl_pointer -> unused
  m_seatRegistry->Request<wayland::seat_t>(1, 5, std::bind(&CWinSystemWayland::OnSeatAdded, this, _1, _2), std::bind(&CWinSystemWayland::OnSeatRemoved, this, _1));
  m_seatRegistry->Bind();

  if (m_seatProcessors.empty())
  {
    CLog::Log(LOGWARNING, "Wayland compositor did not announce a wl_seat - you will not have any input devices for the time being");
  }

  // Now start processing events
  //
  // There are two stages to the event handling:
  // * Initialization (which ends here): Everything runs synchronously and init
  //   code that needs events processed must call roundtrip().
  //   This is done for simplicity because it is a lot easier than to make
  //   everything event-based and thread-safe everywhere in the startup code,
  //   which is also not really necessary.
  // * Runtime (which starts here): Every object creation from now on
  //   needs to take great care to be thread-safe:
  //   Since the event pump is always running now, there is a tiny window between
  //   creating an object and attaching the C++ event handlers during which
  //   events can get queued and dispatched for the object but the handlers have
  //   not been set yet. Consequently, the events would get lost.
  //   However, this does not apply to objects that are created in response to
  //   compositor events. Since the callbacks are called from the event processing
  //   thread and ran strictly sequentially, no other events are dispatched during
  //   the runtime of a callback. Luckily this applies to global binding like
  //   wl_output and wl_seat and thus to most if not all runtime object creation
  //   cases we have to support.
  //   There is another problem when Wayland objects are destructed from the main
  //   thread: An event handler could be running in parallel, resulting in certain
  //   doom. So objects should only be deleted in response to compositor events, too.
  //   They might be hiding behind class member variables, so be wary.
  //   Note that this does not apply to global teardown since the event pump is
  //   stopped then.
  CWinEventsWayland::SetDisplay(&m_connection->GetDisplay());

  return true;
}

bool CWinSystemWayland::DestroyWindow()
{
  // Make sure no more events get processed when we kill the instances
  CWinEventsWayland::SetDisplay(nullptr);

  m_shellSurface.reset();
  // waylandpp automatically calls wl_surface_destroy when the last reference is removed
  m_surface = wayland::surface_t();
  m_windowDecorator.reset();
  m_seatProcessors.clear();
  m_lastSetOutput.proxy_release();
  m_surfaceOutputs.clear();
  m_surfaceSubmissions.clear();
  m_seatRegistry.reset();

  return true;
}

bool CWinSystemWayland::CanDoWindowed()
{
  return true;
}

int CWinSystemWayland::GetNumScreens()
{
  // Multiple screen/resolution support in core Kodi badly needs refactoring, but as
  // it touches a lot of code we just do it like X11 for the moment:
  // Pretend that there is only one screen, show more screens with
  // custom names in the GUI using an #ifdef in DisplaySettings
  // - otherwise we would just get a selection between "Full Screen #1" and
  // "Full Screen #2" etc. instead of actual monitor names.
  return 1;
}

int CWinSystemWayland::GetCurrentScreen()
{
  // See GetNumScreens()
  return 1;
}

void CWinSystemWayland::GetConnectedOutputs(std::vector<std::string>* outputs)
{
  CSingleLock lock(m_outputsMutex);
  std::transform(m_outputs.cbegin(), m_outputs.cend(), std::back_inserter(*outputs),
                 [this](decltype(m_outputs)::value_type const& pair)
                 {
                   return UserFriendlyOutputName(pair.second); });
}

void CWinSystemWayland::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();

  CDisplaySettings::GetInstance().ClearCustomResolutions();

  // Mimic X11:
  // Only show resolutions for the currently selected output
  std::string userOutput = CServiceBroker::GetSettings().GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);

  CSingleLock lock(m_outputsMutex);

  if (m_outputs.empty())
  {
    // *Usually* this should not happen - just give up
    return;
  }

  auto output = FindOutputByUserFriendlyName(userOutput);
  if (!output && m_lastSetOutput)
  {
    // Fallback to current output
    output = FindOutputByWaylandOutput(m_lastSetOutput);
  }
  if (!output)
  {
    // Well just use the first one
    output = m_outputs.begin()->second;
  }

  std::string outputName = UserFriendlyOutputName(output);

  auto const& modes = output->GetModes();
  auto const& currentMode = output->GetCurrentMode();
  auto physicalSize = output->GetPhysicalSize();
  CLog::LogF(LOGINFO, "User wanted output \"%s\", we now have \"%s\" size %dx%d mm with %zu mode(s):", userOutput.c_str(), outputName.c_str(), physicalSize.Width(), physicalSize.Height(), modes.size());

  for (auto const& mode : modes)
  {
    bool isCurrent = (mode == currentMode);
    float pixelRatio = output->GetPixelRatioForMode(mode);
    CLog::LogF(LOGINFO, "- %dx%d @%.3f Hz pixel ratio %.3f%s", mode.size.Width(), mode.size.Height(), mode.refreshMilliHz / 1000.0f, pixelRatio, isCurrent ? " current" : "");

    RESOLUTION_INFO res;
    UpdateDesktopResolution(res, 0, mode.size.Width(), mode.size.Height(), mode.GetRefreshInHz());
    res.strOutput = outputName;
    res.fPixelRatio = pixelRatio;

    if (isCurrent)
    {
      CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = res;
    }
    else
    {
      CDisplaySettings::GetInstance().AddResolutionInfo(res);
    }
  }

  CDisplaySettings::GetInstance().ApplyCalibrations();
}

std::shared_ptr<COutput> CWinSystemWayland::FindOutputByUserFriendlyName(const std::string& name)
{
  CSingleLock lock(m_outputsMutex);
  auto outputIt = std::find_if(m_outputs.begin(), m_outputs.end(),
                               [this, &name](decltype(m_outputs)::value_type const& entry)
                               {
                                 return (name == UserFriendlyOutputName(entry.second));
                               });

  return (outputIt == m_outputs.end() ? nullptr : outputIt->second);
}

std::shared_ptr<COutput> CWinSystemWayland::FindOutputByWaylandOutput(wayland::output_t const& output)
{
  CSingleLock lock(m_outputsMutex);
  auto outputIt = std::find_if(m_outputs.begin(), m_outputs.end(),
                               [this, &output](decltype(m_outputs)::value_type const& entry)
                               {
                                 return (output == entry.second->GetWaylandOutput());
                               });

  return (outputIt == m_outputs.end() ? nullptr : outputIt->second);
}

/**
 * Change resolution and window state on Kodi request
 *
 * This function is used for updating resolution when Kodi initiates a resolution
 * change, such as when changing between full screen and windowed mode or when
 * selecting a different monitor or resolution in the settings.
 *
 * Size updates originating from compositor events (such as configure or buffer
 * scale changes) should not use this function, but \ref SetResolutionInternal
 * instead.
 *
 * \param fullScreen whether to go full screen or windowed
 * \param res resolution to set
 * \return whether the requested resolution was actually set - is false e.g.
 *         when already in full screen mode since the application cannot
 *         set the size then
 */
bool CWinSystemWayland::SetResolutionExternal(bool fullScreen, RESOLUTION_INFO const& res)
{
  // In fullscreen modes, we never change the surface size on Kodi's request,
  // but only when the compositor tells us to. At least xdg_shell specifies
  // that with state fullscreen the dimensions given in configure() must
  // always be observed.
  // This does mean that the compositor has no way of knowing which resolution
  // we would (in theory) want. Since no compositor implements dynamic resolution
  // switching at the moment, this is not a problem. If it is some day implemented
  // in compositors, this code must be changed to match the behavior that is
  // expected then anyway.

  // We can honor the Kodi-requested size only if we are not bound by configure rules,
  // which applies for maximized and fullscreen states.
  // Also, setting an unconfigured size when just going fullscreen makes no sense.
  // Give precedence to the size we have still pending, if any.
  bool mustHonorSize{m_waitingForApply || m_shellSurfaceState.test(IShellSurface::STATE_MAXIMIZED) || m_shellSurfaceState.test(IShellSurface::STATE_FULLSCREEN) || fullScreen};

  CLog::LogF(LOGINFO, "Kodi asked to switch mode to %dx%d @%.3f Hz on output \"%s\" %s", res.iWidth, res.iHeight, res.fRefreshRate, res.strOutput.c_str(), fullScreen ? "full screen" : "windowed");

  if (fullScreen)
  {
    // Try to match output
    auto output = FindOutputByUserFriendlyName(res.strOutput);
    auto wlOutput = output ? output->GetWaylandOutput() : wayland::output_t{};
    if (!m_shellSurfaceState.test(IShellSurface::STATE_FULLSCREEN) || (m_lastSetOutput != wlOutput))
    {
      // Remember the output we set last so we don't set it again until we
      // either go windowed or were on a different output
      m_lastSetOutput = wlOutput;

      if (output)
      {
        CLog::LogF(LOGDEBUG, "Resolved output \"%s\" to bound Wayland global %u", res.strOutput.c_str(), output->GetGlobalName());
      }
      else
      {
        CLog::LogF(LOGINFO, "Could not match output \"%s\" to a currently available Wayland output, falling back to default output", res.strOutput.c_str());
      }

      CLog::LogF(LOGDEBUG, "Setting full-screen with refresh rate %.3f", res.fRefreshRate);
      m_shellSurface->SetFullScreen(wlOutput, res.fRefreshRate);
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Not setting full screen: already full screen on requested output");
    }
  }
  else
  {
    if (m_shellSurfaceState.test(IShellSurface::STATE_FULLSCREEN))
    {
      CLog::LogF(LOGDEBUG, "Setting windowed");
      m_shellSurface->SetWindowed();
    }
    else
    {
      CLog::LogF(LOGDEBUG, "Not setting windowed: already windowed");
    }
  }

  // Set Kodi-provided size only if we are free to choose any size, otherwise
  // wait for the compositor configure
  if (!mustHonorSize)
  {
    CLog::LogF(LOGDEBUG, "Directly setting windowed size %dx%d on Kodi request", res.iWidth, res.iHeight);
    // Kodi is directly setting window size, apply
    auto updateResult = UpdateSizeVariables({res.iWidth, res.iHeight}, m_scale, m_shellSurfaceState, false);
    ApplySizeUpdate(updateResult);
  }

  bool wasInitialSetFullScreen{m_isInitialSetFullScreen};
  m_isInitialSetFullScreen = false;

  // Need to return true
  // * when this SetFullScreen() call was free to change the context size (and possibly did so)
  // * on first SetFullScreen so GraphicsContext gets resolution
  // Otherwise, Kodi must keep the old resolution.
  return !mustHonorSize || wasInitialSetFullScreen;
}

bool CWinSystemWayland::ResizeWindow(int, int, int, int)
{
  // CGraphicContext is "smart" and calls ResizeWindow or SetFullScreen depending
  // on some state like wheter we were already full screen. But actually the processing
  // here is always identical, so we are using a common function to handle both.
  auto& res = CDisplaySettings::GetInstance().GetResolutionInfo(RES_WINDOW);
  // The newWidth/newHeight parameters are taken from RES_WINDOW anyway, so we can just
  // ignore them
  return SetResolutionExternal(false, res);
}

bool CWinSystemWayland::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool)
{
  return SetResolutionExternal(fullScreen, res);
}

void CWinSystemWayland::ApplySizeUpdate(SizeUpdateInformation update)
{
  if (update.bufferScaleChanged)
  {
    // Buffer scale must also match egl size configuration
    ApplyBufferScale();
  }
  if (update.surfaceSizeChanged)
  {
    // Mark everything opaque so the compositor can render it faster
    // Do it here so size always matches the configured egl surface
    CLog::LogF(LOGDEBUG, "Setting opaque region size %dx%d", m_surfaceSize.Width(), m_surfaceSize.Height());
    wayland::region_t opaqueRegion{m_compositor.create_region()};
    opaqueRegion.add(0, 0, m_surfaceSize.Width(), m_surfaceSize.Height());
    m_surface.set_opaque_region(opaqueRegion);
  }
  if (update.configuredSizeChanged)
  {
    // Update window decoration state
    m_windowDecorator->SetState(m_configuredSize, m_scale, m_shellSurfaceState);
    m_shellSurface->SetWindowGeometry(m_windowDecorator->GetWindowGeometry());
  }
  // Set always, because of initialization order GL context has to keep track of
  // whether the size changed. If we skip based on update.bufferSizeChanged here,
  // GL context will never get its initial size set.
  SetContextSize(m_bufferSize);
}

void CWinSystemWayland::ProcessMessages()
{
  if (m_waitingForApply)
  {
    // Do not put multiple size updates into the pipeline, this would only make
    // it more complicated without any real benefit. Wait until the size was reconfigured,
    // then process events again.
    return;
  }

  Actor::Message* message{};
  MessageHandle lastConfigureMessage;
  int skippedConfigures{-1};
  int newScale{m_scale};

  while (m_protocol.ReceiveOutMessage(&message))
  {
    MessageHandle guard{message};
    switch (message->signal)
    {
      case WinSystemWaylandProtocol::CONFIGURE:
        // Do not directly process configures, get the last one queued:
        // While resizing, the compositor will usually send a configure event
        // each time the mouse moves without any throttling (i.e. multiple times
        // per rendered frame).
        // Going through all those and applying them would waste a lot of time when
        // we already know that the size is not final and will change again anyway.
        skippedConfigures++;
        lastConfigureMessage = std::move(guard);
        break;
      case WinSystemWaylandProtocol::OUTPUT_HOTPLUG:
      {
        CLog::LogF(LOGDEBUG, "Output hotplug, re-reading resolutions");
        UpdateResolutions();
        CSingleLock lock(m_outputsMutex);
        auto const& desktopRes = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
        auto output = FindOutputByUserFriendlyName(desktopRes.strOutput);
        auto const& wlOutput = output->GetWaylandOutput();
        // Maybe the output that was added was the one we should be on?
        if (m_bFullScreen && m_lastSetOutput != wlOutput)
        {
          CLog::LogF(LOGDEBUG, "Output hotplug resulted in monitor set in settings appearing, switching");
          // Switch to this output
          m_lastSetOutput = wlOutput;
          m_shellSurface->SetFullScreen(wlOutput, desktopRes.fRefreshRate);
          // SetOutput will result in a configure that updates the actual context size
        }
      }
        break;
      case WinSystemWaylandProtocol::BUFFER_SCALE:
        // Never update buffer scale if not possible to set it
        if (m_surface.can_set_buffer_scale())
        {
          newScale = (reinterpret_cast<WinSystemWaylandProtocol::MsgBufferScale*> (message->data))->scale;
        }
        break;
    }
  }

  if (lastConfigureMessage)
  {
    if (skippedConfigures > 0)
    {
      CLog::LogF(LOGDEBUG, "Skipped %d configures", skippedConfigures);
    }
    // Wayland will tell us here the size of the surface that was actually created,
    // which might be different from what we expected e.g. when fullscreening
    // on an output we chose - the compositor might have decided to use a different
    // output for example
    // It is very important that the EGL native module and the rendering system use the
    // Wayland-announced size for rendering or corrupted graphics output will result.
    auto configure = reinterpret_cast<WinSystemWaylandProtocol::MsgConfigure*> (lastConfigureMessage.Get()->data);
    CLog::LogF(LOGDEBUG, "Configure serial %u: size %dx%d state %s", configure->serial, configure->surfaceSize.Width(), configure->surfaceSize.Height(), IShellSurface::StateToString(configure->state).c_str());


    CSizeInt size = configure->surfaceSize;
    bool sizeIncludesDecoration = true;

    if (size.IsZero())
    {
      if (configure->state.test(IShellSurface::STATE_FULLSCREEN))
      {
        // Do not change current size - UpdateWithConfiguredSize must be called regardless in case
        // scale or something else changed
        size = m_configuredSize;
      }
      else
      {
        // Compositor has no preference and we're windowed
        // -> adopt windowed size that Kodi wants
        auto const& windowed = CDisplaySettings::GetInstance().GetResolutionInfo(RES_WINDOW);
        // Kodi resolution is buffer size, but SetResolutionInternal expects
        // surface size, so divide by m_scale
        size = CSizeInt{windowed.iWidth, windowed.iHeight} / newScale;
        CLog::LogF(LOGDEBUG, "Adapting Kodi windowed size %dx%d", size.Width(), size.Height());
        sizeIncludesDecoration = false;
      }
    }

    SetResolutionInternal(size, newScale, configure->state, sizeIncludesDecoration, true, configure->serial);
  }
  // If we were also configured, scale is already taken care of. But it could
  // also be a scale change without configure, so apply that.
  else if (m_scale != newScale)
  {
    SetResolutionInternal(m_configuredSize, newScale, m_shellSurfaceState, true, false);
  }
}

void CWinSystemWayland::ApplyShellSurfaceState(IShellSurface::StateBitset state)
{
  m_windowDecorator->SetState(m_configuredSize, m_scale, state);
  m_shellSurfaceState = state;
}

void CWinSystemWayland::OnConfigure(std::uint32_t serial, CSizeInt size, IShellSurface::StateBitset state)
{
  if (m_shellSurfaceInitializing)
  {
    CLog::LogF(LOGDEBUG, "Initial configure serial %u: size %dx%d state %s", serial, size.Width(), size.Height(), IShellSurface::StateToString(state).c_str());
    m_shellSurfaceState = state;
    if (!size.IsZero())
    {
      UpdateSizeVariables(size, m_scale, m_shellSurfaceState, true);
    }
    AckConfigure(serial);
  }
  else
  {
    WinSystemWaylandProtocol::MsgConfigure msg{serial, size, state};
    m_protocol.SendOutMessage(WinSystemWaylandProtocol::CONFIGURE, &msg, sizeof(msg));
  }
}

void CWinSystemWayland::AckConfigure(std::uint32_t serial)
{
  // Send ack if we have a new serial number or this is the first time
  // this function is called
  if (serial != m_lastAckedSerial || !m_firstSerialAcked)
  {
    CLog::LogF(LOGDEBUG, "Acking serial %u", serial);
    m_shellSurface->AckConfigure(serial);
    m_lastAckedSerial = serial;
    m_firstSerialAcked = true;
  }
}

/**
 * Recalculate sizes from given parameters, apply them and update Kodi CDisplaySettings
 * resolution if necessary
 *
 * This function should be called when events internal to the windowing system
 * such as a compositor configure lead to a size change.
 *
 * Call only from main thread.
 *
 * \param size configured size, can be zero if compositor does not have a preference
 * \param scale new buffer scale
 * \param sizeIncludesDecoration whether size includes the size of the window decorations if present
 */
void CWinSystemWayland::SetResolutionInternal(CSizeInt size, std::int32_t scale, IShellSurface::StateBitset state, bool sizeIncludesDecoration, bool mustAck, std::uint32_t configureSerial)
{
  // This should never be called while a size set is pending
  assert(!m_waitingForApply);

  bool fullScreen{state.test(IShellSurface::STATE_FULLSCREEN)};
  auto sizes = CalculateSizes(size, scale, state, sizeIncludesDecoration);

  CLog::LogF(LOGDEBUG, "Set size for serial %" PRIu32 ": %dx%d %s decoration at scale %d state %s", configureSerial, size.Width(), size.Height(), sizeIncludesDecoration ? "including" : "excluding", scale, IShellSurface::StateToString(state).c_str());

  // Get actual frame rate from monitor, take highest frame rate if multiple
  float refreshRate{m_fRefreshRate};
  {
    CSingleLock lock(m_surfaceOutputsMutex);
    auto maxRefreshIt = std::max_element(m_surfaceOutputs.cbegin(), m_surfaceOutputs.cend(), OutputCurrentRefreshRateComparer());
    if (maxRefreshIt != m_surfaceOutputs.cend())
    {
      refreshRate = (*maxRefreshIt)->GetCurrentMode().GetRefreshInHz();
      CLog::LogF(LOGDEBUG, "Resolved actual (maximum) refresh rate to %.3f Hz on output \"%s\"", refreshRate, UserFriendlyOutputName(*maxRefreshIt).c_str());
    }
  }

  m_next.mustBeAcked = mustAck;
  m_next.configureSerial = configureSerial;
  m_next.configuredSize = sizes.configuredSize;
  m_next.scale = scale;
  m_next.shellSurfaceState = state;

  // Check if any parameters of the Kodi resolution configuration changed
  if (refreshRate != m_fRefreshRate || sizes.bufferSize != m_bufferSize || m_bFullScreen != fullScreen)
  {
    if (!fullScreen)
    {
      if (m_bFullScreen)
      {
        XBMC_Event msg{XBMC_MODECHANGE};
        msg.mode.res = RES_WINDOW;
        SetWindowResolution(sizes.bufferSize.Width(), sizes.bufferSize.Height());
        CWinEvents::MessagePush(&msg);
        m_waitingForApply = true;
        CLog::LogF(LOGDEBUG, "Queued change to windowed mode size %dx%d", sizes.bufferSize.Width(), sizes.bufferSize.Height());
      }
      else
      {
        XBMC_Event msg{XBMC_VIDEORESIZE};
        msg.resize = {sizes.bufferSize.Width(), sizes.bufferSize.Height()};
        CWinEvents::MessagePush(&msg);
        m_waitingForApply = true;
        CLog::LogF(LOGDEBUG, "Queued change to windowed buffer size %dx%d", sizes.bufferSize.Width(), sizes.bufferSize.Height());
      }
    }
    else
    {
      // Find matching Kodi resolution member
      RESOLUTION res{FindMatchingCustomResolution(sizes.bufferSize, refreshRate)};
      if (res == RES_INVALID)
      {
        // Add new resolution if none found
        RESOLUTION_INFO newResInfo;
        UpdateDesktopResolution(newResInfo, 0, sizes.bufferSize.Width(), sizes.bufferSize.Height(), refreshRate);
        newResInfo.strOutput = CDisplaySettings::GetInstance().GetCurrentResolutionInfo().strOutput; // we just assume the compositor put us on the right output
        CDisplaySettings::GetInstance().AddResolutionInfo(newResInfo);
        CDisplaySettings::GetInstance().ApplyCalibrations();
        res = static_cast<RESOLUTION> (CDisplaySettings::GetInstance().ResolutionInfoSize() - 1);
      }

      XBMC_Event msg{XBMC_MODECHANGE};
      msg.mode.res = res;
      CWinEvents::MessagePush(&msg);
      m_waitingForApply = true;
      CLog::LogF(LOGDEBUG, "Queued change to resolution %d surface size %dx%d scale %d state %s", res, sizes.surfaceSize.Width(), sizes.surfaceSize.Height(), scale, IShellSurface::StateToString(state).c_str());
    }
  }
  else
  {
    // Apply directly, Kodi resolution does not change
    ApplyNextState();
  }
}

void CWinSystemWayland::FinishModeChange(RESOLUTION res)
{
  auto& resInfo = CDisplaySettings::GetInstance().GetResolutionInfo(res);

  ApplyNextState();

  m_fRefreshRate = resInfo.fRefreshRate;
  m_bFullScreen = resInfo.bFullScreen;
  m_waitingForApply = false;
}

void CWinSystemWayland::FinishWindowResize(int, int)
{
  ApplyNextState();
  m_waitingForApply = false;
}

void CWinSystemWayland::ApplyNextState()
{
  CLog::LogF(LOGDEBUG, "Applying next state: serial %" PRIu32 " configured size %dx%d scale %d state %s", m_next.configureSerial, m_next.configuredSize.Width(), m_next.configuredSize.Height(), m_next.scale, IShellSurface::StateToString(m_next.shellSurfaceState).c_str());

  ApplyShellSurfaceState(m_next.shellSurfaceState);
  auto updateResult = UpdateSizeVariables(m_next.configuredSize, m_next.scale, m_next.shellSurfaceState, true);
  ApplySizeUpdate(updateResult);

  if (m_next.mustBeAcked)
  {
    AckConfigure(m_next.configureSerial);
  }
}

CWinSystemWayland::Sizes CWinSystemWayland::CalculateSizes(CSizeInt size, int scale, IShellSurface::StateBitset state, bool sizeIncludesDecoration)
{
  Sizes result;

  // Clamp to a sensible range
  constexpr int MIN_WIDTH{300};
  constexpr int MIN_HEIGHT{200};
  if (size.Width() < MIN_WIDTH)
  {
    CLog::LogF(LOGWARNING, "Width %d is very small, clamping to %d", size.Width(), MIN_WIDTH);
    size.SetWidth(MIN_WIDTH);
  }
  if (size.Height() < MIN_HEIGHT)
  {
    CLog::LogF(LOGWARNING, "Height %d is very small, clamping to %d", size.Height(), MIN_HEIGHT);
    size.SetHeight(MIN_HEIGHT);
  }

  // Depending on whether the size has decorations included (i.e. comes from the
  // compositor or from Kodi), we need to calculate differently
  if (sizeIncludesDecoration)
  {
    result.configuredSize = size;
    result.surfaceSize = m_windowDecorator->CalculateMainSurfaceSize(size, state);
  }
  else
  {
    result.surfaceSize = size;
    result.configuredSize = m_windowDecorator->CalculateFullSurfaceSize(size, state);
  }

  result.bufferSize = result.surfaceSize * scale;

  return result;
}


/**
 * Calculate internal resolution from surface size and set variables
 *
 * \param next surface size
 * \param scale new buffer scale
 * \param state window state to determine whether decorations are enabled at all
 * \param sizeIncludesDecoration if true, given size includes potential window decorations
 * \return whether main buffer (not surface) size changed
 */
CWinSystemWayland::SizeUpdateInformation CWinSystemWayland::UpdateSizeVariables(CSizeInt size, int scale, IShellSurface::StateBitset state, bool sizeIncludesDecoration)
{
  CLog::LogF(LOGDEBUG, "Set size %dx%d scale %d %s decorations with state %s", size.Width(), size.Height(), scale, sizeIncludesDecoration ? "including" : "excluding", IShellSurface::StateToString(state).c_str());

  auto oldSurfaceSize = m_surfaceSize;
  auto oldBufferSize = m_bufferSize;
  auto oldConfiguredSize = m_configuredSize;
  auto oldBufferScale = m_scale;

  m_scale = scale;
  auto sizes = CalculateSizes(size, scale, state, sizeIncludesDecoration);
  m_surfaceSize = sizes.surfaceSize;
  m_bufferSize = sizes.bufferSize;
  m_configuredSize = sizes.configuredSize;

  SizeUpdateInformation changes{m_surfaceSize != oldSurfaceSize, m_bufferSize != oldBufferSize, m_configuredSize != oldConfiguredSize, m_scale != oldBufferScale};

  if (changes.surfaceSizeChanged)
  {
    CLog::LogF(LOGINFO, "Surface size changed: %dx%d -> %dx%d", oldSurfaceSize.Width(), oldSurfaceSize.Height(), m_surfaceSize.Width(), m_surfaceSize.Height());
  }
  if (changes.bufferSizeChanged)
  {
    CLog::LogF(LOGINFO, "Buffer size changed: %dx%d -> %dx%d", oldBufferSize.Width(), oldBufferSize.Height(), m_bufferSize.Width(), m_bufferSize.Height());
  }
  if (changes.configuredSizeChanged)
  {
    CLog::LogF(LOGINFO, "Configured size changed: %dx%d -> %dx%d", oldConfiguredSize.Width(), oldConfiguredSize.Height(), m_configuredSize.Width(), m_configuredSize.Height());
  }
  if (changes.bufferScaleChanged)
  {
    CLog::LogF(LOGINFO, "Buffer scale changed: %d -> %d", oldBufferScale, m_scale);
  }

  return changes;
}

std::string CWinSystemWayland::UserFriendlyOutputName(std::shared_ptr<COutput> const& output)
{
  std::vector<std::string> parts;
  if (!output->GetMake().empty())
  {
    parts.emplace_back(output->GetMake());
  }
  if (!output->GetModel().empty())
  {
    parts.emplace_back(output->GetModel());
  }
  if (parts.empty())
  {
    // Fallback to "unknown" if no name received from compositor
    parts.emplace_back(g_localizeStrings.Get(13205));
  }

  // Add position
  auto pos = output->GetPosition();
  if (pos.x != 0 || pos.y != 0)
  {
    parts.emplace_back(StringUtils::Format("@{}x{}", pos.x, pos.y));
  }

  return StringUtils::Join(parts, " ");
}

bool CWinSystemWayland::Minimize()
{
  m_shellSurface->SetMinimized();
  return true;
}

bool CWinSystemWayland::HasCursor()
{
  CSingleLock lock(m_seatProcessorsMutex);
  return std::any_of(m_seatProcessors.cbegin(), m_seatProcessors.cend(),
                     [](decltype(m_seatProcessors)::value_type const& entry)
                     {
                       return entry.second.HasPointerCapability();
                     });
}

void CWinSystemWayland::ShowOSMouse(bool show)
{
  m_osCursorVisible = show;
}

void CWinSystemWayland::LoadDefaultCursor()
{
  if (!m_cursorSurface)
  {
    // Load default cursor theme and default cursor
    // Size of 24px is what most themes seem to have
    m_cursorTheme = wayland::cursor_theme_t("", 24, m_shm);
    wayland::cursor_t cursor;
    try
    {
      cursor = m_cursorTheme.get_cursor("default");
    }
    catch (std::exception const& e)
    {
      CLog::Log(LOGWARNING, "Could not load default cursor from theme, continuing without OS cursor");
    }
    // Just use the first image, do not handle animation
    m_cursorImage = cursor.image(0);
    m_cursorBuffer = m_cursorImage.get_buffer();
    m_cursorSurface = m_compositor.create_surface();
  }
  // Attach buffer to a surface - it seems that the compositor may change
  // the cursor surface when the pointer leaves our surface, so we reattach the
  // buffer each time
  m_cursorSurface.attach(m_cursorBuffer, 0, 0);
  m_cursorSurface.damage(0, 0, m_cursorImage.width(), m_cursorImage.height());
  m_cursorSurface.commit();
}

void CWinSystemWayland::Register(IDispResource* resource)
{
  CSingleLock lock(m_dispResourcesMutex);
  m_dispResources.emplace(resource);
}

void CWinSystemWayland::Unregister(IDispResource* resource)
{
  CSingleLock lock(m_dispResourcesMutex);
  m_dispResources.erase(resource);
}

void CWinSystemWayland::OnSeatAdded(std::uint32_t name, wayland::proxy_t&& proxy)
{
  CSingleLock lock(m_seatProcessorsMutex);

  wayland::seat_t seat(proxy);
  auto newSeatEmplace = m_seatProcessors.emplace(std::piecewise_construct,
                                                 std::forward_as_tuple(name),
                                                 std::forward_as_tuple(name, seat, m_surface, *m_connection, static_cast<IInputHandler&> (*this)));
  newSeatEmplace.first->second.SetCoordinateScale(m_scale);
}

void CWinSystemWayland::OnSeatRemoved(std::uint32_t name)
{
  CSingleLock lock(m_seatProcessorsMutex);

  m_seatProcessors.erase(name);
}

void CWinSystemWayland::OnOutputAdded(std::uint32_t name, wayland::proxy_t&& proxy)
{
  wayland::output_t output(proxy);
  // This is not accessed from multiple threads
  m_outputsInPreparation.emplace(name, std::make_shared<COutput>(name, output, std::bind(&CWinSystemWayland::OnOutputDone, this, name)));
}

void CWinSystemWayland::OnOutputDone(std::uint32_t name)
{
  auto it = m_outputsInPreparation.find(name);
  if (it != m_outputsInPreparation.end())
  {
    // This output was added for the first time - done is also sent when
    // output parameters change later

    {
      CSingleLock lock(m_outputsMutex);
      // Move from m_outputsInPreparation to m_outputs
      m_outputs.emplace(std::move(*it));
      m_outputsInPreparation.erase(it);
    }

    m_protocol.SendOutMessage(WinSystemWaylandProtocol::OUTPUT_HOTPLUG);
  }

  UpdateBufferScale();
}

void CWinSystemWayland::OnOutputRemoved(std::uint32_t name)
{
  m_outputsInPreparation.erase(name);

  CSingleLock lock(m_outputsMutex);
  if (m_outputs.erase(name) != 0)
  {
    // Theoretically, the compositor should automatically put us on another
    // (visible and connected) output if the output we were on is lost,
    // so there is nothing in particular to do here
  }
}

void CWinSystemWayland::SendFocusChange(bool focus)
{
  g_application.m_AppFocused = focus;
  CSingleLock lock(m_dispResourcesMutex);
  for (auto dispResource : m_dispResources)
  {
    dispResource->OnAppFocusChange(focus);
  }
}

void CWinSystemWayland::OnEnter(std::uint32_t seatGlobalName, InputType type)
{
  // Couple to keyboard focus
  if (type == InputType::KEYBOARD)
  {
    SendFocusChange(true);
  }
  if (type == InputType::POINTER)
  {
    CServiceBroker::GetInputManager().SetMouseActive(true);
  }
}

void CWinSystemWayland::OnLeave(std::uint32_t seatGlobalName, InputType type)
{
  // Couple to keyboard focus
  if (type == InputType::KEYBOARD)
  {
    SendFocusChange(false);
  }
  if (type == InputType::POINTER)
  {
    CServiceBroker::GetInputManager().SetMouseActive(false);
  }
}

void CWinSystemWayland::OnEvent(std::uint32_t seatGlobalName, InputType type, XBMC_Event& event)
{
  CWinEvents::MessagePush(&event);
}

void CWinSystemWayland::OnSetCursor(wayland::pointer_t& pointer, std::uint32_t serial)
{
  if (m_osCursorVisible)
  {
    LoadDefaultCursor();
    if (m_cursorSurface) // Cursor loading could have failed
    {
      pointer.set_cursor(serial, m_cursorSurface, m_cursorImage.hotspot_x(), m_cursorImage.hotspot_y());
    }
  }
  else
  {
    pointer.set_cursor(serial, wayland::surface_t{}, 0, 0);
  }
}

void CWinSystemWayland::UpdateBufferScale()
{
  // Adjust our surface size to the output with the biggest scale in order
  // to get the best quality
  auto const maxBufferScaleIt = std::max_element(m_surfaceOutputs.cbegin(), m_surfaceOutputs.cend(), OutputScaleComparer());
  if (maxBufferScaleIt != m_surfaceOutputs.cend())
  {
    WinSystemWaylandProtocol::MsgBufferScale msg{(*maxBufferScaleIt)->GetScale()};
    m_protocol.SendOutMessage(WinSystemWaylandProtocol::BUFFER_SCALE, &msg, sizeof(msg));
  }
}

void CWinSystemWayland::ApplyBufferScale()
{
  CLog::LogF(LOGINFO, "Setting Wayland buffer scale to %d", m_scale);
  m_surface.set_buffer_scale(m_scale);
  m_windowDecorator->SetState(m_configuredSize, m_scale, m_shellSurfaceState);
  CSingleLock lock(m_seatProcessorsMutex);
  for (auto& seatProcessor : m_seatProcessors)
  {
    seatProcessor.second.SetCoordinateScale(m_scale);
  }
}

void CWinSystemWayland::UpdateTouchDpi()
{
  // If we have multiple outputs with wildly different DPI, this is really just
  // guesswork to get a halfway reasonable value. min/max would probably also be OK.
  float dpiSum = std::accumulate(m_surfaceOutputs.cbegin(), m_surfaceOutputs.cend(),  0.0f,
                                 [](float acc, std::shared_ptr<COutput> const& output)
                                 {
                                   return acc + output->GetCurrentDpi();
                                 });
  float dpi = dpiSum / m_surfaceOutputs.size();
  CLog::LogF(LOGDEBUG, "Computed average dpi of %.3f for touch handler", dpi);
  CGenericTouchInputHandler::GetInstance().SetScreenDPI(dpi);
}

CWinSystemWayland::SurfaceSubmission::SurfaceSubmission(timespec const& submissionTime, wayland::presentation_feedback_t const& feedback)
: submissionTime{submissionTime}, feedback{feedback}
{
}

timespec CWinSystemWayland::GetPresentationClockTime()
{
  timespec time;
  if (clock_gettime(m_presentationClock, &time) != 0)
  {
    throw std::system_error(errno, std::generic_category(), "Error getting time from Wayland presentation clock with clock_gettime");
  }
  return time;
}

void CWinSystemWayland::PrepareFramePresentation()
{
  // Continuously measure display latency (i.e. time between when the frame was rendered
  // and when it becomes visible to the user) to correct AV sync
  if (m_presentation)
  {
    auto tStart = GetPresentationClockTime();
    // wp_presentation_feedback creation is coupled to the surface's commit().
    // eglSwapBuffers() (which will be called after this) will call commit().
    // This creates a new Wayland protocol object in the main thread, but this
    // will not result in a race since the corresponding events are never sent
    // before commit() on the surface, which only occurs afterwards.
    auto feedback = m_presentation.feedback(m_surface);
    // Save feedback objects in list so they don't get destroyed upon exit of this function
    // Hand iterator to lambdas so they do not hold a (then circular) reference
    // to the actual object
    decltype(m_surfaceSubmissions)::iterator iter;
    {
      CSingleLock lock(m_surfaceSubmissionsMutex);
      iter = m_surfaceSubmissions.emplace(m_surfaceSubmissions.end(), tStart, feedback);
    }

    feedback.on_sync_output() = [this](wayland::output_t wloutput)
    {
      m_syncOutputID = wloutput.get_id();
      auto output = FindOutputByWaylandOutput(wloutput);
      if (output)
      {
        m_syncOutputRefreshRate = output->GetCurrentMode().GetRefreshInHz();
      }
      else
      {
        CLog::Log(LOGWARNING, "Could not find Wayland output that is supposedly the sync output");
      }
    };
    feedback.on_presented() = [this,iter](std::uint32_t tvSecHi, std::uint32_t tvSecLo, std::uint32_t tvNsec, std::uint32_t refresh, std::uint32_t seqHi, std::uint32_t seqLo, wayland::presentation_feedback_kind flags)
    {
      timespec tv = { .tv_sec = static_cast<std::time_t> ((static_cast<std::uint64_t>(tvSecHi) << 32) + tvSecLo), .tv_nsec = tvNsec };
      std::int64_t latency{KODI::LINUX::TimespecDifference(iter->submissionTime, tv)};
      std::uint64_t msc{(static_cast<std::uint64_t>(seqHi) << 32) + seqLo};
      m_presentationFeedbackHandlers.Invoke(tv, refresh, m_syncOutputID, m_syncOutputRefreshRate, msc);

      iter->latency = latency / 1e9f; // nanoseconds to seconds
      float adjust{};
      {
        CSingleLock lock(m_surfaceSubmissionsMutex);
        if (m_surfaceSubmissions.size() > LATENCY_MOVING_AVERAGE_SIZE)
        {
          adjust = - m_surfaceSubmissions.front().latency / LATENCY_MOVING_AVERAGE_SIZE;
          m_surfaceSubmissions.pop_front();
        }
      }
      m_latencyMovingAverage = m_latencyMovingAverage + iter->latency / LATENCY_MOVING_AVERAGE_SIZE + adjust;

      CLog::Log(LOGDEBUG, LOGAVTIMING, "Presentation feedback: %" PRIi64 " ns -> moving average %f s", latency, static_cast<double> (m_latencyMovingAverage));
    };
    feedback.on_discarded() = [this,iter]()
    {
      CLog::Log(LOGDEBUG, "Presentation: Frame was discarded by compositor");
      CSingleLock lock(m_surfaceSubmissionsMutex);
      m_surfaceSubmissions.erase(iter);
    };
  }
}

void CWinSystemWayland::FinishFramePresentation()
{
  ProcessMessages();

  m_frameStartTime = CurrentHostCounter();
}

float CWinSystemWayland::GetFrameLatencyAdjustment()
{
  std::int64_t now{CurrentHostCounter()};
  return static_cast<float> (now - m_frameStartTime) / CurrentHostFrequency() * 1000.0f;
}

float CWinSystemWayland::GetDisplayLatency()
{
  if (m_presentation)
  {
    return m_latencyMovingAverage * 1000.0f;
  }
  else
  {
    return CWinSystemBase::GetDisplayLatency();
  }
}

float CWinSystemWayland::GetSyncOutputRefreshRate()
{
  return m_syncOutputRefreshRate;
}

KODI::CSignalRegistration CWinSystemWayland::RegisterOnPresentationFeedback(PresentationFeedbackHandler handler)
{
  return m_presentationFeedbackHandlers.Register(handler);
}

std::unique_ptr<CVideoSync> CWinSystemWayland::GetVideoSync(void* clock)
{
  if (m_surface && m_presentation)
  {
    CLog::LogF(LOGINFO, "Using presentation protocol for video sync");
    return std::unique_ptr<CVideoSync>(new CVideoSyncWpPresentation(clock));
  }
  else
  {
    CLog::LogF(LOGINFO, "No supported method for video sync found");
    return nullptr;
  }
}

#if defined(HAVE_LIBVA)
void* CWinSystemWayland::GetVaDisplay()
{
  return vaGetDisplayWl(m_connection->GetDisplay());
}
#endif

std::unique_ptr<IOSScreenSaver> CWinSystemWayland::GetOSScreenSaverImpl()
{
  if (m_surface)
  {
    std::unique_ptr<IOSScreenSaver> ptr;
    ptr.reset(COSScreenSaverIdleInhibitUnstableV1::TryCreate(*m_connection, m_surface));
    if (ptr)
    {
      CLog::LogF(LOGINFO, "Using idle-inhibit-unstable-v1 protocol for screen saver inhibition");
      return ptr;
    }
  }

#if defined(HAVE_DBUS)
  if (KODI::WINDOWING::LINUX::COSScreenSaverFreedesktop::IsAvailable())
  {
    CLog::LogF(LOGINFO, "Using freedesktop.org DBus interface for screen saver inhibition");
    return std::unique_ptr<IOSScreenSaver>(new KODI::WINDOWING::LINUX::COSScreenSaverFreedesktop);
  }
#endif

  CLog::LogF(LOGINFO, "No supported method for screen saver inhibition found");
  return nullptr;
}

std::string CWinSystemWayland::GetClipboardText()
{
  CSingleLock lock(m_seatProcessorsMutex);
  // Get text of first seat with non-empty selection
  // Actually, the value of the seat that received the Ctrl+V keypress should be used,
  // but this would need a workaround or proper multi-seat support in Kodi - it's
  // probably just not that relevant in practice
  for (auto const& seat : m_seatProcessors)
  {
    auto text = seat.second.GetSelectionText();
    if (text != "")
    {
      return text;
    }
  }
  return "";
}

void CWinSystemWayland::OnWindowMove(const wayland::seat_t& seat, std::uint32_t serial)
{
  m_shellSurface->StartMove(seat, serial);
}

void CWinSystemWayland::OnWindowResize(const wayland::seat_t& seat, std::uint32_t serial, wayland::shell_surface_resize edge)
{
  m_shellSurface->StartResize(seat, serial, edge);
}

void CWinSystemWayland::OnWindowShowContextMenu(const wayland::seat_t& seat, std::uint32_t serial, CPointInt position)
{
  m_shellSurface->ShowShellContextMenu(seat, serial, position);
}

void CWinSystemWayland::OnWindowClose()
{
  KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
}

void CWinSystemWayland::OnWindowMinimize()
{
  m_shellSurface->SetMinimized();
}

void CWinSystemWayland::OnWindowMaximize()
{
  if (m_shellSurfaceState.test(IShellSurface::STATE_MAXIMIZED))
  {
    m_shellSurface->UnsetMaximized();
  }
  else
  {
    m_shellSurface->SetMaximized();
  }
}

void CWinSystemWayland::OnClose()
{
  KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
}

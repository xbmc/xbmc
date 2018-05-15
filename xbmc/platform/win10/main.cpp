#include "pch.h"
#include "platform/win10/Win10App.h"

int __stdcall WinMain(HINSTANCE, HINSTANCE, PCSTR, int)
{
  winrt::init_apartment();
  winrt::Windows::ApplicationModel::Core::CoreApplication::Run(KODI::PLATFORM::WINDOWS10::App());
}

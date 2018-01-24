#include "pch.h"
#include "platform/win10/Win10Main.h"

#pragma comment(lib, "libkodi.lib")

// The main function is only used to initialize our IFrameworkView class.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
  auto viewProvider = GetViewProvider();
  Windows::ApplicationModel::Core::CoreApplication::Run(viewProvider);
	return 0;
}

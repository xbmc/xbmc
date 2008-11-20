#ifndef CONFIG_H_
#define CONFIG_H_

#cmakedefine _WIN32
#cmakedefine _LINUX

#ifdef __APPLE__
#define ARCH "osx"
#endif // __APPLE__

#endif // CONFIG_H_


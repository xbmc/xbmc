#ifndef XMOUNT_H
#define XMOUNT_H

#ifdef __cplusplus
extern "C" {
#endif

  long XUnmount(const char* szDrive);
  long XMount(const char* szDrive, char* szDevice);

#ifdef __cplusplus
}
#endif
#endif

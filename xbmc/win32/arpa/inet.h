#pragma once
#include <sys/socket.h>

extern "C" int inet_pton(int af, const char *src, void *dst);

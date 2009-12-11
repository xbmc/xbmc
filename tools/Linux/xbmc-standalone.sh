#!/bin/sh
if which pulse-session; then
  pulse-session xbmc --standalone "$@"
else
  xbmc --standalone "$@"
fi

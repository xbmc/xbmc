#!/bin/bash
###
# Workaround for joystick hotplugging
# Requires
#  - Json over TCP enabled:
#   System - Settings - Services - Remote Control - Allow programs on this system to control Kodi
#  - a udev rule:
#   /etc/udev/rules.d/98-joystick.rules
#   see ${prefix}/share/xbmc/98-joystick.rules-Sample
#  - netcat
###

command -v nc >/dev/null 2>&1 && NC="nc"
command -v netcat >/dev/null 2>&1 && NC="netcat"
[ -z "$NC" ] && echo "nc or netcat not found" && exit 1

ACTION=$1

case "$ACTION" in
  "enable")
    echo '{"jsonrpc":"2.0","method":"Settings.SetSettingValue", "params":{"setting":"input.enablejoystick","value":true},"id":1}' | $NC localhost 9090 >/dev/null && exit 0
    ;;
  "disable")
    echo '{"jsonrpc":"2.0","method":"Settings.SetSettingValue", "params":{"setting":"input.enablejoystick","value":false},"id":1}' | $NC localhost 9090 >/dev/null && exit 0
    ;;
  "reload")
    echo '{"jsonrpc":"2.0","method":"Settings.SetSettingValue", "params":{"setting":"input.enablejoystick","value":false},"id":1}' | $NC localhost 9090 >/dev/null && \
    echo '{"jsonrpc":"2.0","method":"Settings.SetSettingValue", "params":{"setting":"input.enablejoystick","value":true},"id":1}' | $NC localhost 9090 >/dev/null && exit 0
    ;;
  *)
    echo "usage: $0 {enable|disable|reload}"
    exit 2
    ;;
esac

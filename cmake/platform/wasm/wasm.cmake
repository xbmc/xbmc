# Emscripten WebAssembly platform — GLES maps to WebGL2
set(PLATFORM_REQUIRED_DEPS OpenGLES EGL)
set(PLATFORM_OPTIONAL_DEPS_EXCLUDE
    Alsa
    Avahi
    Bluetooth
    Bluray
    CAP
    CEC
    DBus
    Iso9660pp
    LCMS2
    LircClient
    MDNS
    MicroHttpd
    NFS
    Pipewire
    Plist
    PulseAudio
    SmbClient
    Sndio
    UDEV
    Udfread
    XSLT)

set(APP_RENDER_SYSTEM gles)
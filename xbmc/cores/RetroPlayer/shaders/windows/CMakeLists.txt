set(SOURCES RPWinOutputShader.cpp
            ShaderDX.cpp
            ShaderLutDX.cpp
            ShaderPresetDX.cpp
            ShaderSamplerDX.cpp
            ShaderTextureDX.cpp
            ShaderTextureDXRef.cpp
            ShaderUtilsDX.cpp)

set(HEADERS RPWinOutputShader.h
            ShaderDX.h
            ShaderLutDX.h
            ShaderPresetDX.h
            ShaderSamplerDX.h
            ShaderTextureDX.h
            ShaderTextureDXRef.h
            ShaderTypesDX.h
            ShaderUtilsDX.h)

core_add_library(rp-shaders-windows)

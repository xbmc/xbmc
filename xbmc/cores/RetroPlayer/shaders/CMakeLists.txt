set(SOURCES ShaderPreset.cpp
            ShaderPresetFactory.cpp
            ShaderUtils.cpp
)

set(HEADERS IShader.h
            IShaderLut.h
            IShaderPreset.h
            IShaderPresetLoader.h
            IShaderSampler.h
            IShaderTexture.h
            ShaderPreset.h
            ShaderPresetFactory.h
            ShaderTypes.h
            ShaderUtils.h
)

core_add_library(rp-shaders)

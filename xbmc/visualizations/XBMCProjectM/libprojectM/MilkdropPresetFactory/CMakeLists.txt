PROJECT(MilkdropPresetFactory)
cmake_minimum_required(VERSION 2.4.0)

    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
    endif(COMMAND cmake_policy)

SET(MilkdropPresetFactory_SOURCES BuiltinFuncs.cpp Func.cpp MilkdropPreset.cpp Param.hpp PresetFrameIO.cpp CustomShape.cpp  Eval.cpp MilkdropPresetFactory.cpp PerPixelEqn.cpp BuiltinParams.cpp InitCond.cpp Parser.cpp CustomWave.cpp Expr.cpp PerPointEqn.cpp Param.cpp PerFrameEqn.cpp IdlePreset.cpp)

SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

INCLUDE_DIRECTORIES(${projectM_SOURCE_DIR} ${Renderer_SOURCE_DIR})
LINK_DIRECTORIES(${projectM_BINARY_DIR} ${Renderer_BINARY_DIR})

ADD_LIBRARY(MilkdropPresetFactory STATIC ${MilkdropPresetFactory_SOURCES})
SET_TARGET_PROPERTIES(MilkdropPresetFactory PROPERTIES VERSION 2.00 SOVERSION 2)
TARGET_LINK_LIBRARIES(MilkdropPresetFactory Renderer)

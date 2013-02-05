
find_package(SWIG)
find_package(Java)

if(NOT SWIG_FOUND)
  message(FATAL_ERROR "No swig found")
endif()

set(XMLFILES "")

macro(GENERATE_GROOVY_XML)
  foreach(s ${ARGN})
    string(REPLACE ".i" ".xml" sname ${s})
    get_filename_component(sname ${sname} NAME)
    add_custom_command(
      OUTPUT ${sname}
      COMMAND ${SWIG_EXECUTABLE} -w401 -c++ -o ${CMAKE_CURRENT_BINARY_DIR}/${sname} -xml -I${root}/xbmc -xmllang python ${CMAKE_CURRENT_SOURCE_DIR}/${s}
      DEPENDS ${s}
    )
    list(APPEND XMLFILES ${CMAKE_CURRENT_BINARY_DIR}/${sname})
  endforeach()
endmacro()

set(GROOVY_DIR ${root}/lib/groovy)
set(GROOVY_PATHS "${GROOVY_DIR}/groovy-all-1.8.4.jar:${GROOVY_DIR}/commons-lang-2.6.jar:${root}/tools/codegenerator:${root}/xbmc/interfaces/python")
set(GROOVY_GENERATOR ${root}/tools/codegenerator/Generator.groovy)
set(GROOVY_TEMPLATE ${root}/xbmc/interfaces/python/PythonSwig.cpp.template)

macro(GENERATE_GROOVY_CPP)
  foreach(s ${ARGN})
    string(REPLACE ".xml" ".cpp" sname ${s})
    get_filename_component(sname ${sname} NAME)
    add_custom_command(
      OUTPUT ${sname}
      COMMAND ${Java_JAVA_EXECUTABLE} -cp ${GROOVY_PATHS} groovy.ui.GroovyMain ${GROOVY_GENERATOR} ${s} ${GROOVY_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/${sname}
      DEPENDS ${s}
    )
    list(APPEND CPPFILES ${CMAKE_CURRENT_BINARY_DIR}/${sname})
  endforeach()
endmacro()

set(SWIG_SRC 
  ../swig/AddonModuleXbmc.i
  ../swig/AddonModuleXbmcaddon.i
  ../swig/AddonModuleXbmcgui.i
  ../swig/AddonModuleXbmcplugin.i
  ../swig/AddonModuleXbmcvfs.i 
)

GENERATE_GROOVY_XML(${SWIG_SRC})
GENERATE_GROOVY_CPP(${XMLFILES})
add_custom_target(addon_groovy_xml DEPENDS ${XMLFILES})
add_custom_target(addon_groovy_cpp DEPENDS ${CPPFILES})

find_all_sources(. python_srcs)
add_library(addons_python STATIC ${CPPFILES} ${python_srcs})
target_link_libraries(addons_python addons_legacy)

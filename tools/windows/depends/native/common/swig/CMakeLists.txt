cmake_minimum_required(VERSION 3.13)

project(swig VERSION 4.0.1 LANGUAGES C CXX)

set(PACKAGE_BUGREPORT http://www.swig.org)
set(PACKAGE_NAME ${PROJECT_NAME})
set(SWIG_CXX ${CMAKE_CXX_COMPILER})
set(SWIG_LIB ${CMAKE_INSTALL_PREFIX}/share/swig4.0)
set(SWIG_LIB_WIN_UNIX ${SWIG_LIB})
set(SWIG_PLATFORM ${CMAKE_SYSTEM})

configure_file(Source/Include/swigconfig.h.in
	${CMAKE_CURRENT_SOURCE_DIR}/Source/Include/swigconfig.h
)

add_executable(${PROJECT_NAME}
	Source/CParse/cparse.h
	Source/CParse/cscanner.c
	Source/CParse/parser.c
	Source/CParse/parser.h
	Source/CParse/parser.y
	Source/CParse/templ.c
	Source/CParse/util.c
	Source/DOH/base.c
	Source/DOH/doh.h
	Source/DOH/dohint.h
	Source/DOH/file.c
	Source/DOH/fio.c
	Source/DOH/hash.c
	Source/DOH/list.c
	Source/DOH/memory.c
	Source/DOH/string.c
	Source/DOH/void.c
	Source/Doxygen/doxycommands.h
	Source/Doxygen/doxyentity.cxx
	Source/Doxygen/doxyentity.h
	Source/Doxygen/doxyparser.cxx
	Source/Doxygen/doxyparser.h
	Source/Doxygen/doxytranslator.cxx
	Source/Doxygen/doxytranslator.h
	Source/Doxygen/javadoc.cxx
	Source/Doxygen/javadoc.h
	Source/Doxygen/pydoc.cxx
	Source/Doxygen/pydoc.h
	Source/Include/swigconfig.h
	Source/Include/swigconfig.h.in
	Source/Include/swigwarn.h
	Source/Modules/allegrocl.cxx
	Source/Modules/allocate.cxx
	Source/Modules/browser.cxx
	Source/Modules/cffi.cxx
	Source/Modules/chicken.cxx
	Source/Modules/clisp.cxx
	Source/Modules/contract.cxx
	Source/Modules/csharp.cxx
	Source/Modules/d.cxx
	Source/Modules/directors.cxx
	Source/Modules/emit.cxx
	Source/Modules/go.cxx
	Source/Modules/guile.cxx
	Source/Modules/interface.cxx
	Source/Modules/java.cxx
	Source/Modules/javascript.cxx
	Source/Modules/lang.cxx
	Source/Modules/lua.cxx
	Source/Modules/main.cxx
	Source/Modules/modula3.cxx
	Source/Modules/mzscheme.cxx
	Source/Modules/nested.cxx
	Source/Modules/ocaml.cxx
	Source/Modules/octave.cxx
	Source/Modules/overload.cxx
	Source/Modules/perl5.cxx
	Source/Modules/php.cxx
	Source/Modules/pike.cxx
	Source/Modules/python.cxx
	Source/Modules/r.cxx
	Source/Modules/ruby.cxx
	Source/Modules/s-exp.cxx
	Source/Modules/scilab.cxx
	Source/Modules/swigmain.cxx
	Source/Modules/swigmod.h
	Source/Modules/tcl8.cxx
	Source/Modules/typepass.cxx
	Source/Modules/uffi.cxx
	Source/Modules/utils.cxx
	Source/Modules/xml.cxx
	Source/Preprocessor/cpp.c
	Source/Preprocessor/expr.c
	Source/Preprocessor/preprocessor.h
	Source/Swig/cwrap.c
	Source/Swig/deprecate.c
	Source/Swig/error.c
	Source/Swig/extend.c
	Source/Swig/fragment.c
	Source/Swig/getopt.c
	Source/Swig/include.c
	Source/Swig/misc.c
	Source/Swig/naming.c
	Source/Swig/parms.c
	Source/Swig/scanner.c
	Source/Swig/stype.c
	Source/Swig/swig.h
	Source/Swig/swigfile.h
	Source/Swig/swigopt.h
	Source/Swig/swigparm.h
	Source/Swig/swigscan.h
	Source/Swig/swigtree.h
	Source/Swig/swigwrap.h
	Source/Swig/symbol.c
	Source/Swig/tree.c
	Source/Swig/typemap.c
	Source/Swig/typeobj.c
	Source/Swig/typesys.c
	Source/Swig/wrapfunc.c
)

target_include_directories(${PROJECT_NAME}
	PRIVATE
		Source/CParse
		Source/DOH
		Source/Doxygen
		Source/Include
		Source/Modules
		Source/Preprocessor
		Source/Swig
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		PACKAGE_VERSION="${PROJECT_VERSION}"
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
)

install(DIRECTORY Lib/ DESTINATION share/swig4.0)

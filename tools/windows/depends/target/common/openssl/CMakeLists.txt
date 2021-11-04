cmake_minimum_required(VERSION 3.13)

project(openssl VERSION 1.1.1 LANGUAGES C)

find_package(Perl REQUIRED)

include(CheckSymbolExists)
check_symbol_exists(_X86_ "Windows.h" _X86_)
check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)
check_symbol_exists(_ARM_ "Windows.h" _ARM_)

if(_X86_)
	set(OPENSSL_PLATFORM VC-WIN32)
elseif(_AMD64_)
	set(OPENSSL_PLATFORM VC-WIN64A)
elseif(_ARM_)
	set(OPENSSL_PLATFORM VC-WIN32-ARM)
else()
	message(FATAL_ERROR "Unsupported target architecture")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
	set(OPENSSL_PLATFORM ${OPENSSL_PLATFORM}-UWP)
endif()

execute_process(COMMAND ${PERL_EXECUTABLE} Configure ${OPENSSL_PLATFORM} disable-capieng enable-static-engine no-asm no-dso no-shared no-tests no-ui-console --prefix=${CMAKE_INSTALL_PREFIX} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(openssl ALL COMMAND nmake build_generated libcrypto.lib libssl.lib WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(DIRECTORY include/openssl DESTINATION include/ FILES_MATCHING PATTERN "*.h")
install(FILES libcrypto.lib libssl.lib DESTINATION lib/)

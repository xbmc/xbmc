project(msys)

cmake_minimum_required(VERSION 2.8)

set(MINGW_PATH "${CMAKE_INSTALL_PREFIX}/mingw")
set(MSYS_ETC_FSTAB "${MSYS_PATH}/etc/fstab")

# create /etc/fstab
file(WRITE "${MSYS_ETC_FSTAB}" "${MINGW_PATH} /mingw\n")

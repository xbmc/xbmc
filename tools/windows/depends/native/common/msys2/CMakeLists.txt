cmake_minimum_required(VERSION 3.13)

project(msys2)

file(APPEND etc/fstab "${TARBALL_DIR} /var/cache/pacman/pkg\n")

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/usr/bin/gcc.exe ${CMAKE_SOURCE_DIR}/usr/bin/make.exe
	COMMAND bash.exe --noprofile --norc -c "PATH=/usr/bin; pacman-key --init; pacman-key --populate; GNUPGHOME=%24(pacman-conf gpgdir) gpgconf --kill gpg-agent"
	COMMAND bash.exe --noprofile --norc -c "PATH=/usr/bin; pacman-key --init; pacman --noconfirm -Sy gcc make 2>&1 | sed -E 's/error(: failed retrieving file)/err0r\\1/g'; GNUPGHOME=%24(pacman-conf gpgdir) gpgconf --kill gpg-agent"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/usr/bin)

add_custom_target(${PROJECT_NAME} ALL
	DEPENDS ${CMAKE_SOURCE_DIR}/usr/bin/gcc.exe ${CMAKE_SOURCE_DIR}/usr/bin/make.exe)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/
	DESTINATION ${PROJECT_NAME}
	PATTERN "CMakeLists.txt" EXCLUDE)

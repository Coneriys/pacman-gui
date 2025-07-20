cmake_minimum_required(VERSION 3.16)
project(pacman-gui VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# Find GTK4 and GIO
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(GIO REQUIRED gio-2.0)

add_executable(pacman-gui
        src/main.c
        src/pacman_wrapper.c
        src/ui/main_window.c
        src/ui/dependency_viewer.c
)

target_include_directories(pacman-gui PRIVATE
        ${GTK4_INCLUDE_DIRS}
        ${GIO_INCLUDE_DIRS}
        src/
)

target_link_libraries(pacman-gui
        ${GTK4_LIBRARIES}
        ${GIO_LIBRARIES}
)

target_compile_options(pacman-gui PRIVATE
        ${GTK4_CFLAGS_OTHER}
        ${GIO_CFLAGS_OTHER}
)

target_link_directories(pacman-gui PRIVATE
        ${GTK4_LIBRARY_DIRS}
        ${GIO_LIBRARY_DIRS}
)

# Install rules
install(TARGETS pacman-gui DESTINATION bin)
install(FILES pacman-gui.desktop DESTINATION share/applications)

# CPack configuration for source package
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_SOURCE_IGNORE_FILES "/build/;/.git/;/\\..*")
include(CPack)
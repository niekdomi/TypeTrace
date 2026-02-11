find_package(libevdev REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBINPUT_VARS REQUIRED IMPORTED_TARGET libinput)
pkg_check_modules(UDEV_VARS REQUIRED IMPORTED_TARGET libudev)

set(BACKEND_SOURCES main.cpp)

add_executable(typetrace_backend ${BACKEND_SOURCES})

target_link_libraries(
    typetrace_backend
    PRIVATE
        typetrace_common
        libevdev::libevdev
        ${LIBINPUT_VARS_LIBRARIES}
        ${UDEV_VARS_LIBRARIES}
)

target_include_directories(
    typetrace_backend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBINPUT_VARS_INCLUDE_DIRS}
        ${UDEV_VARS_INCLUDE_DIRS}
)

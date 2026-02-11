find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM_VARS REQUIRED IMPORTED_TARGET gtkmm-4.0)

set(FRONTEND_SOURCES
    main.cpp
)

add_executable(typetrace_frontend ${FRONTEND_SOURCES})

target_link_libraries(
    typetrace_frontend
    PRIVATE
        typetrace_common
        ${GTKMM_VARS_LIBRARIES}
)

target_include_directories(
    typetrace_frontend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(
    typetrace_frontend
    SYSTEM
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GTKMM_VARS_INCLUDE_DIRS}
)

target_link_directories(typetrace_frontend PRIVATE ${GTKMM_VARS_LIBRARY_DIRS})

# Frontend executable for TypeTrace

# Find required dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM_VARS REQUIRED IMPORTED_TARGET gtkmm-4.0)

# Blueprint compiler
find_program(BLUEPRINT_COMPILER blueprint-compiler REQUIRED)

# Compile Blueprint file to UI file
set(BLUEPRINT_FILE ui/window.blp)
set(UI_FILE ${CMAKE_CURRENT_BINARY_DIR}/ui/window.ui)

add_custom_command(
    OUTPUT ${UI_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/ui
    COMMAND
        ${BLUEPRINT_COMPILER} compile --output ${UI_FILE}
        ${CMAKE_CURRENT_SOURCE_DIR}/${BLUEPRINT_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${BLUEPRINT_FILE}
    COMMENT "Compiling Blueprint file ${BLUEPRINT_FILE}"
)

# GResource compilation
find_program(GLIB_COMPILE_RESOURCES glib-compile-resources REQUIRED)

set(GRESOURCE_XML ${CMAKE_CURRENT_SOURCE_DIR}/typetrace.gresource.xml)
set(GRESOURCE_C ${CMAKE_CURRENT_BINARY_DIR}/typetrace.gresource.c)

add_custom_command(
    OUTPUT ${GRESOURCE_C}
    COMMAND
        ${GLIB_COMPILE_RESOURCES} --sourcedir=${CMAKE_CURRENT_BINARY_DIR}
        --c-name=typetrace --generate-source --target=${GRESOURCE_C}
        ${GRESOURCE_XML}
    DEPENDS ${UI_FILE} ${GRESOURCE_XML}
    COMMENT "Compiling GResource"
)

# Source files (frontend is header-only except main.cpp)
set(FRONTEND_SOURCES main.cpp ${GRESOURCE_C})

# Mark GResource C file as C language (not C++)
set_source_files_properties(${GRESOURCE_C} PROPERTIES LANGUAGE C)

# Create executable
add_executable(typetrace_frontend ${FRONTEND_SOURCES})

# Frontend-specific compiler flags (GTK headers trigger many warnings)
target_compile_options(typetrace_frontend PRIVATE -Wno-sign-conversion)

# Link libraries
target_link_libraries(typetrace_frontend PRIVATE ${GTKMM_VARS_LIBRARIES})

# Include directories
target_include_directories(
    typetrace_frontend
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    SYSTEM
    PRIVATE ${GTKMM_VARS_INCLUDE_DIRS}
)

# Link directories
target_link_directories(typetrace_frontend PRIVATE ${GTKMM_VARS_LIBRARY_DIRS})

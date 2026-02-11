cmake_minimum_required(VERSION 3.31)

# ==================================================================
# Project Definition
# ==================================================================

project(
    TypeTrace
    VERSION 0.1.0
    DESCRIPTION "The backend for the TypeTrace project"
    LANGUAGES
        C
        CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)

# ==================================================================
# Compiler Flags
# ==================================================================

# Release optimizations
add_compile_options(
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-flto=full>
)

# Release link-time optimizations
add_link_options($<$<CONFIG:Release>:-flto=full>)

# Warning flags
add_compile_options(
    # Core
    -WCL4
    -Werror
    -Wpedantic
    -Wdeprecated
    # Safety
    -Wnon-virtual-dtor
    -Wsuggest-override
    # -Wunsafe-buffer-usage # Not compatible with GTKmm
    -Wexperimental-lifetime-safety
    -Wconsumed
    -Wthread-safety
    -Wformat=2
    # Type Safety & Performance
    -Wconversion
    -Wcast-align
    # -Wold-style-cast # Not compatible with GTKmm
    -Wfloat-equal
    # -Wdouble-promotion # Not compatible with GTKmm
    # -Wcast-function-type # Not compatible with GTKmm
    # -Wzero-as-null-pointer-constant # Not compatible with GTKmm
    # Logical & Control Flow
    -Wimplicit-fallthrough
    -Wshadow-all
    -Wunreachable-code-aggressive
    -Wloop-analysis
    -Wcovered-switch-default
    -Wno-gnu-statement-expression
)

# ==================================================================
# Installation & Build Info
# ==================================================================
message(STATUS "TypeTrace Backend:")
message(STATUS "  Version: ${PROJECT_VERSION} (${GIT_COMMIT})")
message(STATUS "  Build: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Standard: C++${CMAKE_CXX_STANDARD}")
message(
    STATUS
    "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "  Cache:    ccache (${CCACHE_PROGRAM})")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

find_program(MOLD_LINKER mold)
if(MOLD_LINKER)
    message(STATUS "  Linker:   mold (${MOLD_LINKER})")
    add_link_options(-fuse-ld=mold)
else()
    message(STATUS "  Linker:   default")
endif()

# ------------------------------------------------------------------
# Version Header Generation
# ------------------------------------------------------------------
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
configure_file(
    ${CMAKE_SOURCE_DIR}/typetrace/common/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version.hpp
    @ONLY
)

# ==================================================================
# Subdirectories
# ==================================================================
add_subdirectory(typetrace/common)
add_subdirectory(typetrace/backend)
add_subdirectory(typetrace/frontend)

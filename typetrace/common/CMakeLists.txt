find_package(spdlog CONFIG REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)

add_library(typetrace_common INTERFACE)

target_link_libraries(
    typetrace_common
    INTERFACE
        spdlog::spdlog
        SQLiteCpp
)

target_include_directories(
    typetrace_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/generated
)

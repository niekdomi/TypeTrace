#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace typetrace {

/// Error categories for TypeTrace operations
enum class ErrorCode : std::uint8_t
{
    SYSTEM,      ///< System-level errors (udev, libinput, etc.)
    DATABASE,    ///< Database operation errors
    PERMISSION,  ///< Permission and access errors
    ENVIRONMENT, ///< Environment variable or configuration errors
};

/// Error information structure
struct Error
{
    ErrorCode code;           ///< Error category
    std::string_view message; ///< Detailed error message

    Error(ErrorCode error_code, std::string_view error_message)
        : code(error_code),
          message(error_message)
    {}
};

// TODO (domi): Should we keep those types or just use the same error type for all?

/// Create a system error
[[nodiscard]]
inline auto make_system_error(std::string_view message) -> Error
{
    return Error{ErrorCode::SYSTEM, message};
}

/// Create a database error
[[nodiscard]]
inline auto make_database_error(std::string_view message) -> Error
{
    return Error{ErrorCode::DATABASE, message};
}

/// Create a permission error
[[nodiscard]]
inline auto make_permission_error(std::string_view message) -> Error
{
    return Error{ErrorCode::PERMISSION, message};
}

/// Create an environment error
[[nodiscard]]
inline auto make_environment_error(std::string_view message) -> Error
{
    return Error{ErrorCode::ENVIRONMENT, message};
}

} // namespace typetrace

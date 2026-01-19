#pragma once

#include <cstdint>
#include <string_view>

namespace typetrace {

/// Structure representing a keystroke event
struct KeystrokeEvent
{
    std::string_view key_name; ///< Human-readable name of the key
    std::string_view date;     ///< Date in YYYY-MM-DD format
    std::uint32_t key_code{};  ///< Code of the pressed key
};

static_assert(sizeof(KeystrokeEvent) == 40);

} // namespace typetrace

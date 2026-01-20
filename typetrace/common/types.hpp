#pragma once

#include <cstdint>
#include <string_view>

namespace typetrace {

struct KeystrokeEvent
{
    std::uint32_t key_code;
    std::string_view key_name;
    std::string_view date;
    int count{0}; // Added count field
};

static_assert(sizeof(KeystrokeEvent) <= 48, "KeystrokeEvent is too large");

} // namespace typetrace

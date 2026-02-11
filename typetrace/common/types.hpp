#pragma once

#include <cstdint>
#include <string_view>

namespace typetrace::common {

struct KeystrokeEvent
{
    std::string_view key_name;
    std::string_view date;
    std::uint32_t key_code;
    unsigned int count{0};
};

static_assert(sizeof(KeystrokeEvent) == 40);

} // namespace typetrace::common

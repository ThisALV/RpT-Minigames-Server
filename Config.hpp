#ifndef RPTOGETHER_SERVER_CONFIG_HPP
#define RPTOGETHER_SERVER_CONFIG_HPP

#include <string_view>

#define RPT_RUNTIME_PLATFORM @RPT_TARGET_PLATFORM@ // Necessary for condition includes

namespace RpT::Config {

enum struct Platform {
    Unix, Win32
};

constexpr std::size_t VERSION_MAJOR { @PROJECT_VERSION_MAJOR@ };
constexpr std::size_t VERSION_MINOR { @PROJECT_VERSION_MINOR@ };
constexpr std::size_t VERSION_PATCH { @PROJECT_VERSION_PATCH@ };

constexpr std::string_view VERSION { "@PROJECT_VERSION@" };

constexpr Platform RUNTIME_PLATFORM { Platform::RPT_RUNTIME_PLATFORM };

constexpr std::string_view runtimePlatformName() {
    if constexpr (RUNTIME_PLATFORM == Platform::Unix) {
        return "Unix";
    } else {
        return "Win32";
    }
}

}

#endif // RPTOGETHER_SERVER_CONFIG_HPP

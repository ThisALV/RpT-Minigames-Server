#ifndef RPTOGETHER_SERVER_CONFIG_HPP
#define RPTOGETHER_SERVER_CONFIG_HPP

#include <string_view>


/**
 * @file Config.hpp
 *
 * @brief Configuration header for compile-time constants
 */


/**
 * @brief Platform of build target
 *
 * Available as a macro to allow conditional includes.
 * It's recommended to use RpT::Config::RUNTIME_PLATFORM if it isn't necessary.
 *
 * @see RpT::Config::Platform
 */
#define RPT_RUNTIME_PLATFORM @RPT_TARGET_PLATFORM@ // Necessary for conditional includes


/**
 * @brief %Config constants
 */
namespace RpT::Config {


/**
 * @brief Platforms that can be used as build target
 */
enum struct Platform {
    Unix, Win32
};


/**
 * @brief Major version number
 *
 * Patch number differences will not break existing games of the same minor version.
 */
constexpr std::size_t VERSION_MAJOR { @PROJECT_VERSION_MAJOR@ };

/**
 * @brief Minor version number
 *
 * Minor number, from 1.0, will not break existing games of the same major version.
 * However, under 1.0, this guarantee doesn't apply.
 */
constexpr std::size_t VERSION_MINOR { @PROJECT_VERSION_MINOR@ };

/**
 * @brief Patch version number
 *
 * Major number might break existing games.
 */
constexpr std::size_t VERSION_PATCH { @PROJECT_VERSION_PATCH@ };

/**
 * @brief Version string formatted as MAJOR.MINOR.PATCH
 */
constexpr std::string_view VERSION { "@PROJECT_VERSION@" };

/**
 * @brief Platform build target will be running on
 */
constexpr Platform RUNTIME_PLATFORM { Platform::RPT_RUNTIME_PLATFORM };

/**
 * @brief Returns if build target will be running on Unix
 *
 * @returns If target will run on Unix platform
 */
constexpr bool isUnixBuild() {
    return RUNTIME_PLATFORM == Platform::Unix;
}

/**
 * @brief Returns if build target will be running on Win32
 *
 * @returns If target will run on Win32 platform
 */
constexpr bool isWin32Build() {
    return RUNTIME_PLATFORM == Platform::Win32;
}


/**
 * @brief Get a string representation for RUNTIME_PLATFORM
 *
 * @returns "Unix" for Platform::Unix or "Win32" for Platform::Win32
 */
constexpr std::string_view runtimePlatformName() {
    if constexpr (isUnixBuild()) {
        return "Unix";
    } else {
        return "Win32";
    }
}

}

#endif // RPTOGETHER_SERVER_CONFIG_HPP

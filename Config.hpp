#ifndef RPTOGETHER_SERVER_CONFIG_HPP
#define RPTOGETHER_SERVER_CONFIG_HPP

#include <string_view>

namespace RpT {
namespace Config {

constexpr std::size_t VERION_MAJOR { @PROJECT_VERSION_MAJOR@ };
constexpr std::size_t VERSION_MINOR { @PROJECT_VERSION_MINOR@ };
constexpr std::size_t VERSION_PATCH { @PROJECT_VERSION_PATCH@ };

constexpr std::string_view VERSION { "@PROJECT_VERSION@" };

}
}

#endif // RPTOGETHER_SERVER_CONFIG_HPP

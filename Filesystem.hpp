#ifndef RPTOGETHER_SERVER_FILESYSTEM_HPP
#define RPTOGETHER_SERVER_FILESYSTEM_HPP

/**
 * @file Filesystem.hpp
 *
 * @brief Configuration header required to use supported filesystem library by current build compiler
 */


/// Experimental filesystem support
#define RPT_FILESYSTEM_EXPERIMENTAL 1
/// Standard filesystem support
#define RPT_FILESYSTEM_STANDARD 2

/**
 * @brief <filesystem> library supported by current build compiler
 *
 * Filesystem library support status, must be RPT_FILESYSTEM_EXPERIMENTAL (0) or RPT_FILESYSTEM_STANDARD (1).
 *
 * Available as a macro to allow conditional includes and using directives.
 */
#define RPT_FILESYSTEM_SUPPORT @RPT_FILESYSTEM_SUPPORT@


// Includes correct header and set namespace alias to appropriate filesystem namespace

#if RPT_FILESYSTEM_SUPPORT == RPT_FILESYSTEM_EXPERIMENTAL

#include <experimental/filesystem>
namespace supported_fs = std::experimental::filesystem;

#elif RPT_FILESYSTEM_SUPPORT == RPT_FILESYSTEM_STANDARD

#include <filesystem>
namespace supported_fs = std::filesystem;

#endif

#endif //RPTOGETHER_SERVER_FILESYSTEM_HPP

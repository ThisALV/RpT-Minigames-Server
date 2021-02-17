#ifndef RPTOGETHER_SERVER_EXECUTOR_HPP
#define RPTOGETHER_SERVER_EXECUTOR_HPP

#include <vector>
#include <RpT-Config/Filesystem.hpp>
#include <RpT-Core/LoggerView.hpp>

/**
 * @file Executor.hpp
 */


namespace RpT::Core {

/**
 * @brief RpT main loop executor
 *
 * Run main loop for RpT.
 *
 * @author ThisALV, https://github.com/ThisALV
 */
class Executor {
private:
    LoggerView logger_;

public:
    /**
     * @brief Construct executor with user-defined resources path
     *
     * @param game_resources_path A list of paths the game loader will search for resources on
     * @param game_name Name of game to play during this executor run, can be modified by players later
     */
    Executor(std::vector<supported_fs::path> game_resources_path, std::string game_name);

    // Entity class semantic :

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    bool operator==(const Executor&) const = delete;

    /**
     * @brief Start executor main loop
     *
     * Main loop will be ran until SIGINT, SIGTERM or SIGHUP is fired, or until an error occurres.
     *
     * @note For Win32 runtime platform, CTRL console events will be handled to emulate signals.
     *
     * @return `true` if properly shutdown, `false` if an error occurred
     */
    bool run();
};

}

#endif //RPTOGETHER_SERVER_EXECUTOR_HPP

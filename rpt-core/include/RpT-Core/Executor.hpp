#ifndef RPTOGETHER_SERVER_EXECUTOR_HPP
#define RPTOGETHER_SERVER_EXECUTOR_HPP

#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <RpT-Core/InputOutputInterface.hpp>
#include <RpT-Core/ServiceEventRequestProtocol.hpp>
#include <RpT-Utils/LoggerView.hpp>

/**
 * @file Executor.hpp
 */


/**
 * @brief Server engine and main loop related code
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
    Utils::LoggerView logger_;
    InputOutputInterface& io_interface_;

public:
    /**
     * @brief Construct executor with user-defined resources path and IO interface
     *
     * @param game_resources_path A list of paths the game loader will search for resources on
     * @param game_name Name of game to play during this executor run, can be modified by players later
     * @param io_interface Backend for input and output based main loop events handling
     */
    Executor(std::vector<boost::filesystem::path> game_resources_path, std::string game_name,
             InputOutputInterface& io_interface, Utils::LoggingContext& logger_context);

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

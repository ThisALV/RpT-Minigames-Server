#include <cstdlib>
#include <iostream>
#include <RpT-Config/Config.hpp>
#include <RpT-Core/Executor.hpp>
#include <RpT-Core/CommandLineOptionsParser.hpp>


constexpr int SUCCESS { 0 };
constexpr int INVALID_ARGS { 1 };
constexpr int RUNTIME_ERROR { 2 };


int main(const int argc, const char** argv) {
    try {
        // Read and parse command line options
        const RpT::Core::CommandLineOptionsParser cmd_line_options { argc, argv, { "game" } };

        // Get game name from command line options
        const std::string_view game_name { cmd_line_options.get("game") };

        std::cout << "Running RpT server " << RpT::Config::VERSION
                  << " on " << RpT::Config::runtimePlatformName() << "." << std::endl;

        std::vector<std::filesystem::path> game_resources_path;
        // At max 3 paths : next to server executable, into user directory and into /usr/share for Unix platform
        game_resources_path.reserve(3);

        std::filesystem::path local_path { ".rpt-server" };
        std::filesystem::path user_path; // Platform dependent

        // Get user home directory depending of the current runtime platform
        if constexpr (RpT::Config::isUnixBuild()) {
            user_path = std::getenv("HOME");
        } else {
            user_path = std::getenv("UserProfile");
        }

        user_path /= ".rpt-server"; // Search for .rpt-server hidden subdirectory inside user directory

        if constexpr (RpT::Config::isUnixBuild()) { // Unix systems may have /usr/share directory for programs data
            std::filesystem::path system_path { "/usr/share/rpt-server" };

            game_resources_path.push_back(std::move(system_path)); // Add Unix /usr/share directory
        }

        // Add path for subdirectory inside working directory and inside user directory
        game_resources_path.push_back(std::move(user_path));
        game_resources_path.push_back(std::move(local_path));

        // Create executor with listed resources paths, game name argument and run main loop
        RpT::Core::Executor rpt_executor { std::move(game_resources_path), std::string { game_name } };
        const bool done_successfully { rpt_executor.run() };

        return done_successfully ? SUCCESS : RUNTIME_ERROR; // Process exit code depends on main loop result
    } catch (const RpT::Server::OptionsError& err) {
        std::cerr << "Command line error: " << err.what() << std::endl;
        return INVALID_ARGS;
    }
}

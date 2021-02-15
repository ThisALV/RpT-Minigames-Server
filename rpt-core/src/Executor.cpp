#include <RpT-Core/Executor.hpp>

#include <iostream>

namespace RpT::Core {

Executor::Executor(std::vector<supported_fs::path> game_resources_path, std::string game_name) :
    logger_ { "Executor" } {

    logger_.debug("Game name: {}", game_name);

    for (const supported_fs::path& resource_path : game_resources_path)
        logger_.debug("Game resources path: {}", resource_path.string());
}

bool Executor::run() {
    logger_.info("Start.");

    return true;
}

}

#include <iostream>
#include <RpT-Config/Config.hpp>

int main(const int argc, const char* argv[]) {
    std::cout << "Running RpT server " << RpT::Config::VERSION
              << " on " << RpT::Config::runtimePlatformName() << "." << std::endl;

    return 0;
}

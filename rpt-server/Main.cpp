#include <iostream>
#include <Rpt-Config/Config.hpp>

#if RPT_RUNTIME_PLATFORM == Unix
#define RPT_RUNTIME_PLATFORM_NAME "Unix"
#else
#define RPT_RUNTIME_PLATFORM_NAME "Win32"
#endif

int main(const int argc, const char* argv[]) {
    std::cout << "Running RpT server " RPT_VERSION " on " RPT_RUNTIME_PLATFORM_NAME "." << std::endl;

    return 0;
}

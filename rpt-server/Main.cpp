#include <iostream>
#include <Rpt-Config/Config.hpp>

#define str(s) #s
#define xstr(s) str(s)
#define RPT_RUNTIME_PLATFORM_NAME xstr(RPT_RUNTIME_PLATFORM)

int main(const int argc, const char* argv[]) {
    std::cout << "Running RpT server " RPT_VERSION " on " RPT_RUNTIME_PLATFORM_NAME "." << std::endl;

    return 0;
}

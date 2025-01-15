#include "Boza/App.hpp"
#include "Boza/Logger.hpp"

int main()
{
    boza::App app{ "Test App" };
    if (!app) boza::Logger::error("Failed to initialize App");

    app.run();
}

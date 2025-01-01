#include "App.hpp"

#include "CommandPool.hpp"
#include "Device.hpp"
#include "Instance.hpp"
#include "Logger.hpp"

namespace boza
{
    App::App(const std::string_view& name)
    {
        Logger::create();

        if (![&]
        {
            BOZA_CHECK(Instance::create(name), "Failed to create instance");
            BOZA_CHECK(Device::create(), "Failed to create device");
            BOZA_CHECK(CommandPool::create(), "Failed to create command pool");
            return true;
        }())
        {
            hasThrown = true;
            Logger::error("Failed to initialize the application");
        }
    }

    App::~App()
    {
        CommandPool::destroy();
        Device::destroy();
        Instance::destroy();
        Logger::destroy();
    }
}

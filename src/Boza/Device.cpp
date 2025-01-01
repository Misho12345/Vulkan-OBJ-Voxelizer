#include "pch.hpp"
#include "Device.hpp"

#include "Logger.hpp"
#include "Instance.hpp"

namespace boza
{
    static bool created = false;

    bool Device::create()
    {
        assert(!created);
        Logger::trace("Creating device");
        created = true;

        BOZA_CHECK(choose_physical_device(), "Failed to create physical device");
        BOZA_CHECK(create_logical_device(), "Failed to create logical device");

        vk::defaultDispatchLoaderDynamic =
        {
            Instance::get_vk_instance(), vkGetInstanceProcAddr,
            logical_device, vkGetDeviceProcAddr
        };

        compute_queue =  logical_device.getQueue(compute_queue_family_index, 0);

        return true;
    }

    void Device::destroy()
    {
        assert(created);
        Logger::trace("Destroying logical device");
        created = false;

        logical_device.destroy();
    }


    bool Device::choose_physical_device()
    {
        auto [result, physical_devices] = Instance::get_vk_instance().enumeratePhysicalDevices();
        VK_CHECK(result, "Failed to enumerate physical devices");

        #ifndef NDEBUG
        std::string devices_str = "Physical devices:";
        for (const auto& device : physical_devices)
        {
            vk::PhysicalDeviceProperties properties = device.getProperties();
            devices_str += "\n\t (" + to_string(properties.deviceType) + ") " + std::string(
                properties.deviceName.data());
        }
        Logger::trace(devices_str);
        #endif

        for (const auto& device : physical_devices)
        {
            bool       suitable   = false;
            const auto properties = device.getQueueFamilyProperties();
            for (uint32_t i = 0; i < properties.size(); i++)
            {
                if (properties[i].queueFlags & vk::QueueFlagBits::eCompute)
                {
                    suitable                   = true;
                    compute_queue_family_index = i;
                }
            }

            #ifndef NDEBUG
            std::string properties_str = "Queue family properties for " + std::string(
                device.getProperties().deviceName.data()) + ":";
            for (const auto& prop : properties)
                properties_str += "\n\t" + to_string(prop.queueFlags);
            Logger::trace(properties_str);
            #endif


            if (suitable)
            {
                physical_device = device;
                Logger::trace("A suitable device is chosen - {}", device.getProperties().deviceName.data());
                Logger::trace("Max compute work group invocations - {}", device.getProperties().limits.maxComputeWorkGroupInvocations);
                return true;
            }
        }

        return false;
    }

    bool Device::create_logical_device()
    {
        constexpr float queue_priority = 1.0f;

        vk::DeviceQueueCreateInfo queue_create_info
        {
            {},
            compute_queue_family_index,
            1, &queue_priority
        };

        vk::PhysicalDeviceFeatures device_features = physical_device.getFeatures();

        const vk::DeviceCreateInfo device_info
        {
            {},
            1, &queue_create_info,
            0, nullptr,
            0, nullptr,
            &device_features
        };

        vk::Result result;
        std::tie(result, logical_device) = physical_device.createDevice(device_info);
        VK_CHECK(result, "Failed to create logical device");

        Logger::trace("Logical device created");
        return true;
    }
}

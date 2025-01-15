    #include "Device.hpp"

    #include "Logger.hpp"

    namespace boza
    {
        Device::Device(const Instance& instance) : ok{ true }
        {
            Logger::trace("Creating device");

            if (!choose_physical_device(instance)) return;
            if (!create_logical_device()) return;

            vk::defaultDispatchLoaderDynamic =
            {
                instance.get(), vkGetInstanceProcAddr,
                *logical_device, vkGetDeviceProcAddr
            };

            compute_queue = logical_device->getQueue(compute_queue_family_index, 0);
        }

        Device::~Device()
        {
            if (ok) Logger::trace("Destroying logical device");
        }


        Device::Device(Device&& other) noexcept
        {
            logical_device             = std::move(other.logical_device);
            physical_device            = std::move(other.physical_device);
            compute_queue              = std::move(other.compute_queue);
            compute_queue_family_index = std::exchange(other.compute_queue_family_index, 0);
            ok                         = std::exchange(other.ok, false);

            if (logical_device) vk::defaultDispatchLoaderDynamic.init(*logical_device);
        }

        Device& Device::operator=(Device&& other) noexcept
        {
            if (this != &other)
            {
                logical_device             = std::move(other.logical_device);
                physical_device            = std::move(other.physical_device);
                compute_queue              = std::move(other.compute_queue);
                compute_queue_family_index = std::exchange(other.compute_queue_family_index, 0);
                ok                         = std::exchange(other.ok, false);
            }

            if (logical_device) vk::defaultDispatchLoaderDynamic.init(*logical_device);

            return *this;
        }


        bool Device::choose_physical_device(const Instance& instance)
        {
            auto [result, physical_devices] = instance.get().enumeratePhysicalDevices();
            if (result != vk::Result::eSuccess)
            {
                Logger::error("Failed to enumerate physical devices");
                ok = false;
                return false;
            }

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
                for (uint32_t i = 0; i < properties.size(); ++i)
                {
                    if (properties[i].queueFlags & vk::QueueFlagBits::eCompute)
                    {
                        suitable = true;
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

            Logger::error("No suitable physical device found");
            ok = false;
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

            auto [result, _logical_device] = physical_device.createDeviceUnique(device_info);
            if (result != vk::Result::eSuccess)
            {
                Logger::error("Failed to create logical device");
                ok = false;
                return false;
            }

            logical_device = std::move(_logical_device);
            return true;
        }
    }

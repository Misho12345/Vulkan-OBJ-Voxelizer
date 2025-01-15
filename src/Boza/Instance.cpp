#include "Instance.hpp"
#include "Logger.hpp"

namespace vk
{
    constexpr auto LayerKhronosValidationName = "VK_LAYER_KHRONOS_validation";
    VULKAN_HPP_STORAGE_API DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

namespace boza
{
    Instance::Instance(const std::string_view& app_name) : ok{ true }
    {
        vk::defaultDispatchLoaderDynamic = { vkGetInstanceProcAddr };
        if (!create_vk_instance(app_name)) return;

        vk::defaultDispatchLoaderDynamic = { *instance, vkGetInstanceProcAddr };
        DEBUG_ONLY(if (!create_debug_messenger()) return);
    }

    Instance::~Instance()
    {
        if (ok) Logger::trace("Destroying Vulkan instance");
    }


    Instance::Instance(Instance&& other) noexcept
    {
        instance = std::move(other.instance);
        ok = std::exchange(other.ok, false);
    }

    Instance& Instance::operator=(Instance&& other) noexcept
    {
        if (this != &other)
        {
            instance = std::move(other.instance);
            ok = std::exchange(other.ok, false);
        }

        return *this;
    }


    bool Instance::create_vk_instance(const std::string_view& app_name)
    {
        auto [version_res, version] = vk::enumerateInstanceVersion();
        if (version_res != vk::Result::eSuccess)
        {
            Logger::error("Failed to get Vulkan instance version");
            ok = false;
            return false;
        }

        Logger::info("System can support Vulkan version {}.{}.{}",
                     vk::apiVersionMajor(version),
                     vk::apiVersionMinor(version),
                     vk::apiVersionPatch(version));

        vk::ApplicationInfo app_info
        {
            app_name.data(),
            vk::makeApiVersion(0, 0, 1, 0),
            "Boza",
            vk::makeApiVersion(0, 0, 1, 0),
            version
        };


        std::vector<const char*> enabled_layers;
        std::vector<const char*> enabled_extensions;

        DEBUG_ONLY(enabled_layers.push_back(vk::LayerKhronosValidationName));
        DEBUG_ONLY(enabled_extensions.push_back(vk::EXTDebugUtilsExtensionName));

        if (!check_extensions_and_layers_support(enabled_extensions, enabled_layers)) return false;

        const vk::InstanceCreateInfo create_info
        {
            {},
            &app_info,
            static_cast<uint32_t>(enabled_layers.size()), enabled_layers.data(),
            static_cast<uint32_t>(enabled_extensions.size()), enabled_extensions.data()
        };

        auto [instance_result, _instance] = createInstanceUnique(create_info);
        if (instance_result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create Vulkan instance");
            ok = false;
            return false;
        }

        instance = std::move(_instance);
        Logger::trace("Vulkan instance created");
        return true;
    }

    bool Instance::check_extensions_and_layers_support(
        const std::span<const char*>& extensions,
        const std::span<const char*>& layers)
    {
        if (extensions.empty() && layers.empty()) return true;

        vk::Result                           result;
        std::vector<vk::ExtensionProperties> supported_extensions;
        std::vector<vk::LayerProperties>     supported_layers;

        std::tie(result, supported_extensions) = vk::enumerateInstanceExtensionProperties();
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to enumerate instance extensions");
            ok = false;
            return false;
        }

        std::tie(result, supported_layers) = vk::enumerateInstanceLayerProperties();
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to enumerate instance layers");
            ok = false;
            return false;
        }

        #ifndef NDEBUG
        std::string required_extensions_str = "Required extensions:";
        for (const auto& extension : extensions)
            required_extensions_str += "\n\t" + std::string(extension);
        Logger::trace(required_extensions_str);
        #endif

        for (const auto& extension : extensions)
        {
            bool found = false;
            for (const auto& supported_extension : supported_extensions)
            {
                if (strcmp(extension, supported_extension.extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                Logger::error("Extension {} is not supported", extension);
                ok = false;
                return false;
            }
        }

        #ifndef NDEBUG
        std::string required_layers_str = "Required layers:";
        for (const auto& layer : layers)
            required_layers_str += "\n\t" + std::string(layer);
        Logger::trace(required_layers_str);
        #endif

        for (const auto& layer : layers)
        {
            bool found = false;
            for (const auto& supported_layer : supported_layers)
            {
                if (strcmp(layer, supported_layer.layerName) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                if (layer == vk::LayerKhronosValidationName)
                    Logger::trace("If you use Debug mode, make sure to have the Vulkan SDK installed on your computer");

                ok = false;
                return false;
            }
        }

        return true;
    }


    #ifndef NDEBUG
    bool Instance::create_debug_messenger()
    {
        using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

        constexpr vk::DebugUtilsMessengerCreateInfoEXT create_info
        {
            {},

            /*eVerbose | eInfo |*/ eWarning | eError,
            eGeneral | eValidation | ePerformance,

            [](
                const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                const VkDebugUtilsMessageTypeFlagsEXT        message_type,
                const VkDebugUtilsMessengerCallbackDataEXT*  callback_data,
                void*                                        user_data[[maybe_unused]]) -> VkBool32
            {
                const auto cpp_message_type = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_type);

                const std::string message = std::format("Validation layer {}: {}",
                    to_string(cpp_message_type), callback_data->pMessage);

                using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
                switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity))
                {
                case eVerbose: Logger::trace(message); break;
                case eInfo:    Logger::info (message); break;
                case eWarning: Logger::warn (message); break;
                case eError:   Logger::error(message); break;
                }

                return vk::False;
            },

            nullptr
        };

        auto [result, _debug_messenger] = instance->createDebugUtilsMessengerEXTUnique(create_info);
        if (result != vk::Result::eSuccess)
        {
            Logger::error("Failed to create debug messenger");
            ok = false;
            return false;
        }

        debug_messenger = std::move(_debug_messenger);
        Logger::trace("Debug messenger created");
        return true;
    }
    #endif
}

#include "pch.hpp"
#include "Instance.hpp"

#include "Logger.hpp"

namespace vk
{
    constexpr auto LayerKhronosValidationName = "VK_LAYER_KHRONOS_validation";
    VULKAN_HPP_STORAGE_API DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

namespace boza
{
    static bool created = false;

    bool Instance::create(const std::string_view& app_name)
    {
        assert(!created);
        Logger::trace("Creating Vulkan instance");
        created = true;

        vk::defaultDispatchLoaderDynamic = { vkGetInstanceProcAddr };
        BOZA_CHECK(create_vk_instance(app_name), "Failed to create Vulkan instance");

        vk::defaultDispatchLoaderDynamic = { vk_instance, vkGetInstanceProcAddr };
        DEBUG_ONLY(BOZA_CHECK(create_debug_messenger(), "Failed to create debug messenger"));

        return true;
    }

    void Instance::destroy()
    {
        assert(created);
        Logger::trace("Destroying Vulkan instance");
        created = false;

        DEBUG_ONLY(vk_instance.destroyDebugUtilsMessengerEXT(debug_messenger));
        vk_instance.destroy();
    }


    bool Instance::create_vk_instance(const std::string_view& app_name)
    {
        auto [result, version] = vk::enumerateInstanceVersion();
        VK_CHECK(result, "Failed to get Vulkan instance version");

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

        #ifndef NDEBUG
        enabled_layers.push_back(vk::LayerKhronosValidationName);
        enabled_extensions.push_back(vk::EXTDebugUtilsExtensionName);
        #endif

        BOZA_CHECK(check_extensions_and_layers_support(enabled_extensions, enabled_layers),
                   "Failed to find required extensions or layers");

        const vk::InstanceCreateInfo create_info
        {
            {},
            &app_info,
            static_cast<uint32_t>(enabled_layers.size()), enabled_layers.data(),
            static_cast<uint32_t>(enabled_extensions.size()), enabled_extensions.data()
        };

        std::tie(result, vk_instance) = createInstance(create_info);
        VK_CHECK(result, "Failed to create Vulkan instance");

        Logger::trace("Vulkan instance created");
        return true;
    }


    #ifndef NDEBUG
    VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
        const VkDebugUtilsMessageTypeFlagsEXT       message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void*)
    {
        const auto cpp_message_type = static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(message_type);

        using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        switch (static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(message_severity))
        {
        case eVerbose: Logger::trace("Validation layer {}: {}", to_string(cpp_message_type), callback_data->pMessage); break;
        case eInfo: Logger::info("Validation layer {}: {}", to_string(cpp_message_type), callback_data->pMessage); break;
        case eWarning: Logger::warn("Validation layer {}: {}", to_string(cpp_message_type), callback_data->pMessage); break;
        case eError: Logger::error("Validation layer {}: {}", to_string(cpp_message_type), callback_data->pMessage); break;
        }

        return vk::False;
    }

    bool Instance::create_debug_messenger()
    {
        using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

        constexpr vk::DebugUtilsMessengerCreateInfoEXT create_info
        {
            {},

            /*eVerbose | eInfo |*/ eWarning | eError,
            eGeneral | eValidation | ePerformance,

            debug_callback,
            nullptr
        };

        vk::Result result;
        std::tie(result, debug_messenger) = vk_instance.createDebugUtilsMessengerEXT(create_info);
        VK_CHECK(result, "Failed to create debug messenger");

        Logger::trace("Debug messenger created");
        return true;
    }
    #endif

    bool Instance::check_extensions_and_layers_support(
        const std::span<const char*>& extensions,
        const std::span<const char*>& layers)
    {
        if (extensions.empty() && layers.empty()) return true;

        vk::Result                           result;
        std::vector<vk::ExtensionProperties> supported_extensions;
        std::vector<vk::LayerProperties>     supported_layers;

        std::tie(result, supported_extensions) = vk::enumerateInstanceExtensionProperties();
        VK_CHECK(result, "Failed to enumerate instance extensions");

        std::tie(result, supported_layers) = vk::enumerateInstanceLayerProperties();
        VK_CHECK(result, "Failed to enumerate instance layers");

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

            if (!found) return false;
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

                return false;
            }
        }

        return true;
    }
}

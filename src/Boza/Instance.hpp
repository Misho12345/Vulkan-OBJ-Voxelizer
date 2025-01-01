#pragma once
#include "pch.hpp"

namespace boza
{
    class Instance final
    {
    public:
        DELETE_CONSTRUCTORS(Instance);

        [[nodiscard]]
        static bool create(const std::string_view& app_name);
        static void destroy();

        STATIC_GETTER_CREF(vk_instance);

    private:
        [[nodiscard]]
        static bool check_extensions_and_layers_support(
            const std::span<const char*>& extensions,
            const std::span<const char*>& layers);

        [[nodiscard]] static bool create_vk_instance(const std::string_view& app_name);

        #ifndef NDEBUG
        [[nodiscard]]
        static bool create_debug_messenger();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
            VkDebugUtilsMessageTypeFlagsEXT             message_type,
            const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
            void*                                       user_data);

        inline static vk::DebugUtilsMessengerEXT debug_messenger;
        #endif

        inline static vk::Instance vk_instance;
    };
}

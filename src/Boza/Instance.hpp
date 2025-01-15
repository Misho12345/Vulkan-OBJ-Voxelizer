#pragma once
#include "pch.hpp"

namespace boza
{
    class Instance final
    {
    public:
        Instance(nullptr_t) noexcept {}
        explicit Instance(const std::string_view& app_name);
        ~Instance();

        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        Instance(Instance&& other) noexcept;
        Instance& operator=(Instance&& other) noexcept;

        operator bool() const noexcept { return ok; }

        [[nodiscard]]
        const vk::Instance& get() const noexcept { return *instance; }

    private:
        [[nodiscard]] bool create_vk_instance(const std::string_view& app_name);
        [[nodiscard]] bool check_extensions_and_layers_support(
            const std::span<const char*>& extensions,
            const std::span<const char*>& layers);

        vk::UniqueInstance instance{ nullptr };

        #ifndef NDEBUG
        [[nodiscard]]
        bool create_debug_messenger();
        vk::UniqueDebugUtilsMessengerEXT debug_messenger{ nullptr };
        #endif

        bool ok = false;
    };
}
#pragma once
#include "pch.hpp"

namespace boza
{
    class CommandPool final
    {
    public:
        DELETE_CONSTRUCTORS(CommandPool);

        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static std::optional<vk::CommandBuffer> allocate_command_buffer();
        [[nodiscard]] static std::vector<vk::CommandBuffer> allocate_command_buffers(uint32_t count);

        STATIC_GETTER_CREF(command_pool);
        STATIC_GETTER_CREF(command_buffer);

    private:
        [[nodiscard]] static bool create_command_pool();
        [[nodiscard]] static bool create_command_buffer();

        inline static vk::CommandPool   command_pool;
        inline static vk::CommandBuffer command_buffer;
    };
}

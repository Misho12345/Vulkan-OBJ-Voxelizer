#pragma once

namespace boza
{
    class Device final
    {
    public:
        DELETE_CONSTRUCTORS(Device);

        [[nodiscard]]
        static bool create();
        static void destroy();

        STATIC_GETTER_CREF(physical_device);
        STATIC_GETTER_CREF(logical_device);
        STATIC_GETTER_CREF(compute_queue);
        STATIC_GETTER_CREF(compute_queue_family_index);

    private:
        [[nodiscard]] static bool choose_physical_device();
        [[nodiscard]] static bool create_logical_device();

        inline static vk::PhysicalDevice physical_device;
        inline static vk::Device         logical_device;
        inline static vk::Queue          compute_queue;
        inline static uint32_t           compute_queue_family_index{};
    };
}

#pragma once
#include <source_location>

constexpr std::string get_location(const std::source_location location = std::source_location::current())
{
    return std::format("{} -> {}: line {}", location.file_name(), location.function_name(), location.line());
}

#define ERROR(...) boza::Logger::error("{}: \"{}\"", get_location(), __VA_ARGS__)
#define VK_CHECK_RET_CUSTOM(EXPR, RET, ...) do { if ((EXPR) != vk::Result::eSuccess) { ERROR(__VA_ARGS__); return RET; } } while (0)
#define VK_CHECK(EXPR, ...)                 VK_CHECK_RET_CUSTOM(EXPR, false, __VA_ARGS__)
#define VK_CHECK_RET_OPT(EXPR, ...)         VK_CHECK_RET_CUSTOM(EXPR, std::nullopt, __VA_ARGS__)

#define BOZA_CHECK_RET_CUSTOM(EXPR, RET, ...) do { if (!(EXPR)) { ERROR(__VA_ARGS__); return RET; } } while (0)
#define BOZA_CHECK(EXPR, ...)         BOZA_CHECK_RET_CUSTOM(EXPR, false, __VA_ARGS__)
#define BOZA_CHECK_RET_OPT(EXPR, ...) BOZA_CHECK_RET_CUSTOM(EXPR, std::nullopt, __VA_ARGS__)

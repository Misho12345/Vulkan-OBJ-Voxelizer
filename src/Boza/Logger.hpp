#pragma once
#include "pch.hpp"

namespace boza
{
	template <typename T>
	concept formattable = requires(T t)
	{
	    { std::format("{}", t) } -> std::same_as<std::string>;
	};

    class Logger final
    {
    public:
      	Logger() = delete;
        ~Logger() = delete;

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        static void trace   (const formattable auto& value[[maybe_unused]]) { DEBUG_ONLY(log(Level::Trace, "{}", value)); }
        static void debug   (const formattable auto& value[[maybe_unused]]) { DEBUG_ONLY(log(Level::Debug, "{}", value)); }
        static void info    (const formattable auto& value) { log(Level::Info    , "{}", value); }
        static void warn    (const formattable auto& value) { log(Level::Warn    , "{}", value); }
        static void error   (const formattable auto& value) { log(Level::Error   , "{}", value); }
        static void critical(const formattable auto& value) { log(Level::Critical, "{}", value); }

        template<formattable... Args> static void trace   (const std::format_string<Args...> format[[maybe_unused]], Args&&... args[[maybe_unused]]) { DEBUG_ONLY(log(Level::Trace, format, std::forward<Args>(args)...)); }
        template<formattable... Args> static void debug   (const std::format_string<Args...> format[[maybe_unused]], Args&&... args[[maybe_unused]]) { DEBUG_ONLY(log(Level::Debug, format, std::forward<Args>(args)...)); }
        template<formattable... Args> static void info    (const std::format_string<Args...> format, Args&&... args) { log(Level::Info    , format, std::forward<Args>(args)...); }
        template<formattable... Args> static void warn    (const std::format_string<Args...> format, Args&&... args) { log(Level::Warn    , format, std::forward<Args>(args)...); }
        template<formattable... Args> static void error   (const std::format_string<Args...> format, Args&&... args) { log(Level::Error   , format, std::forward<Args>(args)...); }
        template<formattable... Args> static void critical(const std::format_string<Args...> format, Args&&... args) { log(Level::Critical, format, std::forward<Args>(args)...); }


    private:
		enum class Level
		{
			Trace, Debug,
			Info,  Warn,
			Error, Critical
		};

        [[nodiscard]]
    	static constexpr const char* to_string(const Level level)
		{
			switch (level)
			{
			case Level::Trace:    return DEBUG_ONLY("\033[90m") 	 "trace"	DEBUG_ONLY("\033[0m");
			case Level::Debug:    return DEBUG_ONLY("\033[36m") 	 "debug"	DEBUG_ONLY("\033[0m");
			case Level::Info:     return DEBUG_ONLY("\033[32m") 	 "info"	 	DEBUG_ONLY("\033[0m");
			case Level::Warn:     return DEBUG_ONLY("\033[33m") 	 "warn"	 	DEBUG_ONLY("\033[0m");
			case Level::Error:    return DEBUG_ONLY("\033[31m") 	 "error"	DEBUG_ONLY("\033[0m");
			case Level::Critical: return DEBUG_ONLY("\033[1;37;41m") "critical" DEBUG_ONLY("\033[0m");
			}

        	assert(false);
			std::unreachable();
		}

		[[nodiscard]] static std::mutex& get_mutex();
        [[nodiscard]] static std::string& get_time();

        template<formattable... Args>
        static void log(const Level level, const std::format_string<Args...> format, Args&&... args)
        {
			std::lock_guard lock(get_mutex());

            const std::string args_str = std::format(format, std::forward<Args>(args)...);
	        const std::string message = std::format("[{}] [{}] {}", get_time(), to_string(level), args_str);

			std::cout << message << '\n';
            if (level >= Level::Warn) std::cout.flush();
        }

    	inline static bool created = false;
    };
}

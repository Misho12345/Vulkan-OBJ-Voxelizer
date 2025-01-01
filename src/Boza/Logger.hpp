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
	    DELETE_CONSTRUCTORS(Logger);

        static void create();
        static void destroy();

        static void trace   (const formattable auto& value) { DEBUG_ONLY(log(Level::eTrace, "{}", value)); }
        static void debug   (const formattable auto& value) { DEBUG_ONLY(log(Level::eDebug, "{}", value)); }
        static void info    (const formattable auto& value) { log(Level::eInfo    , "{}", value); }
        static void warn    (const formattable auto& value) { log(Level::eWarn    , "{}", value); }
        static void error   (const formattable auto& value) { log(Level::eError   , "{}", value); }
        static void critical(const formattable auto& value) { log(Level::eCritical, "{}", value); }

        template<formattable... Args> static void trace   (const std::format_string<Args...> format, Args&&... args) { DEBUG_ONLY(log(Level::eTrace, format, std::forward<Args>(args)...)); }
        template<formattable... Args> static void debug   (const std::format_string<Args...> format, Args&&... args) { DEBUG_ONLY(log(Level::eDebug, format, std::forward<Args>(args)...)); }
        template<formattable... Args> static void info    (const std::format_string<Args...> format, Args&&... args) { log(Level::eInfo    , format, std::forward<Args>(args)...); }
        template<formattable... Args> static void warn    (const std::format_string<Args...> format, Args&&... args) { log(Level::eWarn    , format, std::forward<Args>(args)...); }
        template<formattable... Args> static void error   (const std::format_string<Args...> format, Args&&... args) { log(Level::eError   , format, std::forward<Args>(args)...); }
        template<formattable... Args> static void critical(const std::format_string<Args...> format, Args&&... args) { log(Level::eCritical, format, std::forward<Args>(args)...); }


    private:
		enum class Level
		{
			eTrace,
			eDebug,
			eInfo,
			eWarn,
			eError,
			eCritical
		};

        [[nodiscard]]
    	static constexpr const char* to_string(const Level level)
		{
			#ifdef NDEBUG
			switch (level)
			{
			case Level::eTrace:    return "trace";
			case Level::eDebug:    return "debug";
			case Level::eInfo:     return "info";
			case Level::eWarn:     return "warn";
			case Level::eError:    return "error";
			case Level::eCritical: return "critical";
			}
        	#else
			switch (level)
			{
			case Level::eTrace:    return "\033[90mtrace\033[0m";
			case Level::eDebug:    return "debug";
			case Level::eInfo:     return "\033[32minfo\033[0m";
			case Level::eWarn:     return "\033[33mwarn\033[0m";
			case Level::eError:    return "\033[31merror\033[0m";
			case Level::eCritical: return "\033[31mcritical\033[0m";
			}
        	#endif

        	assert(false);
			std::unreachable();
		}

        [[nodiscard]] static std::string& get_time();
        [[nodiscard]] static std::ostream& get_output();
		[[nodiscard]] static std::mutex& get_mutex();

        template<formattable... Args>
        static void log(const Level level, const std::format_string<Args...> format, Args&&... args)
        {
        	assert(created);
			std::lock_guard lock(get_mutex());

            const std::string args_str = std::format(format, std::forward<Args>(args)...);
	        const std::string message = std::format("[{}] [{}] {}", get_time(), to_string(level), args_str);

			get_output() << message << '\n';
            if (level >= Level::eError) get_output().flush();
        }

    	inline static bool created = false;
    };
}

#include "Logger.hpp"

namespace boza
{
    static std::string time;
    static std::mutex time_mutex;

    std::string& Logger::get_time()
    {
    	std::lock_guard lock(time_mutex);

    	const auto now = std::chrono::system_clock::now();
    	std::time_t current_time = std::chrono::system_clock::to_time_t(now);

    	std::tm time_info{};
    	#ifdef _WIN32
    	localtime_s(&time_info, &current_time);
    	#else
    	localtime_r(&current_time, &time_info);
    	#endif

    	std::ostringstream oss;
    	oss << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S");
    	time = oss.str();
    	return time;
    }

    static std::mutex logging_mutex;
    std::mutex& Logger::get_mutex() { return logging_mutex; }
}

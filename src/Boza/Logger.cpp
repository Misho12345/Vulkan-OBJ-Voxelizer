#include "pch.hpp"
#include "Logger.hpp"

namespace boza
{
    static std::ostream* output{ nullptr };
    static std::string   time;
	#ifdef NDEBUG
	static std::ofstream file;
	#endif

    static std::mutex logging_mutex;
    static std::mutex time_mutex;

    static std::thread flush_thread;


    void Logger::create()
    {
		assert(!created);
    	created = true;

        #ifndef NDEBUG
        output = &std::cout;
        #else
		if (!std::filesystem::create_directory("logs"))
		{
			const auto now = std::chrono::system_clock::now();
			const auto cutoff_time = now - std::chrono::hours(24 * 15);

			std::vector<std::filesystem::path> log_files;

			for (const auto& entry : std::filesystem::directory_iterator("logs"))
				if (entry.is_regular_file() && entry.path().extension() == ".log")
					log_files.push_back(entry.path());

			for (const auto& file : log_files)
				if (std::chrono::time_point<std::chrono::system_clock>(last_write_time(file).time_since_epoch()) < cutoff_time)
					std::filesystem::remove(file);

			std::ranges::sort(log_files, [](const auto& a, const auto& b)
			{
				return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
			});

			while (log_files.size() > 5)
			{
				std::filesystem::remove(log_files.front());
				log_files.erase(log_files.begin());
			}
		}


		std::string log_file = std::format("logs/{}.log", get_time());
		std::ranges::replace(log_file, ':', '_');

		file.open(log_file, std::ios::out | std::ios::app);

		if (file.is_open()) output = &file;
		else throw std::runtime_error("Failed to create a log file");

		output = &file;
        #endif

        flush_thread = std::thread([]
        {
            while (true)
            {
            	using namespace std::chrono_literals;
                std::this_thread::sleep_for(3s);

                std::lock_guard lock(logging_mutex);
                if (output == nullptr) break;
                output->flush();
            }
        });
    }

    void Logger::destroy()
    {
    	assert(created);

        output = nullptr;
        flush_thread.join();

        #ifdef NDEBUG
        if (file.is_open()) file.close();
        #endif
    }


    std::string& Logger::get_time()
    {
        std::lock_guard lock(time_mutex);

        const auto        now          = std::chrono::system_clock::now();
        const std::time_t current_time = std::chrono::system_clock::to_time_t(now);
        std::tm*          time_info    = std::localtime(&current_time);

        time = std::format(
            "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
            time_info->tm_year + 1900,
            time_info->tm_mon + 1,
            time_info->tm_mday,
            time_info->tm_hour,
            time_info->tm_min,
            time_info->tm_sec);

        return time;
    }


    std::ostream& Logger::get_output()
    {
    	assert(output != nullptr);
	    return *output;
    }

    std::mutex& Logger::get_mutex() { return logging_mutex; }
}

#include "Log.hpp"
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>

namespace d3156
{
    bool getBoolEnv(const char *env, const bool def)
    {
        if (const char *val = getenv(env)) return std::strncmp(val, "true", 5) == 0;
        return def;
    }

    u_int16_t getFromEnv(const char *env, const u_int16_t def)
    {
        if (const char *val = getenv(env)) {
            try {
                return static_cast<u_int16_t>(std::stoul(val));
            } catch (...) {
            }
        }
        return def;
    }

    enum class OutType { CONSOLE, FILE };

    OutType getOutType()
    {
        const char *val = getenv("OUT");
        return (val && std::strncmp(val, "FILE", 5) == 0) ? OutType::FILE : OutType::CONSOLE;
    }

    static std::string FORMAT  = getenv("FORMAT") ? getenv("FORMAT") : "|{date:%H:%M:%S}|{source}| {message}";
    static OutType OUT         = getOutType();
    static std::string OUT_DIR = getenv("OUT_DIR") ? getenv("OUT_DIR") : "./logs";
    static std::atomic<bool> PER_SOURCE_FILES = getBoolEnv("PER_SOURCE_FILES", false);
    static std::atomic<u_int16_t> R_LEVEL     = getFromEnv("R_LEVEL", 1);
    static std::atomic<u_int16_t> Y_LEVEL     = getFromEnv("Y_LEVEL", 1);
    static std::atomic<u_int16_t> G_LEVEL     = getFromEnv("G_LEVEL", 1);
    static std::atomic<u_int16_t> W_LEVEL     = getFromEnv("W_LEVEL", 1);

    bool LoggerManager::allowed(const LogType type, const int level) noexcept
    {
        switch (type) {
            case LogType::RED: return level <= R_LEVEL;
            case LogType::YELLOW: return level <= Y_LEVEL;
            case LogType::GREEN: return level <= G_LEVEL;
            case LogType::WHITE: return level <= W_LEVEL;
        }
        return false;
    }

    namespace
    {
        class LoggerImpl
        {
        public:
            LoggerImpl()
            {
                constexpr int width = 80;
                std::cout << "\033[1;32mLogger Settings from process Environment\033[0m" << std::endl;
                std::cout << std::string(width, '-') << std::endl;
                std::cout << "\033[34mFORMAT\033[0m           : " << FORMAT << "\033[32m # allow {source}, {file}, {line}, {message}, {date:format strftime}, {level}\033[0m" << std::endl;
                std::cout << "\033[34mOUT\033[0m              : " << (OUT == OutType::FILE ? "FILE" : "CONSOLE") << " \033[32m# allow FILE and CONSOLE\033[0m" << std::endl;
                std::cout << "\033[34mOUT_DIR\033[0m          : " << OUT_DIR << std::endl;
                std::cout << "\033[34mPER_SOURCE_FILES\033[0m : " << PER_SOURCE_FILES << " \033[32m# Save logs in files OUT_DIR/{source}.log\033[0m" << std::endl;
                std::cout << "\033[34mR_LEVEL\033[0m          : " << R_LEVEL << std::endl;
                std::cout << "\033[34mY_LEVEL\033[0m          : " << Y_LEVEL << std::endl;
                std::cout << "\033[34mG_LEVEL\033[0m          : " << G_LEVEL << std::endl;
                std::cout << "\033[34mW_LEVEL\033[0m          : " << W_LEVEL << std::endl;
                std::cout << std::string(width, '=') << std::endl;
                if (OUT == OutType::FILE && !PER_SOURCE_FILES) {
                    std::filesystem::create_directories(OUT_DIR);
                    common_file_.open(OUT_DIR + "/common.log", std::ios::app);
                }
            }

            void log(LogType type, int level, const char *file, int line, const char *source,
                     const std::string &message) noexcept
            {
                try {
                    std::ostringstream oss;
                    std::string formatted = FORMAT;
                    // --- handle {date:...} ---
                    std::regex date_regex(R"(\{date:(.*?)\})");
                    std::smatch match;
                    auto f_start = formatted.cbegin();
                    while (std::regex_search(f_start, formatted.cend(), match, date_regex)) {
                        auto now = std::chrono::system_clock::now();
                        auto t_c = std::chrono::system_clock::to_time_t(now);
                        std::tm tm{};
                        localtime_r(&t_c, &tm);

                        char buf[64];
                        strftime(buf, sizeof(buf), match[1].str().c_str(), &tm);
                        formatted.replace(match.position(0), match.length(0), buf);
                        f_start = formatted.cbegin() + match.position(0) + std::strlen(buf);
                    }

                    if (OUT == OutType::CONSOLE) {
                        std::string color;
                        switch (type) {
                            case LogType::RED: color = "\033[31m"; break;
                            case LogType::YELLOW: color = "\033[33m"; break;
                            case LogType::GREEN: color = "\033[32m"; break;
                            case LogType::WHITE: color = "\033[0m"; break;
                        }
                        replace_all(formatted, "{source}", color + source + "\033[0m");
                    } else
                        replace_all(formatted, "{source}", source);
                    // --- replace other placeholders ---
                    replace_all(formatted, "{file}", file);
                    replace_all(formatted, "{line}", std::to_string(line));
                    replace_all(formatted, "{message}", message);
                    replace_all(formatted, "{level}", std::to_string(level));

                    // --- output ---
                    if (OUT == OutType::CONSOLE) {
                        std::cout << formatted << std::endl;
                    } else {
                        std::ofstream *out_file = getFileStream(source);
                        std::lock_guard<std::mutex> lock(file_mutex_);
                        (*out_file) << formatted << std::endl;
                    }
                } catch (...) {
                    std::cout << "Logger error";
                }
            }

        private:
            std::ofstream *getFileStream(const char *source)
            {
                if (!PER_SOURCE_FILES) return &common_file_;
                std::lock_guard<std::mutex> lock(file_mutex_);
                auto it = file_streams_.find(source);
                if (it != file_streams_.end()) return &it->second;
                auto &ofs        = file_streams_[source];
                std::string path = OUT_DIR + "/" + source + ".log";
                ofs.open(path, std::ios::app);
                return &ofs;
            }

            static void replace_all(std::string &str, const std::string &from, const std::string &to)
            {
                size_t start = 0;
                while ((start = str.find(from, start)) != std::string::npos) {
                    str.replace(start, from.length(), to);
                    start += to.length();
                }
            }

            std::mutex file_mutex_;
            std::ofstream common_file_;
            std::unordered_map<std::string, std::ofstream> file_streams_;
        };
    }

    void LoggerManager::log(const LogType type, const int level, const char *file, const int line, const char *source,
                            std::string &&message) noexcept
    {
        static LoggerImpl impl;
        impl.log(type, level, file, line, source, message);
    }
}

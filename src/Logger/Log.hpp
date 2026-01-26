#pragma once
#include <cstdint>
#include <string>
#include <sstream>

#ifndef LOG_NAME
#define LOG_NAME "UNKNOWN_SOURCE"
#endif

/*FORMAT="|{date:%H:%M:%S}|{source}|{file}|{message}" # allow {source}, {file}, {message}, {date:format strftime}, {line}, {level}
OUT=CONSOLE                                         # allow CONSOLE, FILE
OUT_DIR=./logs/                                     # if OUT=FILE
PER_SOURCE_FILES=true
R_LEVEL=2
Y_LEVEL=2
G_LEVEL=2
W_LEVEL=2*/

#define LOG_IMPL(TYPE, LEVEL, STREAM)                                                                                  \
    do {                                                                                                               \
        if (!d3156::LoggerManager::allowed(TYPE, LEVEL)) break;                                             \
        std::ostringstream oss;                                                                                        \
        oss << STREAM;                                                                                                 \
        d3156::LoggerManager::log(TYPE, LEVEL, __FILE__, __LINE__, LOG_NAME, oss.str());                    \
    } while (0)

#ifdef NO_LOG
#define LOG(level, stream)                                                                                             \
    do {                                                                                                               \
    } while (0)
#else
#define LOG(level, stream) LOG_IMPL(d3156::LogType::WHITE, level, stream)
#endif

#ifdef NO_GLOG
#define G_LOG(level, stream)                                                                                           \
    do {                                                                                                               \
    } while (0)
#else
#define G_LOG(level, stream) LOG_IMPL(d3156::LogType::GREEN, level, stream)
#endif

#ifdef NO_YLOG
#define Y_LOG(level, stream)                                                                                           \
    do {                                                                                                               \
    } while (0)
#else
#define Y_LOG(level, stream) LOG_IMPL(d3156::LogType::YELLOW, level, stream)
#endif

#ifdef NO_RLOG
#define R_LOG(level, stream)                                                                                           \
    do {                                                                                                               \
    } while (0)
#else
#define R_LOG(level, stream) LOG_IMPL(d3156::LogType::RED, level, stream)
#endif

namespace d3156
{
    enum class LogType : uint8_t { WHITE, RED, GREEN, YELLOW };
    namespace LoggerManager
    {
        void log(LogType type, int level, const char *file, int line, const char *source, std::string &&stream) noexcept;
        bool allowed(LogType type, int level) noexcept;
    };

}
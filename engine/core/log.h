#pragma once

#include <fmt/core.h>
#include <ctime>

enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

#ifndef _WIN32
inline static void __debugbreak(void)
{
    asm volatile("int $0x03");
}
#endif

struct Logger {
    const char *level_strings[6] = {
            "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
    };
    const char *level_colors[6] = {
        "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
    };
    FILE* file = nullptr;

    void init(const char* filename) {
        file = fopen(filename, "w");
        if (file == nullptr) {
            fmt::print("Cannot create log file {}!\n", filename);
        }
    }

    void release() {
        if (file) {
            fclose(file);
        }
    }

    template <class... Args>
    void log(int level, const char *filename, int line, std::string_view fmtstr, Args&&... args) {
        char buf[16];
        time_t cur_time = time(NULL);
        auto cur_localtime = localtime(&cur_time);
        buf[strftime(buf, sizeof(buf), "%H:%M:%S", cur_localtime)] = '\0';
        fmt::print("{} {}{:5s}\x1b[0m \x1b[90m{}:{}:\x1b[0m ",
                   buf, level_colors[level], level_strings[level],
                   filename, line);
        fmt::print(fmtstr, std::forward<Args>(args)...);
        fmt::print("\n");
        fflush(stdout);

        if (file) {
            char buf_big[64];
            buf[strftime(buf_big, sizeof(buf_big), "%Y-%m-%d %H:%M:%S", cur_localtime)] = '\0';
            fmt::print(file, "{} {:5s} {}:{}: ", buf_big, level_strings[level], filename, line);
            fmt::print(file, fmtstr, std::forward<Args>(args)...);
            fmt::print(file, "\n");
            fflush(stdout);
        }
    }

    void log_raw(int level, const char* filename, int line, std::string_view str) {
        char buf[16];
        time_t cur_time = time(NULL);
        auto cur_localtime = localtime(&cur_time);
        buf[strftime(buf, sizeof(buf), "%H:%M:%S", cur_localtime)] = '\0';
        fmt::print("{} {}{:5s}\x1b[0m \x1b[90m{}:{}:\x1b[0m {}\n",
                   buf, level_colors[level], level_strings[level],
                   filename, line,
                   str);
        fflush(stdout);

        if (file) {
            char buf_big[64];
            buf[strftime(buf_big, sizeof(buf_big), "%Y-%m-%d %H:%M:%S", cur_localtime)] = '\0';
            fmt::print(file, "{} {:5s} {}:{}: {}\n", buf_big, level_strings[level], filename, line, str);
            fflush(stdout);
        }
    }

};

extern Logger g_logger;

#define log_trace(...) g_logger.log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) g_logger.log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  g_logger.log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  g_logger.log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)

#ifndef NDEBUG
#define log_error(...) g_logger.log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__); __debugbreak()
#define log_fatal(...) g_logger.log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__); __debugbreak()
#else
#define log_error(...) g_logger.log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__);
#define log_fatal(...) g_logger.log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__);
#endif

#define log_assert(cond, ...) if (!(cond)) { g_logger.log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__); __debugbreak(); }

extern void log_init(const char* filename);
extern void log_release();

#include <stdarg.h>

#define AssertError(value, errmsg, ...)                                   \
    if (!(value)) {                                                       \
        printf("[" __FILE__ "] " errmsg "\n" __VA_OPT__(, ) __VA_ARGS__); \
        fflush(stdout);                                                   \
        Breakpoint;                                                       \
    }

#define Error(errmsg, ...)                                                \
    do {                                                                  \
        printf("[" __FILE__ "] " errmsg "\n" __VA_OPT__(, ) __VA_ARGS__); \
        fflush(stdout);                                                   \
        Breakpoint;                                                       \
    } while (0)

#define ParserError(p, errmsg, ...)                                       \
    do {                                                                  \
        print_at(p);                                                      \
        printf("[" __FILE__ "] " errmsg "\n" __VA_OPT__(, ) __VA_ARGS__); \
        fflush(stdout);                                                   \
        Breakpoint;                                                       \
    } while (0)

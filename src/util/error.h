#include <stdarg.h>

#define AssertError(value, errmsg, ...)                            \
    if (!(value)) {                                                \
        printf("[" __FILE__ "]" errmsg__VA_OPT__(, ) __VA_ARGS__); \
        exit(1);                                                   \
    }

#define Error(errmsg, ...)                                     \
    do {                                                            \
        printf("[" __FILE__ "]" errmsg __VA_OPT__(, ) __VA_ARGS__); \
        exit(1);                                                    \
    } while (0)

#define ParserError(p, errmsg, ...) \
    do {\
        print_at(p);\
        printf("[" __FILE__ "]" errmsg __VA_OPT__(, ) __VA_ARGS__); \
            exit(1);\
    } while(0)


